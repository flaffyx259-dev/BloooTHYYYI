#include "Knifebot.hpp"

#include "Ragebot.hpp"

#include "../../SDK/Classes/Player.hpp"
#include "../../SDK/Classes/weapon.hpp"

#include "../Visuals/EventLogger.hpp"

#include "AntiAim.hpp"
#include "Animations.hpp"

Knifebot g_Knifebot;

// s/o esoterik polak maddie d3x dex boris templar wzn nave ko1n imitator raxer zbe etc etc
// badster vice machete nitsuj wav kamay kolo arctosa searchy ingame1128 ducarii etc etc etc
bool Knifebot::CanKnife( LagRecord_t *pRecord, QAngle angAngle ) {
	if( !pRecord )
		return false;

	const Vector forward = angAngle.ToVectors( );

	CGameTrace trace;
	KnifeTrace( forward, false, &trace );

	if( !trace.hit_entity || trace.hit_entity != pRecord->m_pEntity )
		return false;

	const bool bHasArmor = pRecord->m_pEntity->m_ArmorValue( ) > 0;
	const bool bFirst = m_KnifeBotInfo.m_pWeapon->m_flNextPrimaryAttack( ) + 0.4f < g_pGlobalVars->curtime;
	const bool bBack = KnifeIsBehind( pRecord );

	const int nStabDamage = m_knifeDamage.stab[ bHasArmor ][ bBack ];
	const int nSlashDamage = m_knifeDamage.swing[ bFirst ][ bHasArmor ][ bBack ];
	const int nSwingDamage = m_knifeDamage.swing[ false ][ bHasArmor ][ bBack ];

	const int iHealth = pRecord->m_pEntity->m_iHealth( );
	if( iHealth <= nSlashDamage )
		m_KnifeBotInfo.m_bStab = false;

	else if( iHealth <= nStabDamage )
		m_KnifeBotInfo.m_bStab = true;

	else if( iHealth > ( nSlashDamage + nSwingDamage + nStabDamage ) )
		m_KnifeBotInfo.m_bStab = pRecord->m_bIsResolved;

	else
		m_KnifeBotInfo.m_bStab = false;

	if( m_KnifeBotInfo.m_bStab && !KnifeTrace( forward, true, &trace ) )
		return false;

	if( !trace.hit_entity || trace.hit_entity != pRecord->m_pEntity )
		return false;

	return true;
}

bool Knifebot::KnifeTrace( Vector vecDirection, bool bStab, CGameTrace *trace ) {
	if( !trace )
		return false;

	const float flRange = bStab ? 31.5f : 46.5f;
	const Vector vecEnd = m_KnifeBotInfo.m_vecEyePosition + ( vecDirection * flRange );

	CTraceFilter filter{ };
	filter.pSkip = m_KnifeBotInfo.m_pLocal;

	g_pEngineTrace->TraceRay( Ray_t( m_KnifeBotInfo.m_vecEyePosition, vecEnd ), MASK_SOLID, &filter, trace );

	// if the above failed try a hull trace.
	if( trace->fraction >= 1.f ) {
		g_pEngineTrace->TraceRay( Ray_t( m_KnifeBotInfo.m_vecEyePosition, vecEnd, { -16.f, -16.f, -18.f }, { 16.f, 16.f, 18.f } ), MASK_SOLID, &filter, trace );
		return trace->fraction < 1.f;
	}

	return true;
}

bool Knifebot::KnifeIsBehind( LagRecord_t *pRecord ) {
	if( !pRecord )
		return false;

	Vector vecDelta{ pRecord->m_vecOrigin - m_KnifeBotInfo.m_vecEyePosition };
	vecDelta.z = 0.f;
	vecDelta.Normalize( );

	Vector vecTarget = pRecord->m_sAnims[ESides::SIDE_SERVER].m_angAbsAngles.ToVectors( );
	vecTarget.z = 0.f;

	return vecDelta.Dot( vecTarget ) > 0.475f;
}

