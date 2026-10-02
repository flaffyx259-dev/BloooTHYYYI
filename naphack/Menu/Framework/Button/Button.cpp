#include "../gui.h"
#include <algorithm>
#include "../../../pandora.hpp"

void GUI::Controls::Button( std::string name, std::function< void( ) > callback, bool use_unique_id ) {
	Vector2D CursorPos = PopCursorPos( );
	Vector2D DrawPos = ctx->pos + CursorPos;
	Vector2D DrawSize = Vector2D( ctx->ParentSize.x - 25, 19 );

	const float flBackupMultiplier = g_pSurface->DrawGetAlphaMultiplier( );

	std::transform( name.begin( ), name.end( ), name.begin( ), ::tolower );

	if( !ctx->enabled )
		g_pSurface->DrawSetAlphaMultiplier( flBackupMultiplier * 0.3f );

	bool bHovered = ctx->bActiveWindow && ctx->enabled && g_InputSystem.IsInBox( DrawPos, DrawSize ) && g_InputSystem.IsInBox( ctx->ParentPos, ctx->ParentSize );
	bool bActive = ctx->FocusedID == GUI::Hash( name );

	if( bActive || ( bHovered && ( ctx->FocusedID == 0 || ctx->FocusedID == GUI::Hash( name ) ) ) ) {
		Render::Engine::menu_regular.string(
			DrawPos.x + DrawSize.x / 2,
			DrawPos.y + ( DrawSize.y / 2 ) - ( Render::Engine::menu_regular.size( name ).m_height / 2 ),
			bActive ? Color( 220, 220, 220 ) : Color( 150, 150, 150 ), GUI::SplitStr( name, '#' )[ 0 ].data( ), Render::Engine::ALIGN_CENTER );
	}
	else {
		Render::Engine::menu_regular.string(
			DrawPos.x + DrawSize.x / 2,
			DrawPos.y + ( DrawSize.y / 2 ) - ( Render::Engine::menu_regular.size( name ).m_height / 2 ),
			Color( 75, 75, 75 ), GUI::SplitStr( name, '#' )[ 0 ].data( ), Render::Engine::ALIGN_CENTER );
	}

	if( ctx->FocusedID == 0 ) {
		if( bHovered && InputHelper::Pressed( VK_LBUTTON ) ) {
			ctx->FocusedID = GUI::Hash( name );
		}
	}
	else if( ctx->FocusedID == GUI::Hash( name ) ) {
		if( InputHelper::Released( VK_LBUTTON ) ) {
			if( bHovered )
				callback( );

			ctx->FocusedID = 0;
		}
	}

	if( !ctx->enabled )
		g_pSurface->DrawSetAlphaMultiplier( flBackupMultiplier );

	Render::Engine::Rect( DrawPos, DrawSize, Color::Black( ) );
	Render::Engine::Rect( DrawPos + 1, DrawSize - 2, Color::Palette_t::ElementOutlines( ) );

	GUI::PushCursorPos( CursorPos + Vector2D( 0, DrawSize.y + GUI::ObjectPadding( ) ) );

	ctx->szLastElementName = GUI::SplitStr( name, '#' )[ 0 ];
	if( ctx->FocusedID == 0 && bHovered ) {
		GUI::CopyReference( );
	}
}
