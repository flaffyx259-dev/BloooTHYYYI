#pragma once
#include <cstdint>
#include <windows.h>
#include <array>
#include <string>
#include "../Utils/extern/lazy_importer.hpp"
#include "../Utils/extern/XorStr.hpp"

#ifndef PDR_EXPORTS
#define PDR_EXPORTS

struct netvar_t {
	uint32_t hash = 0; // for reading it in the cheat

	uint32_t offset = 0;
};

struct netvars_t {
	int count = 0;
	int magic = 0;
	netvar_t netvars[ 20000 ];
};

extern __declspec( dllexport ) netvars_t g_netvars;

inline uint32_t netvar_get_offset( uint32_t hash, const char *name = nullptr, int name2 = 0 ) {
	for( int i = 0; i < g_netvars.count; i++ ) {
		if( g_netvars.netvars[ i ].hash == hash + g_netvars.magic ) {
			return g_netvars.netvars[ i ].offset;
		}
	}
}

struct interface_t {
	uint32_t hash = 0;
	uint32_t address = 0;
};

struct interfaces_t {
	int count = 0;
	interface_t ifaces[ 100 ];
};

extern __declspec( dllexport ) interfaces_t g_interfaces;

inline uint32_t interfaces_get_interface( uint32_t hash ) {
	for( int i = 0; i < g_interfaces.count; i++ ) {
		auto iface = g_interfaces.ifaces[ i ];

		//  hwid::hwid_data_t data;
		if( iface.hash == hash ) {
			iface.hash = 0x1337;
			return ( iface.address /*^ hwid::get_hwid_data( data )*/ ) ^ static_cast< int >( LI_FN( GetCurrentProcessId )( ) );
		}
	}

	return 0;
}

struct pattern_t {
	char module[ 64 ];
	char pattern[ 128 ];
	uint32_t hash;
	uint32_t address;
};

struct patterns_t {
	int count = 100;
	pattern_t patterns[ 250 ];
};

extern __declspec( dllexport ) patterns_t g_patterns;

inline uint32_t patterns_get_pattern( uint32_t hash ) {
	for( int i = 0; i < g_patterns.count; i++ ) {

		// hwid::hwid_data_t data;
		if( g_patterns.patterns[ i ].hash == hash ) {

			// g_patterns.patterns[ i ].hash = 0x1337;
			return ( g_patterns.patterns[ i ].address /*^ hwid::get_hwid_data( data )*/ ) ^ static_cast< int >( LI_FN( GetCurrentProcessId )( ) );
		}
	}

	return 0;
}

__forceinline std::pair< uint32_t, std::string> loader_hash( const char *p ) {
	size_t s = strlen( p ); // users dont have sse4 processors etc
	size_t result = 0;
	const size_t prime = 6969;
	for( size_t i = 0; i < s; ++i )
		result = p[ i ] + ( result * prime );

	return { result, p };
}

__forceinline std::array<float, 128u> get_array_from_string( const std::string &input ) {
	if( input.empty( ) )
		return { }; // LOL SO LAZY !

	std::array<float, 128u> computed_array{ };
	for( size_t i{ 0 }; i < input.length( ) && i < 128; ++i ) {
		if( input.length( ) > 128 && i >= 125 ) {
			computed_array.at( i ) = static_cast< float >( static_cast < int >( '.' ) );
		}
		else {
			computed_array.at( i ) = static_cast< float >( static_cast< int >( input.at( i ) ) );
		}
	}

	return computed_array;
}

__forceinline std::string get_string_from_array( const std::array<float, 128u> &arr ) {
	std::string computed_string{ };

	for( auto i : arr ) {
		if( i ) {
			computed_string += static_cast< char >( i );
		}
	}

	return computed_string;
}

extern __declspec( dllexport ) uint32_t g_text_start;
extern __declspec( dllexport ) uint32_t g_text_size;
extern __declspec( dllexport ) uint32_t g_text_hash;

extern bool g_is_cracked;
extern bool g_initialised;

#endif