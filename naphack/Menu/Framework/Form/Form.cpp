#include "../gui.h"
#include "../../../pandora.hpp"
#include <iomanip>

#include "../Utils/Config.hpp"
#include "../../../Features/Visuals/EventLogger.hpp"
#include "../Utils/base64.h"

#pragma warning(disable : 4018)

bool TitleBarHovered;

std::map<int, float> TabAnim;
std::map<int, float> TabHoverAnim;

std::map<int, float> flSubTabAnim;
std::map<int, float> flSubTabHoverAnim;

static float flContextFade = 0.f;
static float flTooltipFade = 0.f;

static float flRoundRadius = 3.f;

void retard( ) {
	if( GUI::ctx->FocusedID != 0 )
		GUI::ctx->FocusedID = 0;

	if( GUI::ctx->typing )
		GUI::ctx->typing = false;

	if( GUI::ctx->PopupInfo.HashedID != 0 )
		GUI::ctx->PopupInfo.HashedID = false;

	if( GUI::ctx->PopupInfo.bOpen != false )
		GUI::ctx->PopupInfo.bOpen = false;

	if( GUI::ctx->PopupInfo.ActiveButton != 0 )
		GUI::ctx->PopupInfo.ActiveButton = 0;

	if( GUI::ctx->PopupInfo.flAnimation != 0.f )
		GUI::ctx->PopupInfo.flAnimation = 0.f;

	if( GUI::ctx->HotkeyInfo.HashedID != 0 )
		GUI::ctx->HotkeyInfo.HashedID = 0;

	if( GUI::ctx->dragging )
		GUI::ctx->dragging = false;

	if( GUI::ctx->DropdownInfo.HashedID != 0 )
		GUI::ctx->DropdownInfo.HashedID = 0;

	if( GUI::ctx->ColorPickerInfo.HashedID != 0 )
		GUI::ctx->ColorPickerInfo.HashedID = false;

	if( GUI::ctx->MultiDropdownInfo.HashedID != 0 )
		GUI::ctx->MultiDropdownInfo.HashedID = 0;

	if( flContextFade != 0.f )
		flContextFade = 0.f;
}

// cos kys 
void GUI::Form::HandleDrag( ) {
	if( ctx->FocusedID == 0 ) {
		Vector2D vecMouseDelta = g_InputSystem.GetMouseDelta( );

		if( !ctx->dragging && !ctx->dragging_scrollbar ) {
			if( InputHelper::Pressed( VK_LBUTTON ) && ctx->bActiveWindow && g_InputSystem.IsInBox( ctx->pos, Vector2D( ctx->size.x, 16 ) ) ) {
				ctx->dragging = true;
			}
		}
		else if( ctx->dragging ) {
			if( InputHelper::Down( VK_LBUTTON ) ) {
				ctx->pos -= vecMouseDelta;
				//ctx->flLastActivity = g_pGlobalVars->realtime;
				ctx->override_cursor = LoadCursor( NULL, IDC_SIZEALL );
			}
			else {
				ctx->dragging = false;
			}
		}
	}

}

bool GUI::Form::BeginWindow( std::string name ) {
	static bool center = false;
	if( !center ) {
		// comment out 
		ctx->pos = ( Render::GetScreenSize( ) / 2 ) - ( ctx->size / 2 );
		center = true;
	}

	// reset it 
	ctx->override_cursor = NULL;

	static bool bReset = true;
	if( ctx->animation <= 0.0f ) {
		if( bReset ) {
			TabAnim.clear( );
			TabHoverAnim.clear( );
			retard( );

			//SetCursor( LoadCursor( NULL, IDC_ARROW ) );
			bReset = false;
		}

		if( !ctx->setup )
			return false;
	}

	bReset = true;

	if( ctx->FocusedID == 0 )
		ctx->typing = false;

	// account for overlapping windows
	bool bDont = false;
	for( auto &wnd : m_vecWindows ) {
		if( wnd.m_pContext == ctx ) {
			continue;
		}

		if( wnd.m_pContext->bHoveringWindow || wnd.m_pContext->dragging ) {
			bDont = true;
		}
	}

	ctx->bActiveWindow = true;

	ctx->bHoveringWindow = !bDont && g_InputSystem.IsInBox( ctx->pos, ctx->size );

	if( ctx->bHoveringWindow && InputHelper::Released( VK_LBUTTON ) ) {
		ctx->flLastActivity = g_pGlobalVars->realtime;
	}

	//TitleBarHovered = g_InputSystem.IsInBox( ctx->pos, Vector2D( ctx->size.x, 16 ) );

	HandleDrag( );

	// swap cursor pos stack
	std::stack< Vector2D >( ).swap( ctx->CursorPosStack );

	bool cringe = ctx->animation < 1.f;

	// main form
	Render::Engine::RectFilled( ctx->pos + Vector2D( 0, 16 ), ctx->size - Vector2D( 0, 16 + 35 ), Color::Palette_t::FormColor( ) );
	if( cringe )Render::Engine::RectFilled( ctx->pos + Vector2D( 0, 16 ), ctx->size - Vector2D( 0, 16 + 35 ), Color::Palette_t::FormColor( ) );
	if( cringe )Render::Engine::RectFilled( ctx->pos + Vector2D( 0, 16 ), ctx->size - Vector2D( 0, 16 + 35 ), Color::Palette_t::FormColor( ) );

	// title bar 
	Render::Engine::RectFilled( ctx->pos, Vector2D( ctx->size.x, 16 ), Color::Palette_t::FormColorLight( ) );
	if( cringe )Render::Engine::RectFilled( ctx->pos, Vector2D( ctx->size.x, 16 ), Color::Palette_t::FormColorLight( ) );
	if( cringe )Render::Engine::RectFilled( ctx->pos, Vector2D( ctx->size.x, 16 ), Color::Palette_t::FormColorLight( ) );

	// bottom tabs 
	Render::Engine::RectFilled( ctx->pos + Vector2D( 0, ctx->size.y - 35 ), Vector2D( ctx->size.x, 35 ), Color::Palette_t::FormColorLight( ) );
	if( cringe )Render::Engine::RectFilled( ctx->pos + Vector2D( 0, ctx->size.y - 35 ), Vector2D( ctx->size.x, 35 ), Color::Palette_t::FormColorLight( ) );
	if( cringe )Render::Engine::RectFilled( ctx->pos + Vector2D( 0, ctx->size.y - 35 ), Vector2D( ctx->size.x, 35 ), Color::Palette_t::FormColorLight( ) );

	// top gradient
	Render::Engine::Gradient( ctx->pos + Vector2D( 0, 16 ), Vector2D( ctx->size.x / 2, 1 ), Color( 0, 0, 0, 0 ), g_Vars.menu.ascent.ToRegularColor( ), true );
	Render::Engine::Gradient( ctx->pos + Vector2D( ctx->size.x / 2, 16 ), Vector2D( ctx->size.x / 2, 1 ), g_Vars.menu.ascent.ToRegularColor( ), Color( 0, 0, 0, 0 ), true );

	// bottom gradient
	Render::Engine::Gradient( ctx->pos + Vector2D( 0, ctx->size.y - 36 ), Vector2D( ctx->size.x / 2, 1 ), Color( 0, 0, 0, 0 ), g_Vars.menu.ascent.ToRegularColor( ), true );
	Render::Engine::Gradient( ctx->pos + Vector2D( ctx->size.x / 2, ctx->size.y - 36 ), Vector2D( ctx->size.x / 2, 1 ), g_Vars.menu.ascent.ToRegularColor( ), Color( 0, 0, 0, 0 ), true );

	// outlines
	Render::Engine::Rect( ctx->pos - 1, ctx->size + 2, Color::Black( ) );
	Render::Engine::Rect( ctx->pos, ctx->size, Color::Palette_t::ElementOutlines( ) );

	// cheat name
	Render::Engine::menu_regular.string( ctx->pos.x + ( ctx->size.x / 2 ), ctx->pos.y, ctx->dragging ? Color::White( ) : Color( 150, 150, 150 ), name, Render::Engine::ALIGN_CENTER );

	// at what (x, y) relative to the menu's initial cursor position (0,0) we should draw the menu elements
	GUI::PushCursorPos( Vector2D( 20, 35 ) );
	return true;
}

