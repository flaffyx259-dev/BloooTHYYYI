#include "Movement.hpp"
#include "../../SDK/variables.hpp"
#include "../../SDK/Classes/Player.hpp"
#include "../../SDK/Classes/weapon.hpp"
#include "../Rage/Ragebot.hpp"
#include "../Visuals/EventLogger.hpp"

#include "../Scripting/Scripting.hpp"

#include "../Rage/AntiAim.hpp"
#include "../Rage/FakeLag.hpp"
#include "../Rage/EnginePrediction.hpp"
#include "../Rage/TickbaseShift.hpp"
#include "../Rage/ServerAnimations.hpp"

#include "../Rage/Autowall.hpp"
#include "../Rage/FakePing.hpp"

#include "../Rage/Resolver.hpp"

Movement g_Movement;

void Movement::FixMovement( CUserCmd *cmd, QAngle wishangle ) {
	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal ) {
		return;
	}

	// pasted asf idgaf
	//if( cmd->viewangles.z ) {
	Vector vRealF, vRealR, vRealU;
	QAngle aRealDir = cmd->viewangles;
	aRealDir.Normalize( );

	Math::AngleVectors( aRealDir, vRealF, vRealR, vRealU );
	vRealF[ 2 ] = 0;
	vRealR[ 2 ] = 0;

	vRealF.Normalize( );
	vRealR.Normalize( );

	// Ok now we compute the vectors as the gamemovement but this time with our client viewangles.
	// This is where we want to go.

	QAngle aWishDir = wishangle;
	aWishDir.Normalize( );

	Vector vWishF, vWishR, vWishU;
	Math::AngleVectors( aWishDir, vWishF, vWishR, vWishU );

	vWishF[ 2 ] = 0;
	vWishR[ 2 ] = 0;

	vWishF.Normalize( );
	vWishR.Normalize( );

	// We now compute our wished velocity based on our current movements.
	Vector vWishVel;
	vWishVel[ 0 ] = vWishF[ 0 ] * cmd->forwardmove + vWishR[ 0 ] * cmd->sidemove;
	vWishVel[ 1 ] = vWishF[ 1 ] * cmd->forwardmove + vWishR[ 1 ] * cmd->sidemove;
	vWishVel[ 2 ] = 0;

	float a = vRealF[ 0 ], b = vRealR[ 0 ], c = vRealF[ 1 ], d = vRealR[ 1 ];
	float v = vWishVel[ 0 ], w = vWishVel[ 1 ];

	float flDivide = ( a * d - b * c );
	float x = ( d * v - b * w ) / flDivide;
	float y = ( a * w - c * v ) / flDivide;

	cmd->forwardmove = x;
	cmd->sidemove = y;
	//}
	/*else {
		Vector view_fwd, view_right, view_up, cmd_fwd, cmd_right, cmd_up;
		Math::AngleVectors( wishangle, view_fwd, view_right, view_up );
		Math::AngleVectors( cmd->viewangles, cmd_fwd, cmd_right, cmd_up );

		const auto v8 = sqrtf( ( view_fwd.x * view_fwd.x ) + ( view_fwd.y * view_fwd.y ) );
		const auto v10 = sqrtf( ( view_right.x * view_right.x ) + ( view_right.y * view_right.y ) );
		const auto v12 = sqrtf( view_up.z * view_up.z );

		const Vector norm_view_fwd( ( 1.f / v8 ) * view_fwd.x, ( 1.f / v8 ) * view_fwd.y, 0.f );
		const Vector norm_view_right( ( 1.f / v10 ) * view_right.x, ( 1.f / v10 ) * view_right.y, 0.f );
		const Vector norm_view_up( 0.f, 0.f, ( 1.f / v12 ) * view_up.z );

		const auto v14 = sqrtf( ( cmd_fwd.x * cmd_fwd.x ) + ( cmd_fwd.y * cmd_fwd.y ) );
		const auto v16 = sqrtf( ( cmd_right.x * cmd_right.x ) + ( cmd_right.y * cmd_right.y ) );
		const auto v18 = sqrtf( cmd_up.z * cmd_up.z );

		const Vector norm_cmd_fwd( ( 1.f / v14 ) * cmd_fwd.x, ( 1.f / v14 ) * cmd_fwd.y, 0.f );
		const Vector norm_cmd_right( ( 1.f / v16 ) * cmd_right.x, ( 1.f / v16 ) * cmd_right.y, 0.f );
		const Vector norm_cmd_up( 0.f, 0.f, ( 1.f / v18 ) * cmd_up.z );

		const auto v22 = norm_view_fwd.x * cmd->forwardmove;
		const auto v26 = norm_view_fwd.y * cmd->forwardmove;
		const auto v28 = norm_view_fwd.z * cmd->forwardmove;
		const auto v24 = norm_view_right.x * cmd->sidemove;
		const auto v23 = norm_view_right.y * cmd->sidemove;
		const auto v25 = norm_view_right.z * cmd->sidemove;
		const auto v30 = norm_view_up.x * cmd->upmove;
		const auto v27 = norm_view_up.z * cmd->upmove;
		const auto v29 = norm_view_up.y * cmd->upmove;

		cmd->forwardmove = ( ( ( ( norm_cmd_fwd.x * v24 ) + ( norm_cmd_fwd.y * v23 ) ) + ( norm_cmd_fwd.z * v25 ) )
							 + ( ( ( norm_cmd_fwd.x * v22 ) + ( norm_cmd_fwd.y * v26 ) ) + ( norm_cmd_fwd.z * v28 ) ) )
			+ ( ( ( norm_cmd_fwd.y * v30 ) + ( norm_cmd_fwd.x * v29 ) ) + ( norm_cmd_fwd.z * v27 ) );
		cmd->sidemove = ( ( ( ( norm_cmd_right.x * v24 ) + ( norm_cmd_right.y * v23 ) ) + ( norm_cmd_right.z * v25 ) )
						  + ( ( ( norm_cmd_right.x * v22 ) + ( norm_cmd_right.y * v26 ) ) + ( norm_cmd_right.z * v28 ) ) )
			+ ( ( ( norm_cmd_right.x * v29 ) + ( norm_cmd_right.y * v30 ) ) + ( norm_cmd_right.z * v27 ) );
		cmd->upmove = ( ( ( ( norm_cmd_up.x * v23 ) + ( norm_cmd_up.y * v24 ) ) + ( norm_cmd_up.z * v25 ) )
						+ ( ( ( norm_cmd_up.x * v26 ) + ( norm_cmd_up.y * v22 ) ) + ( norm_cmd_up.z * v28 ) ) )
			+ ( ( ( norm_cmd_up.x * v30 ) + ( norm_cmd_up.y * v29 ) ) + ( norm_cmd_up.z * v27 ) );
	}*/

	if( pLocal->m_MoveType( ) != MOVETYPE_LADDER ) {
		
		switch( g_Vars.misc.leg_movement ) {
			case 1:
				cmd->buttons &= ~( IN_FORWARD | IN_BACK | IN_MOVERIGHT | IN_MOVELEFT );
				break;
			case 2:
			{
				if( cmd->forwardmove > 0 ) {
					cmd->buttons |= IN_BACK;
					cmd->buttons &= ~IN_FORWARD;
				}

				if( cmd->forwardmove < 0 ) {
					cmd->buttons |= IN_FORWARD;
					cmd->buttons &= ~IN_BACK;
				}

				if( cmd->sidemove < 0 ) {
					cmd->buttons |= IN_MOVERIGHT;
					cmd->buttons &= ~IN_MOVELEFT;
				}

				if( cmd->sidemove > 0 ) {
					cmd->buttons |= IN_MOVELEFT;
					cmd->buttons &= ~IN_MOVERIGHT;
				}
			}break;
		}
	}
}

