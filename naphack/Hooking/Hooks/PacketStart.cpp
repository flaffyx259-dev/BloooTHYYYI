#pragma once
#include "../Hooked.hpp"
#include "../../pandora.hpp"
#include "../../Features/Rage/EnginePrediction.hpp"
#include <vector>
#include "../../SDK/Classes/Player.hpp"

void __fastcall Hooked::PacketStart( void *ecx, void *edx, int incoming, int outgoing ) {
	if( !g_pEngine->IsConnected( ) || !g_pEngine->IsInGame( ) )
		return oPacketStart( ecx, incoming, outgoing );

	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal || pLocal->IsDead( ) )
		return oPacketStart( ecx, incoming, outgoing );

	if( g_Prediction.fake_datagram )
		outgoing = g_pClientState->last_command_ack;

	if( std::find( g_Prediction.last_outgoing_commands.begin( ), g_Prediction.last_outgoing_commands.end( ), outgoing ) != g_Prediction.last_outgoing_commands.end( ) )
		oPacketStart( ecx, incoming, outgoing );

	// no c++20 :-/
	/*std::erase_if( g_Prediction.last_outgoing_commands, [ & ] ( int command ) {
		return std::abs( command - outgoing ) >= 150 || command < outgoing;
	} );*/

	g_Prediction.last_outgoing_commands.erase(
		std::remove_if( g_Prediction.last_outgoing_commands.begin( ), g_Prediction.last_outgoing_commands.end( ), [ & ] ( int command ) {
		return std::abs( command - outgoing ) >= 150 || command < outgoing;
	} ), g_Prediction.last_outgoing_commands.end( ) );
}

void __fastcall Hooked::PacketEnd( void *ecx, void * ) {
	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( pLocal && !pLocal->IsDead( ) ) {
		if( g_pClientState->clock_drift_mgr.m_nServerTick == g_pClientState->delta_tick )
			g_Prediction.fix_netvars( ( ( CClientState * )ecx )->last_command_ack );
	}

	oPacketEnd( ecx );
}