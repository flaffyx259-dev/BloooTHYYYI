#pragma once
#include "../../pandora.hpp"

namespace Autowall
{
	struct FireBulletData {
		Vector m_vecStart = Vector( 0, 0, 0 );
		Vector m_vecDirection = Vector( 0, 0, 0 );

		CGameTrace m_EnterTrace;

		int m_iPenetrationCount = 4;
		int m_iHitgroup = -1;

		float m_flTraceLength = 0.f;
		float m_flCurrentDamage = -1.f;
		int m_nMinimumDamage = -1;

		// distance to point
		float m_flMaxLength = 0.0f;
		float m_flPenetrationDistance = 0.0f;

		// should penetrate walls? 
		bool m_bPenetration = false;
		C_CSPlayer* m_Player = nullptr; // attacker
		C_CSPlayer* m_TargetPlayer = nullptr;  // autowall target ( could be nullptr if just trace attack )
		C_WeaponCSBaseGun* m_Weapon = nullptr; // attacker weapon
		CCSWeaponInfo* m_WeaponData = nullptr;

		surfacedata_t* m_EnterSurfaceData = nullptr;
		std::vector<Vector> m_vecPenetratedPositions;
	};

	bool IsBreakable( C_BaseEntity* entity );
	bool IsArmored( C_CSPlayer* player, int hitgroup );
	float ScaleDamage( C_CSPlayer* player, float damage, float armor_ratio, int hitgroup, C_CSPlayer* pOverride = nullptr);
	void TraceLine( const Vector& start, const Vector& end, uint32_t mask, ITraceFilter* ignore, CGameTrace* ptr );
	void ClipTraceToPlayers( const Vector& vecAbsStart, const Vector& vecAbsEnd, uint32_t mask, ITraceFilter* filter, CGameTrace* tr, float flMaxRange = 60.0f, float flMinRange = 0.0f );
	void ClipTraceToPlayer( const Vector vecAbsStart, const Vector& vecAbsEnd, uint32_t mask, ITraceFilter* filter, CGameTrace* tr, C_CSPlayer* player );
	bool TraceToExit( CGameTrace* enter_trace, Vector start, Vector direction, CGameTrace* exit_trace, C_CSPlayer *player );
	bool HandleBulletPenetration( FireBulletData* data );
	float FireBullets( FireBulletData* data );
	int ClipRayToHitbox( const Ray_t& ray, mstudiobbox_t* pBox, matrix3x4_t& matrix, CGameTrace& tr );
	float GetPenetrationDamage( C_CSPlayer *pLocal, C_WeaponCSBaseGun *pWeapon );
}

namespace penetration {
	struct PenetrationInput_t {
		Vector m_start;

		C_CSPlayer *m_from;
		C_CSPlayer *m_target;
		Vector  m_pos;
		float	m_damage;
		bool	m_can_pen;

		float m_flMaxLength;
	};

	struct PenetrationOutput_t {
		C_CSPlayer *m_target;
		float   m_damage;
		int     m_hitgroup;
		bool    m_pen;
		int		m_pen_count;

		__forceinline PenetrationOutput_t( ) : m_target{ nullptr }, m_damage{ 0.f }, m_hitgroup{ -1 }, m_pen{ false } {}
	};

	bool  run( PenetrationInput_t *in, PenetrationOutput_t *out );
}