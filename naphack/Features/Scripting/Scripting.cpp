#include "Scripting.hpp"
#include "Wrappers/render.h"
#include "Wrappers/interfaces.h"
#include "Wrappers/exploits.h"
#include "Wrappers/penetration.h"
#include "Wrappers/material.h"

#include "../Visuals/EventLogger.hpp"
#include "../Rage/AntiAim.hpp"
#include "../Rage/Ragebot.hpp"
#include "../Visuals/Visuals.hpp"
#include <filesystem>

#include <fstream>

namespace Scripting {
#if defined(LUA_SCRIPTING)
	bool initialized;
	std::vector<std::pair<std::string, std::shared_ptr<luaState_t>>> states;
	std::string path;

	void print_error( std::string test ) {
		// nice blue: Color_f( 0.f, 0.6f, 0.9f )
		g_EventLog.PushEvent( test, Color_f( 1.f, 0.f, 0.f ), true, XorStr( "LUA" ) );
	}

	void print( std::string test ) {
		g_EventLog.PushEvent( test, Color_f( 1.f, 1.f, 1.f ), false, XorStr( "LUA" ) );
	}

	void log( std::string text ) {
		g_EventLog.PushEvent( text, Color_f( 1.f, 1.f, 1.f ), true );
	}

	void log_color( std::string text, Color color ) {
		g_EventLog.PushEvent( text, Color_f( color.r( ), color.g( ), color.b( ), color.a( ) ), true );
	}

	void log_prefix_color( std::string text, Color color, std::string prefix = XorStr( "" ) ) {
		g_EventLog.PushEvent( text, Color_f( color.r( ), color.g( ), color.b( ), color.a( ) ), true, prefix );
	}

	void log_prefix_color_visible( std::string text, Color color, std::string prefix = XorStr( "" ), bool visible = true ) {
		g_EventLog.PushEvent( text, Color_f( color.r( ), color.g( ), color.b( ), color.a( ) ), visible, prefix );
	}

	uintptr_t pattern_scan( const std::string &module_name, const std::string &pattern, sol::this_state state ) {
		auto ret = Memory::Scan( module_name, pattern, false );
		if( !ret ) {
			luaL_error( state.lua_state( ), std::string( XorStr( "could not find signature '" ) ).append( pattern ).append( XorStr( "'" ) ).data( ) );
		}
		return ret;
	}

	uintptr_t create_interface( const std::string &module_name, const std::string &interface_version, sol::this_state state ) {
		auto image = GetModuleHandleA( module_name.data( ) );
		if( !image ) {
			luaL_error( state.lua_state( ), std::string( XorStr( "could not find module '" ) ).append( module_name ).append( XorStr( "' while creating interface" ) ).data( ) );
			return 0;
		}

		auto fn = ( CreateInterfaceFn )( GetProcAddress( image, XorStr( "CreateInterface" ) ) );
		if( !fn ) {
			luaL_error( state.lua_state( ), XorStr( "unexpected error while creating interface" ) );
			return 0;
		}

		if( auto result = fn( interface_version.data( ), nullptr ); result != nullptr ) {
			return ( uintptr_t )result;
		}

		luaL_error( state.lua_state( ), std::string( XorStr( "could not create interface '" ) ).append( interface_version ).append( XorStr( "'" ) ).data( ) );
		return 0;
	}

	std::string get_latency( ) {
		std::string szPing = XorStr( "0" );
		if( g_pEngine->IsConnected( ) ) {
			const auto netchannel = g_pEngine->GetNetChannelInfo( );
			if( netchannel ) {
				szPing = std::to_string( std::max( 0, ( int )std::round( netchannel->GetLatency( FLOW_OUTGOING ) * 1000.f ) ) );
			}
		}

		return szPing;
	}

	std::string get_time( const std::string &format ) {
		time_t t = std::time( nullptr );
		std::stringstream time_stream{ };
		time_stream << std::put_time( std::localtime( &t ), format.data( ) );

		return time_stream.str( );
	}

	int get_current_threat( ) {
		C_CSPlayer* pEntity = g_AntiAim.GetBestPlayer( );
		if( !pEntity )
			return 0;

		int pEntityIndex = pEntity->EntIndex( );
		if( !pEntityIndex )
			return 0;

		return pEntityIndex;
	}

	void set_quick_peek_assist_lock( bool value ) {
		g_Vars.globals.m_bLockAutoPeek = value;
	}

	__int64 unix_timestamp( ) {
		std::time_t t = std::time( 0 );
		return t;
	}

	bool match_hashes( uint32_t s1, uint32_t s2 ) {
		if( s1 == s2 )
			return true;

		return false;
	}

	bool client_is_alive( ) {
		const auto pLocal = C_CSPlayer::GetLocalPlayer( );
		if( !pLocal )
			return false;

		return !pLocal->IsDead( );
	}

	sol::object ui_get_group( sol::this_state state, std::string tab, /*std::string sub_tab,*/ std::string group, std::string name, std::string weapon_group ) {
		auto lua = sol::state_view( state.lua_state( ) );
		auto state_ptr = lua[ XorStr( "__lua_state" ) ].get<std::shared_ptr<luaState_t>>( );

		std::pair<std::string, uiItem *> item;
		int type = 0;
		auto someepicstring = tab.append( XorStr( "." ) )/*.append( sub_tab ).append( XorStr( "." ) )*/.append( group ).append( XorStr( "." ) ).append( name ).append( XorStr( "." ) ).append( weapon_group );
		std::transform( someepicstring.begin( ), someepicstring.end( ), someepicstring.begin( ), ::tolower );

		bool found = false;
		for( auto lol : gui_items ) {
			if( match_hashes( lol.first, hash_32_fnv1a( someepicstring.c_str( ) ) ) ) {
				item = lol;
				if( item.second ) {
					type = item.second->get_type( );
				}
				else {
					type = 1337;
				}

				found = true;
				break;
			}
		}

		if( found ) {
			switch( type ) {
				case Scripting::itemType_t::CHECKBOX:
				{
					return sol::make_object( state, ( Scripting::checkboxItem * )item.second );
				}
				case Scripting::itemType_t::DROPDOWN:
				{
					return sol::make_object( state, ( Scripting::dropdownItem * )item.second );
				}
				case Scripting::itemType_t::MULTIDROPDOWN:
				{
					return sol::make_object( state, ( Scripting::multidropdownItem * )item.second );
				}
				case Scripting::itemType_t::HOTKEY:
				{
					return sol::make_object( state, ( Scripting::hotkeyItem * )item.second );
				}
				case Scripting::itemType_t::COLORPICKER:
				{
					return sol::make_object( state, ( Scripting::colorpickerItem * )item.second );
				}
				case Scripting::itemType_t::SLIDER:
				{
					return sol::make_object( state, ( Scripting::sliderItem * )item.second );
				}
				case Scripting::itemType_t::SLIDER_FLOAT:
				{
					return sol::make_object( state, ( Scripting::sliderFloatItem * )item.second );
				}
			}
		}
		else {
			print_error( XorStr( "Couldn't find menu item " ) + someepicstring );
		}

		return sol::make_object( state, item );
	}

	sol::object ui_get( sol::this_state state, std::string tab, /*std::string sub_tab,*/ std::string group, std::string name ) {
		return ui_get_group( state, tab, /*sub_tab,*/ group, name, XorStr( "" ) );
	}

