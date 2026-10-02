#include "CCSGameRules.hpp"
#include "../Displacement.hpp"

bool CCSGameRules::m_bIsValveDS( ){
	if( !this )
		return false;

	return *( bool* )( ( uintptr_t )this + 0x75 );
}

bool CCSGameRules::m_bFreezePeriod( ){
	if( !this )
		return false;
	
	return *( bool* )( ( uintptr_t )this + 0x20 );
}

float CCSGameRules::m_flRestartRoundTime( ){
	if( !this )
		return 0.f;
	
	return *( float* )( ( uintptr_t )this + 0x50 );
}

int CCSGameRules::m_totalRoundsPlayed( ) {
	if( !this )
		return 0;

	return *( int * )( ( uintptr_t )this + 0x64 );
}