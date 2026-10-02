#pragma once
#include "../../Utils/extern/XorStr.hpp"
#include <Windows.h>
#include <Winternl.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <string>

#include "../../SDK/variables.hpp"
#include "../Exports.h"

enum protection_status_t
{
	STATUS_SAFE,
	STATUS_UNSAFE_SYSTEM,

	STATUS_WINAPI_DEBUGGER,
	STATUS_WINAPI_REMOTE_DEBUGGER,
	STATUS_PEB_DEBUGGER,
	STATUS_THREAD_DEBUGGER,
	STATUS_THREAD_REGISTER_DEBUGGER,
	STATUS_KUSER_KERNEL_DEBUGGER,

	STATUS_PROGRAM_NOSNAPSHOT,
	STATUS_PROGRAM_EBUG,
	STATUS_PROGRAM_DBG,

	STATUS_DRIVER_NOENUM,
	STATUS_DRIVER_SANDBOXIE,
	STATUS_DRIVER_WINPCAP,
	STATUS_DRIVER_PROCESS_HACKER,
	STATUS_DRIVER_CHEAT_ENGINE,
};

static constexpr uintptr_t KUSER_SHARED_DATA = 0x7FFE0000;

#include "../../Utils/extern/lazy_importer.hpp"

class CProtection {
public:
	std::string error_string;
	char* random_data;

	__forceinline uint32_t crc_rt( const uint8_t* p, size_t size ) {
		size_t s = size;
		size_t result = 0;
		const size_t prime = 31;
		for( size_t i = 0; i < s; ++i )
			result = p[ i ] + ( result * prime );

		return result;
	}

	__forceinline bool check_idk( ) {
#ifdef DEV
		return false;
#endif

		static const auto text_hash = crc_rt( ( uint8_t* )g_text_start, g_text_size );

		if( text_hash != g_text_hash ) {
			g_is_cracked = true;
			return true;
		}

		if( *( uint16_t* )g_Vars.globals.hModule == IMAGE_DOS_SIGNATURE ) {
			g_is_cracked = true;
			return true;
		}

		//if( ( ( IMAGE_DOS_HEADER* )g_Vars.globals.hModule )->e_lfanew != 0 ) {
		//	g_is_cracked = true;
		//	return true;
		//}

		return g_is_cracked;
	}

	__forceinline bool check_round( ) {
#ifdef DEV
		return false;
#endif

		static const auto text_hash = crc_rt( ( uint8_t* )g_text_start, g_text_size );

		if( text_hash != g_text_hash ) {
			g_is_cracked = true;
			return true;
		}

		if( *( uint16_t* )g_Vars.globals.hModule == IMAGE_DOS_SIGNATURE ) {
			g_is_cracked = true;
			return true;
		}

		//if( ( ( IMAGE_DOS_HEADER* )g_Vars.globals.hModule )->e_lfanew != 0 ) {
		//	g_is_cracked = true;
		//	return true;
		//}

		return g_is_cracked;
	}

	__forceinline bool check_start( ) {
#ifdef DEV
		return false;
#endif

		static const auto text_hash = crc_rt( ( uint8_t* )g_text_start, g_text_size );

		if( text_hash != g_text_hash ) {
			g_is_cracked = true;
			return true;
		}

		if( *( uint16_t* )g_Vars.globals.hModule == IMAGE_DOS_SIGNATURE ) {
			g_is_cracked = true;
			return true;
		}

		//if( ( ( IMAGE_DOS_HEADER* )g_Vars.globals.hModule )->e_lfanew != 0 ) {
		//	g_is_cracked = true;
		//	return true;
		//}

		return g_is_cracked;
	}
};

extern CProtection g_protection;