void Movement::PrePrediction( CUserCmd *cmd, bool *bSendPacket ) {
	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	if( pLocal->IsDead( ) )
		return;

	#if defined(DEV)
		if( g_Vars.misc.fastduck )
			cmd->buttons |= IN_BULLRUSH;
	#endif

	auto pUnpredictedData = g_Prediction.get_initial_vars( );

	// reset it, so we don't fuck up autostop lol
	m_bDidStop = false;

	const bool bOnLand = !( pUnpredictedData->flags & FL_ONGROUND ) && ( pLocal->m_fFlags( ) & FL_ONGROUND );

	// store the original viewangles
	// fix movement with these later
	g_Movement.m_vecMovementAngles = cmd->viewangles;
	g_Movement.m_flSidemove = cmd->sidemove;
	g_Movement.m_flForwardmove = cmd->forwardmove;
	g_Movement.m_bModifiedMovementBeforePrediction = false;

	// bunny hop
	bool bHoldingJump = cmd->buttons & IN_JUMP;
	if( g_Vars.misc.bunnyhop ) {
		// holding space
		if( bHoldingJump ) {
			// valid movetype
			if( pLocal->m_MoveType( ) != MOVETYPE_LADDER &&
				pLocal->m_MoveType( ) != MOVETYPE_NOCLIP ) {
				// not on ground
				if( !( pLocal->m_fFlags( ) & FL_ONGROUND ) ) {
					// remove jump only when in air
					// will get added back the tick we're on ground
					cmd->buttons &= ~IN_JUMP;
				}
			}
		}
	}

	static QAngle vecLastView;
	static Vector vecLastVelocity;
	QAngle vecCurView;
	bool bDefinitelyGround = pLocal->m_fFlags( ) & FL_ONGROUND && pUnpredictedData->flags & FL_ONGROUND;
	static int nHopCount = 0;

	if( bDefinitelyGround ) {
		nHopCount = 0;

		vecLastVelocity = pLocal->m_vecVelocity( );
		g_pEngine->GetViewAngles( vecLastView );
	}
	else {
		if( bHoldingJump ) {
			g_pEngine->GetViewAngles( vecCurView );

			if( bOnLand || g_Movement.PressingMovementKeys( cmd, false ) ) {
				nHopCount++;
			}
		}
	}

	// auto strafers
	if( g_Vars.misc.autostrafer && !( cmd->buttons & IN_SPEED ) && !( g_Vars.misc.slow_walk && g_Vars.misc.slow_walk_bind.enabled ) ) {
		// make sure we're not on ground and we've hopped atleast once
		if( !( pLocal->m_fFlags( ) & FL_ONGROUND ) && !bOnLand && ( nHopCount > 0 || ( g_Vars.misc.autostrafer_wasd && g_Movement.PressingMovementKeys( cmd ) ) || ( fabs( vecCurView.y - vecLastView.y ) >= 15.f ) || ( vecLastVelocity.Length2D( ) >= 65.f ) ) ) {
			// valid movetype
			if( pLocal->m_MoveType( ) == MOVETYPE_WALK ) {
				// save velocity
				Vector vecVelocity = pLocal->m_vecVelocity( );

				// save speed, compute the ideal strafe
				const float flSpeed = vecVelocity.Length2D( );

				static float flStrafe = 1.f;

				const float flIdealStrafe = std::min( 90.f, 845.5f / flSpeed );
				const float flVelocityYaw = RAD2DEG( std::atan2( vecVelocity.y, vecVelocity.x ) );

				if( g_Vars.misc.autostrafer_wasd && ( cmd->forwardmove != 0.f || cmd->sidemove != 0.f ) ) {
					cmd->forwardmove /*= cmd->sidemove*/ = 0.f;

					enum EDirections {
						FORWARDS = 0,
						BACKWARDS = 180,
						LEFT = 90,
						RIGHT = -90,
						BACK_LEFT = 135,
						BACK_RIGHT = -135
					};

					float flWishDirection{ };

					const bool bHoldingW = cmd->buttons & IN_FORWARD;
					const bool bHoldingA = cmd->buttons & IN_MOVELEFT;
					const bool bHoldingS = cmd->buttons & IN_BACK;
					const bool bHoldingD = cmd->buttons & IN_MOVERIGHT;

					if( bHoldingW ) {
						if( bHoldingA ) {
							flWishDirection += ( EDirections::LEFT / 2 );
						}
						else if( bHoldingD ) {
							flWishDirection += ( EDirections::RIGHT / 2 );
						}
						else {
							flWishDirection += EDirections::FORWARDS;
						}
					}
					else if( bHoldingS ) {
						if( bHoldingA ) {
							flWishDirection += EDirections::BACK_LEFT;
						}
						else if( bHoldingD ) {
							flWishDirection += EDirections::BACK_RIGHT;
						}
						else {
							flWishDirection += EDirections::BACKWARDS;
						}
					}
					else if( bHoldingA ) {
						flWishDirection += EDirections::LEFT;
					}
					else if( bHoldingD ) {
						flWishDirection += EDirections::RIGHT;
					}

					m_vecMovementAngles.yaw += std::remainderf( flWishDirection, 360.f );
				}

				const float flAngDiff = Math::AngleNormalize( m_vecMovementAngles.yaw - flVelocityYaw );
				const float flAbsAngDiff = fabsf( flAngDiff );

				const float flStep = flIdealStrafe + ( flIdealStrafe * ( 1.f - ( g_Vars.misc.autostrafer_smooth / 100.f ) ) );
				if( flAbsAngDiff > 170.f && flSpeed > 80.f || flAngDiff > flStep && flSpeed > 80.f ) {
					m_vecMovementAngles.yaw = flStep + flVelocityYaw;
					cmd->sidemove = -g_Vars.cl_sidespeed->GetFloat( );
				}
				else if( -flStep <= flAngDiff || flSpeed <= 80.f ) {
					m_vecMovementAngles.yaw += flIdealStrafe * flStrafe;
					cmd->sidemove = g_Vars.cl_sidespeed->GetFloat( ) * flStrafe;
				}
				else {
					m_vecMovementAngles.yaw = flVelocityYaw - flStep;
					cmd->sidemove = g_Vars.cl_sidespeed->GetFloat( );
				}

				flStrafe = -flStrafe;

				m_vecMovementAngles.Normalize( );
			}
		}
	}

	float flMaxSpeed = pLocal->GetMaxSpeed( );


	bool bWillRetreat = g_Vars.misc.autopeek && g_Vars.misc.autopeek_bind.enabled && ( ( /*g_Vars.misc.autopeek_retrack == 0 &&*/ g_Vars.globals.m_bShotAutopeek ) || ( g_Vars.misc.autopeek_retrack == 1 && !g_Movement.PressingMovementKeys( cmd ) ) );
	if( ( g_TickbaseController.m_bShifting && !g_TickbaseController.m_bTeleportUncharge ) ) {
		const auto pWeapon = reinterpret_cast< C_WeaponCSBaseGun * >( pLocal->m_hActiveWeapon( ).Get( ) );
		if( pWeapon ) {
			bool bDoubleTap = g_TickbaseController.m_bShifting && !g_TickbaseController.m_bTeleportUncharge && g_TickbaseController.m_bDoAutoStop && !bWillRetreat;
			if( bDoubleTap ) {
				flMaxSpeed = pWeapon->GetMaxSpeed( ) * 0.3000001;
				cmd->buttons &= ~IN_SPEED;
				StopToSpeed( flMaxSpeed - 0.1f, cmd );
			}
		}
	}

	if( !g_Vars.globals.m_bDidRagebot ) {
		if( ( g_Vars.misc.quickstop && !g_Vars.globals.m_bShotAutopeek && pLocal->m_fFlags( ) & FL_ONGROUND && !( cmd->buttons & IN_JUMP ) ) ) {

			if( !g_Vars.globals.m_bRunningExploit && !g_Vars.globals.m_bDidQuickStop ) {
				Vector vecVelocity = pLocal->m_vecVelocity( );

				if( !g_Movement.PressingMovementKeys( cmd ) ) {
					InstantStop( cmd );

					g_Movement.m_bModifiedMovementBeforePrediction = true;
					g_Vars.globals.m_bDoingQuickStop = true;
				}
			}

			if( !g_Movement.PressingMovementKeys( cmd ) )
				if( !( pLocal->m_vecVelocity( ).Length( ) > 1.0f ) ) {
					g_Vars.globals.m_bDoingQuickStop = false;
					g_Vars.globals.m_bDidQuickStop = true;
				}
		}
		else {
			g_Vars.globals.m_bDoingQuickStop = false;
		}
	}

	if( pLocal->m_vecVelocity( ).Length2D( ) > 10.f && g_Movement.PressingMovementKeys( cmd ) )
		g_Vars.globals.m_bDidQuickStop = false;

	if( !g_Vars.misc.quickstop ) {
		g_Vars.globals.m_bDoingQuickStop = false;
		g_Vars.globals.m_bDidQuickStop = true;
	}

	if( g_Vars.globals.m_bForceFakewalk && !( g_Vars.rage.exploit && g_Vars.rage.double_tap_bind.enabled ) ) {
		// nice, we stopped fully after autostopping aimbot
		if( pLocal->m_vecVelocity( ).Length2D( ) < 5.f && g_Ragebot.m_stop == EAutoStopType::STOP_NONE )
			m_bDoForceFakewalk = true;
	}

	g_Movement.FakeDuck( bSendPacket, cmd, m_bDoForceFakewalk );

	// disable force fake walk
	if( !g_Movement.PressingMovementKeys( cmd ) || cmd->weaponselect || ( g_Ragebot.m_AimbotInfo.m_pSettings && !g_Ragebot.m_AimbotInfo.m_pSettings->lock_fakewalk_on_movement && !g_Ragebot.m_stop ) )
		m_bDoForceFakewalk = g_Vars.globals.m_bForceFakewalk = false;

	if( !m_bDoForceFakewalk ) {
		if( g_Ragebot.m_stop != EAutoStopType::STOP_NONE ) {
			// we are using regular autostop, run this.
			if( g_Ragebot.m_stop == EAutoStopType::STOP_SLIDE ) {
				AutoStop( cmd );
			}

			// we are using fake walk autostop, instant stop us first.
			if( g_Ragebot.m_stop == EAutoStopType::STOP_FAKE )
				InstantStop( cmd );

			// reset our autostop mode.
			g_Ragebot.m_stop = EAutoStopType::STOP_NONE;
		}
	}
}

