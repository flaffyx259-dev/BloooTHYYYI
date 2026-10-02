#include "../../pandora.hpp"
#include "../Hooked.hpp"
#include "../../SDK/Displacement.hpp"


void __fastcall Hooked::DrawSetColor( ISurface* thisptr, void* edx, int r, int g, int b, int a ) {
	//g_Vars.globals.szLastHookCalled = XorStr( "45" );

	if( !g_Vars.esp.remove_scope ) {
		return oDrawSetColor( thisptr, r, g, b, a );
	}

	if( uintptr_t( _ReturnAddress( ) ) == Engine::Displacement.Data.m_uRetScopeArc
		|| uintptr_t( _ReturnAddress( ) ) == Engine::Displacement.Data.m_uRetScopeLens ) {
		// We don't want this to draw, so we set the alpha to 0
		return oDrawSetColor( thisptr, r, g, b, 0 );
	}

	if( ( uintptr_t( _ReturnAddress( ) ) != Engine::Displacement.Data.m_uRetScopeClear &&
			uintptr_t( _ReturnAddress( ) ) != Engine::Displacement.Data.m_uRetScopeBlur ) )
		return oDrawSetColor( thisptr, r, g, b, a );

	oDrawSetColor( thisptr, r, g, b, 0 );
}