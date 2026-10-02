#include "DormantAimbot.hpp"
#include "Ragebot.hpp"
#include "../Miscellaneous/PlayerList.hpp"
#include "../Visuals/Visuals.hpp"
#include "TickbaseShift.hpp"
#include "EnginePrediction.hpp"
#include "../Visuals/EventLogger.hpp"

DormantAimbot g_DormantAimbot;

void DormantAimbot::Run( ) {
	return;
}


int DormantAimbot::GetMinimalDamage( C_CSPlayer *pPlayer ) {
	if( !pPlayer )
		return 420;

	auto GetHPDamage = [ & ] ( int nDamage ) {
		const int nPlayerHP = pPlayer->m_iHealth( );
		return nDamage > 100 ? ( nPlayerHP + ( nDamage - 100 ) ) : nDamage;
	};

	int returnDamage = GetHPDamage( g_Ragebot.m_AimbotInfo.m_pSettings->minimal_damage );

	if( g_Vars.rage.min_damage_override_key.enabled ) {
		returnDamage = GetHPDamage( g_Ragebot.m_AimbotInfo.m_pSettings->minimal_damage_override );
	}

	const float flMaxBodyDamage = Autowall::ScaleDamage( pPlayer, g_Ragebot.m_AimbotInfo.m_pWeaponInfo->m_iWeaponDamage, g_Ragebot.m_AimbotInfo.m_pWeaponInfo->m_flArmorRatio, Hitgroup_Stomach ) - 1.f;

	// automatic min dmg
	if( returnDamage == 0 ) {

		// force lethal shot
		if( flMaxBodyDamage > pPlayer->m_iHealth( ) ) {
			returnDamage = flMaxBodyDamage;


			returnDamage = std::min( returnDamage, pPlayer->m_iHealth( ) + 1 );
		}

		// cant kill with one lethal shot, aim to kill in 2 bullets
		if( pPlayer->m_iHealth( ) > flMaxBodyDamage ) {
			int tempDamage = flMaxBodyDamage / 2;
			returnDamage = flMaxBodyDamage / 2;
		}

		returnDamage *= 0.4f;
	}

	if( g_Ragebot.m_AimbotInfo.m_pSettings->scale_damage_on_hp && returnDamage <= 100 )
		//returnDamage *= static_cast< float >( pPoint->m_pTarget->m_pEntity->m_iHealth( ) ) / 100.f;
		returnDamage = std::min( returnDamage, pPlayer->m_iHealth( ) );

	if( returnDamage > flMaxBodyDamage && pPlayer->m_iHealth( ) < flMaxBodyDamage * 0.5f ) {
		returnDamage = flMaxBodyDamage * 0.5f;
	}

	return returnDamage;
}