#include "IWeaponSystem.hpp"

CCSWeaponInfo* IWeaponSystem::GetWeaponInfo( int nItemDefinitionIndex ) {
	using GetWeaponInfoFn = CCSWeaponInfo * ( __thiscall* )( IWeaponSystem*, int );
	return ( *( GetWeaponInfoFn** )this )[ 2 ]( this, nItemDefinitionIndex );
}