#include "../gui.h"
#include <algorithm>
#include "../../../pandora.hpp"
#include "../../../Features/Scripting/Scripting.hpp"

std::unordered_map<size_t, float> uHoverAnimation_;
bool GUI::Controls::MultiDropdown( std::string name, std::vector< MultiItem_t > options, int max_items ) {
	Vector2D CursorPos = PopCursorPos( );
	Vector2D DrawPos = ctx->pos + CursorPos;
	DrawPos.y += 1;
	Vector2D DrawSize = Vector2D( ctx->ParentSize.x - 25, 19 );

	std::transform( name.begin( ), name.end( ), name.begin( ), ::tolower );

	const float flBackupMultiplier = g_pSurface->DrawGetAlphaMultiplier( );

	if( !ctx->enabled )
		g_pSurface->DrawSetAlphaMultiplier( flBackupMultiplier * 0.3f );

	bool bHovered = ctx->bActiveWindow && ctx->enabled && g_InputSystem.IsInBox( DrawPos + Vector2D( 0, 12 ), DrawSize )
		&& g_InputSystem.IsInBox( ctx->ParentPos, ctx->ParentSize );
	bool bActive = ctx->bActiveWindow && ctx->enabled && ctx->MultiDropdownInfo.HashedID == GUI::Hash( name );

	char buf[ 128 ] = { 0 };

	bool bRemovingChars = false;
	for( int i = 0; i < options.size( ); i++ ) {
		auto &it = options.at( i ).value;

		if( !*it )
			continue;

		strcat( buf, options.at( i ).name.data( ) );

		if( i != options.size( ) - 1 )
			strcat( buf, ( ", " ) );

		auto max_text_width = DrawSize.x - 10;
		if( Render::Engine::menu_regular.size( buf ).m_width > max_text_width ) {
			// remove last character until string width is okay
			if( max_text_width >= 1 ) {
				while( Render::Engine::menu_regular.size( buf ).m_width > max_text_width || buf[ strlen( buf ) - 1 ] == ',' || buf[ strlen( buf ) - 1 ] == ' ' ) {
					buf[ strlen( buf ) - 1 ] = '\0';
					bRemovingChars = true;
				}
			}

			break;
		}
	}

	if( !options.size( ) ) {
		strcat( buf, XorStr( "none" ) );
	}

	bool bEnabled = true;
	if( strlen( buf ) == 0 ) {
		bEnabled = false;
		strcat( buf, XorStr( "none" ) );
	}

	if( buf[ strlen( buf ) - 1 ] == ' ' )
		buf[ strlen( buf ) - 2 ] = '\0';

	if( buf[ strlen( buf ) - 1 ] == ' ' )
		buf[ strlen( buf ) - 2 ] = '\0';

	if( bRemovingChars )
		strcat( buf, XorStr( "..." ) );

	Render::Engine::menu_regular.string(
		DrawPos.x,
		DrawPos.y - 2,
		( ( bEnabled || bActive ) ) ? Color( 150, 150, 150 ) : ( ( bHovered && ( ctx->FocusedID == 0 || ctx->FocusedID == GUI::Hash( name ) ) ) ? Color( 125, 125, 125 ) : Color( 75, 75, 75 ) ), GUI::SplitStr( name, '#' )[ 0 ].data( ) );

	DrawPos.y += 12;

	std::string str = buf;
	std::transform( str.begin( ), str.end( ), str.begin( ), ::tolower );
	Render::Engine::menu_small.string( DrawPos.x + DrawSize.x / 2, DrawPos.y + 2,
									   ( /*bEnabled ||*/ bActive ) ? Color( 150, 150, 150 ) : ( ( bHovered && ( ctx->FocusedID == 0 || ctx->FocusedID == GUI::Hash( name ) ) ) ? Color( 125, 125, 125 ) : Color( 75, 75, 75 ) ), str, Render::Engine::ALIGN_CENTER );

	auto DrawArrow = [ & ] ( Vector2D vecPos, Color clr ) {
		static int count = 0;
		for( int i = 7; i > 0; i -= 2 ) {
			count++;
			Render::Engine::Rect( vecPos + Vector2D( count, count ), Vector2D( i, 1 ), clr );
		}

		count = 0;
	};

	Render::Engine::Gradient( DrawPos + 2, Vector2D( 20, DrawSize.y - 4 ), Color::Palette_t::FormColorLight( ), Color::Palette_t::FormColorLight( ).OverrideAlpha( 0 ), true );
	Render::Engine::Gradient( DrawPos + Vector2D( DrawSize.x - 2 - 35, 2 ), Vector2D( 35, DrawSize.y - 4 ), Color::Palette_t::FormColorLight( ).OverrideAlpha( 0 ), Color::Palette_t::FormColorLight( ), true );


	DrawArrow( DrawPos + Vector2D( DrawSize.x - 14, DrawSize.y - 13 ), Color( 75, 75, 75 ) );

	if( ctx->FocusedID == 0 ) {
		if( bHovered && InputHelper::Pressed( VK_LBUTTON ) )
			ctx->FocusedID = GUI::Hash( name );
	}
	else if( ctx->FocusedID == GUI::Hash( name ) ) {
		if( bHovered ) {
			ctx->MultiDropdownInfo.Elements = options;
			ctx->MultiDropdownInfo.MaxItems = max_items;
			ctx->MultiDropdownInfo.Size = DrawSize.x;
			ctx->MultiDropdownInfo.Pos = DrawPos + Vector2D( 0, DrawSize.y + 1 );
			ctx->MultiDropdownInfo.HashedID = GUI::Hash( name );
		}
	}

	if( !ctx->enabled )
		g_pSurface->DrawSetAlphaMultiplier( flBackupMultiplier );

	Render::Engine::Rect( DrawPos, DrawSize, /*bActive ? g_Vars.menu.ascent.ToRegularColor( ) :*/ Color::Black( ) );
	Render::Engine::Rect( DrawPos + 1, DrawSize - 2, /*bActive ? g_Vars.menu.ascent.ToRegularColor( ) :*/ Color::Palette_t::ElementOutlines( ) );

	GUI::PushCursorPos( CursorPos + Vector2D( 0, DrawSize.y + GUI::ObjectPadding( ) + 12 ) );
#if defined(LUA_SCRIPTING)
	if( ctx->setup ) {
		auto item = new Scripting::multidropdownItem;
		item->init( name );

		for( auto it : options ) {
			item->items->push_back( it );
		}

		std::string yep = std::string( ctx->CurrentTab ).append( XorStr( "." ) )/*.append( ctx->CurrentSubTab ).append( XorStr( "." ) )*/.append( ctx->CurrentGroup ).append( XorStr( "." ) ).append( GUI::SplitStr( name, '#' )[ 0 ].data( ) ).append( XorStr( "." ) ).append( ctx->CurrentWeaponGroup );
		std::transform( yep.begin( ), yep.end( ), yep.begin( ), ::tolower );

		Scripting::gui_items.emplace_back( hash_32_fnv1a( yep.c_str( ) ), item );
	}
#endif

	ctx->szLastElementName = GUI::SplitStr( name, '#' )[ 0 ];
	if( ctx->FocusedID == 0 && bHovered ) {
		GUI::CopyReference( );
	}

	return true;
		}
