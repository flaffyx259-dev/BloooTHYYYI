#include "AntiAim.hpp"

#include "../Miscellaneous/Movement.hpp"
#include "../Rage/FakeLag.hpp"
#include "../Visuals/Visuals.hpp"
#include "ServerAnimations.hpp"
#include "EnginePrediction.hpp"
#include "TickbaseShift.hpp"
#include "../../SDK/Classes/Player.hpp"
#include "../../SDK/Classes/weapon.hpp"
#include "Autowall.hpp"
#include "../Visuals/EventLogger.hpp"
#include "../Miscellaneous/PlayerList.hpp"

AntiAim g_AntiAim;

// #define DMG_BASED_FREESTAND

C_CSPlayer *AntiAim::GetBestPlayer( bool distance ) {
	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return nullptr;

	float flBestDistance = FLT_MAX;
	float flBestFoV = FLT_MAX;
	C_CSPlayer *pBestPlayer = nullptr;

	QAngle angViewAngle;
	g_pEngine->GetViewAngles( angViewAngle );

	for( int i = 1; i <= g_pGlobalVars->maxClients; i++ ) {
		auto player = C_CSPlayer::GetPlayerByIndex( i );
		if( !player || player->IsDead( ) )
			continue;

		if( player->IsDormant( ) )
			continue;

		if( player->IsTeammate( pLocal ) )
			continue;

		if( g_PlayerList.GetSettings( player->GetSteamID( ) ).m_bAddToWhitelist && !g_Vars.misc.force_ignore_whitelist.enabled )
			continue;

		if( distance ) {
			float flDistance = ( player->m_vecOrigin( ) - pLocal->m_vecOrigin( ) ).LengthSquared( );
			if( flDistance < flBestDistance ) {
				flBestDistance = flDistance;
				pBestPlayer = player;
			}
		}
		else {
			float flFoV = Math::GetFov( angViewAngle, pLocal->GetEyePosition( ), player->m_vecOrigin( ) );

			if( flFoV < flBestFoV ) {
				flBestFoV = flFoV;
				pBestPlayer = player;
			}
		}
	}

	return pBestPlayer;
}

float AntiAim::UpdateFreestandPriority( float flLength, int nIndex, bool bDogshit ) {
	float flReturn = 1.f;

	// over 50% of the total length, prioritize this shit.
	if( nIndex > ( flLength * ( 0.5f ) ) )
		flReturn = ( 1.25f );

	// over 90% of the total length, prioritize this shit.
	if( nIndex > ( flLength * ( 0.75f ) ) )
		flReturn = bDogshit ? 1.25f : 1.5f;

	// over 90% of the total length, prioritize this shit.
	if( nIndex > ( flLength * ( 0.9f ) ) )
		flReturn = ( 2.f );

	return flReturn;
}

void AntiAim::AutoDirection( C_CSPlayer *pLocal ) {
	if( GetSide( ) != SIDE_MAX )
		return;

	// constants.
	const float flXored4 = ( 4.f );
	const float flXored32 = ( 32.f );
	const float flXored90 = ( 90.f );

	// get our view angles.
	QAngle view_angles;
	g_pEngine->GetViewAngles( view_angles );

	// get our shoot pos.
	Vector local_start = pLocal->GetEyePosition( );

	// best target.
	struct AutoTarget_t { float fov; C_CSPlayer *player; };
	AutoTarget_t target{ 180.f + 1.f, nullptr };

	// note; a DoEdgeAntiAim call used to run here and its result was discarded
	// (the consuming condition was commented out, and angEdgeAngle was left
	// uninitialized when no wall was found). dropped it - Think already calls
	// DoEdgeAntiAim separately and uses the result there.

	// iterate players.
	target.player = GetBestPlayer( );

	if( !target.player ) {
		// set angle to backwards.
		m_flAutoYaw = -1.f;
		m_flAutoDist = -1.f;
		m_bHasValidAuto = false;
		m_eAutoSide = ESide::SIDE_BACK;
		return;
	}

	// get target away angle.
	QAngle away = Math::CalcAngle( target.player->m_vecOrigin( ), pLocal->m_vecOrigin( ) );

	// construct vector of angles to test.
	std::vector< AdaptiveAngle > angles{ };
	if( !g_Vars.globals.m_bRunningExploit )
		angles.emplace_back( 180.f );
	angles.emplace_back( flXored90 );
	angles.emplace_back( -flXored90 );

	// start the trace at the enemy shoot pos.
	Vector start = target.player->GetEyePosition( );

	// see if we got any valid result.
	// if this is false the path was not obstructed with anything.
	bool valid{ false };

	// iterate vector of angles.
	for( auto it = angles.begin( ); it != angles.end( ); ++it ) {

		// compute the 'rough' estimation of where our head will be.
		Vector end{ local_start.x + std::cos( DEG2RAD( away.y + it->m_yaw ) ) * flXored32,
			local_start.y + std::sin( DEG2RAD( away.y + it->m_yaw ) ) * flXored32,
			local_start.z };

		// compute the direction.
		Vector dir = end - start;
		float len = dir.Normalize( );

		// should never happen.
		if( len <= 0.f )
			continue;

		// step thru the total distance, 4 units per step.
		for( float i{ 0.f }; i < len; i += flXored4 ) {
			// get the current step position.
			Vector point = start + ( dir * i );

			// get the contents at this point.
			int contents = g_pEngineTrace->GetPointContents_WorldOnly( point, MASK_SHOT_HULL );

			// contains nothing that can stop a bullet.
			if( !( contents & MASK_SHOT_HULL ) )
				continue;

			// append 'penetrated distance'.
			// note; the step index is explicitly narrowed, it used to be an
			// implicit float -> int conversion.
			it->m_dist += ( flXored4 * UpdateFreestandPriority( len, static_cast< int >( i ) ) );

			// mark that we found anything.
			valid = true;
		}
	}

	if( !valid /*|| !bEdgeDetected*/ ) {
		// set angle to backwards.
		m_flAutoYaw = Math::AngleNormalize( away.y + 180.f );
		m_flAutoTime = -1.f;
		m_bHasValidAuto = true;
		m_eAutoSide = ESide::SIDE_BACK;
		return;
	}

	// put the most distance at the front of the container.
	std::sort( angles.begin( ), angles.end( ),
			   [ ] ( const AdaptiveAngle &a, const AdaptiveAngle &b ) {
		return a.m_dist > b.m_dist;
	} );

	// the best angle should be at the front now.
	AdaptiveAngle *best = &angles.front( );

	// how long has passed sinec we've updated our angles?
	float last_update_time = g_pGlobalVars->curtime - m_flAutoTime;

	// note; this used to be an exact float inequality against an accumulated
	// distance, which let the chosen side flip every single tick. require the
	// new candidate to beat the stored one by a meaningful margin, and hold the
	// current side for a minimum time before switching away from it.
	constexpr float flSwitchMargin = 16.f;  // 4 ray-march steps worth of cover.
	constexpr float flMinHoldTime = 0.2f;

	// no side picked yet (or we just came back from the no-cover fallback,
	// which parks m_flAutoTime at -1) -> take the best result immediately.
	const bool bNoCurrentSide = !m_bHasValidAuto || m_flAutoTime < 0.f;

	// meaningfully more cover than what we currently sit behind.
	const bool bClearlyBetter = ( best->m_dist - m_flAutoDist ) > flSwitchMargin;

	// our current side lost cover; allow following it down so we don't stay
	// committed to an angle that is no longer covered.
	const bool bCurrentGotWorse = ( m_flAutoDist - best->m_dist ) > flSwitchMargin;

	if( bNoCurrentSide || ( ( bClearlyBetter || bCurrentGotWorse ) && last_update_time >= flMinHoldTime ) ) {
		auto TranslateSide = [&] ( float a ) {
			if( a <= -flXored90 ) {
				return ESide::SIDE_RIGHT;
			}

			if( a >= flXored90 ) {
				return ESide::SIDE_LEFT;
			}

			return ESide::SIDE_BACK;
		};

		// set yaw to the best result.
		m_eAutoSide = TranslateSide( best->m_yaw );
		m_flAutoYaw = Math::AngleNormalize( away.y + best->m_yaw );
		m_flAutoDist = best->m_dist;
		m_flAutoTime = g_pGlobalVars->curtime;
		m_bHasValidAuto = true;
	}

}

