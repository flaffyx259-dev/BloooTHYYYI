#pragma once
#include "FakeLag.hpp"
#include "../Miscellaneous/Movement.hpp"

#include "../../SDK/Classes/Player.hpp"
#include "../../SDK/Classes/weapon.hpp"
#include "../Rage/TickbaseShift.hpp"
#include "ServerAnimations.hpp"
#include "AntiAim.hpp"
#include "EnginePrediction.hpp"
#include "Autowall.hpp"

FakeLag g_FakeLag;

int BreakDuringBodyTimer( Encrypted_t<CUserCmd> pCmd ) {
	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return false;

	if( !g_Vars.rage.anti_aim_fake_body )
		return false;

	if( !g_Vars.rage.anti_aim_fake_body_balance )
		return false;

	const float flVelocityEpsilon = pLocal->m_fFlags( ) & FL_DUCKING ? 3.75f : 1.5f;
	if( !g_Vars.globals.m_bFakeWalking && ( g_Movement.PressingMovementKeys( pCmd.Xor( ) ) || pLocal->m_vecVelocity( ).Length2D( ) > flVelocityEpsilon ) && pLocal->m_fFlags( ) & FL_ONGROUND )
		return false;

	const int nTicksLeftTillUpdate = TIME_TO_TICKS( g_ServerAnimations.m_uServerAnimations.m_flLowerBodyRealignTimer - TICKS_TO_TIME( pLocal->m_nTickBase( ) ) );

	// this will fakelag for 14 ticks, and the choke cycle will end 4 ticks before the lby update 
	if( nTicksLeftTillUpdate >= 4 && nTicksLeftTillUpdate <= 18 ) {
		// printf( "before %i\n", nTicksLeftTillUpdate );
		return 1;
	}

	// this will fakelag for 14 ticks, for the first 4 ticks after u break lby
	if( nTicksLeftTillUpdate <= TIME_TO_TICKS( 1.1f ) - 4 && nTicksLeftTillUpdate >= TIME_TO_TICKS( 1.1f ) - 18 ) {
		// printf( "after %i\n", nTicksLeftTillUpdate );
		return 1;
	}

	const int nMiddleOfPred = int( TIME_TO_TICKS( 1.1f ) / 2 );

	// this will fakelag for 3 ticks, in the middle of the body timer 
	if( nTicksLeftTillUpdate <= nMiddleOfPred - 6 && nTicksLeftTillUpdate >= nMiddleOfPred - 12 ) {
		// printf( "middle %i\n", nTicksLeftTillUpdate );
		return 2;
	}

	return 0;
}

bool FakeLag::ShouldFakeLag( Encrypted_t<CUserCmd> pCmd ) {
	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return false;

	if( pLocal->IsDead( ) )
		return false;

	bool bDontHandle = false;
	static float flSpawnTime = 0.f;
	if( ( g_pGameRules->m_bFreezePeriod( ) || pLocal->m_fFlags( ) & 0x40 || flSpawnTime != pLocal->m_flSpawnTime( ) ) ) {
		bDontHandle = true;
		flSpawnTime = pLocal->m_flSpawnTime( );
	}

	if( bDontHandle )
		return false;

	bool bReturnValue = false;
	const float flVelocityEpsilon = pLocal->m_fFlags( ) & FL_DUCKING ? 3.75f : 1.5f;

	if( g_Vars.rage.fake_lag_moving ) {
		if( !g_Vars.globals.m_bFakeWalking && ( g_Movement.PressingMovementKeys( pCmd.Xor( ) ) || pLocal->m_vecVelocity( ).Length2D( ) > flVelocityEpsilon ) && pLocal->m_fFlags( ) & FL_ONGROUND )
			bReturnValue = true;
	}

	if( g_Vars.rage.fake_lag_air ) {
		if( !( pLocal->m_fFlags( ) & FL_ONGROUND ) )
			bReturnValue = true;
	}

	if( g_Vars.rage.fake_lag_unduck ) {
		if( ( !( pCmd->buttons & IN_DUCK ) && pLocal->m_flDuckAmount( ) > 0.0f ) )
			bReturnValue = true;
	}

	if( g_Vars.rage.fake_lag_peeking ) {
		if( g_Movement.m_bPeeking )
			bReturnValue = false;
	}

	if( !g_Movement.PressingMovementKeys( pCmd.Xor( ) ) && pLocal->m_vecVelocity( ).Length2D( ) <= flVelocityEpsilon && pLocal->m_fFlags( ) & FL_ONGROUND ) {
		if( ( ( TICKS_TO_TIME( pLocal->m_nTickBase( ) ) + ( g_pGlobalVars->interval_per_tick * 2 ) ) > g_ServerAnimations.m_uServerAnimations.m_flLowerBodyRealignTimer ) ) {
			bReturnValue = false;
			g_AntiAim.m_bLastPacket = false;
			g_AntiAim.m_bHasOverriden = true;
		}
	}

	if( BreakDuringBodyTimer( pCmd ) > 0 )
		bReturnValue = true;

	return bReturnValue;
}

