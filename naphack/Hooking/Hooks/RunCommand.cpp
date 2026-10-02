#include "../Hooked.hpp"
#include "../../Features/Rage/EnginePrediction.hpp"
#include "../../SDK/Classes/weapon.hpp"
#include "../../SDK/Valve/CBaseHandle.hpp"
#include "../../SDK/Classes/Player.hpp"
#include "../../SDK/Displacement.hpp"
#include <deque>
#include "../../Features/Rage/TickbaseShift.hpp"


int nTickbaseRecords[ 150 ] = { };
bool bInAttackRecords[ 150 ] = { };
bool bCanShootRecords[ 150 ] = { };

void __fastcall Hooked::RunCommand( void* ecx, void* edx, C_CSPlayer* player, CUserCmd* ucmd, IMoveHelper* moveHelper ) {
	g_Vars.globals.szLastHookCalled = XorStr( "32" );

	C_CSPlayer* pLocal = C_CSPlayer::GetLocalPlayer( );

	if( !pLocal || !player || player != pLocal || !ucmd ) {
		oRunCommand( ecx, player, ucmd, moveHelper );
		return;
	}

	if( ucmd->tick_count > ( g_pGlobalVars->tickcount + g_Vars.sv_max_usercmd_future_ticks->GetInt( ) + 64 ) ) {
		return;
	}

	nTickbaseRecords[ ucmd->command_number % 150 ] = player->m_nTickBase( );
	bInAttackRecords[ ucmd->command_number % 150 ] = ( ucmd->buttons & ( IN_ATTACK2 | IN_ATTACK ) ) != 0;
	bCanShootRecords[ ucmd->command_number % 150 ] = player->CanShoot( true );

	// sure let it stay, whatever
	auto FixPostponeTime = [ player ] ( int command_number ) {
		auto pWeapon = ( C_WeaponCSBaseGun* )player->m_hActiveWeapon( ).Get( );
		if( pWeapon ) {
			auto postpone = FLT_MAX;
			if( pWeapon->m_iItemDefinitionIndex( ) == WEAPON_REVOLVER ) {
				auto tick_rate = 64;
				if( g_pGlobalVars->interval_per_tick ) {
					tick_rate = int( 1.0f / g_pGlobalVars->interval_per_tick );
				}

				if( tick_rate >> 1 > 1 ) {
					auto cmd_nr = command_number - 1;
					auto shoot_nr = 0;
					for( int i = 1; i < tick_rate >> 1; ++i ) {
						shoot_nr = cmd_nr;
						if( !bInAttackRecords[ cmd_nr % 150 ] || !bCanShootRecords[ cmd_nr % 150 ] )
							break;

						--cmd_nr;
					}

					if( shoot_nr && g_pGlobalVars->interval_per_tick ) {
						auto tick = 1 - ( signed int )( float )( -0.03348f / g_pGlobalVars->interval_per_tick );
						if( command_number - shoot_nr >= tick )
							postpone = TICKS_TO_TIME( nTickbaseRecords[ ( tick + shoot_nr ) % 150 ] ) + 0.2f;
					}
				}
				pWeapon->m_flPostponeFireReadyTime( ) = postpone; 
			}
		}
	};

	FixPostponeTime( ucmd->command_number );
	oRunCommand( ecx, player, ucmd, moveHelper );
	FixPostponeTime( ucmd->command_number );

	g_Prediction.update_viewmodel_info( ucmd );
	g_Prediction.store_netvars( ucmd->command_number );

	static auto vphysics_state = Engine::g_PropManager.GetOffset( XorStr( "DT_BasePlayer" ), XorStr( "m_vphysicsCollisionState" ) );
	*( int * )( uintptr_t( player ) + vphysics_state ) = 0;
}