bool AntiAim::DoEdgeAntiAim( C_CSPlayer *player, QAngle &out ) {
	// if we use this for resolver ever, we should only prevent
	// this from running if we have manual aa enabled
	if( player && player->EntIndex( ) == g_pEngine->GetLocalPlayer( ) ) {
		if( GetSide( ) != SIDE_MAX )
			return false;
	}

	if( player->m_MoveType( ) == MOVETYPE_LADDER )
		return false;

	Vector vecStart = player->GetEyePosition( );
	// down a bit
	vecStart.z -= 4.f;

	float flClosestDistance = 100.0f;
	const float flRange = 25.f;

	for( float flAngle = 0; flAngle < ( M_PI * 2.f ); flAngle += ( M_PI * 2.f / 8.f ) ) {
		const Vector vecRotatedPosition( flRange * cos( flAngle ) + vecStart.x, flRange * sin( flAngle ) + vecStart.y, vecStart.z );

		CTraceFilterWorldAndPropsOnly filter;
		CGameTrace tr;

		g_pEngineTrace->TraceRay(
			Ray_t( vecStart, vecRotatedPosition ),
			MASK_NPCWORLDSTATIC,
			( ITraceFilter * )&filter,
			&tr
		);

		//g_pDebugOverlay->AddLineOverlay( vecStart, vecRotatedPosition, 255, 255, 255, 0, 0.1f );
		//g_pDebugOverlay->AddTextOverlay( tr.endpos, 0.1f, "%.1f", tr.fraction );

		const float flDistance = vecStart.Distance( tr.endpos );

		if( flDistance < flClosestDistance && tr.fraction < 1.f ) {
			flClosestDistance = flDistance;
			out.y = RAD2DEG( flAngle );
		}
	}

	return flClosestDistance <= flRange;
}

void AntiAim::DoPitch( CUserCmd *pCmd, C_CSPlayer *pLocal ) {
	switch( g_Vars.rage.anti_aim_pitch ) {
		case 1:
			// down.
			pCmd->viewangles.x = 89.f;
			break;
		case 2:
			// up.
			pCmd->viewangles.x = -89.f;
			break;
		case 3:
			// zero.
			pCmd->viewangles.x = 0.f;
			break;
	}
}

void AntiAim::DoRealYaw( CUserCmd *pCmd, C_CSPlayer *pLocal ) {
	if( GetSide( ) != SIDE_MAX )
		return;

	if( g_Vars.rage.anti_aim_at_players ) {
		auto pBestPlayer = GetBestPlayer( );
		if( pBestPlayer ) {
			pCmd->viewangles.y = Math::CalcAngle( pBestPlayer->m_vecOrigin( ), pLocal->m_vecOrigin( ) ).y;
		}
	}

	switch( g_Vars.rage.anti_aim_yaw ) {
		case 0:
			break;
		case 1: // 180
			pCmd->viewangles.y += 179.f;
			break;
		case 2: // 180z
		{
			static bool bShould180z = false;

			// choose your starting angle
			float startPoint = 110.0f;

			// fix it to one float
			static float currentAng = startPoint;

			bool bOnGround = ( pLocal->m_fFlags( ) & FL_ONGROUND ) || ( g_Prediction.get_initial_vars( )->flags & FL_ONGROUND );
			if( !bOnGround ) {
				// increment it if we're in air
				currentAng += 5.0f;

				// clamp it incase it goes out of our maximum spin rage
				if( currentAng >= 250.f )
					currentAng = 250.f;

				// yurr
				bShould180z = true;

				// do 180 z ))
				pCmd->viewangles.y += currentAng;
			}
			else {
				// next tick, start at starting point
				currentAng = startPoint;

				// if we were in air tick before, go back to start point 
				// in order to properly rotate next (so fakelag doesnt fuck it up)
				pCmd->viewangles.y += bShould180z ? startPoint : 179.0f;

				// set for future idkf nigger
				bShould180z = false;
			}
			break;
		}
		case 3: // avoid air
		{
			const float flLowerBodyYawTarget = pLocal->m_flLowerBodyYawTarget( );

			static bool bSwapSide = false;
			static bool bSwappedAngle = false;
			static float flLastGroundBodyYaw = -1337.f;

			float flNewAngle = flLowerBodyYawTarget + ( bSwapSide ? 135.f : -135.f );

			const bool bOnGround = ( pLocal->m_fFlags( ) & FL_ONGROUND ) || ( g_Prediction.get_initial_vars( )->flags & FL_ONGROUND );
			const bool bInAir = ( !( pLocal->m_fFlags( ) & FL_ONGROUND ) || !( g_Prediction.get_initial_vars( )->flags & FL_ONGROUND ) || ( pCmd->buttons & IN_JUMP ) );

			// get our last moving angle (resolvers might compare to this)
			if( !bInAir && pLocal->m_PlayerAnimState( )->m_flVelocityLengthXY > 0.1f )
				flLastGroundBodyYaw = flLowerBodyYawTarget;

			// we are on ground and haven't swapped our angle for when we go in air next
			if( !bInAir ) {
				// haven't swapped yet, swap our angle
				if( !bSwappedAngle ) {
					bSwapSide = !bSwapSide;
					bSwappedAngle = true;
				}
			}

			// not on ground anymore, next time we go on ground we can compare
			else {
				bSwappedAngle = false;
			}

			// get delta between our last move and our new angle
			const float flLastMoveDelta = fabsf( Math::AngleDiff( flNewAngle, flLastGroundBodyYaw ) );
			if( flLastMoveDelta <= 55 ) {
				// delta between new angle and last move angle is too close, offset our new angle more
				flNewAngle += 35;
			}

			// in air, use our new angle
			if( !bOnGround ) {
				pCmd->viewangles.y = flNewAngle;
			}

			// regular angle (back)
			else {
				pCmd->viewangles.y += 179.f;
			}

			break;
		}
	}

	if( g_Vars.rage.anti_aim_yaw_add_jitter ) {
		const float flAdditive = m_bJitterUpdate ? -g_Vars.rage.anti_aim_yaw_jitter : g_Vars.rage.anti_aim_yaw_jitter;

		pCmd->viewangles.y += flAdditive;
	}
}

