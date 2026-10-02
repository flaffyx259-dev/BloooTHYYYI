#include "InputHelper.h"
#include "../../pandora.hpp"

#include "../../Utils/extern/lazy_importer.hpp"

#include <Windows.h>

void InputHelper::Update( ) {
	HWND focused = GetFocus( );

	// huge fps issue.
	//static auto BIG_FUNC = LI_FN( GetAsyncKeyState );

	for( int i = 0; i < 256; i++ ) {
		if( focused == Interfaces::hWindow ) {
			PrevKeyState[ i ] = KeyState[ i ];
			KeyState[ i ] = GetAsyncKeyState( i );

			g_Vars.globals.m_bIsGameFocused = true;
		}
		else {
			PrevKeyState[ i ] = false;
			KeyState[ i ] = false;

			g_Vars.globals.m_bIsGameFocused = false;
		}
	}
}

bool InputHelper::Down( int key ) {
	return KeyState[ key ] && PrevKeyState[ key ];
}

bool InputHelper::Pressed( int key ) {
	return KeyState[ key ] && !PrevKeyState[ key ];
}

bool InputHelper::Released( int key ) {
	return !KeyState[ key ] && PrevKeyState[ key ];
}