#pragma once

#include "auto.hpp"
#include "../Classes/DataMap.hpp"

#include <functional>
#include <thread>
#include <chrono>
#include <signal.h>

#include "../../Utils/extern/XorStr.hpp"
#include "../../Utils/extern/lazy_importer.hpp"
#include "../../Loader/Exports.h"
#include "../variables.hpp"

#define SDK_concat(x, y) x##y
#define SDK_concatiate(x, y) SDK_concat(x, y)
#define SDK_pad(size)                                         \
private:                                                          \
  std::uint8_t SDK_concatiate(__pad, __COUNTER__)[size] = {}; \
                                                                  \
public:

namespace SDK::Memory {
	inline HWND hTheFuckingWindow;

	char *find_utf8( const char *module, const char *string );
	void strcpy_protected( char *dest, const char *src );
	std::uintptr_t Scan( const std::string &image_name, const std::string &signature, bool bRequest = true );

	void RequestAddr( const uint32_t hash );

#if 0
	__forceinline std::uintptr_t GetAddress( const std::pair< uint32_t, std::string> hash ) {
	#ifdef BETA_MODE
		//const auto xd = g_Vars.globals.szLastHookCalled.data( );
	#endif

		const auto old_hash = hash.first;
		//RequestAddr( hash.first );

		int *npt = nullptr;

		if( old_hash == hash.first && !g_is_cracked ) {
			const auto address = patterns_get_pattern( hash.first );
			if( !address || address < 0 ) {
				LI_FN( MessageBoxA )( hTheFuckingWindow, hash.second.data( ), XorStr( "error 102 " ), MB_OK );
			#if 0
				// ok dude i do not even know
				// confusing the confused LOL
				while( true || g_is_cracked || !g_is_cracked || !npt ) {
					const auto result2 = LI_FN( MessageBoxA )( hTheFuckingWindow, hash.second.data( ), XorStr( "error 102 " ), MB_OK );

					while( true ) {
						if( result2 || g_is_cracked ) {
							LI_FN( raise )( SIGSEGV );
						}

						LI_FN( raise )( SIGSEGV );
					}

					LI_FN( raise )( SIGSEGV );
				}
			#endif
			}

			return address;
		}
	}
#endif

	std::uintptr_t Scan( const std::uintptr_t image, const std::string &signature, bool bRequest = true );
	uintptr_t *GetVirtualTablePointer( const char *ModuleName, const char *TableName );
	unsigned int FindInDataMap( datamap_t *pMap, const char *name );
	// we have like 5 lambdas for this in random places
	DWORD CallableFromRelative( DWORD nAddress );

	inline void ThreadSleep( DWORD msecs ) {
		std::this_thread::sleep_for( std::chrono::milliseconds( msecs ) );
	}

	template <typename T>
	inline bool ResolveSymbol( T *out, const std::function<T( )> &fn ) {
		unsigned int i = 0;
		while( true ) {
			*out = fn( );
			if( *out ) {
				break;
			}

			i++;

			//10 seconds
			if( i > 200 ) {
				return false;
			}

			ThreadSleep( 50 );
		}

		return true;
	}
}
