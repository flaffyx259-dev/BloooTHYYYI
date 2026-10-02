#pragma once

#include "../../SDK/sdk.hpp"
#include <unordered_map>
#include "../../SDK/Valve/CBaseHandle.hpp"

class PlayerList {
public:
	struct PlayerListInfo_t {
		// pre-defined, don't modify
		CBaseHandle m_nPlayerHandle;
		int m_bConnected = false;
		std::string m_szPlayerName;

		// regular vars, free to modify
		bool m_bAddToWhitelist = false;
		bool m_bDisableVisuals = false;
		bool m_bDisableResolver = false;
		bool m_bHighPriority = false;
		int m_iExtrapolationOverride = 0;
	};

	std::unordered_map< __int64, PlayerListInfo_t > m_vecPlayerData;
	std::unordered_map< __int64, PlayerListInfo_t > GetPlayerData( );

	void UpdatePlayerList( );
	void ClearPlayerData( );

	std::vector<std::string> GetConnectedPlayers( );

	// only use for comparison purposes
	inline PlayerListInfo_t GetSettings( __int64 ulSteamID ) {
		if( m_vecPlayerData.empty( ) )
			return { };

		if( m_vecPlayerData.find( ulSteamID ) == m_vecPlayerData.end( ) )
			return { };

		return m_vecPlayerData.at( ulSteamID );
	}
};

extern PlayerList g_PlayerList;