void AntiAim::DoFakeYaw( CUserCmd *pCmd, C_CSPlayer *pLocal ) {
	m_bJitterUpdate = !m_bJitterUpdate;

	switch( g_Vars.rage.anti_aim_fake_yaw ) {
		case 1:
			// set base to opposite of direction.
			pCmd->viewangles.y = m_flLastRealAngle + 180.f;

			// apply 45 degree jitter.
			pCmd->viewangles.y += RandomFloat( -90.f, 90.f );
			break;
		case 2:
			// set base to opposite of direction.
			pCmd->viewangles.y = m_flLastRealAngle + 180.f;

			// apply offset correction.
			pCmd->viewangles.y += g_Vars.rage.anti_aim_fake_yaw_relative;
			break;
		case 3:
		{
			// get fake jitter range from menu.
			float range = g_Vars.rage.anti_aim_fake_yaw_jitter;

			// set base to opposite of direction.
			pCmd->viewangles.y = m_flLastRealAngle + 180.f;

			// apply jitter.
			pCmd->viewangles.y += RandomFloat( -range, range );
			break;
		}
		case 4:
			pCmd->viewangles.y = m_flLastRealAngle + 90.f + std::fmod( g_pGlobalVars->curtime * 360.f, 180.f );
			break;
		case 5:
			pCmd->viewangles.y = RandomFloat( -180.f, 180.f );
			break;
	}
}

bool AntiAim::IsAbleToFlick( C_CSPlayer *pLocal ) {
	// FUCK IT until we fix it XD
	//if( g_Vars.globals.m_bFakeWalking && !g_AntiAim.m_bAllowFakeWalkFlick )
	//	return false;

	if( g_Vars.globals.m_bRunningExploit )
		return false;

	return !g_pClientState->m_nChokedCommands( )
		&& g_Vars.rage.anti_aim_fake_body
		&& ( pLocal->m_fFlags( ) & FL_ONGROUND )
		&& pLocal->m_PlayerAnimState( )->m_flVelocityLengthXY < 0.1f;
}

void AntiAim::PerformBodyFlick( CUserCmd *pCmd, bool *bSendPacket, float flLastMove ) {
	// (c) andosa
	float flDelta = g_Vars.rage.anti_aim_fake_body_amount;

	// keep lby at last move
	/*if( g_Vars.rage.anti_aim_twist ) {
		pCmd->viewangles.y = flLastMove;
	}
	else*/ {
		// 'freestand' lby flick
		if( g_Vars.rage.anti_aim_fake_body_hide ) {
			g_AntiAim.m_bHidingLBYFlick = m_flEdgeOrAutoYaw != FLT_MAX;

			static int nLastAutoSide = ESide::SIDE_MAX;

			// when using edge priority sometimes u will not freestand, causing
			// ur flick to flick out. we can see if this is the case by checking delta
			// between the current detected freestand angle and our actual angle, and 
			// if we're not at that angle then we should break towards the other side
			const float flFreestandAngleDelta = abs( m_flAutoYaw - m_flLastRealAngle );

			// if our freestand is facing left or right, break backwards
			if( m_eAutoSide == ESide::SIDE_LEFT ) {
				nLastAutoSide = ESide::SIDE_LEFT;

				if( flFreestandAngleDelta > 35.f ) {
					m_eBreakerSide = ESide::SIDE_RIGHT;
					pCmd->viewangles.y = m_flLastRealAngle - flDelta;
				}
				else {
					m_eBreakerSide = ESide::SIDE_LEFT;
					pCmd->viewangles.y = m_flLastRealAngle + flDelta;
				}
			}
			else if( m_eAutoSide == ESide::SIDE_RIGHT ) {
				nLastAutoSide = ESide::SIDE_RIGHT;

				if( flFreestandAngleDelta > 35.f ) {
					m_eBreakerSide = ESide::SIDE_LEFT;
					pCmd->viewangles.y = m_flLastRealAngle + flDelta;
				}
				else {
					m_eBreakerSide = ESide::SIDE_RIGHT;
					pCmd->viewangles.y = m_flLastRealAngle - flDelta;
				}
			}

			// don't break if we're backwards
			if( m_eAutoSide == ESide::SIDE_BACK && m_eAutoSide != nLastAutoSide ) {
				if( nLastAutoSide == ESide::SIDE_LEFT ) {
					m_eBreakerSide = ESide::SIDE_RIGHT;
					pCmd->viewangles.y = m_flLastRealAngle - flDelta;
				}
				else {
					m_eBreakerSide = ESide::SIDE_LEFT;
					pCmd->viewangles.y = m_flLastRealAngle + flDelta;
				}
			}
		}
		else {
			g_AntiAim.m_bHidingLBYFlick = false;

			if( g_Vars.rage.anti_aim_fake_body_side == 0 ) {
				m_eBreakerSide = ESide::SIDE_LEFT;
				pCmd->viewangles.y = m_flLastRealAngle + flDelta;
			}
			else {
				m_eBreakerSide = ESide::SIDE_RIGHT;
				pCmd->viewangles.y = m_flLastRealAngle - flDelta;
			}
		}
	}

	if( g_pClientState->m_nChokedCommands( ) < 1 ) {
		if( !g_Vars.globals.m_bFakeWalking )
			*bSendPacket = false;
	}
}

