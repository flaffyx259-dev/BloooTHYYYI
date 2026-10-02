#include "AutomaticPurchase.hpp"

#include "../../SDK/Classes/Player.hpp"
#include "../../SDK/Classes/weapon.hpp"
#include "../Visuals/EventLogger.hpp"

AutomaticPurchase g_AutomaticPurchase;

// chase <3
// chase why so cute............
// we love chase!
void AutomaticPurchase::GameEvent( ) {
	if( !g_Vars.misc.autobuy_enabled )
		return;

	const auto pLocal = C_CSPlayer::GetLocalPlayer( );

	bool bFoundAuto = false;
	bool bPistolRound = false;

	int m_totalRoundsPlayed = 0;
	if( g_pGameRules.IsValid( ) )
		m_totalRoundsPlayed = g_pGameRules->m_totalRoundsPlayed( );

	if( m_totalRoundsPlayed == 0 || m_totalRoundsPlayed == g_Vars.mp_maxrounds->GetInt( ) / 2 )
		bPistolRound = true;

	if( bPistolRound )
		return;

	int iTeam = pLocal ? pLocal->m_iTeamNum( ) : 1337;
	if( pLocal ) {
		auto weapons = pLocal->m_hMyWeapons( );
		for( size_t i = 0; i < 48; ++i ) {
			auto weaponHandle = weapons[ i ];
			if( !weaponHandle.IsValid( ) )
				break;

			auto pWeapon = ( C_BaseCombatWeapon * )weaponHandle.Get( );
			if( !pWeapon )
				continue;

			if( pWeapon->m_iItemDefinitionIndex( ) == WEAPON_SCAR20 ||
				pWeapon->m_iItemDefinitionIndex( ) == WEAPON_G3SG1 ) {
				bFoundAuto = true;
			}
		}
	}

	if( g_Vars.misc.autobuy_first_weapon > 0 ) {
		switch( g_Vars.misc.autobuy_first_weapon ) {
		case 1: // scar20 / g3sg1
		{
			if( bFoundAuto ) {
				g_EventLog.PushEvent( XorStr( "found auto-sniper, not buying" ), Color_f( .8f, .8f, .8f ), false );
				break;
			}

			if( pLocal ) {
				if( iTeam == TEAM_TT ) {
					g_pEngine->ClientCmd( XorStr( "buy g3sg1" ) );
				}
				else if( iTeam == TEAM_CT ) {
					g_pEngine->ClientCmd( XorStr( "buy scar20" ) );
				}
			}
			else { // LOL
				g_pEngine->ClientCmd( XorStr( "buy scar20; buy g3sg1" ) );
			}

			// m_iQueuedUpCommands += 1;
			break;
		}
		case 2:
			g_pEngine->ClientCmd( XorStr( "buy ssg08" ) );
			break;
		case 3:
			g_pEngine->ClientCmd( XorStr( "buy awp" ) );
			break;
		default:
			break;
		}
	}

	if( g_Vars.misc.autobuy_second_weapon > 0 ) {
		switch( g_Vars.misc.autobuy_second_weapon ) {
		case 1: // elite
			g_pEngine->ClientCmd( XorStr( "buy elite" ) );
			//m_iQueuedUpCommands += 1;
			break;
		case 2: // deagle / revolver
			g_pEngine->ClientCmd( XorStr( "buy deagle" ) );
			//m_iQueuedUpCommands += 1;
			break;
		case 3: // deagle / revolver
			g_pEngine->ClientCmd( XorStr( "buy tec9" ) );
			//m_iQueuedUpCommands += 1;
			break;
		default:
			break;
		}
	}

	if( g_Vars.misc.autobuy_armor ) {
		g_pEngine->ClientCmd( XorStr( "buy vest" ) );
		g_pEngine->ClientCmd( XorStr( "buy vesthelm" ) );

		//m_iQueuedUpCommands += 2;
	}

	if( g_Vars.misc.autobuy_hegrenade ) {
		g_pEngine->ClientCmd( XorStr( "buy hegrenade" ) );

		//m_iQueuedUpCommands += 1;
	}

	if( g_Vars.misc.autobuy_molotovgrenade ) {
		if( pLocal ) {
			if( iTeam == TEAM_TT )
				g_pEngine->ClientCmd( XorStr( "buy molotov" ) );
			else if( iTeam == TEAM_CT )
				g_pEngine->ClientCmd( XorStr( "buy incgrenade" ) );
		}
		else {
			g_pEngine->ClientCmd( XorStr( "buy incgrenade; buy molotov" ) );
		}

		//m_iQueuedUpCommands += 1;
	}

	if( g_Vars.misc.autobuy_smokegreanade ) {
		g_pEngine->ClientCmd( XorStr( "buy smokegrenade" ) );

		//m_iQueuedUpCommands += 1;
	}

	if( g_Vars.misc.autobuy_flashbang ) {
		g_pEngine->ClientCmd( XorStr( "buy flashbang" ) );

		//m_iQueuedUpCommands += 1;
	}

	if( g_Vars.misc.autobuy_zeus ) {
		g_pEngine->ClientCmd( XorStr( "buy taser" ) );

		//m_iQueuedUpCommands += 1;
	}

	if( g_Vars.misc.autobuy_defusekit ) {
		g_pEngine->ClientCmd( XorStr( "buy defuser" ) );

		//m_iQueuedUpCommands += 1;
	}

	if( g_Vars.misc.autobuy_decoy ) {
		g_pEngine->ClientCmd( XorStr( "buy decoy" ) );

		//m_iQueuedUpCommands += 1;
	}
}
