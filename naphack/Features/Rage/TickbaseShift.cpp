#include "TickbaseShift.hpp"
#include "../../pandora.hpp"
#include "../../SDK/Classes/player.hpp"
#include "../../SDK/Classes/weapon.hpp"
#include "../Rage/EnginePrediction.hpp"
#include "../../Libraries/minhook-master/include/MinHook.h"
#include "../../Hooking/Hooked.hpp"
#include "../../SDK/Displacement.hpp"
#include "../../Features/Visuals/EventLogger.hpp"
#include "../Miscellaneous/Movement.hpp"
#include "AntiAim.hpp"
#include "Ragebot.hpp"

TickbaseSystem g_TickbaseController;

void TickbaseSystem::Charge( ) {
	m_bPrepareCharge = m_bCharge = true;
}

void TickbaseSystem::UnCharge( bool forceProcess ) {
	if( !IsCharged( ) ) {
		m_bTapShot = false;
		m_nTicksCharged = 0;
		g_TickbaseController.m_bPerformDefensive = false;
		return;
	}

	m_bPrepareCharge = m_bCharge = m_bPreparedRecharge = false;
	if( forceProcess && m_pCurrentCmd && m_pCurrentPacket ) {
		if( m_bSentFirstAfterCharge ) {
			// since we want to process the ticks
			// we notify cl_move to process these.
			m_bTeleportUncharge = true;

			// reset stuff :)
			m_bTapShot = false;

			m_nTicksCharged = 0;
		}
	}
	else {
		// we don't want to process these
		// just set to 0
		m_iProcessTicks = 0;

		// reset stuff :)
		m_bTapShot = false;
	}

	g_TickbaseController.m_bPerformDefensive = false;
}

bool TickbaseSystem::CanCharge( ) {
	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return false;

	if( !g_pGameRules.IsValid( ) )
		return false;

	if( pLocal->IsDead( ) ) {
		UnCharge( false );

		return false;
	}

	if( g_Vars.misc.slow_walk && g_Vars.misc.slow_walk_bind.enabled ) {
		UnCharge( true );
		return false;
	}

	const auto pWeapon = reinterpret_cast< C_WeaponCSBaseGun * >( pLocal->m_hActiveWeapon( ).Get( ) );
	if( !pWeapon )
		return false;

	auto pWeaponData = pWeapon->GetCSWeaponData( );
	if( !pWeaponData.IsValid( ) )
		return false;

	if( !g_TickbaseController.m_bForceRecharge ) {
		if( ( pWeaponData->m_iWeaponType == WEAPONTYPE_KNIFE ) && !g_Vars.rage.double_tap_recharge_knife ) {
			if( !g_Vars.rage.double_tap_bind.enabled && !g_Vars.rage.hide_shots_bind.enabled ) {
				UnCharge( true );
			}

			return false;
		}
	}

	if( /*g_pGameRules->m_bFreezePeriod( ) ||*/ g_pGameRules->m_bIsValveDS( ) ) {
		UnCharge( false );
		g_TickbaseController.m_bForceRecharge = false;

		return false;
	}

	if( g_Vars.rage.double_tap_bind.enabled || g_Vars.rage.hide_shots_bind.enabled ) {
		float flRechargeDelay = 0.55f;

		static float flNextPrimaryAttack = pWeapon->m_flNextPrimaryAttack( );
		static float flReloadLength = fabs( g_pGlobalVars->curtime - flNextPrimaryAttack );

		if( flNextPrimaryAttack != pWeapon->m_flNextPrimaryAttack( ) ) {
			flReloadLength = fabs( g_pGlobalVars->curtime - pWeapon->m_flNextPrimaryAttack( ) );
			flNextPrimaryAttack = pWeapon->m_flNextPrimaryAttack( );
		}

		// cray cray
		flRechargeDelay = flReloadLength + .25f;

		if( pWeapon->m_iItemDefinitionIndex( ) == WEAPON_SSG08 || pWeapon->m_iItemDefinitionIndex( ) == WEAPON_AWP )
			flRechargeDelay = 0.33f;

		if( g_Vars.rage.double_tap_recharge_custom_delay )
			flRechargeDelay = g_Vars.rage.double_tap_recharge_delay * 0.11f;

		const bool bAttacking = m_pCurrentCmd && ( m_pCurrentCmd->buttons & IN_ATTACK ) && pWeapon->m_iItemDefinitionIndex( ) != WEAPON_REVOLVER /*: false*/;

		// we want to automatically recharge
		if( ( ( /*g_Ragebot.m_vecAimData.empty( ) &&*/ fabsf( g_pGlobalVars->realtime - m_flLastExploitTime ) > flRechargeDelay && ( !bAttacking ) ) || g_TickbaseController.m_bForceRecharge ) && !IsCharged( ) ) {
			Charge( );
		}
	}
	else {
		// not enabled?
		// we prolly want to uncharge
		UnCharge( true );
	}

	if( !m_bCharge || !m_bPreparedRecharge )
		return false;

	return true;
}

