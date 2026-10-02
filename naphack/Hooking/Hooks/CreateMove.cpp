#include "../Hooked.hpp"
#include "../../SDK/Displacement.hpp"
#include "../../SDK/Classes/Player.hpp"
#include "../../SDK/Classes/weapon.hpp"
#include "../../Features/Rage/EnginePrediction.hpp"
#include <intrin.h>
#include "../../Features/Rage/Ragebot.hpp"
#include "../../Utils/InputSys.hpp"
#include "../../Features/Rage/FakePing.hpp"
#include "../../SDK/Classes/Exploits.hpp"
#include "../../Features/Rage/Animations.hpp"
#include "../../Utils/Threading/threading.h"
#include "../../Features/Rage/Resolver.hpp"
#include <thread>
#include "../../Features/Rage/TickbaseShift.hpp"
#include "../../Features/Visuals/EventLogger.hpp"
#include "../../Features/Miscellaneous/Movement.hpp"
#include "../../Features/Rage/ServerAnimations.hpp"
#include "../../Features/Rage/FakeLag.hpp"
#include "../../Features/Rage/AntiAim.hpp"
#include "../../Features/Miscellaneous/Communication.hpp"
#include "../../Features/Miscellaneous/Miscellaneous.hpp"

#include "../../Features/Scripting/Scripting.hpp"
#include "../../Features/Rage/Knifebot.hpp"
#include "../../Features/Rage/DormantAimbot.hpp"
#include "../../Features/Visuals/Visuals.hpp"

//extern int g_AUTOWALL_CALLS;