void AntiAim::HandleManual( CUserCmd *pCmd, C_CSPlayer *pLocal ) {
	if( g_Vars.rage.anti_aim_manual_right_key.enabled ) {
		m_eSide = SIDE_RIGHT;
	}
	else if( g_Vars.rage.anti_aim_manual_left_key.enabled ) {
		m_eSide = SIDE_LEFT;
	}
	else if( g_Vars.rage.anti_aim_manual_back_key.enabled ) {
		m_eSide = SIDE_BACK;
	}
	else if( g_Vars.rage.anti_aim_manual_forward_key.enabled ) {
		m_eSide = SIDE_FWD;
	}
	else {
		m_eSide = SIDE_MAX;
	}

	const bool bInAir = ( !( pLocal->m_fFlags( ) & FL_ONGROUND ) || !( g_Prediction.get_initial_vars( )->flags & FL_ONGROUND ) || ( pCmd->buttons & IN_JUMP ) );
	if( g_Vars.rage.disable_anti_aim_manual_air && bInAir )
		return;

	switch( m_eSide ) {
		case SIDE_LEFT:
			pCmd->viewangles.y += 90.f;
			break;
		case SIDE_RIGHT:
			pCmd->viewangles.y -= 90.f;
			break;
		case SIDE_BACK:
			pCmd->viewangles.y += 180.f;
			break;
		case SIDE_FWD:

			break;
	}
}

void AntiAim::DesyncLastMove( CUserCmd *pCmd, bool *bSendPacket ) {
	if( g_Vars.globals.m_bFakeWalking ) {
		return;
	}

	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal ) {
		return;
	}

	
}

namespace {
	bool EntityIsHostageClass( C_BaseEntity *pEntity, const char *szNetworkName, int iFallbackClassId ) {
		const auto pClass = pEntity->GetClientClass( );
		if( !pClass )
			return false;

		// prefer the network name over the hardcoded class id: ids shift
		// between game builds, the name does not.
		if( pClass->m_pNetworkName )
			return !strcmp( pClass->m_pNetworkName, szNetworkName );

		return pClass->m_ClassID == iFallbackClassId;
	}

	bool IsCarryingHostage( C_CSPlayer *pLocal ) {
		// primary: the netvar handle, resolved through the entity list.
		// note; CBaseHandle::IsValid( ) only compares against the 0xFFFFFFFF
		// sentinel, so it is true for a stale or zero handle too. resolve it
		// through the entity list instead, otherwise "am i carrying" is a
		// coin flip and anti-aim dies for the whole carry.
		if( Engine::Displacement.DT_CSPlayer.m_hCarriedHostage != 0
			&& pLocal->m_hCarriedHostage( ).Get( ) != nullptr )
			return true;

		// fallback: while carried, the hostage becomes a carriable prop owned
		// by the carrier. find it by owner in case the netvar offset failed to
		// resolve (which used to kill the whole feature silently) or the
		// handle is stale.
		for( int i = g_pGlobalVars->maxClients + 1; i < g_pEntityList->GetHighestEntityIndex( ); ++i ) {
			const auto pEntity = reinterpret_cast< C_BaseEntity * >( g_pEntityList->GetClientEntity( i ) );
			if( !pEntity )
				continue;

			if( !EntityIsHostageClass( pEntity, XorStr( "CHostageCarriableProp" ), CHostageCarriableProp ) )
				continue;

			if( pEntity->m_hOwnerEntity( ).Get( ) == pLocal )
				return true;
		}

		return false;
	}
}