void Movement::InPrediction( CUserCmd *cmd ) {
	AutoPeek( cmd );

	// credits to raxer again
	// this won't really be used for now and it might be intensive, just commenting it out for now
	// well better to just check it here

	m_bPeeking = IsPeekingEx( g_AntiAim.GetBestPlayer( ), cmd );

	/*
	static float bPeekTimer = 0.f;
	static bool bPeeking = false;

	if( bPeekTimer > g_pGlobalVars->curtime + TICKS_TO_TIME( 3 ) ) {
		bPeekTimer = 0.f;
		bPeeking = false;
	}

	m_bPeeking = bPeekTimer >= g_pGlobalVars->curtime;

	if( IsPeeking( g_AntiAim.GetBestPlayer( ), cmd ) ) {
		if( !bPeeking && bPeekTimer < g_pGlobalVars->curtime ) {
			bPeekTimer = g_pGlobalVars->curtime + TICKS_TO_TIME( 3 );

			m_bPeeking = false;
		}

		m_bPeeking = bPeeking = true;
	}
	*/
}

void Movement::PostPrediction( CUserCmd *cmd ) {
	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	if( pLocal->IsDead( ) )
		return;

	auto pUnpredictedData = g_Prediction.get_networked_vars( cmd->command_number );

	/*if( g_Movement.m_bPeeking ) {
		g_pDebugOverlay->AddBoxOverlay( C_CSPlayer::GetLocalPlayer( ) ? C_CSPlayer::GetLocalPlayer( )->GetEyePosition( ) : Vector{ 0,0,0 }, { -2,-2,-2 }, { 2,2,2 }, {}, 255, 255, 255, 100, g_pGlobalVars->interval_per_tick * 4 );
	}*/

	const auto flSpeed = pLocal->m_vecVelocity( ).Length2D( );

	const bool bOnLand = !( pUnpredictedData->flags & FL_ONGROUND ) && ( pLocal->m_fFlags( ) & FL_ONGROUND );

	if( pUnpredictedData->flags & FL_ONGROUND && !( pLocal->m_fFlags( ) & FL_ONGROUND ) )
		if( g_Vars.misc.edge_jump && g_Vars.misc.edge_jump_key.enabled )
			cmd->buttons |= IN_JUMP;

#if defined(LUA_SCRIPTING)
	Scripting::Script::DoCallback( hash_32_fnv1a_const( XorStr( "post_move" ) ), cmd );
#endif

	// lerp our latency.
	g_FakePing.GetTargetLatency( );

	// XDD !!! 
	cmd->forwardmove = std::clamp<float>( cmd->forwardmove, -450.f, 450.f );
	cmd->sidemove = std::clamp<float>( cmd->sidemove, -450.f, 450.f );
	cmd->upmove = std::clamp<float>( cmd->upmove, -320.f, 320.f );

	cmd->viewangles.Clamp( );

	// fix movement after all movement code has ran
	g_Movement.FixMovement( cmd, m_vecMovementAngles );
}

void Movement::AutoPeek( CUserCmd *cmd ) {
	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	if( pLocal->IsDead( ) )
		return;

	auto pUnpredictedData = g_Prediction.get_initial_vars( );

	if( g_Vars.misc.autopeek && g_Vars.misc.autopeek_bind.enabled ) {
		if( ( pLocal->m_fFlags( ) & FL_ONGROUND ) ) {
			if( m_vecAutoPeekPos.IsZero( ) ) {
				m_vecAutoPeekPos = pLocal->GetAbsOrigin( );
			}
		}
	}
	else {
		m_vecAutoPeekPos = Vector( );
		g_Vars.globals.m_bShotAutopeek = false;
	}

	m_bRetrack = false;
	if( /*g_Vars.misc.autopeek_retrack_shot*/ true ) {
		if( g_Vars.globals.m_bShotAutopeek )
			m_bRetrack = true;
	}
	//else {
	//	g_Vars.globals.m_bShotAutopeek = false;
	//}

	if( g_Vars.misc.autopeek_retrack == 1 ) {
		if( !g_Movement.PressingMovementKeys( cmd ) )
			m_bRetrack = true;
	}

	static std::array<bool, 4> bKeysHeldPrior = {};
	bool bShouldLock = false;

	if( !g_Vars.globals.m_bShotAutopeek ) {
		bKeysHeldPrior[ 0 ] = cmd->buttons & IN_MOVELEFT;
		bKeysHeldPrior[ 1 ] = cmd->buttons & IN_MOVERIGHT;
		bKeysHeldPrior[ 2 ] = cmd->buttons & IN_FORWARD;
		bKeysHeldPrior[ 3 ] = cmd->buttons & IN_BACK;
	}
	else {
		auto CheckKeyState = [&] ( int keyIdx, int key ) -> bool {
			bool bRet = false;

			// still holding that same movement key? lock him in place
			if( bKeysHeldPrior[ keyIdx ] && ( cmd->buttons & key ) ) {
				bRet = true;
			}

			// let go of the key, don't lock him anymore it 
			if( !( cmd->buttons & key ) ) {
				bKeysHeldPrior[ keyIdx ] = false;
				bRet = false;
			}

			return bRet;
		};

		// update lock state
		if( !bShouldLock )
			bShouldLock = CheckKeyState( 0, IN_MOVELEFT );

		if( !bShouldLock )
			bShouldLock = CheckKeyState( 1, IN_MOVERIGHT );

		if( !bShouldLock )
			bShouldLock = CheckKeyState( 2, IN_FORWARD );

		if( !bShouldLock )
			bShouldLock = CheckKeyState( 3, IN_BACK );
	}

	// lock player if they are still holding the same movement key
	bool bReadyToFireAgain = pLocal->CanShoot( true, false );

	// can we dt / shift again?
	if( g_TickbaseController.IsCharged( ) || g_Vars.rage.double_tap_bind.enabled )
		bReadyToFireAgain = g_TickbaseController.CanShift( true );

	if( !bReadyToFireAgain && bShouldLock )
		m_bRetrack = true;

	if( !g_Vars.globals.m_bLockAutoPeek )
		bShouldLock = false;

	if( g_Vars.misc.autopeek && m_bRetrack && !m_vecAutoPeekPos.IsZero( ) && ( pLocal->m_fFlags( ) & FL_ONGROUND ) && ( pUnpredictedData->flags & FL_ONGROUND ) && !( cmd->buttons & IN_JUMP ) && !g_Vars.globals.m_bDidRagebot && !g_TickbaseController.m_bDelay ) {
		Vector vecPosDelta = m_vecAutoPeekPos - pLocal->GetAbsOrigin( );

		float flLength = vecPosDelta.Length2D( );

		// too far away from initial spot
		if( flLength > 5.f ) {
			m_vecMovementAngles = vecPosDelta.ToEulerAngles( );

			cmd->forwardmove = flLength * 20.f;
			cmd->sidemove = 0.0f;

			if( bReadyToFireAgain )
				g_Vars.globals.m_bShotAutopeek = false;
		}
		// we reached our destination) that will be $5 sir
		else {
			if( ( !bReadyToFireAgain && bShouldLock ) ) {
				g_Movement.MovementControl( cmd, pLocal->m_flDuckAmount( ) ? 3.25f : 1.1f, false );
			}

			if( bReadyToFireAgain || !bShouldLock )
				g_Vars.globals.m_bShotAutopeek = false;
		}
	}
	else {
		m_bRetrack = false;
	}
}