bool TickbaseSystem::IncrementProcessTicks( ) {
	g_TickbaseController.m_iMaxProcessTicks = 15 - 1;//g_Vars.rage.exploit_reserve;

	if( !CanCharge( ) ) {
		m_bInCharge = false;
		return false;
	}

	if( IsCharged( ) ) {
		m_bForceRecharge = m_bInCharge = m_bPrepareCharge = m_bCharge = m_bPreparedRecharge = false;

		return false;
	}

	m_bInCharge = true;
	m_bBreakingLC = m_bTapShot = false;
	m_bDisabledFakelag = false;

	// yeah
	m_nTicksCharged = 0;

	++m_iProcessTicks;

	if( g_Vars.rage.double_tap_bind.enabled ) {
		m_nChargeType = ChargeType::CHARGE_DOUBLETAP;
	}
	else {
		if( g_Vars.rage.hide_shots_bind.enabled ) {
			m_nChargeType = ChargeType::CHARGE_HIDESHOTS;
		}
	}

	return m_iProcessTicks <= m_iMaxProcessTicks;
}

bool TickbaseSystem::IsCharged( ) {
	return m_iProcessTicks >= m_iMaxProcessTicks;
}

void TickbaseSystem::CL_Move( bool bFinalTick, float accumulated_extra_samples ) {
	if( !g_Vars.rage.exploit ) {
		UnCharge( );
		Hooked::oCL_Move( bFinalTick, accumulated_extra_samples );

		m_nTicksCharged = 0;
		g_TickbaseController.m_vecTickbaseFix.clear( );
		m_nBreakLCCorrection = -1;
		m_bDelayDont = false;
		return;
	}

	const bool bWasCharging = m_bInCharge;
	if( IncrementProcessTicks( ) ) {
		g_TickbaseController.m_vecTickbaseFix.clear( );
		m_nBreakLCCorrection = -1;
		m_bDelayDont = false;
		return;
	}

	// regular cmd
	Hooked::oCL_Move( bFinalTick, accumulated_extra_samples );

	// we were charging and are not anymore?
	// we gotta fix something up when not breaking lc
	if( bWasCharging && g_TickbaseController.m_pCurrentCmd && ( g_TickbaseController.m_nChargeType != ChargeType::CHARGE_DOUBLETAP || !g_TickbaseController.m_bBreakLC ) ) {
		g_TickbaseController.m_vecTickbaseFix.push_back( { g_TickbaseController.m_pCurrentCmd->command_number, g_TickbaseController.m_iMaxProcessTicks, true, false } );
	}

	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( ( m_bTeleportUncharge || m_bShiftMove ) && pLocal ) {
		if( m_bDelay ) {
			m_bDelayDont = true;
			return;
		}

		m_bDelayDont = false;

		m_bShiftMove = false;

		int nShiftAmount = std::clamp( m_iProcessTicks, 0, g_TickbaseController.m_iMaxProcessTicks );

		m_iShiftAmount = 0;

		UnCharge( false );

		if( !m_bTeleportUncharge ) {
			if( m_pWeapon ) {
				if( m_pWeapon->m_iItemDefinitionIndex( ) != WEAPON_SSG08 && m_pWeapon->m_iItemDefinitionIndex( ) != WEAPON_AWP && m_pWeapon->m_iItemDefinitionIndex( ) != WEAPON_REVOLVER ) {
					// foo sure men :))
					m_bTapShot = true;
				}

				//if( m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_DEAGLE ) {
				//	m_bDoZeroMove = true;
				//}
			}
			else {
				// foo sure men :))
				m_bTapShot = true;
			}
		}

		// don't communicate :)))
		m_bDontCommunicate = true;

		m_bShifting = true;

		m_iCurrentShiftedTick = 0;

		// run extra commands
		while( nShiftAmount > 0 && !pLocal->IsDead( ) ) {
			++m_iCurrentShiftedTick;

			bFinalTick = nShiftAmount == 1;

			if( bFinalTick ) {
				m_bDontCommunicate = false;
				m_bFORCESEND = true;
			}

			Hooked::oCL_Move( bFinalTick, 0.f ); 

			--nShiftAmount;
		}

		m_bShifting = false;

		m_bDontCommunicate = false;

		if( !m_bTeleportUncharge ) {
			g_TickbaseController.m_flLastExploitTime = g_pGlobalVars->realtime;
		}

		m_bTeleportUncharge = false;
		m_bDoZeroMove = false;

		return;
	}
}

