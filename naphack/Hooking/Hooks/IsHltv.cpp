#include "../../pandora.hpp"
#include "../Hooked.hpp"
#include "../../SDK/Displacement.hpp"
#include "../../SDK/Classes/Player.hpp"

bool __fastcall Hooked::IsHltv( IVEngineClient* EngineClient, uint32_t ) {
	if( g_Vars.globals.m_bInBoneSetup )
		return true;

	if( _ReturnAddress( ) == ( void* )Engine::Displacement.Data.m_uRetSetupVelocity && g_Vars.globals.m_bUpdatingAnimations && g_pEngine->IsInGame( ) )
		return true;

	if( _ReturnAddress( ) == ( void * )Engine::Displacement.Data.m_uRetAccumulateLayers )
		return true;

	return oIsHltv( EngineClient );
}