void Movement::InstantStop( CUserCmd *cmd ) {
	// shift умножает ускорение на 0.52 (CCSGameMovement::Accelerate), а встречный стрейф - тоже ускорение
	cmd->buttons &= ~IN_SPEED;

	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	auto weapon = ( C_WeaponCSBaseGun * )pLocal->m_hActiveWeapon( ).Get( );
	if( !weapon )
		return;

	auto weapon_data = weapon->GetCSWeaponData( ).Xor( );
	if( !weapon_data )
		return;

	Vector vecVelocity = pLocal->m_vecVelocity( );

	Vector vecVelocityAngle;
	Math::VectorAngles( vecVelocity, vecVelocityAngle );
	QAngle angVelocityAngle = QAngle( vecVelocityAngle.x, vecVelocityAngle.y, vecVelocityAngle.z );

	float flSpeed = vecVelocity.Length( );
	if( flSpeed > 15.f ) {
		StopToSpeed( 0.f, cmd );
	}
	else {
		cmd->sidemove = cmd->forwardmove = 0.f;
	}
}

void Movement::AutoStop( CUserCmd *cmd ) {
	// shift умножает ускорение на 0.52 (CCSGameMovement::Accelerate), а встречный стрейф - тоже ускорение
	cmd->buttons &= ~IN_SPEED;

	C_CSPlayer *const pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	C_WeaponCSBaseGun *weapon = ( C_WeaponCSBaseGun * )pLocal->m_hActiveWeapon( ).Get( );
	if( !weapon )
		return;

	// get our velocity.
	Vector vecVelocity = pLocal->m_vecVelocity( );
	float flVelocityLength = vecVelocity.Length( );

	// get our max weapon speed.
	float flWeaponMaxSpeed = weapon->GetMaxSpeed( ) * 0.3000001;
	StopToSpeed( flWeaponMaxSpeed - 0.1f, cmd );
}

void Movement::accelerate( const Vector &wishdir, float wishspeed, float accel, CMoveData *mv, C_CSPlayer *ent ) {
	auto stored_accel = accel;
	auto currentspeed = mv->m_vecVelocity.Dot( wishdir );
	const float addspeed = wishspeed - currentspeed;

	if( addspeed <= 0 )
		return;

	if( currentspeed < 0 )
		currentspeed = 0;

	const auto is_ducking = mv->m_nButtons & IN_DUCK || ent->m_flDuckAmount( ) > 0.55f || ent->m_fFlags( ) & FL_DUCKING;
	const auto is_walking = ( mv->m_nButtons & IN_SPEED ) != 0 && !is_ducking;

	constexpr auto max_speed = 250.0f;
	auto acceleration_scale = std::max( max_speed, wishspeed );
	auto goal_speed = acceleration_scale;

	auto is_slow_sniper_scoped = false;

	const auto wpn = ( C_WeaponCSBaseGun * )ent->m_hActiveWeapon( ).Get( );
	static auto sv_accelerate_use_weapon_speed = g_pCVar->FindVar( XorStr( "sv_accelerate_use_weapon_speed" ) );
	if( sv_accelerate_use_weapon_speed->GetInt( ) && wpn ) {
		// своё имя: внутренняя max_speed затеняла внешнюю (250), и max_speed / max_speed всегда давало 1
		const float flWeaponMaxSpeed = wpn->GetCSWeaponData( ).IsValid( )
			? ( ent->m_bIsScoped( ) ? wpn->GetCSWeaponData( )->m_flMaxSpeed2 : wpn->GetCSWeaponData( )->m_flMaxSpeed )
			: 260.f;

		is_slow_sniper_scoped = ( wpn->m_zoomLevel( ) > 0 && wpn->GetCSWeaponData( )->m_iZoomLevels[ 0 ] > 1 && ( flWeaponMaxSpeed * 0.52f ) < 110.0f );

		// CCSGameMovement::Accelerate: MIN( 1, GetMaxSpeed() / 250 )
		goal_speed *= std::min( 1.0f, flWeaponMaxSpeed / max_speed );

		if( ( !is_ducking && !is_walking ) || ( ( is_walking || is_ducking ) && is_slow_sniper_scoped ) )
			acceleration_scale *= std::min( 1.0f, flWeaponMaxSpeed / max_speed );
	}

	if( is_ducking ) {
		if( !is_slow_sniper_scoped )
			acceleration_scale *= 0.34f;

		goal_speed *= 0.34f;
	}

	if( is_walking ) {
		if( !is_slow_sniper_scoped )
			acceleration_scale *= 0.52f;

		goal_speed *= 0.52f;
	}

	if( is_walking && currentspeed > ( goal_speed - 5 ) )
		stored_accel *= std::clamp( 1.0f - ( std::max( 0.0f, currentspeed - ( goal_speed - 5 ) ) / std::max( 0.0f, goal_speed - ( goal_speed - 5 ) ) ), 0.0f, 1.0f );

	auto accelspeed = stored_accel * g_pGlobalVars->interval_per_tick * acceleration_scale * ent->m_surfaceFriction( );

	if( accelspeed > addspeed )
		accelspeed = addspeed;

	mv->m_vecVelocity += ( wishdir * accelspeed );
}

__forceinline float VectorNormalize( Vector &vec ) {
	const auto sqrlen = vec.LengthSquared( ) + 1.0e-10f;
	float invlen;
	const auto xx = _mm_load_ss( &sqrlen );
	auto xr = _mm_rsqrt_ss( xx );
	auto xt = _mm_mul_ss( xr, xr );
	xt = _mm_mul_ss( xt, xx );
	xt = _mm_sub_ss( _mm_set_ss( 3.f ), xt );
	xt = _mm_mul_ss( xt, _mm_set_ss( 0.5f ) );
	xr = _mm_mul_ss( xr, xt );
	_mm_store_ss( &invlen, xr );
	vec.x *= invlen;
	vec.y *= invlen;
	vec.z *= invlen;
	return sqrlen * invlen;
}

__forceinline float VectorNormalize( float *v ) {
	return VectorNormalize( *( reinterpret_cast< Vector * >( v ) ) );
}

