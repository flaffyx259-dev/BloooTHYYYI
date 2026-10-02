#include <algorithm>
#include "../gui.h"
#include "../../../pandora.hpp"
#include "../../../Features/Scripting/Scripting.hpp"

bool GUI::Controls::Slider( std::string name, float *var_name, float min, float max, const std::string &display, float increment, std::pair<std::string, std::string> tt ) {
	Vector2D CursorPos = GUI::SplitStr( name, '#' )[ 0 ].size( ) ? PopCursorPos( ) : GetLastCursorPos( );
	Vector2D DrawPos = ctx->pos + CursorPos;
	Vector2D DrawSize = Vector2D( ctx->ParentSize.x - 25, 10 );

	const float flBackupMultiplier = g_pSurface->DrawGetAlphaMultiplier( );

	std::transform( name.begin( ), name.end( ), name.begin( ), ::tolower );

	if( !ctx->enabled )
		g_pSurface->DrawSetAlphaMultiplier( flBackupMultiplier * 0.3f );

	bool bEmpty = GUI::SplitStr( name, '#' )[ 0 ].empty( );
	bool bHovered = ctx->bActiveWindow && ctx->enabled && g_InputSystem.IsInBox( DrawPos + Vector2D( 0, 14 ), DrawSize )
		&& g_InputSystem.IsInBox( ctx->ParentPos, ctx->ParentSize ) && ( ctx->FocusedID == 0 && ctx->FocusedID != GUI::Hash( name ) );


	if( GUI::SplitStr( name, '#' )[ 0 ].size( ) ) {
		Render::Engine::menu_regular.string( DrawPos.x, DrawPos.y - 1, ( ctx->FocusedID == GUI::Hash( name ) || *var_name != min ) ? Color( 150, 150, 150 ) : ( bHovered ? Color( 125, 125, 125 ) : Color( 75, 75, 75 ) ),
											 GUI::SplitStr( name, '#' )[ 0 ].data( ) );
	}

	DrawPos.y += 14;

	if( *var_name < min )
		*var_name = min;
	else if( *var_name > max )
		*var_name = max;

	if( ctx->bActiveWindow && g_InputSystem.IsInBox( ctx->ParentPos, ctx->ParentSize ) && g_InputSystem.IsInBox( DrawPos, DrawSize ) ) {
		// minus
		if( ctx->FocusedID == 0 && InputHelper::Down( VK_CONTROL ) ) {
			if( g_InputSystem.GetScrollMouse( ) < 0 ) {
				bool is_whole_number = ( *var_name ) == std::ceilf( ( *var_name ) );
				// goal is to take away 1, but if the number is on like 50.6 we take away .6 first
				if( !is_whole_number && ( increment == std::ceilf( increment ) ) ) {
					float delta = ( *var_name ) - std::ceil( ( *var_name ) );
					( *var_name ) = std::clamp( ( *var_name ) - ( delta + increment ), min, max );
				}
				else {
					( *var_name ) = std::clamp( ( *var_name ) - increment, min, max );
				}
			}

			if( g_InputSystem.GetScrollMouse( ) > 0 ) {
				bool is_whole_number = ( *var_name ) == std::ceilf( ( *var_name ) );
				// goal is to add 1, but if the number is on like 50.6 we add .4 first
				if( !is_whole_number && ( increment == std::ceilf( increment ) ) ) {
					float delta = fabs( ( *var_name ) - std::ceil( ( *var_name ) ) );
					( *var_name ) = std::clamp( ( *var_name ) + ( delta ), min, max );
				}
				else {
					( *var_name ) = std::clamp( ( *var_name ) + increment, min, max );
				}
			}
		}
	}

	float slider_progress = GUI::MapNumber( *var_name, min, max, 0.0f, 1.0f );
	int slider_fill_width = std::clamp<int>( static_cast< int32_t >( static_cast< float >( DrawSize.x ) * slider_progress ), 2, DrawSize.x );

	char buffer[ 128 ] = { };
	char display_buffer[ 128 ] = { };
	sprintf_s( buffer, display.data( ), *var_name );
	sprintf_s( display_buffer, XorStr( " (%s)" ), buffer );

	Render::Engine::Rect( DrawPos + 1, DrawSize - 2, Color::Palette_t::ElementOutlines( ) );

	Render::Engine::Gradient( DrawPos + 2, Vector2D( slider_fill_width, DrawSize.y ) - 4, g_Vars.menu.ascent.ToRegularColor( ), g_Vars.menu.ascent.ToRegularColor( ) * 0.8f, false );

	if( slider_progress > 0.f )
		Render::Engine::Rect( DrawPos + 1, Vector2D( slider_fill_width, DrawSize.y ) - 2, g_Vars.menu.ascent.ToRegularColor( ) );

	if( ctx->FocusedID == 0 ) {
		if( InputHelper::Pressed( VK_LBUTTON ) && bHovered )
			ctx->FocusedID = GUI::Hash( name );
	}
	else if( ctx->FocusedID == GUI::Hash( name ) ) {
		if( InputHelper::Down( VK_LBUTTON ) ) {
			float offset = std::clamp<float>( Vector2D( g_InputSystem.GetMousePosition( ) - DrawPos ).x, 0, DrawSize.x );

			*var_name = std::clamp( GUI::MapNumber( offset, 0, DrawSize.x, min, max ), min, max );
		}
		else {
			// Fuck
			ctx->FocusedID = 0;
		}

	}


	Render::Engine::menu_small.string( DrawPos.x + Render::Engine::menu_small.size( GUI::SplitStr( name, '#' )[ 0 ] ).m_width, DrawPos.y - 14 - 1,
									   ( ctx->FocusedID == GUI::Hash( name ) || *var_name != min ) ? Color( 150, 150, 150 ) : ( bHovered ? Color( 125, 125, 125 ) : Color( 75, 75, 75 ) ), display_buffer,
									   Render::Engine::ALIGN_LEFT );

	if( !ctx->enabled )
		g_pSurface->DrawSetAlphaMultiplier( flBackupMultiplier );

	Render::Engine::Rect( DrawPos, DrawSize, Color::Black( ) );

	//int nAccountForSpace = 0;
	//if( ( DrawPos.x + slider_fill_width + ( Render::Engine::menu_small.size( buffer ).m_width / 2 ) ) > DrawPos.x + DrawSize.x ) {
	//	nAccountForSpace = fabsf( ( DrawPos.x + slider_fill_width + ( Render::Engine::menu_small.size( buffer ).m_width / 2 ) ) - ( DrawPos.x + DrawSize.x ) );
	//}

	//if( DrawPos.x + slider_fill_width - ( Render::Engine::menu_small.size( buffer ).m_width / 2 ) < DrawPos.x )
	//	nAccountForSpace = -( fabsf( ( DrawPos.x + slider_fill_width - ( Render::Engine::menu_small.size( buffer ).m_width / 2 ) ) - DrawPos.x ) );

	//Render::Engine::menu_small.string( DrawPos.x /*+ slider_fill_width - nAccountForSpace*/ + DrawSize.x, DrawPos.y - 14 - 1,
	//								   ( ctx->FocusedID == GUI::Hash( name ) || *var_name != min ) ? Color( 150, 150, 150 ) : ( bHovered ? Color( 125, 125, 125 ) : Color( 75, 75, 75 ) ), buffer,
	//								   Render::Engine::ALIGN_RIGHT );

#if defined(LUA_SCRIPTING)
	if( ctx->setup ) {
		auto item = new Scripting::sliderFloatItem;

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

	PushCursorPos( CursorPos + Vector2D( 0, DrawSize.y + ( 14 ) + GUI::ObjectPadding( ) ) );
	return ctx->FocusedID == GUI::Hash( name );
}

bool GUI::Controls::Slider( std::string name, int *var_name, int min, int max, const std::string &display, int increment, std::pair<std::string, std::string> tt ) {
	Vector2D CursorPos = GUI::SplitStr( name, '#' )[ 0 ].size( ) ? PopCursorPos( ) : GetLastCursorPos( );
	Vector2D DrawPos = ctx->pos + CursorPos;
	Vector2D DrawSize = Vector2D( ctx->ParentSize.x - 25, 10 );

	std::transform( name.begin( ), name.end( ), name.begin( ), ::tolower );

	bool bEmpty = GUI::SplitStr( name, '#' )[ 0 ].empty( );
	bool bHovered = ctx->bActiveWindow && ctx->enabled && g_InputSystem.IsInBox( DrawPos + Vector2D( 0, 14 ), DrawSize ) && g_InputSystem.IsInBox( ctx->ParentPos, ctx->ParentSize );

	const float flBackupMultiplier = g_pSurface->DrawGetAlphaMultiplier( );

	if( !ctx->enabled )
		g_pSurface->DrawSetAlphaMultiplier( flBackupMultiplier * 0.3f );

	if( !bEmpty ) {
		Render::Engine::menu_regular.string( DrawPos.x, DrawPos.y - 1, ( ctx->FocusedID == GUI::Hash( name ) || *var_name != min ) ? Color( 150, 150, 150 ) : ( bHovered ? Color( 125, 125, 125 ) : Color( 75, 75, 75 ) ),
											 GUI::SplitStr( name, '#' )[ 0 ].data( ) );

	}

	DrawPos.y += 14;

	if( *var_name < min )
		*var_name = min;
	else if( *var_name > max )
		*var_name = max;

	if( ctx->bActiveWindow && g_InputSystem.IsInBox( DrawPos, DrawSize ) && g_InputSystem.IsInBox( ctx->ParentPos, ctx->ParentSize ) ) {
		// minus
		if( ctx->FocusedID == 0 && InputHelper::Down( VK_CONTROL ) ) {
			if( g_InputSystem.GetScrollMouse( ) < 0 ) {
				bool is_whole_number = ( *var_name ) == std::ceilf( ( *var_name ) );
				// goal is to take away 1, but if the number is on like 50.6 we take away .6 first
				if( !is_whole_number ) {
					int delta = ( *var_name ) - std::ceil( ( *var_name ) );
					( *var_name ) = std::clamp( ( *var_name ) - ( delta + increment ), min, max );
				}
				else {
					( *var_name ) = std::clamp( ( *var_name ) - increment, min, max );
				}
			}

			if( g_InputSystem.GetScrollMouse( ) > 0 ) {
				bool is_whole_number = ( *var_name ) == std::ceilf( ( *var_name ) );
				// goal is to add 1, but if the number is on like 50.6 we add .4 first
				if( !is_whole_number ) {
					int delta = fabs( ( *var_name ) - std::ceil( ( *var_name ) ) );
					( *var_name ) = std::clamp( ( *var_name ) + ( delta ), min, max );
				}
				else {
					( *var_name ) = std::clamp( ( *var_name ) + increment, min, max );
				}
			}
		}
	}

	float slider_progress = GUI::MapNumber( static_cast< float >( *var_name ), static_cast< float >( min ), static_cast< float >( max ), 0.0f, 1.0f );
	int slider_fill_width = std::clamp<int>( static_cast< int >( static_cast< float >( DrawSize.x ) * slider_progress ), 2, DrawSize.x );

	char buffer[ 128 ] = { };
	char display_buffer[ 128 ] = { };
	sprintf_s( buffer, display.data( ), *var_name );
	sprintf_s( display_buffer, XorStr( " (%s)" ), buffer );

	Render::Engine::Rect( DrawPos + 1, DrawSize - 2, Color::Palette_t::ElementOutlines( ) );

	Render::Engine::Gradient( DrawPos + 2, Vector2D( slider_fill_width, DrawSize.y ) - 4, g_Vars.menu.ascent.ToRegularColor( ), g_Vars.menu.ascent.ToRegularColor( ) * 0.8f, false );

	if( slider_progress > 0.f )
		Render::Engine::Rect( DrawPos + 1, Vector2D( slider_fill_width, DrawSize.y ) - 2, g_Vars.menu.ascent.ToRegularColor( ) );

	if( ctx->FocusedID == 0 ) {
		if( InputHelper::Pressed( VK_LBUTTON ) && bHovered )
			ctx->FocusedID = GUI::Hash( name );
	}
	else if( ctx->FocusedID == GUI::Hash( name ) ) {
		if( InputHelper::Down( VK_LBUTTON ) ) {
			float offset = std::clamp<float>( Vector2D( g_InputSystem.GetMousePosition( ) - DrawPos ).x, 0, DrawSize.x );

			*var_name = std::ceil( std::clamp( static_cast< int >( GUI::MapNumber( offset, 0, DrawSize.x, min, max ) ), min, max ) );
		}
		else {
			// Fuck
			ctx->FocusedID = 0;
		}
	}

	Render::Engine::menu_small.string( DrawPos.x + Render::Engine::menu_small.size( GUI::SplitStr( name, '#' )[ 0 ] ).m_width + 1, DrawPos.y - 14 - 1,
									   ( ctx->FocusedID == GUI::Hash( name ) || *var_name != min ) ? Color( 150, 150, 150 ) : ( bHovered ? Color( 125, 125, 125 ) : Color( 75, 75, 75 ) ), display_buffer,
									   Render::Engine::ALIGN_LEFT );

	if( !ctx->enabled )
		g_pSurface->DrawSetAlphaMultiplier( flBackupMultiplier );

	Render::Engine::Rect( DrawPos, DrawSize, Color::Black( ) );

	//int nAccountForSpace = 0;
	//if( ( DrawPos.x + slider_fill_width + ( Render::Engine::menu_small.size( display_buffer ).m_width / 2 ) ) > DrawPos.x + DrawSize.x ) {
	//	nAccountForSpace = fabsf( ( DrawPos.x + slider_fill_width + ( Render::Engine::menu_small.size( display_buffer ).m_width / 2 ) ) - ( DrawPos.x + DrawSize.x ) );
	//}

	//if( DrawPos.x + slider_fill_width - ( Render::Engine::menu_small.size( display_buffer ).m_width / 2 ) < DrawPos.x )
	//	nAccountForSpace = -( fabsf( ( DrawPos.x + slider_fill_width - ( Render::Engine::menu_small.size( display_buffer ).m_width / 2 ) ) - DrawPos.x ) );

	//Render::Engine::menu_small.string( DrawPos.x /*+ slider_fill_width - nAccountForSpace*/ + DrawSize.x, DrawPos.y - 14 - 1,
	//								   ( ctx->FocusedID == GUI::Hash( name ) || *var_name != min ) ? Color( 150, 150, 150 ) : ( bHovered ? Color( 125, 125, 125 ) : Color( 75, 75, 75 ) ), buffer,
	//								   Render::Engine::ALIGN_RIGHT );


#if defined(LUA_SCRIPTING)
	if( ctx->setup ) {
		auto item = new Scripting::sliderItem;
		item->value = var_name;

		std::string yep = std::string( ctx->CurrentTab ).append( XorStr( "." ) )/*.append( ctx->CurrentSubTab ).append( XorStr( "." ) )*/.append( ctx->CurrentGroup ).append( XorStr( "." ) ).append( GUI::SplitStr( name, '#' )[ 0 ].data( ) ).append( XorStr( "." ) ).append( ctx->CurrentWeaponGroup );
		std::transform( yep.begin( ), yep.end( ), yep.begin( ), ::tolower );

		Scripting::gui_items.emplace_back( hash_32_fnv1a( yep.c_str( ) ), item );
	}
#endif


	PushCursorPos( CursorPos + Vector2D( 0, DrawSize.y + ( 14 ) + GUI::ObjectPadding( ) ) );
	return ctx->FocusedID == GUI::Hash( name );
}