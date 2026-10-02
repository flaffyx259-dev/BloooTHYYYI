#include "EnginePrediction.hpp"
#include "../../SDK/Classes/Player.hpp"
#include "../../SDK/Classes/weapon.hpp"
#include "../../pandora.hpp"
#include "../../SDK/Valve/CBaseHandle.hpp"
#include "../../SDK/displacement.hpp"
#include "../Rage/TickbaseShift.hpp"
#include "../../Hooking/Hooked.hpp"
#include <deque>
#include "../Rage/FakeLag.hpp"
#include "../../Loader/Exports.h"
#include "../Visuals/Visuals.hpp"

c_engine_prediction g_Prediction;

// welcome back to 2017
int EstimateTickbase( CUserCmd *cmd ) {
	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal || !cmd )
		return -1;

	static int nEstimatedTickbase = 0;
	static CUserCmd *pLastCmd = nullptr;

	if( !pLastCmd || pLastCmd->hasbeenpredicted ) {
		nEstimatedTickbase = pLocal->m_nTickBase( );
	}
	else {
		// required because prediction only runs on frames, not ticks
		// so if your framerate goes below tickrate, m_nTickBase won't update every tick
		++nEstimatedTickbase;
	}

	pLastCmd = cmd;

	return nEstimatedTickbase;
}

void c_engine_prediction::update( ) {
	if( g_pClientState->signon_state < 6 )
		return;

	if( g_Vars.globals.m_eLastStage == ClientFrameStage_t::FRAME_NET_UPDATE_END )
		g_pPrediction->Update( g_pClientState->delta_tick,
							   g_pClientState->delta_tick > 0,
							   g_pClientState->last_command_ack,
							   g_pClientState->last_outgoing_command + g_pClientState->choked_commands );
}

class c_movedata {
public:
	bool run_out_of_functions : 1;
	bool game_code_moved_player : 1;
	int player_handle;
	int impulse_command;
	Vector view_angles;
	Vector abs_view_angles;
	int buttons;
	int old_buttons;
	float forward_move;
	float side_move;
	float up_move;
	float max_speed;
	float client_max_speed;
	Vector velocity;
	Vector angles;
	Vector old_angle;
	float step_height;
	Vector wish_vel;
	Vector jump_vel;
	Vector constraint_center;
	float constraint_radius;
	float constraint_width;
	float constraint_speed_factor;
	bool constratint_past_radius;
	Vector abs_origin;
};