void Movement::walk_move( CMoveData *mv, C_CSPlayer *ent ) {
	static ConVar *sv_accelerate = g_Vars.sv_accelerate;

	Vector forward, right, up;
	Math::AngleVectors( mv->m_vecViewAngles, forward, right, up ); // Determine movement angles

	// Copy movement amounts
	auto const fmove = mv->m_flForwardMove;
	auto const smove = mv->m_flSideMove;

	forward.z = right.z = 0.f;

	VectorNormalize( forward );
	VectorNormalize( right );

	Vector wishvel( forward.x * fmove + right.x * smove, forward.y * fmove + right.y * smove, 0.f );

	auto wishdir = wishvel;
	auto wishspeed = VectorNormalize( wishdir );

	// clamp to server defined max speed
	if( wishspeed != 0 && ( wishspeed > mv->m_flMaxSpeed ) ) {
		wishvel = wishvel * ( mv->m_flMaxSpeed / wishspeed );
		wishspeed = mv->m_flMaxSpeed;
	}

	// Set pmove velocity
	mv->m_vecVelocity[ 2 ] = 0;
	accelerate( wishdir, wishspeed, sv_accelerate->GetFloat( ), mv, ent );
	mv->m_vecVelocity[ 2 ] = 0;

	// Additional max speed clamp to keep us from going faster than allowed while turning
	if( mv->m_vecVelocity.LengthSquared( ) > mv->m_flMaxSpeed * mv->m_flMaxSpeed ) {
		float fRatio = mv->m_flMaxSpeed / mv->m_vecVelocity.Length( );
		mv->m_vecVelocity *= fRatio;
	}

	// If we made it all the way, then copy trace end as new player position.
	mv->m_outWishVel += wishdir * wishspeed;
}

void Movement::friction( CMoveData *mv, C_CSPlayer *player ) {
	// Calculate speed
	const auto speed = mv->m_vecVelocity.Length( );

	// If too slow, return
	if( speed < 0.1f )
		return;

	auto drop = 0.f;

	// apply ground friction
	if( player->m_hGroundEntity( ).Get( ) != 0 )  // On an entity that is the ground
	{
		const auto friction = g_Vars.sv_friction->GetFloat( ) * player->m_surfaceFriction( );

		// Bleed off some speed, but if we have less than the bleed
		//  threshold, bleed the threshold amount.
		const auto control = ( speed < g_Vars.sv_stopspeed->GetFloat( ) ) ? g_Vars.sv_stopspeed->GetFloat( ) : speed;

		// Add the amount to the drop amount.
		drop += control * friction * g_pGlobalVars->interval_per_tick;
	}

	// scale the velocity
	auto newspeed = speed - drop;
	if( newspeed < 0 )
		newspeed = 0;

	if( newspeed != speed ) {
		// Determine proportion of old speed we are using.
		newspeed /= speed;
		// Adjust velocity according to proportion.
		mv->m_vecVelocity *= newspeed;
	}

	mv->m_outWishVel -= mv->m_vecVelocity * ( 1.f - newspeed );
}

void Movement::friction( Vector &vecVelocity, C_CSPlayer *player ) {
	// Calculate speed
	const auto speed = vecVelocity.Length( );

	// If too slow, return
	if( speed < 0.1f )
		return;

	auto drop = 0.f;

	// apply ground friction
	if( player->m_hGroundEntity( ).Get( ) != 0 )  // On an entity that is the ground
	{
		const auto friction = g_Vars.sv_friction->GetFloat( ) * player->m_surfaceFriction( );

		// Bleed off some speed, but if we have less than the bleed
		//  threshold, bleed the threshold amount.
		const auto control = ( speed < g_Vars.sv_stopspeed->GetFloat( ) ) ? g_Vars.sv_stopspeed->GetFloat( ) : speed;

		// Add the amount to the drop amount.
		drop += control * friction * g_pGlobalVars->interval_per_tick;
	}

	// scale the velocity
	auto newspeed = speed - drop;
	if( newspeed < 0 )
		newspeed = 0;

	if( newspeed != speed ) {
		// Determine proportion of old speed we are using.
		newspeed /= speed;
		// Adjust velocity according to proportion.
		vecVelocity *= newspeed;
	}
}

void Movement::check_parameters( CMoveData *mv, C_CSPlayer *player ) {
	const auto speed_squared = ( mv->m_flForwardMove * mv->m_flForwardMove ) +
		( mv->m_flSideMove * mv->m_flSideMove ) +
		( mv->m_flUpMove * mv->m_flUpMove );

	// Slow down by the speed factor
	auto speed_factor = 1.0f;
	if( player->m_SurfaceData( ) )
		speed_factor = player->m_SurfaceData( )->game.maxSpeedFactor;

	const auto weapon = ( C_WeaponCSBaseGun * )player->m_hActiveWeapon( ).Get( );
	float max_speed = weapon && weapon->GetCSWeaponData( ).IsValid( ) ? player->m_bIsScoped( ) ? weapon->GetCSWeaponData( )->m_flMaxSpeed2 : weapon->GetCSWeaponData( )->m_flMaxSpeed : 260.f;
	mv->m_flMaxSpeed = weapon ? max_speed : mv->m_flClientMaxSpeed;

	mv->m_flMaxSpeed *= speed_factor;

	// CCSGameMovement::CheckParameters: на земле скорость режется m_flVelocityModifier (замедление от попадания)
	if( player->m_fFlags( ) & FL_ONGROUND )
		mv->m_flMaxSpeed *= player->m_flVelocityModifier( );

	// stamina slowing factor
	if( player->m_flStamina( ) > 0 ) {
		auto speed_scale = std::clamp( 1.0f - player->m_flStamina( ) / 100.f, 0.f, 1.f );
		speed_scale *= speed_scale;	// square the scale factor so that it correlates more closely with the jump penalty (i.e. a 50% stamina jumps .25 * normal height)
		mv->m_flMaxSpeed *= speed_scale;
	}

	// Same thing but only do the sqrt if we have to.
	if( ( speed_squared != 0.0 ) && ( speed_squared > mv->m_flMaxSpeed * mv->m_flMaxSpeed ) ) {
		const auto ratio = mv->m_flMaxSpeed / sqrt( speed_squared );
		mv->m_flForwardMove *= ratio;
		mv->m_flSideMove *= ratio;
		mv->m_flUpMove *= ratio;
	}
}

void Movement::StopToSpeed( float speed, CMoveData *mv, C_CSPlayer *player ) {
	friction( mv, player );
	check_parameters( mv, player );

	auto mv_cp = *mv;
	walk_move( &mv_cp, player );
	if( mv_cp.m_vecVelocity.Length2D( ) <= speed )
		return;

	if( mv->m_vecVelocity.Length2D( ) > speed ) {
		Vector ang{};
		Math::VectorAngles( mv->m_vecVelocity * -1.f, ang );
		ang.y = Math::AngleNormalize( mv->m_vecViewAngles.y - ang.y );

		Vector forward{};
		Math::AngleVectors( QAngle( ang.x, ang.y, ang.z ), forward );
		forward = Vector( forward.x, forward.y, 0.f ).Normalized( );

		const auto target_move_len = std::min( mv->m_vecVelocity.Length2D( ) - speed, mv->m_flMaxSpeed );
		mv->m_flForwardMove = forward.x * target_move_len;
		mv->m_flSideMove = forward.y * target_move_len;
		mv->m_flUpMove = 0.f;
		return;
	}

	Vector forward, right, up;
	Math::AngleVectors( mv->m_vecViewAngles, forward, right, up );
	forward.z = right.z = 0.f;

	VectorNormalize( forward );
	VectorNormalize( right );

	Vector wishdir( forward.x * mv->m_flForwardMove + right.x * mv->m_flSideMove, forward.y * mv->m_flForwardMove + right.y * mv->m_flSideMove, 0.f );
	VectorNormalize( wishdir );

	const auto currentspeed = mv->m_vecVelocity.Dot( wishdir );
	const auto target_accelspeed = speed - mv->m_vecVelocity.Length2D( );
	const auto target_move_len = std::min( currentspeed + target_accelspeed, mv->m_flMaxSpeed );

	const auto speed_squared = ( mv->m_flForwardMove * mv->m_flForwardMove ) + ( mv->m_flSideMove * mv->m_flSideMove ) + ( mv->m_flUpMove * mv->m_flUpMove );
	const auto ratio = target_move_len / sqrt( speed_squared );
	if( ratio < 1.f ) {
		mv->m_flForwardMove *= ratio;
		mv->m_flSideMove *= ratio;
		mv->m_flUpMove *= ratio;
	}
}

