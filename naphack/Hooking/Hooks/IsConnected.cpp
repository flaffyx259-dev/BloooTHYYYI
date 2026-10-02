#include "../hooked.hpp"
#include "../../SDK/Classes/Player.hpp"
#include "../../SDK/Displacement.hpp"

bool __fastcall Hooked::IsConnected( void ) {
	/*
	- string: "IsLoadoutAllowed"
	- follow up v8::FunctionTemplate::New function
	- inside it go to second function that is being called after "if" statement.
	- after that u need to open first function that is inside it.[ before( *( int ( ** )( void ) )( *( _DWORD* ) dword_152350E4 + 516 ) )( ); ]
	*/

	// g_Vars.globals.szLastHookCalled = XorStr( "13" );

	static uintptr_t force_inventory_open = Memory::Scan( XorStr( "client.dll" ), XorStr( "75 04 B0 01 5F" ) ) - 2;
	if( _ReturnAddress( ) == ( void * )force_inventory_open && g_Vars.misc.unlock_inventory ) {
		return false;
	}

	return Hooked::oIsConnected( );
}