#include "../../pandora.hpp"
#include "../Hooked.hpp"
#include "../../SDK/Displacement.hpp"
#include "../../SDK/Classes/Player.hpp"
#include "../../Features/Rage/EnginePrediction.hpp"
#include "../../Features/Rage/TickbaseShift.hpp"

void __fastcall Hooked::PhysicsSimulate( void *ecx, void *edx ) {
	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !ecx || !pLocal || pLocal->IsDead( ) || pLocal != ecx )
		return oPhysicsSimulate( ecx );

	int &simulation_tick = *( int * )( ( std::uintptr_t )ecx + 0x2A8 );
	if( g_pGlobalVars->tickcount == simulation_tick || g_pEngine->IsHLTV( ) || pLocal->m_fFlags( ) & 128u ) {
		oPhysicsSimulate( ecx );
		return;
	}

	auto ctx = ( C_CommandContext * )( uintptr_t( ecx ) + 0x34D0 );
	if( !ctx ) {
		return oPhysicsSimulate( ecx );
	}

	if( ( ctx->cmd.buttons & IN_ATTACK ) ||
		( ctx->cmd.buttons & IN_ATTACK2 ) )
		pLocal->m_bIsLookingAtWeapon( ) = false;

	if( ctx->cmd.command_number == g_pClientState->last_outgoing_command + g_pClientState->choked_commands + 1 ) {
		pLocal->m_flVelocityModifier( ) = g_Prediction.velocity_modifier;
	}

	oPhysicsSimulate( ecx );
}
