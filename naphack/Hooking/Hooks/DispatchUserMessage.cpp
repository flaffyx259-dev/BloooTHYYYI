#include "../hooked.hpp"
#include "../../SDK/Classes/player.hpp"
#include "../../SDK/variables.hpp"

bool __fastcall Hooked::DispatchUserMessage( void *ECX, void *EDX, int msg_type, int unk1, int nBytes, bf_read &msg_data ) {
	if( !g_pGameRules.IsValid( ) )
		return oDispatchUserMessage( ECX, msg_type, unk1, nBytes, msg_data );

	g_Vars.globals.szLastHookCalled = XorStr( "4" );

	if( msg_type == /*CS_UM_TextMsg*/ 7 || msg_type == /*CS_UM_HudMsg*/8 )
		if( g_Vars.misc.disable_server_messages && !g_pGameRules->m_bIsValveDS( ) )
			return true;

	return oDispatchUserMessage( ECX, msg_type, unk1, nBytes, msg_data );
}
