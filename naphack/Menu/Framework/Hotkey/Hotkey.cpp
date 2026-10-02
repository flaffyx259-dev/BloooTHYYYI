#include "../GUI.h"
#include "../../../pandora.hpp"
#include "../../../Features/Scripting/Scripting.hpp"

std::string GetHotkeyName( int key ) {
	auto Code = MapVirtualKeyA( key, MAPVK_VK_TO_VSC );

	int Result;
	char Buffer[ 128 ];

	switch( key ) {
		case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN:
		case VK_RCONTROL: case VK_RMENU:
		case VK_LWIN: case VK_RWIN: case VK_APPS:
		case VK_PRIOR: case VK_NEXT:
		case VK_END: case VK_HOME:
		case VK_INSERT: case VK_DELETE:
		case VK_DIVIDE:
		case VK_NUMLOCK:
			Code |= KF_EXTENDED;
		default:
			Result = GetKeyNameTextA( Code << 16, Buffer, 128 );
	}

	if( Result == 0 ) {
		switch( key ) {
			case VK_XBUTTON1:
				return XorStr( "[M4]" );
			case VK_XBUTTON2:
				return XorStr( "[M5]" );
			case VK_LBUTTON:
				return XorStr( "[M1]" );
			case VK_MBUTTON:
				return XorStr( "[M3]" );
			case VK_RBUTTON:
				return XorStr( "[M2]" );
			default:
				return XorStr( "[NONE]" );
		}
	}

	auto transformer = std::string( Buffer );
	std::transform( transformer.begin( ), transformer.end( ), transformer.begin( ), ::toupper );

	char finalBuff[ 128 ] = {};
	sprintf( finalBuff, XorStr( "[%s]" ), transformer.data( ) );

	return finalBuff;
};

void GUI::Controls::Hotkey( std::string name, hotkey_t *hotkey, bool bSeparateElement ) {
	std::transform( name.begin( ), name.end( ), name.begin( ), ::tolower );

	std::string szStringBeforeID = GUI::SplitStr( name, '#' )[ 0 ];
	size_t id = GUI::Hash( name );

	std::string szHotkeyText = GetHotkeyName( hotkey->key );

	if( ctx->FocusedID == id && ctx->HotkeyInfo.bLeftClicked )
		szHotkeyText = XorStr( "[...]" );

	// stick to last control if name is empty
	Vector2D CursorPos = bSeparateElement ? PopCursorPos( ) : GetLastCursorPos( );
	Vector2D DrawPos = ctx->pos + CursorPos;
	Vector2D DrawSize = Vector2D( Render::Engine::menu_bold.size( szHotkeyText.data( ) ).m_width, Render::Engine::menu_bold.size( szHotkeyText.data( ) ).m_height );

	DrawPos.x += ctx->ParentSize.x - 25 - DrawSize.x;

	const float flBackupMultiplier = g_pSurface->DrawGetAlphaMultiplier( );

	if( !ctx->enabled )
		g_pSurface->DrawSetAlphaMultiplier( flBackupMultiplier * 0.3f );

	bool bHovered = ctx->bActiveWindow && ctx->enabled && g_InputSystem.IsInBox( DrawPos, DrawSize ) && g_InputSystem.IsInBox( ctx->ParentPos, ctx->ParentSize );

	if( !szStringBeforeID.empty( ) && bSeparateElement ) {
		DrawPos.x -= ctx->ParentSize.x - 25 - DrawSize.x;

		if( ctx->enabled && g_InputSystem.IsInBox( DrawPos, Vector2D( Render::Engine::menu_regular.size( szStringBeforeID ).m_width, Render::Engine::menu_regular.size( szStringBeforeID ).m_height ) ) ) {
			bHovered = ctx->bActiveWindow && ctx->enabled && ctx->FocusedID == 0 || ctx->FocusedID == GUI::Hash( name );
		}

		Render::Engine::menu_regular.string( DrawPos.x, DrawPos.y - 1,
											 ( ( ctx->FocusedID == GUI::Hash( name ) || ctx->FocusedID == GUI::Hash( name ) + 69 ) && ctx->enabled ) ? Color( 150, 150, 150 ) :
											 bHovered ? Color( 100, 100, 100 ) : Color( 75, 75, 75 ),
											 szStringBeforeID.data( ) );

		DrawPos.x += ctx->ParentSize.x - 25 - DrawSize.x;
	}

	Render::Engine::menu_bold.string( DrawPos.x, DrawPos.y, ( ( ctx->FocusedID == GUI::Hash( name ) || ctx->FocusedID == GUI::Hash( name ) + 69 ) && ctx->enabled ) ? g_Vars.menu.ascent.ToRegularColor( ) :
									  bHovered ? Color( 100, 100, 100 ) : Color( 75, 75, 75 ), szHotkeyText );
	static Vector2D vecLastCursorPos;
	if( ctx->FocusedID == 0 ) {
		vecLastCursorPos = g_InputSystem.GetMousePosition( );

		if( bHovered && InputHelper::Released( VK_LBUTTON ) ) {
			ctx->FocusedID = GUI::Hash( name );
			ctx->HotkeyInfo.bLeftClicked = true;
		}

		if( bHovered && InputHelper::Pressed( VK_RBUTTON ) ) {
			ctx->FocusedID = GUI::Hash( name );
			ctx->HotkeyInfo.bLeftClicked = false;

			ctx->DropdownInfo.Elements = {
			 XorStr( "Hold" ),
			 XorStr( "Toggle" ),
			 XorStr( "Always on" ) };

			ctx->DropdownInfo.Option = &hotkey->cond;
			ctx->DropdownInfo.Size = 100;
			ctx->DropdownInfo.MaxItems = 4;
			ctx->DropdownInfo.Pos = DrawPos + Vector2D( 0, 10 );
			ctx->DropdownInfo.HashedID = GUI::Hash( name );
		}

	}
	else if( ctx->FocusedID == GUI::Hash( name ) ) {
		if( ctx->HotkeyInfo.bLeftClicked ) {
			if( !bHovered && InputHelper::Pressed( VK_LBUTTON ) ) {
				ctx->FocusedID = 0;
				ctx->HotkeyInfo.bLeftClicked = false;
			}

			for( int i = 0; i < 255; i++ ) {
				if( InputHelper::Released( i ) ) {
					ctx->FocusedID = 0;
					ctx->HotkeyInfo.bLeftClicked = false;

					( *hotkey ).key = ( i );

					if( i == VK_ESCAPE ) {
						( *hotkey ).key = 0;
					}

					break;
				}
			}
		}
		else {
			ctx->HotkeyInfo.HashedID = GUI::Hash( name );
			ctx->HotkeyInfo.vecPos = vecLastCursorPos;
			ctx->HotkeyInfo.pHotkey = hotkey;
		}
	}

	if( !ctx->enabled )
		g_pSurface->DrawSetAlphaMultiplier( flBackupMultiplier );

	if( bSeparateElement )
		GUI::PushCursorPos( CursorPos + Vector2D( 0, Render::Engine::menu_regular.size( GUI::SplitStr( name, '#' )[ 0 ].data( ) ).m_height + GUI::ObjectPadding( ) ) );

#if defined(LUA_SCRIPTING)
	if( ctx->setup ) {
		auto item = new Scripting::hotkeyItem;
		item->hotkey = hotkey;

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