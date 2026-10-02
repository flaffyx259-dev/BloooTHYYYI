#include "gui.h"
#include "../../pandora.hpp"

#include "../../Features/Visuals/EventLogger.hpp"

Vector2D last_cursor_pos;

void GUI::CopyReference( bool bForceCantReference ) {
	// this won't work if you press m3 and then ctrl, but oh well.
	if( !InputHelper::Down( VK_CONTROL ) )
		return;

	if( !InputHelper::Pressed( VK_MBUTTON ) )
		return;

	if( !bForceCantReference && ctx->CurrentTab != XorStr( "scripts" ) && ctx != pSkinchanger ) {
		if( OpenClipboard( nullptr ) ) {
			EmptyClipboard( );
			char site_buffer[ 128 ] = {};

			if ( ctx->CurrentWeaponGroup.empty( ) )
				sprintf( site_buffer, XorStr( "ui.get(\"%s\", \"%s\", \"%s\")" ), ctx->CurrentTab.data(  ), /*ctx->CurrentSubTab.data( ),*/ ctx->CurrentGroup.data( ), GUI::SplitStr( ctx->szLastElementName.data( ), '#' )[ 0 ].data( ) );
			else
				sprintf( site_buffer, XorStr( "ui.get(\"%s\", \"%s\", \"%s\", \"%s\")" ), ctx->CurrentTab.data( ),/* ctx->CurrentSubTab.data( ),*/ ctx->CurrentGroup.data( ), GUI::SplitStr( ctx->szLastElementName.data( ), '#' )[ 0 ].data( ), ctx->CurrentWeaponGroup.data( ) );

			HGLOBAL clipboard_buffer = GlobalAlloc( GMEM_DDESHARE, sizeof( site_buffer ) + 1 );
			if( clipboard_buffer ) {
				char *buffer = ( char * )GlobalLock( clipboard_buffer );
				if( buffer ) {
					strcpy( buffer, site_buffer );

					GlobalUnlock( clipboard_buffer );
					SetClipboardData( CF_TEXT, clipboard_buffer );

					g_EventLog.PushEvent( XorStr( "copied ui reference to clipboard" ), Color_f( 1.f, 1.f, 1.f ), false, XorStr("LUA" ) );
				} else {
					g_EventLog.PushEvent( XorStr( "failed to copy ui reference to clipboard (unexpected error)" ), Color_f( 1.f, 1.f, 1.f ) );
				}
			} else {
				g_EventLog.PushEvent( XorStr( "failed to copy ui reference to clipboard (unexpected error)" ), Color_f( 1.f, 1.f, 1.f ) );
			}

			CloseClipboard( );

			return;
		}
	}

	g_EventLog.PushEvent( XorStr( "failed to copy ui reference to clipboard. (unable to reference)" ), Color_f( 1.f, 1.f, 1.f ) );
}

void GUI::SetNextTooltip( std::string tooltip, std::string highlighted ) {
	ctx->tooltip = std::make_pair( tooltip, highlighted );
}

bool GUI::ContextMenuOpen( ) {
	return ( !ctx->HotkeyInfo.bLeftClicked && ctx->HotkeyInfo.HashedID != 0 ) ||
		( ctx->ColorPickerInfo.bRightClicked && ctx->ColorPickerInfo.HashedID != 0 ) ||
		ctx->MultiDropdownInfo.HashedID != 0 ||
		ctx->DropdownInfo.HashedID != 0;
}

Vector2D GUI::PopCursorPos( ) {
	if( ctx->CursorPosStack.empty( ) )
		return Vector2D( );

	Vector2D ret = ctx->CursorPosStack.top( );
	ctx->CursorPosStack.pop( );
	last_cursor_pos = ret;
	return ret;
}

Vector2D GUI::GetLastCursorPos( ) {
	return last_cursor_pos;
}

void GUI::PushCursorPos( const Vector2D &cursor_pos ) {
	ctx->CursorPosStack.push( cursor_pos );
}

void GUI::AdjustGroupPos( const Vector2D &cursor_pos ) {
	ctx->NextGroupAdj = cursor_pos;
}

size_t GUI::Hash( const std::string &name ) {
	return std::hash< std::string >( )( name );
}

float GUI::AnimationInterval( float interval ) {
	return ( 1.0f / interval ) * g_pGlobalVars->frametime;
}

float GUI::MapNumber( float input, float input_min, float input_max, float output_min, float output_max ) {
	return ( input - input_min ) / ( input_max - input_min ) * ( output_max - output_min ) + output_min;
}

std::vector<std::string> GUI::SplitStr( const std::string &str, char separator ) {
	std::vector<std::string> output;
	std::string::size_type prev_pos = 0, pos = 0;

	while( ( pos = str.find( separator, pos ) ) != std::string::npos ) {
		std::string substring( str.substr( prev_pos, pos - prev_pos ) );
		output.push_back( substring );

		prev_pos = pos++;
	}

	output.push_back( str.substr( prev_pos, pos - prev_pos ) );
	return output;
}

void GUI::DrawArrow( Vector2D vecPos, Color clr, bool bFaceDown ) {
	if( !bFaceDown ) {
		vecPos.y -= 4;
		vecPos.x += 1;//idk
	}

	static int count = 0;
	for( int i = 7; i > 0; i -= 2 ) {
		count++;

		// the top line
		if( ( i == 7 && bFaceDown ) || ( i == 1 && !bFaceDown ) ) {
			if( bFaceDown )
				Render::Engine::Rect( vecPos + Vector2D( count, count - 1 ), Vector2D( i, 1 ), Color::Black( ).OverrideAlpha( clr.a( ) ) );
			else
				Render::Engine::Rect( vecPos + Vector2D( ( 4 - count ), 9 - ( 4 - count ) ), Vector2D( 8 - i, 1 ), Color::Black( ).OverrideAlpha( clr.a( ) ) );
		}

		if( bFaceDown ) {
			Render::Engine::Rect( vecPos + Vector2D( count, count ), Vector2D( i, 1 ), clr );
		}
		else {
			Render::Engine::Rect( vecPos + Vector2D( ( 4 - count ), 8 - ( 4 - count ) ), Vector2D( 8 - i, 1 ), clr );
		}
	}

	count = 0;
}