void c_engine_prediction::start( C_CSPlayer *pLocal, CUserCmd *pCmd ) {
	if( !pLocal || !pCmd ) {
		reset( );
		return;
	}

	in_prediction = true;

	auto old_tickbase = pLocal->m_nTickBase( );

	int nCustomTickbase = 0;

	// correct tickbase.
	if( !m_pLastCmd || m_pLastCmd->hasbeenpredicted )
		m_iSeqDiff = pCmd->command_number - pLocal->m_nTickBase( );

	m_pLastCmd = pCmd;

	nCustomTickbase = MAX( pLocal->m_nTickBase( ), pCmd->command_number - m_iSeqDiff );

	unpred_vars.store( pLocal );
	initial_vars.store( pLocal, pCmd );

	restore_vars[ pCmd->command_number % 150 ].store( pLocal, pCmd );

	g_pGlobalVars->curtime = TICKS_TO_TIME( nCustomTickbase );
	g_pGlobalVars->frametime = pLocal->m_fFlags( ) & 0x40/*fl_frozen*/ ? 0.f : g_pGlobalVars->interval_per_tick;

	C_BaseEntity::SetPredictionRandomSeed( MD5_PseudoRandom( pCmd->command_number ) & 0x7FFFFFFF );
	C_BaseEntity::SetPredictionPlayer( pLocal );

	g_pPrediction->m_bInPrediction = true;
	g_pPrediction->m_bIsFirstTimePredicted = false;

	static c_movedata movedata{};
	std::memset( &movedata, 0, sizeof( c_movedata ) );

	auto &net_vars = networked_vars[ pCmd->command_number % 150 ];
	net_vars.ground_entity = pLocal->m_hGroundEntity( );

	g_pGameMovement->StartTrackPredictionErrors( pLocal );
	g_pMoveHelper->SetHost( pLocal );

	*( CUserCmd ** )( ( std::uintptr_t )pLocal + 0x3314 ) = pCmd;
	*( CUserCmd * )( ( std::uintptr_t )pLocal + 0x326C ) = *pCmd;

	const auto buttons = pCmd->buttons;
	int buttonsChanged = buttons ^ *reinterpret_cast< int * >( uintptr_t( pLocal ) + 0x31E8 );
	*reinterpret_cast< int * >( uintptr_t( pLocal ) + 0x31DC ) = ( uintptr_t( pLocal ) + 0x31E8 );
	*reinterpret_cast< int * >( uintptr_t( pLocal ) + 0x31E8 ) = buttons;
	*reinterpret_cast< int * >( uintptr_t( pLocal ) + 0x31E0 ) = buttons & buttonsChanged;  // m_afButtonPressed ~ The changed ones still down are "pressed"
	*reinterpret_cast< int * >( uintptr_t( pLocal ) + 0x31E4 ) = buttonsChanged & ~buttons; // m_afButtonReleased ~ The ones not down are "released"

	g_pPrediction->CheckMovingGround( pLocal, g_pGlobalVars->frametime );
	g_pPrediction->SetLocalViewAngles( pCmd->viewangles );

	pLocal->PreThink( );
	pLocal->Think( );

	g_pMoveHelper->SetHost( pLocal );
	g_pPrediction->SetupMove( pLocal, pCmd, g_pMoveHelper.Xor( ), &movedata );
	g_pGameMovement->ProcessMovement( pLocal, ( CMoveData * )&movedata );
	g_pPrediction->FinishMove( pLocal, pCmd, &movedata );

	g_pMoveHelper->ProcessImpacts( );

	pLocal->RunPostThink( );

	g_pGameMovement->FinishTrackPredictionErrors( pLocal );
	g_pMoveHelper->SetHost( nullptr );

	net_vars.store( pLocal, pCmd );

	if( auto pWeapon = ( C_WeaponCSBaseGun * )pLocal->m_hActiveWeapon( ).Get( ); pWeapon ) {
		pWeapon->UpdateAccuracyPenalty( );

		if( auto pData = pWeapon->GetCSWeaponData( ); pData.IsValid( ) ) {
			auto weapon_id = pWeapon->m_iItemDefinitionIndex( );

			auto is_special_weapon = pWeapon->m_weaponMode( ) != 0;

			ideal_inaccuracy = 0.f;

			if( weapon_id == WEAPON_SSG08 && !( pLocal->m_fFlags( ) & FL_ONGROUND ) )
				ideal_inaccuracy = 0.00875f;
			else if( pLocal->m_fFlags( ) & FL_DUCKING ) {
				if( is_special_weapon )
					ideal_inaccuracy = pData->m_flInaccuracyCrouchAlt;
				else
					ideal_inaccuracy = pData->m_flInaccuracyCrouch;
			}
			else if( is_special_weapon )
				ideal_inaccuracy = pData->m_flInaccuracyStandAlt;
			else
				ideal_inaccuracy = pData->m_flInaccuracyStand;
		}
	}

	pLocal->m_nTickBase( ) = old_tickbase;
}

void c_engine_prediction::end( C_CSPlayer *pLocal ) {
	if( !pLocal ) {
		reset( );
		return;
	}

	in_prediction = false;

	*( CUserCmd ** )( ( std::uintptr_t )pLocal + 0x3314 ) = nullptr;

	C_BaseEntity::SetPredictionRandomSeed( -1 );
	C_BaseEntity::SetPredictionPlayer( 0 );

	unpred_vars.restore( pLocal );

	g_pGameMovement->Reset( );
}

bool c_engine_prediction::available( ) {
	if( !g_pEngine->IsConnected( ) || !g_pEngine->IsInGame( ) )
		return false;

	auto net_chan = g_pEngine->GetNetChannelInfo( );
	if( !net_chan )
		return false;

	if( net_chan->IsLoopback( ) )
		return false;

	return true;
}