bool AntiAim::HandleUseAction( CUserCmd *pCmd, C_CSPlayer *pLocal ) {
	// not holding +use, nothing to do. reset everything.
	if( !( pCmd->buttons & IN_USE ) ) {
		m_eUseAction = USE_NONE;
		m_bUseActionLatched = false;
		m_bRestoringUseAngles = false;
		m_flUseStartTime = 0.f;
		m_bPrevInUse = false;
		return false;
	}

	// rising edge of +use: the server has not seen this press yet, so any
	// latch left over from an older interaction (an earlier defuse, an already
	// active carry) refers to the past. snapshot the carry state and re-open
	// the aim window so the new interaction can start. without this, pressing
	// E while carrying went out with fully desynced angles, the server
	// FindUseEntity trace missed, and nothing happened (the "lock").
	const bool bUsePressed = !m_bPrevInUse;
	m_bPrevInUse = true;

	const bool bCarryingNow = IsCarryingHostage( pLocal );

	if( bUsePressed ) {
		m_bCarryingAtPress = bCarryingNow;
		m_bUseActionLatched = false;
		m_flUseStartTime = g_pGlobalVars->curtime;
	}

	const Vector vecEyePos = pLocal->GetEyePosition( );

	// classify the interaction and find the point we need to be aiming at.
	m_eUseAction = USE_NONE;
	Vector vecTarget{ };

	// defuse; reuse the planted c4 lookup Movement::DefuseTheBomb already did
	// this tick (CreateMove.cpp runs it before AntiAim::Think) instead of
	// scanning the entity list a second time.
	if( g_Movement.m_bNearBomb ) {
		m_eUseAction = USE_DEFUSE;
		vecTarget = g_Movement.m_vecNearBombOrigin;
	}
	else {
		// already carrying one. the server latched the pickup long ago and does
		// not re-trace our aim while we hold it, so full anti-aim is safe.
		// this used to fall through to the USE_NONE bail below, which disabled
		// anti-aim for the entire duration of the carry.
		if( IsCarryingHostage( pLocal ) ) {
			m_eUseAction = USE_HOSTAGE;
		}
		else {
			// hostage pickup; we need real angles on it until it latches.
			// use range is 64 units from the eye position; give a little slack.
			constexpr float flUseRangeSqr = 80.f * 80.f;

			float flBestDistSqr = flUseRangeSqr;
			C_BaseEntity *pBestHostage = nullptr;

			for( int i = 1; i < g_pEntityList->GetHighestEntityIndex( ); ++i ) {
				const auto pEntity = reinterpret_cast< C_BaseEntity * >( g_pEntityList->GetClientEntity( i ) );
				if( !pEntity )
					continue;

				if( !EntityIsHostageClass( pEntity, XorStr( "CHostage" ), CHostage ) )
					continue;

				const float flDistSqr = ( pEntity->m_vecOrigin( ) - vecEyePos ).LengthSquared( );
				if( flDistSqr < flBestDistSqr ) {
					flBestDistSqr = flDistSqr;
					pBestHostage = pEntity;
				}
			}

			if( pBestHostage ) {
				m_eUseAction = USE_HOSTAGE;

				// aim at the hostage's chest rather than its feet, the origin
				// is at ground level and can fall below the use trace.
				vecTarget = pBestHostage->m_vecOrigin( );
				vecTarget.z += 48.f;
			}
		}
	}

	// not something we care about (doors, weapon pickups, ...).
	// keep the original behavior and bail out of anti-aim entirely.
	if( m_eUseAction == USE_NONE ) {
		m_bUseActionLatched = false;
		m_bRestoringUseAngles = false;
		return true;
	}

	// has the server latched the interaction yet?
	if( m_eUseAction == USE_DEFUSE )
		m_bUseActionLatched = pLocal->m_bIsDefusing( );
	else {
		// latched once the carry state differs from the press snapshot:
		// pickup (false -> true) or stop-follow (true -> false). same pattern
		// as the defuse path above: real angles only until the server latches,
		// full anti-aim for the rest. the safety timeout covers a stale carry
		// netvar: a missed latch must not glue real angles (AA off) forever.
		const bool bCarryChanged = ( IsCarryingHostage( pLocal ) != m_bCarryingAtPress );
		const bool bTimedOut = ( g_pGlobalVars->curtime - m_flUseStartTime ) > 0.75f;
		m_bUseActionLatched = bCarryChanged || bTimedOut;
	}

	// latched. csgo only traces the aim to *start* the action; from here it just
	// checks that +use is held and that we stay in range. full anti-aim is safe
	// for the whole remaining (multi-second) duration.
	if( m_bUseActionLatched ) {
		m_bRestoringUseAngles = false;
		return false;
	}

	// not latched yet; hold real angles so the server's
	// FindUseEntity trace can actually find it.
	//
	// no safety timeout that gives up on the *defuse* here on purpose: a failed
	// defuse is worse than a brief angle restore, so we keep aiming for as long
	// as +use is held. (the hostage path has its own timeout above.)
	m_bRestoringUseAngles = true;

	if( m_eUseAction == USE_HOSTAGE && bCarryingNow ) {
		// interact press while already carrying (stop-follow, door, ...): the
		// player is looking at the target themselves, so leave the viewangles
		// untouched. overwriting them with the scanned hostage behind us would
		// snap the view around and the trace would still miss.
		// note; deliberately not feeding m_flLastUseRestore here: merely
		// holding +use must not suppress the standing distortion (see hpp).
	}
	else {
		// note; CalcAngle( src, dst ) builds the angle from dst *towards* src
		// (delta = src - dst), matching the "away" usage in DoRealYaw.
		m_angUseTarget = Math::CalcAngle( vecTarget, vecEyePos );
		m_angUseTarget.Normalize( );

		pCmd->viewangles = m_angUseTarget;

		// we just handed the server our real angles, so LBY is not where the
		// distortion logic thinks it is. suppress it briefly ( gate below ).
		m_flLastUseRestore = g_pGlobalVars->realtime;
	}

	return true;
}

