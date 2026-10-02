#include "../../pandora.hpp"
#include "../Hooked.hpp"
#include "../../SDK/Displacement.hpp"

bool __fastcall Hooked::ProcessTempEntities( void* ecx, void* edx, void* msg )
{
	auto backup = g_pClientState->m_nMaxClients( );

	g_pClientState->m_nMaxClients( ) = 1;
	bool ret = oProcessTempEntities( ecx, msg );
	g_pClientState->m_nMaxClients( ) = backup;

//	Hooked::CL_FireEvents( );
	g_pEngine->FireEvents( );

	return ret;
}
