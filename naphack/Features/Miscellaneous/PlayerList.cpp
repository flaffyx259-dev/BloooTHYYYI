#include "PlayerList.hpp"

#include "../../SDK/variables.hpp"
#include "../../SDK/Classes/Player.hpp"
#include "../../SDK/Classes/weapon.hpp"

PlayerList g_PlayerList;

void PlayerList::UpdatePlayerList( ) {
	// whoops
	if( !g_pPlayerResource.IsValid( ) || !( *g_pPlayerResource.Xor( ) ) ) {
		ClearPlayerData( );
		return;
	}

	if( !g_pEngine->IsConnected( ) || !g_pEngine->IsInGame( ) ) {
		ClearPlayerData( );
		return;
	}

	// erase outdated playerlist entries
	for( auto a = m_vecPlayerData.begin( ); a != m_vecPlayerData.end( );) {
		auto pEntity = ( C_CSPlayer * )g_pEntityList->GetClientEntityFromHandle( a->second.m_nPlayerHandle );

		a->second.m_bConnected = true;

		// player isnt connected, can't do shit on him
		if( !pEntity || ( pEntity && !( *g_pPlayerResource.Xor( ) )->GetConnected( pEntity->EntIndex( ) ) ) ) {
			a->second.m_bConnected = false;
		}

		// go to next entry
		a = next( a );
	}

	// update player list
	for( int i = 1; i <= g_pGlobalVars->maxClients; ++i ) {
		auto pEntity = ( C_CSPlayer * )g_pEntityList->GetClientEntity( i );

		// don't wanna plist ourselves kek
		if( i == g_pEngine->GetLocalPlayer( ) )
			continue;

		// we need their name bruh
		player_info_t info;
		if( !g_pEngine->GetPlayerInfo( i, &info ) )
			continue;

		// i mean, come on
		if( info.ishltv )
			continue;

		// setup this entry
		PlayerListInfo_t uEntry;
		uEntry.m_nPlayerHandle = pEntity ? pEntity->GetRefEHandle( ) : CBaseHandle( 420 + i );
		uEntry.m_szPlayerName = info.szName;
		uEntry.m_bConnected = true;

		// epic (handle bots separately)
		const __int64 ulSteamId = info.fakeplayer ? info.iSteamID : info.steamID64;
		if( m_vecPlayerData.find( ulSteamId ) == m_vecPlayerData.end( ) ) {
			m_vecPlayerData.insert( { ulSteamId, uEntry } );
		}
		else {
			// le update
			if( m_vecPlayerData[ ulSteamId ].m_nPlayerHandle != uEntry.m_nPlayerHandle ) {
				m_vecPlayerData[ ulSteamId ].m_nPlayerHandle = uEntry.m_nPlayerHandle;
				m_vecPlayerData[ ulSteamId ].m_szPlayerName = uEntry.m_szPlayerName;
				m_vecPlayerData[ ulSteamId ].m_bConnected = uEntry.m_bConnected;
			}
		}
	}
}

std::unordered_map< __int64, PlayerList::PlayerListInfo_t > PlayerList::GetPlayerData( ) {
	if( m_vecPlayerData.empty( ) )
		return { };

	return m_vecPlayerData;
}

void PlayerList::ClearPlayerData( ) {
	if( m_vecPlayerData.empty( ) )
		return;

	// lmao
	for( auto a = m_vecPlayerData.begin( ); a != m_vecPlayerData.end( ); ) {
		a->second.m_bConnected = false;

		// go to next entry
		a = next( a );
	}
}

std::vector<std::string> PlayerList::GetConnectedPlayers( ) {
	if( m_vecPlayerData.empty( ) )
		return { };

	std::vector<std::string> vecPlayers;

	for( auto a : m_vecPlayerData ) {
		if( hash_32_fnv1a( a.second.m_szPlayerName.data( ) ) == hash_32_fnv1a_const( XorStr( "Deez1337Nuts" ) ) )
			continue;

		if( !a.second.m_bConnected )
			continue;

		vecPlayers.push_back( a.second.m_szPlayerName );
	}

	return vecPlayers;
}