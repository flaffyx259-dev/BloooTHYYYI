#include "../../pandora.hpp"
#include "../Hooked.hpp"
#include "../../SDK/Displacement.hpp"

#include "../../Loader/Exports.h"

void __fastcall Hooked::AddBoxOverlay( void* ecx, void* edx, const Vector& origin, const Vector& mins, const Vector& max, QAngle const& orientation, int r, int g, int b, int a, float duration ) {
	g_Vars.globals.szLastHookCalled = XorStr( "41" );

	if( !g_Vars.esp.server_impacts || uintptr_t( _ReturnAddress( ) ) != Engine::Displacement.Data.m_FireBulletsReturn )
		return oAddBoxOverlay( ecx, origin, mins, max, orientation, r, g, b, a, duration );

	return oAddBoxOverlay( ecx, origin, { -1.5, -1.5, -1.5 }, { 1.5, 1.5, 1.5 }, orientation,
		g_Vars.esp.client_impacts_color.r * 255, g_Vars.esp.client_impacts_color.g * 255, g_Vars.esp.client_impacts_color.b * 255, g_Vars.esp.client_impacts_color.a * 255,
		duration );
}