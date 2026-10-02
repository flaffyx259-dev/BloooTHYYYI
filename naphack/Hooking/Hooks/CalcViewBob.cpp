#include "../../pandora.hpp"
#include "../Hooked.hpp"
#include "../../SDK/Displacement.hpp"
#include "../../SDK/Classes/Player.hpp"
#include "../../Features/Rage/ServerAnimations.hpp"

void __fastcall Hooked::CalcViewBob( C_BasePlayer* player, void* edx, Vector& eyeOrigin ) {
	return;
}

void __fastcall Hooked::CalcView( C_CSPlayer *pPlayer, void *edx, Vector &vecEyeOrigin, QAngle &angEyeAngles, float &flZNear, float &flZFar, float &flFov ) {
	auto pEntity = ( C_CSPlayer * )pPlayer;
	if( pEntity->EntIndex( ) != g_pEngine->GetLocalPlayer( ) )
		return oCalcView( pPlayer, vecEyeOrigin, angEyeAngles, flZNear, flZFar, flFov );

	// prevent client's ModifyEyePos from being called
	// https://i.imgur.com/LMYq6Jf.png

	const bool m_bUseNewAnimstate = *( int * )( ( uintptr_t )pPlayer + 0x39E1 );
	*( bool * )( ( uintptr_t )pPlayer + 0x39E1 ) = false;
	oCalcView( pPlayer, vecEyeOrigin, angEyeAngles, flZNear, flZFar, flFov );
	*( bool * )( ( uintptr_t )pPlayer + 0x39E1 ) = m_bUseNewAnimstate;

	// firstperson radar fix: v_angle is REAL for the render window so the
	// radar draws REAL, but CalcView builds eye angles (camera + viewmodel /
	// hands) from it, so both would swing to real. pull eye yaw back to
	// fake with the pure stored delta - radar keeps reading v_angle
	// (untouched), hands stay static where YOU look. only yaw was swapped.
	// NOTE: delta MUST come from stored pure values, never from reading
	// v_angle back here: RemoveVisualEffects subtracts punch from v_angle
	// between START and CalcView, a read-back is punch-polluted and leaks
	// recoil into the eye (camera jerk on shoot/landing).
	if( g_ServerAnimations.m_bRadarVAngleSwapped
		&& g_Vars.misc.radar_real_yaw
		&& !pEntity->IsDead( )
		&& !g_pInput->m_fCameraInThirdPerson ) {
		const float flDelta = g_ServerAnimations.m_flRadarSwapRealYaw - g_ServerAnimations.m_angRadarSavedVAngle.y;
		angEyeAngles.y = Math::AngleNormalize( angEyeAngles.y - flDelta );
	}
}