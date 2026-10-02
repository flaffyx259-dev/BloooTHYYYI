#include "../../pandora.hpp"
#include "../Hooked.hpp"
#include "../../SDK/Displacement.hpp"
#include "../../SDK/Classes/Player.hpp"

void __fastcall Hooked::OnDataChanged( C_CSPlayer* pPlayer, uint32_t, int a1 ) {
	g_Vars.globals.szLastHookCalled = XorStr( "13413231" );

	// https://github.com/perilouswithadollarsign/cstrike15_src/blob/f82112a2388b841d72cb62ca48ab1846dfcc11c8/game/client/cstrike15/c_cs_player.cpp#L5357
	// we don't want us scoping in/out interfering with fog related stuff
	// m_bOldIsScoped = m_bIsScoped
	// *( bool* )( uintptr_t( pPlayer ) + 0xB314 ) = *( bool* )( uintptr_t( pPlayer ) + 0x3920 );

	// method above doesn't work.....
	const auto v14 = *( bool* )( uintptr_t( pPlayer ) + 0x3920 );
	*( bool* )( uintptr_t( pPlayer ) + 0x3920 ) = false;
	oOnDataChanged( pPlayer, a1 );
	*( bool* )( uintptr_t( pPlayer ) + 0x3920 ) = v14;

	// note - michal;
	// ok well neither of the methods work....
	// haven't tried this yet coz i'm tired, but try hooking C_BasePlayer::FogControllerChanged( )
	// and setting bSnap to false before original call, if this causes the fog to not snap and instead
	// fade out, then we can manipulate curtime there and restore after original call (to some stupid
	// value that won't make it fade out, idk)
	
	// here u go =D
	// https://i.imgur.com/854OL5P.png
	// direct reference: [actual address in first opcode] E8 ? ? ? ? 83 3D ? ? ? ? ? 74 2B 
}