#ifdef DEV // need it for debugging :)
bool CreateMoveHandler( void *ecx, float ft, CUserCmd *_cmd, Encrypted_t<bool> bSendPacket ) {
#else
__forceinline bool CreateMoveHandler( void *ecx, float ft, CUserCmd * _cmd, Encrypted_t<bool> bSendPacket ) {
#endif
	auto bResult = Hooked::oCreateMove( ecx, ft, _cmd );

	g_Vars.globals.m_bDidRagebot = false;

	const float flCurrentForwardMove = _cmd->forwardmove;
	const float flCurrentSideMove = _cmd->sidemove;

	//g_AUTOWALL_CALLS = 0;

	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal || pLocal->IsDead( ) ) {
		g_Vars.globals.m_bShotAutopeek = false;
		g_Movement.m_vecAutoPeekPos.Init( );

		g_TickbaseController.UnCharge( );

		// g_Ragebot.m_vecLastSkippedPlayers.clear( );
		g_Prediction.reset( );
		g_Vars.globals.m_bInCreateMove = false;
		return bResult;
	}

	auto pWeapon = reinterpret_cast< C_WeaponCSBaseGun * >( pLocal->m_hActiveWeapon( ).Get( ) );
	if( !pWeapon ) {
		g_Prediction.reset( );
		g_Vars.globals.m_bInCreateMove = false;

		return bResult;
	}

	auto pWeaponData = pWeapon->GetCSWeaponData( ).Xor( );
	if( !pWeaponData ) {
		g_Prediction.reset( );
		g_Vars.globals.m_bInCreateMove = false;

		return bResult;
	}

	g_TickbaseController.m_pWeapon = pWeapon;
	g_TickbaseController.m_pWeaponData = pWeaponData;

	//auto pVerifiedCmd = &g_pInput->m_pVerifiedCommands[ sequence_number % MULTIPLAYER_BACKUP ];
	Encrypted_t<CUserCmd> cmd( _cmd );
	if( !cmd.IsValid( ) ) {
		g_Vars.globals.m_bInCreateMove = false;
		return bResult;
	}

	if( g_Vars.globals.m_bDisableInput ) {
		// just looks nicer
		auto RemoveButtons = [ & ] ( int key ) { cmd->buttons &= ~key; };

		RemoveButtons( IN_ATTACK );
		RemoveButtons( IN_ATTACK2 ); // don't want to scope in with menu open

		if( GUI::ctx->typing ) {
			RemoveButtons( IN_MOVERIGHT );
			RemoveButtons( IN_MOVELEFT );
			RemoveButtons( IN_FORWARD );
			RemoveButtons( IN_BACK );
		}
	}

	// automatic weapons doeee
	if( g_Vars.rage.enabled || ( g_Vars.rage.exploit && ( g_Vars.rage.hide_shots_bind.enabled || g_Vars.rage.double_tap_bind.enabled ) ) ) {
		if( pWeaponData ) {
			if( cmd->buttons & IN_ATTACK
				&& pWeapon->m_iItemDefinitionIndex( ) != WEAPON_C4
				&& pWeapon->m_iItemDefinitionIndex( ) != WEAPON_REVOLVER
				&& pWeaponData->m_iWeaponType >= WEAPONTYPE_KNIFE
				&& pWeaponData->m_iWeaponType <= WEAPONTYPE_MACHINEGUN
				&& !pLocal->CanShoot( ) )
				cmd->buttons &= ~IN_ATTACK;
		}
	}

	// applies 4 zeus aswell
	g_Vars.globals.m_bHoldingKnife = pWeaponData->m_iWeaponType == WEAPONTYPE_KNIFE;

	g_Movement.PrePrediction( cmd.Xor( ), bSendPacket.Xor( ) );

	// moved to paint, it doesn't work perfectly on cmove..
	// g_Misc.ForceCrosshair( );

	bool bDontCharge = false;

	// we sent the last one, we should be breaking again
	if( g_TickbaseController.m_nTicksCharged && g_TickbaseController.m_nLCShiftAmount == g_TickbaseController.m_iMaxProcessTicks && g_Vars.globals.m_bSentPreviousTick && g_Vars.rage.double_tap_bind.enabled ) {
		g_TickbaseController.m_bBreakingLC = true;
	}

	// ONLY hide shots enabled but controller thinks we still break lc?
	// reset it.
	if( g_Vars.rage.hide_shots_bind.enabled && !g_Vars.rage.double_tap_bind.enabled && g_TickbaseController.m_bBreakingLC ) {
		g_TickbaseController.m_bBreakingLC = false;
	}

	static auto nUnduckedTicks = 0;
	static bool bWasDucked = false;

	// HAHAHAHHAAHHAHA
	// DELAY SHOT WHEN UNDUCK SO EYEPOS IS NOT FUCKED :D
	if( g_TickbaseController.IsCharged( ) && g_Vars.rage.exploit && ( ( g_Vars.rage.hide_shots_bind.enabled && !g_Vars.rage.double_tap_bind.enabled ) ) ) {
		bool bDelay = false;

		bool bUnducking = ( !( cmd->buttons & IN_DUCK ) && pLocal->m_flDuckAmount( ) > 0.0f );
		if( pLocal->m_flDuckAmount( ) && !bUnducking ) {
			bWasDucked = true;
			nUnduckedTicks = 0;
		}
		else {
			if( nUnduckedTicks < 14 ) {
				bDelay = true;
				++nUnduckedTicks;
			}
			else {
				bWasDucked = false;
			}
		}

		g_TickbaseController.m_bDelayAimbot = bDelay;
	}
	else {
		g_TickbaseController.m_bDelayAimbot = false;
	}

	// printf( "%i\n", *( int * )( uintptr_t( pLocal ) + 0x103FC ) );
	// https://i.imgur.com/IfH2oIY.png clean
	// g_Vars.globals.m_bBuyMenuOpen = *( bool * )( uintptr_t( pLocal ) + 0x103FC );

	/*if( g_Vars.misc.time_shift_key.enabled && g_Vars.misc.time_shift && g_FakePing.m_bCanTimeShift ) {
		auto pNetChannel = g_pEngine->GetNetChannelInfo( );
		if( pNetChannel ) {
			for( int i = 0; i < 128; ++i ) {
				pNetChannel->SendDatagram( nullptr );
			}
		}

		printf( "shifted time\n" );
	}*/

	const float nFrameRate = ( 1.f / g_TickbaseController.m_flFramerate );
	const float nTickRate = float( 1.f / g_pGlobalVars->interval_per_tick );

	// if ur fps drops before 64*2
	if( nFrameRate < nTickRate ) {
		g_Ragebot.m_flLimitTime = 10.f;
		g_Ragebot.m_flLimitTargets = g_pGlobalVars->realtime + g_Ragebot.m_flLimitTime;
	}

	g_Ragebot.m_flFrameRateMultiplier = std::clamp( ( g_Ragebot.m_flLimitTargets - g_pGlobalVars->realtime ) / g_Ragebot.m_flLimitTime, 0.f, 1.0f );

	g_Prediction.start( pLocal, cmd.Xor( ) );
	{
		bool bCanShift = g_TickbaseController.CanShift( false, true ) && g_TickbaseController.m_bBreakingLC && g_Vars.rage.exploit && g_Vars.rage.double_tap_bind.enabled;
		const auto nBackupTickbase = pLocal->m_nTickBase( );

		// make aimbot run with "correct" tickbase...
		if( bCanShift ) {
			pLocal->m_nTickBase( ) += g_TickbaseController.GetCorrection( );
		}


		/*auto pPredictedData = g_Prediction.get_networked_vars( cmd->command_number );
		g_Vars.globals.m_flInaccuracy = pPredictedData->inaccuracy;

		static float flLastInaccuracy = g_Vars.globals.m_flInaccuracy;
		bool bShooting = cmd->buttons & IN_ATTACK && pLocal->CanShoot( );

		if( !bShooting ) {
			flLastInaccuracy = g_Vars.globals.m_flInaccuracy;
		}
		else {
			printf( "inaccuracy: %.6f\n", flLastInaccuracy );
		}*/
		 
		// lal dont need to do it faster than this anyway
		if( !( cmd->tick_count % 5 ) && !g_TickbaseController.m_bTeleportUncharge )
			g_Vars.globals.m_flPenetrationDamage = Autowall::GetPenetrationDamage( pLocal, pWeapon );

		g_AntiAim.m_bHasOverriden = false;
		g_FakeLag.HandleFakeLag( bSendPacket, cmd );

		if( g_Ragebot.m_bSendNextCommand ) {
			if( !g_Vars.globals.m_bFakeWalking )
				*bSendPacket.Xor( ) = true;

			g_Ragebot.m_bSendNextCommand = false;
		}

		// the choke decision for this command is settled now, so the tick this command will
		// arrive on is finally known. record validity is judged against it, so refresh it
		// before the ragebot walks the history.
		g_Animations.UpdateArrivalTick( );

		if( !g_TickbaseController.m_bTeleportUncharge && !g_TickbaseController.m_bShifting && !g_TickbaseController.m_bDelay )
			g_Movement.DefuseTheBomb( cmd.Xor( ) );

		// run aimbot.
		for( int i = 1; i <= g_pGlobalVars->maxClients; i++ ) {
			if( i == g_pEngine->GetLocalPlayer( ) )
				continue;

			auto player = C_CSPlayer::GetPlayerByIndex( i );
			if( !player || !player->IsPlayer( ) )
				continue;

			auto lag_data = g_Animations.GetAnimationEntry( i );
			if( !lag_data )
				continue;

			lag_data->m_backupRecord.SetupRecord( player, true );
		}

		g_Ragebot.think( bSendPacket.Xor( ), cmd.Xor( ) );

		for( int i = 0; i <= 64; ++i ) {
			if( g_Visuals.vecAimpoints[ i ].size( ) ) {
				g_Visuals.vecAimpointsSane[ i ] = std::move( g_Visuals.vecAimpoints[ i ] );
				g_Visuals.flLastPointsUpdate[ i ] = g_pGlobalVars->realtime;
			}

			g_Visuals.vecAimpoints[ i ].clear( );

			if( fabsf( g_Visuals.flLastPointsUpdate[ i ] - g_pGlobalVars->realtime ) > 0.2f )
				g_Visuals.vecAimpointsSane[ i ].clear( );
		}

		for( int i = 1; i <= g_pGlobalVars->maxClients; i++ ) {
			if( i == g_pEngine->GetLocalPlayer( ) )
				continue;

			auto player = C_CSPlayer::GetPlayerByIndex( i );
			if( !player || !player->IsPlayer( ) )
				continue;

			auto lag_data = g_Animations.GetAnimationEntry( i );
			if( !lag_data )
				continue;

			lag_data->m_backupRecord.ApplyRecord( player );
		}

		g_Movement.InPrediction( cmd.Xor( ) );

		g_DormantAimbot.Run( );

		if( !bDontCharge ) {
			if( !g_TickbaseController.m_bTeleportUncharge && !g_TickbaseController.m_bShifting )
				bDontCharge = g_Knifebot.Run( bSendPacket.Xor( ), cmd.Xor( ) );
		}

		g_ServerAnimations.m_uRenderAnimations.m_bDoingRealFlick =
			g_ServerAnimations.m_uRenderAnimations.m_bDoingPreFlick = false;
		g_AntiAim.Think( cmd.Xor( ), bSendPacket.Xor( ), bSendPacket.Xor( ) );

		if( cmd->buttons & IN_ATTACK
			&& pWeapon->m_iItemDefinitionIndex( ) != WEAPON_C4
			&& pWeaponData->m_iWeaponType >= WEAPONTYPE_KNIFE
			&& pWeaponData->m_iWeaponType <= WEAPONTYPE_MACHINEGUN
			&& pLocal->CanShoot( ) ) {

			if( g_TickbaseController.IsCharged( ) && g_TickbaseController.CanShift( ) && !g_Vars.rage.double_tap_bind.enabled && g_Vars.rage.hide_shots_bind.enabled ) {
				g_Vars.globals.m_bShotWhileHiding = true;
			}

			bDontCharge = true;
		}
		else {
			if( !g_Vars.globals.m_bShotWhileHiding )
				g_ServerAnimations.m_angPreviousAngle = cmd->viewangles;
		}

		if( g_Vars.globals.m_bSentPreviousTick && g_Vars.globals.m_bShotWhileHidingTicks > 1 ) {
			g_Vars.globals.m_bShotWhileHiding = false;

			g_Vars.globals.m_bShotWhileHidingTicks = 0;
		}


	#if defined(LUA_SCRIPTING)
		Scripting::Script::DoCallback( hash_32_fnv1a_const( XorStr( "predicted_move" ) ), cmd.Xor( ) );
	#endif

		g_TickbaseController.RunExploits( bSendPacket, cmd );

		// fix animations after all movement related functions have been called
		g_ServerAnimations.HandleAnimations( bSendPacket.Xor( ), cmd.Xor( ) );

		// s/o estk
		if( g_TickbaseController.m_bShifting && !g_TickbaseController.m_bTeleportUncharge ) {
			cmd->buttons &= ~0x801u;
		}

		if( ( ( cmd->buttons & IN_ATTACK ) || g_Vars.globals.m_bDidRagebot || ( ( cmd->buttons & IN_ATTACK2 ) && pWeaponData->m_iWeaponType == WEAPONTYPE_KNIFE && pWeapon->m_iItemDefinitionIndex( ) != WEAPON_ZEUS && pWeapon->m_iItemDefinitionIndex( ) != WEAPON_HEALTHSHOT ) )
			&& pWeapon->m_iItemDefinitionIndex( ) != WEAPON_C4
			&& pWeaponData->m_iWeaponType >= WEAPONTYPE_KNIFE
			&& pWeaponData->m_iWeaponType <= WEAPONTYPE_MACHINEGUN
			&& pLocal->CanShoot( ) ) {

			g_TickbaseController.m_flLastExploitTime = g_pGlobalVars->realtime;

			if( *bSendPacket.Xor( ) == false || g_pClientState->m_nChokedCommands( ) > 0 )
				g_Vars.globals.m_bShotWhileChoking = true;

			g_Vars.globals.m_flLastShotRealtime = g_pGlobalVars->realtime;

			//g_AntiAim.HandleShotAntiAim( bSendPacket, cmd.Xor( ) );

			g_ServerAnimations.m_angChokedShotAngle = cmd->viewangles;

			g_Vars.globals.m_bShotAutopeek = pWeaponData->m_iWeaponType != WEAPONTYPE_KNIFE;

			bDontCharge = true;
		}

		if( pWeaponData->m_iWeaponType == WEAPONTYPE_GRENADE ) {
			if( !pWeapon->m_bPinPulled( ) || ( cmd->buttons & ( IN_ATTACK | IN_ATTACK2 ) ) ) {
				float m_fThrowTime = pWeapon->m_fThrowTime( );
				if( m_fThrowTime > 0.f ) {
					if( *bSendPacket.Xor( ) == false || g_pClientState->m_nChokedCommands( ) > 0 )
						g_Vars.globals.m_bShotWhileChoking = true;

					g_ServerAnimations.m_angChokedShotAngle = cmd->viewangles;
				}
			}
		}

		if( bCanShift ) {
			pLocal->m_nTickBase( ) = nBackupTickbase;
		}

		g_Vars.globals.m_iWeaponIndex = pWeapon->m_iItemDefinitionIndex( );
		g_Vars.globals.m_flLastDuckAmount = pLocal->m_flDuckAmount( );
	}
	g_Prediction.end( pLocal );

	//if( cmd->buttons & IN_ATTACK && pLocal->CanShoot( ) ) {
	//	if( auto pNetChannel = g_pEngine->GetNetChannelInfo( ); pNetChannel ) {
	//		// start at current server tick
	//		const int nStartTick = g_pClientState->m_ClockDriftManager( ).m_nServerTick + 1;

	//		// try to predict when our shot will be registered
	//		int nArrivalTick = nStartTick;

	//		// account for latency
	//		int nServerLatencyTicks = TIME_TO_TICKS( pNetChannel->GetLatency( FLOW_OUTGOING ) );
	//		int nClientLatencyTicks = TIME_TO_TICKS( pNetChannel->GetLatency( FLOW_INCOMING ) );
	//		nArrivalTick += nServerLatencyTicks /*+ nClientLatencyTicks*/;

	//		// account for our current choke cycle
	//		int nChokeCycle = std::max( g_FakeLag.m_iAwaitingChoke, 0 ) - g_pClientState->m_nChokedCommands( );
	//		if( g_FakeLag.m_iAwaitingChoke > 1 ) {
	//			nArrivalTick += std::max( nChokeCycle, 0 );
	//		}

	//		printf( "\nfired shot at tick %i:\n\toutgoing latency ticks: %i\n\tincoming latency ticks: %i\n\tchoke cycle accounting: %i\n\tpredicted arrival tick: %i\n",
	//				nStartTick,
	//				nServerLatencyTicks,
	//				nClientLatencyTicks,
	//				g_FakeLag.m_iAwaitingChoke > 1 ? std::max( nChokeCycle, 0 ) : -1337,
	//				nArrivalTick );
	//	}

	//}

	//if( g_TickbaseController.m_bDoZeroMove ) {
	//	cmd->sidemove = cmd->forwardmove = 0.f;
	//}

	g_Movement.PostPrediction( cmd.Xor( ) );

	// we don't want this to be sent.
	if( g_TickbaseController.m_bShifting /*|| g_TickbaseController.m_bTeleportUncharge */ || ( g_TickbaseController.m_bDelayDont && g_TickbaseController.m_bShiftMove ) ) {
		*bSendPacket.Xor( ) = false;

		g_TickbaseController.m_bDelay = false;
	}

	if( g_TickbaseController.m_bTeleportUncharge && g_Vars.globals.m_bFakeWalking ) {
		cmd->sidemove = cmd->forwardmove = 0.f;
	}

	if( g_TickbaseController.m_nTicksAfterUncharge >= 1 && !g_TickbaseController.IsCharged( ) ) {
		if( g_TickbaseController.m_nTicksAfterUncharge < 14 )
			++g_TickbaseController.m_nTicksAfterUncharge;
	}
	else {
		g_TickbaseController.m_nTicksAfterUncharge = 0;
	}

	// maybe we do though :)
	if( g_TickbaseController.m_bFORCESEND ) {
		// gotta send this :)
		*bSendPacket.Xor( ) = true;

		// we were not breaking lc (this means we have to fix uncharge)
		if( !g_TickbaseController.m_bBreakingLC && g_TickbaseController.m_bTeleportUncharge && ( g_TickbaseController.m_nTicksCharged > 1 || g_TickbaseController.m_nChargeType != ChargeType::CHARGE_DOUBLETAP ) ) {
			g_TickbaseController.m_vecTickbaseFix.push_back( { cmd->command_number, g_TickbaseController.m_iMaxProcessTicks, false, true } );
		}

		++g_TickbaseController.m_nTicksAfterUncharge;

		// reset it.
		g_TickbaseController.m_bFORCESEND = false;

		if( g_TickbaseController.m_nBreakLCCorrection >= 0 && g_TickbaseController.m_nBreakLCCorrection <= 6 )
			g_TickbaseController.m_vecTickbaseFix.push_back( { cmd->command_number, g_TickbaseController.m_iMaxProcessTicks - g_TickbaseController.m_nBreakLCCorrection, false, true } );
	}

	if( g_Vars.globals.m_bShotWhileHiding ) {
		g_Vars.globals.m_bShotWhileHidingTicks++;
	}

	// everytime fps drops below tickrate we want to recharge a tick :)
	// this essentially fixes stuff like when being charged and tabbing out of game
	// should also fix other issues
	// though idk, people with bad fps (not like their game already feels like shit)
	// will maybe notice stutters due to the recharge..
	if( g_TickbaseController.m_flFramerate && g_pGlobalVars->interval_per_tick && !bDontCharge && !g_Vars.globals.m_bIsGameFocused && g_TickbaseController.m_iProcessTicks && !g_TickbaseController.m_bPerformDefensive ) {
		static ConVar *fps_max = g_pCVar->FindVar( XorStr( "fps_max" ) );
		if( fps_max ) {
			const int nTickrate = std::max( int( 1.0f / g_pGlobalVars->interval_per_tick ), 64 );

			// user issue if this is below tickrate
			// fps_max sucks in this game, doesn't really limit fps to the amount u put there
			if( !fps_max->GetInt( ) || ( fps_max->GetInt( ) - 6 ) >= nTickrate ) {
				if( ( 1 / g_TickbaseController.m_flFramerate ) < nTickrate ) {
					if( g_TickbaseController.m_nAllowLowFpsRecharge >= 64 ) {
						g_EventLog.PushEvent( XorStr( "forced recharge [framerate]" ), Color_f( 1.f, 0.f, 0.f ), false );

						g_TickbaseController.m_bForceRecharge = true;
						g_TickbaseController.UnCharge( false );
						g_TickbaseController.Charge( );
					}
				}
				else {
					if( g_TickbaseController.m_nAllowLowFpsRecharge < 64 )
						++g_TickbaseController.m_nAllowLowFpsRecharge;
				}
			}
		}
	}

	//const auto pNetChannel = g_pEngine->GetNetChannelInfo( );
	//if( pNetChannel && g_TickbaseController.IsCharged( ) && !g_TickbaseController.m_bForceRecharge ) {
	//	const float flIncomingLoss = pNetChannel->GetAvgLoss( FLOW_INCOMING );

	//	if( g_TickbaseController.m_flLastLoss == 0.f ) {
	//		g_TickbaseController.m_bAllowLossRecharge = true;
	//	}

	//	bool bLoss = flIncomingLoss < g_TickbaseController.m_flLastLoss;

	//	g_TickbaseController.m_flLastLoss = flIncomingLoss;

	//	if( bLoss && !g_Vars.rage.double_tap_recharge_shot_delay && g_TickbaseController.m_bAllowLossRecharge && !g_TickbaseController.m_bPerformDefensive ) {
	//		g_EventLog.PushEvent( XorStr( "forced recharge [packet loss]" ), Color_f( 1.f, 0.f, 0.f ), false );

	//		// maybe put these into one func
	//		g_TickbaseController.m_bForceRecharge = true;
	//		g_TickbaseController.UnCharge( false );
	//		g_TickbaseController.Charge( );

	//		g_TickbaseController.m_bAllowLossRecharge = false;
	//	}
	//}

	if( !( *bSendPacket.Xor( ) ) ) {
		//g_AntiAim.m_iLastChokedCommandNumber = cmd->command_number;
	}

	if( g_TickbaseController.IsCharged( ) ) {
		if( g_TickbaseController.m_nTicksCharged < 14 )
			++g_TickbaseController.m_nTicksCharged;
	}

	if( g_TickbaseController.m_nTicksCharged > 0 && *bSendPacket.Xor( ) ) {
		g_TickbaseController.m_bSentFirstAfterCharge = true;
	}

	g_Vars.globals.m_bSentPreviousTick = ( *bSendPacket.Xor( ) );

	cmd->viewangles.Clamp( );

	// Run anti-aim, animate localplayer, ...
	if( *bSendPacket.Xor( ) ) {
		g_Vars.globals.m_bShotWhileChoking = false;
		//g_AntiAim.m_nShotCommandNumber = 0;

		g_ServerAnimations.m_uServerAnimations.m_bBreakingTeleportDst = ( pLocal->m_vecOrigin( ) - g_ServerAnimations.m_uServerAnimations.m_vecLastOrigin ).LengthSquared( ) > 4096.f;
		g_ServerAnimations.m_uServerAnimations.m_vecLastOrigin = pLocal->m_vecOrigin( );
	}

	if( g_Vars.globals.m_bDidRagebot && !g_Movement.m_bModifiedMovementBeforePrediction ) {
		_cmd->forwardmove = flCurrentForwardMove;
		_cmd->sidemove = flCurrentSideMove;
	}

	// сервер выкидывает ВСЮ команду, если tick_count дальше sv_max_usercmd_future_ticks от его тика.
	// m_nServerTick (последний снапшот) отстаёт от реального тика сервера, так что запас гарантирован.
	const int nMaxTickCount = static_cast< int >( g_pClientState->m_ClockDriftManager( ).m_nServerTick ) + g_Vars.sv_max_usercmd_future_ticks->GetInt( );
	if( _cmd->tick_count > nMaxTickCount )
		_cmd->tick_count = nMaxTickCount;

	//std::memcpy( &pVerifiedCmd->m_cmd, cmd.Xor( ), 0x64 );
	//pVerifiedCmd->m_crc = cmd->GetChecksum( );

	return false;
}

