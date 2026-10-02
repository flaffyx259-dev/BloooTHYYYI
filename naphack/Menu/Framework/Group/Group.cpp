#include "../gui.h"
#include <algorithm>
#include "../../../pandora.hpp"
#include "../../../Utils/extern/XorStr.hpp"

Vector2D _GroupPos;
Vector2D _GroupSize;
Vector2D _CursorPos;
int id;

D3DVIEWPORT9 vp;

std::map< int, int > Scrolling;
std::map< int, float > AnimMult;
std::map< int, float > ActiveAnim;
std::map< int, bool > bHoveringScroll;

void GUI::Group::BeginGroup( const std::string &name, const Vector2D &size ) {
	size_t id = GUI::Hash( name + ( ctx->Tabs.empty( ) ? XorStr( "-1." ) : ( ctx->Tabs[ ctx->ActiveTab ].second + "." + ( ctx->SubTabs.at( ctx->ActiveTab ).empty( ) ? XorStr( "-1" ) : ctx->SubTabs.at( ctx->ActiveTab )[ ctx->ActiveSubTab.at( ctx->ActiveTab ) ] ) ) ) ) + ( ctx->ActiveTab == 0 ? g_Vars.globals.m_iCurrentRageGroup : 0 );
	::id = id;

	::GUI::ctx->CurrentGroup = name;

	if( Scrolling.find( id ) == Scrolling.end( ) ) {
		Scrolling.insert( { id, 0 } );
		AnimMult.insert( { id, 0.f } );
		ActiveAnim.insert( { id, 0.f } );
		bHoveringScroll.insert( { id, false } );
	}

	Vector2D CursorPos = PopCursorPos( ) + ctx->NextGroupAdj;
	Vector2D GroupSize;

	float WidthPercent = ( float )size.x / 100.f;
	float HeightPercent = ( float )size.y / 100.f;

	// assume we have subtabs in this tab
	int WidthAvailable = ctx->size.x - 56;
	int HeightAvailable = ctx->size.y - 105;

	if( CursorPos.x == 20 && WidthPercent == 1.0f ) {
		WidthAvailable += 15;
	}

	GroupSize.x = int( WidthAvailable * WidthPercent );

	if( CursorPos.y == 35 && HeightPercent == 1.0f ) {
		HeightAvailable += 15;
	}

	GroupSize.y = int( HeightAvailable * HeightPercent );

	if( ( CursorPos.y - 15 + GroupSize.y ) > ctx->size.y - 15 ) {
		CursorPos.x += GroupSize.x + 15;
		CursorPos.y = 35;

		PushCursorPos( CursorPos );
		Group::BeginGroup( name, size );
	}
	else {
		Vector2D draw_pos = ctx->pos + CursorPos;

		_GroupPos = Vector2D( ( int )draw_pos.x, ( int )draw_pos.y );
		_GroupSize = Vector2D( ( int )GroupSize.x, ( int )GroupSize.y );

		Vector2D FinalPos( CursorPos + Vector2D( 15, 35 + Scrolling[ id ] ) );
		FinalPos = Vector2D( static_cast< int >( FinalPos.x ), static_cast< int >( FinalPos.y ) );

		Render::Engine::RectFilled( _GroupPos + 2, _GroupSize - 4, Color::Palette_t::FormColorLight( ) );
		Render::Engine::Rect( _GroupPos, _GroupSize, Color::Black( ) );
		Render::Engine::Rect( _GroupPos + 1, _GroupSize - 2, Color::Palette_t::ElementOutlines( ) );

		if( GUI::SplitStr( name, '#' )[ 0 ].size( ) ) {
			auto szText = GUI::SplitStr( name, '#' )[ 0 ];
			std::transform( szText.begin( ), szText.end( ), szText.begin( ), ::tolower );

			Render::Engine::menu_regular.string( _GroupPos.x + ( _GroupSize.x / 2 ), _GroupPos.y + 5, Color( 220, 220, 220 ), szText, Render::Engine::ALIGN_CENTER );
		}
		else {
			FinalPos = Vector2D( CursorPos );
		}

		FinalPos = Vector2D( static_cast< int >( FinalPos.x ), static_cast< int >( FinalPos.y ) );

		PushCursorPos( FinalPos );

		if( GUI::SplitStr( name, '#' )[ 0 ].size( ) )
			Render::Engine::SetClip( draw_pos + Vector2D( 0, 23 ), _GroupSize - Vector2D( 0, 25 ) );

		ctx->parent = XorStr( "root." ) + ( ctx->Tabs.empty( ) ? XorStr( "-1." ) : ( ctx->Tabs[ ctx->ActiveTab ].second + XorStr( "." ) + ( ctx->SubTabs.at( ctx->ActiveTab ).empty( ) ? XorStr( "-1" ) : ctx->SubTabs.at( ctx->ActiveTab )[ ctx->ActiveSubTab.at( ctx->ActiveTab ) ] ) + XorStr( "." ) ) ) + name;
		ctx->NextGroupPos = CursorPos + Vector2D( 0, ( int )GroupSize.y + 15 );

		ctx->ParentPos = draw_pos + Vector2D( 0, 24 );
		ctx->ParentSize = Vector2D( ( int )GroupSize.x, ( int )GroupSize.y - 24 );

		if( !GUI::SplitStr( name, '#' )[ 0 ].size( ) ) {
			ctx->ParentPos = draw_pos;
			ctx->ParentSize = Vector2D( ( int )GroupSize.x, ( int )GroupSize.y );
		}

		// if our cursor was in a groupbox and the scrollbar was expanded
		// and we open a context menu, we dont wanna pull back the scrollbar,
		// instead freeze it in place (regardless if our cursor is out of group bounds)
		// until we close the context menu
		if( !GUI::ContextMenuOpen( ) ) {
			AnimMult[ id ] = GUI::Approach( AnimMult[ id ], ( ctx->FocusedID == 0 && g_InputSystem.IsInBox( ctx->ParentPos, ctx->ParentSize ) ) || ctx->FocusedID == id ? 1.f : 0.f, g_pGlobalVars->frametime * 14.f );

			if( AnimMult[ id ] >= 0.9f )
				AnimMult[ id ] = 1.f;
		}
	}

	ctx->NextGroupAdj = Vector2D( 0, 0 );
}

