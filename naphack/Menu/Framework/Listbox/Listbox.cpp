#include "../GUI.h"
#include "../../../pandora.hpp"
#include "../../../Features/Scripting/Scripting.hpp"

#pragma warning(disable : 4018)

std::map< int, std::vector<std::string> > vecElements;
std::map< int, float > iSlider;
std::map< int, std::string > szText;
std::map<size_t, float> LastShiftHoldTime;

bool GUI::Controls::Listbox( std::string id, std::vector<std::string> elements, int *option, bool bSearchBar, int iSizeInElements ) {
	Vector2D CursorPos = GUI::PopCursorPos( );
	Vector2D DrawPos = ctx->pos + CursorPos;

	std::map< size_t, std::vector<std::string> > vecFoundElements;
	std::map< int, int > iPos;

	if( iSlider.find( GUI::Hash( id ) ) == iSlider.end( ) ) {
		iSlider.insert( { GUI::Hash( id ), 0.f } );
	}

	if( vecElements.find( GUI::Hash( id ) ) == vecElements.end( ) ) {
		vecElements.insert( { GUI::Hash( id ), elements } );
	}

	if( LastShiftHoldTime.find( GUI::Hash( id ) ) == LastShiftHoldTime.end( ) ) {
		LastShiftHoldTime.insert( { GUI::Hash( id ), 0.f } );
	}

	if( vecFoundElements.find( GUI::Hash( id ) ) == vecFoundElements.end( ) ) {
		vecFoundElements.insert( { GUI::Hash( id ), { "" } } );
	}

	if( szText.find( GUI::Hash( id ) ) == szText.end( ) ) {
		szText.insert( { GUI::Hash( id ), "" } );
	}

	const int kurwa = 19;
	if( iPos.find( GUI::Hash( id ) ) == iPos.end( ) ) {
		iPos.insert( { GUI::Hash( id ), -kurwa } );
	}

	if( bSearchBar ) {
		DrawPos.y += 19;
	}

	if( *option < 0 )
		*option = 0;
	else if( *option >= elements.size( ) )
		*option = elements.size( ) - 1;

	if( vecElements[ GUI::Hash( id ) ].size( ) != elements.size( ) ) {
		iSlider[ GUI::Hash( id ) ] = 0.f;
		vecElements[ GUI::Hash( id ) ] = elements;
	}

	const float flBackupMultiplier = g_pSurface->DrawGetAlphaMultiplier( );

	if( !ctx->enabled )
		g_pSurface->DrawSetAlphaMultiplier( flBackupMultiplier * 0.3f );

	int iWidth = ( int )( ctx->ParentSize.x - 25 );

	bool bHoveredSearch = ctx->bActiveWindow && g_InputSystem.IsInBox( DrawPos - Vector2D( 0, kurwa ), Vector2D( iWidth, kurwa + 1 ) ) && g_InputSystem.IsInBox( ctx->ParentPos, ctx->ParentSize ) && bSearchBar;
	bool bHovered = ctx->bActiveWindow && g_InputSystem.IsInBox( DrawPos, Vector2D( iWidth, kurwa * iSizeInElements ) ) && g_InputSystem.IsInBox( ctx->ParentPos, ctx->ParentSize );

	Render::Engine::RectFilled( DrawPos, Vector2D( iWidth, kurwa * iSizeInElements ), Color( 25, 25, 25 ) );

	D3DVIEWPORT9 clip = { DrawPos.x, DrawPos.y + 1, iWidth, ( kurwa * iSizeInElements ) - 2, 0.f, 1.0f };

	bool bDrawBottomArrow = false;
	bool bDrawTopArrow = false;
	for( int i = 0; i < elements.size( ); i++ ) {
		std::string transformed_elements( elements.at( i ) );
		std::transform( transformed_elements.begin( ), transformed_elements.end( ), transformed_elements.begin( ), ::tolower );

		std::string transformed_text( szText[ GUI::Hash( id ) ] );
		std::transform( transformed_text.begin( ), transformed_text.end( ), transformed_text.begin( ), ::tolower );

		if( ( transformed_elements.find( transformed_text ) == std::string::npos ) && transformed_text.size( ) > 0 ) {
			continue;
		}
		else {
			vecFoundElements[ GUI::Hash( id ) ].push_back( elements.at( i ) );
			iPos[ GUI::Hash( id ) ] += kurwa;
		}

		bool scrollbar = vecFoundElements[ GUI::Hash( id ) ].size( ) > iSizeInElements;

		Vector2D OptionPos = DrawPos + Vector2D( 0, iPos[ GUI::Hash( id ) ] + iSlider[ GUI::Hash( id ) ] );
		Vector2D OptionSize = Vector2D( iWidth, kurwa );

		bool bInBoundsTop = OptionPos.y > ( DrawPos.y - kurwa );
		bool bInBoundsBottom = OptionPos.y + OptionSize.y <= ( DrawPos.y + kurwa ) + kurwa * ( iSizeInElements - 1 );

		bool bIsInBounds = bInBoundsTop && bInBoundsBottom;

		if( !bInBoundsBottom ) { bDrawBottomArrow = true; };
		if( !bInBoundsTop ) { bDrawTopArrow = true; };

		bool bHoveredMeme = bHovered && bIsInBounds && g_InputSystem.IsInBox( OptionPos, OptionSize - Vector2D( 6, 0 ) ) && !bHoveredSearch && g_InputSystem.IsInBox( ctx->ParentPos, ctx->ParentSize ) && ctx->FocusedID == 0;
		if( bHoveredMeme ) {
			if( InputHelper::Pressed( VK_LBUTTON ) ) {
				*option = i;
			}
		}

		if( bIsInBounds ) {
			if( *option == i ) {
				std::string name( elements[ i ].c_str( ) );
				if( name.length( ) > 20 ) {
					name.resize( 20 );
					name.append( XorStr( "..." ) );
				}

				Render::Engine::RectFilled( OptionPos, OptionSize, Color( 0, 0, 0, 60 ) );
				Render::Engine::menu_regular.string( OptionPos.x + OptionSize.x / 2, OptionPos.y + 2, g_Vars.menu.ascent.ToRegularColor( ), name, Render::Engine::ALIGN_CENTER );
			}
			else {
				std::string name( elements[ i ].c_str( ) );
				if( name.length( ) > 20 ) {
					name.resize( 20 );
					name.append( XorStr( "..." ) );
				}

				if( ( bHoveredMeme && ( ctx->FocusedID == 0 || ctx->FocusedID == GUI::Hash( id ) ) ) )
					Render::Engine::RectFilled( OptionPos, OptionSize, Color( 0, 0, 0, 30 ) );

				Render::Engine::menu_regular.string( OptionPos.x + OptionSize.x / 2, OptionPos.y + 2, ( bHoveredMeme && ( ctx->FocusedID == 0 || ctx->FocusedID == GUI::Hash( id ) ) ) ? Color( 125, 125, 125 ) : Color( 75, 75, 75 ), name, Render::Engine::ALIGN_CENTER );
			}
		}
	}

	int swag = 3;

	int scrollbar_height = -1;
	int scrollbar_pos = -1;

	if( vecFoundElements[ GUI::Hash( id ) ].size( ) > iSizeInElements ) {
		scrollbar_height = ( ( float )( kurwa * ( iSizeInElements + 1 ) ) / ( float )( kurwa * ( vecFoundElements[ GUI::Hash( id ) ].size( ) + 1 ) ) ) * ( kurwa * ( iSizeInElements + 1 ) );
		scrollbar_pos = std::min( std::max( ( -( float )iSlider[ GUI::Hash( id ) ] / ( float )( kurwa * vecFoundElements[ GUI::Hash( id ) ].size( ) ) ) * ( float )( kurwa * iSizeInElements ), 2.f ), ( kurwa * iSizeInElements ) - scrollbar_height - 2.f );
	}

	// listbox
	if( ( bHovered || ctx->FocusedID == GUI::Hash( id + "s" ) ) && vecFoundElements[ GUI::Hash( id ) ].size( ) > iSizeInElements && ( scrollbar_height != -1 && scrollbar_pos != -1 ) ) {
		if( ctx->FocusedID == 0 ) {
			if( g_InputSystem.GetScrollMouse( ) > 0 ) {
				iSlider[ GUI::Hash( id ) ] = iSlider[ GUI::Hash( id ) ] + kurwa;
			}
			else if( g_InputSystem.GetScrollMouse( ) < 0 ) {
				iSlider[ GUI::Hash( id ) ] = iSlider[ GUI::Hash( id ) ] - kurwa;
			}
		}

		bool bHoveredScroll = g_InputSystem.IsInBox( DrawPos + Vector2D( iWidth - 5, scrollbar_pos + 1 ), Vector2D( 4, scrollbar_height ) );

		static float prev_mouse = 0.f;

		const auto diff = prev_mouse - g_InputSystem.GetMousePosition( ).y;

		if( ctx->FocusedID == 0 ) {
			if( bHoveredScroll && InputHelper::Down( VK_LBUTTON ) ) {
				ctx->FocusedID = GUI::Hash( id + "s" );
			}
		}
		else if( ctx->FocusedID == GUI::Hash( id + "s" ) ) {
			if( InputHelper::Down( VK_LBUTTON ) ) {
				//iSlider[ GUI::Hash( id ) ] += ( diff / 2 ) * ( fabs( ( kurwa * vecFoundElements[ GUI::Hash( id ) ].size( ) ) - scrollbar_height ) / iSizeInElements );

				const auto scale = [ ] ( int in, int bmin, int bmax, int lmin, int lmax ) {
					return float( ( lmax - lmin ) * ( in - bmin ) ) / float( bmax - bmin ) + lmin;
				};

				// i think not perfect
				auto gs = kurwa * iSizeInElements;
				auto pizdo = std::max( float( float( gs * ( gs - 12 * 2 ) )
											  / float( ( kurwa * vecFoundElements[ GUI::Hash( id ) ].size( ) ) + ( gs - 12 * 2 ) ) ), 30.f );

				// iSlider[ GUI::Hash( id ) ] += scale( InputHelper::MouseDelta.y, 0, gs - pizdo, 0, ( kurwa * vecFoundElements[ GUI::Hash( id ) ].size( ) ) );
			}
			else {
				prev_mouse = 0.f;
				ctx->FocusedID = 0;
			}
		}

		prev_mouse = g_InputSystem.GetMousePosition( ).y;

		ctx->hovered_listbox = true;
		iSlider[ GUI::Hash( id ) ] = std::clamp<int>( iSlider[ GUI::Hash( id ) ], ( ( kurwa * ( vecFoundElements[ GUI::Hash( id ) ].size( ) - 1 ) ) * -1 ) + ( int )( kurwa * iSizeInElements ), 0 );
	}
	else {
		ctx->hovered_listbox = false;
	}

	if( vecFoundElements[ GUI::Hash( id ) ].size( ) > iSizeInElements && ( scrollbar_height != -1 && scrollbar_pos != -1 ) ) {
		Render::Engine::RectFilled( DrawPos + Vector2D( iWidth - swag - 1, scrollbar_pos - 1 ), Vector2D( swag, scrollbar_height + 2 ), Color::MenuColors::Outline( ) );
	}

	// search
	if( bSearchBar ) {
		Render::Engine::RectFilled( DrawPos - Vector2D( 0, kurwa ), Vector2D( iWidth, kurwa + 1 ), Color( 25, 25, 25 ) );

		if( ctx->FocusedID == 0 ) {
			if( ctx->typing )
				ctx->typing = false;

			if( bHoveredSearch && g_InputSystem.IsKeyDown( VK_LBUTTON ) )
				ctx->FocusedID = GUI::Hash( id + XorStr( "search" ) );
		}
		else if( ctx->FocusedID == GUI::Hash( id + XorStr( "search" ) ) ) {
			if( !ctx->typing )
				ctx->typing = true;

			if( g_InputSystem.WasKeyPressed( VK_LBUTTON ) && !bHoveredSearch ) {
				ctx->FocusedID = 0;
			}

			for( int i = 0; i < 255; i++ ) {
				if( InputHelper::Pressed( i ) ) {
					if( i != VK_BACK ) {
						if( i == VK_ESCAPE || i == VK_RETURN ) {
							ctx->FocusedID = 0;
						}
						else if( i >= 'A' && i <= 'Z' ) {
							szText[ GUI::Hash( id ) ] += ( ( g_InputSystem.IsKeyDown( VK_SHIFT ) ) || GetKeyState( VK_CAPITAL ) ) ? i : i + 32;
							iSlider[ GUI::Hash( id ) ] = 0;
						}
						else if( i >= '0' && i <= '9' || i == ' ' ) {
							szText[ GUI::Hash( id ) ] += i;
							iSlider[ GUI::Hash( id ) ] = 0;
						}
					}
					else {
						szText[ GUI::Hash( id ) ] = szText[ GUI::Hash( id ) ].substr( 0, szText[ GUI::Hash( id ) ].size( ) - 1 );
						iSlider[ GUI::Hash( id ) ] = 0;
					}
				}

				// left control + backspace = delete every fucking thing
				if( InputHelper::Down( VK_LCONTROL ) && GetAsyncKeyState( VK_BACK ) ) {
					szText[ GUI::Hash( id ) ] = szText[ GUI::Hash( id ) ].substr( 0, szText[ GUI::Hash( id ) ].size( ) - 1 );
					iSlider[ GUI::Hash( id ) ] = 0;
				}
			}

			// if we arent holding backspace store the time
			if( !GetAsyncKeyState( VK_BACK ) ) {
				LastShiftHoldTime[ GUI::Hash( id ) ] = g_pGlobalVars->curtime;
			}

			// the delta between the current time and last "non held shift" time exceeds 500ms,
			// this means that we've been holding shift for 500ms. let's erase.
			if( g_pGlobalVars->curtime - LastShiftHoldTime[ GUI::Hash( id ) ] > 0.5f ) {
				szText[ GUI::Hash( id ) ] = szText[ GUI::Hash( id ) ].substr( 0, szText[ GUI::Hash( id ) ].size( ) - 1 );
				iSlider[ GUI::Hash( id ) ] = 0;
			}
		}

		Render::Engine::menu_small.string( DrawPos.x + 4, DrawPos.y - 15, Color( 125, 125, 125 ), ( szText[ GUI::Hash( id ) ] + ( ctx->FocusedID == GUI::Hash( id + XorStr( "search" ) ) ? '_' : ' ' ) ).c_str( ) );
	}

	if( bSearchBar ) {
		Render::Engine::Rect( DrawPos - Vector2D( 0, kurwa ) + 1, Vector2D( iWidth, kurwa * iSizeInElements ) + Vector2D( 0, kurwa ) - 2, Color::Palette_t::ElementOutlines( ) );
		Render::Engine::Rect( DrawPos - Vector2D( 0, kurwa ) + 1, Vector2D( iWidth, kurwa + 2 ) - 2, Color::Palette_t::ElementOutlines( ) );
		Render::Engine::Rect( DrawPos - Vector2D( 0, kurwa ), Vector2D( iWidth, kurwa * iSizeInElements ) + Vector2D( 0, kurwa ), Color::Black( ) );
	}
	else {
		Render::Engine::Rect( DrawPos + 1, Vector2D( iWidth, kurwa * iSizeInElements ) - 2, Color::Palette_t::ElementOutlines( ) );
		Render::Engine::Rect( DrawPos, Vector2D( iWidth, kurwa * iSizeInElements ), Color::Black( ) );
	}

	if( !ctx->enabled )
		g_pSurface->DrawSetAlphaMultiplier( flBackupMultiplier );

	ctx->szLastElementName = id;
	if( ctx->FocusedID == 0 && bHovered ) {
		GUI::CopyReference( );
	}

	GUI::PushCursorPos( CursorPos + Vector2D( 0, ( kurwa * iSizeInElements ) + ( bSearchBar ? ( GUI::ObjectPadding( ) + kurwa ) : GUI::ObjectPadding( ) ) ) );
	return ctx->FocusedID == GUI::Hash( id );
}

