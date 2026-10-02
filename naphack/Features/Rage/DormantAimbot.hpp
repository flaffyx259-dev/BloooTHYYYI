#pragma once
#include "../../SDK/sdk.hpp"
#include "Animations.hpp"
#include "Autowall.hpp"

class DormantAimbot {
public:
	void Run( );

private:
	int GetMinimalDamage( C_CSPlayer *pPlayer );
};

extern DormantAimbot g_DormantAimbot;