int FakeLag::DetermineFakeLagAmount( Encrypted_t<CUserCmd> pCmd ) {
	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return 0;

	bool bRunningDesync = false;
	static bool bDesyncLand;
	const bool bForceDesync = g_Vars.rage.anti_aim_desync_land_force && g_Vars.rage.anti_aim_desync_land_key.enabled;
	const bool bDesyncing = g_Vars.rage.anti_aim_desync_land_first || bForceDesync;


	auto pUnpredictedData = g_Prediction.get_initial_vars( );
	if( pUnpredictedData && bDesyncing ) {
		// we don't want to actually modify our real netvars
		Vector vecOrigin = pLocal->m_vecOrigin( ), vecVelocity = pLocal->m_vecVelocity( );
		int fPredictedFlags = pLocal->m_fFlags( );

		// simulate two movement ticks
		for( int i = 0; i < 2; ++i ) {
			g_Movement.PlayerMove( pLocal, vecOrigin, vecVelocity, fPredictedFlags, pUnpredictedData->flags & FL_ONGROUND );
		}

		if( bDesyncLand ) {
			bRunningDesync = true;
			bDesyncLand = false;
		}

		if( pLocal->m_fFlags( ) != fPredictedFlags ) {
			bRunningDesync = true;
			bDesyncLand = true;
		}
	}

	const auto ApplyType = [&] ( int iLagAmount ) {
		const float extrapolated_speed = pLocal->m_vecVelocity( ).Length( ) * g_pGlobalVars->interval_per_tick;
		switch( g_Vars.rage.fake_lag_type ) {
			case 0: // max
			case 3:
				break;
			case 1: // dyn
				iLagAmount = std::min< int >( static_cast< int >( std::ceilf( 64 / extrapolated_speed ) ), static_cast< int >( iLagAmount ) );
				iLagAmount = std::clamp( iLagAmount, 2, m_iLagLimit );
				break;
			case 2: // fluc
				if( !bDesyncLand ) {
					if( pCmd->tick_count % 40 < 20 ) {
						iLagAmount = iLagAmount;
					}
					else {
						iLagAmount = 2;
					}
				}

				break;
		}

		return iLagAmount;
	};

	int iLagAmount = g_Vars.rage.fake_lag_amount;

	// TROLLER??
	if( g_TickbaseController.m_bShifting ) {
		iLagAmount = std::clamp<int>( iLagAmount, 0, 14 );
	}

	// this will fakelag for 14 ticks, and the choke cycle will end 4 ticks before the lby update 
	if( const int nBodyBreak = BreakDuringBodyTimer( pCmd ); nBodyBreak > 0 ) {
		//printf( "before %i\n", nTicksLeftTillUpdate );
		iLagAmount = nBodyBreak == 1 ? 14 : 6;
	}

	return ApplyType( iLagAmount );
}

