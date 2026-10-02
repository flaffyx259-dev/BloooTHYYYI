#pragma once

#include "../../SDK/sdk.hpp"

struct impact_t {
	float x, y, z;
	float time;
	int m_iHurtEventID = -1;
};

struct hitmarker_t {
	impact_t impact;
	int damage;
	float time, alpha, distance;
	float enlargment, expiry;
	float move_damage;
	int index;
};

class Hitmarker {
	void PlayerHurt( IGameEvent* pEvent );
	void BulletImpact( IGameEvent* pEvent );

	std::vector<impact_t> m_vecImpacts;
	std::vector<hitmarker_t> m_vecHitmarkers;

	bool m_bFirstMarker;
	float m_flRandomRotation;
	float m_flRandomEnlargement;
	int m_iHurtEventID;
public:
	float m_flMarkerAlpha;
	float m_flMarkerTime;
	Color m_uMarkerColor;

public:
	void Draw( );
	void GameEvent( IGameEvent* pEvent );
};

extern Hitmarker g_Hitmarker;