bool c_engine_prediction::should_reduce_ping( ) {
	if( !available( ) )
		return false;

	ping_backup.restore( );
	return true;
}

// note - michal;
// todo, why is this fucked here but not in airflow
// this is rly important/good to have cos it decreases ping and improves hitreg
void c_engine_prediction::update_ping_values( bool final_tick ) {
	if( !available( ) )
		return;

	ping_backup_t backup;
	backup.store( );

	Hooked::oCL_ReadPackets( final_tick );
	ping_backup.store( );

	backup.restore( );
}

void c_engine_prediction::fix_netvars( int tick ) {
	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	pred_error_occured = false;

	auto netvars = &compressed_netvars[ tick % 150 ];
	if( netvars->cmd_number != tick )
		return;

	auto aim_punch_diff = netvars->aimpunch - pLocal->m_aimPunchAngle( );
	auto view_punch_diff = netvars->viewpunch - pLocal->m_viewPunchAngle( );
	auto aim_punch_vel_diff = netvars->aimpunch_vel - pLocal->m_aimPunchAngleVel( );
	auto view_offset_diff = netvars->viewoffset - pLocal->m_vecViewOffset( );
	auto fall_velocity_diff = netvars->fall_velocity - pLocal->m_flFallVelocity( );

	{
		if( abs( aim_punch_diff.x ) <= 0.03125f && abs( aim_punch_diff.y ) <= 0.03125f && abs( aim_punch_diff.z ) <= 0.03125f )
			pLocal->m_aimPunchAngle( ) = netvars->aimpunch;
		else
			pred_error_occured = true;

		if( std::abs( view_punch_diff.x ) <= 0.03125f && std::abs( view_punch_diff.y ) <= 0.03125f && std::abs( view_punch_diff.z ) <= 0.03125f )
			pLocal->m_viewPunchAngle( ) = netvars->viewpunch;
		else
			pred_error_occured = true;

		if( std::abs( aim_punch_vel_diff.x ) <= 0.03125f && std::abs( aim_punch_vel_diff.y ) < 0.03125f && std::abs( aim_punch_vel_diff.z ) <= 0.03125f )
			pLocal->m_aimPunchAngleVel( ) = netvars->aimpunch_vel;
		else
			pred_error_occured = true;

		if( std::abs( view_offset_diff.z ) <= 0.065f )
			pLocal->m_vecViewOffset( ) = netvars->viewoffset;

		if( std::abs( fall_velocity_diff ) <= 0.5f )
			pLocal->m_flFallVelocity( ) = netvars->fall_velocity;
	}
}

void c_engine_prediction::store_netvars( int tick ) {
	auto netvars = &compressed_netvars[ tick % 150 ];
	netvars->store( tick );
}

void c_engine_prediction::update_viewmodel_info( CUserCmd *cmd ) {
	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	auto viewmodel = ( C_BaseViewModel * )( pLocal->m_hViewModel( ).Get( ) );
	if( !viewmodel )
		return;

	auto &current_viewmodel = viewmodel_info[ cmd->command_number % 150 ];
	current_viewmodel.store( cmd, viewmodel );

	if( cmd->weaponselect ) {
		auto previous_command = ( cmd->command_number - 1 );
		auto previous_viewmodel = viewmodel_info[ previous_command % 150 ];

		if( previous_viewmodel.cmd_tick == previous_command && previous_viewmodel.model_index != viewmodel->m_nModelIndex( ) ) {
			auto round_sequence = ( previous_viewmodel.new_sequence_parity + 1 ) & 7;
			if( ( ( round_sequence + 1 ) & 7 ) == current_viewmodel.new_sequence_parity ) {
				current_viewmodel.new_sequence_parity = round_sequence;
				viewmodel->m_nNewSequenceParity( ) = round_sequence;
			}
		}
	}
}

