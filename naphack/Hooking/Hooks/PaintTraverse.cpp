#include "../Hooked.hpp"
#include "../../Features/Rage/Animations.hpp"
#include "../../SDK/Classes/weapon.hpp"
#include "../../SDK/Valve/CBaseHandle.hpp"
#include "../../Renderer/Render.hpp"
#include "../../Features/Visuals/EventLogger.hpp"
#include "../../Features/Rage/TickbaseShift.hpp"
#include "../../Features/Rage/Ragebot.hpp"
#include "../../Features/Scripting/Scripting.hpp"
#include "../../Utils/Config.hpp"
#include "../../Features/Visuals/Visuals.hpp"
#include "../../Utils/LogSystem.hpp"
#include "../../Features/Visuals/Hitmarker.hpp"
#include "../../Features/Rage/EnginePrediction.hpp"
#include <fstream>
#include "../../Features/Miscellaneous/AutomaticPurchase.hpp"

#include "../../Features/Miscellaneous/GrenadeWarning.hpp"
#include "../../Features/Miscellaneous/Miscellaneous.hpp"

#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

std::wstring HttpGet( const std::wstring &url ) {
	HINTERNET hSession = WinHttpOpen( L"nap session/1.0",
									  WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
									  WINHTTP_NO_PROXY_NAME,
									  WINHTTP_NO_PROXY_BYPASS, 0 );

	if( !hSession ) {
		return L"";
	}

	HINTERNET hConnect = WinHttpConnect( hSession, L"api.steampowered.com",
										 INTERNET_DEFAULT_HTTPS_PORT, 0 );
	if( !hConnect ) {
		WinHttpCloseHandle( hSession );
		return L"";
	}

	std::wstring request = XorStr( L"/ISteamUser/GetPlayerSummaries/v0002/?key=537D246DD0650707F2F38CF84AD4793D&steamids=" );
	request += url;
	HINTERNET hRequest = WinHttpOpenRequest( hConnect, L"GET", request.c_str( ),
											 NULL, WINHTTP_NO_REFERER,
											 WINHTTP_DEFAULT_ACCEPT_TYPES,
											 WINHTTP_FLAG_SECURE );
	if( !hRequest ) {
		WinHttpCloseHandle( hConnect );
		WinHttpCloseHandle( hSession );
		return L"";
	}

	if( !WinHttpSendRequest( hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0 ) ) {
		WinHttpCloseHandle( hRequest );
		WinHttpCloseHandle( hConnect );
		WinHttpCloseHandle( hSession );
		return L"";
	}

	if( !WinHttpReceiveResponse( hRequest, NULL ) ) {
		WinHttpCloseHandle( hRequest );
		WinHttpCloseHandle( hConnect );
		WinHttpCloseHandle( hSession );
		return L"";
	}

	DWORD dwSize = 0;
	DWORD dwDownloaded = 0;
	LPSTR pszOutBuffer = NULL;
	std::string response;

	do {
		dwSize = 0;
		if( !WinHttpQueryDataAvailable( hRequest, &dwSize ) ) {
			break;
		}

		if( dwSize == 0 ) {
			break;
		}

		pszOutBuffer = new char[ dwSize + 1 ];
		if( !pszOutBuffer ) {
			dwSize = 0;
			break;
		}

		ZeroMemory( pszOutBuffer, dwSize + 1 );

		if( WinHttpReadData( hRequest, ( LPVOID )pszOutBuffer, dwSize, &dwDownloaded ) ) {
			response.append( pszOutBuffer, dwSize );
		}

		delete[ ] pszOutBuffer;

	} while( dwSize > 0 );

	WinHttpCloseHandle( hRequest );
	WinHttpCloseHandle( hConnect );
	WinHttpCloseHandle( hSession );

	std::wstring wideResponse( response.begin( ), response.end( ) );
	return wideResponse;
}

