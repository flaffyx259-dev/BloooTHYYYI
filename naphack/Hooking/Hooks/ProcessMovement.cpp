#include "../Hooked.hpp"
#include "../../SDK/Classes/Player.hpp"
#include "../../SDK/Displacement.hpp"
#include "../../Features/Rage/Animations.hpp"

void __fastcall Hooked::ProcessMovement( void* ecx, void* edx, C_CSPlayer* a3, CMoveData* a4 ) {
	auto pEncMoveData = Encrypted_t( a4 );

	if( pEncMoveData.IsValid( ) )
		pEncMoveData->m_bGameCodeMovedPlayer = false;

	oProcessMovement( ecx, a3, pEncMoveData.Xor( ) );

	/*if( ecx && g_Simulation.m_pGameMovement != ( IGameMovement * )ecx )
		g_Simulation.m_pGameMovement = ( IGameMovement * )ecx;*/
}