#if defined(LUA_SCRIPTING)
// HAHAHHAHAHAHAHAHHAHAH
bool GUI::Controls::Luabox( const std::string &id, std::vector<std::string> elements, int *option, bool bSearchBar, int iSizeInElements ) {
	Vector2D CursorPos = GUI::PopCursorPos( );
	Vector2D DrawPos = ctx->pos + CursorPos;

	std::map< size_t, std::vector<std::string> > vecFoundElements;
	std::map< int, int > iPos;

	if( iSlider.find( GUI::Hash( id ) ) == iSlider.end( ) ) {
		iSlider.insert( { GUI::Hash( id ), 0.f } );
	}

	if( vecElements.find( GUI::Hash( id ) ) == vecElements.end( ) ) {
		vecElements.insert( { GUI::Hash( id ), elements } );
	}

	if( LastShiftHoldTime.find( GUI::Hash( id ) ) == LastShiftHoldTime.end( ) ) {
		LastShiftHoldTime.insert( { GUI::Hash( id ), 0.f } );
	}

	if( vecFoundElements.find( GUI::Hash( id ) ) == vecFoundElements.end( ) ) {
		vecFoundElements.insert( { GUI::Hash( id ), { "" } } );
	}

	if( szText.find( GUI::Hash( id ) ) == szText.end( ) ) {
		szText.insert( { GUI::Hash( id ), "" } );
	}

	const int kurwa = 19;
	if( iPos.find( GUI::Hash( id ) ) == iPos.end( ) ) {
		iPos.insert( { GUI::Hash( id ), -kurwa } );
	}

	if( bSearchBar ) {
		DrawPos.y += 19;
	}

	if( *option < 0 )
		*option = 0;
	else if( *option >= elements.size( ) )
		*option = elements.size( ) - 1;

	if( vecElements[ GUI::Hash( id ) ].size( ) != elements.size( ) ) {
		iSlider[ GUI::Hash( id ) ] = 0.f;
		vecElements[ GUI::Hash( id ) ] = elements;
	}

	const float flBackupMultiplier = g_pSurface->DrawGetAlphaMultiplier( );

	if( !ctx->enabled )
		g_pSurface->DrawSetAlphaMultiplier( flBackupMultiplier * 0.3f );

	int iWidth = std::min( ( int )ctx->ParentSize.x - 25, 270 );

	bool bHoveredSearch = ctx->bActiveWindow && g_InputSystem.IsInBox( DrawPos - Vector2D( 0, kurwa ), Vector2D( iWidth, kurwa + 1 ) ) && g_InputSystem.IsInBox( ctx->ParentPos, ctx->ParentSize ) && bSearchBar;
	bool bHovered = ctx->bActiveWindow && g_InputSystem.IsInBox( DrawPos, Vector2D( iWidth, kurwa * iSizeInElements ) ) && g_InputSystem.IsInBox( ctx->ParentPos, ctx->ParentSize );

	Render::Engine::RectFilled( DrawPos, Vector2D( iWidth, kurwa * iSizeInElements ), Color( 25, 25, 25 ) );

	D3DVIEWPORT9 clip = { DrawPos.x, DrawPos.y + 1, iWidth, ( kurwa * iSizeInElements ) - 2, 0.f, 1.0f };

	bool bDrawBottomArrow = false;
	bool bDrawTopArrow = false;
	for( int i = 0; i < elements.size( ); i++ ) {
		std::string transformed_elements( elements.at( i ) );
		std::transform( transformed_elements.begin( ), transformed_elements.end( ), transformed_elements.begin( ), ::tolower );

		std::string transformed_text( szText[ GUI::Hash( id ) ] );
		std::transform( transformed_text.begin( ), transformed_text.end( ), transformed_text.begin( ), ::tolower );

		if( ( transformed_elements.find( transformed_text ) == std::string::npos ) && transformed_text.size( ) > 0 ) {
			continue;
		}
		else {
			vecFoundElements[ GUI::Hash( id ) ].push_back( elements.at( i ) );
			iPos[ GUI::Hash( id ) ] += kurwa;
		}

		bool scrollbar = vecFoundElements[ GUI::Hash( id ) ].size( ) > iSizeInElements;

		Vector2D OptionPos = DrawPos + Vector2D( 0, iPos[ GUI::Hash( id ) ] + iSlider[ GUI::Hash( id ) ] );
		Vector2D OptionSize = Vector2D( iWidth, kurwa );

		bool bInBoundsTop = OptionPos.y > ( DrawPos.y - kurwa );
		bool bInBoundsBottom = OptionPos.y + OptionSize.y <= ( DrawPos.y + kurwa ) + kurwa * ( iSizeInElements - 1 );

		bool bIsInBounds = bInBoundsTop && bInBoundsBottom;

		if( !bInBoundsBottom ) { bDrawBottomArrow = true; };
		if( !bInBoundsTop ) { bDrawTopArrow = true; };

		bool bHoveredMeme = bHovered && bIsInBounds && g_InputSystem.IsInBox( OptionPos, OptionSize - Vector2D( 6, 0 ) ) && !bHoveredSearch && g_InputSystem.IsInBox( ctx->ParentPos, ctx->ParentSize ) && ctx->FocusedID == 0;
		if( bHoveredMeme ) {
			if( InputHelper::Pressed( VK_LBUTTON ) ) {
				*option = i;
			}
		}

		if( bIsInBounds ) {
			auto temp = elements.at( i );
			const bool bLoaded = Scripting::Script::IsScriptLoaded( temp.append( XorStr( ".lua" ) ) );

			if( *option == i ) {
				std::string name( elements[ i ].c_str( ) );
				if( name.length( ) > 20 ) {
					name.resize( 20 );
					name.append( XorStr( "..." ) );
				}

				Render::Engine::RectFilled( OptionPos, OptionSize, Color( 0, 0, 0, 60 ) );
				Render::Engine::menu_regular.string( OptionPos.x + OptionSize.x / 2, OptionPos.y + 2, !bLoaded ? Color( 220, 220, 220 ) : g_Vars.menu.ascent.ToRegularColor( ), name, Render::Engine::ALIGN_CENTER );
			}
			else {
				std::string name( elements[ i ].c_str( ) );
				if( name.length( ) > 20 ) {
					name.resize( 20 );
					name.append( XorStr( "..." ) );
				}

				if( ( bHoveredMeme && ( ctx->FocusedID == 0 || ctx->FocusedID == GUI::Hash( id ) ) ) )
					Render::Engine::RectFilled( OptionPos, OptionSize, Color( 0, 0, 0, 30 ) );

				Render::Engine::menu_regular.string( OptionPos.x + OptionSize.x / 2, OptionPos.y + 2,
													 bLoaded ? g_Vars.menu.ascent.ToRegularColor( ) :
													 ( bHoveredMeme && ( ctx->FocusedID == 0 || ctx->FocusedID == GUI::Hash( id ) ) )
													 ? Color( 125, 125, 125 ) :
													 Color( 75, 75, 75 ), name, Render::Engine::ALIGN_CENTER );
			}
		}
	}

	int swag = 3;

	int scrollbar_height = -1;
	int scrollbar_pos = -1;

	if( vecFoundElements[ GUI::Hash( id ) ].size( ) > iSizeInElements ) {
		scrollbar_height = ( ( float )( kurwa * ( iSizeInElements + 1 ) ) / ( float )( kurwa * ( vecFoundElements[ GUI::Hash( id ) ].size( ) + 1 ) ) ) * ( kurwa * ( iSizeInElements + 1 ) );
		scrollbar_pos = std::min( std::max( ( -( float )iSlider[ GUI::Hash( id ) ] / ( float )( kurwa * vecFoundElements[ GUI::Hash( id ) ].size( ) ) ) * ( float )( kurwa * iSizeInElements ), 2.f ), ( kurwa * iSizeInElements ) - scrollbar_height - 2.f );
	}

	// listbox
	if( ( bHovered || ctx->FocusedID == GUI::Hash( id + "s" ) ) && vecFoundElements[ GUI::Hash( id ) ].size( ) > iSizeInElements && ( scrollbar_height != -1 && scrollbar_pos != -1 ) ) {
		if( ctx->FocusedID == 0 ) {
			if( g_InputSystem.GetScrollMouse( ) > 0 ) {
				iSlider[ GUI::Hash( id ) ] = iSlider[ GUI::Hash( id ) ] + kurwa;
			}
			else if( g_InputSystem.GetScrollMouse( ) < 0 ) {
				iSlider[ GUI::Hash( id ) ] = iSlider[ GUI::Hash( id ) ] - kurwa;
			}
		}

		bool bHoveredScroll = g_InputSystem.IsInBox( DrawPos + Vector2D( iWidth - 5, scrollbar_pos + 1 ), Vector2D( 4, scrollbar_height ) );

		static float prev_mouse = 0.f;

		const auto diff = prev_mouse - g_InputSystem.GetMousePosition( ).y;

		if( ctx->FocusedID == 0 ) {
			if( bHoveredScroll && InputHelper::Down( VK_LBUTTON ) ) {
				ctx->FocusedID = GUI::Hash( id + "s" );
			}
		}
		else if( ctx->FocusedID == GUI::Hash( id + "s" ) ) {
			if( InputHelper::Down( VK_LBUTTON ) ) {
				//iSlider[ GUI::Hash( id ) ] += ( diff / 2 ) * ( fabs( ( kurwa * vecFoundElements[ GUI::Hash( id ) ].size( ) ) - scrollbar_height ) / iSizeInElements );

				const auto scale = [ ] ( int in, int bmin, int bmax, int lmin, int lmax ) {
					return float( ( lmax - lmin ) * ( in - bmin ) ) / float( bmax - bmin ) + lmin;
				};

				// i think not perfect
				auto gs = kurwa * iSizeInElements;
				auto pizdo = std::max( float( float( gs * ( gs - 12 * 2 ) )
											  / float( ( kurwa * vecFoundElements[ GUI::Hash( id ) ].size( ) ) + ( gs - 12 * 2 ) ) ), 30.f );

				// iSlider[ GUI::Hash( id ) ] += scale( InputHelper::MouseDelta.y, 0, gs - pizdo, 0, ( kurwa * vecFoundElements[ GUI::Hash( id ) ].size( ) ) );
			}
			else {
				prev_mouse = 0.f;
				ctx->FocusedID = 0;
			}
		}

		prev_mouse = g_InputSystem.GetMousePosition( ).y;

		ctx->hovered_listbox = true;
		iSlider[ GUI::Hash( id ) ] = std::clamp<int>( iSlider[ GUI::Hash( id ) ], ( ( kurwa * ( vecFoundElements[ GUI::Hash( id ) ].size( ) - 1 ) ) * -1 ) + ( int )( kurwa * iSizeInElements ), 0 );
	}
	else {
		ctx->hovered_listbox = false;
	}

	if( vecFoundElements[ GUI::Hash( id ) ].size( ) > iSizeInElements && ( scrollbar_height != -1 && scrollbar_pos != -1 ) ) {
		Render::Engine::RectFilled( DrawPos + Vector2D( iWidth - swag - 1, scrollbar_pos - 1 ), Vector2D( swag, scrollbar_height + 2 ), Color::MenuColors::Outline( ) );
	}

	// search
	if( bSearchBar ) {
		Render::Engine::RectFilled( DrawPos - Vector2D( 0, kurwa ), Vector2D( iWidth, kurwa + 1 ), Color( 25, 25, 25 ) );

		if( ctx->FocusedID == 0 ) {
			if( ctx->typing )
				ctx->typing = false;

			if( bHoveredSearch && g_InputSystem.IsKeyDown( VK_LBUTTON ) )
				ctx->FocusedID = GUI::Hash( id + XorStr( "search" ) );
		}
		else if( ctx->FocusedID == GUI::Hash( id + XorStr( "search" ) ) ) {
			if( !ctx->typing )
				ctx->typing = true;

			if( g_InputSystem.WasKeyPressed( VK_LBUTTON ) && !bHoveredSearch ) {
				ctx->FocusedID = 0;
			}

			for( int i = 0; i < 255; i++ ) {
				if( InputHelper::Pressed( i ) ) {
					if( i != VK_BACK ) {
						if( i == VK_ESCAPE || i == VK_RETURN ) {
							ctx->FocusedID = 0;
						}
						else if( i >= 'A' && i <= 'Z' ) {
							szText[ GUI::Hash( id ) ] += ( ( g_InputSystem.IsKeyDown( VK_SHIFT ) ) || GetKeyState( VK_CAPITAL ) ) ? i : i + 32;
							iSlider[ GUI::Hash( id ) ] = 0;
						}
						else if( i >= '0' && i <= '9' || i == ' ' ) {
							szText[ GUI::Hash( id ) ] += i;
							iSlider[ GUI::Hash( id ) ] = 0;
						}
					}
					else {
						szText[ GUI::Hash( id ) ] = szText[ GUI::Hash( id ) ].substr( 0, szText[ GUI::Hash( id ) ].size( ) - 1 );
						iSlider[ GUI::Hash( id ) ] = 0;
					}
				}

				// left control + backspace = delete every fucking thing
				if( InputHelper::Down( VK_LCONTROL ) && GetAsyncKeyState( VK_BACK ) ) {
					szText[ GUI::Hash( id ) ] = szText[ GUI::Hash( id ) ].substr( 0, szText[ GUI::Hash( id ) ].size( ) - 1 );
					iSlider[ GUI::Hash( id ) ] = 0;
				}
			}

			// if we arent holding backspace store the time
			if( !GetAsyncKeyState( VK_BACK ) ) {
				LastShiftHoldTime[ GUI::Hash( id ) ] = g_pGlobalVars->curtime;
			}

			// the delta between the current time and last "non held shift" time exceeds 500ms,
			// this means that we've been holding shift for 500ms. let's erase.
			if( g_pGlobalVars->curtime - LastShiftHoldTime[ GUI::Hash( id ) ] > 0.5f ) {
				szText[ GUI::Hash( id ) ] = szText[ GUI::Hash( id ) ].substr( 0, szText[ GUI::Hash( id ) ].size( ) - 1 );
				iSlider[ GUI::Hash( id ) ] = 0;
			}
		}

		Render::Engine::menu_small.string( DrawPos.x + 4, DrawPos.y - 15, Color( 125, 125, 125 ), ( szText[ GUI::Hash( id ) ] + ( ctx->FocusedID == GUI::Hash( id + XorStr( "search" ) ) ? '_' : ' ' ) ).c_str( ) );
	}

	if( bSearchBar ) {
		Render::Engine::Rect( DrawPos - Vector2D( 0, kurwa ) + 1, Vector2D( iWidth, kurwa * iSizeInElements ) + Vector2D( 0, kurwa ) - 2, Color::Palette_t::ElementOutlines( ) );
		Render::Engine::Rect( DrawPos - Vector2D( 0, kurwa ) + 1, Vector2D( iWidth, kurwa + 2 ) - 2, Color::Palette_t::ElementOutlines( ) );
		Render::Engine::Rect( DrawPos - Vector2D( 0, kurwa ), Vector2D( iWidth, kurwa * iSizeInElements ) + Vector2D( 0, kurwa ), Color::Black( ) );
	}
	else {
		Render::Engine::Rect( DrawPos + 1, Vector2D( iWidth, kurwa * iSizeInElements ) - 2, Color::Palette_t::ElementOutlines( ) );
		Render::Engine::Rect( DrawPos, Vector2D( iWidth, kurwa * iSizeInElements ), Color::Black( ) );
	}

	if( !ctx->enabled )
		g_pSurface->DrawSetAlphaMultiplier( flBackupMultiplier );

	GUI::PushCursorPos( CursorPos + Vector2D( 0, ( kurwa * iSizeInElements ) + ( bSearchBar ? ( GUI::ObjectPadding( ) + kurwa ) : GUI::ObjectPadding( ) ) ) );
	return ctx->FocusedID == GUI::Hash( id );
}

#endif