bool TickbaseSystem::DoTheTapping( Encrypted_t<bool> bSendPacket, Encrypted_t<CUserCmd> pFrom ) {
	if( g_Vars.globals.m_bFakeWalking )
		return false;

	C_CSPlayer *const pLocal = C_CSPlayer::GetLocalPlayer( );

	if( !pLocal )
		return false;

	if( !m_pWeapon )
		return false;

	const int nCommandNumber = pFrom->command_number + 1;
	auto pCmd = Encrypted_t<CUserCmd>( &g_pInput->m_pCommands[ nCommandNumber % MULTIPLAYER_BACKUP ] );

	INetChannel *pNetChannel = g_pEngine->GetNetChannelInfo( );

	if( !pCmd.IsValid( ) || !pNetChannel )
		return false;

	memcpy( pCmd.Xor( ), pFrom.Xor( ), 0x64 );

	QAngle angMoveViewangles = pCmd->viewangles;

	pCmd->command_number = pFrom->command_number + 1;

	pCmd->buttons &= ~0x801u;

	UnCharge( );

	int nAutoPeek = 0;
	auto vecPosDelta = g_Movement.m_vecAutoPeekPos - pLocal->GetAbsOrigin( );
	bool bShouldAutoPeek = !g_Movement.m_vecAutoPeekPos.IsZero( ) && ( pLocal->m_fFlags( ) & FL_ONGROUND ) && ( g_Prediction.get_initial_vars( )->flags & FL_ONGROUND ) && !( pCmd->buttons & IN_JUMP );
	if( bShouldAutoPeek ) {
		if( vecPosDelta.Length2D( ) > 8.5f ) {
			nAutoPeek = 1;
		}
		else {
			nAutoPeek = 2;
		}
	}

	// fix movement on our copied cmd (as movement fix hasn't run on pFrom yet...)
	g_Movement.FixMovement( pCmd.Xor( ), g_Movement.m_vecMovementAngles );

	// how to shift tickbase..
	for( int i = 0; i < m_iMaxProcessTicks; i++ ) {
		const int nNewCommandNumber = i + pCmd->command_number;
		auto pNewCommand = Encrypted_t<CUserCmd>( &g_pInput->m_pCommands[ nNewCommandNumber % MULTIPLAYER_BACKUP ] );
		if( !pNewCommand.IsValid( ) )
			continue;

		auto pVerify = Encrypted_t<CVerifiedUserCmd>( &g_pInput->m_pVerifiedCommands[ nNewCommandNumber % MULTIPLAYER_BACKUP ] );
		if( !pVerify.IsValid( ) )
			continue;

		memcpy( pNewCommand.Xor( ), pCmd.Xor( ), 0x64 );

		const bool bHasBeenPredicted = pNewCommand->tick_count == 0x7F7FFFFF;
		pNewCommand->command_number = nNewCommandNumber;
		pNewCommand->hasbeenpredicted = bHasBeenPredicted;

		pNewCommand->viewangles.Clamp( );

		if( nAutoPeek == 1 ) {
			g_Vars.globals.m_bShotAutopeek = true;

			angMoveViewangles = vecPosDelta.ToEulerAngles( );
			pNewCommand->forwardmove = g_Vars.cl_forwardspeed->GetFloat( );
			pNewCommand->sidemove = 0.0f;

			// fix movement
			g_Movement.FixMovement( pNewCommand.Xor( ), angMoveViewangles );
		}
		else if( nAutoPeek == 2 ) {
			g_Movement.MovementControl( pNewCommand.Xor( ), 1.f, false );

			g_Movement.FixMovement( pNewCommand.Xor( ), g_Movement.m_vecMovementAngles );
		}
		else {
			// no autopeek? we can autostop.
			if( m_bDoAutoStop ) {
				const float flMaxSpeed = m_pWeapon->GetMaxSpeed( ) * 0.33000001;
				g_Movement.MovementControl( pNewCommand.Xor( ), flMaxSpeed );
				g_Movement.FixMovement( pNewCommand.Xor( ), g_Movement.m_vecMovementAngles ); 
			}
		}

		memcpy( &pVerify->m_cmd, pNewCommand.Xor( ), 0x64 );

		pVerify->m_crc = pNewCommand->GetChecksum( );

		++g_pClientState->m_nChokedCommands( );
		++pNetChannel->m_nChokedPackets;
		++pNetChannel->m_nOutSequenceNr;
	}

	g_TickbaseController.m_flLastExploitTime = g_pGlobalVars->realtime;

	return true;
}

