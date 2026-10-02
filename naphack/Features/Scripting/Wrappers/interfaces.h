#pragma once

#include "entity.h"

namespace Wrappers::Interfaces {
	namespace EntityList {
		Wrappers::Entity::CEntity GetClientEntity( int entnum ) {
			// return and construct the entity class.
			return Wrappers::Entity::CEntity( reinterpret_cast<C_CSPlayer*>( ::g_pEntityList->GetClientEntity( entnum ) ) );
		}

		Wrappers::Entity::CEntity GetClientEntityFromHandle( Wrappers::Entity::NetVarType netvar ) {
			return Wrappers::Entity::CEntity( reinterpret_cast< C_CSPlayer* >( ::g_pEntityList->GetClientEntityFromHandle( netvar.get_handle( ) ) ) );
		}

		int GetHighestEntityIndex( ) {
			return ::g_pEntityList->GetHighestEntityIndex( );
		}

		std::vector<int> GetAll( const char* name ) {
			std::vector<int> temp;

			for( int i = 1; i <= ::g_pEntityList->GetHighestEntityIndex( ); ++i ) {
				auto player = reinterpret_cast<C_CSPlayer*>( ::g_pEntityList->GetClientEntity( i ) );
				if( !player )
					continue;

				if( player->is( name ) ) {
					temp.push_back( player->m_entIndex );
				}
			}

			return temp;
		}
	}

	namespace ConVar {
		::ConVar* FindVar( const char* var_name ) {
			return ::g_pCVar->FindVar( var_name );
		}
	}

	namespace EngineTrace {
		::CGameTrace TraceRay( const Vector &start, const Vector &end, Wrappers::Entity::CEntity entity, const int mask ) {
			::CGameTrace tr{ };

			::CTraceFilter filter{ };
			filter.pSkip = entity.operator C_CSPlayer * ( );
			::g_pEngineTrace->TraceRay( { start, end }, mask, &filter, &tr );

			return tr;
		}


	}

	namespace GlobalVars {
		float curtime( ) {
			return ::g_pGlobalVars->curtime;
		}

		float realtime( ) {
			return ::g_pGlobalVars->realtime;
		}

		float frametime( ) {
			return ::g_pGlobalVars->frametime;
		}

		int framecount( ) {
			return ::g_pGlobalVars->framecount;
		}

		int maxClients( ) {
			return ::g_pGlobalVars->maxClients;
		}

		int interval_per_tick( ) {
			return ::g_pGlobalVars->interval_per_tick;
		}

		int choked_commands( ) {
			if( !::g_pClientState.IsValid( ) )
				return -1;

			return ::g_pClientState->m_nChokedCommands( );
		}

		
	}

	namespace Engine {
		int GetPlayerForUserID( int userID ) {
			return ::g_pEngine->GetPlayerForUserID( userID );
		}

		bool IsInGame( ) {
			return ::g_pEngine->IsInGame( );
		}

		int GetLocalPlayer( ) {
			return ::g_pEngine->GetLocalPlayer( );
		}

		player_info_t GetPlayerInfo( int ent_num ) {
			player_info_t xd;
			::g_pEngine->GetPlayerInfo( ent_num, &xd );
			return xd;
		}

		QAngle GetViewAngles( ) {
			QAngle va;
			::g_pEngine->GetViewAngles( va );

			return va;
		}

		void SetViewAngles( QAngle va ) {
			::g_pEngine->SetViewAngles( va );
		}

		void ExecuteClientCmd( const char* szCmdString ) {
			::g_pEngine->ClientCmd_Unrestricted( szCmdString );
		}

		bool IsConnected( ) {
			return ::g_pEngine->IsConnected( );
		}

		float GetRestartRoundTime( ) {
			float flRestartRoundTime = 0;

			if( g_pGameRules.IsValid( ) )
				flRestartRoundTime = g_pGameRules->m_flRestartRoundTime( );
			
			return flRestartRoundTime;
		}
	}
}