	sol::object ui_current_weap_config( sol::this_state state, std::string group, std::string name ) {
		auto lua = sol::state_view( state.lua_state( ) );
		auto state_ptr = lua[ XorStr( "__lua_state" ) ].get<std::shared_ptr<luaState_t>>( );

		bool bReturn = false;

		// so fucking ugly
		auto translateActiveToString = [ & ] ( ) -> std::string {
			const auto config = g_Ragebot.GetRageSettings( );
			if( !config ) {
				bReturn = true;

				return XorStr( "" );
			}

			std::string ret{ XorStr( "general" ) };
			if( config == &g_Vars.rage_autosnipers ) {
				ret = XorStr( "auto-sniper" );
			}
			else if( config == &g_Vars.rage_awp ) {
				ret = XorStr( "awp" );
			}
			else if( config == &g_Vars.rage_deagle ) {
				ret = XorStr( "deagle" );
			}
			else if( config == &g_Vars.rage_revolver ) {
				ret = XorStr( "revolver" );
			}
			else if( config == &g_Vars.rage_pistols ) {
				ret = XorStr( "pistols" );
			}
			else if( config == &g_Vars.rage_scout ) {
				ret = XorStr( "scout" );
			}
			else if( config == &g_Vars.rage_default ) {
				// just to make sure (:
				ret = XorStr( "general" );
			}

			return ret;
		};

		std::pair<std::string, uiItem *> item;
		int type = 0;
		auto someepicstring = std::string( XorStr( "Rage" ) ).append( XorStr( "." ) ).append( XorStr( "Ragebot" ) ).append( XorStr( "." ) ).append( group ).append( XorStr( "." ) ).append( name ).append( XorStr( "." ) ).append( translateActiveToString( ) );
		std::transform( someepicstring.begin( ), someepicstring.end( ), someepicstring.begin( ), ::tolower );

		bool found = false;
		for( auto lol : gui_items ) {
			if( match_hashes( lol.first, hash_32_fnv1a( someepicstring.c_str( ) ) ) ) {
				item = lol;
				if( item.second ) {
					type = item.second->get_type( );
				}
				else {
					type = 1337;
				}

				found = true;
				break;
			}
		}

		if( found ) {
			switch( type ) {
				case Scripting::itemType_t::CHECKBOX:
				{
					return sol::make_object( state, ( Scripting::checkboxItem * )item.second );
				}
				case Scripting::itemType_t::DROPDOWN:
				{
					return sol::make_object( state, ( Scripting::dropdownItem * )item.second );
				}
				case Scripting::itemType_t::MULTIDROPDOWN:
				{
					return sol::make_object( state, ( Scripting::multidropdownItem * )item.second );
				}
				case Scripting::itemType_t::HOTKEY:
				{
					return sol::make_object( state, ( Scripting::hotkeyItem * )item.second );
				}
				case Scripting::itemType_t::COLORPICKER:
				{
					return sol::make_object( state, ( Scripting::colorpickerItem * )item.second );
				}
				case Scripting::itemType_t::SLIDER:
				{
					return sol::make_object( state, ( Scripting::sliderItem * )item.second );
				}
				case Scripting::itemType_t::SLIDER_FLOAT:
				{
					return sol::make_object( state, ( Scripting::sliderFloatItem * )item.second );
				}
			}
		}
		else {
			print_error( XorStr( "Couldn't find menu item " ) + someepicstring );
		}

		return sol::make_object( state, item );
	}

	bool ui_is_open( ) {
		return g_Vars.globals.menuOpen;
	}

	bool aa_invert( ) {
		return false;// g_AntiAim.m_bInverted;
	}

	bool aa_run( CUserCmd *cmd, bool skip_fire = false ) {
		if( !cmd )
			return false;

		return true;// g_AntiAim.ShouldAntiAim( cmd, skip_fire );
	}

	QAngle camera_angles( ) {
		return g_Vars.globals.m_angCameraAngles;
	}

	Vector camera_position( ) {
		return g_Vars.globals.m_vecCameraPosition;
	}

	void force_cursor( bool state ) {
		g_Vars.globals.m_bDisableInput = state;
	}

	void add_esp_flag( const std::string &flag, Color color, int nEntIndex ) {
		nEntIndex = std::clamp<int>( nEntIndex, 0, 64 );

		//g_Visuals.m_arrEspFlags[ nEntIndex ].push_back( { flag, color } );
	}

	void set_thirdperson_animation( bool value ) {
		g_Vars.globals.m_bThirdpersonAnimation = value;
	}

	void set_fading_chams( bool value ) {
		g_Vars.globals.m_bFadeOutChams = value;
	}

	void set_visual_font( int index, Render::Engine::Font font ) {
		switch( index ) {
			// flags
			case 0:
				Render::Engine::esp_pixel = font;
				break;
			// weapon
			case 1:
				Render::Engine::esp_bold_wpn = font;
				g_Vars.globals.m_bChangedDefaultWpnFont = true;
				break;
			// name
			case 2:
				Render::Engine::esp_bold = font;
				g_Vars.globals.m_bChangedDefaultNameFont = true;
				break;
			// none
			default:
				break;
		}
	}

	void set_visual_case( int index, bool value ) {
		switch( index ) {
			// flags
			case 0:
				g_Vars.globals.m_bFlagsUppercase = value;
				break;
			// weapon
			case 1:
				g_Vars.globals.m_bWeaponUppercase = value;
				break;
			// none
			default:
				break;
		}
	}

	void set_flag_offset( int value ) {
		g_Vars.globals.m_nFlagsOffset = value;
	}

	void use_collision_bounding_boxes( bool value ) {
		g_Vars.globals.m_bUseCollisionBoundingBoxes = value;
	}

	void health_bar_string_left( bool value ) {
		g_Vars.globals.m_bHealthBarStringLeft = value;
	}

	std::string client_map_name( ) {
		if( !g_pClientState.IsValid( ) )
			return XorStr( "" );

		return reinterpret_cast< const char * >( ( uintptr_t( g_pClientState.Xor( ) ) + 0x28C ) );
	}

	bool load_script_file( const std::string &name ) {
		std::string gay = XorStr( "" );
		return Script::LoadScript( name, gay );
	}

	/*bool load_script_cloud( const std::string &name, int owner_id ) {
		g_Socket.m_bLoaded = false;

		std::string script_name = name;
		script_name.append( XorStr( "-cl_" ) ).append( std::to_string( owner_id ) );

		auto temp = Script::Cloud::cloud_scripts;

		bool sleep = false;
		for( int i = 0; i < temp.size( ); ++i ) {
			auto entry = &temp.at( i );
			if( !entry )
				continue;

			if( entry->m_owner_id != owner_id )
				continue;

			if( entry->m_cached_name == script_name ) {
				g_Socket.ScriptDataRequest( entry->m_script_id );
				sleep = true;
				break;
			}
		}

		if( sleep ) {
			int tries = 0;
			while( !g_Socket.m_bLoaded && tries < 100 ) {
				++tries;
				std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
			}
		}

		return g_Socket.m_bLoaded;
	}*/

	void Script::AddCallback( std::string name, sol::protected_function callback, sol::this_state state ) {
		if( !callback.valid( ) )
			return;

		auto lua = sol::state_view( state.lua_state( ) );
		auto state_ptr = lua[ XorStr( "__lua_state" ) ].get<std::shared_ptr<luaState_t>>( );

		state_ptr->callbacks.emplace_back( hash_32_fnv1a( name.data( ) ), callback );
	}

	void Script::RemoveCallback( std::string name, sol::protected_function callback, sol::this_state state ) {
		if( !callback.valid( ) )
			return;

		auto lua = sol::state_view( state.lua_state( ) );
		auto state_ptr = lua[ XorStr( "__lua_state" ) ].get<std::shared_ptr<luaState_t>>( );

		auto it = std::find_if( state_ptr->callbacks.begin( ), state_ptr->callbacks.end( ), [ & ] ( const auto &it ) -> bool {
			return it.first == hash_32_fnv1a( name.data( ) ) && it.second == callback;
		} );

		if( it != state_ptr->callbacks.end( ) ) {
			state_ptr->callbacks.erase( it );
		}
	}

	void init( ) {
		if( initialized )
			return;

		Scripting::path = GetScriptsDirectory( );
		/*path += XorStr( "\\Pandora" );

		CreateDirectory( path.c_str( ), nullptr );
		path += XorStr( "\\lua" );
		CreateDirectory( path.c_str( ), nullptr );

		path.append( XorStr( "\\" ) );*/

		/*std::string someShit = path;

		std::fstream file;
		file.open( someShit.append( XorStr( "autorun.lua" ) ), std::ios::app );
		file.close( );

		Script::LoadScript( XorStr( "autorun.lua" ) );*/

		initialized = true;
	}

	std::pair<int, int> get_mouse_position( ) {
		const auto pos = g_InputSystem.GetMousePosition( );

		return std::make_pair( pos.x, pos.y );
	}

