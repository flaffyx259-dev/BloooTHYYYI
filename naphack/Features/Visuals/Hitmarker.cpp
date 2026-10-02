#include "Hitmarker.hpp"

#include "EventLogger.hpp"
#include "../Scripting/Scripting.hpp"

#include "../../SDK/Classes/Player.hpp"

Hitmarker g_Hitmarker;

void Hitmarker::Draw( ) {
	if( !g_Vars.esp.visualize_hitmarker_world && !g_Vars.esp.vizualize_hitmarker && !g_Vars.esp.visualize_damage ) {
		m_vecHitmarkers.clear( );
		return;
	}

	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	auto RenderHitmarker = [ & ] ( Vector2D vecCenter, const int nPaddingFromCenter, const int nSize, const int nWidth, Color color ) {
		for( int i = 0; i < nSize; ++i ) // top left
			Render::Engine::RectFilled( ( vecCenter - nPaddingFromCenter ) - i, Vector2D( nWidth, nWidth ), color );

		for( int i = 0; i < nSize; ++i ) // bottom left
			Render::Engine::RectFilled( vecCenter - Vector2D( nPaddingFromCenter, -nPaddingFromCenter ) - Vector2D( i, -i ), Vector2D( nWidth, nWidth ), color );

		for( int i = 0; i < nSize; ++i ) // top right
			Render::Engine::RectFilled( vecCenter + Vector2D( nPaddingFromCenter, -nPaddingFromCenter ) + Vector2D( i, -i ), Vector2D( nWidth, nWidth ), color );

		for( int i = 0; i < nSize; ++i ) // bottom right
			Render::Engine::RectFilled( ( vecCenter + nPaddingFromCenter ) + i, Vector2D( nWidth, nWidth ), color );
	};

	const Vector2D vCenter = Render::GetScreenSize( ) * 0.5f;

	auto DrawAngularLine = [ ] ( int x, int y, float rad, float length, float gap, Color uColor ) -> void {
		const float flRadians = DEG2RAD( rad );
		Render::Engine::Line(
			Vector2D( ( int )round( x + ( sin( flRadians ) * length ) ), ( int )round( y + ( cos( flRadians ) * length ) ) ),
			Vector2D( ( int )round( x + ( sin( flRadians ) * gap ) ), ( int )round( y + ( cos( flRadians ) * gap ) ) ),
			uColor );
	};

	// i'm sorry i had to, it was nice asf (+ i showcased it to p3x and others and they all said 
	// that the old screen hitmarker was much nicer and unique :P)
	if( g_Vars.esp.vizualize_hitmarker ) {
		static Vector2D vDrawCenter = vCenter;
		if( m_flMarkerAlpha == 255.f ) {
			vDrawCenter = vCenter;
			m_bFirstMarker = false;
		}

		constexpr float flFadeFactor = 255.f / 1.0f;
		float flFadeIncrement = ( flFadeFactor * g_pGlobalVars->frametime );

		//if( m_flMarkerTime + 0.5f <= g_pGlobalVars->realtime )
		m_flMarkerAlpha -= flFadeIncrement;

		if( m_flMarkerAlpha <= 0 ) {
			m_bFirstMarker = true;
			m_flMarkerAlpha = 0;
		}
		else {
			RenderHitmarker( vDrawCenter, 6, 6, 1, Color::White( ).OverrideAlpha( m_flMarkerAlpha ) );
		}
	}

	for( auto it = m_vecHitmarkers.begin( ); it != m_vecHitmarkers.end( ); ) {
		if( it->distance == FLT_MAX ) {
			it = m_vecHitmarkers.erase( it );
			continue;
		}

		if( fabs( it->time - g_pGlobalVars->realtime ) > 0.5f ) {
			// i do this frametime*10 gui::approach shit everywhere it's SO BEAUTIFUL
			it->alpha -= 255.f / 1.0f * g_pGlobalVars->frametime;
		}

		if( it->alpha <= 0.f ) {
			it = m_vecHitmarkers.erase( it );
			continue;
		}

		it->move_damage -= 30.f / 1.5f * g_pGlobalVars->frametime;

		const Vector origin = Vector( it->impact.x, it->impact.y, it->impact.z );
		if( g_Vars.esp.visualize_hitmarker_world ) {
			Vector2D screen{ };
			if( Render::Engine::WorldToScreen( origin, screen ) ) {
				RenderHitmarker( screen, 3, 4, 1, Color::White( ).OverrideAlpha( it->alpha ) );
			}
		}

		if( g_Vars.esp.visualize_damage ) {
			Vector2D screen{ };

			if( Render::Engine::WorldToScreen( origin, screen ) ) {
				//screen.y += it->move_damage;

				Render::Engine::esp_bold.string( screen.x, screen.y - 12 + it->move_damage,
												 Color::Red( ).OverrideAlpha( 220 * ( it->alpha / 255.f ), true ), std::string( "- " ).append( std::to_string( it->damage ) ), Render::Engine::ALIGN_CENTER );
			}
		}

		it++;
	}

}

