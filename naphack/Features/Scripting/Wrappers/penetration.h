#pragma once
#include "../../../SDK/variables.hpp"
#include "../../Rage/Autowall.hpp"

namespace Wrappers::Penetration { 
	bool AimingAtPlayer( ) {
		return false;//return g_Vars.globals.m_bAimAtEnemyThruWallOrVisibleLoool;
	}	
	
	int GetPenDamage( ) {
		return static_cast< int >( g_Vars.globals.m_flPenetrationDamage );
	}

	std::tuple<float, int, int> SimulateBullet( Wrappers::Entity::CEntity entity, const Vector& start, const Vector& end ) {
		Autowall::FireBulletData data{ };
		data.m_bPenetration = true;
		data.m_TargetPlayer = nullptr;
		data.m_Player = entity.operator C_CSPlayer *( );
		data.m_vecStart = start;

		data.m_vecDirection = end - start;
		data.m_vecDirection.Normalize( );

		Autowall::FireBullets( &data );

		return std::make_tuple( static_cast< int >( data.m_flCurrentDamage ), 4 - data.m_iPenetrationCount, data.m_iHitgroup );
	}
}