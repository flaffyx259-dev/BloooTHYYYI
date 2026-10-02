#include "../../pandora.hpp"
#include "../Hooked.hpp"
#include "../../SDK/Displacement.hpp"
#include "../../SDK/Classes/Player.hpp"
#include "../../Features/Rage/ServerAnimations.hpp"

void __fastcall Hooked::ModifyEyePosition( CCSGOPlayerAnimState* ecx, void* edx, Vector* eye_position ) {
	//g_Vars.globals.szLastHookCalled = XorStr( "40" );
	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !ecx || !pLocal || !eye_position ) {
		return oModifyEyePosition( ecx, eye_position );
	}

	return ecx->ModifyEyePosition( eye_position );
}