void c_engine_prediction::fix_viewmodel( int stage ) {
	if( stage != FRAME_NET_UPDATE_POSTDATAUPDATE_START )
		return;

	if( !g_pEngine->IsInGame( ) )
		return;

	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	if( !pLocal || pLocal->IsDead( ) )
		return;

	auto pWeapon = ( C_WeaponCSBaseGun * )pLocal->m_hActiveWeapon( ).Get( );
	if( !pWeapon )
		return;

	auto viewmodel = ( C_BaseViewModel * )( pLocal->m_hViewModel( ).Get( ) );
	if( !viewmodel )
		return;

	auto command = g_pClientState->command_ack;
	auto current_viewmodel = &viewmodel_info[ command % 150 ];
	if( current_viewmodel->cmd_tick != command )
		return;

	auto cycle = viewmodel->m_flCycle( );
	auto old_cycle = viewmodel->m_flOldCycle( );
	auto anim_time = viewmodel->m_flAnimTime( );
	auto old_anim_time = viewmodel->m_flOldAnimTime( );

	if( anim_time != old_anim_time
		&& cycle != old_cycle
		&& cycle == 0.f && anim_time == g_pGlobalVars->curtime ) {
		// NOTE: every condition must be && here. the old code had
		// "... model_index == ... || looking_at_weapon == ..." which due to
		// operator precedence evaluated as "(A && B && C && D && E) || F".
		// when not inspecting, looking_at_weapon matches (false == false) so
		// EVERY server-driven restart (draw / slash / inspect) got reverted
		// to m_flOldCycle/m_flOldAnimTime -> weapon appears with no draw
		// animation, knife slashes and F-inspect never play.
		if( current_viewmodel->active_weapon == pWeapon
			&& current_viewmodel->sequence == viewmodel->m_nSequence( )
			&& current_viewmodel->animation_parity == viewmodel->m_nAnimationParity( )
			&& current_viewmodel->new_sequence_parity == viewmodel->m_nNewSequenceParity( )
			&& current_viewmodel->model_index == viewmodel->m_nModelIndex( )
			&& current_viewmodel->looking_at_weapon == pLocal->m_bIsLookingAtWeapon( ) ) {
			viewmodel->m_flAnimTime( ) = viewmodel->m_flOldAnimTime( );
			viewmodel->m_flCycle( ) = viewmodel->m_flOldCycle( );
		}
	}
}

void c_engine_prediction::ping_backup_t::store( ) {
	if( !g_pClientState.Xor( ) )
		return;

	curtime = g_pGlobalVars->curtime;
	frametime = g_pGlobalVars->frametime;
	tickcount = g_pGlobalVars->tickcount;
	cs_tickcount = g_pClientState->old_tickcount;
}

void c_engine_prediction::ping_backup_t::restore( ) {
	if( !g_pClientState.Xor( ) )
		return;

	g_pGlobalVars->curtime = curtime;
	g_pGlobalVars->frametime = frametime;
	g_pGlobalVars->tickcount = tickcount;
	g_pClientState->old_tickcount = cs_tickcount;
}

void c_engine_prediction::compressed_netvars_t::store( int tick ) {
	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	tickbase = pLocal->m_nTickBase( );
	fall_velocity = pLocal->m_flFallVelocity( );
	velocity_modifier = pLocal->m_flVelocityModifier( );
	aimpunch = pLocal->m_aimPunchAngle( );
	viewpunch = pLocal->m_viewPunchAngle( );
	aimpunch_vel = pLocal->m_aimPunchAngleVel( );
	viewoffset = pLocal->m_vecViewOffset( );
	origin = pLocal->m_vecOrigin( );
	base_velocity = pLocal->m_vecBaseVelocity( );
	velocity = pLocal->m_vecVelocity( );
	network_origin = pLocal->m_vecNetworkOrigin( );
	//stamina = pLocal->stamina( );
	cmd_number = tick;

	filled = true;
}


void unpred_vars_t::store( C_CSPlayer *pLocal ) {
	in_prediction = g_pPrediction->m_bInPrediction;
	first_time_predicted = g_pPrediction->m_bIsFirstTimePredicted;

	flags = pLocal->m_fFlags( );
	velocity = pLocal->m_vecVelocity( );

	if( auto pWeapon = ( C_WeaponCSBaseGun * )pLocal->m_hActiveWeapon( ).Get( ); pWeapon ) {
		recoil_index = pWeapon->m_flRecoilIndex( );
		accuracy_penalty = pWeapon->m_fAccuracyPenalty( );
	}

	curtime = g_pGlobalVars->curtime;
	frametime = g_pGlobalVars->frametime;

	ground_entity = pLocal->m_hGroundEntity( );

	// hardcoded because it doesn't parse with netvars
	predicted_cmd = *( CUserCmd ** )( ( std::uintptr_t )pLocal + 0x3314 );
	updated_cmd = *( CUserCmd * )( ( std::uintptr_t )pLocal + 0x326C );
}

