#pragma once
#include "../../SDK/sdk.hpp"

// forward dec.
class LagRecord_t;

class Knifebot {
	struct Knifebot_t {
		C_CSPlayer* m_pLocal;
		C_WeaponCSBaseGun* m_pWeapon;
		Vector m_vecEyePosition;

		bool m_bFound;

		bool m_bStab;
		QAngle m_angViewAngle;
		float m_flSimulationTime;
	};

private:

	struct table_t {
		uint8_t swing[ 2 ][ 2 ][ 2 ]; // [ first ][ armor ][ back ]
		uint8_t stab[ 2 ][ 2 ];		  // [ armor ][ back ]
	};

	const table_t m_knifeDamage{ { { { 25, 90 }, { 21, 76 } }, { { 40, 90 }, { 34, 76 } } }, { { 65, 180 }, { 55, 153 } } };

	std::array< QAngle, 12 > m_arrKnifeAngles {
		QAngle{ 0.f, 0.f, 0.f }, QAngle{ 0.f, -90.f, 0.f }, QAngle{ 0.f, 90.f, 0.f }, QAngle{ 0.f, 180.f, 0.f },
		QAngle{ -80.f, 0.f, 0.f }, QAngle{ -80.f, -90.f, 0.f }, QAngle{ -80.f, 90.f, 0.f }, QAngle{ -80.f, 180.f, 0.f },
		QAngle{ 80.f, 0.f, 0.f }, QAngle{ 80.f, -90.f, 0.f }, QAngle{ 80.f, 90.f, 0.f }, QAngle{ 80.f, 180.f, 0.f }
	};

	Knifebot_t m_KnifeBotInfo;

	bool CanKnife( LagRecord_t* pRecord, QAngle angAngle );
	bool KnifeTrace( Vector vecDirection, bool bStab, CGameTrace* trace );
	bool KnifeIsBehind( LagRecord_t* pRecord );

public:
	bool Run( bool* bSendPacket, CUserCmd* pCmd );
};

extern Knifebot g_Knifebot;