void CreateFakeCommands( Encrypted_t<bool> bSendPacket, Encrypted_t<CUserCmd> pCmd, int nNumCommand ) {
	if( !*bSendPacket.Xor( ) )
		return;

	for( int i = pCmd->command_number; i < pCmd->command_number + nNumCommand; i++ ) {
		CUserCmd *pFakeCmd = &g_pInput->m_pCommands[ ( i + 1 ) % 150 ];
		if( pFakeCmd != pCmd.Xor( ) )
			memcpy( pFakeCmd, pCmd.Xor( ), 0x64 );

		pFakeCmd->command_number = i + 1;
		pFakeCmd->tick_count += 300;
		pFakeCmd->hasbeenpredicted = true;
		pFakeCmd->buttons |= pFakeCmd->buttons & ( IN_BULLRUSH | IN_SPEED | IN_USE | IN_DUCK );

		CVerifiedUserCmd *pVerified = &g_pInput->m_pVerifiedCommands[ ( i + 1 ) % 150 ];
		if( pVerified )
			memcpy( &pVerified->m_cmd, pFakeCmd, 0x64 );

		pVerified->m_crc = pFakeCmd->GetChecksum( );

		++g_pClientState->m_nChokedCommands( );
	}

	*bSendPacket.Xor( ) = true;

	g_TickbaseController.m_bDoubleTapRelated = true;
}

