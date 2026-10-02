#include "../gui.h"
#include <algorithm>
#include "../../../pandora.hpp"
#include "../../../Features/Scripting/Scripting.hpp"

std::unordered_map<size_t, float> uHoverAnimation;
bool GUI::Controls::Dropdown( std::string name, std::vector< std::string > options, int *var_name, int max_items, std::pair<std::string, std::string> tt ) {
	std::string szStrignBeforeID = GUI::SplitStr( name, '#' )[ 0 ].data( );

	Vector2D CursorPos = PopCursorPos( );
	Vector2D DrawPos = ctx->pos + CursorPos;
	DrawPos.y += 1;
	Vector2D DrawSize = Vector2D( ctx->ParentSize.x - 25, 19 );

	if( *var_name < 0 )
		*var_name = 0;
	else if( *var_name >= options.size( ) )
		*var_name = options.size( ) - 1;

	std::transform( name.begin( ), name.end( ), name.begin( ), ::tolower );

	const float flBackupMultiplier = g_pSurface->DrawGetAlphaMultiplier( );

	if( !ctx->enabled )
		g_pSurface->DrawSetAlphaMultiplier( flBackupMultiplier * 0.3f );

	bool bHovered = ctx->bActiveWindow && ctx->enabled && g_InputSystem.IsInBox( DrawPos + Vector2D( 0, szStrignBeforeID.empty( ) ? 0 : 12 ), DrawSize )
		&& g_InputSystem.IsInBox( ctx->ParentPos, ctx->ParentSize );

	bool bActive = ctx->enabled && ctx->DropdownInfo.HashedID == GUI::Hash( name );

	if( !szStrignBeforeID.empty( ) ) {
		Render::Engine::menu_regular.string(
			DrawPos.x,
			DrawPos.y - 2,
			( *var_name > 0 || bActive ) ? Color( 150, 150, 150 ) : ( ( bHovered && ( ctx->FocusedID == 0 || ctx->FocusedID == GUI::Hash( name ) ) ) ? Color( 125, 125, 125 ) : Color( 75, 75, 75 ) ), GUI::SplitStr( name, '#' )[ 0 ].data( ) );

		DrawPos.y += 12;
	}

	char buf[ 128 ] = { 0 };

	if( options.size( ) ) {
		strcat( buf, options.at( *var_name ).data( ) );
	}
	else {
		strcat( buf, XorStr( "none" ) );
	}

	auto max_text_width = DrawSize.x - 26;
	if( Render::Engine::menu_small.size( buf ).m_width > max_text_width ) {
		// remove last character until string width is okay
		if( max_text_width >= 1 ) {
			while( Render::Engine::menu_small.size( buf ).m_width > max_text_width || buf[ strlen( buf ) - 1 ] == ',' || buf[ strlen( buf ) - 1 ] == ' ' ) {
				buf[ strlen( buf ) - 1 ] = '\0';
			}
		}
	}

	if( buf[ strlen( buf ) - 1 ] == ' ' )
		buf[ strlen( buf ) - 2 ] = '\0';

	if( buf[ strlen( buf ) - 1 ] == ' ' )
		buf[ strlen( buf ) - 2 ] = '\0';


	std::string str = buf;
	std::transform( str.begin( ), str.end( ), str.begin( ), ::tolower );
	Render::Engine::menu_small.string( DrawPos.x + DrawSize.x / 2, DrawPos.y + 2,
									   ( /**var_name > 0 ||*/ bActive ) ? Color( 150, 150, 150 ) : ( ( bHovered && ( ctx->FocusedID == 0 || ctx->FocusedID == GUI::Hash( name ) ) ) ? Color( 125, 125, 125 ) : Color( 75, 75, 75 ) ), str, Render::Engine::ALIGN_CENTER );


	auto DrawArrow = [ & ] ( Vector2D vecPos, Color clr ) {
		static int count = 0;
		for( int i = 7; i > 0; i -= 2 ) {
			count++;
			Render::Engine::Rect( vecPos + Vector2D( count, count ), Vector2D( i, 1 ), clr );
		}

		count = 0;
	};

	Render::Engine::Gradient( DrawPos + 2, Vector2D( 8, DrawSize.y - 4 ), Color::Palette_t::FormColorLight( ), Color::Palette_t::FormColorLight( ).OverrideAlpha( 0 ), true );
	Render::Engine::Gradient( DrawPos + Vector2D(DrawSize.x - 2 - 8, 2), Vector2D( 8, DrawSize.y - 4 ), Color::Palette_t::FormColorLight( ).OverrideAlpha( 0 ), Color::Palette_t::FormColorLight( ), true );

	DrawArrow( DrawPos + Vector2D( DrawSize.x - 14, DrawSize.y - 13 ), Color( 75, 75, 75 ) );

	if( ctx->FocusedID == 0 ) {
		if( bHovered && InputHelper::Pressed( VK_LBUTTON ) ) {
			ctx->FocusedID = GUI::Hash( name );
		}
	}
	else if( ctx->FocusedID == GUI::Hash( name ) ) {
		if( InputHelper::Released( VK_LBUTTON ) ) {
			ctx->DropdownInfo.Elements = options;
			ctx->DropdownInfo.Option = var_name;
			ctx->DropdownInfo.Size = DrawSize.x;
			ctx->DropdownInfo.MaxItems = max_items;
			ctx->DropdownInfo.Pos = DrawPos + Vector2D( 0, DrawSize.y + 1 );
			ctx->DropdownInfo.HashedID = GUI::Hash( name );
		}
	}

	if( !ctx->enabled )
		g_pSurface->DrawSetAlphaMultiplier( flBackupMultiplier );

	Render::Engine::Rect( DrawPos, DrawSize, /*bActive ? g_Vars.menu.ascent.ToRegularColor( ) :*/ Color::Black( ) );
	Render::Engine::Rect( DrawPos + 1, DrawSize - 2, /*bActive ? g_Vars.menu.ascent.ToRegularColor( ) :*/ Color::Palette_t::ElementOutlines( ) );

#if defined(LUA_SCRIPTING)
	if( ctx->setup ) {
		auto item = new Scripting::dropdownItem;
		item->value = var_name;

		std::string yep = std::string( ctx->CurrentTab ).append( XorStr( "." ) )/*.append( ctx->CurrentSubTab ).append( XorStr( "." ) )*/.append( ctx->CurrentGroup ).append( XorStr( "." ) ).append( GUI::SplitStr( name, '#' )[ 0 ].data( ) ).append( XorStr( "." ) ).append( ctx->CurrentWeaponGroup );
		std::transform( yep.begin( ), yep.end( ), yep.begin( ), ::tolower );

		Scripting::gui_items.emplace_back( hash_32_fnv1a( yep.c_str( ) ), item );
	}
#endif

	ctx->szLastElementName = GUI::SplitStr( name, '#' )[ 0 ];
	if( ctx->FocusedID == 0 && bHovered ) {
		GUI::CopyReference( );
	}

	GUI::PushCursorPos( CursorPos + Vector2D( 0, DrawSize.y + GUI::ObjectPadding( ) + ( szStrignBeforeID.empty( ) ? 0 : 12 ) ) );
	return 69 + GUI::Hash( XorStr( "fuck you" ) );
}
