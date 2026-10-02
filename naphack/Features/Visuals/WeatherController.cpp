#include "WeatherController.hpp"
#include "../../pandora.hpp"
#include "../../SDK/Classes/Player.hpp"
#include "../../Renderer/Render.hpp"

WeatherController g_WeatherController;

void WeatherController::ReleasePrecipitationEntity( ) {
	if( !m_bCreatedEntity ) {
		return;
	}

	for( int i = 0; i <= g_pEntityList->GetHighestEntityIndex( ); i++ ) {
		C_BaseEntity *pEntity = ( C_BaseEntity * )g_pEntityList->GetClientEntity( i );
		if( !pEntity )
			continue;

		const ClientClass *pClientClass = pEntity->GetClientClass( );
		if( !pClientClass )
			continue;

		if( pClientClass->m_ClassID == ClassId_t::CPrecipitation ) {
			if( pEntity->GetClientNetworkable( ) ) {
				pEntity->GetClientNetworkable( )->SetDestroyedOnRecreateEntities( );
				pEntity->GetClientNetworkable( )->Release( );
				break;
			}
		}
	}


	if( m_bCreatedCollision ) {
		g_pVPhysicsCollision->VCollideUnload( &m_vCollide );
		m_bCreatedCollision = false;
	}
}

void WeatherController::CreatePrecipitationEntity( PrecipitationType_t nPrecipType ) {
	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	// find the correct class id which we want to operate on
	static ClientClass *pPrecipitation = nullptr;
	if( !pPrecipitation ) {
		for( auto pClientClass = g_pClient->GetAllClasses( ); pClientClass && !pPrecipitation; pClientClass = pClientClass->m_pNext ) {
			if( pClientClass->m_ClassID == ClassId_t::CPrecipitation ) {
				pPrecipitation = pClientClass;
			}
		}
	}

	// we found the class id, let's create an entity in its place
	if( pPrecipitation && pPrecipitation->m_pCreateFn ) {
		IClientNetworkable *pRainNetworkable = ( ( IClientNetworkable * ( * )( int, int ) )pPrecipitation->m_pCreateFn )( MAX_EDICTS - 1, 0 );
		if( !pRainNetworkable ) {
			return;
		}

		IClientUnknown *pRainUnknown = ( ( IClientRenderable * )pRainNetworkable )->GetIClientUnknown( );
		if( !pRainUnknown ) {
			return;
		}

		C_BaseEntity *pRainEntity = pRainUnknown->GetBaseEntity( );
		if( !pRainEntity ) {
			m_bCreatedCollision = m_bCreatedEntity = false;
			return;
		}

		if( !pRainEntity->GetClientNetworkable( ) ) {
			m_bCreatedCollision = m_bCreatedEntity = false;
			return;
		}

		pRainNetworkable->PreDataUpdate( 0 );
		pRainNetworkable->OnPreDataChanged( 0 );

		// https://github.com/perilouswithadollarsign/cstrike15_src/blob/f82112a2388b841d72cb62ca48ab1846dfcc11c8/game/client/c_baseentity.cpp#L1358
		// InitializeAsClientEntity does a bunch of useless shit which
		// makes the game crash when the game/cheat attempts to release 
		// the entity we created, therefore let's only take what we need
		pRainEntity->m_entIndex = -1;

		// update this entities precipitation type 
		// and let the weather controller know
		pRainEntity->m_nPrecipType( ) = nPrecipType;

		// update render mins/maxs of the rain entity
		pRainEntity->OBBMins( ) = Vector( -2048.f, -2048.f, -2048.f );
		pRainEntity->OBBMaxs( ) = Vector( 2048.f, 2048.f, 2048.f );

		// initialise new collision data
		if( !m_bCreatedCollision ) {
			//memset( &m_vCollide, 0, sizeof( m_vCollide ) );

			// (c) dalkr, sharklaser for this fuckin arrPrecipitationCollision array thingy
			g_pVPhysicsCollision->VCollideLoad( &m_vCollide, 1, ( const char * )arrPrecipitationCollision, 546, false );
			m_bCreatedCollision = true;
		}

		// set this entities model index to some unique
		// model index, that way we can differentiate between
		// entites and apply our own vcollide when needed
		pRainEntity->m_nModelIndex( ) = -1;

		// tell game to update entity (apply our changes)
		pRainEntity->GetClientNetworkable( )->OnDataChanged( 0 );
		pRainEntity->GetClientNetworkable( )->PostDataUpdate( 0 );

		// mark successful entity creation if everything went fine 
		// and our entity pointer isn't nullptr.
		if( pRainEntity ) {
			m_bCreatedEntity = true;
		}
	}
}

