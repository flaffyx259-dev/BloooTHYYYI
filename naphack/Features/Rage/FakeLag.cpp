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

	static bool bDesyncLand;

	const bool bForceDesync = g_Vars.rage.anti_aim_desync_land_force && g_Vars.rage.anti_aim_desync_land_key.enabled;
	const bool bDesyncing = g_Vars.rage.anti_aim_desync_land_first || bForceDesync;

	auto pUnpredictedData = g_Prediction.get_initial_vars( );
	if( pUnpredictedData && bDesyncing ) {
		Vector vecOrigin = pLocal->m_vecOrigin( ), vecVelocity = pLocal->m_vecVelocity( );
		int fPredictedFlags = pLocal->m_fFlags( );

		for( int i = 0; i < 2; ++i )
			g_Movement.PlayerMove( pLocal, vecOrigin, vecVelocity, fPredictedFlags, pUnpredictedData->flags & FL_ONGROUND );

		if( bDesyncLand )
			bDesyncLand = false;

		// приземление = в предсказании появился FL_ONGROUND
		if( !( pLocal->m_fFlags( ) & FL_ONGROUND ) && ( fPredictedFlags & FL_ONGROUND ) )
			bDesyncLand = true;
	}

	// движок не даёт чокать больше 14 ( см. "14 -> 16" в HandleFakeLag ).
	const int nLimit = std::max( 1, std::min( m_iLagLimit, 14 ) );

	// сколько команд чокать, чтобы сдвиг между записями сервера был > 64 юнитов:
	// BacktrackEntity ( delta.LengthSqr( ) > 64 * 64 ) считает это телепортом -> по истории
	// не стрельнуть, только экстраполяцией. в пакете choke + 1 команд, ceil = запас 1 тик.
	const float flUnitsPerTick = pLocal->m_vecVelocity( ).Length( ) * g_pGlobalVars->interval_per_tick;
	const int nBreakLC = flUnitsPerTick > 0.1f ? static_cast< int >( std::ceilf( 64.f / flUnitsPerTick ) ) : 999;
	const bool bCanBreakLC = nBreakLC <= nLimit;

	int iLagAmount = g_Vars.rage.fake_lag_amount;
	if( g_TickbaseController.m_bShifting )
		iLagAmount = std::clamp( iLagAmount, 0, 14 );

	if( const int nBodyBreak = BreakDuringBodyTimer( pCmd ); nBodyBreak > 0 )
		iLagAmount = nBodyBreak == 1 ? 14 : 6;

	switch( g_Vars.rage.fake_lag_type ) {
		case 0: // max
			break;
		case 1: // dyn: ровно столько, сколько нужно для слома lagcomp
			if( bCanBreakLC )
				iLagAmount = std::min( iLagAmount, nBreakLC );
			iLagAmount = std::max( iLagAmount, std::min( 2, nLimit ) );
			break;
		case 2: // fluc
			if( !bDesyncLand )
				iLagAmount = ( pCmd->tick_count % 40 < 20 ) ? iLagAmount : 2;
			break;
		case 3: { // adaptive
			// нижняя граница - слом lagcomp, длина каждого цикла СЛУЧАЙНАЯ. экстраполяция
			// делит прошедшие тики на прошлый чок -> при разных циклах она ждёт не тот
			// апдейт и ставит точку не туда ( на 250 ед/с разница в 6 тиков = ~23 юнита ).
			const int nMin = std::clamp( bCanBreakLC ? nBreakLC : nLimit, std::min( 2, nLimit ), nLimit );
			iLagAmount = RandomInt( nMin, nLimit );

			// два одинаковых цикла подряд - снова предсказуемо
			for( int i = 0; i < 3 && iLagAmount == m_iLastCycleChoke && nMin < nLimit; ++i )
				iLagAmount = RandomInt( nMin, nLimit );
			break;
		}
	}

	return std::clamp( iLagAmount, 1, nLimit );
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
		m_iAwaitingChoke = 1;

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

	const int nChoked = g_pClientState->m_nChokedCommands( );

	// пик ( IsPeekingEx: сейчас не попадаем, через 4 тика попадём ). пока мы ещё за стеной,
	// отправляем пакет - последняя запись у сервера и противника "за стеной", - а следующий
	// цикл максимальный: выход из-за угла противник увидит позже всего.
	const bool bPeekStart = g_Vars.rage.fake_lag_peeking && g_Movement.m_bPeeking && !m_bWasPeeking;
	m_bWasPeeking = g_Movement.m_bPeeking;

	if( bPeekStart )
		m_bForceMaxNext = true;

	if( bPeekStart && nChoked > 0 ) {
		*bSendPacket.Xor( ) = true; // сброс цикла, пока нас не видно
	}
	else if( ShouldFakeLag( pCmd ) || m_bForceMaxNext ) {
		// длина цикла выбирается ОДИН раз, на первой команде, и держится до отправки.
		if( nChoked == 0 ) {
			m_iLastCycleChoke = m_iCycleChoke;
			m_iCycleChoke = m_bForceMaxNext ? std::max( 1, std::min( m_iLagLimit, 14 ) ) : DetermineFakeLagAmount( pCmd );
			m_bForceMaxNext = false;
		}

		m_iAwaitingChoke = m_iCycleChoke;
		*bSendPacket.Xor( ) = false;
	}

	// стоя AntiAim всё равно шлёт пачки по 2 ( RAX ). держим чок в согласии с ним,
	// иначе UpdateArrivalTick считает прибытие наших команд на ~12 тиков позже.
	if( g_Vars.rage.anti_aim_active && ( pLocal->m_fFlags( ) & FL_ONGROUND ) &&
		pLocal->m_PlayerAnimState( )->m_flVelocityLengthXY <= 0.1f && !g_Vars.globals.m_bRunningExploit ) {
		m_iAwaitingChoke = m_iCycleChoke = 1;
		*bSendPacket.Xor( ) = nChoked >= 1;
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