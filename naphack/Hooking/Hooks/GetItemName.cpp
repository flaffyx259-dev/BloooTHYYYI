#include "../hooked.hpp"
#include "../../SDK/Classes/weapon.hpp"
#include "../../SDK/Classes/entity.hpp"
#include "../../SDK/Classes/player.hpp"

wchar_t* __fastcall Hooked::GetItemName( void* ecx, void* edx, int a2 ) {
	if( !ecx )
		return nullptr;

	if( g_Vars.globals.m_bDontCallingTheSkinNameMaybe )
		a2 = true;

	auto ret = oGetItemName( ecx, a2 );
	return ret;
}