void unpred_vars_t::restore( C_CSPlayer *pLocal ) {
	g_pPrediction->m_bInPrediction = in_prediction;
	g_pPrediction->m_bIsFirstTimePredicted = first_time_predicted;

	if( auto pWeapon = ( C_WeaponCSBaseGun * )pLocal->m_hActiveWeapon( ).Get( ); pWeapon ) {
		pWeapon->m_flRecoilIndex( ) = recoil_index;
		pWeapon->m_fAccuracyPenalty( ) = accuracy_penalty;
	}

	g_pGlobalVars->curtime = curtime;
	g_pGlobalVars->frametime = frametime;

	*( CUserCmd ** )( ( std::uintptr_t )pLocal + 0x3314 ) = predicted_cmd;
	*( CUserCmd * )( ( std::uintptr_t )pLocal + 0x326C ) = updated_cmd;
}

void networked_vars_t::store( C_CSPlayer *pLocal, CUserCmd *cmd, bool no_ground_entity ) {
	walking = pLocal->m_bIsWalking( );
	scoped = pLocal->m_bIsScoped( );

	command_number = cmd->command_number;

	forwardmove = cmd->forwardmove;
	sidemove = cmd->sidemove;

	tickbase = pLocal->m_nTickBase( );
	move_state = pLocal->m_iMoveState( );
	move_type = pLocal->m_MoveType( );
	flags = pLocal->m_fFlags( );

	if( auto pWeapon = ( C_WeaponCSBaseGun * )pLocal->m_hActiveWeapon( ).Get( ); pWeapon ) {
		spread = pWeapon->GetSpread( );
		inaccuracy = pWeapon->GetInaccuracy( );

		recoil_index = pWeapon->m_flRecoilIndex( );
		acc_penalty = pWeapon->m_fAccuracyPenalty( );
	}

	lower_body_yaw = pLocal->m_flLowerBodyYawTarget( );
	thirdperson_recoil = pLocal->m_flThirdpersonRecoil( );
	duck_amount = pLocal->m_flDuckAmount( );
	//stamina = pLocal->stamina( );
	fall_velocity = pLocal->m_flFallVelocity( );
	velocity_modifier = pLocal->m_flVelocityModifier( );

	origin = pLocal->m_vecOrigin( );
	abs_origin = pLocal->GetAbsOrigin( );
	velocity = pLocal->m_vecVelocity( );
	viewoffset = pLocal->m_vecViewOffset( );
	aimpunch = pLocal->m_aimPunchAngle( );
	aimpunch_vel = pLocal->m_aimPunchAngleVel( );
	viewpunch = pLocal->m_viewPunchAngle( );
	ladder_normal = pLocal->m_vecLadderNormal( );
	base_velocity = pLocal->m_vecBaseVelocity( );
	network_origin = pLocal->m_vecNetworkOrigin( );

	if( !no_ground_entity )
		ground_entity = pLocal->m_hGroundEntity( );

	viewangles = cmd->viewangles;

	done = true;
}