void Movement::StopToSpeed( float speed, CUserCmd *cmd ) {
	C_CSPlayer *const pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	CMoveData data = g_pGameMovement->SetupMove( pLocal, cmd );
	StopToSpeed( speed, &data, pLocal );

	cmd->forwardmove = data.m_flForwardMove;
	cmd->sidemove = data.m_flSideMove;

	walk_move( &data, pLocal );
}

void Movement::FakeDuck( bool *bSendPacket, CUserCmd *cmd, bool bForce ) {
	g_Vars.globals.m_bFakeWalking = false;
	g_AntiAim.m_bAllowFakeWalkFlick = false;

	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	if( pLocal->IsDead( ) )
		return;

	if( !g_Vars.misc.slow_walk )
		return;

	bool bEnabledKeybind = g_Vars.misc.slow_walk_bind.enabled || bForce;

	static bool bWasMovingBeforeFakewalk = false;
	if( !bEnabledKeybind ) {
		bWasMovingBeforeFakewalk = pLocal->m_vecVelocity( ).Length2D( ) >= 0.1f;
		return;
	}

	if( bWasMovingBeforeFakewalk ) {
		if( pLocal->m_vecVelocity( ).Length2D( ) >= 0.1f ) {
			InstantStop( cmd );
			g_Movement.m_bModifiedMovementBeforePrediction = true;
		}
		else {
			bWasMovingBeforeFakewalk = false;
		}

		return;
	}

	if( !( pLocal->m_fFlags( ) & FL_ONGROUND ) )
		return;

	if( cmd->buttons & IN_JUMP )
		return;

	if( !g_Movement.PressingMovementKeys( cmd ) )
		return;

	if( !( pLocal->m_fFlags( ) & FL_ONGROUND ) )
		return;

	auto pWeapon = ( C_WeaponCSBaseGun * )pLocal->m_hActiveWeapon( ).Get( );
	if( !pWeapon )
		return;

	cmd->buttons &= ~IN_SPEED;

	auto vecPredictedVelocity = pLocal->m_vecVelocity( );
	int nTicksToStop;
	for( nTicksToStop = 0; nTicksToStop < g_Vars.sv_maxusrcmdprocessticks->GetInt( ) - 2; ++nTicksToStop ) {
		if( vecPredictedVelocity.Length2D( ) < 0.1f )
			break;

		// predict velocity into the future
		if( vecPredictedVelocity.Length( ) >= 0.1f ) {
			const float flSpeedToStop = std::max< float >( vecPredictedVelocity.Length( ), g_Vars.sv_stopspeed->GetFloat( ) );
			const float flStopTime = std::max< float >( g_pGlobalVars->interval_per_tick, g_pGlobalVars->frametime );
			vecPredictedVelocity *= std::max< float >( 0.f, vecPredictedVelocity.Length( ) - g_Vars.sv_friction->GetFloat( ) * flSpeedToStop * flStopTime / vecPredictedVelocity.Length( ) );
		}
	}

	const int nTicksTillFlick = TIME_TO_TICKS( g_ServerAnimations.m_uServerAnimations.m_flLowerBodyRealignTimer ) - ( pLocal->m_nTickBase( ) + 2 );
	int nMaxChokeTicks = g_Vars.sv_maxusrcmdprocessticks->GetInt( ) - 2;
	if( nMaxChokeTicks > nTicksTillFlick )
		nMaxChokeTicks = nTicksTillFlick;

	const bool bFlickThisTick = TICKS_TO_TIME( pLocal->m_nTickBase( ) ) > g_ServerAnimations.m_uServerAnimations.m_flLowerBodyRealignTimer || g_ServerAnimations.m_uServerAnimations.m_bRealignBreaker;
	const int nTicksLeftToStop = nMaxChokeTicks - g_pClientState->m_nChokedCommands( );

	g_FakeLag.m_iAwaitingChoke = nMaxChokeTicks;

	if( !( bFlickThisTick || ( TICKS_TO_TIME( pLocal->m_nTickBase( ) ) + g_pGlobalVars->interval_per_tick ) > g_ServerAnimations.m_uServerAnimations.m_flLowerBodyRealignTimer ) ) {
		if( g_pClientState->m_nChokedCommands( ) < nMaxChokeTicks || nTicksToStop ) {
			*bSendPacket = false;
		}

		if( !pLocal->m_vecVelocity( ).Length2D( ) && !*bSendPacket && g_pClientState->m_nChokedCommands( ) > nMaxChokeTicks ) {
			*bSendPacket = true;
		}
	}

	if( nTicksToStop > nTicksLeftToStop - 1 || !g_pClientState->m_nChokedCommands( ) ) {
		InstantStop( cmd );

		g_Movement.m_bModifiedMovementBeforePrediction = true;
		//g_AntiAim.m_bAllowFakeWalkFlick = true;
	}

	g_Vars.globals.m_bFakeWalking = true;
}

void Movement::MovementControl( CUserCmd *cmd, float velocity, bool yep, bool bUseCmdMove ) {
	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	if( velocity <= 0.52f )
		cmd->buttons |= IN_SPEED;
	else
		cmd->buttons &= ~IN_SPEED;

	auto pUnpredictedData = g_Prediction.get_initial_vars( );

	const float forwardmove = bUseCmdMove ? cmd->forwardmove : pUnpredictedData->forwardmove;
	const float sidemove = bUseCmdMove ? cmd->sidemove : pUnpredictedData->sidemove;

	float movement_speed = std::sqrtf( forwardmove * forwardmove + sidemove * sidemove );
	if( movement_speed > 0.f ) {
		if( movement_speed > velocity ) {
			float mov_speed = pUnpredictedData->velocity.Length2D( );
			if( ( ( velocity + 1.0f ) <= mov_speed ) && yep ) {
				cmd->forwardmove = 0.0f;
				cmd->sidemove = 0.0f;
			}
			else {
				float forward_ratio = forwardmove / movement_speed;
				float side_ratio = sidemove / movement_speed;

				cmd->forwardmove = forward_ratio * velocity;
				cmd->sidemove = side_ratio * velocity;
			}
		}
	}
}

void Movement::DefuseTheBomb( CUserCmd *cmd ) {
	//if( !g_Vars.rage.enabled || !g_Vars.rage.key.enabled )
	//	return;

	if( !cmd )
		return;

	// note; m_bNearBomb is consumed by AntiAim::Think, so it must never be left
	// stale from a previous tick on any of the early-out paths below.
	if( !( cmd->buttons & IN_USE ) ) {
		m_bNearBomb = false;
		return;
	}

	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal ) {
		m_bNearBomb = false;
		return;
	}

	if( pLocal->m_iTeamNum( ) != TEAM_CT || pLocal->m_bInNoDefuseArea( ) ) {
		m_bNearBomb = false;
		return;
	}

	m_bNearBomb = false;

	C_PlantedC4 *plantedC4 = nullptr;
	for( int i = 64; i < g_pEntityList->GetHighestEntityIndex( ); ++i ) {
		if( i == 2047 )
			continue;

		const auto entity = reinterpret_cast< C_BaseEntity * >( g_pEntityList->GetClientEntity( i ) );
		if( !entity )
			continue;

		if( !entity->GetClientClass( ) )
			continue;

		if( entity->GetClientClass( )->m_ClassID == CPlantedC4 ) {
			plantedC4 = reinterpret_cast< C_PlantedC4 * >( entity );
			break;
		}
	}

	if( plantedC4 != nullptr ) {
		if( !plantedC4->m_bBombDefused( ) ) {
			const auto &vecOrigin = plantedC4->m_vecOrigin( );

			const float distance = pLocal->m_vecOrigin( ).Distance( vecOrigin );

			if( distance <= 70.f ) {
				//const auto angle = Math::CalcAngle( vecOrigin, eyePos );

				//cmd->viewangles.x = angle.x;
				//cmd->viewangles.y = angle.y;
				//cmd->viewangles.z = angle.z;

				//cmd->viewangles.Clamp( );

				m_bNearBomb = true;

				// cache it so AntiAim::Think can aim at the bomb to start the
				// defuse without running a second entity scan.
				m_vecNearBombOrigin = vecOrigin;
			}
		}
	}
}