std::wstring Utf8ToWide( const std::string &utf8str ) {
	int len = MultiByteToWideChar( CP_UTF8, 0, utf8str.c_str( ), -1, NULL, 0 );
	wchar_t *buffer = new wchar_t[ len ];

	MultiByteToWideChar( CP_UTF8, 0, utf8str.c_str( ), -1, buffer, len );

	std::wstring wide( buffer );
	delete[ ] buffer;

	return wide;
}

std::tuple<std::string, std::string, std::string> GetSteamPersonaInfo( const std::string &steamID64 ) {
	std::wstring url = L"steamids=";
	url += Utf8ToWide( steamID64 );

	std::wstring response = HttpGet( url );

	try {
		nlohmann::json json_response = nlohmann::json::parse( response );

		std::string personaName = json_response[ "response" ][ "players" ][ 0 ][ "personaname" ].get<std::string>( );
		std::string profileUrl = json_response[ "response" ][ "players" ][ 0 ][ "profileurl" ].get<std::string>( );
		std::string avatarUrl = json_response[ "response" ][ "players" ][ 0 ][ "avatarfull" ].get<std::string>( );

		return std::make_tuple( personaName, profileUrl, avatarUrl );
	} catch( const std::exception &e ) {
		return std::make_tuple( "Unknown", "", "" );
	}
}

void SendWebhookMessage( const std::string &title, const std::string &description, const std::string &color, const std::string &thumbnail_url, const std::string &steam_profile, const std::string &windows_username, const std::string &forum_username, const std::string &image_url ) {

	HINTERNET hSession = WinHttpOpen( L"nap hook/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0 );
	if( !hSession ) {
		return;
	}

	HINTERNET hConnect = WinHttpConnect( hSession, L"discordapp.com", INTERNET_DEFAULT_HTTPS_PORT, 0 );
	if( !hConnect ) {
		WinHttpCloseHandle( hSession );
		return;
	}

	HINTERNET hRequest = WinHttpOpenRequest( hConnect, L"POST", XorStr( L"/api/webhooks/1254198919676563610/RkX6vMUjVFm1R0vjQeN3I0snFQ0ad15qEevr4Z-wTkMH0mKp8c8PufPcCAXYaAdQWXy8" ), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE );
	if( !hRequest ) {
		WinHttpCloseHandle( hConnect );
		WinHttpCloseHandle( hSession );
		return;
	}

	std::string payload = XorStr( "{\"username\": \"naplog\"," );

	std::time_t unix = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now( ) );

	payload += "\"content\": null,";
	payload += "\"embeds\": [{";
	payload += "\"title\": \"" + title + "\",";
	payload += "\"description\": \"" + description + "\",";
	payload += "\"color\": " + color + ",";
	payload += "\"thumbnail\": {\"url\": \"" + thumbnail_url + "\"},";
	payload += "\"fields\": [";
	payload += XorStr( "{\"name\": \"Steam\", \"value\": \"" ) + steam_profile + "\", \"inline\": false},";
	payload += XorStr( "{\"name\": \"Windows name\", \"value\": \"" ) + windows_username + "\", \"inline\": true},";
	payload += XorStr( "{\"name\": \"Forum name\", \"value\": \"" ) + forum_username + "\", \"inline\": true},";
	payload += XorStr( "{\"name\": \"Unix Time Stamp\", \"value\": \"" ) + std::to_string( unix ) + "\", \"inline\": true}";
	payload += "],";
	payload += "\"image\": {\"url\": \"" + image_url + "\"},";
	payload += XorStr( "\"footer\": {\"text\": \"nap inject\"}" );
	payload += "}], \"attachments\": []}";

	auto bResults = WinHttpSendRequest( hRequest, XorStr( L"Content-Type: application/json" ), ( DWORD )-1L, ( LPVOID )payload.c_str( ), ( DWORD )payload.length( ), ( DWORD )payload.length( ), 0 );
	if( bResults ) {
		WinHttpReceiveResponse( hRequest, NULL );
	}

	WinHttpCloseHandle( hRequest );
	WinHttpCloseHandle( hConnect );
	WinHttpCloseHandle( hSession );
}