	bool Script::LoadScript( std::string script_name, std::string &script_data, int owner_id, const std::string &scriptId, bool test ) {
		if( script_name.empty( ) ) {
			script_data = XorStr( "" );
			return false;
		}

		std::string original_name = script_name;

		loadingscript = true;

		UnloadScript( script_name );

		auto state = std::make_shared<luaState_t>( );

		// if hack is cracked and they find this
		// they could easily patch but idgaf
		// if u use a cracked version and get ratted through
		// some injected lua, it's ur fault
		if( g_Vars.menu.unsafe_scripts ) {
			// yooooo 
			state->state.open_libraries(
				sol::lib::base,
				sol::lib::package,
				sol::lib::string,
				sol::lib::math,
				sol::lib::table,
				sol::lib::bit32,
				sol::lib::utf8,
				//sol::lib::jit,
				sol::lib::ffi );
		}
		else {
			// yooooo 
			state->state.open_libraries(
				sol::lib::base,
				sol::lib::package,
				sol::lib::string,
				sol::lib::math,
				sol::lib::table,
				sol::lib::bit32,
				sol::lib::utf8 );
		}

		// le state.
		state->state[ XorStr( "__lua_state" ) ] = state;
		state->script_name = script_name;

		std::string package_path = state->state.globals( )[ XorStr( "package" ) ][ XorStr( "path" ) ];

		auto package_path_append = path;
		package_path_append.append( XorStr( "?.lua;" ) ).append( path ).append( XorStr( "\\libraries\\?.lua;" ) );

		state->state.globals( )[ XorStr( "package" ) ][ XorStr( "path" ) ] = package_path.append( package_path_append );


		// tables.
		auto callback_table = state->state[ XorStr( "callbacks" ) ].get_or_create<sol::table>( );
		auto render_table = state->state[ XorStr( "render" ) ].get_or_create<sol::table>( );
		auto client_table = state->state[ XorStr( "client" ) ].get_or_create<sol::table>( );
		auto exploits_table = state->state[ XorStr( "exploits" ) ].get_or_create<sol::table>( );
		auto aa_table = state->state[ XorStr( "anti_aim" ) ].get_or_create<sol::table>( );
		auto esp_table = state->state[ XorStr( "esp" ) ].get_or_create<sol::table>( );
		auto ui_table = state->state[ XorStr( "ui" ) ].get_or_create<sol::table>( );
		auto engine_trace_table = state->state[ XorStr( "engine_trace" ) ].get_or_create<sol::table>( );
		auto font_flags_table = state->state[ XorStr( "font_flags" ) ].get_or_create<sol::table>( );
		auto input_table = state->state[ XorStr( "input" ) ].get_or_create<sol::table>( );

		auto ent_list = state->state[ XorStr( "entity_list" ) ].get_or_create<sol::table>( );
		auto engine = state->state[ XorStr( "engine" ) ].get_or_create<sol::table>( );
		auto penetration = state->state[ XorStr( "penetration" ) ].get_or_create<sol::table>( );
		auto cvar = state->state[ XorStr( "cvar" ) ].get_or_create<sol::table>( );
		auto cmd = state->state[ XorStr( "command" ) ].get_or_create<sol::table>( );
		auto materials = state->state[ XorStr( "materials" ) ].get_or_create<sol::table>( );

		materials[ XorStr( "create_material" ) ] = sol::overload( &Wrappers::Material::CreateMaterial );
		materials[ XorStr( "find_material" ) ] = sol::overload( &Wrappers::Material::FindMaterialNoGroup, &Wrappers::Material::FindMaterial );

		input_table[ XorStr( "mouse_position" ) ] = &get_mouse_position;
		input_table[ XorStr( "key_down" ) ] = &InputHelper::Down;
		input_table[ XorStr( "key_pressed" ) ] = &InputHelper::Pressed;
		input_table[ XorStr( "force_cursor" ) ] = &force_cursor;

		engine_trace_table[ XorStr( "trace_ray" ) ] = sol::overload( &Wrappers::Interfaces::EngineTrace::TraceRay );

		// globals xd
		state->state[ XorStr( "print" ) ] = sol::overload( &print );

		aa_table[ std::string( XorStr( "inverted" ) ) ] = sol::overload( &aa_invert );
		aa_table[ std::string( XorStr( "should_run" ) ) ] = sol::overload( &aa_run );

		esp_table[ std::string( XorStr( "set_thirdperson_animation" ) ) ] = sol::overload( &set_thirdperson_animation );
		esp_table[ std::string( XorStr( "set_fading_chams" ) ) ] = sol::overload( &set_fading_chams );
		esp_table[ std::string( XorStr( "set_visual_font" ) ) ] = sol::overload( &set_visual_font );
		esp_table[ std::string( XorStr( "set_visual_case" ) ) ] = sol::overload( &set_visual_case );
		esp_table[ std::string( XorStr( "set_flag_offset" ) ) ] = sol::overload( &set_flag_offset );
		esp_table[ std::string( XorStr( "use_collision_bounding_boxes" ) ) ] = sol::overload( &use_collision_bounding_boxes );
		esp_table[ std::string( XorStr( "health_bar_string_left" ) ) ] = sol::overload( &health_bar_string_left );
		esp_table[ std::string( XorStr( "add_player_flag" ) ) ] = sol::overload( &add_esp_flag );

		// usertypes
		auto color_ut = state->state.new_usertype<Color>( XorStr( "color" ) );
		color_ut[ sol::meta_function::construct ] = sol::constructors<Color( int, int, int ), Color( int, int, int, int )>( );
		color_ut[ std::string( XorStr( "r" ) ) ] = sol::overload( &Color::r );
		color_ut[ std::string( XorStr( "g" ) ) ] = sol::overload( &Color::g );
		color_ut[ std::string( XorStr( "b" ) ) ] = sol::overload( &Color::b );
		color_ut[ std::string( XorStr( "a" ) ) ] = sol::overload( &Color::a );

		auto vector_ut = state->state.new_usertype<Vector>( XorStr( "vector" ) );
		vector_ut[ sol::meta_function::construct ] = sol::constructors<Vector( float, float, float )>( );
		vector_ut[ std::string( XorStr( "x" ) ) ] = &Vector::x;
		vector_ut[ std::string( XorStr( "y" ) ) ] = &Vector::y;
		vector_ut[ std::string( XorStr( "z" ) ) ] = &Vector::z;

		auto qangle_ut = state->state.new_usertype<QAngle>( XorStr( "qangle" ) );
		qangle_ut[ sol::meta_function::construct ] = sol::constructors<QAngle( float, float, float )>( );
		qangle_ut[ std::string( XorStr( "x" ) ) ] = &QAngle::x;
		qangle_ut[ std::string( XorStr( "y" ) ) ] = &QAngle::y;
		qangle_ut[ std::string( XorStr( "z" ) ) ] = &QAngle::z;

		auto vector2d_ut = state->state.new_usertype<Vector2D>( XorStr( "vector2d" ) );
		vector2d_ut[ sol::meta_function::construct ] = sol::constructors<Vector2D( float, float )>( );
		vector2d_ut[ std::string( XorStr( "x" ) ) ] = &Vector2D::x;
		vector2d_ut[ std::string( XorStr( "y" ) ) ] = &Vector2D::y;

		auto vertex_ut = state->state.new_usertype<Vertex_t>( XorStr( "vertex" ) );
		vertex_ut[ sol::meta_function::construct ] = sol::constructors<Vertex_t( Vector2D, Vector2D )>( );
		vertex_ut[ std::string( XorStr( "position" ) ) ] = &Vertex_t::m_Position;
		vertex_ut[ std::string( XorStr( "texCoord" ) ) ] = &Vertex_t::m_TexCoord;

		auto event_ut = state->state.new_usertype<IGameEvent>( XorStr( "__IGameEvent" ), sol::no_constructor );
		event_ut[ std::string( XorStr( "is_reliable" ) ) ] = sol::overload( &IGameEvent::IsReliable );
		event_ut[ std::string( XorStr( "is_local" ) ) ] = sol::overload( &IGameEvent::IsLocal );
		event_ut[ std::string( XorStr( "is_empty" ) ) ] = sol::overload( &IGameEvent::IsEmpty );
		event_ut[ std::string( XorStr( "get_bool" ) ) ] = sol::overload( &IGameEvent::get_bool );
		event_ut[ std::string( XorStr( "get_int" ) ) ] = sol::overload( &IGameEvent::get_int );
		event_ut[ std::string( XorStr( "get_uint64" ) ) ] = sol::overload( &IGameEvent::get_uint64 );
		event_ut[ std::string( XorStr( "get_float" ) ) ] = sol::overload( &IGameEvent::get_float );
		event_ut[ std::string( XorStr( "get_string" ) ) ] = sol::overload( &IGameEvent::get_string );
		event_ut[ std::string( XorStr( "get_wstring" ) ) ] = sol::overload( &IGameEvent::get_wstring );


		auto materialvar_ut = state->state.new_usertype<Wrappers::Material::LuaMaterialVar>( XorStr( "__LuaMaterialVar" ), sol::no_constructor );
		materialvar_ut[ std::string( XorStr( "set_int_value" ) ) ] = sol::overload( &Wrappers::Material::LuaMaterialVar::SetIntValue );
		materialvar_ut[ std::string( XorStr( "set_float_value" ) ) ] = sol::overload( &Wrappers::Material::LuaMaterialVar::SetFloatValue );
		materialvar_ut[ std::string( XorStr( "set_string_value" ) ) ] = sol::overload( &Wrappers::Material::LuaMaterialVar::SetStringValue );
		materialvar_ut[ std::string( XorStr( "set_vec_value" ) ) ] = sol::overload( &Wrappers::Material::LuaMaterialVar::SetVecValue );

		auto lua_material_ut = state->state.new_usertype<Wrappers::Material::LuaMaterial>( XorStr( "__LuaMaterial" ), sol::no_constructor );
		lua_material_ut[ std::string( XorStr( "is_valid" ) ) ] = sol::overload( &Wrappers::Material::LuaMaterial::IsValid );
		lua_material_ut[ std::string( XorStr( "modulate_alpha" ) ) ] = sol::overload( &Wrappers::Material::LuaMaterial::ModulateAlpha );
		lua_material_ut[ std::string( XorStr( "modulate_color" ) ) ] = sol::overload( &Wrappers::Material::LuaMaterial::ModulateColor );
		lua_material_ut[ std::string( XorStr( "set_material_var_flag" ) ) ] = sol::overload( &Wrappers::Material::LuaMaterial::SetMaterialFlag );
		lua_material_ut[ std::string( XorStr( "find_var" ) ) ] = sol::overload( &Wrappers::Material::LuaMaterial::FindMaterialVar );

		auto modeldrawcontext_ut = state->state.new_usertype<Wrappers::Material::ModelDrawContext>( XorStr( "__ModelDrawContext" ), sol::no_constructor );
		modeldrawcontext_ut[ std::string( XorStr( "get_entity" ) ) ] = sol::overload( &Wrappers::Material::ModelDrawContext::GetEntity );
		modeldrawcontext_ut[ std::string( XorStr( "get_model" ) ) ] = sol::overload( &Wrappers::Material::ModelDrawContext::GetModelName );
		modeldrawcontext_ut[ std::string( XorStr( "draw_model" ) ) ] = sol::overload( &Wrappers::Material::ModelDrawContext::DrawModel );
		modeldrawcontext_ut[ std::string( XorStr( "force_material_override" ) ) ] = sol::overload( &Wrappers::Material::ModelDrawContext::ForceMaterialOverride );


		auto netvar_ut = state->state.new_usertype<Wrappers::Entity::NetVarType>( XorStr( "__NetVarType" ), sol::no_constructor );
		// get
		netvar_ut[ std::string( XorStr( "get_bool" ) ) ] = sol::overload( &Wrappers::Entity::NetVarType::get_bool );
		netvar_ut[ std::string( XorStr( "get_int" ) ) ] = sol::overload( &Wrappers::Entity::NetVarType::get_int );
		netvar_ut[ std::string( XorStr( "get_float" ) ) ] = sol::overload( &Wrappers::Entity::NetVarType::get_float );
		netvar_ut[ std::string( XorStr( "get_string" ) ) ] = sol::overload( &Wrappers::Entity::NetVarType::get_string );
		netvar_ut[ std::string( XorStr( "get_vector" ) ) ] = sol::overload( &Wrappers::Entity::NetVarType::get_vector );
		netvar_ut[ std::string( XorStr( "get_vector2d" ) ) ] = sol::overload( &Wrappers::Entity::NetVarType::get_vector2d );
		netvar_ut[ std::string( XorStr( "get_float_index" ) ) ] = sol::overload( &Wrappers::Entity::NetVarType::get_float_index );

		//set
		netvar_ut[ std::string( XorStr( "set_bool" ) ) ] = sol::overload( &Wrappers::Entity::NetVarType::set_bool );
		netvar_ut[ std::string( XorStr( "set_int" ) ) ] = sol::overload( &Wrappers::Entity::NetVarType::set_int );
		netvar_ut[ std::string( XorStr( "set_float" ) ) ] = sol::overload( &Wrappers::Entity::NetVarType::set_float );
		netvar_ut[ std::string( XorStr( "set_vector" ) ) ] = sol::overload( &Wrappers::Entity::NetVarType::set_vector );
		netvar_ut[ std::string( XorStr( "set_vector2d" ) ) ] = sol::overload( &Wrappers::Entity::NetVarType::set_vector2d );
		netvar_ut[ std::string( XorStr( "set_float_index" ) ) ] = sol::overload( &Wrappers::Entity::NetVarType::set_float_index );

		auto convar_ut = state->state.new_usertype<ConVar>( XorStr( "__ConVar" ), sol::no_constructor );
		convar_ut[ std::string( XorStr( "get_int" ) ) ] = sol::overload( &ConVar::GetInt );
		convar_ut[ std::string( XorStr( "get_bool" ) ) ] = sol::overload( &ConVar::GetBool );
		convar_ut[ std::string( XorStr( "get_float" ) ) ] = sol::overload( &ConVar::GetFloat );
		convar_ut[ std::string( XorStr( "get_string" ) ) ] = sol::overload( &ConVar::GetString );
		convar_ut[ std::string( XorStr( "set_value_int" ) ) ] = sol::overload( &ConVar::SetValueInt );
		convar_ut[ std::string( XorStr( "set_value_float" ) ) ] = sol::overload( &ConVar::SetValueFloat );
		convar_ut[ std::string( XorStr( "set_value_string" ) ) ] = sol::overload( &ConVar::SetValueString );
		convar_ut[ std::string( XorStr( "is_valid" ) ) ] = sol::overload( &ConVar::Valid );

		auto global_vars_ut = state->state.new_usertype<CGlobalVars>( XorStr( "__global_vars" ), sol::no_constructor );
		global_vars_ut[ std::string( XorStr( "curtime" ) ) ] = &CGlobalVars::curtime;
		global_vars_ut[ std::string( XorStr( "frametime" ) ) ] = &CGlobalVars::frametime;
		global_vars_ut[ std::string( XorStr( "absoluteframetime" ) ) ] = &CGlobalVars::absoluteframetime;
		global_vars_ut[ std::string( XorStr( "framecount" ) ) ] = &CGlobalVars::framecount;
		global_vars_ut[ std::string( XorStr( "realtime" ) ) ] = &CGlobalVars::realtime;
		global_vars_ut[ std::string( XorStr( "max_clients" ) ) ] = &CGlobalVars::maxClients;
		global_vars_ut[ std::string( XorStr( "interval_per_tick" ) ) ] = &CGlobalVars::interval_per_tick;
		global_vars_ut[ std::string( XorStr( "tickcount" ) ) ] = &CGlobalVars::tickcount;

		auto player_info_ut = state->state.new_usertype<player_info_t>( XorStr( "__player_info_t" ), sol::no_constructor );
		player_info_ut[ std::string( XorStr( "fakeplayer" ) ) ] = &player_info_t::fakeplayer;
		player_info_ut[ std::string( XorStr( "name" ) ) ] = &player_info_t::szName;
		player_info_ut[ std::string( XorStr( "steamID64" ) ) ] = &player_info_t::steamID64;
		player_info_ut[ std::string( XorStr( "szSteamID" ) ) ] = &player_info_t::szSteamID;
		player_info_ut[ std::string( XorStr( "userId" ) ) ] = &player_info_t::userId;
		player_info_ut[ std::string( XorStr( "steamID" ) ) ] = &player_info_t::iSteamID;

		auto game_trace_ut = state->state.new_usertype<CGameTrace>( XorStr( "__game_trace" ), sol::no_constructor );
		game_trace_ut[ std::string( XorStr( "allsolid" ) ) ] = &CGameTrace::allsolid;
		game_trace_ut[ std::string( XorStr( "endpos" ) ) ] = &CGameTrace::endpos;
		game_trace_ut[ std::string( XorStr( "fraction" ) ) ] = &CGameTrace::fraction;
		game_trace_ut[ std::string( XorStr( "hitbox" ) ) ] = &CGameTrace::hitbox;
		game_trace_ut[ std::string( XorStr( "hitgroup" ) ) ] = &CGameTrace::hitgroup;
		game_trace_ut[ std::string( XorStr( "startsolid" ) ) ] = &CGameTrace::startsolid;

		auto usercmd_ut = state->state.new_usertype<CUserCmd>( XorStr( "_______usercmd" ), sol::no_constructor );
		usercmd_ut[ std::string( XorStr( "command_number" ) ) ] = &CUserCmd::command_number;
		usercmd_ut[ std::string( XorStr( "tick_count" ) ) ] = &CUserCmd::tick_count;
		usercmd_ut[ std::string( XorStr( "viewangles" ) ) ] = &CUserCmd::viewangles;
		usercmd_ut[ std::string( XorStr( "aimdirection" ) ) ] = &CUserCmd::aimdirection;
		usercmd_ut[ std::string( XorStr( "forwardmove" ) ) ] = &CUserCmd::forwardmove;
		usercmd_ut[ std::string( XorStr( "sidemove" ) ) ] = &CUserCmd::sidemove;
		usercmd_ut[ std::string( XorStr( "upmove" ) ) ] = &CUserCmd::upmove;
		usercmd_ut[ std::string( XorStr( "buttons" ) ) ] = &CUserCmd::buttons;
		usercmd_ut[ std::string( XorStr( "impulse" ) ) ] = &CUserCmd::impulse;
		usercmd_ut[ std::string( XorStr( "weaponselect" ) ) ] = &CUserCmd::weaponselect;
		usercmd_ut[ std::string( XorStr( "weaponsubtype" ) ) ] = &CUserCmd::weaponsubtype;
		usercmd_ut[ std::string( XorStr( "random_seed" ) ) ] = &CUserCmd::random_seed;
		usercmd_ut[ std::string( XorStr( "mousedx" ) ) ] = &CUserCmd::mousedx;
		usercmd_ut[ std::string( XorStr( "mousedy" ) ) ] = &CUserCmd::mousedy;
		usercmd_ut[ std::string( XorStr( "hasbeenpredicted" ) ) ] = &CUserCmd::hasbeenpredicted;
		//usercmd_ut[ std::string( XorStr( "send_packet" ) ) ] = &CUserCmd::send_packet;
		usercmd_ut[ std::string( XorStr( "has_flag" ) ) ] = sol::overload( &CUserCmd::has_flag );
		usercmd_ut[ std::string( XorStr( "set_flag" ) ) ] = sol::overload( &CUserCmd::set_flag );
		usercmd_ut[ std::string( XorStr( "remove_flag" ) ) ] = sol::overload( &CUserCmd::remove_flag );

		// buttons
		cmd[ std::string( XorStr( "in_attack" ) ) ] = IN_ATTACK;
		cmd[ std::string( XorStr( "in_jump" ) ) ] = IN_JUMP;
		cmd[ std::string( XorStr( "in_duck" ) ) ] = IN_DUCK;
		cmd[ std::string( XorStr( "in_forward" ) ) ] = IN_FORWARD;
		cmd[ std::string( XorStr( "in_back" ) ) ] = IN_BACK;
		cmd[ std::string( XorStr( "in_use" ) ) ] = IN_USE;
		cmd[ std::string( XorStr( "in_cancel" ) ) ] = IN_CANCEL;
		cmd[ std::string( XorStr( "in_left" ) ) ] = IN_LEFT;
		cmd[ std::string( XorStr( "in_right" ) ) ] = IN_RIGHT;
		cmd[ std::string( XorStr( "in_moveleft" ) ) ] = IN_MOVELEFT;
		cmd[ std::string( XorStr( "in_moveright" ) ) ] = IN_MOVERIGHT;
		cmd[ std::string( XorStr( "in_attack2" ) ) ] = IN_ATTACK2;
		cmd[ std::string( XorStr( "in_reload" ) ) ] = IN_RELOAD;
		cmd[ std::string( XorStr( "in_speed" ) ) ] = IN_SPEED;
		cmd[ std::string( XorStr( "in_bullrush" ) ) ] = IN_BULLRUSH;

		auto entity_ut = state->state.new_usertype<Wrappers::Entity::CEntity>( XorStr( "entity" )/*, sol::no_constructor*/ );
		entity_ut[ sol::meta_function::construct ] = sol::constructors< Wrappers::Entity::CEntity( C_CSPlayer * )>( );
		entity_ut[ std::string( XorStr( "index" ) ) ] = sol::overload( &Wrappers::Entity::CEntity::index );
		entity_ut[ std::string( XorStr( "dormant" ) ) ] = sol::overload( &Wrappers::Entity::CEntity::dormant );
		entity_ut[ std::string( XorStr( "origin" ) ) ] = sol::overload( &Wrappers::Entity::CEntity::origin );
		entity_ut[ std::string( XorStr( "eye_position" ) ) ] = sol::overload( &Wrappers::Entity::CEntity::eye_position );
		entity_ut[ std::string( XorStr( "hitbox_position" ) ) ] = sol::overload( &Wrappers::Entity::CEntity::hitbox_position );
		entity_ut[ std::string( XorStr( "has_weapon" ) ) ] = sol::overload( &Wrappers::Entity::CEntity::has_weapon );
		entity_ut[ std::string( XorStr( "class_id" ) ) ] = sol::overload( &Wrappers::Entity::CEntity::GetClassID );
		entity_ut[ std::string( XorStr( "get_prop" ) ) ] = sol::overload( &Wrappers::Entity::CEntity::GetVar );

		//auto font_size_ut = state->state.new_usertype<Render::Engine::FontSize_t>( XorStr( "__FontSize" ), sol::no_constructor );
		//font_size_ut[ std::string( XorStr( "width" ) ) ] = &Render::Engine::FontSize_t::m_width;
		//font_size_ut[ std::string( XorStr( "height" ) ) ] = &Render::Engine::FontSize_t::m_height;

		auto font_ut = state->state.new_usertype<Render::Engine::Font>( XorStr( "__Font" ), sol::no_constructor );
		font_ut[ std::string( XorStr( "text" ) ) ] = sol::overload( &Render::Engine::Font::lua_string );
		font_ut[ std::string( XorStr( "get_size" ) ) ] = sol::overload( &Render::Engine::Font::lua_size );

		// menu item definitions
		auto checkbox_ut = state->state.new_usertype<checkboxItem>( XorStr( "__checkboxItem" ), sol::no_constructor );
		checkbox_ut[ std::string( XorStr( "get" ) ) ] = sol::overload( &checkboxItem::get );
		checkbox_ut[ std::string( XorStr( "set" ) ) ] = sol::overload( &checkboxItem::set );
		checkbox_ut[ std::string( XorStr( "set_visible" ) ) ] = sol::overload( &checkboxItem::set_visible );

		auto dropdown_ut = state->state.new_usertype<dropdownItem>( XorStr( "__dropdownItem" ), sol::no_constructor );
		dropdown_ut[ std::string( XorStr( "get" ) ) ] = sol::overload( &dropdownItem::get );
		dropdown_ut[ std::string( XorStr( "set" ) ) ] = sol::overload( &dropdownItem::set );
		dropdown_ut[ std::string( XorStr( "update_items" ) ) ] = sol::overload( &dropdownItem::update_items );
		dropdown_ut[ std::string( XorStr( "set_visible" ) ) ] = sol::overload( &dropdownItem::set_visible );

		auto multidropdown_ut = state->state.new_usertype<multidropdownItem>( XorStr( "__multidropdownItem" ), sol::no_constructor );
		multidropdown_ut[ std::string( XorStr( "get" ) ) ] = sol::overload( &multidropdownItem::get );
		multidropdown_ut[ std::string( XorStr( "set" ) ) ] = sol::overload( &multidropdownItem::set );
		multidropdown_ut[ std::string( XorStr( "set_visible" ) ) ] = sol::overload( &multidropdownItem::set_visible );

		auto colorpicker_ut = state->state.new_usertype<colorpickerItem>( XorStr( "__colorpickerItem" ), sol::no_constructor );
		colorpicker_ut[ std::string( XorStr( "get" ) ) ] = sol::overload( &colorpickerItem::get_color );
		colorpicker_ut[ std::string( XorStr( "set" ) ) ] = sol::overload( &colorpickerItem::set_color );
		colorpicker_ut[ std::string( XorStr( "set_visible" ) ) ] = sol::overload( &colorpickerItem::set_visible );

		auto hotkey_ut = state->state.new_usertype<hotkeyItem>( XorStr( "__hotkeyItem" ), sol::no_constructor );
		hotkey_ut[ std::string( XorStr( "get" ) ) ] = sol::overload( &hotkeyItem::get_hotkey );
		hotkey_ut[ std::string( XorStr( "set_visible" ) ) ] = sol::overload( &hotkeyItem::set_visible );
		hotkey_ut[ std::string( XorStr( "set" ) ) ] = sol::overload( &hotkeyItem::set_hotkey );
		hotkey_ut[ std::string( XorStr( "set_key_cond" ) ) ] = sol::overload( &hotkeyItem::set_hotkey_cond );

		auto slider_ut = state->state.new_usertype<sliderItem>( XorStr( "__sliderItem" ), sol::no_constructor );
		slider_ut[ std::string( XorStr( "get" ) ) ] = sol::overload( &sliderItem::get );
		slider_ut[ std::string( XorStr( "set" ) ) ] = sol::overload( &sliderItem::set );
		slider_ut[ std::string( XorStr( "set_visible" ) ) ] = sol::overload( &sliderItem::set_visible );

		auto slider_float_ut = state->state.new_usertype<sliderFloatItem>( XorStr( "__sliderFloatItem" ), sol::no_constructor );
		slider_float_ut[ std::string( XorStr( "get" ) ) ] = sol::overload( &sliderFloatItem::get );
		slider_float_ut[ std::string( XorStr( "set" ) ) ] = sol::overload( &sliderFloatItem::set );
		slider_float_ut[ std::string( XorStr( "set_visible" ) ) ] = sol::overload( &sliderFloatItem::set_visible );

		auto label_ut = state->state.new_usertype<labelItem>( XorStr( "__labelItem" ), sol::no_constructor );
		label_ut[ std::string( XorStr( "set_visible" ) ) ] = sol::overload( &labelItem::set_visible );

		auto uiitem_ut = state->state.new_usertype<uiItem>( XorStr( "__uiItem" ), sol::no_constructor );
		uiitem_ut[ std::string( XorStr( "get" ) ) ] = sol::overload( &uiItem::get );
		uiitem_ut[ std::string( XorStr( "set_visible" ) ) ] = sol::overload( &uiItem::set_visible );

		auto button_ut = state->state.new_usertype<buttonItem>( XorStr( "__buttonItem" ), sol::no_constructor );
		button_ut[ std::string( XorStr( "add_callback" ) ) ] = sol::overload( &buttonItem::add_callback );
		button_ut[ std::string( XorStr( "set_visible" ) ) ] = sol::overload( &buttonItem::set_visible );

		auto textbox_ut = state->state.new_usertype<textBoxItem>( XorStr( "__textboxItem" ), sol::no_constructor );
		textbox_ut[ std::string( XorStr( "get" ) ) ] = sol::overload( &textBoxItem::get );
		//textbox_ut[ std::string( XorStr( "set" ) ) ] = sol::overload( &textBoxItem::set );
		textbox_ut[ std::string( XorStr( "set_visible" ) ) ] = sol::overload( &textBoxItem::set_visible );

		/*auto shot_miss_ut = state->state.new_usertype<shot_miss_t>( XorStr( "____shotmiss_t" ), sol::no_constructor );
		shot_miss_ut[ std::string( XorStr( "id" ) ) ] = &shot_miss_t::id;
		shot_miss_ut[ std::string( XorStr( "target_index" ) ) ] = &shot_miss_t::target_index;
		shot_miss_ut[ std::string( XorStr( "hitchance" ) ) ] = &shot_miss_t::hitchance;
		shot_miss_ut[ std::string( XorStr( "hitgroup" ) ) ] = &shot_miss_t::hitgroup;
		shot_miss_ut[ std::string( XorStr( "damage" ) ) ] = &shot_miss_t::damage;
		shot_miss_ut[ std::string( XorStr( "reason" ) ) ] = &shot_miss_t::reason;

		auto shot_fire_ut = state->state.new_usertype<shot_fire_t>( XorStr( "____shotfire_t" ), sol::no_constructor );
		shot_fire_ut[ std::string( XorStr( "id" ) ) ] = &shot_fire_t::id;
		shot_fire_ut[ std::string( XorStr( "target_index" ) ) ] = &shot_fire_t::target_index;
		shot_fire_ut[ std::string( XorStr( "hitchance" ) ) ] = &shot_fire_t::hitchance;
		shot_fire_ut[ std::string( XorStr( "hitgroup" ) ) ] = &shot_fire_t::hitgroup;
		shot_fire_ut[ std::string( XorStr( "damage" ) ) ] = &shot_fire_t::damage;
		shot_fire_ut[ std::string( XorStr( "bt" ) ) ] = &shot_fire_t::bt;
		shot_fire_ut[ std::string( XorStr( "complex_safe" ) ) ] = &shot_fire_t::complex_safe;
		shot_fire_ut[ std::string( XorStr( "basic_safe" ) ) ] = &shot_fire_t::basic_safe;*/

		// yoooooooooo we gotta be able to register...
		callback_table[ XorStr( "register" ) ] = sol::overload( &AddCallback );
		callback_table[ XorStr( "deregister" ) ] = sol::overload( &RemoveCallback );

		// client yo
		client_table[ XorStr( "username" ) ] = get_string_from_array( g_Vars.globals.user_info.the_array );
		client_table[ XorStr( "log" ) ] = sol::overload( &log, &log_color, &log_prefix_color, &log_prefix_color_visible );
		client_table[ XorStr( "load_script" ) ] = sol::overload( &load_script_file/*, &load_script_cloud*/ );
		client_table[ XorStr( "choked_commands" ) ] = sol::overload( &Wrappers::Interfaces::GlobalVars::choked_commands );
		client_table[ XorStr( "is_alive" ) ] = sol::overload( &client_is_alive );
		client_table[ XorStr( "latency" ) ] = sol::overload( &get_latency );
		client_table[ XorStr( "local_time" ) ] = sol::overload( &get_time );
		client_table[ XorStr( "current_threat" ) ] = sol::overload( &get_current_threat );
		client_table[ XorStr( "set_quick_peek_assist_lock" ) ] = sol::overload( &set_quick_peek_assist_lock );
		client_table[ XorStr( "map_name" ) ] = &client_map_name;
		client_table[ XorStr( "unix_timestamp" ) ] = &unix_timestamp;
		client_table[ XorStr( "camera_angles" ) ] = &camera_angles;
		client_table[ XorStr( "camera_position" ) ] = &camera_position;

		if( g_Vars.menu.unsafe_scripts ) {
			client_table[ XorStr( "find_sig" ) ] = &pattern_scan;
			client_table[ XorStr( "create_interface" ) ] = &create_interface;
		}

		// interfaces
		ent_list[ XorStr( "get_client_entity" ) ] = sol::overload( &Wrappers::Interfaces::EntityList::GetClientEntity, &Wrappers::Interfaces::EntityList::GetClientEntityFromHandle );
		ent_list[ XorStr( "get_highest_entity_index" ) ] = sol::overload( &Wrappers::Interfaces::EntityList::GetHighestEntityIndex );
		ent_list[ XorStr( "get_all" ) ] = sol::overload( &Wrappers::Interfaces::EntityList::GetAll );

		engine[ XorStr( "get_player_for_user_id" ) ] = sol::overload( &Wrappers::Interfaces::Engine::GetPlayerForUserID );
		engine[ XorStr( "in_game" ) ] = sol::overload( &Wrappers::Interfaces::Engine::IsInGame );
		engine[ XorStr( "is_connected" ) ] = sol::overload( &Wrappers::Interfaces::Engine::IsConnected );
		engine[ XorStr( "get_restart_round_time" ) ] = sol::overload( &Wrappers::Interfaces::Engine::GetRestartRoundTime );
		engine[ XorStr( "execute_client_cmd" ) ] = sol::overload( &Wrappers::Interfaces::Engine::ExecuteClientCmd );
		engine[ XorStr( "set_view_angles" ) ] = sol::overload( &Wrappers::Interfaces::Engine::SetViewAngles );
		engine[ XorStr( "get_view_angles" ) ] = sol::overload( &Wrappers::Interfaces::Engine::GetViewAngles );
		engine[ XorStr( "get_local_player" ) ] = sol::overload( &Wrappers::Interfaces::Engine::GetLocalPlayer );
		engine[ XorStr( "get_player_info" ) ] = sol::overload( &Wrappers::Interfaces::Engine::GetPlayerInfo );


		// lol so epic
		state->state[ XorStr( "global_vars" ) ] = g_pGlobalVars.Xor( );

		cvar[ XorStr( "find_var" ) ] = sol::overload( &Wrappers::Interfaces::ConVar::FindVar );

		// rendeRii
		render_table[ XorStr( "rectangle" ) ] = sol::overload( &Wrappers::Renderer::Rect );
		render_table[ XorStr( "rectangle_filled" ) ] = sol::overload( &Wrappers::Renderer::RectFilled );
		render_table[ XorStr( "world_to_screen" ) ] = sol::overload( &Wrappers::Renderer::WorldToScreen );
		render_table[ XorStr( "polygon" ) ] = sol::overload( &Wrappers::Renderer::Polygon );
		render_table[ XorStr( "triangle_filled" ) ] = sol::overload( &Wrappers::Renderer::FilledTriangle );
		render_table[ XorStr( "circle_world" ) ] = sol::overload( &Wrappers::Renderer::WorldCircle );
		render_table[ XorStr( "line" ) ] = sol::overload( &Wrappers::Renderer::Line );
		render_table[ XorStr( "circle_filled" ) ] = sol::overload( &Wrappers::Renderer::CircleFilled );
		render_table[ XorStr( "gradient" ) ] = sol::overload( &Wrappers::Renderer::Gradient );
		render_table[ XorStr( "text" ) ] = sol::overload( &Wrappers::Renderer::Text );
		render_table[ XorStr( "get_text_size" ) ] = sol::overload( &Wrappers::Renderer::GetTextSize );
		render_table[ XorStr( "get_screen" ) ] = sol::overload( &Wrappers::Renderer::GetScreenSize );
		render_table[ XorStr( "create_font" ) ] = sol::overload( &Wrappers::Renderer::GetFont );
		render_table[ XorStr( "rectangle_rounded" ) ] = &Wrappers::Renderer::RectRounded;
		render_table[ XorStr( "rectangle_rounded_filled" ) ] = &Wrappers::Renderer::RectRoundedFilled;

		font_flags_table[ XorStr( "none" ) ] = FONTFLAG_NONE;
		font_flags_table[ XorStr( "dropshadow" ) ] = FONTFLAG_DROPSHADOW;
		font_flags_table[ XorStr( "antialias" ) ] = FONTFLAG_ANTIALIAS;
		font_flags_table[ XorStr( "outline" ) ] = FONTFLAG_OUTLINE;

		// exploits
		exploits_table[ XorStr( "process_ticks" ) ] = sol::overload( &Wrappers::Exploits::GetProcessTicks );
		exploits_table[ XorStr( "force_recharge" ) ] = sol::overload( &Wrappers::Exploits::ForceRecharge );
		exploits_table[ XorStr( "max_process_ticks" ) ] = sol::overload( &Wrappers::Exploits::GetMaxProcessTicks );
		exploits_table[ XorStr( "shifted" ) ] = sol::overload( &Wrappers::Exploits::DidShift );
		exploits_table[ XorStr( "charging" ) ] = sol::overload( &Wrappers::Exploits::IsCharging );
		exploits_table[ XorStr( "ready" ) ] = sol::overload( &Wrappers::Exploits::IsReady );
		exploits_table[ XorStr( "force_uncharge" ) ] = &Wrappers::Exploits::ForceUncharge;
	#if defined(DEV) || defined(BETA_MODE)
		exploits_table[ XorStr( "performing_defensive" ) ] = &Wrappers::Exploits::PerformingDefensive;
	#endif

		// pen data
		penetration[ XorStr( "damage" ) ] = sol::overload( &Wrappers::Penetration::GetPenDamage );
		penetration[ XorStr( "simulate_bullet" ) ] = sol::overload( &Wrappers::Penetration::SimulateBullet );
		penetration[ XorStr( "aiming_at_player" ) ] = sol::overload( &Wrappers::Penetration::AimingAtPlayer );


		ui_table[ XorStr( "add_checkbox" ) ] = sol::overload( [ & ] ( sol::this_state state, std::string name ) {
			auto lua = sol::state_view( state.lua_state( ) );
			auto state_ptr = lua[ XorStr( "__lua_state" ) ].get<std::shared_ptr<luaState_t>>( );

			auto item = new checkboxItem;

			auto temp = name;
			item->init( temp.append( state_ptr->script_name ) );

			state_ptr->ui_items.emplace_back( name, item );

			return sol::make_object( state, item );
		} );

		ui_table[ XorStr( "add_dropdown" ) ] = sol::overload( [ & ] ( sol::this_state state, std::string name, std::vector<std::string> items ) {
			auto lua = sol::state_view( state.lua_state( ) );
			auto state_ptr = lua[ XorStr( "__lua_state" ) ].get<std::shared_ptr<luaState_t>>( );

			auto item = new dropdownItem;

			auto temp = name;
			item->init( temp.append( state_ptr->script_name ) );

			item->items = items;

			state_ptr->ui_items.emplace_back( name, item );

			return sol::make_object( state, item );
		} );

		ui_table[ XorStr( "add_multi_dropdown" ) ] = sol::overload( [ & ] ( sol::this_state state, std::string name, std::vector<std::string> items ) {
			auto lua = sol::state_view( state.lua_state( ) );
			auto state_ptr = lua[ XorStr( "__lua_state" ) ].get<std::shared_ptr<luaState_t>>( );

			auto item = new multidropdownItem;

			item->init( name );

			for( auto it : items ) {
				item->items->push_back( { it, g_Vars.lua.Add<bool>( it.append( name ).append( state_ptr->script_name ).c_str( ), false ) } );
			}

			state_ptr->ui_items.emplace_back( name, item );

			return sol::make_object( state, item );
		} );

		ui_table[ XorStr( "add_hotkey" ) ] = sol::overload( [ & ] ( sol::this_state state, std::string name ) {
			auto lua = sol::state_view( state.lua_state( ) );
			auto state_ptr = lua[ XorStr( "__lua_state" ) ].get<std::shared_ptr<luaState_t>>( );

			auto item = new hotkeyItem;

			auto temp = name;

			item->init( temp.append( XorStr( " (" ) ).append( state_ptr->script_name ).append( XorStr( ")" ) ) );

			state_ptr->ui_items.emplace_back( name, item );

			return sol::make_object( state, item );
		} );

		ui_table[ XorStr( "add_colorpicker" ) ] = sol::overload( [ & ] ( sol::this_state state, std::string name ) {
			auto lua = sol::state_view( state.lua_state( ) );
			auto state_ptr = lua[ XorStr( "__lua_state" ) ].get<std::shared_ptr<luaState_t>>( );

			auto item = new colorpickerItem;

			auto temp = name;

			item->init( temp.append( state_ptr->script_name ) );

			state_ptr->ui_items.emplace_back( name, item );

			return sol::make_object( state, item );
		} );


		ui_table[ XorStr( "add_slider" ) ] = sol::overload( [ & ] ( sol::this_state state, std::string name, int min, int max ) {
			auto lua = sol::state_view( state.lua_state( ) );
			auto state_ptr = lua[ XorStr( "__lua_state" ) ].get<std::shared_ptr<luaState_t>>( );

			if( max == 0 )
				max = 1;

			auto item = new sliderItem;
			item->min = min;
			item->max = max;

			auto temp = name;
			item->init( temp.append( state_ptr->script_name ) );

			state_ptr->ui_items.emplace_back( name, item );

			return sol::make_object( state, item );
		} );

		ui_table[ XorStr( "add_slider_float" ) ] = sol::overload( [ & ] ( sol::this_state state, std::string name, float min, float max ) {
			auto lua = sol::state_view( state.lua_state( ) );
			auto state_ptr = lua[ XorStr( "__lua_state" ) ].get<std::shared_ptr<luaState_t>>( );

			if( max == 0 )
				max = 1;

			auto item = new sliderFloatItem;
			item->min = min;
			item->max = max;

			auto temp = name;
			item->init( temp.append( state_ptr->script_name ) );

			state_ptr->ui_items.emplace_back( name, item );

			return sol::make_object( state, item );
		} );

		ui_table[ XorStr( "add_label" ) ] = sol::overload( [ ] ( sol::this_state state, std::string name ) {
			auto lua = sol::state_view( state.lua_state( ) );
			auto state_ptr = lua[ XorStr( "__lua_state" ) ].get<std::shared_ptr<luaState_t>>( );

			auto item = new labelItem;

			state_ptr->ui_items.emplace_back( name, item );

			return sol::make_object( state, item );
		} );

		ui_table[ XorStr( "add_button" ) ] = sol::overload( [ ] ( sol::this_state state, std::string name ) {
			auto lua = sol::state_view( state.lua_state( ) );
			auto state_ptr = lua[ XorStr( "__lua_state" ) ].get<std::shared_ptr<luaState_t>>( );

			auto item = new buttonItem;

			state_ptr->ui_items.emplace_back( name, item );

			return sol::make_object( state, item );
		} );

		ui_table[ XorStr( "add_textbox" ) ] = sol::overload( [ & ] ( sol::this_state state, std::string name ) {
			auto lua = sol::state_view( state.lua_state( ) );
			auto state_ptr = lua[ XorStr( "__lua_state" ) ].get<std::shared_ptr<luaState_t>>( );

			auto item = new textBoxItem;

			auto temp = name;
			item->init( temp.append( state_ptr->script_name ) );

			state_ptr->ui_items.emplace_back( name, item );

			return sol::make_object( state, item );
		} );

		// TODO: float slider...
		ui_table[ XorStr( "get" ) ] = sol::overload( &ui_get, &ui_get_group );
		ui_table[ XorStr( "get_rage" ) ] = sol::overload( &ui_current_weap_config );
		ui_table[ XorStr( "is_open" ) ] = sol::overload( &ui_is_open );

		// now load the script like a real men.
		auto pathToFolder = path;
		auto pathToFile = pathToFolder.append( script_name );

		std::string decoded = XorStr( "" );
		auto result = state->state.load_file( pathToFile );
		decoded = XorStr( "" );

		// oo, no bueno :/
		if( !result.valid( ) ) {
			sol::error err = result;

			// print eror.
			print_error( err.what( ) );

			// bruh, we done here..
			loadingscript = false;
			return false;
		}
		else {
			// ok now that we loaded shit, let's call (xaxaxaxaxa)
			auto call_result = result.call( );

			// oo, no bueno :/
			if( !call_result.valid( ) ) {
				sol::error err = call_result;

				// print eror.
				print_error( err.what( ) );

				// bruh, we done here..
				loadingscript = false;
				return false;
			}

			if( !test ) {
				// now EMPLACE the fucking script.............
				states.emplace_back( script_name, state );

				for( auto &child : g_Vars.m_children ) {
					if( child->GetName( ) == hash_32_fnv1a_const( XorStr( "lua" ) ) ) {
						child->Load( g_Vars.m_json[ std::to_string( child->GetName( ) ) ] );
					}
				}
			}

			loadingscript = false;
			return true;
		}

		return false;
	}

