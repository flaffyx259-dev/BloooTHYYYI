#include "../../pandora.hpp"
#include "../Hooked.hpp"


void __fastcall Hooked::PerformScreenOverlay( void *ecx, void *edx, int a1, int a2, int a3, int a4 ) {
	if( !ecx || !g_pGameRules.IsValid( ) )
		return;

	if( !g_Vars.misc.disable_server_messages || g_pGameRules->m_bIsValveDS( ) )
		return oPerformScreenOverlay( ecx, a1, a2, a3, a4 );
}