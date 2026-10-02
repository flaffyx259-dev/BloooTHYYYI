#include "../GUI.h"
#include "../../../pandora.hpp"
#include "../../../Features/Visuals/EventLogger.hpp"
#include "../../../Features/Scripting/Scripting.hpp"

void GUI::Controls::ColorPicker( std::string name, Color_f *color, bool bRenderAlpha, bool bSeparateElement, int nOffset ) {
	std::transform( name.begin( ), name.end( ), name.begin( ), ::tolower );
	std::string szStringBeforeID = GUI::SplitStr( name, '#' )[ 0 ];

	Vector2D CursorPos = bSeparateElement ? PopCursorPos( ) : GetLastCursorPos( );
	Vector2D DrawPos = ctx->pos + CursorPos;
	Vector2D DrawSize = Vector2D( 12, 12 );

	DrawPos.x += ctx->ParentSize.x - 25 - DrawSize.x;

	if( !bSeparateElement ) {
		DrawPos.x -= 12 * ( nOffset )+( 3 * nOffset );
	}

	const float flBackupMultiplier = g_pSurface->DrawGetAlphaMultiplier( );

	if( !ctx->enabled )
		g_pSurface->DrawSetAlphaMultiplier( flBackupMultiplier * 0.3f );

	Render::Engine::Gradient( DrawPos + 1, DrawSize - 2, color->ToRegularColor( ).OverrideAlpha( 255 ), color->ToRegularColor( ).OverrideAlpha( 255 ) * 0.9f, false );
	Render::Engine::Rect( DrawPos, DrawSize, color->ToRegularColor( ).OverrideAlpha( 255 ) );

	bool bHovered = ctx->bActiveWindow && ctx->enabled && g_InputSystem.IsInBox( DrawPos, DrawSize ) && g_InputSystem.IsInBox( ctx->ParentPos, ctx->ParentSize );

	if( !szStringBeforeID.empty( ) && bSeparateElement ) {
		DrawPos.x -= ctx->ParentSize.x - 25 - DrawSize.x;
		if( g_InputSystem.IsInBox( DrawPos, Vector2D( Render::Engine::menu_regular.size( szStringBeforeID ).m_width, Render::Engine::menu_regular.size( szStringBeforeID ).m_height ) ) ) {
			bHovered = ctx->bActiveWindow && ctx->enabled && ctx->FocusedID == 0 || ctx->FocusedID == GUI::Hash( name );
		}

		Render::Engine::menu_regular.string( DrawPos.x, DrawPos.y - 1,
											 ( ( ctx->FocusedID == GUI::Hash( name ) || ctx->FocusedID == GUI::Hash( name ) + 69 ) && ctx->enabled ) ? Color( 150, 150, 150 ) :
											 bHovered ? Color( 100, 100, 100 ) : Color( 75, 75, 75 ),
											 szStringBeforeID.data( ) );

		DrawPos.x += ctx->ParentSize.x - 25 - DrawSize.x;
	}

	static Vector2D vecLastCursorPos;
	if( ctx->FocusedID == 0 && ( ctx != m_vecWindows[ 0 ].m_pContext || m_vecWindows.size( ) == 1 ) ) {
		vecLastCursorPos = g_InputSystem.GetMousePosition( );

		if( bHovered && InputHelper::Pressed( VK_LBUTTON ) ) {
			ctx->FocusedID = GUI::Hash( name );
			ctx->ColorPickerInfo.bRightClicked = false;
		}

		if( bHovered && InputHelper::Pressed( VK_RBUTTON ) ) {
			ctx->FocusedID = GUI::Hash( name );
			ctx->ColorPickerInfo.bRightClicked = true;
		}
	}
	else if( ctx->FocusedID == GUI::Hash( name ) ) {
		ctx->ColorPickerInfo.HashedID = GUI::Hash( name );
		ctx->ColorPickerInfo.vecBasePos = DrawPos;
		ctx->ColorPickerInfo.vecPos = vecLastCursorPos;
		ctx->ColorPickerInfo.fColor = color;
		ctx->ColorPickerInfo.bRenderAlpha = bRenderAlpha;
		ctx->ColorPickerInfo.bFirstColorInit = true;
	}

	if( !ctx->enabled )
		g_pSurface->DrawSetAlphaMultiplier( flBackupMultiplier );

	if( bSeparateElement )
		GUI::PushCursorPos( CursorPos + Vector2D( 0, Render::Engine::menu_regular.size( GUI::SplitStr( name, '#' )[ 0 ].data( ) ).m_height + GUI::ObjectPadding( ) - 1 ) );

#if defined(LUA_SCRIPTING)
	if( ctx->setup ) {
		auto item = new Scripting::colorpickerItem;
		item->color = color;

		std::string yep = std::string( ctx->CurrentTab ).append( XorStr( "." ) )/*.append( ctx->CurrentSubTab ).append( XorStr( "." ) )*/.append( ctx->CurrentGroup ).append( XorStr( "." ) ).append( GUI::SplitStr( name, '#' )[ 0 ].data( ) ).append( XorStr( "." ) ).append( ctx->CurrentWeaponGroup );
		std::transform( yep.begin( ), yep.end( ), yep.begin( ), ::tolower );

		Scripting::gui_items.emplace_back( hash_32_fnv1a( yep.data( ) ), item );
	}
#endif

	ctx->szLastElementName = GUI::SplitStr( name, '#' )[ 0 ];
	if( ctx->FocusedID == 0 && bHovered ) {
		GUI::CopyReference( );
	}
}