void AntiAim::Think( CUserCmd *pCmd, bool *bSendPacket, bool *bFinalPacket ) {
	bool attack, attack2;

	m_bLbyUpdateThisTick = false;

	if( !g_Vars.rage.anti_aim_active )
		return;

	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	bool bFrozen = pLocal->m_fFlags( ) & 0x40;
	if( g_pGameRules.IsValid( ) ) {
		if( g_pGameRules->m_bFreezePeriod( ) ) {
			bFrozen = true;
		}
	}

	if( bFrozen )
		return;

	auto pWeapon = reinterpret_cast< C_WeaponCSBaseGun * >( pLocal->m_hActiveWeapon( ).Get( ) );
	if( !pWeapon )
		return;

	auto pWeaponData = pWeapon->GetCSWeaponData( );
	if( !pWeaponData.IsValid( ) )
		return;

	attack = pCmd->buttons & IN_ATTACK;
	attack2 = pCmd->buttons & IN_ATTACK2;

	// disable on round freeze period.
	if( g_pGameRules.Xor( ) ) {
		if( g_pGameRules->m_bFreezePeriod( ) )
			return;
	}

	// note; the ladder case used to bail out of anti-aim entirely. instead we
	// flag it and only skip the parts that break climbing (real pitch/yaw),
	// keeping fake yaw and LBY breaking alive. standing still on a ladder is
	// unaffected and keeps full anti-aim as before.
	//
	// known limitation: full *real*-yaw anti-aim while actively climbing is not
	// achievable without pre-compensating forwardmove/sidemove for LadderMove's
	// view-relative projection. that compensation is deliberately out of scope.
	m_bClimbingLadder = ( pCmd->forwardmove || pCmd->sidemove ) && ( pLocal->m_MoveType( ) == MOVETYPE_LADDER );

	// disable on noclip / after the game is over.
	// ( parenthesized explicitly; the original relied on && binding tighter )
	if( g_Vars.globals.m_bGameOver || ( pLocal->m_MoveType( ) == MOVETYPE_NOCLIP ) )
		return;

	// disable if shooting.
	if( attack && pLocal->CanShoot( ) && pWeaponData->m_iWeaponType != WEAPONTYPE_GRENADE )
		return;

	// CBaseCSGrenade::ItemPostFrame()
	// https://github.com/VSES/SourceEngine2007/blob/master/src_main/game/shared/cstrike/weapon_basecsgrenade.cpp#L209
	if( pWeaponData->m_iWeaponType == WEAPONTYPE_GRENADE && pWeapon->m_fThrowTime( ) > 0.f )
		return;

	// disable if knifing.
	if( attack2 && pWeaponData->m_iWeaponType == WEAPONTYPE_KNIFE && pLocal->CanShoot( ) )
		return;

	// +use: aim at the bomb/hostage only until the server latches the
	// interaction, then run full anti-aim for the rest of it.
	// this also feeds m_flLastUseRestore, consumed by the distortion gate below.
	if( HandleUseAction( pCmd, pLocal ) )
		return;

	// while actively climbing, LadderMove derives the climb direction from the
	// view forward vector, so the real pitch and yaw have to stay intact.
	// fake yaw / LBY breaking below are unaffected and keep running.
	if( !m_bClimbingLadder )
		DoPitch( pCmd, pLocal );

	// note - maxwell; don't allow two send ticks in a row....)))
	if( *bSendPacket && m_bLastPacket )
		*bSendPacket = false;

	bool bDoTheFakeFlick = true;
	// ruffian mode
	if( g_Vars.rage.anti_aim_fake_flick == 2 ) {
		if( ( pCmd->tick_count % RandomInt( 50, 100 ) ) > RandomInt( 0, 100 ) )
			bDoTheFakeFlick = false;
	}

	auto bAllowDistortion = [&] ( ) -> bool {
		if( !g_Vars.rage.anti_aim_distortion_key.enabled && g_Vars.rage.anti_aim_distortion_key.key > 0 )
			return false;

		if( g_Vars.rage.anti_aim_distortion_disable_on_manual && g_AntiAim.GetSide( ) != SIDE_MAX )
			return false;

		if( !( pLocal->m_fFlags( ) & FL_ONGROUND ) )
			return g_Vars.rage.anti_aim_distortion_air;

		if( pLocal->m_vecVelocity( ).Length2D( ) > 0.1f || g_Movement.PressingMovementKeys( pCmd ) )
			return g_Vars.rage.anti_aim_distortion_move;

		// when standing, only allow distortion to kick in
		// when we're sure we're successfully breaking lby
		// else if we're failing, our head will start spinning
		// and it will eventually peek out and we'll get tapped

		if( pLocal->m_flDuckAmount( ) > 0.f )
			return g_Vars.rage.anti_aim_distortion_crouch;

		return g_Vars.rage.anti_aim_distortion_stand;
	};

	// if on ground only allow distort when standing
	// if in air we can always allow distortion
	m_bDistorting = bAllowDistortion( );
	bool bWillRetreat = g_Vars.misc.autopeek && g_Vars.misc.autopeek_bind.enabled && ( ( g_Vars.misc.autopeek_retrack == 0 && g_Vars.globals.m_bShotAutopeek ) || ( g_Vars.misc.autopeek_retrack == 1 && !g_Movement.PressingMovementKeys( pCmd ) ) );

	g_Vars.globals.m_bRunningExploit = bDoTheFakeFlick && !bWillRetreat && g_Vars.rage.anti_aim_fake_flick > 0 && g_Vars.rage.anti_aim_fake_flick_key.enabled && !g_Movement.PressingMovementKeys( pCmd ) && !( pCmd->buttons & IN_JUMP );

	m_flEdgeOrAutoYaw = FLT_MAX;

	const auto RoundToMultiple = [&] ( int in, int multiple ) {
		const auto ratio = static_cast< double >( in ) / multiple;
		const auto iratio = std::lround( ratio );
		return static_cast< int >( iratio * multiple );
	};

	auto pBestPlayer = GetBestPlayer( );

	float flBreakAngle = 180.f;
	float flTowards = FLT_MAX;

	if( pBestPlayer ) {
		flTowards = Math::CalcAngle( pBestPlayer->m_vecOrigin( ), pLocal->m_vecOrigin( ) ).y;
		flBreakAngle = flTowards + 180.f;
	}

	float flWantedBody = Math::AngleNormalize( RoundToMultiple( int( flBreakAngle ), 45 ) );

	static float flLastMoveReal = 1337.f;
	bool bMoving = false;

	const bool bFlickThisTick = TICKS_TO_TIME( pLocal->m_nTickBase( ) ) > g_ServerAnimations.m_uServerAnimations.m_flLowerBodyRealignTimer || g_ServerAnimations.m_uServerAnimations.m_bRealignBreaker;
	const bool bTickBeforeFlick = ( TICKS_TO_TIME( pLocal->m_nTickBase( ) ) + g_pGlobalVars->interval_per_tick ) > g_ServerAnimations.m_uServerAnimations.m_flLowerBodyRealignTimer;

	static QAngle angLastAngle = pCmd->viewangles;
	if( !*bSendPacket || !*bFinalPacket || g_AntiAim.m_bHasOverriden ) {
		HandleManual( pCmd, pLocal );

		// note - maxwell; this has to be ran always because huge iq.
		AutoDirection( pLocal );

		if( !m_bClimbingLadder )
			DoRealYaw( pCmd, pLocal );

		// holy.
		bool bCanFreestand = ( pLocal->m_fFlags( ) & FL_ONGROUND ) && g_Vars.rage.anti_aim_freestand && ( g_Vars.rage.anti_aim_freestand_key.enabled || g_Vars.rage.anti_aim_freestand_key.key == 0 );
		bool bCanEdge = ( pLocal->m_fFlags( ) & FL_ONGROUND ) && g_Vars.rage.anti_aim_edge && ( g_Vars.rage.anti_aim_edge_key.enabled || g_Vars.rage.anti_aim_edge_key.key == 0 );

		bool bEdgeDetected = false;
		bool bFreestanding = false;
		QAngle angEdgeAngle{ };

		m_bEdgeYawApplied = m_bAutoYawApplied = false;

		bEdgeDetected = DoEdgeAntiAim( pLocal, angEdgeAngle );

		pCmd->viewangles.y += g_Vars.rage.anti_aim_base_yaw_additive;

		// climbing needs the real view vector intact; edge/auto yaw would
		// rotate it and break LadderMove.
		if( GetSide( ) == SIDE_MAX && !m_bClimbingLadder ) {
			// note; edge yaw only wins when it is actually enabled. when a wall
			// is detected but edge yaw is off we fall through to auto yaw
			// instead of doing nothing, which used to silently kill auto yaw
			// near any wall.
			if( bEdgeDetected && bCanEdge ) {
				pCmd->viewangles.y = angEdgeAngle.y;
				m_flEdgeOrAutoYaw = angEdgeAngle.y;
				m_bEdgeYawApplied = true;
			}
			else {
				bFreestanding = m_bHasValidAuto;

				if( bFreestanding && bCanFreestand ) {
					pCmd->viewangles.y = m_flAutoYaw;
					m_flEdgeOrAutoYaw = m_flAutoYaw;
					m_bAutoYawApplied = true;
				}
			}
		}

		if( bFreestanding && bCanFreestand && g_Vars.rage.anti_aim_distortion_hide_flick )
			flWantedBody = Math::AngleNormalize( RoundToMultiple( int( m_flAutoYaw ), 45 ) );

		// angle towards the player is conveniently
		// also the angle we would be at if our head 
		// was in the wall, so let's stick it in the wall
		if( bEdgeDetected && flTowards != FLT_MAX && fabsf( Math::AngleDiff( flTowards, angEdgeAngle.y ) < 5.f ) )
			flWantedBody = Math::AngleNormalize( RoundToMultiple( int( angEdgeAngle.y ), 45 ) );

		if( !g_Vars.rage.anti_aim_lock_angle_key.enabled )
			angLastAngle = pCmd->viewangles;
		else
			pCmd->viewangles.y = angLastAngle.y;

		m_bEdging = ( bFreestanding && bCanFreestand ) || ( bEdgeDetected && bCanEdge );

		bool bEnsureBreaking = true;
		if( pLocal->m_vecVelocity( ).Length2D( ) <= 0.1f || g_Vars.globals.m_bFakeWalking ) {
			// make sure we aren't fail breaking, and our lby is where we want it to be
			bEnsureBreaking = pLocal->m_flLowerBodyYawTarget( ) == flWantedBody;
		}

		if( bEnsureBreaking ) {
			// suppress distortion for one LBY cycle after a +use angle restore.
			// keyed on the restore, not on IN_USE being held: a hostage carry
			// holds +use for many seconds and must not suppress anything.
			if( m_flLastUseRestore > 0.f && fabsf( m_flLastUseRestore - g_pGlobalVars->realtime ) < 1.1f + 0.22f ) {
				bEnsureBreaking = false;
			}
		}

		static float flLastMoveBody = 1337.f;
		static float flLastMoveTime = 1337.f;

		if( pLocal->m_vecVelocity( ).Length2D( ) > 0.1f && !g_Vars.globals.m_bFakeWalking ) {
			flLastMoveBody = m_flLastRealAngle;
			flLastMoveTime = g_pGlobalVars->realtime;
		}

		bool bBreakMove = flLastMoveBody != 1337.f && g_Vars.rage.anti_aim_distortion_hide_moving;
		float flMoveDelta = std::fabsf( flLastMoveTime - g_pGlobalVars->realtime );

		// we can safely do this here, the enemies don't get info about
		// where our real angle is (of course, else resolvers wouldnt be a thing)
		// so we can put our head anywhere (ideally non stop moving at a random pace)
		// and since we keep our lby static in one place they have no clue where
		// our head is so they can bruteforce all they want but they will miss (unless
		// they headshout us due to spread or just luck lawl)
		if( m_bDistorting ) {
			if( bBreakMove && ( flMoveDelta < ( 1.1f * 2.f ) || g_ServerAnimations.m_uServerAnimations.m_bFirstFlick ) ) {
				pCmd->viewangles.y = flLastMoveBody + 180.f;

				float flAdditive = std::fmod( g_pGlobalVars->curtime * ( m_flRandDistortFactor * m_flRandDistortSpeed ), 120.f );
				if( fabsf( Math::AngleDiff( flAdditive, 120.f ) ) < 5.f ) {
					static bool bSwitchSide = false;
					bSwitchSide = !bSwitchSide;

					RandomSeed( g_pGlobalVars->tickcount );
					m_flRandDistortFactor = RandomFloat( 10.f, 80.f );
					RandomSeed( g_pGlobalVars->tickcount + 1 );
					m_flRandDistortSpeed = RandomFloat( 2.5f, 12.5f );

					if( bSwitchSide ) {
						m_flRandDistortFactor = fabsf( m_flRandDistortFactor );
					}
					else {
						m_flRandDistortFactor = -fabsf( m_flRandDistortFactor );
					}

					flAdditive = std::fmod( g_pGlobalVars->curtime * ( m_flRandDistortFactor * m_flRandDistortSpeed ), 120.f );
				}

				pCmd->viewangles.y += flAdditive;

			}
			else if( !g_ServerAnimations.m_uServerAnimations.m_bFirstFlick && bEnsureBreaking ) {
				// add random shit to aa lawl
				pCmd->viewangles.y += std::fmodf( g_pGlobalVars->curtime * ( m_flRandDistortFactor * m_flRandDistortSpeed ), 360.f );
			}
		}

		DesyncLastMove( pCmd, nullptr );

		auto pUnpredictedData = g_Prediction.get_initial_vars( );
		if( pUnpredictedData ) {
			const bool bOnGround = pUnpredictedData->flags & FL_ONGROUND && pLocal->m_fFlags( ) & FL_ONGROUND;

			static float flTargetLBY = 0.f;
			static bool bPerformDesync = false;

			const bool bForceDesync = g_Vars.rage.anti_aim_desync_land_force && g_Vars.rage.anti_aim_desync_land_key.enabled;

			// initial mode
			if( g_Vars.rage.anti_aim_desync_land_first || bForceDesync ) {
				// 2 consecutive ticks on ground, allow initial desync
				if( ( pLocal->m_fFlags( ) & FL_ONGROUND ) && ( pUnpredictedData->flags & FL_ONGROUND ) || bForceDesync ) {
					bPerformDesync = true;

					// place lby 135 deg away from last  
					// real angle from when we were on ground
					flTargetLBY = Math::AngleNormalize( m_flLastRealAngle + 135.f );
				}
			}

			static float flAttemptedLBY = 0.f;
			if( bPerformDesync || bForceDesync ) {
				// we've successfully desynced our lby, no need to continue
				if( pLocal->m_flLowerBodyYawTarget( ) == Math::AngleNormalize( flTargetLBY ) ) {
					bPerformDesync = false;
				}

				// we don't want to actually modify our real netvars
				Vector vecOrigin = pLocal->m_vecOrigin( ), vecVelocity = pLocal->m_vecVelocity( );
				int fPredictedFlags = pLocal->m_fFlags( );

				// simulate two movement ticks
				for( int i = 0; i < 2; ++i ) {
					g_Movement.PlayerMove( pLocal, vecOrigin, vecVelocity, fPredictedFlags, pUnpredictedData->flags & FL_ONGROUND );
				}

				if( !bOnGround ) {
					static bool bDesyncLand = false;

					if( bDesyncLand ) {
						pCmd->viewangles.y = flAttemptedLBY = flTargetLBY;
						*bSendPacket = true;

						bDesyncLand = false;
					}

					if( pLocal->m_fFlags( ) != fPredictedFlags ) {
						pCmd->viewangles.y = flAttemptedLBY = flTargetLBY;
						*bSendPacket = true;

						bDesyncLand = true;
					}
				}
			}

			g_AntiAim.m_bLandDesynced =
				( !bOnGround || g_InputSystem.IsKeyDown( VK_SPACE ) ) && fabsf( Math::AngleDiff( pLocal->m_flLowerBodyYawTarget( ), Math::AngleNormalize( flAttemptedLBY ) ) ) < 1.5f;

		}

		m_flLastRealAngle = pCmd->viewangles.y;

		if( pLocal->m_vecVelocity( ).Length2D( ) >= 0.1f ) {
			flLastMoveReal = m_flLastRealAngle;
			bMoving = true;
		}

		angViewangle = pCmd->viewangles;
	}
	else {
		if( !g_Vars.globals.m_bRunningExploit )
			DoFakeYaw( pCmd, pLocal );
	}

	float flYawToAddTo = g_Vars.rage.anti_aim_lock_angle_key.enabled ? angLastAngle.y : pCmd->viewangles.y;

	if( g_Vars.globals.m_bRunningExploit ) {
		if( !g_pClientState->m_nChokedCommands( ) ) {
			static int stage = 0;

			switch( stage ) {
				case 0:
					pCmd->viewangles.y = flYawToAddTo + 110.f;
					pCmd->forwardmove = 21.f;
					break;
				case 1:
					pCmd->viewangles.y = flYawToAddTo - 30.f;
					break;
				case 2:
					pCmd->forwardmove = -21.f;
					pCmd->viewangles.y = flYawToAddTo - 125.f;
					break;
				case 4:
					stage = -2;
					break;
				default:
					break;
			}

			stage++;
		}
	}

	bool bIIsAbleToFlick = IsAbleToFlick( pLocal );

	// fake body.
	if( bIIsAbleToFlick ) {
		if( bFlickThisTick ) {
			m_bLbyUpdateThisTick = true;
		}

		static bool bFlickNextTick = false;
		static bool bWasHidingFlick = false;

		if( bFlickThisTick ) {
			g_AntiAim.m_bHidingLBYFlick = false;

			if( m_bDistorting ) {
				// break lby "away" from the target/threat player,
				// except we only allow significant (>45deg) changes, to prevent
				// getting tapped with a slight mouse movement
				pCmd->viewangles.y = flWantedBody;

				static bool bSwitchSide = false;
				bSwitchSide = !bSwitchSide;

				RandomSeed( g_pGlobalVars->tickcount );
				m_flRandDistortFactor = RandomFloat( 10.f, 80.f );
				RandomSeed( g_pGlobalVars->tickcount + 1 );
				m_flRandDistortSpeed = RandomFloat( 2.5f, 12.5f );

				if( g_Vars.rage.anti_aim_distortion_side == 0 ) {
					if( bSwitchSide ) {
						m_flRandDistortFactor = fabsf( m_flRandDistortFactor );
					}
					else {
						m_flRandDistortFactor = -fabsf( m_flRandDistortFactor );
					}
				}
				else {
					m_flRandDistortFactor = fabsf( m_flRandDistortFactor );
					if( g_Vars.rage.anti_aim_distortion_side == 1 )
						m_flRandDistortFactor = -fabsf( m_flRandDistortFactor );
				}
			}
			else {
				float flRoundedLastMove = Math::AngleNormalize( RoundToMultiple( int( flLastMoveReal ), 45 ) );


				PerformBodyFlick( pCmd, bSendPacket, flRoundedLastMove );
			}

			g_ServerAnimations.m_uRenderAnimations.m_bDoingRealFlick = true;
			g_ServerAnimations.m_uServerAnimations.m_bRealignBreaker = false;

			g_ServerAnimations.m_uServerAnimations.m_bFirstFlick = false;

			g_ServerAnimations.m_uServerAnimations.m_flLowerBodyRealignTimer = TICKS_TO_TIME( pLocal->m_nTickBase( ) ) + 1.1f;
			g_ServerAnimations.m_uRenderAnimations.m_flLowerBodyRealignTimer = g_pGlobalVars->realtime + 1.1f;

			bWasHidingFlick = g_AntiAim.m_bHidingLBYFlick;
		}
		else {
			g_AntiAim.m_bHidingLBYFlick = bWasHidingFlick;

			if( bTickBeforeFlick ) {
				// when breaking under 100 delta, we need the delta between the preflick and the actual flick
				// to be equal to 100 (as low as possibly) so that we can actually break to the angle we want to
				if( m_bDistorting ) {
					const float flTargetAngleToBreakTo = flWantedBody;
					const float flWhereToPreBreakTo = flTargetAngleToBreakTo + 100.f;

					pCmd->viewangles.y = flWhereToPreBreakTo;

					g_AntiAim.m_flLastPrebrakeAngle = pCmd->viewangles.y;
				}
				else {
					/*if( g_Vars.rage.anti_aim_twist ) {
						pCmd->viewangles.y = flLastMoveReal + 101.f;
					}
					else*/ {
						float flDeltaFrom100 = fabs( g_Vars.rage.anti_aim_fake_body_amount - 100 );

						float flAngleToBreakTo = m_flLastRealAngle - flDeltaFrom100;

						if( m_eBreakerSide == ESide::SIDE_RIGHT ) {
							flDeltaFrom100 = fabs( g_Vars.rage.anti_aim_fake_body_amount + 235.f );
							flAngleToBreakTo = m_flLastRealAngle - flDeltaFrom100;;
						}

						if( g_Vars.rage.anti_aim_fake_body_amount <= 100.f )
							pCmd->viewangles.y = flAngleToBreakTo;
					}
				}

				g_ServerAnimations.m_uRenderAnimations.m_bDoingPreFlick = true;
				m_bLbyUpdateThisTick = true;
			}
		}
	}

	m_bLastPacket = *bSendPacket;
	pCmd->viewangles.y = Math::AngleNormalize( pCmd->viewangles.y );
}