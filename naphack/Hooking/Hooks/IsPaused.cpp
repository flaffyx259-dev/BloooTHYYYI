#include "../Hooked.hpp"
#include "../../SDK/Displacement.hpp"
#include "../../Features/Rage/TickbaseShift.hpp"

bool __fastcall Hooked::IsPaused( void* ecx, void* edx ) {
	static const auto ExtrapolationRet = reinterpret_cast< void* >( Engine::Displacement.Data.m_uRetExtrapolation );

	if( g_TickbaseController.m_bOverrideIsPaused || ( _ReturnAddress( ) == ExtrapolationRet ) ) {
		return true;
	}

	return oIsPaused( ecx );
}

bool __fastcall Hooked::InterpolateViewmodel( void* ecx, void* edx, float curtime ) {
	g_TickbaseController.m_bOverrideIsPaused = g_TickbaseController.m_bInCharge;

	auto ret = oInterpolateViewmodel( ecx, curtime );

	g_TickbaseController.m_bOverrideIsPaused = false;

	return ret;
}