void Hitmarker::GameEvent( IGameEvent *pEvent ) {
	if( !pEvent )
		return;

	auto eventHash = hash_32_fnv1a( pEvent->GetName( ) );
	if( eventHash == hash_32_fnv1a_const( XorStr( "player_hurt" ) ) || eventHash == hash_32_fnv1a_const( XorStr( "player_blind" ) ) ) {
		PlayerHurt( pEvent );
	}
	else if( eventHash == hash_32_fnv1a_const( XorStr( "bullet_impact" ) ) ) {
		BulletImpact( pEvent );
	}
}

void Hitmarker::PlayerHurt( IGameEvent *pEvent ) {
	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	const int attacker = g_pEngine->GetPlayerForUserID( pEvent->GetInt( XorStr( "attacker" ) ) );
	const int player = g_pEngine->GetPlayerForUserID( pEvent->GetInt( XorStr( "userid" ) ) );
	const auto weapon = hash_32_fnv1a( pEvent->GetString( XorStr( "weapon" ) ) );

	auto WeDontWantThis = [ & ] ( ) -> bool {
		// these won't give us any impacts..
		// kinda ghetto ik but who cares
		const bool bYEP = weapon == hash_32_fnv1a_const( XorStr( "smokegrenade" ) ) ||
			weapon == hash_32_fnv1a_const( XorStr( "hegrenade" ) ) ||
			weapon == hash_32_fnv1a_const( XorStr( "molotov" ) ) ||
			weapon == hash_32_fnv1a_const( XorStr( "incgrenade" ) ) ||
			weapon == hash_32_fnv1a_const( XorStr( "decoy" ) ) ||
			weapon == hash_32_fnv1a_const( XorStr( "inferno" ) ) ||
			weapon == hash_32_fnv1a_const( XorStr( "flashbang" ) );

		return bYEP;
	};

	if( attacker != g_pEngine->GetLocalPlayer( ) || player == g_pEngine->GetLocalPlayer( ) || WeDontWantThis( ) )
		return;

	const auto pPlayer = C_CSPlayer::GetPlayerByIndex( player );
	const auto pAttacker = C_CSPlayer::GetPlayerByIndex( attacker );

	if( !pPlayer || !pAttacker )
		return;

	// so we know, which impacts to delete :)
	++m_iHurtEventID;

	float flBestDistance = FLT_MAX;
	impact_t bestImpact{ };
	for( auto it = m_vecImpacts.begin( ); it != m_vecImpacts.end( ); ) {
		if( g_pGlobalVars->curtime > it->time + 1.f ) {
			it = m_vecImpacts.erase( it );
			continue;
		}

		// this one was already matched / tested on :)
		if( it->m_iHurtEventID != -1 ) {
			it = m_vecImpacts.erase( it );
			continue;
		}

		const Vector vecPos = Vector( it->x, it->y, it->z );
		const float flDistance = vecPos.Distance( pPlayer->GetAbsOrigin( ) );

		it->m_iHurtEventID = m_iHurtEventID;

		if( flDistance < flBestDistance ) {
			flBestDistance = flDistance;
			bestImpact = *it;

			it = m_vecImpacts.erase( it );
		}
		else {
			it++;
		}
	}

	hitmarker_t hitmarker{ };
	hitmarker.distance = flBestDistance;
	if( hitmarker.distance != FLT_MAX )
		hitmarker.impact = bestImpact;

	hitmarker.time = g_pGlobalVars->realtime;
	hitmarker.alpha = 255.f;
	hitmarker.damage = pEvent->GetInt( XorStr( "dmg_health" ) );
	hitmarker.enlargment = 0.f;
	hitmarker.index = m_iHurtEventID % 2;
	hitmarker.move_damage = 0;

	static auto sv_showimpacts_time = g_pCVar->FindVar( XorStr( "sv_showimpacts_time" ) );

	// expire at the same time as the impacts
	hitmarker.expiry = sv_showimpacts_time->GetFloat( );

	m_uMarkerColor = pEvent->GetInt( XorStr( "hitgroup" ) ) == Hitgroup_Head ? g_Vars.esp.hitmarker_screen_headshot_color.ToRegularColor( ) : g_Vars.esp.hitmarker_screen_regular_color.ToRegularColor( );
	m_flMarkerAlpha = 255.f;
	m_flMarkerTime = g_pGlobalVars->realtime;

#ifdef LUA_SCRIPTING
	Scripting::Script::DoCallback( hash_32_fnv1a_const( XorStr( "on_hitmarker" ) ), hitmarker.damage, Vector( hitmarker.impact.x, hitmarker.impact.y, hitmarker.impact.z ), pEvent->GetInt( XorStr( "hitgroup" ) ), pPlayer->GetAbsOrigin( ) );
#endif

	m_vecHitmarkers.push_back( hitmarker );
}

void Hitmarker::BulletImpact( IGameEvent *pEvent ) {
	int index = g_pEngine->GetPlayerForUserID( pEvent->GetInt( XorStr( "userid" ) ) );
	if( !index )
		return;

	if( index != g_pEngine->GetLocalPlayer( ) )
		return;

	impact_t impact{ };
	impact.x = pEvent->GetFloat( XorStr( "x" ) );
	impact.y = pEvent->GetFloat( XorStr( "y" ) );
	impact.z = pEvent->GetFloat( XorStr( "z" ) );
	impact.time = g_pGlobalVars->curtime;

	m_vecImpacts.push_back( impact );
}
