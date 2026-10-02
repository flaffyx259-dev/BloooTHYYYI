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

namespace {
	bool InitialOnGround( C_CSPlayer *pLocal ) {
		const auto pData = g_Prediction.get_initial_vars( );
		return pData ? ( pData->flags & FL_ONGROUND ) != 0
		             : ( pLocal->m_fFlags( ) & FL_ONGROUND ) != 0;
	}

	float GetBodyYaw( C_CSPlayer *pLocal ) {
		return g_ServerAnimations.m_bPredictedLBYValid ? g_ServerAnimations.m_flPredictedLBY : pLocal->m_flLowerBodyYawTarget( );
	}
}

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

namespace {
	struct FreestandThreat_t {
		Vector m_vecEye;
		float  m_flWeight;
	};

	struct FreestandCandidate_t {
		float m_flRel;      // смещение от направления на главного врага
		float m_flScore;
		bool  m_bExposed;   // голову в этой точке видит хотя бы один враг
	};

	// то же "толщина укрытия", но сначала один TraceRay: чистый луч -> голова открыта,
	// без сотен GetPointContents. шагаем только от первого попадания до головы.
	// само попадание считаем всегда - проп ( ящик, дверь ) не виден WorldOnly-contents.
	float MeasureCover( const Vector &vecStart, const Vector &vecEnd ) {
		constexpr float flStep = 4.f;

		Vector vecDir = vecEnd - vecStart;
		const float flLen = vecDir.Normalize( );
		if( flLen <= 0.f )
			return 0.f;

		CTraceFilterWorldAndPropsOnly filter;
		CGameTrace tr;
		g_pEngineTrace->TraceRay( Ray_t( vecStart, vecEnd ), MASK_SHOT_HULL, ( ITraceFilter * )&filter, &tr );

		if( tr.fraction >= 1.f && !tr.startsolid )
			return 0.f;

		const float flHit = tr.startsolid ? 0.f : flLen * tr.fraction;

		float flCover = flStep * g_AntiAim.UpdateFreestandPriority( flLen, static_cast< int >( flHit ) );

		for( float i = flHit + flStep; i < flLen; i += flStep ) {
			const Vector vecPoint = vecStart + vecDir * i;
			if( !( g_pEngineTrace->GetPointContents_WorldOnly( vecPoint, MASK_SHOT_HULL ) & MASK_SHOT_HULL ) )
				continue;

			flCover += flStep * g_AntiAim.UpdateFreestandPriority( flLen, static_cast< int >( i ) );
		}

		return flCover;
	}

	ESide SideFromRel( float flRel ) {
		if( flRel > 45.f && flRel < 135.f )
			return SIDE_LEFT;
		if( flRel < -45.f && flRel > -135.f )
			return SIDE_RIGHT;
		return SIDE_BACK;
	}
}