bool __fastcall Hooked::CreateMove( void *ecx, void *, float ft, CUserCmd * _cmd ) {
	g_Vars.globals.m_bInCreateMove = true;
	auto pLocal = C_CSPlayer::GetLocalPlayer( );

	g_Vars.globals.szLastHookCalled = XorStr( "2" );
	if( !_cmd || !_cmd->command_number ) {
		g_Vars.globals.m_bInCreateMove = false;
		return oCreateMove( ecx, ft, _cmd );
	}

	bool *bSendPacketPtr = reinterpret_cast< bool * >( uintptr_t( ( uintptr_t * )_AddressOfReturnAddress( ) ) + 0x14 );

	auto bSendPacket = Encrypted_t<bool>( bSendPacketPtr );

	if( !pLocal || g_pClientState->m_nDeltaTick( ) == -1 || !bSendPacket.Xor( ) ) {
		g_TickbaseController.UnCharge( );
		g_Prediction.last_outgoing_commands.clear( );

		g_Vars.globals.m_bInCreateMove = false;
		return oCreateMove( ecx, ft, _cmd );;
	}

	// was for testing but yuh
	g_TickbaseController.m_pCurrentCmd = _cmd;
	g_TickbaseController.m_pCurrentPacket = bSendPacket.Xor( );

	// if we here, we not charging anymore FOR SURE
	g_TickbaseController.m_bInCharge = false;

	g_Misc.PreserveKillfeed( );

	if( !( *bSendPacket.Xor( ) ) )
		*bSendPacket.Xor( ) = true;

	auto result = CreateMoveHandler( ecx, ft, _cmd, bSendPacket );

	auto net_channel = ( INetChannel * )g_pClientState->net_channel;
	if( net_channel ) {
		if( !*bSendPacket.Xor( ) && net_channel->m_nChokedPackets > 0 ) {
			g_Prediction.fake_datagram = true;

			const auto m_nChokedPackets = net_channel->m_nChokedPackets;

			net_channel->m_nChokedPackets = 0;
			net_channel->SendDatagram( );
			--net_channel->m_nOutSequenceNr;
			net_channel->m_nChokedPackets = m_nChokedPackets;
		}
		else
			g_Prediction.last_outgoing_commands.emplace_back( _cmd->command_number );

		g_Prediction.fake_datagram = false;
	}

	g_Vars.globals.m_bInCreateMove = false;

	return result;
}

