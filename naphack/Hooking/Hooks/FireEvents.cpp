#include "../hooked.hpp"
#include "../../SDK/Classes/Player.hpp"
#include "../../Features/Rage/FakePing.hpp"

struct CEventInfo {
	uint16_t classID;
	char pad_0002[ 2 ];
	float fire_delay;
	char pad_0008[ 4 ];
	void *pClientClass;
	void *pData;
	char pad_0014[ 36 ];
	CEventInfo *next;
	char pad_0038[ 8 ];
};

void __fastcall Hooked::FireEvents( void *ecx, void *edx ) {

	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( pLocal ) {
		for( CEventInfo *i = *( CEventInfo ** )( uintptr_t( g_pClientState.Xor( ) ) + 0x4DEC );
			 i != nullptr;
			 i = i->next ) {
			if( !i->classID )
				continue;

			if( i->classID == CTEFireBullets || i->classID - 1 == CTEFireBullets )
				i->fire_delay = 0.0f;
		}
	}

	oFireEvents( ecx );
}

void __fastcall Hooked::ProcessPacket( void *ecx, void *edx, void *packet, bool header ) {

	g_FakePing.m_bInProcessPacket = true;
	oProcessPacket( ecx, packet, header );
	g_FakePing.m_bInProcessPacket = false;

	g_FakePing.UpdateIncomingSequences( reinterpret_cast< INetChannel * >( ecx ) );

	// fire the events.
	g_pEngine->FireEvents( );
}