void AntiAim::AutoDirection( C_CSPlayer *pLocal ) {
	if( GetSide( ) != SIDE_MAX )
		return;

	constexpr float flHeadRadius   = 32.f;  // грубая оценка положения головы, как раньше
	constexpr float flLead         = 0.2f;  // упреждение позиции врага ( сек ), подстраивается
	constexpr float flExposedCost  = 64.f;  // штраф за открытость одному врагу ( = 16 шагов укрытия )
	constexpr float flSwitchMargin = 16.f;
	constexpr float flMinHoldTime  = 0.2f;

	const Vector vecLocalEye = pLocal->GetEyePosition( );

	C_CSPlayer *pMain = GetBestPlayer( true );
	if( !pMain ) {
		m_flAutoYaw = -1.f;
		m_flAutoDist = -1.f;
		m_flAutoRelYaw = 180.f;
		m_bHasValidAuto = false;
		m_eAutoSide = SIDE_BACK;
		return;
	}

	const float flAway = Math::CalcAngle( pMain->m_vecOrigin( ), pLocal->m_vecOrigin( ) ).y;

	// все враги, с упреждением: сторона выбирается под будущий пик,
	// а не под точку, с которой он уже ушёл.
	std::vector< FreestandThreat_t > vecThreats;
	for( int i = 1; i <= g_pGlobalVars->maxClients; i++ ) {
		auto player = C_CSPlayer::GetPlayerByIndex( i );
		if( !player || player->IsDead( ) || player->IsDormant( ) || player->IsTeammate( pLocal ) )
			continue;

		if( g_PlayerList.GetSettings( player->GetSteamID( ) ).m_bAddToWhitelist && !g_Vars.misc.force_ignore_whitelist.enabled )
			continue;

		Vector vecEye = player->GetEyePosition( );

		Vector vecVel = player->m_vecVelocity( );
		vecVel.z = 0.f;
		if( vecVel.Length2D( ) > 1.f ) {
			CTraceFilterWorldAndPropsOnly filter;
			CGameTrace tr;
			g_pEngineTrace->TraceRay( Ray_t( vecEye, vecEye + vecVel * flLead ), MASK_SHOT_HULL, ( ITraceFilter * )&filter, &tr );

			// не упираемся вплотную в стену, иначе следующий трейс начнётся в solid.
			vecEye += ( tr.endpos - vecEye ) * 0.9f;
		}

		const float flDist = ( player->m_vecOrigin( ) - pLocal->m_vecOrigin( ) ).Length( );

		// ближние опаснее: 1.0 до 512 юнитов, дальше спадает, но не ниже 0.25.
		float flWeight = 512.f / ( flDist > 1.f ? flDist : 1.f );
		if( flWeight > 1.f )  flWeight = 1.f;
		if( flWeight < 0.25f ) flWeight = 0.25f;

		vecThreats.push_back( { vecEye, flWeight } );
	}

	std::vector< FreestandCandidate_t > vecCandidates;
	if( !g_Vars.globals.m_bRunningExploit )
		vecCandidates.push_back( { 180.f, 0.f, false } );
	vecCandidates.push_back( { 90.f, 0.f, false } );
	vecCandidates.push_back( { -90.f, 0.f, false } );

	bool bAnyCover = false;
	for( auto &cand : vecCandidates ) {
		const float flRad = DEG2RAD( flAway + cand.m_flRel );
		const Vector vecHead{ vecLocalEye.x + std::cos( flRad ) * flHeadRadius,
			vecLocalEye.y + std::sin( flRad ) * flHeadRadius,
			vecLocalEye.z };

		for( const auto &threat : vecThreats ) {
			const float flCover = MeasureCover( threat.m_vecEye, vecHead );
			if( flCover > 0.f ) {
				cand.m_flScore += flCover * threat.m_flWeight;
				bAnyCover = true;
			}
			else {
				// открыт хотя бы одному - сильный штраф, укрытие от другого это не перекроет.
				cand.m_flScore -= flExposedCost * threat.m_flWeight;
				cand.m_bExposed = true;
			}
		}
	}

	// укрытия нет нигде -> назад.
	if( !bAnyCover ) {
		m_flAutoRelYaw = 180.f;
		m_flAutoYaw = Math::AngleNormalize( flAway + 180.f );
		m_flAutoDist = 0.f;
		m_flAutoTime = -1.f;
		m_bHasValidAuto = true;
		m_eAutoSide = SIDE_BACK;
		return;
	}

	// max_element возвращает первый максимум -> при равенстве побеждает back ( он первый ).
	const auto itBest = std::max_element( vecCandidates.begin( ), vecCandidates.end( ),
		[ ] ( const FreestandCandidate_t &a, const FreestandCandidate_t &b ) {
			return a.m_flScore < b.m_flScore;
		} );

	// СВЕЖАЯ оценка текущей стороны в этом тике, а не сохранённая при выборе.
	const FreestandCandidate_t *pCurrent = nullptr;
	if( m_bHasValidAuto && m_flAutoTime >= 0.f ) {
		for( const auto &cand : vecCandidates ) {
			if( fabsf( Math::AngleDiff( cand.m_flRel, m_flAutoRelYaw ) ) < 1.f ) {
				pCurrent = &cand;
				break;
			}
		}
	}

	bool bSwitch = false;
	if( !pCurrent ) {
		bSwitch = true;
	}
	else if( &*itBest != pCurrent ) {
		// текущая сторона открылась, а лучшая закрыта -> уходим сразу, без hold time.
		const bool bEscape = pCurrent->m_bExposed && !itBest->m_bExposed;
		const bool bClearlyBetter = ( itBest->m_flScore - pCurrent->m_flScore ) > flSwitchMargin;
		const bool bHeldEnough = ( g_pGlobalVars->curtime - m_flAutoTime ) >= flMinHoldTime;

		bSwitch = bEscape || ( bClearlyBetter && bHeldEnough );
	}

	if( bSwitch ) {
		m_flAutoRelYaw = itBest->m_flRel;
		m_flAutoTime = g_pGlobalVars->curtime;
	}

	const FreestandCandidate_t &chosen = bSwitch ? *itBest : *pCurrent;
	m_flAutoDist = chosen.m_flScore;

	// абсолютный угол пересчитывается КАЖДЫЙ тик от текущей позиции врага.
	m_flAutoYaw = Math::AngleNormalize( flAway + m_flAutoRelYaw );
	m_eAutoSide = SideFromRel( m_flAutoRelYaw );
	m_bHasValidAuto = true;
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

			bool bOnGround = ( pLocal->m_fFlags( ) & FL_ONGROUND ) || InitialOnGround( pLocal );
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
			const float flLowerBodyYawTarget = GetBodyYaw( pLocal );

			static bool bSwapSide = false;
			static bool bSwappedAngle = false;
			static float flLastGroundBodyYaw = -1337.f;

			float flNewAngle = flLowerBodyYawTarget + ( bSwapSide ? 135.f : -135.f );

			const bool bOnGround = ( pLocal->m_fFlags( ) & FL_ONGROUND ) || InitialOnGround( pLocal );
			const bool bInAir = ( !( pLocal->m_fFlags( ) & FL_ONGROUND ) || !InitialOnGround( pLocal ) || ( pCmd->buttons & IN_JUMP ) );

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

			// при range > 90 фейк мог встать прямо на реал.
			pCmd->viewangles.y = KeepAwayFromBody( pCmd->viewangles.y, m_flLastRealAngle, 90.f );
			break;
		}
		case 4:
			pCmd->viewangles.y = m_flLastRealAngle + 90.f + std::fmod( g_pGlobalVars->curtime * 360.f, 180.f );
			break;
		case 5:
			pCmd->viewangles.y = KeepAwayFromBody( RandomFloat( -180.f, 180.f ), m_flLastRealAngle, 90.f );
			break;
		case 6: {
			// RAX: в сетевые eye angles уходит LBY. противник его и так видит -> ноль новой
			// информации. в движении LBY = eye первой команды пачки ( сервер ставит его сам ),
			// так что и там мы не выдаём ничего сверх того, что уже выдал сервер.
			pCmd->viewangles.y = GetBodyYaw( pLocal );
			break;
		}
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

	const bool bInAir = ( !( pLocal->m_fFlags( ) & FL_ONGROUND ) || !InitialOnGround( pLocal ) || ( pCmd->buttons & IN_JUMP ) );
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