void TickbaseSystem::RunExploits( Encrypted_t<bool> bSendPacket, Encrypted_t<CUserCmd> pCmd ) {
	C_CSPlayer *pLocal = C_CSPlayer::GetLocalPlayer( );

	if( !pLocal )
		return;

	const auto pWeapon = reinterpret_cast< C_WeaponCSBaseGun * >( pLocal->m_hActiveWeapon( ).Get( ) );
	if( !pWeapon ) 
		return;

	if( !m_pWeaponData )
		return;

	if( g_TickbaseController.m_bTeleportUncharge || g_TickbaseController.m_bShifting || g_TickbaseController.m_bDelay ) {
		if( g_TickbaseController.m_bTeleportUncharge && !g_TickbaseController.m_bShifting )
			*bSendPacket.Xor( ) = false;

		return;
	}

	const bool bInAttack = ( pCmd->buttons & IN_ATTACK );
	if( !m_bTapShot ) {
		const bool bCanShoot = pLocal->CanShoot( );
		bool bCanShiftDoubleTap = CanShift( true );
		if( g_Vars.rage.double_tap_bind.enabled && bCanShiftDoubleTap && bInAttack && bCanShoot ) {

			m_bDoAutoStop = false;
			if( m_pWeapon ) {
				const auto pSettings = g_Ragebot.GetRageSettings( );
				if( pSettings ) {
					if( g_Vars.globals.m_bDidRagebot && pSettings->auto_stop_between_shots && m_pWeapon->m_iItemDefinitionIndex( ) != WEAPON_SSG08 && m_pWeapon->m_iItemDefinitionIndex( ) != WEAPON_AWP ) {
						// tell createmove to run slow walk when we want to (between shots)
						// TODO: maybe make a dropdown (stop between shots / move between shots)
						// stop between shots would be this, move between shots would be just how it was
						m_bDoAutoStop = true;
					}
				}
			}

			m_bShiftMove = true;

			if( m_nBreakLCCorrection >= 0 && m_nBreakLCCorrection <= 6 && g_TickbaseController.m_nLCShiftAmount <= 6 && g_Vars.misc.autopeek && g_Vars.misc.autopeek_bind.enabled && false ) {
				*bSendPacket.Xor( ) = true;
				m_iShiftAmount = 14;

				m_bDelay = true;

				g_TickbaseController.m_vecTickbaseFix.push_back( { pCmd->command_number, m_iMaxProcessTicks - m_nBreakLCCorrection, true, true } );
			} else {
				// we send on the last tick
				// don't send this one yet!
				*bSendPacket.Xor( ) = false;
				m_bDelay = false;
			}

			//if( g_TickbaseController.m_nLCShiftAmount >= g_TickbaseController.m_iMaxProcessTicks || true ) {
			//	m_bShiftMove = true;

			//	// we send on the last tick
			//	// don't send this one yet!
			//	*bSendPacket.Xor( ) = false;
			//}
			//else {
			//	m_bShiftMove = false;
			//	if( DoTheTapping( bSendPacket, pCmd ) ) {
			//		if( m_pWeapon->m_iItemDefinitionIndex( ) != WEAPON_SSG08 && m_pWeapon->m_iItemDefinitionIndex( ) != WEAPON_AWP && m_pWeapon->m_iItemDefinitionIndex( ) != WEAPON_REVOLVER ) {
			//			// foo sure men :))
			//			m_bTapShot = true;
			//		}

			//		// go send this nigguh
			//		*bSendPacket.Xor( ) = true;
			//	}
			//}

			//if( g_TickbaseController.m_nLCShiftAmount < g_TickbaseController.m_iMaxProcessTicks ) {
			//	int nCorrection = g_TickbaseController.m_iMaxProcessTicks - g_TickbaseController.m_nBreakLCCorrection;
			//	//if( g_Vars.misc.autopeek && g_Vars.misc.autopeek_bind.enabled )
			//	//	nCorrection -= 1;

			//	g_TickbaseController.m_vecTickbaseFix.push_back( { pCmd->command_number, g_TickbaseController.m_iMaxProcessTicks - g_TickbaseController.m_nBreakLCCorrection, false, true } );

			//	// after this we assume the difference is fixed, no need to re-attempt at a later point
			//	g_TickbaseController.m_nBreakLCCorrection = g_TickbaseController.m_iMaxProcessTicks;
			//}

			g_TickbaseController.m_bPerformDefensive = false;
		}
		else if( g_Vars.rage.double_tap_bind.enabled && CanShift( false, true ) && !g_Vars.globals.m_bDidRagebot ) { // not shooting (no double tap) -> break lc
			if( g_TickbaseController.m_bBreakLC ) {
				g_TickbaseController.m_nLCShiftAmount = g_TickbaseController.m_iMaxProcessTicks;

				int peek_max = 6;

				// the force defensive feature (force disabled/not useable because of the stutter it causes)
				g_Vars.rage.double_tap_lc_break = false;

				if( false ) {
					const bool bOnLand = !( g_Prediction.get_initial_vars( )->flags & FL_ONGROUND ) && ( pLocal->m_fFlags( ) & FL_ONGROUND );
					if( /*g_Vars.rage.double_tap_peek
						&& */( ( ( pLocal->m_vecVelocity( ).Length2D( ) >= 3.5f || g_Movement.PressingMovementKeys( pCmd.Xor( ) ) || m_nBreakLCCorrection >= 0 /*&& g_Ragebot.m_vecAimData.empty( )*/ ) )
							   && g_TickbaseController.m_nTicksCharged >= 3
							   && pLocal->m_fFlags( ) & FL_ONGROUND
							   && !bOnLand ) || ( g_Vars.rage.double_tap_lc_break && g_TickbaseController.m_nTicksCharged >= 3 ) ) {
						if( pWeapon->m_iItemDefinitionIndex( ) != WEAPON_REVOLVER && m_pWeaponData->m_iWeaponType != WEAPONTYPE_KNIFE ) {
							g_TickbaseController.m_bPerformDefensive = ( g_Vars.rage.double_tap_lc_break /*&& g_Ragebot.m_vecAimData.empty( )*/ ) || ( g_Movement.m_bPeeking /*&& g_Ragebot.m_vecAimData.empty( )*/ ) || ( m_nBreakLCCorrection >= 0 /*&& g_Ragebot.m_vecAimData.empty( )*/ && m_nBreakLCCorrection < peek_max );

							if( !g_TickbaseController.m_bPerformDefensive || ( g_Vars.rage.double_tap_lc_break && m_nBreakLCCorrection == m_iMaxProcessTicks ) ) {
								if( m_nBreakLCCorrection >= 0 && m_iMaxProcessTicks - m_nBreakLCCorrection != 0 && !m_bFixedEarlier ) {
									g_TickbaseController.m_vecTickbaseFix.push_back( { pCmd->command_number, m_iMaxProcessTicks - m_nBreakLCCorrection, false, true } );
								}

								m_nBreakLCCorrection = -1;
								m_bFixedEarlier = false;

								if( g_Vars.rage.double_tap_lc_break ) {
									g_TickbaseController.m_bPerformDefensive = false;
								}
							}

							if( m_nBreakLCCorrection == peek_max )
								g_TickbaseController.m_nLCShiftAmount = peek_max;

							//if( g_TickbaseController.m_bPerformDefensive && m_nBreakLCCorrection == -1 ) {
							//	g_pDebugOverlay->AddBoxOverlay( pLocal->m_vecOrigin( ), { -10, -10, -10 }, { 10, 10 ,10 }, { }, 255, 255, 255, 127, 1.5f );
							//}

							// TODO: test with these bSending changes...
							bool bSending = *bSendPacket.Xor( );

							int nShiftIt = 2;

							// peeking bruh
							if( g_TickbaseController.m_bPerformDefensive && m_nBreakLCCorrection < peek_max && ( bSending || ( !bSending && m_nBreakLCCorrection == -1 ) ) ) {
								if( m_nBreakLCCorrection == -1 ) {
									m_nBreakLCCorrection = 0;

									g_TickbaseController.m_nLCShiftAmount = m_nBreakLCCorrection;

									g_TickbaseController.m_vecTickbaseFix.push_back( { pCmd->command_number, g_TickbaseController.m_iMaxProcessTicks - m_nBreakLCCorrection, true, false } );

									//g_EventLog.PushEvent( std::string( XorStr( "DEFENSIVE START ->  tick: " ) ).append( std::to_string( m_nBreakLCCorrection ) ).append( XorStr( " cmd: " ) ).append( std::to_string( pCmd->command_number ) ), Color_f::White, false );
									//g_EventLog.PushEvent( std::string( XorStr( "SIMTIME START ->  " ) ).append( std::to_string( pLocal->m_flSimulationTime( ) ) ), Color_f::White, false );
								} else {
									// TODO: look into logic here
									// and make sure to fix the difference (e.g. we were at 8 (peek) and now 14 (regular shift))

									bool bIncremented = false;
									if( m_nBreakLCCorrection < peek_max ) {
										m_nBreakLCCorrection += nShiftIt;
									}

									if( m_nBreakLCCorrection > peek_max ) {
										m_nBreakLCCorrection = std::clamp( m_nBreakLCCorrection, 0, peek_max );
									}

									g_TickbaseController.m_nLCShiftAmount = m_nBreakLCCorrection;

									g_TickbaseController.m_vecTickbaseFix.push_back( { pCmd->command_number, nShiftIt, false, true } );

									//g_EventLog.PushEvent( std::string( XorStr( "DEFENSIVE ->  tick: " ) ).append( std::to_string( m_nBreakLCCorrection ) ).append( XorStr( " cmd: " ) ).append( std::to_string( pCmd->command_number ) ), Color_f::White, false );
									//g_EventLog.PushEvent( std::string( XorStr( "SIMTIME ->  " ) ).append( std::to_string( pLocal->m_flSimulationTime( ) ) ), Color_f::White, false );
								}

								//printf( "CREATE: %i %i\n", pCmd->command_number, g_TickbaseController.m_nLCShiftAmount );

								// make sure we shift
								*bSendPacket.Xor( ) = true;
							} else {
								if( g_TickbaseController.m_bPerformDefensive && m_nBreakLCCorrection < peek_max && ( !bSending ) )
									g_TickbaseController.m_nLCShiftAmount = m_nBreakLCCorrection;

								if( m_nBreakLCCorrection == peek_max && false ) {
									if( m_nBreakLCCorrection >= 0 && m_iMaxProcessTicks - m_nBreakLCCorrection != 0 && !m_bFixedEarlier ) {
										g_TickbaseController.m_vecTickbaseFix.push_back( { pCmd->command_number, m_iMaxProcessTicks - m_nBreakLCCorrection, false, true } );
										m_bFixedEarlier = true;
									}
								}
							}
						} else {
							m_nBreakLCCorrection = -1;
						}
					} else {
						// if we haven't reached 14 ticks last cycle
						// we have to fix the difference
						if( m_nBreakLCCorrection >= 0 ) {
							if( m_iMaxProcessTicks - m_nBreakLCCorrection != 0 ) {
								g_TickbaseController.m_vecTickbaseFix.push_back( { pCmd->command_number, m_iMaxProcessTicks - m_nBreakLCCorrection, false, true } );
							}
						}

						m_nBreakLCCorrection = -1;

						g_TickbaseController.m_bPerformDefensive = false;
					}
				}

				g_TickbaseController.m_bDoubleTapRelated = false;

				m_iShiftAmount = g_TickbaseController.m_nLCShiftAmount;//g_TickbaseController.m_nLCShiftAmount;
			
				//CreateFakeCommands( bSendPacket, pCmd, m_iShiftAmount );

				// send her
				//if( !g_TickbaseController.m_nTicksCharged && !g_TickbaseController.m_bBreakingLC )
					//*bSendPacket.Xor( ) = true;

				m_bBreakingLC = true;
			}
		}
		else { // hide shots (or no dt possible)
			if( g_Vars.rage.hide_shots_bind.enabled && CanShift( ) && bCanShoot && bInAttack ) {
				DoRegularShifting( 7 );

				g_TickbaseController.m_bDoubleTapRelated = false;

				g_TickbaseController.m_vecTickbaseFix.push_back( { pCmd->command_number, m_iShiftAmount, true, true } );

				// always send so we don't fuck with tickbase etc
				*bSendPacket.Xor( ) = true;
			}
		}
	}

	const float flDisableTheShitDelay = 0.3f;

	// last shot too long ago?
	if( fabsf( g_pGlobalVars->realtime - m_flLastExploitTime ) > flDisableTheShitDelay ) {
		m_bTapShot = false;
	}
}