void Movement::MouseDelta( CUserCmd *cmd ) {
	if( !cmd )
		return;

	static QAngle angDelta{ };

	// compute current delta
	QAngle delta = cmd->viewangles - angDelta;
	delta.Clamp( );

	static ConVar *sensitivity = g_pCVar->FindVar( XorStr( "sensitivity" ) );

	if( !sensitivity )
		return;

	if( delta.x != 0.f ) {
		static ConVar *m_pitch = g_pCVar->FindVar( XorStr( "m_pitch" ) );

		if( !m_pitch )
			return;

		int nFinaldy = static_cast< int >( ( delta.x / m_pitch->GetFloat( ) ) / sensitivity->GetFloat( ) );
		if( nFinaldy <= 32767 ) {
			if( nFinaldy >= -32768 ) {
				if( nFinaldy >= 1 || nFinaldy < 0 ) {
					if( nFinaldy <= -1 || nFinaldy > 0 )
						nFinaldy = nFinaldy;
					else
						nFinaldy = -1;
				}
				else {
					nFinaldy = 1;
				}
			}
			else {
				nFinaldy = 32768;
			}
		}
		else {
			nFinaldy = 32767;
		}

		cmd->mousedy = static_cast< short >( nFinaldy );
	}

	if( delta.y != 0.f ) {
		static ConVar *m_yaw = g_pCVar->FindVar( XorStr( "m_yaw" ) );

		if( !m_yaw )
			return;

		int nFinaldx = static_cast< int >( ( delta.y / m_yaw->GetFloat( ) ) / sensitivity->GetFloat( ) );
		if( nFinaldx <= 32767 ) {
			if( nFinaldx >= -32768 ) {
				if( nFinaldx >= 1 || nFinaldx < 0 ) {
					if( nFinaldx <= -1 || nFinaldx > 0 )
						nFinaldx = nFinaldx;
					else
						nFinaldx = -1;
				}
				else {
					nFinaldx = 1;
				}
			}
			else {
				nFinaldx = 32768;
			}
		}
		else {
			nFinaldx = 32767;
		}

		cmd->mousedx = static_cast< short >( nFinaldx );
	}

	// store current viewangles
	angDelta = cmd->viewangles;
}

bool Movement::PlayerMove( C_CSPlayer *pEntity, Vector &vecOrigin, Vector &vecVelocity, int &fFlags, bool bOnGround ) {
	if( !pEntity )
		return false;

	if( !( fFlags & FL_ONGROUND ) )
		vecVelocity.z -= TICKS_TO_TIME( g_Vars.sv_gravity->GetFloat( ) );
	else if( ( pEntity->m_fFlags( ) & FL_ONGROUND ) && !bOnGround )
		vecVelocity.z = g_Vars.sv_jump_impulse->GetFloat( );

	const auto src = vecOrigin;
	auto end = src + vecVelocity * g_pGlobalVars->interval_per_tick;

	Ray_t r;
	r.Init( src, end, pEntity->OBBMins( ), pEntity->OBBMaxs( ) );

	CGameTrace t{ };
	CTraceFilter filter;
	filter.pSkip = pEntity;

	g_pEngineTrace->TraceRay( r, MASK_PLAYERSOLID, &filter, &t );

	if( t.fraction != 1.f ) {
		for( auto i = 0; i < 2; i++ ) {
			vecVelocity -= t.plane.normal * vecVelocity.Dot( t.plane.normal );

			const auto Dot = vecVelocity.Dot( t.plane.normal );
			if( Dot < 0.f )
				vecVelocity -= Vector( Dot * t.plane.normal.x,
									   Dot * t.plane.normal.y, Dot * t.plane.normal.z );

			end = t.endpos + vecVelocity * TICKS_TO_TIME( 1.f - t.fraction );

			r.Init( t.endpos, end, pEntity->OBBMins( ), pEntity->OBBMaxs( ) );
			g_pEngineTrace->TraceRay( r, MASK_PLAYERSOLID, &filter, &t );

			if( t.fraction == 1.f )
				break;
		}
	}

	vecOrigin = end = t.endpos;
	end.z -= 2.f;

	r.Init( vecOrigin, end, pEntity->OBBMins( ), pEntity->OBBMaxs( ) );
	g_pEngineTrace->TraceRay( r, MASK_PLAYERSOLID, &filter, &t );

	fFlags &= ~FL_ONGROUND;

	if( t.DidHit( ) && t.plane.normal.z > .7f )
		fFlags |= FL_ONGROUND;

	return false;
}

void GetWallCorner( const Vector &vecStart, Vector &vecEnd, float flRange ) {
	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	Ray_t ray;
	if( flRange > 0.0f )
		ray.Init( vecStart, vecEnd, Vector( -flRange, -flRange, -flRange ), Vector( flRange, flRange, flRange ) );
	else
		ray.Init( vecStart, vecEnd );

	CTraceFilter filter;
	filter.pSkip = pLocal;

	CGameTrace trace;
	g_pEngineTrace->TraceRay( ray, 0x46004003u, &filter, &trace );
	if( trace.fraction <= 0.99f ) {
		vecEnd = vecStart + ( ( vecEnd - vecStart ) * trace.fraction );
	}
}