	void Script::UnloadScript( std::string name ) {
		const auto it = std::find_if( states.begin( ), states.end( ), [ &name ] ( const auto &it ) -> bool {
			return it.first == name;
		} );

		GUI::ctx->FocusedID = 0;
		GUI::ctx->MultiDropdownInfo.HashedID = 0;
		GUI::ctx->DropdownInfo.HashedID = 0;

		if( it != states.end( ) ) {
			for( auto &item : it->second->ui_items ) {
				if( item.second->get_type( ) == HOTKEY ) {
					auto hurensohn = std::find( g_keybinds.begin( ), g_keybinds.end( ), reinterpret_cast< hotkeyItem * >( item.second )->hotkey );
					auto hurensohn2 = std::find_if( g_KeybindNames.begin( ), g_KeybindNames.end( ), [ & ] ( const auto &it ) -> bool {
						return it.first == reinterpret_cast< hotkeyItem * >( item.second )->hotkey;
					} );

					if( hurensohn != g_keybinds.end( ) )
						g_keybinds.erase( hurensohn );

					if( hurensohn2 != g_KeybindNames.end( ) )
						g_KeybindNames.erase( hurensohn2 );
				}
			}

			states.erase( it );
		}
	}

	bool Script::IsScriptLoaded( std::string name ) {
		const auto it = std::find_if( states.begin( ), states.end( ), [ &name ] ( const auto &it ) -> bool {
			return it.first == name;
		} );

		return it != states.end( );
	}