bool __cdecl Hooked::ReportHit( Hit_t * hit ) {
	return oReportHit( hit );
}

bool __cdecl Hooked::IsUsingStaticPropDebugMode( ) {
	return false;
}

void __vectorcall Hooked::CL_Move( bool bFinalTick, float accumulated_extra_samples ) {
	g_Prediction.ReadPackets( bFinalTick );
	g_Prediction.update( );
	//g_Prediction.update_ping_values( bFinalTick );

	return Hooked::oCL_Move( bFinalTick, accumulated_extra_samples );


#if 0

	// :/ not sure how to perfectly fix prediction with this
#if 0
	if( auto pLocal = C_CSPlayer::GetLocalPlayer( ); pLocal /*&& !pLocal->IsDead( )*/ ) {
		using CL_ReadPackets_t = void( __thiscall * )( bool final_tick );
		// this is _Host_RunFrame_Client not CL_ReadPackets ("53 8A D9 8B 0D ? ? ? ? 56 57 8B B9")
		static CL_ReadPackets_t  CL_ReadPackets = ( CL_ReadPackets_t )( Engine::Displacement.Function.m_CLReadPackets );

		// note - michal;
		// really shit and gay but this is a way better solution XD
		// and by just setting clock correction and restoring cl_clockcorrection,
		// the viewmodel and shit gets fucked when moving ur mouse (jitters)
		// https://i.imgur.com/KJLeGvw.png https://streamable.com/3mf5d4
		auto clockDriftManager = &g_pClientState->m_ClockDriftManager( );
		if( clockDriftManager ) {
			auto m_iCurClockOffset = clockDriftManager->m_iCurClockOffset;
			auto m_nServerTick = clockDriftManager->m_nServerTick;
			auto m_nClientTick = clockDriftManager->m_nClientTick;

			auto realtime = g_pGlobalVars->realtime;
			auto framecount = g_pGlobalVars->framecount;
			auto absoluteframetime = g_pGlobalVars->absoluteframetime;
			auto absoluteframestarttimestddev = g_pGlobalVars->absoluteframestarttimestddev;
			auto curtime = g_pGlobalVars->curtime;
			auto frametime = g_pGlobalVars->frametime;
			auto maxClients = g_pGlobalVars->maxClients;
			auto tickcount = g_pGlobalVars->tickcount;
			auto interval_per_tick = g_pGlobalVars->interval_per_tick;
			auto interpolation_amount = g_pGlobalVars->interpolation_amount;
			auto simTicksThisFrame = g_pGlobalVars->simTicksThisFrame;

			const auto nTickbase = pLocal->m_nTickBase( );

			CL_ReadPackets( bFinalTick );

			pLocal->m_nTickBase( ) = nTickbase;

			if( clockDriftManager ) {
				clockDriftManager->m_iCurClockOffset = m_iCurClockOffset;
				clockDriftManager->m_nServerTick = m_nServerTick;
				clockDriftManager->m_nClientTick = m_nClientTick;
			}

			g_pGlobalVars->realtime = realtime;
			g_pGlobalVars->framecount = framecount;
			g_pGlobalVars->absoluteframetime = absoluteframetime;
			g_pGlobalVars->absoluteframestarttimestddev = absoluteframestarttimestddev;
			g_pGlobalVars->curtime = curtime;
			g_pGlobalVars->frametime = frametime;
			g_pGlobalVars->maxClients = maxClients;
			g_pGlobalVars->tickcount = tickcount;
			g_pGlobalVars->interval_per_tick = interval_per_tick;
			g_pGlobalVars->interpolation_amount = interpolation_amount;
			g_pGlobalVars->simTicksThisFrame = simTicksThisFrame;
		}

		if( clockDriftManager ) {
			const auto nTickbase = pLocal->m_nTickBase( );

			// update prediction for newly achieved data :)
			g_Prediction.RunGamePrediction( );

			pLocal->m_nTickBase( ) = nTickbase;
		}
	}
#endif
	// g_Prediction.UpdatePing( bFinalTick );

	//g_TickbaseController.CL_Move( bFinalTick, accumulated_extra_samples );
#endif
}