void WeatherController::UpdateWeather( ) {
	static int nCurrentSoundGUID = -1;
	if( !g_pEngine->IsInGame( ) || !g_pEngine->IsConnected( ) ) {
		g_pEngineSound->StopSoundByGuid( nCurrentSoundGUID, true );

		return;
	}
	
	int nCurrentWeatherOption = g_Vars.esp.weather_type + 1;

	if( !g_Vars.esp.weather_controller )
		nCurrentWeatherOption = 0;

	static int nOldWeatherOption = nCurrentWeatherOption;
	static bool bShouldUpdateWeather = false;

	// disabled/changed weather effect option
	if( nOldWeatherOption != nCurrentWeatherOption ) {
		nOldWeatherOption = nCurrentWeatherOption;
		goto FORCE_RELEASE;
	}

	// disable currently playing sound
	if( nCurrentWeatherOption == 0 ) {
		g_pEngineSound->StopSoundByGuid( nCurrentSoundGUID, true );
	}

	static bool bHadFullUpdate = false;
	// detect full updates (game releases entities)
	if( g_pClientState->m_nDeltaTick( ) == -1 ) {
		bHadFullUpdate = true;
	}

	// had a full update, entities should be recreated
	if( bHadFullUpdate && g_pClientState->m_nDeltaTick( ) != -1 ) {
		bHadFullUpdate = false;

		// force release & recreate entity
		goto FORCE_RELEASE;
	}

	// local player respawned (sanity incase server/map swap happens)
	if( auto pLocal = C_CSPlayer::GetLocalPlayer( ); pLocal ) {
		static float flSpawnTime = 0.f;
		if( flSpawnTime != pLocal->m_flSpawnTime( ) ) {
			flSpawnTime = pLocal->m_flSpawnTime( );

			// force release & recreate entity
			goto FORCE_RELEASE;
		}
	}

	// we previously had a precipitation entity but we disabled
	// the weather effects, release it
	if( bShouldUpdateWeather && nCurrentWeatherOption == 0 ) {
	FORCE_RELEASE:

		ReleasePrecipitationEntity( );
		bShouldUpdateWeather = false;
	}

	// haven't had a precip entity or cleared one previously
	// tell weather controller to re-create one again
	if( !bShouldUpdateWeather && nCurrentWeatherOption > 0 ) {
		m_bCreatedCollision = m_bCreatedEntity = false;
		bShouldUpdateWeather = true;
	}

	PrecipitationType_t nPrecipType = PrecipitationType_t::NUM_PRECIPITATION_TYPES;
	switch( nCurrentWeatherOption ) {
		case 1:
			nPrecipType = PrecipitationType_t::PRECIPITATION_TYPE_PARTICLERAIN;
			break;
		case 2:
			nPrecipType = PrecipitationType_t::PRECIPITATION_TYPE_PARTICLESNOW;
			break;
	}

	// create a new precipitation entity
	// with our desired precipitation type
	if( bShouldUpdateWeather && !m_bCreatedEntity ) {
		if( nPrecipType == PrecipitationType_t::PRECIPITATION_TYPE_PARTICLERAIN ) {
			g_pEngineSound->StopSoundByGuid( nCurrentSoundGUID, true );

			nCurrentSoundGUID = g_pEngineSound->EmitAmbientSound( XorStr( "ambient/weather/aztec_rain_lp_01.wav" ), 0.5f );
		}
		else {
			g_pEngineSound->StopSoundByGuid( nCurrentSoundGUID, true );

			nCurrentSoundGUID = g_pEngineSound->EmitAmbientSound( XorStr( "ambient/wind/dry_air_short.wav" ), 0.75f );
		}

		CreatePrecipitationEntity( nPrecipType );
	}
}