void networked_vars_t::restore( C_CSPlayer *pLocal, bool animations ) {
	if( !done )
		return;

	if( auto pWeapon = ( C_WeaponCSBaseGun * )pLocal->m_hActiveWeapon( ).Get( ); pWeapon ) {
		pWeapon->m_flRecoilIndex( ) = recoil_index;
		pWeapon->m_fAccuracyPenalty( ) = acc_penalty;
	}

	pLocal->m_flDuckAmount( ) = duck_amount;
	pLocal->m_vecOrigin( ) = origin;
	pLocal->SetAbsOrigin( abs_origin );
	pLocal->m_vecViewOffset( ) = viewoffset;
	pLocal->m_aimPunchAngle( ) = aimpunch;
	pLocal->m_aimPunchAngleVel( ) = aimpunch_vel;
	pLocal->m_viewPunchAngle( ) = viewpunch;

	if( animations ) {
		pLocal->m_bIsWalking( ) = walking;
		pLocal->m_bIsScoped( ) = scoped;

		pLocal->m_flLowerBodyYawTarget( ) = lower_body_yaw;
		pLocal->m_flThirdpersonRecoil( ) = thirdperson_recoil;

		pLocal->m_iMoveState( ) = move_state;
		pLocal->m_MoveType( ) = move_type;

		pLocal->m_hGroundEntity( ) = ground_entity;
		pLocal->m_fFlags( ) = flags;

		pLocal->SetAbsVelocity( velocity );
		pLocal->m_vecVelocity( ) = velocity;
		pLocal->m_vecLadderNormal( ) = ladder_normal;
	}
}

void viewmodel_info_t::store( CUserCmd *cmd, C_BaseViewModel *viewmodel ) {
	cmd_tick = cmd->command_number;

	auto owner = ( C_CSPlayer * )( viewmodel->m_hOwner( ).Get( ) );
	if( owner ) {
		looking_at_weapon = owner->m_bIsLookingAtWeapon( );
		active_weapon = ( C_WeaponCSBaseGun * )( owner->m_hActiveWeapon( ).Get( ) );
	}

	sequence = viewmodel->m_nSequence( );
	animation_parity = viewmodel->m_nAnimationParity( );
	new_sequence_parity = viewmodel->m_nNewSequenceParity( );
	model_index = viewmodel->m_nModelIndex( );

	anim_time = viewmodel->m_flAnimTime( );
	old_anim_time = viewmodel->m_flOldAnimTime( );

	cycle = viewmodel->m_flCycle( );
	old_cycle = viewmodel->m_flOldCycle( );
}

void viewmodel_info_t::reset( ) {
	looking_at_weapon = false;

	cmd_tick = 0;
	sequence = 0;
	animation_parity = 0;
	new_sequence_parity = 0;
	model_index = 0;

	anim_time = 0.f;
	old_anim_time = 0.f;

	cycle = 0.f;
	old_cycle = 0.f;

	active_weapon = nullptr;
}

void c_engine_prediction::ReadPackets( bool bFinal ) {
	/* Check SignonState */
	if( !g_pClientState.IsValid( ) || g_pClientState->signon_state < 6/*SIGNONSTATE_FULL*/ ) {
		m_bReadPackets = true;
		return;
	}

	if( !g_pEngine->IsConnected( ) || !g_pEngine->IsInGame( ) ) {
		m_bReadPackets = true;
		return;
	}

	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal ) {
		m_bReadPackets = true;
		return;
	}

	if( pLocal->IsDead( ) ) {
		m_bReadPackets = true;
		return;
	}

	/* Check netchannel, so skip local servers */
	INetChannel *m_NetChannel = g_pEngine->GetNetChannelInfo( );
	if( !m_NetChannel || m_NetChannel->IsLoopback( ) ) {
		m_bReadPackets = true;
		return;
	}

	/* save players origins */
	std::array < Vector, 65 > m_Origins;
	for( int nPlayer = 1; nPlayer <= 64; nPlayer++ ) {
		C_CSPlayer *Player = C_CSPlayer::GetPlayerByIndex( nPlayer );
		if( !Player || Player->IsDead( ) )
			continue;

		m_Origins[ nPlayer ] = Player->GetAbsOrigin( );
	}

	/* call CL_ReadPackets */
	m_bReadPackets = true;

	Hooked::oCL_ReadPackets( bFinal );

	m_bReadPackets = false;

	/* restore players origins */
	for( int nPlayer = 1; nPlayer <= 64; nPlayer++ ) {
		C_CSPlayer *Player = C_CSPlayer::GetPlayerByIndex( nPlayer );
		if( !Player || Player->IsDead( ) )
			continue;

		Player->SetAbsOrigin( m_Origins[ nPlayer ] );
	}
}