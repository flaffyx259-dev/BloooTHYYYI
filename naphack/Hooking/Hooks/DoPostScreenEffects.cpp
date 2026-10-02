#include "../Hooked.hpp"
#include "../../Features/Visuals/Models.hpp"
#include "../../Features/Visuals/Glow.hpp"
#include "../../SDK/variables.hpp"

namespace Hooked
{
	int __fastcall DoPostScreenEffects( void* ecx, void*, int a1 ) {
		g_Vars.globals.szLastHookCalled = XorStr( "5" );

		g_Glow.OnPostScreenEffects( );
		g_Models.HandleAimbotMatrices( );
		g_Models.HandleDormantMatrices( );

		g_Vars.globals.m_bInPostScreenEffects = true;
		auto result = oDoPostScreenEffects( ecx, a1 );
		g_Vars.globals.m_bInPostScreenEffects = false;

		return result;
	}
}