void GUI::Group::EndGroup( ) {
	Vector2D v1 = PopCursorPos( );
	int MaxHeight = ( int )v1.y - ( ( int )_GroupPos.y - ( int )ctx->pos.y ) - Scrolling[ id ];
	int VisibleHeight = _GroupSize.y;
	MaxHeight = abs( MaxHeight );

	PushCursorPos( v1 );

	int ScrollBarHeight, ScrollBarPos;
	if( MaxHeight > VisibleHeight ) {
		ScrollBarHeight = ( ( float )_GroupSize.y / ( float )MaxHeight ) * _GroupSize.y;
		ScrollBarPos = std::min( std::max( ( -( float )Scrolling[ id ] / ( float )MaxHeight ) * ( float )_GroupSize.y, 2.f ), _GroupSize.y - ScrollBarHeight - 2.f );

		static int ScrollBarClickedOffset = 0;

		// dont scroll if there is a listbox that we're about to scroll in
		if( ctx->bActiveWindow && !ctx->hovered_listbox && !ctx->MultiDropdownInfo.DraggingScroll && !ctx->DropdownInfo.DraggingScroll ) {
		#if 1
			bHoveringScroll[ id ] = g_InputSystem.IsInBox( _GroupPos + Vector2D( ( _GroupSize.x - 8 ) - 4, ScrollBarPos + 22 ), Vector2D( 8, ScrollBarHeight - 22 ) );

			if( ctx->FocusedID == 0 && InputHelper::Down( VK_LBUTTON ) && bHoveringScroll[ id ] ) {
				ScrollBarClickedOffset = ( int )g_InputSystem.GetMousePosition( ).y - ( ( int )_GroupPos.y + ScrollBarPos );

				ctx->FocusedID = id;
			}
			else if( ctx->FocusedID == id ) {
				bool bWithinScrollableBounds = true;

				if( InputHelper::Down( VK_LBUTTON ) ) {
					if( ( g_InputSystem.GetMousePosition( ).y < ( _GroupPos.y + 22 ) && Scrolling[ id ] >= -5 ) ||
						( g_InputSystem.GetMousePosition( ).y > _GroupPos.y + _GroupSize.y && fabs( Scrolling[ id ] - ( -MaxHeight + ( int )_GroupSize.y ) ) <= 5 ) )
						bWithinScrollableBounds = false;

					if( bWithinScrollableBounds ) {
						int currentScrollBarPos = ( int )g_InputSystem.GetMousePosition( ).y - ( ( int )_GroupPos.y + ScrollBarClickedOffset );

						// Clamp the scrollbar position within the group's bounds
						ScrollBarPos = std::clamp<int>( currentScrollBarPos, -2, ( int )_GroupSize.y - ScrollBarHeight + 2 );

						// Calculate the new scroll value based on the scrollbar position
						Scrolling[ id ] = ( -ScrollBarPos / ( float )_GroupSize.y * MaxHeight ) - 1;
					}
				}
				else {

					ctx->FocusedID = 0;
				}
			}

		#endif
			if( ctx->FocusedID == 0 && !GUI::ContextMenuOpen( ) ) {
				if( ctx->bActiveWindow && g_InputSystem.IsInBox( _GroupPos, _GroupSize - Vector2D( 6, 0 ) ) && !InputHelper::Down( VK_CONTROL ) ) {
					if( ctx->FocusedID != id ) {
						Scrolling[ id ] += 20 * g_InputSystem.GetScrollMouse( );
					}
				}
			}

			Scrolling[ id ] = std::clamp<int>( Scrolling[ id ], -MaxHeight + ( int )_GroupSize.y, 0 );
		}
	}
	else {
		Scrolling[ id ] = 0;
	}

	PushCursorPos( ctx->NextGroupPos );
	ctx->NextGroupPos = Vector2D( 0, 0 );

	Render::Engine::ResetClip( );

	// can scroll
	if( MaxHeight > VisibleHeight ) {
		// top gradient
		if( Scrolling[ id ] < -5 )
			Render::Engine::Gradient( _GroupPos + Vector2D( 0, 21 ) + 2, Vector2D( _GroupSize.x, 30 ) - 4,
									  Color::Palette_t::FormColorLight( ), Color::Palette_t::FormColorLight( ).OverrideAlpha( 0 ), false );

		// bottom gradient
		if( fabs( Scrolling[ id ] - ( -MaxHeight + ( int )_GroupSize.y ) ) > 5 )
			Render::Engine::Gradient( _GroupPos + Vector2D( 0, _GroupSize.y - 30 ) + 2, Vector2D( _GroupSize.x, 30 ) - 4,
									  Color::Palette_t::FormColorLight( ).OverrideAlpha( 0 ), Color::Palette_t::FormColorLight( ), false );

		Render::Engine::RectFilled( _GroupPos + Vector2D( ( _GroupSize.x - 3 ) - 4, std::clamp( ScrollBarPos, 2,
																								( int )_GroupSize.y - ScrollBarHeight ) + 20 ), Vector2D( 3, ScrollBarHeight - 22 ),
									bHoveringScroll[ id ] || ctx->FocusedID == id ? Color( 36, 36, 36 ) : Color::Palette_t::ElementOutlines( ) );

	}

	ctx->enabled = true;
	_GroupPos = _GroupSize = Vector2D( 0, 0 );
}