void GUI::Form::EndWindow( std::string name ) {

	// render tabs and subtabs
	if( ctx->Tabs.size( ) > 0 ) {

		std::array<Vector2D, 10> vecTabPos;
		std::array<Vector2D, 10> vecTabSize;

		for( int i = 0; i < ctx->Tabs.size( ); ++i ) {
			const bool bHomePage = i == ctx->Tabs.size( ) - 1;

			vecTabSize[ i ] = Vector2D( Render::Engine::menu_regular.size( ctx->Tabs[ i ].first ).m_width + 40, 34 );

			if( i == 0 )
				vecTabPos[ i ] = ctx->pos + Vector2D( 1, ctx->size.y - 35 );
			else {
				if( bHomePage )
					vecTabPos[ i ] = ctx->pos + Vector2D( ctx->size.x - 30, ctx->size.y - 31 );
				else
					vecTabPos[ i ] = vecTabPos[ i - 1 ] + Vector2D( vecTabSize[ i - 1 ].x + 5, 0 );
			}

			bool bHovered = ( bHomePage ?
							  g_InputSystem.IsInBox( vecTabPos[ i ] - Vector2D( vecTabSize[ i ].x, 4 ), vecTabSize[ i ] + Vector2D( 30, 0 ) ) :
							  g_InputSystem.IsInBox( vecTabPos[ i ], vecTabSize[ i ] ) ) && ctx->FocusedID == 0;

			static int nWantedTab = -1;
			if( ctx->FocusedID == 0 ) {
				if( bHovered && InputHelper::Pressed( VK_LBUTTON ) ) {
					ctx->FocusedID = GUI::Hash( XorStr( "tab" ) ) + i;
					nWantedTab = i;
				}
			}
			else if( ctx->FocusedID == GUI::Hash( XorStr( "tab" ) ) + i ) {
				if( InputHelper::Released( VK_LBUTTON ) ) {
					ctx->ActiveTab = nWantedTab;
					retard( );
				}
			}

			Color tabColor = Color( 170, 170, 170 );
			Color infoColor = Color( 90, 90, 90 );
			if( bHovered ) {
				tabColor = Color( 200, 200, 200 );
				infoColor = Color( 110, 110, 110 );
			}

			if( ctx->ActiveTab == i ) {
				tabColor = g_Vars.menu.ascent.ToRegularColor( );
				infoColor = Color( 130, 130, 130 );
			}

			// render 'home' differently
			if( bHomePage ) {
				Render::Engine::menu_regular.string( vecTabPos[ i ].x, vecTabPos[ i ].y, tabColor, ctx->Tabs[ i ].first, Render::Engine::ALIGN_RIGHT );
				Render::Engine::menu_regular.string( vecTabPos[ i ].x, vecTabPos[ i ].y + 12, infoColor, ctx->Tabs[ i ].second, Render::Engine::ALIGN_RIGHT );

				Render::Engine::Texture( vecTabPos[ i ] + Vector2D( 4, 3 ), { 21,21 }, ( ETextures )i, tabColor );

				continue;
			}

			Render::Engine::menu_regular.string( vecTabPos[ i ].x + 32, vecTabPos[ i ].y + 5, tabColor, ctx->Tabs[ i ].first );
			Render::Engine::menu_regular.string( vecTabPos[ i ].x + 32, vecTabPos[ i ].y + 17, infoColor, ctx->Tabs[ i ].second );

			Render::Engine::Texture( vecTabPos[ i ] + Vector2D( 6, 7 ), { 21,21 }, ( ETextures )i, tabColor );
			Render::Engine::Rect( vecTabPos[ i ] + Vector2D( vecTabSize[ i ].x + 2, 5 ), Vector2D( 1, vecTabSize[ i ].y - 10 ), Color( 79, 79, 79 ) );
		}

	}

	static float ColorPickerHue = -1.f;
	static std::map<size_t, float> ColorPickerAlpha;

	if( ctx->ColorPickerInfo.HashedID != 0 ) {
		size_t id = ctx->ColorPickerInfo.HashedID;

		Vector2D DrawPos = ctx->ColorPickerInfo.vecPos;

		DrawPos.x = int( DrawPos.x );
		DrawPos.y = int( DrawPos.y );

		static std::unordered_map<size_t, float> flNewSaturation;
		static std::unordered_map<size_t, float> flNewBrightness;

		if( ctx->ColorPickerInfo.bRightClicked ) {
			DrawPos.x -= 1;

			bool bIsValidHexColor = false;
			bool bHasHashtag = false;

			Vector2D PastePos = DrawPos + Vector2D( 0, 20 );

			// copy
			if( g_InputSystem.IsInBox( DrawPos, Vector2D( 85, 20 ) ) ) {
				Render::Engine::RectFilled( DrawPos, Vector2D( 85, 20 ), { 30, 30, 30 } );
				if( InputHelper::Pressed( VK_LBUTTON ) ) {
					std::stringstream color_hex;

					color_hex << XorStr( "#" );
					color_hex << std::hex << std::setw( 2 ) << std::setfill( '0' ) << std::uppercase << int( ( ( *ctx->ColorPickerInfo.fColor ).ToRegularColor( ) ).r( ) );
					color_hex << std::hex << std::setw( 2 ) << std::setfill( '0' ) << std::uppercase << int( ( ( *ctx->ColorPickerInfo.fColor ).ToRegularColor( ) ).g( ) );
					color_hex << std::hex << std::setw( 2 ) << std::setfill( '0' ) << std::uppercase << int( ( ( *ctx->ColorPickerInfo.fColor ).ToRegularColor( ) ).b( ) );
					color_hex << std::hex << std::setw( 2 ) << std::setfill( '0' ) << std::uppercase << int( ( ( *ctx->ColorPickerInfo.fColor ).ToRegularColor( ) ).a( ) );

					if( OpenClipboard( nullptr ) ) {
						EmptyClipboard( );
						HGLOBAL clipboard_buffer = GlobalAlloc( GMEM_DDESHARE, color_hex.str( ).size( ) + 1 );
						char *buffer = ( char * )GlobalLock( clipboard_buffer );
						strcpy( buffer, color_hex.str( ).c_str( ) );

						GlobalUnlock( clipboard_buffer );
						SetClipboardData( CF_TEXT, clipboard_buffer );
						CloseClipboard( );
					}

					ctx->FocusedID = ctx->ColorPickerInfo.HashedID = 0;
				}
			}
			else {
				Render::Engine::RectFilled( DrawPos, Vector2D( 85, 20 ), { 25, 25, 25 } );
			}

			// paste
			if( g_InputSystem.IsInBox( PastePos, Vector2D( 85, 20 ) ) ) {
				Render::Engine::RectFilled( PastePos, Vector2D( 85, 20 ), { 30, 30, 30 } );
				if( InputHelper::Pressed( VK_LBUTTON ) ) {
					ColorPickerHue = -1.f;

					// bruh this better be the right format...
					if( IsClipboardFormatAvailable( CF_TEXT ) ) {
						if( OpenClipboard( nullptr ) ) {
							std::string input( ( char * )GetClipboardData( CF_TEXT ) );
							if( !input.empty( ) ) {
								bHasHashtag = input.at( 0 ) == '#';

								bIsValidHexColor = ( input.size( ) <= 8 + int( bHasHashtag ) );

								// anything smaller than xxxxxx
								if( input.size( ) < 6 )
									bIsValidHexColor = false;

								if( bIsValidHexColor ) {
									for( int i = 0; i < input.length( ); ++i ) {
										auto n = input[ i ];

										if( bHasHashtag && n == '#' )
											continue;

										if( !std::isxdigit( n ) ) {
											bIsValidHexColor = false;
											break;
										}
									}
								}

								// check if we have a hex color copied in our clipboard
								if( bIsValidHexColor ) {
									int component_r = std::stoi( input.substr( 0 + int( bHasHashtag ), 2 ), 0, 16 );
									int component_g = std::stoi( input.substr( 2 + int( bHasHashtag ), 2 ), 0, 16 );
									int component_b = std::stoi( input.substr( 4 + int( bHasHashtag ), 2 ), 0, 16 );
									int component_a = input.size( ) > ( 6 + int( bHasHashtag ) ) ? std::stoi( input.substr( 6 + int( bHasHashtag ), 2 ), 0, 16 ) : 255;

									//printf( "%i\n", component_a );

									Color clr = Color( component_r, component_g, component_b, component_a );
									( *ctx->ColorPickerInfo.fColor ).SetColor( clr );

									if( ColorPickerAlpha.find( id ) == ColorPickerAlpha.end( ) ) {
										ColorPickerAlpha.insert( { id, clr.a( ) } );
									}
									else {
										ColorPickerAlpha.at( id ) = clr.a( );
									}

									Color::Hsv_t clrHSV = Color::RGBtoHSV( clr );

									if( flNewSaturation.find( id ) == flNewSaturation.end( ) ) {
										flNewSaturation.insert( { id, clrHSV.Saturation } );
										flNewBrightness.insert( { id, clrHSV.Value } );
									}
									else {
										flNewSaturation[ id ] = clrHSV.Saturation;
										flNewBrightness[ id ] = clrHSV.Value;
									}
								}
							}
							CloseClipboard( );
						}
					}


					ctx->FocusedID = ctx->ColorPickerInfo.HashedID = 0;
				}
			}
			else {
				Render::Engine::RectFilled( PastePos, Vector2D( 85, 20 ), { 25, 25, 25 } );
			}

			Render::Engine::menu_small.string( DrawPos.x + 8, DrawPos.y + 3, Color( 150, 150, 150 ), XorStr( "copy" ) );
			Render::Engine::menu_small.string( PastePos.x + 8, PastePos.y + 3, Color( 150, 150, 150 ), XorStr( "paste" ) );

			Render::Engine::Rect( DrawPos, Vector2D( 85, 40 ), { 10, 10, 10 } );

			if( InputHelper::Pressed( VK_LBUTTON ) ) {
				ctx->FocusedID = ctx->ColorPickerInfo.HashedID = 0;
			}
		}
		else {
			Vector2D DrawSize = Vector2D( 200, 200 );

			Render::Engine::RectFilled( DrawPos, DrawSize - Vector2D( 0, ctx->ColorPickerInfo.bRenderAlpha ? 0 : 15 ), /*3.f,*/ Color::Palette_t::FormColor( ) );
			Render::Engine::Rect( DrawPos, DrawSize - Vector2D( 0, ctx->ColorPickerInfo.bRenderAlpha ? 0 : 15 ), /*3.f,*/ Color::Black( ) );
			Render::Engine::Rect( DrawPos + 1, DrawSize - Vector2D( 0, ctx->ColorPickerInfo.bRenderAlpha ? 0 : 15 ) - 2, /*3.f,*/ Color::Palette_t::ElementOutlines( ) );

			Vector2D ColorDrawPos( DrawPos + 5 );
			Vector2D ColorDrawSize( Vector2D( DrawSize.x - 30, DrawSize.y - 25 ) );

			Vector2D AlphaDrawPos( ColorDrawPos + Vector2D( 0, ColorDrawSize.y + 4 ) );
			Vector2D AlphaDrawSize = Vector2D( ColorDrawSize.x, 11 );

			Vector2D HueDrawPos( ColorDrawPos + Vector2D( ColorDrawSize.x + 4, -1 ) );
			Vector2D HueDrawSize = Vector2D( 16, ColorDrawSize.y );

			if( ColorPickerAlpha.find( ctx->ColorPickerInfo.HashedID ) == ColorPickerAlpha.end( ) ) {
				ColorPickerAlpha.insert( { ctx->ColorPickerInfo.HashedID, 255.f } );
			}

			Color::Hsv_t clrHSV = Color::RGBtoHSV( ( *ctx->ColorPickerInfo.fColor ).ToRegularColor( ) );

			float flNewHue;
			if( flNewSaturation.find( id ) == flNewSaturation.end( ) ) {
				flNewSaturation.insert( { id, clrHSV.Saturation } );
				flNewBrightness.insert( { id, clrHSV.Value } );
			}

			if( ColorPickerHue == -1.f ) {
				flNewHue = clrHSV.Hue;
				ColorPickerHue = flNewHue;
			}
			else {
				flNewHue = ColorPickerHue;
			}

			for( int i = 0; i < HueDrawSize.y; i++ ) {
				float Hue = ( ( float )i / HueDrawSize.y ) * 360.f;
				Color HueColor = Color::HSVtoRGB( Hue, 1, 1 );

				Render::Engine::Rect( HueDrawPos + Vector2D( 0, i + 1 ), Vector2D( HueDrawSize.x, 1 ), HueColor.OverrideAlpha( 255 ) );
			}

			Render::Engine::Rect( HueDrawPos + Vector2D( 0, 1 ), HueDrawSize, Color( 10, 10, 10, 255 ) );

			// handle dragging hue
			bool bHoveringHue = g_InputSystem.IsInBox( HueDrawPos, HueDrawSize );
			if( !ctx->ColorPickerInfo.bPickingHue && !ctx->ColorPickerInfo.bPickingAlpha && !ctx->ColorPickerInfo.bPickingColor ) {
				if( bHoveringHue && InputHelper::Pressed( VK_LBUTTON ) ) {
					ctx->ColorPickerInfo.bPickingHue = true;
				}
			}
			else if( ctx->ColorPickerInfo.bPickingHue ) {
				if( InputHelper::Down( VK_LBUTTON ) ) {
					flNewHue = ( ( g_InputSystem.GetMousePosition( ).y - HueDrawPos.y ) / HueDrawSize.y ) * 360.f;

					flNewHue = std::clamp<float>( flNewHue, 0.f, 359.f );
					ColorPickerHue = flNewHue;
				}
				else {
					ctx->ColorPickerInfo.bPickingHue = false;
				}
			}

			// hue
			float HueAdditive = ( HueDrawSize.y * ( flNewHue / 360.f ) );

			Render::Engine::RectFilled( HueDrawPos + Vector2D( 1, ( HueAdditive - 3 ) < 0 ? 1 : HueAdditive - 2 ), Vector2D( HueDrawSize.x - 2, 3 ), Color( 255, 255, 255, 180 ) );
			Render::Engine::Rect( HueDrawPos + Vector2D( 1, ( HueAdditive - 3 ) < 0 ? 1 : HueAdditive - 2 ), Vector2D( HueDrawSize.x - 2, 3 ), Color( 10, 10, 10, 255 ) );

			if( ctx->ColorPickerInfo.bRenderAlpha ) {
				Render::Engine::RectFilled( AlphaDrawPos, AlphaDrawSize, ( ( *ctx->ColorPickerInfo.fColor ).ToRegularColor( ) ).OverrideAlpha( 255, true ) );
				Render::Engine::Rect( AlphaDrawPos, AlphaDrawSize, Color( 10, 10, 10, 255 ) );

				// handle dragging alpha
				ColorPickerAlpha.at( ctx->ColorPickerInfo.HashedID ) = ( ( *ctx->ColorPickerInfo.fColor ).ToRegularColor( ) ).a( );

				bool bHoveringAlpha = g_InputSystem.IsInBox( AlphaDrawPos, AlphaDrawSize );
				if( !ctx->ColorPickerInfo.bPickingAlpha && !ctx->ColorPickerInfo.bPickingHue && !ctx->ColorPickerInfo.bPickingColor ) {
					if( bHoveringAlpha && InputHelper::Pressed( VK_LBUTTON ) ) {
						ctx->ColorPickerInfo.bPickingAlpha = true;
					}
				}
				else if( ctx->ColorPickerInfo.bPickingAlpha ) {
					if( InputHelper::Down( VK_LBUTTON ) ) {
						float FinalValue = std::clamp<float>( GUI::MapNumber( std::clamp<float>(
							Vector2D( g_InputSystem.GetMousePosition( ) - AlphaDrawPos ).x, 0, AlphaDrawSize.x ),
							0, AlphaDrawSize.x, 0, 255 ), 0, 255 );

						ColorPickerAlpha.at( ctx->ColorPickerInfo.HashedID ) = FinalValue;
					}
					else {
						ctx->ColorPickerInfo.bPickingAlpha = false;
					}
				}
			}

			// handle picking color
			bool bHoveredPicker = g_InputSystem.IsInBox( ColorDrawPos, ColorDrawSize );
			if( !ctx->ColorPickerInfo.bPickingColor && !ctx->ColorPickerInfo.bPickingHue && !ctx->ColorPickerInfo.bPickingAlpha ) {
				if( bHoveredPicker && InputHelper::Pressed( VK_LBUTTON ) ) {
					ctx->ColorPickerInfo.bPickingColor = true;
				}
			}
			else if( ctx->ColorPickerInfo.bPickingColor ) {
				if( InputHelper::Down( VK_LBUTTON ) ) {
					Vector2D vecCursorDelta = g_InputSystem.GetMousePosition( ) - ColorDrawPos;

					if( vecCursorDelta.x < 0.f )
						vecCursorDelta.x = 0.f;

					if( vecCursorDelta.x > ColorDrawSize.x )
						vecCursorDelta.x = ColorDrawSize.x;

					if( vecCursorDelta.y < 0.f )
						vecCursorDelta.y = 0.f;

					if( vecCursorDelta.y > ColorDrawSize.y )
						vecCursorDelta.y = ColorDrawSize.y;

					flNewSaturation[ id ] = float( vecCursorDelta.x ) / float( ColorDrawSize.x );
					flNewBrightness[ id ] = 1.f - ( float( vecCursorDelta.y ) / float( ColorDrawSize.y ) );
				}
				else {
					ctx->ColorPickerInfo.bPickingColor = false;
				}
			}

			if( ctx->ColorPickerInfo.bRenderAlpha ) {
				( *ctx->ColorPickerInfo.fColor ).SetColor( Color::HSVtoRGB( flNewHue, flNewSaturation[ id ], flNewBrightness[ id ] )
														   .OverrideAlpha( ColorPickerAlpha.at( ctx->ColorPickerInfo.HashedID ) ) );

				// render alpha bar drag hand
				float AlphaAdditive = ( AlphaDrawSize.x * ( ColorPickerAlpha.at( ctx->ColorPickerInfo.HashedID ) / 255.f ) );

				Render::Engine::RectFilled( AlphaDrawPos + Vector2D( ( AlphaAdditive - 3 ) < 0 ? 0 : AlphaAdditive - 3, 1 ), Vector2D( 3, AlphaDrawSize.y - 2 ), Color( 255, 255, 255, 180 ) );
				Render::Engine::Rect( AlphaDrawPos + Vector2D( ( AlphaAdditive - 3 ) < 0 ? 0 : AlphaAdditive - 3, 1 ), Vector2D( 3, AlphaDrawSize.y - 2 ), Color( 10, 10, 10, 255 ) );
			}
			else {
				( *ctx->ColorPickerInfo.fColor ).SetColor( Color::HSVtoRGB( flNewHue, flNewSaturation[ id ], flNewBrightness[ id ] ) );
			}

			Render::Engine::Gradient( ColorDrawPos, ColorDrawSize, Color( 255, 255, 255 ), Color::HSVtoRGB( flNewHue, 1, 1 ).OverrideAlpha( 255 ), true );
			Render::Engine::Gradient( ColorDrawPos, ColorDrawSize, Color( 0, 0, 0, 0 ), Color( 0, 0, 0, 255 ), false );
			Render::Engine::Texture( ColorDrawPos, ColorDrawSize, ETextures::GRADIENT, Color::White( ) );
			Render::Engine::Rect( ColorDrawPos, ColorDrawSize, Color( 10, 10, 10, 255 ) );

			// render color hand grab
			Vector2D vecGrabPos = ( ColorDrawPos + Vector2D( ColorDrawSize.x * flNewSaturation[ id ], ColorDrawSize.y * ( 1.f - flNewBrightness[ id ] ) ) ) - 1;
			vecGrabPos.x = std::clamp<float>( vecGrabPos.x, ColorDrawPos.x + 1, ColorDrawPos.x + ColorDrawSize.x - 4 );
			vecGrabPos.y = std::clamp<float>( vecGrabPos.y, ColorDrawPos.y + 1, ColorDrawPos.y + ColorDrawSize.y - 5 );

			Render::Engine::Rect( vecGrabPos, Vector2D( 4, 4 ), Color::Black( ) );
			Render::Engine::Rect( vecGrabPos + 1, Vector2D( 4, 4 ) - 2, Color( 255, 255, 255, 180 ) );

			if( InputHelper::Pressed( VK_LBUTTON ) && !g_InputSystem.IsInBox( DrawPos, DrawSize ) ) {
				ctx->ColorPickerInfo.HashedID = 0;
				ctx->FocusedID = 0;
			}
		}
	}
	else {
		ColorPickerHue = -1.f;
	}

	if( ctx->DropdownInfo.HashedID != 0 ) {
		Vector2D DrawPos = ctx->DropdownInfo.Pos;
		size_t id = ctx->DropdownInfo.HashedID;

		if( ctx->DropdownInfo.uScroll.find( id ) == ctx->DropdownInfo.uScroll.end( ) ) {
			ctx->DropdownInfo.uScroll.insert( { id, 0.f } );
		}

		const int nElementsCount = std::clamp<int>( ctx->DropdownInfo.Elements.size( ), 0, ctx->DropdownInfo.MaxItems );
		const bool bNeedsScroll = ctx->DropdownInfo.Elements.size( ) > ctx->DropdownInfo.MaxItems;

		const int nMaxHeight = 19 * nElementsCount;
		const int nActualHeight = 19 * ctx->DropdownInfo.Elements.size( );

		const bool bHoveredMain = g_InputSystem.IsInBox( DrawPos, Vector2D( ctx->DropdownInfo.Size, nMaxHeight ) );

		// don't ask.............
		Vector2D ScrollbarPos = DrawPos + Vector2D( 2, 2 ) + Vector2D(
			ctx->DropdownInfo.Size - 6,
			( ( float )ctx->DropdownInfo.uScroll[ id ] / ( float )nActualHeight ) * ( float )nMaxHeight );

		Vector2D ScrollbarSize = Vector2D( 3, ( ( float )nMaxHeight / ( float )nActualHeight ) * nMaxHeight );

		const bool bHoveredScroll = g_InputSystem.IsInBox( ScrollbarPos, ScrollbarSize );

		if( bNeedsScroll ) {
			//FIXME if you are in the middle of a scroll (drag or not)
			// and you suddenly unhover then it will stop, and if you hover
			// back it will continue: this is rlly easy to fix, but im just tired
			if( bHoveredMain || bHoveredScroll || ctx->DropdownInfo.DraggingScroll ) {
				// Hey, yeah, saw you, then I told you that I got you whenever
				// Gotta watch you when you movin', 'cause you think you so clever
				// Swear you treated me so cold, just like the snow in December
				// Don't know if we could last together forever
				// Gave you time and then you told me that you want me to go
				// Could I make it up to you ?
				// Guess I'll never know
				// I think I'm losin' all my friends and I'm attractin' these hoes
				// Thought they were my friends, but they foe, for sure

				static float prev_mouse = 0.f;

				const auto diff = prev_mouse - g_InputSystem.GetMousePosition( ).y;
				const auto scale = [ ] ( int in, int bmin, int bmax, int lmin, int lmax ) {
					return float( ( lmax - lmin ) * ( in - bmin ) ) / float( bmax - bmin ) + lmin;
				};

				if( !ctx->DropdownInfo.DraggingScroll && InputHelper::Down( VK_LBUTTON ) && bHoveredScroll ) {
					ctx->DropdownInfo.DraggingScroll = true;
				}
				else if( ctx->DropdownInfo.DraggingScroll ) {
					if( InputHelper::Down( VK_LBUTTON ) ) {
						ctx->DropdownInfo.uScroll[ id ] -= scale( diff, 0, nActualHeight - ScrollbarSize.y, 0, nActualHeight - nMaxHeight );//FIXME it works fine on large sliders, but on bigger ones it sucks (idk why for any of them)
					}
					else {
						prev_mouse = 0.f;
						ctx->DropdownInfo.DraggingScroll = false;
					}
				}

				prev_mouse = g_InputSystem.GetMousePosition( ).y;

				if( !ctx->DropdownInfo.DraggingScroll )
					ctx->DropdownInfo.uScroll[ id ] = ctx->DropdownInfo.uScroll[ id ] + 10 * ( -g_InputSystem.GetScrollMouse( ) );
			}
			else if( !bHoveredScroll ) {
				ctx->DropdownInfo.DraggingScroll = false;
			}

			ctx->DropdownInfo.uScroll[ id ] = std::clamp<float>( ctx->DropdownInfo.uScroll[ id ], 0, nActualHeight - nMaxHeight );
		}

		Render::Engine::SetClip( DrawPos, Vector2D( ctx->DropdownInfo.Size, ( 19 * nElementsCount ) ) );
		for( int i = 0; i < ctx->DropdownInfo.Elements.size( ); i++ ) {
			Vector2D ElementPos = DrawPos + Vector2D( 0, 19 * i ) - Vector2D( 0, ctx->DropdownInfo.uScroll[ id ] );

			Vector2D ElementSize = Vector2D( ctx->DropdownInfo.Size, 19 );
			size_t hash = id + GUI::Hash( std::to_string( i ) );

			bool bHover = g_InputSystem.IsInBox( ElementPos, ElementSize - Vector2D( 6, 0 ) );
			bool bOutOfBounds = ElementPos.y > DrawPos.y + ( 19 * ( nElementsCount - 1 ) ) || ElementPos.y < DrawPos.y;

			Render::Engine::RectFilled( ElementPos, ElementSize, Color::Palette_t::FormColorLight( ) );
			if( *ctx->DropdownInfo.Option == i )
				Render::Engine::RectFilled( ElementPos, ElementSize, Color( 0, 0, 0, 30 ) );
			else if( bHover )
				Render::Engine::RectFilled( ElementPos, ElementSize, Color( 0, 0, 0, 15 ) );

			if( ctx->DropdownInfo.Elements[ i ].length( ) > 26 ) {
				ctx->DropdownInfo.Elements[ i ].resize( 26 );
				ctx->DropdownInfo.Elements[ i ] += XorStr( "..." );
			}

			auto str = ctx->DropdownInfo.Elements[ i ];
			std::transform( str.begin( ), str.end( ), str.begin( ), ::tolower );
			Render::Engine::menu_small.string( ElementPos.x + ElementSize.x / 2, ElementPos.y + 4,
											   *ctx->DropdownInfo.Option == i ? g_Vars.menu.ascent.ToRegularColor( ) : ( bHover ? Color( 125, 125, 125 ) : Color( 75, 75, 75 ) ),
											   str, Render::Engine::ALIGN_CENTER );

			if( bHover && InputHelper::Down( VK_LBUTTON ) && !bOutOfBounds ) {
				*ctx->DropdownInfo.Option = i;
				ctx->DropdownInfo.HashedID = ctx->FocusedID = 0;
				ctx->DropdownInfo.DraggingScroll = false;
				GUI::ctx->HotkeyInfo.HashedID = 0;
			}

			if( bNeedsScroll )
				Render::Engine::RectFilled(
					ScrollbarPos,
					ScrollbarSize,
					Color::Palette_t::ElementOutlines( ) );
		}
		Render::Engine::ResetClip( );

		Render::Engine::Rect(
			DrawPos,
			Vector2D( ctx->DropdownInfo.Size, ( 19 * nElementsCount ) ),
			Color::Black( ) );

		if( InputHelper::Pressed( VK_LBUTTON ) && !bHoveredMain ) {
			ctx->DropdownInfo.HashedID = ctx->FocusedID = 0;
			ctx->DropdownInfo.DraggingScroll = false;
			GUI::ctx->HotkeyInfo.HashedID = 0;
		}
	}

	if( ctx->MultiDropdownInfo.HashedID != 0 ) {
		Vector2D DrawPos = ctx->MultiDropdownInfo.Pos;
		size_t id = ctx->MultiDropdownInfo.HashedID;

		if( ctx->MultiDropdownInfo.uScroll.find( id ) == ctx->MultiDropdownInfo.uScroll.end( ) ) {
			ctx->MultiDropdownInfo.uScroll.insert( { id, 0.f } );
		}

		const int nElementsCount = std::clamp<int>( ctx->MultiDropdownInfo.Elements.size( ), 0, ctx->MultiDropdownInfo.MaxItems );
		const bool bNeedsScroll = ctx->MultiDropdownInfo.Elements.size( ) > ctx->MultiDropdownInfo.MaxItems;

		const int nMaxHeight = 19 * nElementsCount;
		const int nActualHeight = 19 * ctx->MultiDropdownInfo.Elements.size( );

		const bool bHoveredMain = g_InputSystem.IsInBox( DrawPos, Vector2D( ctx->MultiDropdownInfo.Size, nMaxHeight ) );

		// don't ask.............
		Vector2D ScrollbarPos = DrawPos + Vector2D( 2, 2 ) + Vector2D(
			ctx->MultiDropdownInfo.Size - 6,
			( ( float )ctx->MultiDropdownInfo.uScroll[ id ] / ( float )nActualHeight ) * ( float )nMaxHeight );

		Vector2D ScrollbarSize = Vector2D( 3, ( ( float )nMaxHeight / ( float )nActualHeight ) * nMaxHeight );

		const bool bHoveredScroll = g_InputSystem.IsInBox( ScrollbarPos, ScrollbarSize );

		if( bNeedsScroll ) {
			//FIXME if you are in the middle of a scroll (drag or not)
			// and you suddenly unhover then it will stop, and if you hover
			// back it will continue: this is rlly easy to fix, but im just tired
			if( bHoveredMain || bHoveredScroll || ctx->MultiDropdownInfo.DraggingScroll ) {
				static float prev_mouse = 0.f;

				const auto diff = prev_mouse - g_InputSystem.GetMousePosition( ).y;
				const auto scale = [ ] ( int in, int bmin, int bmax, int lmin, int lmax ) {
					return float( ( lmax - lmin ) * ( in - bmin ) ) / float( bmax - bmin ) + lmin;
				};

				// Ayy, I'm runnin' to the money, you know how I'm comin'
				// Monday 'til Sunday night, be thumbin', thumbin', thumbin'
				// (...)
				// I told her throw that ass back so I can bust it like a bubble
				// South Memphis nigga in this bitch, yeah, you know you in trouble
				// Ain't nuthin but a P thang, baby
				// Young iced - out nigga going crazy
				if( !ctx->MultiDropdownInfo.DraggingScroll && InputHelper::Down( VK_LBUTTON ) && bHoveredScroll ) {
					ctx->MultiDropdownInfo.DraggingScroll = true;
				}
				else if( ctx->MultiDropdownInfo.DraggingScroll ) {
					if( InputHelper::Down( VK_LBUTTON ) ) {
						ctx->MultiDropdownInfo.uScroll[ id ] -= scale( diff, 0, nActualHeight - ScrollbarSize.y, 0, nActualHeight - nMaxHeight );//FIXME it works fine on large sliders, but on bigger ones it sucks
					}
					else {
						prev_mouse = 0.f;
						ctx->MultiDropdownInfo.DraggingScroll = false;
					}
				}

				prev_mouse = g_InputSystem.GetMousePosition( ).y;

				if( !ctx->MultiDropdownInfo.DraggingScroll )
					ctx->MultiDropdownInfo.uScroll[ id ] = ctx->MultiDropdownInfo.uScroll[ id ] + 10 * ( -g_InputSystem.GetScrollMouse( ) );
			}

			ctx->MultiDropdownInfo.uScroll[ id ] = std::clamp<float>( ctx->MultiDropdownInfo.uScroll[ id ], 0, nActualHeight - nMaxHeight );
		}
		else if( !bHoveredScroll ) {
			ctx->MultiDropdownInfo.DraggingScroll = false;
		}

		Render::Engine::SetClip( DrawPos, Vector2D( ctx->MultiDropdownInfo.Size, ( nMaxHeight ) ) );
		for( int i = 0; i < ctx->MultiDropdownInfo.Elements.size( ); i++ ) {
			Vector2D ElementPos = DrawPos + Vector2D( 0, 19 * i ) - Vector2D( 0, ctx->MultiDropdownInfo.uScroll[ id ] );;
			Vector2D ElementSize = Vector2D( ctx->MultiDropdownInfo.Size, 19 );
			size_t hash = id + GUI::Hash( std::to_string( i ) );

			bool bHover = g_InputSystem.IsInBox( ElementPos, ElementSize );

			bool bOutOfBounds = ElementPos.y > DrawPos.y + ( 19 * ( nElementsCount - 1 ) ) || ElementPos.y < DrawPos.y;

			if( bHover ) {
				if( InputHelper::Pressed( VK_LBUTTON ) && !bOutOfBounds ) {
					*ctx->MultiDropdownInfo.Elements.at( i ).value ^= 1;
					ctx->MultiDropdownInfo.DraggingScroll = false;
				}
			}

			if( ctx->MultiDropdownInfo.Elements[ i ].name.length( ) > 26 ) {
				ctx->MultiDropdownInfo.Elements[ i ].name.resize( 26 );
				ctx->MultiDropdownInfo.Elements[ i ].name += XorStr( "..." );
			}

			Render::Engine::RectFilled( ElementPos, ElementSize, Color::Palette_t::FormColorLight( ) );
			if( *ctx->MultiDropdownInfo.Elements.at( i ).value )
				Render::Engine::RectFilled( ElementPos, ElementSize, Color( 0, 0, 0, 30 ) );
			else if( bHover )
				Render::Engine::RectFilled( ElementPos, ElementSize, Color( 0, 0, 0, 15 ) );

			auto str = ctx->MultiDropdownInfo.Elements.at( i ).name;
			std::transform( str.begin( ), str.end( ), str.begin( ), ::tolower );
			Render::Engine::menu_small.string( ElementPos.x + ElementSize.x / 2, ElementPos.y + 4,
											   *ctx->MultiDropdownInfo.Elements.at( i ).value ? g_Vars.menu.ascent.ToRegularColor( ) : ( bHover ? Color( 125, 125, 125 ) : Color( 75, 75, 75 ) ),
											   str, Render::Engine::ALIGN_CENTER );

			if( bNeedsScroll )
				Render::Engine::RectFilled(
					ScrollbarPos,
					ScrollbarSize,
					Color::Palette_t::ElementOutlines( ) );
		}
		Render::Engine::ResetClip( );

		Render::Engine::Rect(
			DrawPos,
			Vector2D( ctx->MultiDropdownInfo.Size, ( nMaxHeight ) ),
			Color::Black( ) );



		if( InputHelper::Pressed( VK_LBUTTON ) ) {
			if( !g_InputSystem.IsInBox( DrawPos, Vector2D( ctx->MultiDropdownInfo.Size, ( 19 * ctx->MultiDropdownInfo.Elements.size( ) ) ) ) )
				ctx->MultiDropdownInfo.HashedID = ctx->FocusedID = 0;
			ctx->MultiDropdownInfo.DraggingScroll = false;
		}
	}

	ctx->Tabs.clear( );

	if( !ctx->SubTabs.empty( ) ) {
		if( !ctx->SubTabs.at( ctx->ActiveTab ).empty( ) ) {
			ctx->SubTabs.at( ctx->ActiveTab ).clear( );
		}

		ctx->SubTabs.clear( );
	}

	if( ctx->ConfigInfo.flAnimation[ ctx->ConfigInfo.HashedID ] > 0.f ) {
		size_t id = ctx->ConfigInfo.HashedID;

		Vector2D vecPos = ctx->ConfigInfo.vecPos;

		vecPos.x = int( vecPos.x );
		vecPos.y = int( vecPos.y );

		float flBackupMultiplier = g_pSurface->DrawGetAlphaMultiplier( );

		g_pSurface->DrawSetAlphaMultiplier( flBackupMultiplier * ctx->ConfigInfo.flAnimation[ ctx->ConfigInfo.HashedID ] );

		vecPos.x -= 1;

		Vector2D vecMainSize = Vector2D( 120, 40 );

		static std::unordered_map<size_t, float> flButtonHover;

		if( flButtonHover.find( id ) == flButtonHover.end( ) ) {
			flButtonHover.insert( { id + 1, 0.f } );
			flButtonHover.insert( { id + 2, 0.f } );
		}

		// cringe "dropshadow"XD
		for( int n = 1; n < 5; ++n ) {
			Render::Engine::RoundedRect( vecPos - n, vecMainSize + ( n * 2 ), 3.f, Color::Black( ).OverrideAlpha( ( 150 * float( ( 5 - n ) / 5.f ) ) ) );
		}

		Render::Engine::FilledRoundedRect( vecPos, vecMainSize, 3.f, Color::Palette_t::FormColor( ).OverrideAlpha( 255 ) );
		Render::Engine::RoundedRect( vecPos, vecMainSize, 3.f, Color::Palette_t::ElementOutlines( ).OverrideAlpha( 255 ) );

		Vector2D vecButtonSize = Vector2D( int( ( vecMainSize.x - 30 ) / 2 ), 20 );

		Vector2D vecDrawPos;

		// copy
		vecDrawPos = vecPos + Vector2D( int( vecMainSize.x / 2 - vecButtonSize.x - 5 ), vecMainSize.y - 30 );
		bool bCopyHovered = g_InputSystem.IsInBox( vecDrawPos, vecButtonSize );

		flButtonHover[ id + 1 ] = GUI::Approach( flButtonHover[ id + 1 ], bCopyHovered || ctx->ConfigInfo.bCopyingFiggy ? 1.f : 0.f, g_pGlobalVars->frametime * 16.f );

		Render::Engine::GradientRoundedVertical( vecDrawPos + 2, vecButtonSize - 4, 3.f, CORNER_ALL,
												 Color::Blend( Color::Palette_t::FormColor( ), Color::Palette_t::FormColor( ) * 1.7f, flButtonHover[ id + 1 ] ),
												 Color::Blend( Color::Palette_t::FormColor( ) * 0.74f, ( Color::Palette_t::FormColor( ) * 0.74f ) * 1.7f, ( flButtonHover[ id + 1 ] ) ) );

		Render::Engine::RoundedRect( vecDrawPos, vecButtonSize, 3.f, Color::Black( ) );
		Render::Engine::RoundedRect( vecDrawPos + 1, vecButtonSize - 2, 3.f, Color::Palette_t::ElementOutlines( ) );

		Render::Engine::menu_regular.string( vecDrawPos.x + vecButtonSize.x / 2, vecDrawPos.y + vecButtonSize.y / 2 - ( Render::Engine::menu_regular.size( XorStr( "Copy" ) ).m_height / 2 ) - 1,
											 ctx->ConfigInfo.bCopyingFiggy ? g_Vars.menu.ascent.ToRegularColor( ) : Color::Blend( Color( 120, 120, 120 ), Color( 220, 220, 220 ), flButtonHover[ id + 1 ] ), XorStr( "Copy" ), Render::Engine::ALIGN_CENTER );

		// paste
		vecDrawPos = vecPos + Vector2D( int( vecMainSize.x / 2 + 5 ), vecMainSize.y - 30 );
		bool bPasteHovered = g_InputSystem.IsInBox( vecDrawPos, vecButtonSize );

		flButtonHover[ id + 2 ] = GUI::Approach( flButtonHover[ id + 2 ], bPasteHovered || ctx->ConfigInfo.bPastingFiggy ? 1.f : 0.f, g_pGlobalVars->frametime * 16.f );

		Render::Engine::GradientRoundedVertical( vecDrawPos + 2, vecButtonSize - 4, 3.f, CORNER_ALL,
												 Color::Blend( Color::Palette_t::FormColor( ), Color::Palette_t::FormColor( ) * 1.7f, flButtonHover[ id + 2 ] ),
												 Color::Blend( Color::Palette_t::FormColor( ) * 0.74f, ( Color::Palette_t::FormColor( ) * 0.74f ) * 1.7f, ( flButtonHover[ id + 2 ] ) ) );

		Render::Engine::RoundedRect( vecDrawPos, vecButtonSize, 3.f, Color::Black( ) );
		Render::Engine::RoundedRect( vecDrawPos + 1, vecButtonSize - 2, 3.f, Color::Palette_t::ElementOutlines( ) );

		Render::Engine::menu_regular.string( vecDrawPos.x + vecButtonSize.x / 2, vecDrawPos.y + vecButtonSize.y / 2 - ( Render::Engine::menu_regular.size( XorStr( "Paste" ) ).m_height / 2 ) - 1,
											 ctx->ConfigInfo.bPastingFiggy ? g_Vars.menu.ascent.ToRegularColor( ) : Color::Blend( Color( 120, 120, 120 ), Color( 220, 220, 220 ), flButtonHover[ id + 2 ] ), XorStr( "Paste" ), Render::Engine::ALIGN_CENTER );

		if( bCopyHovered || bPasteHovered ) {
			ctx->override_cursor = LoadCursor( NULL, IDC_HAND );
		}

		if( !ctx->ConfigInfo.bCopyingFiggy && !ctx->ConfigInfo.bPastingFiggy ) {
			if( bCopyHovered && InputHelper::Pressed( VK_LBUTTON ) ) {
				ctx->ConfigInfo.bCopyingFiggy = true;
			}
		}
		else if( ctx->ConfigInfo.bCopyingFiggy ) {
			if( InputHelper::Released( VK_LBUTTON ) ) {
				if( bCopyHovered ) {
					ctx->ConfigInfo.fnCopy( );
				}

				ctx->ConfigInfo.bCopyingFiggy = false;
			}
		}

		if( !ctx->ConfigInfo.bCopyingFiggy && !ctx->ConfigInfo.bPastingFiggy ) {
			if( bPasteHovered && InputHelper::Pressed( VK_LBUTTON ) ) {
				ctx->ConfigInfo.bPastingFiggy = true;
			}
		}
		else if( ctx->ConfigInfo.bPastingFiggy ) {
			if( InputHelper::Released( VK_LBUTTON ) ) {
				if( bPasteHovered ) {
					ctx->ConfigInfo.fnPaste( );
				}

				ctx->ConfigInfo.bPastingFiggy = false;
			}
		}

		if( InputHelper::Pressed( VK_LBUTTON ) && !g_InputSystem.IsInBox( vecPos, vecMainSize ) ) {
			ctx->FocusedID = 0;
			ctx->ConfigInfo.bOpen = true;
		}

		g_pSurface->DrawSetAlphaMultiplier( flBackupMultiplier );
	}

	// render our custom cursor 
	/*if( !g_Vars.globals.hackUnload )
		if( ctx->override_cursor )
			SetCursor( ctx->override_cursor );
		else
			SetCursor( LoadCursor( NULL, IDC_ARROW ) );*/
}