void FakeLag::HandleFakeLag( Encrypted_t<bool> bSendPacket, Encrypted_t<CUserCmd> pCmd ) {
	if( g_TickbaseController.m_bPrepareCharge ) {
		*bSendPacket.Xor( ) = true;

		m_iAwaitingChoke = 1;

		if( !g_pClientState->m_nChokedCommands( ) ) {
			g_TickbaseController.m_bPreparedRecharge = true;

			g_TickbaseController.m_bPrepareCharge = false;
		}

		return;
	}

	bool bDisable = ( fabsf( g_pGlobalVars->realtime - g_TickbaseController.m_flLastExploitTime ) < 0.2f ) && g_Vars.rage.exploit && g_Vars.rage.double_tap_bind.enabled && !g_TickbaseController.m_bDisabledFakelag && !g_TickbaseController.m_bShifting && !g_TickbaseController.m_bTapShot;

	if( !g_Vars.rage.fake_lag || bDisable ) {
		if( !g_Vars.globals.m_bFakeWalking ) {
			if( bDisable ) {
				m_iAwaitingChoke = 1;

				*bSendPacket.Xor( ) = true;
				g_TickbaseController.m_bDisabledFakelag = true;
			}
		}

		return;
	}

	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	auto pWeapon = reinterpret_cast< C_WeaponCSBaseGun * >( pLocal->m_hActiveWeapon( ).Get( ) );
	if( !pWeapon )
		return;

	auto pWeaponData = pWeapon->GetCSWeaponData( );
	if( !pWeaponData.IsValid( ) )
		return;

	if( !g_pGameRules.IsValid( ) )
		return;

	// hehe.
	if( g_Vars.globals.m_bFakeWalking || g_pGameRules->m_bFreezePeriod( ) ) {
		return;
	}

	m_iAwaitingChoke = 1;

	if( ShouldFakeLag( pCmd ) ) {
		m_iAwaitingChoke = DetermineFakeLagAmount( pCmd );

		*bSendPacket.Xor( ) = false;
	}

	m_iAwaitingChoke = std::max( m_iAwaitingChoke, 1 );
	if( g_pClientState->m_nChokedCommands( ) >= m_iAwaitingChoke ) {
		//bDoTheThing = false;

		m_iAwaitingChoke = 1;

		*bSendPacket.Xor( ) = true;
	}

	if( pWeaponData->m_iWeaponType == WEAPONTYPE_GRENADE ) {
		if( !pWeapon->m_bPinPulled( ) || ( pCmd->buttons & ( IN_ATTACK | IN_ATTACK2 ) ) ) {
			float m_fThrowTime = pWeapon->m_fThrowTime( );
			if( m_fThrowTime > 0.f ) {
				m_iAwaitingChoke = 1;

				*bSendPacket.Xor( ) = true;
				return;
			}
		}
	}
	else {
		if( ( ( ( pCmd->buttons & IN_ATTACK ) || ( pWeaponData->m_iWeaponType == WEAPONTYPE_KNIFE && pWeapon->m_iItemDefinitionIndex( ) != WEAPON_ZEUS && pWeapon->m_iItemDefinitionIndex( ) != WEAPON_HEALTHSHOT && ( pCmd->buttons & IN_ATTACK2 ) ) ) && pLocal->CanShoot( ) ) /*|| fabsf( g_pGlobalVars->realtime - g_TickbaseController.m_flLastExploitTime ) < 0.1f*/ /*|| fabsf( g_pGlobalVars->realtime - g_Vars.globals.m_flLastShotRealtime ) < 0.1f*/ ) {
			// send attack packet when not fakeducking
			/*if( pCmd->buttons & IN_ATTACK ) {
				if( g_pClientState->m_nChokedCommands( ) == 0 ) {
					*bSendPacket.Xor( ) = false;
					return;
				}
			}*/

			/*if( !g_Vars.globals.m_bFakeWalking ) {
				m_iAwaitingChoke = 1;

				*bSendPacket.Xor( ) = false;
			}*/
		}
	}

	// note - michal;
	// this should never really happens, but incase we ever go above
	// the fakelag limit, then force send a packet

	// change from 14 to 16 once we bypass the 14 tick choke limit
	m_iLagLimit = g_pGameRules->m_bIsValveDS( ) ? 6 : ( g_Vars.sv_maxusrcmdprocessticks->GetInt( ) - 1 );

	int nClampedTicks = std::clamp( g_TickbaseController.m_iProcessTicks, 0, g_TickbaseController.m_iMaxProcessTicks );

	m_iLagLimit = std::clamp( m_iLagLimit - nClampedTicks, 1, m_iLagLimit );

	if( ( g_pClientState->m_nChokedCommands( ) >= m_iLagLimit ) || ( g_pClientState->m_nChokedCommands( ) + 1 ) > m_iLagLimit ) {
		m_iAwaitingChoke = 1;

		*bSendPacket.Xor( ) = true;
	}
}