// реал не подпускаем к LBY ближе flMinDelta. 100 = 58 ( ширина aim matrix ) + 35 ( порог флика )
// + запас: ноги ( они в пределах eye ± 58 ) всегда остаются > 35 от LBY, поэтому флик в тот же
// LBY проходит без префлика, а голова никогда не стоит там, куда стреляет LBY-резолвер.
float AntiAim::KeepAwayFromBody( float flYaw, float flBody, float flMinDelta ) {
	const float flDelta = Math::AngleDiff( flYaw, flBody );
	if( fabsf( flDelta ) >= flMinDelta )
		return flYaw;

	return Math::AngleNormalize( flBody + ( flDelta >= 0.f ? flMinDelta : -flMinDelta ) );
}

// куда флинкать. первый флик после остановки ( RAX hide_yaw ): уводим LBY на 120 в сторону
// реала, чтобы он ушёл от последнего угла движения. дальше - флик в ТОТ ЖЕ LBY: значение
// не меняется, у резолверов нет события "LBY update" и нет body delta для обучения.
float AntiAim::GetFlickYaw( C_CSPlayer *pLocal ) {
	const float flBody = GetBodyYaw( pLocal );

	if( g_ServerAnimations.m_uServerAnimations.m_bFirstFlick ) {
		const float flToReal = Math::AngleDiff( m_flLastRealAngle, flBody );
		return Math::AngleNormalize( flBody + ( flToReal >= 0.f ? 120.f : -120.f ) );
	}

	return flBody;
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
	// стоя пачки по 2 команды ( см. ниже ) -> анимация каждые 2 тика, префлик = прошлая анимация
	const bool bTickBeforeFlick = !bFlickThisTick &&
		( TICKS_TO_TIME( pLocal->m_nTickBase( ) ) + TICKS_TO_TIME( 2 ) ) > g_ServerAnimations.m_uServerAnimations.m_flLowerBodyRealignTimer;

	// стоя: пачки по 2. сервер анимирует только ПЕРВУЮ команду пакета ( Update выходит
	// при том же framecount ), сетевые eye angles - из последней.
	const bool bStandingPacks = ( pLocal->m_fFlags( ) & FL_ONGROUND ) && pLocal->m_PlayerAnimState( )->m_flVelocityLengthXY <= 0.1f &&
		!g_Vars.globals.m_bFakeWalking && !g_Vars.globals.m_bRunningExploit;

	if( bStandingPacks )
		*bSendPacket = g_pClientState->m_nChokedCommands( ) >= 1;

	static QAngle angLastAngle = pCmd->viewangles;

	// стоя ветку решает только итоговый send. m_bHasOverriden ( FakeLag перед фликом )
	// раньше отправлял реал во второй команде пачки.
	if( !*bSendPacket || !*bFinalPacket || ( g_AntiAim.m_bHasOverriden && !bStandingPacks ) ) {
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
		if( bEdgeDetected && flTowards != FLT_MAX && fabsf( Math::AngleDiff( flTowards, angEdgeAngle.y ) ) < 5.f )
			flWantedBody = Math::AngleNormalize( RoundToMultiple( int( angEdgeAngle.y ), 45 ) );

		if( !g_Vars.rage.anti_aim_lock_angle_key.enabled )
			angLastAngle = pCmd->viewangles;
		else
			pCmd->viewangles.y = angLastAngle.y;

		m_bEdging = ( bFreestanding && bCanFreestand ) || ( bEdgeDetected && bCanEdge );

		bool bEnsureBreaking = true;
		if( pLocal->m_vecVelocity( ).Length2D( ) <= 0.1f || g_Vars.globals.m_bFakeWalking ) {
			// сетевой LBY квантуется, точное == ненадёжно. смысл теперь другой —
			// «ломаем ли мы LBY вообще»: флик идёт в тот же LBY, так что проверяем
			// дельту реала от LBY, а не совпадение LBY с желаемым углом.
			bEnsureBreaking = fabsf( Math::AngleDiff( GetBodyYaw( pLocal ), m_flLastRealAngle ) ) >= 35.f;
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
				if( m_bEdgeYawApplied || m_bAutoYawApplied ) {
					// голова за стеной: не выносим её на +180 от движения ( это может быть
					// открытая сторона ), держим угол укрытия, но не ближе 100 к LBY движения.
					pCmd->viewangles.y = KeepAwayFromBody( pCmd->viewangles.y, flLastMoveBody, 100.f );
				}
				else {
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
			}
			else if( !g_ServerAnimations.m_uServerAnimations.m_bFirstFlick && bEnsureBreaking ) {
				const float flPhase = std::fmodf( g_pGlobalVars->curtime * ( m_flRandDistortFactor * m_flRandDistortSpeed ), 360.f );

				if( m_bEdgeYawApplied || m_bAutoYawApplied ) {
					// голова за стеной: качаем ±30 вокруг угла freestand/edge, а не крутим 360.
					pCmd->viewangles.y += 30.f * std::sin( DEG2RAD( flPhase ) );
				}
				else {
					pCmd->viewangles.y += flPhase;
				}
			}
		}

		// работает и поверх distortion: голова крутится, но вне сектора LBY ± 100.
		// до первого флика после остановки не трогаем: LBY там ещё = последний угол движения.
		if( ( pLocal->m_fFlags( ) & FL_ONGROUND ) && pLocal->m_PlayerAnimState( )->m_flVelocityLengthXY <= 0.1f &&
			!g_ServerAnimations.m_uServerAnimations.m_bFirstFlick && !m_bClimbingLadder && !g_Vars.globals.m_bFakeWalking )
			pCmd->viewangles.y = KeepAwayFromBody( pCmd->viewangles.y, GetBodyYaw( pLocal ), 100.f );

		DesyncLastMove( pCmd, bSendPacket );

		auto pUnpredictedData = g_Prediction.get_initial_vars( );
		if( pUnpredictedData ) {
			const bool bOnGround = pUnpredictedData->flags & FL_ONGROUND && pLocal->m_fFlags( ) & FL_ONGROUND;

			static float flTargetLBY = 0.f;
			static bool bPerformDesync = false;

			const bool bForceDesync = g_Vars.rage.anti_aim_desync_land_force && g_Vars.rage.anti_aim_desync_land_key.enabled;

			// initial mode
			if( g_Vars.rage.anti_aim_desync_land_first || bForceDesync ) {
				// 2 consecutive ticks on ground, allow initial desync
				if( ( ( pLocal->m_fFlags( ) & FL_ONGROUND ) && ( pUnpredictedData->flags & FL_ONGROUND ) ) || bForceDesync ) {
					bPerformDesync = true;

					// place lby 135 deg away from last  
					// real angle from when we were on ground
					flTargetLBY = Math::AngleNormalize( m_flLastRealAngle + 135.f );
				}
			}

			static float flAttemptedLBY = 0.f;
			if( bPerformDesync || bForceDesync ) {
				// we've successfully desynced our lby, no need to continue
				if( fabsf( Math::AngleDiff( pLocal->m_flLowerBodyYawTarget( ), flTargetLBY ) ) < 1.5f ) {
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

					// приземление = в предсказании появился FL_ONGROUND.
					if( !( pLocal->m_fFlags( ) & FL_ONGROUND ) && ( fPredictedFlags & FL_ONGROUND ) ) {
						pCmd->viewangles.y = flAttemptedLBY = flTargetLBY;
						*bSendPacket = true;

						bDesyncLand = true;
					}
				}
			}

			g_AntiAim.m_bLandDesynced =
				( !bOnGround || ( pCmd->buttons & IN_JUMP ) ) && fabsf( Math::AngleDiff( pLocal->m_flLowerBodyYawTarget( ), Math::AngleNormalize( flAttemptedLBY ) ) ) < 1.5f;

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
		static bool bWasHidingFlick = false;
		// случайная задержка обновления LBY ( исправление 5 ). переживает тики,
		// поэтому static. шаг 2: анимация стоя идёт каждые 2 тика ( пачки по 2 ).
		static int nDelayTicks = 0;

		bool bDelayedFlick = false;
		if( bFlickThisTick ) {
			// сервер обновит LBY только при |foot - eye| > 35 ( SetUpVelocity ). держим eye в 25° от ног
			// случайные 0..6 тиков после истечения таймера: прогноз флика у резолверов промахивается,
			// а наш таймер остаётся истёкшим, как и у сервера.
			const int nSinceExpire = TIME_TO_TICKS( TICKS_TO_TIME( pLocal->m_nTickBase( ) ) - g_ServerAnimations.m_uServerAnimations.m_flLowerBodyRealignTimer );
			const float flFoot = g_ServerAnimations.m_uServerAnimations.m_flFootYaw;
			const float flFlickTarget = GetFlickYaw( pLocal );

			if( !g_ServerAnimations.m_uServerAnimations.m_bFirstFlick && nSinceExpire < nDelayTicks &&
				fabsf( Math::AngleDiff( flFlickTarget, flFoot ) ) > 35.f + 100.f * TICKS_TO_TIME( 2 ) ) {
				// ApproachAngle( target, value, speed ): от ног на 25° в сторону от угла флика
				pCmd->viewangles.y = Math::ApproachAngle( flFlickTarget + 180.f, flFoot, 25.f );
				bDelayedFlick = true;
			}
			else {
				nDelayTicks = RandomInt( 0, 3 ) * 2; // анимация стоя каждые 2 тика
			}
		}

		if( bDelayedFlick ) {
			// флик не делаем, таймер и флаги не трогаем.
			// команда останется зачоканной правилом чока 1 ниже, в сеть ничего не уйдёт.
		}
		else if( bFlickThisTick ) {
			m_bLbyUpdateThisTick = true;
			g_AntiAim.m_bHidingLBYFlick = false;

			float flFlickYaw = GetFlickYaw( pLocal );

			// флик сработает только при |foot - eye| > 35. если ноги слишком близко к цели,
			// таймер ниже всё равно уедет на +1.1, а у сервера он останется истёкшим ->
			// LBY уйдёт в первую подходящую команду, возможно в реал. поэтому флик в запасной
			// угол: 60 от ног ( ноги ужмутся до eye ± 58, это > 35 ), на стороне от реала.
			const float flFootNow = g_ServerAnimations.m_uServerAnimations.m_flFootYaw;
			constexpr float flFlickMargin = 35.f + 10.f;
			if( fabsf( Math::AngleDiff( flFlickYaw, flFootNow ) ) <= flFlickMargin ) {
				const float flRealSide = Math::AngleDiff( m_flLastRealAngle, flFootNow );
				flFlickYaw = Math::AngleNormalize( flFootNow + ( flRealSide >= 0.f ? -60.f : 60.f ) );
			}

			pCmd->viewangles.y = flFlickYaw;
			if( g_pClientState->m_nChokedCommands( ) < 1 && !g_Vars.globals.m_bFakeWalking )
				*bSendPacket = false; // флик - первая команда пачки, её и анимирует сервер

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

		g_ServerAnimations.m_uRenderAnimations.m_bDoingRealFlick = true;
		g_ServerAnimations.m_uServerAnimations.m_bRealignBreaker = false;

		// таймер LBY больше не пишем здесь: Think идёт до HandleAnimations, поэтому
		// запись "now + 1.1" убивала проверку flServerTime > timer в модели SetUpVelocity.
		// флик подтверждает модель ( блок D в ServerAnimations ).

		bWasHidingFlick = g_AntiAim.m_bHidingLBYFlick;
		}
		else {
			g_AntiAim.m_bHidingLBYFlick = bWasHidingFlick;

			if( bTickBeforeFlick ) {
				// RAX: ноги держатся в пределах eye ± 58. префлик на flick ± ( 116 + approach ) ставит
				// их минимум в 58 от угла флика -> на флике |foot - eye| > 35 гарантированно.
				const float flFlickYaw = GetFlickYaw( pLocal );
				const float flApproach = 100.f * TICKS_TO_TIME( 2 ); // ноги тянутся к LBY 100°/с
				const float flAdd = 116.f + flApproach;

				const float flA = Math::AngleNormalize( flFlickYaw - flAdd );
				const float flB = Math::AngleNormalize( flFlickYaw + flAdd );

				// обе стороны дают ноги >= 58 от угла флика, выбор свободный.
				// голова за стеной -> сторона ближе к укрытию, иначе как раньше.
				if( m_flEdgeOrAutoYaw != FLT_MAX ) {
					pCmd->viewangles.y = fabsf( Math::AngleDiff( flA, m_flEdgeOrAutoYaw ) ) < fabsf( Math::AngleDiff( flB, m_flEdgeOrAutoYaw ) ) ? flA : flB;
				}
				else {
					const float flAnimYaw = g_ServerAnimations.m_uServerAnimations.m_flEyeYaw;
					pCmd->viewangles.y = fabsf( Math::AngleDiff( flA, flAnimYaw ) ) > fabsf( Math::AngleDiff( flB, flAnimYaw ) ) ? flA : flB;
				}
				g_AntiAim.m_flLastPrebrakeAngle = pCmd->viewangles.y;

				g_ServerAnimations.m_uRenderAnimations.m_bDoingPreFlick = true;
				m_bLbyUpdateThisTick = true;
			}
		}
	}

	m_bLastPacket = *bSendPacket;
	pCmd->viewangles.y = Math::AngleNormalize( pCmd->viewangles.y );
}