bool GUI::Form::BeginTab( std::string name, std::string text2 ) {
	ctx->Tabs.push_back( { name, text2 } );

	for( int i = 0; i < ctx->Tabs.size( ); ++i ) {
		if( ctx->SubTabs.find( i ) == ctx->SubTabs.end( ) ) {
			ctx->SubTabs.insert( { i, { } } );
			ctx->ActiveSubTab.insert( { i, { } } );
		}
	}

	ctx->CurrentTab = name;
	return ( ctx->ActiveTab == ctx->Tabs.size( ) - 1 ) || ctx->setup;
}

bool GUI::Form::BeginSubTab( std::string name ) {
	if( !ctx->SubTabs.size( ) )
		return false;

	ctx->SubTabs.at( ctx->ActiveTab ).push_back( name );

	ctx->CurrentSubTab = name;
	return ( ctx->ActiveSubTab.at( ctx->ActiveTab ) == ctx->SubTabs.at( ctx->ActiveTab ).size( ) - 1 ) || ctx->setup;
}

void GUI::Form::AddPopup( std::string name, std::string innerString, bool bHasConfirmButton, std::function<void( )> fnOnConfirm ) {
	ctx->PopupInfo.HashedID = GUI::Hash( name + ctx->parent );

	ctx->PopupInfo.bOpen = true;

	ctx->PopupInfo.szTitle = std::move( name );
	ctx->PopupInfo.szInnerString = std::move( innerString );
	ctx->PopupInfo.fnFunction = std::move( fnOnConfirm );
	ctx->PopupInfo.bHasConfirmButton = bHasConfirmButton;

	ctx->PopupInfo.vecCenterPosition = ctx->pos + ( ctx->size / 2 );
}

// 10, 10, 10
Color Color::Palette_t::FormColor( ) {
	return Color( 17, 17, 17 );
}

// 18, 18, 18
Color Color::Palette_t::FormColorDark( ) {
	return Color( 25, 25, 25 );
}

// 26, 26, 26
Color Color::Palette_t::ElementOutlines( ) {
	return Color( 32, 32, 32 );
}

// 16, 16, 16
Color Color::Palette_t::FormColorLight( ) {
	return Color( 24, 24, 24 );
}

// 228, 228, 242
Color Color::Palette_t::MenuWhite( ) {
	return Color( 225, 225, 225 );
}

// 73, 73, 78
Color Color::Palette_t::MenuGrey( ) {
	return Color( 86, 86, 86 );
}