bool Knifebot::Run( bool *bSendPacket, CUserCmd *pCmd ) {
	if( !bSendPacket || !pCmd || !g_Vars.rage.auto_fire || !g_Vars.rage.enabled || !g_Vars.rage.key.enabled )
		return false;

	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return false;

	const auto pWeapon = reinterpret_cast< C_WeaponCSBaseGun * >( pLocal->m_hActiveWeapon( ).Get( ) );
	if( !pWeapon )
		return false;

	auto pWeaponData = pWeapon->GetCSWeaponData( );
	if( !pWeaponData.IsValid( ) )
		return false;

	if( pWeapon->m_iItemDefinitionIndex( ) == WEAPON_ZEUS || pWeaponData->m_iWeaponType != WEAPONTYPE_KNIFE )
		return false;

	if( !pLocal->CanShoot( ) )
		return false;

	// setup knifebot info variables
	m_KnifeBotInfo.m_pLocal = pLocal;
	m_KnifeBotInfo.m_pWeapon = pWeapon;
	m_KnifeBotInfo.m_vecEyePosition = pLocal->GetEyePosition( false );

	// this tick we did not find anything yet..
	m_KnifeBotInfo.m_bFound = false;
	m_KnifeBotInfo.m_bStab = false;
	m_KnifeBotInfo.m_angViewAngle = QAngle{ 0.f, 0.f, 0.f };
	m_KnifeBotInfo.m_flSimulationTime = 0.f;


	C_CSPlayer *const pEntity = g_AntiAim.GetBestPlayer( true );
	if( !pEntity
		|| pEntity->IsDead( )
		|| pEntity->IsDormant( )
		|| !pEntity->IsPlayer( )
		|| pEntity->IsTeammate( pLocal )
		|| pEntity->m_bGunGameImmunity( ) )
		return false;
	
	if( g_Ragebot.m_arrNapUsers.at( pEntity->EntIndex( ) ).first && !g_Vars.menu.whitelist_disable_key.enabled )
		return false;

	auto pAnimData = g_Animations.GetAnimationEntry( pEntity->m_entIndex );
	if( !pAnimData || pAnimData->m_deqRecords.empty( ) )
		return false;

	// setup backup record
	LagRecord_t backup{ };
	backup.SetupRecord( pEntity, true );

	if( g_Vars.cl_lagcompensation->GetInt( ) == 0 ) {
		auto &record = pAnimData->m_deqRecords.front( );

		for( const auto &angle : m_arrKnifeAngles ) {
			if( !CanKnife( &record, angle ) )
				continue;

			m_KnifeBotInfo.m_bFound = true;
			m_KnifeBotInfo.m_flSimulationTime = record.m_flSimulationTime;
			m_KnifeBotInfo.m_angViewAngle = angle;

			break;
		}
	}
	else {
		// safe up previous record for optimization
		LagRecord_t *previousRecord = nullptr;

		for( auto &record : pAnimData->m_deqRecords ) {
			if( !record.IsRecordValid() ) {
				continue;
			}

			if( !previousRecord
				|| previousRecord->m_vecOrigin != record.m_vecOrigin
				|| previousRecord->m_angEyeAngles.y != record.m_angEyeAngles.y
				|| previousRecord->m_vecMaxs != record.m_vecMaxs
				|| previousRecord->m_vecMins != record.m_vecMins ) {
				previousRecord = &record;

				record.ApplyRecord( pEntity );

				for( const auto &angle : m_arrKnifeAngles ) {
					if( !CanKnife( &record, angle ) )
						continue;

					m_KnifeBotInfo.m_bFound = true;
					m_KnifeBotInfo.m_flSimulationTime = record.m_flSimulationTime;
					m_KnifeBotInfo.m_angViewAngle = angle;

					break;
				}

				backup.ApplyRecord( pEntity );
			}
		}
	}

	if( m_KnifeBotInfo.m_bFound ) {
		// viewangle ::)
		pCmd->viewangles = m_KnifeBotInfo.m_angViewAngle;

		if( !g_Vars.rage.silent_aim )
			g_pEngine->SetViewAngles( pCmd->viewangles );

		// SET THE RIGHT BUTTONS
		pCmd->buttons |= !m_KnifeBotInfo.m_bStab ? IN_ATTACK : IN_ATTACK2;

		// set tick 
		if( g_Vars.cl_lagcompensation->GetInt( ) == 1 )
			pCmd->tick_count = TIME_TO_TICKS( m_KnifeBotInfo.m_flSimulationTime ) + TIME_TO_TICKS( g_Animations.m_fLerpTime );

		// send this :)
		*bSendPacket = false;
	}

	return m_KnifeBotInfo.m_bFound;
}
