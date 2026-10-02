#pragma once
#include "../../SDK/Classes/Player.hpp"
#include <functional>

class Glow {
public:
	void OnPostScreenEffects( );
	void ApplyPlayerOutline( C_CSPlayer* pEntity, Color_f flColor, std::function<void()> oRenderModel );
};

extern Glow g_Glow;