	std::vector<std::string> GetScripts( ) {
		static bool created_paths = true;
		if( created_paths ) {
			namespace fs = std::filesystem;
			fs::path full_path( fs::current_path( ) );
			std::wstring str = full_path.wstring( ) + XorStr( L"\\naphack" );

			CreateDirectoryW( str.c_str( ), nullptr );
			str += XorStr( L"\\scripts" );
			CreateDirectoryW( str.c_str( ), nullptr );

			created_paths = false;
		}

		std::string file_extension = XorStr( ".LUA" );
		std::vector<std::string> names;

		WIN32_FIND_DATAA find_data;
		HANDLE preset_file = FindFirstFileA( ( XorStr( "naphack\\scripts\\*" ) + file_extension ).c_str( ), &find_data );

		if( preset_file != INVALID_HANDLE_VALUE ) {
			do {
				if( find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
					continue;

				std::string s = find_data.cFileName;
				int pos = s.find( XorStr( ".lua" ) );

				// not found, NIGGUH
				if( pos == -1 )
					continue;

				s.erase( s.begin( ) + pos, s.begin( ) + pos + 5 );

				std::wstring wstr( s.begin( ), s.end( ) );

				names.push_back( Math::WideToMultiByte( wstr ) );
			} while( FindNextFileA( preset_file, &find_data ) );

			FindClose( preset_file );
		}

		return names;
	}

	void OpenFolder( ) {
		namespace fs = std::filesystem;
		fs::path full_path( fs::current_path( ) );

		std::wstring str = full_path.wstring( ) + XorStr( L"\\naphack\\scripts" );

		PIDLIST_ABSOLUTE pidl;
		if( SUCCEEDED( SHParseDisplayName( str.c_str( ), 0, &pidl, 0, 0 ) ) ) {
			// we don't want to actually select anything in the folder, so we pass an empty
			// PIDL in the array. if you want to select one or more items in the opened
			// folder you'd need to build the PIDL array appropriately
			ITEMIDLIST idNull = { 0 };
			LPCITEMIDLIST pidlNull[ 1 ] = { &idNull };
			SHOpenFolderAndSelectItems( pidl, 1, pidlNull, 0 );

			// LIFEEEEHAAAACK BITCH!!! (◣_◢)
			using ILFree_t = void( __stdcall * )( LPITEMIDLIST );
			static ILFree_t ILFree_fn = ( ILFree_t )GetProcAddress( GetModuleHandleA( XorStr( "SHELL32" ) ), XorStr( "ILFree" ) );
			ILFree_fn( pidl );
		}
	}

	void ReloadActiveScripts( ) {
		auto temp = states;

		// first we save the current lua data
		// so it will be loaded/applied again when a script is loaded.
		for( auto &child : g_Vars.m_children ) {
			if( child->GetName( ) == hash_32_fnv1a_const( XorStr( "lua" ) ) ) {
				child->Save( );

				auto json = child->GetJson( );
				g_Vars.m_json[ std::to_string( child->GetName( ) ) ] = ( json );
			}
		}

		for( auto script : temp ) {
			std::string xd = XorStr( "" );
			Script::LoadScript( script.first, xd );
		}
	}

	void UnloadActiveScripts( ) {
		auto temp = states;

		for( auto script : temp ) {
			Script::UnloadScript( script.first );
		}
	}

#endif
}