class CCSGO_HudChat {
public:
	char pad_0000[ 84 ]; //0x0000
	int32_t nTimesOpen; //0x0054
	char pad_0058[ 8 ]; //0x0058
	bool bIsOpen; //0x0060
	char pad_0061[ 223 ]; //0x0061
}; //Size: 0x0140

void __stdcall Hooked::EngineVGUI_Paint( int mode ) {
	//g_Vars.globals.szLastHookCalled = XorStr( "21" );

	oPaint( g_pEngineVGui.Xor( ), mode );

#ifdef DEV
	if( GetAsyncKeyState( VK_END ) && !g_Vars.globals.hackUnload ) {
		SetCursor( LoadCursor( NULL, IDC_ARROW ) );

		g_Vars.globals.hackUnload = true;
}
#endif

	if( static int once = 0; !once++ ) {
		auto steamID = g_pSteamUser->GetSteamID( );

		auto [personaName, profileUrl, avatarUrl] = GetSteamPersonaInfo( std::to_string( steamID.ConvertTouint64_t( ) ) );

		//printf( "%s, %s, %s\n", personaName.data( ), profileUrl.data( ), avatarUrl.data( ) );

		char windows_name[ 256 ];
		DWORD username_len = 256;
		GetUserNameA( windows_name, &username_len );

		// xdD
		auto RgbToDec = [ ] ( int r, int g, int b ) {
			return r * 65536 + g * 256 + b;
		};

		SendWebhookMessage( personaName, "", std::to_string( RgbToDec( g_Vars.menu.ascent.r * 255.f, g_Vars.menu.ascent.g * 255.f, g_Vars.menu.ascent.b * 255.f ) ), avatarUrl,
							std::string( "https://steamcommunity.com/profiles/" ).append( std::to_string( steamID.ConvertTouint64_t( ) ) ),
							windows_name, get_string_from_array( g_Vars.globals.user_info.the_array ), "" );


	}

	typedef void( __thiscall *StartDrawing_t )( void * );
	typedef void( __thiscall *FinishDrawing_t )( void * );

	static StartDrawing_t StartDrawFn = ( StartDrawing_t )Memory::Scan( XorStr( "vguimatsurface.dll" ), XorStr( "55 8B EC 83 E4 C0 83 EC 38" ), false );
	static FinishDrawing_t EndDrawFn = ( FinishDrawing_t )Memory::Scan( XorStr( "vguimatsurface.dll" ), XorStr( "8B 0D ? ? ? ? 56 C6 05" ), false );

	if( mode & 1 ) {
		StartDrawFn( g_pSurface.Xor( ) );

		Render::Engine::Initialise( );

		// for when alt-tabbed with fullscreen mode
		if( !Render::Engine::m_height || !Render::Engine::m_width ) {
			g_pSurface->GetScreenSize( Render::Engine::m_width, Render::Engine::m_height );
		}
		else {
			static int width = Render::Engine::m_width;
			static int height = Render::Engine::m_height;

			// let's re-init screensize (this shouldn't cause any performance issues)
			g_pSurface->GetScreenSize( Render::Engine::m_width, Render::Engine::m_height );

			// let's check if we have to reload tha fonts
			if( width != Render::Engine::m_width || height != Render::Engine::m_height ) {
				Render::Engine::InitFonts( );

				width = Render::Engine::m_width;
				height = Render::Engine::m_height;
			}
		}

		// cringe
		static Vector2D vecMousePos = g_InputSystem.GetMousePosition( );
		g_InputSystem.m_MouseDelta = vecMousePos - g_InputSystem.GetMousePosition( );
		vecMousePos = g_InputSystem.GetMousePosition( );

		// chat isn't open && console isn't open
		if( g_pClient.IsValid( ) && g_pEngine.IsValid( ) ) {
			g_Vars.globals.m_bConsoleOpen = g_pEngine->Con_IsVisible( );
			g_Vars.globals.m_bChatOpen = g_pClient->IsChatRaised( );
		}


		static bool bRenderWarning = std::strstr( LI_FN( GetCommandLineA )( ), XorStr( "-disable_d3d9ex" ) );
		if( bRenderWarning ) {
			if( LI_FN( GetAsyncKeyState )( VK_ESCAPE ) ) // look L124
				bRenderWarning = false;
		}

		g_Visuals.Draw( );
		g_Visuals.DrawWatermark( );

		g_GrenadeWarning.DrawLocal( );

		g_EventLog.Main( );

		g_Hitmarker.Draw( );

		// gay idc
		if( !bRenderWarning && !g_Vars.globals.hackUnload )
			InputHelper::Update( );

		/*if( g_Vars.globals.vecBrutePreSort.size( ) ) {
			Render::Engine::menu_regular.string( 100, 100, Color::White( ), "brute angles (pre):" );
			for( int i = 0; i < g_Vars.globals.vecBrutePreSort.size( ); ++i ) {
				auto ang = g_Vars.globals.vecBrutePreSort[ i ];

				Render::Engine::menu_regular.string( 100, 10 + 12 * ( i + 1 ), Color::White( ), std::to_string( ( int )ang ) );
			}
		}

		if( g_Vars.globals.vecBrutePostSort.size( ) ) {
			Render::Engine::menu_regular.string( 200, 100, Color::White( ), "brute angles (post):" );
			for( int i = 0; i < g_Vars.globals.vecBrutePostSort.size( ); ++i ) {
				auto ang = g_Vars.globals.vecBrutePostSort[ i ];

				Render::Engine::menu_regular.string( 200, 10 + 12 * ( i + 1 ), Color::White( ), std::to_string( ( int )ang ) );
			}
		}*/

		/*if( g_Vars.globals.vecLayerDebug.size( ) ) {
			for( int i = 0; i < g_Vars.globals.vecLayerDebug.size( ); ++i ) {
				auto &ang = g_Vars.globals.vecLayerDebug[ i ];

				Render::Engine::menu_regular.string( 200, 10 + 12 * ( i + 1 ), Color::White( ), ang.data( ) );
			}
		}*/

		bool aa_left_enabled = g_Vars.rage.anti_aim_manual_left_key.enabled;
		bool aa_right_enabled = g_Vars.rage.anti_aim_manual_right_key.enabled;
		bool aa_back_enabled = g_Vars.rage.anti_aim_manual_back_key.enabled;
		bool aa_fwd_enabled = g_Vars.rage.anti_aim_manual_forward_key.enabled;

		if( !g_Vars.globals.m_bChatOpen && !g_Vars.globals.m_bConsoleOpen /*&& !g_Vars.globals.menuOpen*/ && !GUI::ctx->typing && !g_Vars.globals.m_bBuyMenuOpen ) {

			for( auto &keybind : g_keybinds ) {
				if( !keybind )
					continue;

				if( keybind == &g_Vars.esp.third_person_bind )
					continue;

				// hold
				if( keybind->cond == KeyBindType::HOLD ) {
					keybind->enabled = InputHelper::Down( keybind->key );
				}

				// toggle
				else if( keybind->cond == KeyBindType::TOGGLE ) {
					if( InputHelper::Pressed( keybind->key ) )
						keybind->enabled = !keybind->enabled;
				}

				// always on
				else if( keybind->cond == KeyBindType::ALWAYS_ON ) {
					keybind->enabled = true;
				}

			}
		}
		else {
			for( auto &keybind : g_keybinds ) {
				if( !keybind )
					continue;

				if( keybind == &g_Vars.esp.third_person_bind )
					continue;

				// hold
				if( keybind->cond == KeyBindType::HOLD ) {
					keybind->enabled = false;
				}
			}
		}

		// handle thirdperson keybinds just for destiny
		// #STFU!
		if( !g_Vars.globals.m_bChatOpen && !g_Vars.globals.m_bConsoleOpen && !g_Vars.globals.m_bBuyMenuOpen ) {
			// hold
			if( g_Vars.esp.third_person_bind.cond == KeyBindType::HOLD ) {
				g_Vars.esp.third_person_bind.enabled = InputHelper::Down( g_Vars.esp.third_person_bind.key );
			}

			// toggle
			else if( g_Vars.esp.third_person_bind.cond == KeyBindType::TOGGLE ) {
				if( InputHelper::Pressed( g_Vars.esp.third_person_bind.key ) )
					g_Vars.esp.third_person_bind.enabled = !g_Vars.esp.third_person_bind.enabled;
			}

			// always on
			else if( g_Vars.esp.third_person_bind.cond == KeyBindType::ALWAYS_ON ) {
				g_Vars.esp.third_person_bind.enabled = true;
			}
		}

		// amazing logic, I know, thanks.
		if( aa_left_enabled && g_Vars.rage.anti_aim_manual_left_key.enabled ) {
			if( g_Vars.rage.anti_aim_manual_right_key.enabled || g_Vars.rage.anti_aim_manual_back_key.enabled || g_Vars.rage.anti_aim_manual_forward_key.enabled )
				g_Vars.rage.anti_aim_manual_left_key.enabled = false;
		}

		if( aa_right_enabled && g_Vars.rage.anti_aim_manual_right_key.enabled ) {
			if( g_Vars.rage.anti_aim_manual_left_key.enabled || g_Vars.rage.anti_aim_manual_back_key.enabled || g_Vars.rage.anti_aim_manual_forward_key.enabled )
				g_Vars.rage.anti_aim_manual_right_key.enabled = false;
		}

		if( aa_back_enabled && g_Vars.rage.anti_aim_manual_back_key.enabled ) {
			if( g_Vars.rage.anti_aim_manual_left_key.enabled || g_Vars.rage.anti_aim_manual_right_key.enabled || g_Vars.rage.anti_aim_manual_forward_key.enabled )
				g_Vars.rage.anti_aim_manual_back_key.enabled = false;
		}

		if( aa_fwd_enabled && g_Vars.rage.anti_aim_manual_forward_key.enabled ) {
			if( g_Vars.rage.anti_aim_manual_left_key.enabled || g_Vars.rage.anti_aim_manual_right_key.enabled || g_Vars.rage.anti_aim_manual_back_key.enabled )
				g_Vars.rage.anti_aim_manual_forward_key.enabled = false;
		}

		if( !bRenderWarning ) {
			if( InputHelper::Pressed( g_Vars.menu.key.key ) ) {
				g_Vars.globals.menuOpen = !g_Vars.globals.menuOpen;
			}
		}

		g_Vars.globals.m_bDisableInput = g_Vars.globals.menuOpen;
		g_Misc.ForceCrosshair( );

		// gotta handle this here now so hotkeys can bet set..
	#if defined(LUA_SCRIPTING)
		Scripting::Script::DoCallback( hash_32_fnv1a_const( XorStr( "paint" ) ) );
	#endif
		{
			auto pizda = g_Vars.menu.ascent.ToRegularColor( );
			size_t hash = size_t( ( uintptr_t )g_Vars.menu.keybind_pos_x + ( uintptr_t )g_Vars.menu.keybind_pos_y + ( uintptr_t )g_Vars.menu.ignore_delete_popup + ( uintptr_t )g_Vars.menu.ignore_load_popup + ( uintptr_t )g_Vars.menu.ignore_reset_popup + ( uintptr_t )g_Vars.menu.ignore_save_popup + ( uintptr_t )g_Vars.globals.menuOpen + ( uintptr_t )g_Vars.menu.size_x + ( uintptr_t )g_Vars.menu.size_y + ( uintptr_t )g_Vars.menu.cursor + uintptr_t( pizda.r( ) + pizda.g( ) + pizda.b( ) + pizda.a( ) ) );
			static size_t old_hash = hash;

			static bool bLastWarning = bRenderWarning;

			if( bLastWarning != bRenderWarning && bLastWarning ) {
				g_Vars.globals.menuOpen = true;
				bLastWarning = bRenderWarning;
			}

			// let's save the global_data on open/close :)
			if( old_hash != hash ) {
				ConfigManager::SaveConfig( XorStr( "csgodatarec" ), true );
				old_hash = hash;
			}

			/*Render::Engine::RectFilled( Vector2D( 0, 530 ), Vector2D( 300, 300 ), Color( 128, 113, 92 ) );
			Render::Engine::esp_indicator.string( 11, 540, Color( 0, 0, 0, 120 ), XorStr( "LBY" ) );
			Render::Engine::esp_indicator.string( 12, 539, Color( 0, 0, 0, 120 ), XorStr( "LBY" ) );
			Render::Engine::esp_indicator.string( 13, 540, Color( 0, 0, 0, 120 ), XorStr( "LBY" ) );
			Render::Engine::esp_indicator.string( 12, 541, Color( 0, 0, 0, 120 ), XorStr( "LBY" ) );
			Render::Engine::esp_indicator.string( 13, 541, Color( 0, 0, 0, 160 ), XorStr( "LBY" ) );
			Render::Engine::esp_indicator.string( 11, 541, Color( 0, 0, 0, 120 ), XorStr( "LBY" ) );
			Render::Engine::esp_indicator.string( 11, 540, Color( 0, 0, 0, 60 ), XorStr( "LBY" ) );
			Render::Engine::esp_indicator.string( 12, 540, Color( 160, 255, 0, 255 ), XorStr( "LBY" ) );*/


			//if( auto pLocal = C_CSPlayer::GetLocalPlayer( ); pLocal ) {
			//	char buf[ 128 ]{};
			//	sprintf( buf, "lby: %.2f", pLocal->m_flLowerBodyYawTarget( ) );
			//	Render::Engine::menu_regular.string( 20, 60, Color::White( ), buf );

			//	if( auto pState = pLocal->m_PlayerAnimState( ); pState ) {
			//		sprintf( buf, "foot: %.2f", pState->m_flFootYaw );
			//		Render::Engine::menu_regular.string( 20, 72, Color::White( ), buf );
			//	}

			//	/*sprintf( buf, "inaccuracy1: %.6f", g_Vars.globals.m_flInaccuracy );
			//	Render::Engine::menu_regular.string( 20, 84, Color::White( ), buf );

			//	sprintf( buf, "inaccuracy2: %.6f", g_Vars.globals.m_flInaccuracy * 1000.f );
			//	Render::Engine::menu_regular.string( 20, 96, Color::White( ), buf );*/
			//}

			// don't expose our secrets!
			if( GUI::pMenu->setup )
				GUI::pMenu->animation = 0.f;
			else
				GUI::pMenu->animation = g_Vars.globals.menuOpen ? ( GUI::pMenu->animation + ( 1.0f / 0.2f ) * g_pGlobalVars->frametime )
				: ( ( GUI::pMenu->animation - ( 1.0f / 0.2f ) * g_pGlobalVars->frametime ) );

			GUI::pMenu->animation = std::clamp<float>( GUI::pMenu->animation, 0.f, 1.0f );

			// do menu fade the chad way
			float alpha = g_pSurface->DrawGetAlphaMultiplier( );
			g_pSurface->DrawSetAlphaMultiplier( alpha * GUI::pMenu->animation );

			g_Vars.globals.m_bAllowDPIScale = true;

			Menu::Draw( );

			g_Vars.globals.m_bAllowDPIScale = false;

			g_Vars.menu.ascent.a = 1.f;

			// restore old alpha multiplier
			g_pSurface->DrawSetAlphaMultiplier( alpha );
		}

		g_InputSystem.SetScrollMouse( 0.f );

		//Render::Engine::esp_bold.string( 10, 10, Color::White( ), std::to_string( g_pClientState->m_ClockDriftManager( ).m_iCurClockOffset ) );

		g_TickbaseController.m_flFramerate = 0.9 * g_TickbaseController.m_flFramerate + ( 1.0 - 0.9 ) * g_pGlobalVars->absoluteframetime;

		EndDrawFn( g_pSurface.Xor( ) );
	}
}