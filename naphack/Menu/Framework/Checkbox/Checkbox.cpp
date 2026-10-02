#include "../gui.h"
#include "../../../pandora.hpp"
#include "../../../Features/Scripting/Scripting.hpp"

bool GUI::Controls::Checkbox( std::string name, bool *value, std::pair<std::string, std::string> tt ) {
	Vector2D CursorPos = PopCursorPos( );
	Vector2D DrawPos = ctx->pos + CursorPos;
	Vector2D DrawSize = Vector2D( 12, 12 );

	bool bHovered = ctx->enabled && g_InputSystem.IsInBox( DrawPos, DrawSize + Vector2D( 12 + Render::Engine::menu_regular.size( GUI::SplitStr( name, '#' )[ 0 ].data( ) ).m_width ) )
		&& g_InputSystem.IsInBox( ctx->ParentPos, ctx->ParentSize );

	std::transform( name.begin( ), name.end( ), name.begin( ), ::tolower );

	if( !bHovered )
		bHovered = ctx->bActiveWindow && ctx->enabled && g_InputSystem.IsInBox( DrawPos, DrawSize ) && g_InputSystem.IsInBox( ctx->ParentPos, ctx->ParentSize );

	const float flBackupMultiplier = g_pSurface->DrawGetAlphaMultiplier( );

	if( !ctx->enabled )
		g_pSurface->DrawSetAlphaMultiplier( flBackupMultiplier * 0.3f );

	if( *value ) {
		Render::Engine::Gradient( DrawPos + 2, DrawSize - 4, g_Vars.menu.ascent.ToRegularColor( ), g_Vars.menu.ascent.ToRegularColor( ) * 0.8f, false );
		Render::Engine::Rect( DrawPos + 1, DrawSize - 2, g_Vars.menu.ascent.ToRegularColor( ) );
	}

	Render::Engine::menu_regular.string(
		DrawPos.x + 19,
		DrawPos.y - 1,
		( *value ) ? Color( 150, 150, 150 ) : ( ( bHovered && ( ctx->FocusedID == 0 || ctx->FocusedID == GUI::Hash( name ) ) ) ? Color( 125, 125, 125 ) : Color( 75, 75, 75 ) ),
		GUI::SplitStr( name, '#' )[ 0 ].data( ) );

	if( ctx->FocusedID == 0 ) {
		if( bHovered && InputHelper::Pressed( VK_LBUTTON ) ) {
			ctx->FocusedID = GUI::Hash( name );
		}
	}
	else if( ctx->FocusedID == GUI::Hash( name ) ) {
		if( InputHelper::Released( VK_LBUTTON ) ) {
			if( bHovered ) {
				if( *value == true )
					*value = false;
				else {
					*value = true;
				}

				ctx->FocusedID = 0;
			}

			ctx->FocusedID = 0;
		}
	}

	if( !ctx->enabled )
		g_pSurface->DrawSetAlphaMultiplier( flBackupMultiplier );

	Render::Engine::Rect( DrawPos, DrawSize, Color::Black( ) );
	if( !*value )
		Render::Engine::Rect( DrawPos + 1, DrawSize - 2, Color::Palette_t::ElementOutlines( ) );

#if defined(LUA_SCRIPTING)
	if( ctx->setup ) {
		auto item = new Scripting::checkboxItem;
		item->value = value;

		std::string yep = std::string( ctx->CurrentTab ).append( XorStr( "." ) )/*.append( ctx->CurrentSubTab ).append( XorStr( "." ) )*/.append( ctx->CurrentGroup ).append( XorStr( "." ) ).append( GUI::SplitStr( name, '#' )[ 0 ].data( ) ).append( XorStr( "." ) ).append( ctx->CurrentWeaponGroup );
		std::transform( yep.begin( ), yep.end( ), yep.begin( ), ::tolower );

		Scripting::gui_items.emplace_back( hash_32_fnv1a( yep.data( ) ), item );
	}
#endif

	ctx->szLastElementName = GUI::SplitStr( name, '#' )[ 0 ];
	if( ctx->FocusedID == 0 && bHovered ) {
		GUI::CopyReference( );
	}

	GUI::PushCursorPos( CursorPos + Vector2D( 0, DrawSize.y + GUI::ObjectPadding( ) ) );
	return *value || ctx->setup;
}
