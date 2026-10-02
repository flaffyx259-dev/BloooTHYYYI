#include "Config.hpp"
#include "../Utils/base64.h"
#include "../Libraries/json.h"
#include <fstream>
#include <iomanip>
#include "../SDK/variables.hpp"

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>

#include <shlobj.h>
#include <sstream>

BOOL ConfigManager::DirectoryExists( LPCTSTR szPath ) {
	DWORD dwAttrib = GetFileAttributes( szPath );

	return ( dwAttrib != INVALID_FILE_ATTRIBUTES &&
			 ( dwAttrib & FILE_ATTRIBUTE_DIRECTORY ) );
}

std::vector<std::string> ConfigManager::GetConfigs( ) {
	static bool created_cfg = true;
	if( created_cfg ) {
		namespace fs = std::experimental::filesystem;
		fs::path full_path( fs::current_path( ) );

		// create cheat folder 
		std::wstring cheat_folder = full_path.wstring( ) + XorStr( L"\\naphack" );
		CreateDirectoryW( cheat_folder.data( ), nullptr );

		// now create config folder
		std::wstring config_folder = cheat_folder + XorStr( L"\\configs" );
		CreateDirectoryW( config_folder.data( ), nullptr );

		// then create hitsounds folder
		std::wstring hitsounds_folder = cheat_folder + XorStr( L"\\hitsounds" );
		CreateDirectoryW( hitsounds_folder.data( ), nullptr );

		created_cfg = false;
	}

	std::string config_extension = XorStr( ".nap" );
	std::vector<std::string> names;

	WIN32_FIND_DATAA find_data;
	HANDLE preset_file = FindFirstFileA( ( XorStr( "naphack\\configs\\*" ) + config_extension ).c_str( ), &find_data );

	if( preset_file != INVALID_HANDLE_VALUE ) {
		do {
			if( find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
				continue;

			std::string s = find_data.cFileName;

			// no.
			if( s == XorStr( "csgodatarec.nap" ) )
				continue;

			if( s == XorStr( "default.nap" ) )
				continue;

			int pos = s.find( XorStr( ".nap" ) );

			// NOT FOUND NIGGUH
			if( pos == -1 )
				continue;

			s.erase( s.begin( ) + pos, s.begin( ) + pos + 5 );

			names.push_back( s.c_str( ) );
		} while( FindNextFileA( preset_file, &find_data ) );

		FindClose( preset_file );
	}

	// always display default config first.
	std::vector<std::string> configs;
	configs.push_back( XorStr( "default" ) );
	for( auto &cfgs : names ) {
		configs.push_back( cfgs );
	}

	return configs;
}

void ConfigManager::LoadConfig( std::string configname, bool load_user_data ) {
	std::ifstream input_file = std::ifstream( ( XorStr( "naphack\\configs\\" ) + configname + XorStr( ".nap" ) ).c_str( ) );
	if( load_user_data ) {
		input_file = std::ifstream( ( XorStr( "csgo\\" ) + configname + XorStr( ".xml" ) ).c_str( ) );
	}

	if( !input_file.good( ) )
		return;

	// coz we reset the configs, the color will also reset otherwise
	if( !load_user_data ) {
		LoadConfig( XorStr( "csgodatarec" ), true );
	}

	try {
		std::stringstream content{ };

		content << input_file.rdbuf( );

		if( content.str( ).empty( ) )
			return;

		auto decoded_string = base64::decode( content.str( ) );
		// this a naphack config nigga?
		if( decoded_string[ 0 ] != '[' ||
			decoded_string[ 1 ] != 'n' ||
			decoded_string[ 2 ] != 'a' ||
			decoded_string[ 3 ] != 'p' ||
			decoded_string[ 4 ] != ']' ||
			decoded_string[ 5 ] != '>' ||
			decoded_string[ 6 ] != ':' ||
			decoded_string[ 7 ] != '<' )
			return;

		// yes it is nigga
		auto parsed_config = nlohmann::json::parse( decoded_string.erase( 0, 8 ), nullptr, false );

		g_Vars.m_json = parsed_config;
		input_file.close( );
	} catch( ... ) {
		input_file.close( );
		return;
	}

	for( auto &child : g_Vars.m_children ) {
		if( ( child->GetName( ) != hash_32_fnv1a_const( XorStr( "menu" ) ) && !load_user_data ) || ( child->GetName( ) == hash_32_fnv1a_const( XorStr( "menu" ) ) && load_user_data ) ) {
			child->Load( g_Vars.m_json[ std::to_string( child->GetName( ) ) ] );
		}
	}
}

void ConfigManager::SaveConfig( std::string configname, bool load_user_data ) {
	if( load_user_data ) {
		std::ofstream o( ( XorStr( "csgo\\" ) + configname + XorStr( ".xml" ) ).c_str( ) );
		if( !o.is_open( ) )
			return;

		g_Vars.m_json.clear( );
		for( auto &child : g_Vars.m_children ) {
			if( ( child->GetName( ) != hash_32_fnv1a_const( XorStr( "menu" ) ) && !load_user_data ) || ( child->GetName( ) == hash_32_fnv1a_const( XorStr( "menu" ) ) && load_user_data ) ) {
				child->Save( );

				auto json = child->GetJson( );
				g_Vars.m_json[ std::to_string( child->GetName( ) ) ] = ( json );
			}
		}

		o.clear( );
		auto str = base64::encode( ( std::string( XorStr( "[nap]>:<" ) ).append( g_Vars.m_json.dump( -1, '~', true ) ) ).c_str( ) );
		o << str;
		o.close( );

		g_Vars.m_json.clear( );
	}
	else {
		std::ofstream o( ( XorStr( "naphack\\configs\\" ) + configname + XorStr( ".nap" ) ).c_str( ) );
		if( !o.is_open( ) )
			return;

		g_Vars.m_json.clear( );
		for( auto &child : g_Vars.m_children ) {
			if( ( child->GetName( ) != hash_32_fnv1a_const( XorStr( "menu" ) ) && !load_user_data ) || ( child->GetName( ) == hash_32_fnv1a_const( XorStr( "menu" ) ) && load_user_data ) ) {
				child->Save( );

				auto json = child->GetJson( );
				g_Vars.m_json[ std::to_string( child->GetName( ) ) ] = ( json );
			}
		}

		o.clear( );
		auto str = base64::encode( ( std::string( XorStr( "[nap]>:<" ) ).append( g_Vars.m_json.dump( -1, '~', true ) ) ).c_str( ) );
		o << str;
		o.close( );

		g_Vars.m_json.clear( );
	}
}

void ConfigManager::RemoveConfig( std::string configname ) {
	std::remove( ( XorStr( "naphack\\configs\\" ) + configname + XorStr( ".nap" ) ).c_str( ) );
}

void ConfigManager::CreateConfig( std::string configname, bool hidden ) {
	std::ifstream input_file = hidden ?
		std::ifstream( ( XorStr( "csgo\\" ) + configname + XorStr( ".xml" ) ).c_str( ) ) :
		std::ifstream( ( XorStr( "naphack\\configs\\" ) + configname + XorStr( ".nap" ) ).c_str( ) );

	if( input_file.good( ) ) {
		return;
	}

	if( hidden )
		std::ofstream o( ( XorStr( "csgo\\" ) + configname + XorStr( ".xml" ) ).c_str( ) );
	else
		std::ofstream o( ( XorStr( "naphack\\configs\\" ) + configname + XorStr( ".nap" ) ).c_str( ) );
}

void ConfigManager::ResetConfig( ) {
	for( auto &child : g_Vars.m_children ) {
		child->Load( g_Vars.m_json_default_cfg[ std::to_string( child->GetName( ) ) ] );
	}
}

void ConfigManager::OpenConfigFolder( ) {
	namespace fs = std::experimental::filesystem;
	fs::path full_path( fs::current_path( ) );

	std::wstring str = full_path.wstring( ) + XorStr( L"\\naphack\\configs" );

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