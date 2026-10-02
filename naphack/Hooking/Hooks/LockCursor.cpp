#include "../Hooked.hpp"
#include "../../SDK/Classes/Player.hpp"

void __stdcall Hooked::LockCursor( ) {
   g_Vars.globals.szLastHookCalled = XorStr( "16" );

   if( !g_pEngine.IsValid( ) || !g_pSurface.IsValid( ) ) {
	   if( g_pSurface.IsValid( ) ) {
		   oLockCursor( ( void* )g_pSurface.Xor( ) );
	   }

	   return;
   }

   oLockCursor( ( void* )g_pSurface.Xor( ) );

   auto pLocal = C_CSPlayer::GetLocalPlayer( );

   bool state = true;
   if( !g_pEngine->IsInGame( ) || ( pLocal != nullptr ) || GUI::ctx->typing ) {
	   if( pLocal != nullptr ) {
		   if( pLocal->IsDead( ) ) {
			   state = !g_Vars.globals.m_bDisableInput;
		   }
		   else {
			   if( GUI::ctx->typing || !g_pEngine->IsInGame( ) ) {
				   state = !g_Vars.globals.m_bDisableInput;
			   }
		   }
	   }
	   else {
		   if( GUI::ctx->typing || !g_pEngine->IsInGame( ) ) {
			   state = !g_Vars.globals.m_bDisableInput;
		   }
	   } 
   }

   g_pInputSystem->EnableInput( state );

   if ( g_Vars.globals.m_bDisableInput )
	  g_pSurface->UnlockCursor( );
}