void TickbaseSystem::DoRegularShifting( int iAmount ) {
	if( iAmount >= 11 ) {
		m_bTapShot = true;
	}

	m_iShiftAmount = iAmount;
}

bool TickbaseSystem::CanShift( bool bDoubleTap, bool bNoWeap ) {
	if( !IsCharged( ) )
		return false;

	C_CSPlayer *pLocal = C_CSPlayer::GetLocalPlayer( );

	if( !pLocal )
		return false;

	if( !m_pWeapon )
		return false;

	if( !m_pWeaponData )
		return false;

	if( !bNoWeap ) {
		if( m_pWeaponData->m_iWeaponType == WEAPONTYPE_C4 ||
			( m_pWeaponData->m_iWeaponType == WEAPONTYPE_KNIFE && !bDoubleTap ) ||
			m_pWeaponData->m_iWeaponType == WEAPONTYPE_GRENADE ||
			( m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_REVOLVER && !bDoubleTap ) ||
			m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_ZEUS ||
			m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_HEALTHSHOT ) {
			return false;
		}
	}

	if( bDoubleTap ) {
		const auto nBackupTickbase = pLocal->m_nTickBase( );

		// hack.
		pLocal->m_nTickBase( ) -= m_iMaxProcessTicks;//bIsOneShot ? 10 : m_iMaxProcessTicks;

		if( g_Vars.rage.double_tap_bind.enabled && g_TickbaseController.m_bBreakingLC && g_TickbaseController.m_bBreakLC ) {
			// in createmove we already account for tickbase
			// to fix aimbot etc
			// though revolver fixes up itself in runcommand with tickbase :D !
			// so to fix other issues (like aa disabling) we undo the fix here.
			if( ( m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_REVOLVER ) ) {
				pLocal->m_nTickBase( ) += g_TickbaseController.m_nLCShiftAmount;
			}
		}

		const bool bCanShoot = pLocal->CanShoot( );
		pLocal->m_nTickBase( ) = nBackupTickbase;

		return  ( m_pWeaponData->m_iWeaponType == WEAPONTYPE_KNIFE || ( m_pWeaponData->m_iWeaponType != WEAPONTYPE_KNIFE && m_pWeapon->m_iClip1( ) >= 1 ) ) && bCanShoot;
	}

	return true;
}

int TickbaseSystem::GetCorrection( ) {
	// we can just return this here, because when we use it
	// it won't be updated yet = we got the amount we should have shifted (the amount we are behind) before
	return g_TickbaseController.m_nLCShiftAmount;
}
