#include "CrashHandler.hpp"
#include "LogSystem.hpp"
#include <winternl.h>

ICrashHandler g_CrashHandler;

long __stdcall ICrashHandler::OnProgramCrash( struct _EXCEPTION_POINTERS *ExceptionInfo ) {
	struct _LDR_DATA_TABLE_ENTRY_ { // nice undocumented struct nn
		PVOID Reserved1[ 2 ];
		LIST_ENTRY InMemoryOrderLinks;
		PVOID Reserved2[ 2 ];
		PVOID DllBase;
		PVOID Reserved3[ 2 ];
		UNICODE_STRING FullDllName;
		UNICODE_STRING BaseDllName;
	};

	auto peb = reinterpret_cast< PEB * >( reinterpret_cast< TEB * >( __readfsdword( 0x18 ) )->ProcessEnvironmentBlock );

	auto module_list = &peb->Ldr->InMemoryOrderModuleList;

	static std::wstring lastExceptionDLL;
	static uintptr_t lastExceptionAddr;

	for( auto i = module_list->Flink; i != module_list; i = i->Flink ) {
		auto entry = CONTAINING_RECORD( i, _LDR_DATA_TABLE_ENTRY_, InMemoryOrderLinks );
		if( !entry )
			continue;

		auto dos = reinterpret_cast< IMAGE_DOS_HEADER * >( entry->DllBase );
		auto nt = reinterpret_cast< IMAGE_NT_HEADERS * >( ( uintptr_t )entry->DllBase + dos->e_lfanew );

		if( ( uintptr_t )ExceptionInfo->ExceptionRecord->ExceptionAddress >= ( uintptr_t )entry->DllBase &&
			( uintptr_t )ExceptionInfo->ExceptionRecord->ExceptionAddress <= ( uintptr_t )entry->DllBase + nt->OptionalHeader.SizeOfImage ) {

			auto ecxAddr = ( uintptr_t )ExceptionInfo->ExceptionRecord->ExceptionAddress - ( uintptr_t )entry->DllBase;
			if( lastExceptionDLL.find( entry->BaseDllName.Buffer ) == std::string::npos || ecxAddr != lastExceptionAddr ) {
				g_Log.Log( XorStr( ".nap" ), XorStr( "%ws: exception caught at address 0x%08X" ),
						   entry->BaseDllName.Buffer,
						   ecxAddr );
			}

			lastExceptionDLL = entry->BaseDllName.Buffer;
			lastExceptionAddr = ecxAddr;


			return EXCEPTION_CONTINUE_SEARCH;
		}
	}
	g_Log.Log( XorStr( ".nap" ), XorStr( "blootoothYzio: exception caught at address 0x%08X" ),
			   ( uintptr_t )ExceptionInfo->ExceptionRecord->ExceptionAddress - ( uintptr_t )g_Vars.globals.hModule );

	std::stringstream strHackCrashLog;
	int iExceptionType = ExceptionInfo->ExceptionRecord->ExceptionInformation[ 0 ];
	int iExceptionAddress = ExceptionInfo->ExceptionRecord->ExceptionInformation[ 1 ];
	switch( ExceptionInfo->ExceptionRecord->ExceptionCode ) {
		case EXCEPTION_ACCESS_VIOLATION:
			if( iExceptionType == 0 ) {
				// bad read
				g_Log.Log( XorStr( ".nap" ), XorStr( "EXCEPTION_ACCESS_VIOLATION (attempted to read from: 0x%08x)" ), iExceptionAddress );
			}
			else if( iExceptionType == 1 ) {
				// bad write
				g_Log.Log( XorStr( ".nap" ), XorStr( "EXCEPTION_ACCESS_VIOLATION (attempted to write to: 0x%08x)" ), iExceptionAddress );
			}
			else if( iExceptionType == 8 ) {
				// user-mode data execution prevention (DEP)
				g_Log.Log( XorStr( ".nap" ), XorStr( "EXCEPTION_ACCESS_VIOLATION (dep: 0x%08x)" ), iExceptionAddress );
			}
			else {
				// unknown, shouldn't happen
				g_Log.Log( XorStr( ".nap" ), XorStr( "EXCEPTION_ACCESS_VIOLATION (unknown: 0x%08x)" ), iExceptionAddress );
			}
			break;

		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
			g_Log.Log( XorStr( ".nap" ), XorStr( "EXCEPTION_ARRAY_BOUNDS_EXCEEDED" ) );
			break;

		case EXCEPTION_BREAKPOINT:
			g_Log.Log( XorStr( ".nap" ), XorStr( "EXCEPTION_BREAKPOINT" ) );
			break;

		case EXCEPTION_DATATYPE_MISALIGNMENT:
			g_Log.Log( XorStr( ".nap" ), XorStr( "EXCEPTION_DATATYPE_MISALIGNMENT" ) );
			break;

		case EXCEPTION_FLT_DENORMAL_OPERAND:
			g_Log.Log( XorStr( ".nap" ), XorStr( "EXCEPTION_FLT_DENORMAL_OPERAND" ) );
			break;

		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
			g_Log.Log( XorStr( ".nap" ), XorStr( "EXCEPTION_FLT_DIVIDE_BY_ZERO" ) );
			break;

		case EXCEPTION_FLT_INEXACT_RESULT:
			g_Log.Log( XorStr( ".nap" ), XorStr( "EXCEPTION_FLT_INEXACT_RESULT" ) );
			break;

		case EXCEPTION_FLT_INVALID_OPERATION:
			g_Log.Log( XorStr( ".nap" ), XorStr( "EXCEPTION_FLT_INVALID_OPERATION" ) );
			break;

		case EXCEPTION_FLT_OVERFLOW:
			g_Log.Log( XorStr( ".nap" ), XorStr( "EXCEPTION_FLT_OVERFLOW" ) );
			break;

		case EXCEPTION_FLT_STACK_CHECK:
			g_Log.Log( XorStr( ".nap" ), XorStr( "EXCEPTION_FLT_STACK_CHECK" ) );
			break;

		case EXCEPTION_FLT_UNDERFLOW:
			g_Log.Log( XorStr( ".nap" ), XorStr( "EXCEPTION_FLT_UNDERFLOW" ) );
			break;

		case EXCEPTION_ILLEGAL_INSTRUCTION:
			g_Log.Log( XorStr( ".nap" ), XorStr( "EXCEPTION_ILLEGAL_INSTRUCTION" ) );
			break;

		case EXCEPTION_IN_PAGE_ERROR:
			if( iExceptionType == 0 ) {
				// bad read
				g_Log.Log( XorStr( ".nap" ), XorStr( "EXCEPTION_IN_PAGE_ERROR (attempted to read from: 0x%08x)" ), iExceptionAddress );
			}
			else if( iExceptionType == 1 ) {
				// bad write
				g_Log.Log( XorStr( ".nap" ), XorStr( "EXCEPTION_IN_PAGE_ERROR (attempted to write to: 0x%08x)" ), iExceptionAddress );
			}
			else if( iExceptionType == 8 ) {
				// user-mode data execution prevention (DEP)
				g_Log.Log( XorStr( ".nap" ), XorStr( "EXCEPTION_IN_PAGE_ERROR (dep: 0x%08x)" ), iExceptionAddress );
			}
			else {
				// unknown, shouldn't happen
				g_Log.Log( XorStr( ".nap" ), XorStr( "EXCEPTION_IN_PAGE_ERROR (unknown: 0x%08x)" ), iExceptionAddress );
			}
			break;

		case EXCEPTION_INT_DIVIDE_BY_ZERO:
			g_Log.Log( XorStr( ".nap" ), XorStr( "EXCEPTION_INT_DIVIDE_BY_ZERO" ) );
			break;

		case EXCEPTION_INT_OVERFLOW:
			g_Log.Log( XorStr( ".nap" ), XorStr( "EXCEPTION_INT_OVERFLOW" ) );
			break;

		case EXCEPTION_INVALID_DISPOSITION:
			g_Log.Log( XorStr( ".nap" ), XorStr( "EXCEPTION_INVALID_DISPOSITION" ) );
			break;

		case EXCEPTION_NONCONTINUABLE_EXCEPTION:
			g_Log.Log( XorStr( ".nap" ), XorStr( "EXCEPTION_NONCONTINUABLE_EXCEPTION" ) );
			break;

		case EXCEPTION_PRIV_INSTRUCTION:
			g_Log.Log( XorStr( ".nap" ), XorStr( "EXCEPTION_PRIV_INSTRUCTION" ) );
			break;

		case EXCEPTION_SINGLE_STEP:
			g_Log.Log( XorStr( ".nap" ), XorStr( "EXCEPTION_SINGLE_STEP" ) );
			break;

		case EXCEPTION_STACK_OVERFLOW:
			g_Log.Log( XorStr( ".nap" ), XorStr( "EXCEPTION_STACK_OVERFLOW" ) );
			break;

		case DBG_CONTROL_C:
			g_Log.Log( XorStr( ".nap" ), XorStr( "DBG_CONTROL_C" ) );
			break;

		default:
			g_Log.Log( XorStr( ".nap" ), XorStr( "unknown: %08x" ), ExceptionInfo->ExceptionRecord->ExceptionCode );
	}

	return EXCEPTION_CONTINUE_SEARCH;
}
