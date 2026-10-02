#include <algorithm>
#include "../gui.h"
#include "../../../pandora.hpp"
#include "../../../Features/Scripting/Scripting.hpp"


std::map< int, std::string > _szText;
std::map<size_t, float> _LastShiftHoldTime;

struct key_code_info {
	int vk;

	wchar_t regular;
	wchar_t shift;
};

key_code_info special_characters[ 22 ] = {
	{ 48, L'0', L')' },
	{ 49, L'1', L'!' },
	{ 50, L'2', L'@' },
	{ 51, L'3', L'#' },
	{ 52, L'4', L'$' },
	{ 53, L'5', L'%' },
	{ 54, L'6', L'^' },
	{ 55, L'7', L'&' },
	{ 56, L'8', L'*' },
	{ 57, L'9', L'(' },
	{ 192, L'`', L'~' },
	{ 189, L'-', L'_' },
	{ 187, L'=', L'+' },
	{ 219, L'[', L'{' },
	{ 220, L'\\', L'|' },
	{ 221, L']', L'}' },
	{ 186, L';', L':' },
	{ 222, L'\'', L'"' },
	{ 188, L',', L'<' },
	{ 190, L'.', L'>' },
	{ 191, L'/', L'?' }
};

bool GUI::Controls::Textbox( std::string name, std::string *input, int max_text, bool numbers_only ) {
	Vector2D CursorPos = PopCursorPos( );
	Vector2D DrawPos = ctx->pos + CursorPos;

	std::transform( name.begin( ), name.end( ), name.begin( ), ::tolower );

	Vector2D DrawSize( ctx->ParentSize.x - 25, 18 );

	if( _szText.find( GUI::Hash( name ) ) == _szText.end( ) ) {
		_szText.insert( { GUI::Hash( name ), "" } );
	}

	if( _LastShiftHoldTime.find( GUI::Hash( name ) ) == _LastShiftHoldTime.end( ) ) {
		_LastShiftHoldTime.insert( { GUI::Hash( name ), 0.f } );
	}

	const float flBackupMultiplier = g_pSurface->DrawGetAlphaMultiplier( );

	if( !ctx->enabled )
		g_pSurface->DrawSetAlphaMultiplier( flBackupMultiplier * 0.3f );

	bool bHoveredSearch = ctx->bActiveWindow && ctx->enabled && g_InputSystem.IsInBox( DrawPos + Vector2D( 0, 12 ), Vector2D( DrawSize.x, DrawSize.y ) ) && g_InputSystem.IsInBox( ctx->ParentPos, ctx->ParentSize );

	Render::Engine::menu_regular.string( DrawPos.x + 1, DrawPos.y - 3,
										 ( !( *input ).empty( ) || ctx->FocusedID == GUI::Hash( name ) ) ? Color( 150, 150, 150 ) : ( ( bHoveredSearch && ( ctx->FocusedID == 0 || ctx->FocusedID == GUI::Hash( name ) ) ) ? Color( 125, 125, 125 ) : Color( 75, 75, 75 ) ), GUI::SplitStr( name, '#' )[ 0 ].data( ) );

	DrawPos.y += 12;

	// search
	Render::Engine::RectFilled( DrawPos, DrawSize, Color( 25, 25, 25 ) );

	if( ctx->FocusedID == 0 ) {
		if( ctx->typing )
			ctx->typing = false;

		if( bHoveredSearch && g_InputSystem.IsKeyDown( VK_LBUTTON ) )
			ctx->FocusedID = GUI::Hash( name );
	}
	else if( ctx->FocusedID == GUI::Hash( name ) ) {
		if( !ctx->typing )
			ctx->typing = true;

		if( g_InputSystem.WasKeyPressed( VK_LBUTTON ) && !bHoveredSearch ) {
			ctx->FocusedID = 0;
		}

		for( int i = ( int )'A'; i <= ( int )'Z'; i++ ) {
			if( isalpha( i ) ) {
				if( InputHelper::Pressed( i ) ) {
					( *input ) += ( ( g_InputSystem.IsKeyDown( VK_SHIFT ) ) || GetKeyState( VK_CAPITAL ) ) ? i : i + 32;
				}
			}
		}

		for( int i = 0; i < 22; i++ )
			if( InputHelper::Pressed( special_characters[ i ].vk ) )
				( *input ) += InputHelper::Down( VK_SHIFT ) ? special_characters[ i ].shift : special_characters[ i ].regular;

		if( InputHelper::Pressed( VK_BACK ) )
			( *input ) = ( *input ).substr( 0, ( *input ).size( ) - 1 );

		if( InputHelper::Pressed( VK_ESCAPE ) || InputHelper::Pressed( VK_RETURN ) )
			ctx->FocusedID = 0;

		// if we arent holding backspace store the time
		if( !GetAsyncKeyState( VK_BACK ) ) {
			_LastShiftHoldTime[ GUI::Hash( name ) ] = g_pGlobalVars->curtime;
		}

		// the delta between the current time and last "non held shift" time exceeds 500ms,
		// this means that we've been holding shift for 500ms. let's erase.
		if( g_pGlobalVars->curtime - _LastShiftHoldTime[ GUI::Hash( name ) ] > 0.5f ) {
			*input = ( *input ).substr( 0, ( *input ).size( ) - 1 );
		}
	}

	( *input ) = ( *input ).substr( 0, max_text );

	auto txt = ( *input );

	Render::Engine::menu_small.string( DrawPos.x + 5, DrawPos.y + 3,
									   ( ctx->FocusedID == GUI::Hash( name ) ) ? Color( 150, 150, 150 ) : ( ( bHoveredSearch && ( ctx->FocusedID == 0 || ctx->FocusedID == GUI::Hash( name ) ) ) ? Color( 125, 125, 125 ) : Color( 75, 75, 75 ) ),
									   ( txt + ( ctx->FocusedID == GUI::Hash( name ) ? '_' : ' ' ) ).c_str( ) );

	Render::Engine::Rect( DrawPos, DrawSize, Color::Black( ) );
	Render::Engine::Rect( DrawPos + 1, DrawSize - 2, Color::Palette_t::ElementOutlines( ) );

	if( !ctx->enabled )
		g_pSurface->DrawSetAlphaMultiplier( flBackupMultiplier );

#if defined(LUA_SCRIPTING)
	if( ctx->setup ) {
		auto item = new Scripting::textBoxItem;
		item->value = input;

		std::string yep = std::string( ctx->CurrentTab ).append( XorStr( "." ) )/*.append( ctx->CurrentSubTab ).append( XorStr( "." ) )*/.append( ctx->CurrentGroup ).append( XorStr( "." ) ).append( GUI::SplitStr( name, '#' )[ 0 ].data( ) ).append( XorStr( "." ) ).append( ctx->CurrentWeaponGroup );
		std::transform( yep.begin( ), yep.end( ), yep.begin( ), ::tolower );

		Scripting::gui_items.emplace_back( hash_32_fnv1a( yep.data( ) ), item );
	}
#endif

	ctx->szLastElementName = GUI::SplitStr( name, '#' )[ 0 ];
	if( ctx->FocusedID == 0 && bHoveredSearch ) {
		GUI::CopyReference( );
	}

	PushCursorPos( CursorPos + Vector2D( 0, DrawSize.y + Render::Engine::menu_regular.size( name ).m_height + GUI::ObjectPadding( ) ) );
	return ctx->FocusedID == GUI::Hash( name );
}