// credits to raxer	
bool Movement::IsPeeking( C_CSPlayer *pEntity, CUserCmd *cmd ) {
	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal || g_TickbaseController.m_bShifting )
		return false;

	const float flSpeed = pLocal->GetAbsVelocity( ).Length2D( );
	bool bDont = flSpeed < 3.25f || m_bRetrack;

	if( bDont ) {
		m_flOldSpeed = pLocal->GetAbsVelocity( ).Length2D( );
	}
	else {
		//if( m_flOldSpeed >= 0.f ) {
		//	if( std::fabs( flSpeed - m_flOldSpeed ) >= 35.f && !pEntity ) {
		//		m_flOldSpeed = -1.f;
		//		return true;
		//	}
		//}
	}

	Vector vecMitigatedVelocity = pLocal->m_vecVelocity( );
	Vector vecOriginalEyePos = pLocal->GetEyePosition( );
	Vector vecPredictedEyePos = pLocal->GetEyePosition( );
	auto fFlags = pLocal->m_fFlags( );

	int nTraceTicks = 4;

	if( vecMitigatedVelocity.Length2D( ) <= 150.f ) {
		nTraceTicks = 6;
	}

	vecMitigatedVelocity = pLocal->m_vecVelocity( ) * ( g_pGlobalVars->interval_per_tick * nTraceTicks );
	vecPredictedEyePos = vecOriginalEyePos + vecMitigatedVelocity;

	//	g_pDebugOverlay->AddBoxOverlay( vecPredictedEyePos, { -2,-2,-2 }, { 2,2,2 }, {}, 255, 255, 255, 100, g_pGlobalVars->interval_per_tick * 4 );

	if( pEntity && pEntity->m_CachedBoneData( ).Base( ) ) {
		Autowall::FireBulletData data{ };
		data.m_bPenetration = true;
		data.m_TargetPlayer = pEntity;
		data.m_Player = pLocal;
		data.m_vecStart = vecPredictedEyePos;

		// try autowalling pelvis bone
		matrix3x4_t vecTargetBone = pEntity->m_CachedBoneData( ).Base( )[ 0 ];
		Vector vecTargetPosition = Vector( vecTargetBone[ 0 ][ 3 ], vecTargetBone[ 1 ][ 3 ], vecTargetBone[ 2 ][ 3 ] );

		auto fnCanHitFromSide = [&] ( int nSide ) -> bool {
			data.m_vecStart = vecPredictedEyePos;

			if( nSide == 0 ) {
				data.m_vecDirection = vecTargetPosition - vecPredictedEyePos;
			}
			else {
				if( !bDont || !g_TickbaseController.IsCharged( ) )
					return false;

				// calculate direction from bestOrigin to our origin
				auto flYaw = Math::CalcAngle( pEntity->m_vecOrigin( ), pLocal->m_vecOrigin( ) );

				Vector vecForward, vecRight, vecUp;
				Math::AngleVectors( flYaw, vecForward, vecRight, vecUp );

				if( nSide == 1 ) {
					data.m_vecStart += ( vecRight * 40.f );
				}
				else {
					data.m_vecStart -= ( vecRight * 40.f );
				}

				data.m_vecDirection = vecTargetPosition - data.m_vecStart;
			}

			data.m_vecDirection.Normalize( );

			return Autowall::FireBullets( &data ) > 1.f;
		};

		// yep
		if( fnCanHitFromSide( 0 ) || fnCanHitFromSide( 1 ) || fnCanHitFromSide( 2 ) ) {
			return true;
		}
		// no luck, try head bone
		else {
			vecTargetBone = pEntity->m_CachedBoneData( ).Base( )[ 8 ];
			vecTargetPosition = Vector( vecTargetBone[ 0 ][ 3 ], vecTargetBone[ 1 ][ 3 ], vecTargetBone[ 2 ][ 3 ] );

			auto fnCanHitFromSide = [&] ( int nSide ) -> bool {
				data.m_vecStart = vecPredictedEyePos;
				if( nSide == 0 ) {
					data.m_vecDirection = vecTargetPosition - data.m_vecStart;
				}
				else {
					if( !bDont || !g_TickbaseController.IsCharged( ) )
						return false;

					// calculate direction from bestOrigin to our origin
					auto flYaw = Math::CalcAngle( pEntity->m_vecOrigin( ), pLocal->m_vecOrigin( ) );

					Vector vecForward, vecRight, vecUp;
					Math::AngleVectors( flYaw, vecForward, vecRight, vecUp );

					if( nSide == 1 ) {
						data.m_vecStart += ( vecRight * 40.f );
					}
					else {
						data.m_vecStart -= ( vecRight * 40.f );
					}

					data.m_vecDirection = vecTargetPosition - data.m_vecStart;
				}

				data.m_vecDirection.Normalize( );

				return Autowall::FireBullets( &data ) > 1.f;
			};

			// maybe perhap head do damage
			if( fnCanHitFromSide( 0 ) || fnCanHitFromSide( 1 ) || fnCanHitFromSide( 2 ) ) {
				return true;
			}
		}
	}

	if( bDont )
		return false;

	std::vector<std::pair<C_CSPlayer *, int>> vecPlayers{ };
	for( int i = 1; i <= g_pGlobalVars->maxClients; ++i ) {
		const auto player = C_CSPlayer::GetPlayerByIndex( i );
		if( !player )
			continue;

		if( player->IsDead( ) || player->m_bGunGameImmunity( ) || player->IsDormant( ) || player == pEntity )
			continue;

		if( player->IsTeammate( pLocal ) )
			continue;

		vecPlayers.push_back( { player, i } );
	}

	if( pEntity ) {
		std::sort( vecPlayers.begin( ), vecPlayers.end( ), [&] ( const std::pair<C_CSPlayer *, int> &a, const std::pair<C_CSPlayer *, int> &b ) -> bool {
			if( !a.first || !b.first )
				return false;

			return a.first->m_vecOrigin( ).Distance( pEntity->m_vecOrigin( ) ) < b.first->m_vecOrigin( ).Distance( pEntity->m_vecOrigin( ) );
		} );
	}

	if( vecPlayers.size( ) > 3 ) {
		vecPlayers.resize( 3 );
	}

	//if( pEntity )
	//	vecPlayers.push_back( { pEntity, pEntity->m_entIndex } );

	// coz we are not peeking our crosshair target
	// let's try all the other ones too.
	for( auto &entry : vecPlayers ) {
		if( !entry.first || !entry.first->m_CachedBoneData( ).Base( ) )
			continue;

		Autowall::FireBulletData data{ };
		data.m_bPenetration = true;
		data.m_TargetPlayer = entry.first;
		data.m_Player = pLocal;
		data.m_vecStart = vecPredictedEyePos;

		// try autowalling pelvis bone
		matrix3x4_t vecTargetBone = entry.first->m_CachedBoneData( ).Base( )[ 0 ];
		Vector vecTargetPosition = Vector( vecTargetBone[ 0 ][ 3 ], vecTargetBone[ 1 ][ 3 ], vecTargetBone[ 2 ][ 3 ] );

		data.m_vecDirection = vecTargetPosition - vecPredictedEyePos;
		data.m_vecDirection.Normalize( );

		// yep
		if( Autowall::FireBullets( &data ) > 1.f ) {
			return true;
		}
		// no luck, try head bone
		else {
			vecTargetBone = entry.first->m_CachedBoneData( ).Base( )[ 8 ];
			vecTargetPosition = Vector( vecTargetBone[ 0 ][ 3 ], vecTargetBone[ 1 ][ 3 ], vecTargetBone[ 2 ][ 3 ] );

			data.m_vecDirection = vecTargetPosition - vecPredictedEyePos;
			data.m_vecDirection.Normalize( );

			// maybe perhap head do damage
			if( Autowall::FireBullets( &data ) > 1.f ) {
				return true;
			}
		}
	}

	//if( m_flOldSpeed >= 0.f ) {
	//	if( std::fabs( flSpeed - m_flOldSpeed ) >= 35.f ) {
	//		m_flOldSpeed = -1.f;
	//		return true;
	//	}
	//}

	return false;
}

bool Movement::IsPeekingEx( C_CSPlayer *pEntity, CUserCmd *cmd ) {
	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal || !g_Vars.rage.fake_lag_peeking )
		return false;

	if( !pEntity )
		return false;

	networked_vars_t *pNetworkedVars = g_Prediction.get_initial_vars( );
	if( !pNetworkedVars )
		return false;

	// get postitions
	Vector vecStart = pLocal->GetEyePosition( );
	Vector vecEndFirst = vecStart;
	Vector vecEndSecond = vecStart;
	Vector vecVelocity = pLocal->m_vecVelocity( );

	const bool bInAir = ( !( pLocal->m_fFlags( ) & FL_ONGROUND ) || !( pNetworkedVars->flags & FL_ONGROUND ) || ( cmd->buttons & IN_JUMP ) );
	if( bInAir )
		return false;

	// lambda-callback to check if we can hit our target
	static auto CanHitTarget = [ & ]( C_CSPlayer *pEntity, Vector vecTracePos ) {
		// this will be overriden.
		bool bHitTarget = false;

		Vector vecEnemyHitboxPos = pEntity->GetHitboxPosition( HITBOX_PELVIS );

		// run simulated damage.
		Autowall::FireBulletData data{ };
		data.m_bPenetration = true;
		data.m_TargetPlayer = nullptr;
		data.m_Player = pLocal;
		data.m_vecStart = vecTracePos;

		data.m_vecDirection = vecEnemyHitboxPos - vecTracePos;
		data.m_vecDirection.Normalize( );

		Autowall::FireBullets( &data );

		int nDamage = static_cast< int >( data.m_flCurrentDamage );

		// we found damage, we can hit the enemy
		if( nDamage > 0 )
			bHitTarget = true;

		return bHitTarget;
	};

	// extrapolate our position
	// eye position + velocity * ticks (how far you want to extrapolate) * tickinterval
	vecEndFirst += ( vecVelocity * 2.f * g_pGlobalVars->interval_per_tick );
	vecEndSecond += ( vecVelocity * 4.f * g_pGlobalVars->interval_per_tick );

	// we can hit target from our current position, no point in tracing further
	if( CanHitTarget( pEntity, vecStart ) )
		return false;

	// if we aren't, check if we can hit them from our extrapolated positions
	const bool bFirstHit = CanHitTarget( pEntity, vecEndFirst );
	const bool bSecondHit = CanHitTarget( pEntity, vecEndSecond );

	// we can't hit our target 2 ticks ahead, but we can hit them 4 ticks ahead, we are peeking, stop lagging
	if( !bFirstHit && bSecondHit )
		return true;

	return false;
}