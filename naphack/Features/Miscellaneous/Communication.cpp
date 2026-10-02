#include "Communication.hpp"
#include "../../SDK/variables.hpp"
#include "../../SDK/Classes/Player.hpp"
#include "../../SDK/Classes/weapon.hpp"
#include "../../SDK/Displacement.hpp"
#include "../Visuals/Visuals.hpp"
#include "../Rage/Ragebot.hpp"

Communication g_Communication;

int encode( std::string username ) {
	int sent = 0;
	for( char ch : username ) {
		sent = ( sent << 8 ) + ( unsigned char )ch;
	}
	return sent;
}

std::string decode( int received ) {
	if( received == 0 )
		return "";

	std::string final;
	while( received > 0 ) {
		char c = ( char )( received & 0xFF );
		final = c + final;
		received >>= 8;
	}
	return final;
}

bool Communication::SendDataMessage( VoiceData_t *data ) {
	if( !g_pEngine->GetNetChannelInfo( ) || !data )
		return true;

	CCLCMsg_VoiceData uMessage;
	memset( &uMessage, 0, sizeof( uMessage ) );

	using ConstructVoiceMsg_t = uint32_t( __fastcall * )( void *, void * );
	static ConstructVoiceMsg_t ConstructVoiceMsg = reinterpret_cast< ConstructVoiceMsg_t >( Engine::Displacement.Function.m_ConstructVoiceMsg );

	ConstructVoiceMsg( ( void * )&uMessage, nullptr );

	_String_t unk;

	uMessage.SetData( data );
	uMessage.data = &unk;
	uMessage.format = 0/*VoiceFormat_Steam*/;
	uMessage.flags = 63;

	std::string username = get_string_from_array( g_Vars.globals.user_info.the_array );

	// this is now part4!
	uMessage.xuid_low = 412;

	return g_pEngine->GetNetChannelInfo( )->SendNetMsg( ( INetMessage * )&uMessage, false, true );
}

void Communication::SendUpdatePacket( int nIndex, Vector vecOrigin ) {
	if( !g_pEngine->GetNetChannelInfo( ) )
		return;

	// local server
	if( g_pEngine->GetNetChannelInfo( )->IsLoopback( ) )
		return;

	// identify our msgs with the first few numbers of the server ip
	const auto szServerIP = std::string( g_pEngine->GetNetChannelInfo( )->GetAddress( ) );

	// setup packet info
	Packet_t uPacket;
	uPacket.uHash = 512;
	uPacket.nEntIndex = nIndex;

	std::string username = get_string_from_array( g_Vars.globals.user_info.the_array );

	int part1 = encode( username.substr( 0, 4 ) );
	int part2 = username.size( ) > 4 ? encode( username.substr( 4, 4 ) ) : 0;
	int part3 = username.size( ) > 8 ? encode( username.substr( 8, 4 ) ) : 0;
	int part4 = username.size( ) > 12 ? encode( username.substr( 12, 4 ) ) : 0;

	/*unsigned int part5 = username.size( ) > 16 ? encode( username.substr( 16, 4 ) ) : 0;
	unsigned int part6 = username.size( ) > 20 ? encode( username.substr( 20, 4 ) ) : 0;
	unsigned int part7 = username.size( ) > 24 ? encode( username.substr( 24, 4 ) ) : 0;
	unsigned int part8 = username.size( ) > 28 ? encode( username.substr( 28, 4 ) ) : 0;*/

	uPacket.p1 = part1;
	uPacket.p2 = part2;
	uPacket.p3 = part3;
	uPacket.p4 = part4;

	// send our message in an encrypted format
	uPacket.Encrypt( );

	// copy it over
	VoiceData_t data;
	memcpy( data.GetRawData( ), &uPacket, sizeof( uPacket ) );

	// send it over
	if( !SendDataMessage( &data ) )
		return;

	// g_pCVar->ConsoleColorPrintf( Color::White( ), "broadcasted shared esp packet: index %i\n", nIndex );
}

bool Communication::OnMessageHook( std::function<bool( )> oSVCMsg_VoiceData, void *msg ) {
	CSVCMsg_VoiceData *pReceived = ( CSVCMsg_VoiceData * )msg;

	// should be a regular one then..
	if( pReceived->xuid_low != 412 ) {
		return oSVCMsg_VoiceData( );
	}

	int nSenderIndex = pReceived->client + 1;

	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal ) {
		return true;
	}

	if( pLocal->EntIndex( ) == nSenderIndex ) {
		return true;
	}

	// check if its empty
	VoiceData_t uData = pReceived->GetData( );
	if( uData.section_number == 0 && uData.sequence_bytes == 0 && uData.uncompressed_sample_offset == 0 ) {
		return true;
	}

	Packet_t *pPacket = ( Packet_t * )uData.GetRawData( );
	if( !pPacket ) {
		return true;
	}

	// identify our msgs with the first few numbers of the server ip
	const auto szServerIP = std::string( g_pEngine->GetNetChannelInfo( )->GetAddress( ) );

	// awesome, we received a player update
	if( pPacket->uHash == 512 && g_Vars.menu.whitelist/*g_Vars.esp.shared_esp*/ ) {
		// decrypt our received message
		pPacket->Decrypt( );

		// grab the player we received data abt
		// auto pEntity = reinterpret_cast< C_BaseEntity * >( g_pEntityList->GetClientEntity( pPacket->nEntIndex ) );

		// new update coming in
		//const Vector vecReceivedOrigin = Vector( pPacket->nOriginX, pPacket->nOriginY, pPacket->nOriginZ );
		std::string receivedName = decode( pPacket->p1 ) + decode( pPacket->p2 ) + decode( pPacket->p3 ) + decode( pPacket->p4 );
		if( !receivedName.empty( ) /*&& pPacket->nEntIndex <= 65*/ ) {
			g_Ragebot.m_arrNapUsers.at( nSenderIndex ).first = true;
			g_Ragebot.m_arrNapUsers.at( nSenderIndex ).second = receivedName;

			//printf( "%s\n", receivedName.data() );

			//g_ExtendedVisuals.m_arrOverridePlayers.at( pPacket->nEntIndex ).m_eOverrideType = EOverrideType::ESP_SHARED;
			//g_ExtendedVisuals.m_arrOverridePlayers.at( pPacket->nEntIndex ).m_vecLastOrigin = g_ExtendedVisuals.m_arrOverridePlayers.at( pPacket->nEntIndex ).m_vecOrigin;
			//g_ExtendedVisuals.m_arrOverridePlayers.at( pPacket->nEntIndex ).m_vecOrigin = vecReceivedOrigin;
			//g_ExtendedVisuals.m_arrOverridePlayers.at( pPacket->nEntIndex ).m_flReceiveTime = g_pGlobalVars->realtime;
			//g_ExtendedVisuals.m_arrOverridePlayers.at( pPacket->nEntIndex ).m_flServerTime = TICKS_TO_TIME( g_pClientState->clock_drift_mgr.m_nServerTick );
		}
	}

	return true;
}