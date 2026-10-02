#include "GrenadeWarning.hpp"
#include "../Rage/Autowall.hpp"
#include "../Visuals/Visuals.hpp"
#include "../Rage/TickbaseShift.hpp"

#include "../../SDK/Classes/weapon.hpp"

GrenadeWarning g_GrenadeWarning;

bool GrenadeWarning::GrenadeData_t::PhysicsSimulate( ) {
	if( m_bDetonated )
		return true;

	const auto flVelocityZ = m_vecVelocity.z - ( g_Vars.sv_gravity->GetFloat( ) * 0.4f ) * g_pGlobalVars->interval_per_tick;

	const auto move = Vector(
		m_vecVelocity.x * g_pGlobalVars->interval_per_tick,
		m_vecVelocity.y * g_pGlobalVars->interval_per_tick,
		( m_vecVelocity.z + flVelocityZ ) / 2.f * g_pGlobalVars->interval_per_tick
	);

	m_vecVelocity.z = flVelocityZ;

	auto trace = CGameTrace( );

	PhysicsPushEntity( move, trace );

	if( m_bDetonated )
		return true;

	if( trace.fraction != 1.f ) {
		UpdatePath( true );

		PerformFlyCollision( trace );
	}

	return false;
}

void GrenadeWarning::GrenadeData_t::PhysicsTraceEntity( const Vector &src, const Vector &dst, std::uint32_t mask, CGameTrace &trace ) {
	TraceHull(
		src, dst, { -2.f, -2.f, -2.f }, { 2.f, 2.f, 2.f },
		mask, m_pOwner, m_iCollisionGroup, &trace
	);

	if( trace.startsolid && ( trace.contents & CONTENTS_CURRENT_90 ) ) {
		TraceHull(
			src, dst, { -2.f, -2.f, -2.f }, { 2.f, 2.f, 2.f },
			mask & ~CONTENTS_CURRENT_90, m_pOwner, m_iCollisionGroup, &trace
		);
	}

	if( !trace.DidHit( )
		|| !trace.hit_entity
		|| !reinterpret_cast< C_CSPlayer * >( trace.hit_entity )->IsPlayer( ) )
		return;

	TraceLine( src, dst, mask, m_pOwner, m_iCollisionGroup, &trace );
}

void GrenadeWarning::GrenadeData_t::PhysicsPushEntity( const Vector &push, CGameTrace &trace ) {
	PhysicsTraceEntity( m_vecOrigin, m_vecOrigin + push,
						m_iCollisionGroup == COLLISION_GROUP_DEBRIS
						? ( MASK_SOLID | CONTENTS_CURRENT_90 ) & ~CONTENTS_MONSTER
						: MASK_SOLID | CONTENTS_OPAQUE | CONTENTS_IGNORE_NODRAW_OPAQUE | CONTENTS_CURRENT_90 | CONTENTS_HITBOX,
						trace
	);

	if( trace.startsolid ) {
		m_iCollisionGroup = COLLISION_GROUP_INTERACTIVE_DEBRIS;

		TraceLine(
			m_vecOrigin - push, m_vecOrigin + push,
			( MASK_SOLID | CONTENTS_CURRENT_90 ) & ~CONTENTS_MONSTER,
			m_pOwner, m_iCollisionGroup, &trace
		);
	}

	if( trace.fraction ) {
		m_vecOrigin = trace.endpos;
	}

	if( !trace.hit_entity )
		return;

	if( reinterpret_cast< C_CSPlayer * >( trace.hit_entity )->IsPlayer( )
		|| m_iWeapIndex != WEAPON_TAGRENADE && m_iWeapIndex != WEAPON_MOLOTOV && m_iWeapIndex != WEAPON_INC )
		return;

	if( m_iWeapIndex != WEAPON_TAGRENADE
		&& trace.plane.normal.z < std::cos( DEG2RAD( g_Vars.weapon_molotov_maxdetonateslope->GetFloat( ) ) ) )
		return;

	Detonate( true );
}

void GrenadeWarning::GrenadeData_t::PerformFlyCollision( CGameTrace &trace ) {
	auto flSurfaceElasticity = 1.f;

	if( trace.hit_entity ) {
		if( Autowall::IsBreakable( reinterpret_cast< C_CSPlayer * >( trace.hit_entity ) ) ) {

			if( reinterpret_cast< C_CSPlayer * >( trace.hit_entity ) && reinterpret_cast< C_CSPlayer * >( trace.hit_entity )->m_iHealth( ) <= 0 ) {
				m_vecVelocity *= 0.4f;
				m_pOwner = reinterpret_cast< C_CSPlayer * >( trace.hit_entity );
				return;
			}
		}

		const auto bIsPlayer = reinterpret_cast< C_CSPlayer * >( trace.hit_entity )->IsPlayer( );
		if( bIsPlayer ) {
			flSurfaceElasticity = 0.3f;
		}

		if( trace.hit_entity->EntIndex( ) ) {
			if( bIsPlayer
				&& m_pLastHitEntity == trace.hit_entity ) {
				m_iCollisionGroup = COLLISION_GROUP_DEBRIS;

				return;
			}

			m_pLastHitEntity = trace.hit_entity;
		}
	}

	auto vecVelocity = Vector( );

	const auto flBack = m_vecVelocity.Dot( trace.plane.normal ) * 2.f;

	for( auto i = 0u; i < 3u; i++ ) {
		const auto change = trace.plane.normal[ i ] * flBack;

		vecVelocity[ i ] = m_vecVelocity[ i ] - change;

		if( std::fabs( vecVelocity[ i ] ) >= 1.f )
			continue;

		vecVelocity[ i ] = 0.f;
	}

	vecVelocity *= std::clamp< float >( flSurfaceElasticity * 0.45f, 0.f, 0.9f );

	if( trace.plane.normal.z > 0.7f ) {
		const auto flSpeed = vecVelocity.LengthSquared( );
		if( flSpeed > 96000.f ) {
			const auto l = vecVelocity.Normalized( ).Dot( trace.plane.normal );
			if( l > 0.5f ) {
				vecVelocity *= 1.f - l + 0.5f;
			}
		}

		if( flSpeed < 400.f ) {
			m_vecVelocity = Vector( 0, 0, 0 );
		}
		else {
			m_vecVelocity = vecVelocity;

			PhysicsPushEntity( vecVelocity * ( ( 1.f - trace.fraction ) * g_pGlobalVars->interval_per_tick ), trace );
		}
	}
	else {
		m_vecVelocity = vecVelocity;

		PhysicsPushEntity( vecVelocity * ( ( 1.f - trace.fraction ) * g_pGlobalVars->interval_per_tick ), trace );
	}

	if( m_nBounces > 20 )
		return Detonate( false );

	++m_nBounces;
}

void GrenadeWarning::GrenadeData_t::Think( ) {
	switch( m_iWeapIndex ) {
		case WEAPON_SMOKEGRENADE:
			if( m_vecVelocity.LengthSquared( ) <= 0.01f ) {
				Detonate( false );
			}

			break;
		case WEAPON_DECOY:
			if( m_vecVelocity.LengthSquared( ) <= 0.04f ) {
				Detonate( false );
			}

			break;
		case WEAPON_FLASHBANG:
		case WEAPON_HEGRENADE:
		case WEAPON_MOLOTOV:
		case WEAPON_INC:
			if( TICKS_TO_TIME( m_iTick ) > m_flDetonationTime ) {
				Detonate( false );
			}

			break;
	}

	m_iNextThinkTick = m_iTick + TIME_TO_TICKS( 0.2f );
}

void GrenadeWarning::GrenadeData_t::Predict( const Vector &origin, const Vector &velocity, float throw_time, int offset ) {
	m_vecOrigin = origin;
	m_vecVelocity = velocity;
	m_iCollisionGroup = COLLISION_GROUP_PROJECTILE;

	const auto iTick = TIME_TO_TICKS( 1.f / 30.f );

	m_iLastUpdateTick = -iTick;

	switch( m_iWeapIndex ) {
		case WEAPON_SMOKEGRENADE: m_iNextThinkTick = TIME_TO_TICKS( 1.5f ); break;
		case WEAPON_DECOY: m_iNextThinkTick = TIME_TO_TICKS( 2.f ); break;
		case WEAPON_FLASHBANG:
		case WEAPON_HEGRENADE:
			m_flDetonationTime = 1.5f;
			m_iNextThinkTick = TIME_TO_TICKS( 0.02f );

			break;
		case WEAPON_MOLOTOV:
		case WEAPON_INC:
			m_flDetonationTime = g_Vars.molotov_throw_detonate_time->GetFloat( );
			m_iNextThinkTick = TIME_TO_TICKS( 0.02f );

			break;
	}

	for( ; m_iTick < TIME_TO_TICKS( 60.f ); ++m_iTick ) {
		if( m_iNextThinkTick <= m_iTick ) {
			Think( );
		}

		if( m_iTick < offset )
			continue;

		if( PhysicsSimulate( ) )
			break;

		if( m_iLastUpdateTick + iTick > m_iTick )
			continue;

		UpdatePath( false );
	}

	if( m_iLastUpdateTick + iTick <= m_iTick ) {
		UpdatePath( false );
	}

	m_flExpireTime = throw_time + TICKS_TO_TIME( m_iTick );
}

// smoke radius used by the game for molotov extinguish checks
static constexpr float kSmokeRadius = 144.f;
// he grenade damage falloff constants
static constexpr float kHeDamageBase = 105.0f;
static constexpr float kHeDamageOffset = 25.0f;
static constexpr float kHeDamageFalloff = 140.0f;
static constexpr float kHeDamageRadius = 475.f;
// molotov / incendiary flame spread reach
static constexpr float kFireRadius = 215.f;
// flames cannot climb, so ignore anything far above/below the puddle
static constexpr float kFireVerticalReach = 80.f;

// frame-rate independent smoothing toward a target
static float ApproachSmooth( float flCurrent, float flTarget, float flRate ) {
	const float flAlpha = std::clamp( g_pGlobalVars->frametime * flRate, 0.f, 1.f );
	const float flResult = flCurrent + ( flTarget - flCurrent ) * flAlpha;

	// snap once we're close enough, avoids endless sub-pixel crawl
	if( std::fabsf( flResult - flTarget ) < 0.05f )
		return flTarget;

	return flResult;
}

bool GrenadeWarning::GrenadeData_t::IsInsideSmoke( const Vector &point ) {
	for( const auto &smoke : g_Visuals.vecSmokesOrigin ) {
		if( smoke.second.IsZero( ) )
			continue;

		if( smoke.second.Distance( point ) <= kSmokeRadius )
			return true;
	}

	return false;
}

GrenadeWarning::GrenadeData_t::Danger_t GrenadeWarning::GrenadeData_t::EvaluateDanger( C_CSPlayer *pLocal, const Vector &vecDetonation, bool bFireReached ) const {
	Danger_t danger{ };

	if( !pLocal )
		return danger;

	const Vector vecLocal = pLocal->GetAbsOrigin( );

	if( m_iWeapIndex == WEAPON_HEGRENADE ) {
		if( vecLocal.Distance( vecDetonation ) > kHeDamageRadius )
			return danger;

		auto &boneData = pLocal->m_CachedBoneData( );
		const int nBones = boneData.Count( );

		// bone 8 is the one we relocate to our origin; bail out if the model
		// doesn't have it instead of scribbling out of bounds.
		if( nBones <= 8 || !boneData.Base( ) )
			return danger;

		matrix3x4_t pBackupMatrix[ MAXSTUDIOBONES ];
		const int nBackupCount = std::min( nBones, static_cast< int >( MAXSTUDIOBONES ) );
		std::memcpy( pBackupMatrix, boneData.Base( ), sizeof( matrix3x4_t ) * nBackupCount );

		// note - michal; yeah..... had some issues tracing to eyepos or origin (obv eyepos cos aa and obv origin cos nothing to hit half the time)
		// so i did the same stuff we do for the autowall bbox stuff and ye it seems to work perfectly XD
		boneData.Base( )[ 8 ].MatrixSetColumn( vecLocal, 3 );

		// back up the bone cache scalars too - the old code overwrote them and never
		// put them back, which left the local player's bone cache permanently forced.
		const int nBackupBoneCounter = pLocal->m_iMostRecentModelBoneCounter( );
		const float flBackupSetupTime = pLocal->m_flLastBoneSetupTime( );
		const unsigned long ulBackupReadable = pLocal->m_BoneAccessor( ).m_ReadableBones;
		const unsigned long ulBackupWritable = pLocal->m_BoneAccessor( ).m_WritableBones;

		pLocal->m_iMostRecentModelBoneCounter( ) = *( int * )Engine::Displacement.Data.m_uModelBoneCounter;
		pLocal->m_BoneAccessor( ).m_ReadableBones = pLocal->m_BoneAccessor( ).m_WritableBones = 0xFFFFFFFF;
		pLocal->m_flLastBoneSetupTime( ) = FLT_MAX;

		CGameTrace trace{ };
		CTraceFilter filter{ };
		filter.pSkip = GetNadeEntity( );

		g_pEngineTrace->TraceRay( Ray_t( vecDetonation, vecLocal ), MASK_SHOT, ( ITraceFilter * )&filter, &trace );

		std::memcpy( boneData.Base( ), pBackupMatrix, sizeof( matrix3x4_t ) * nBackupCount );

		pLocal->m_iMostRecentModelBoneCounter( ) = nBackupBoneCounter;
		pLocal->m_flLastBoneSetupTime( ) = flBackupSetupTime;
		pLocal->m_BoneAccessor( ).m_ReadableBones = ulBackupReadable;
		pLocal->m_BoneAccessor( ).m_WritableBones = ulBackupWritable;

		if( !trace.hit_entity || trace.hit_entity != pLocal )
			return danger;

		// TODO: make this perfect / reverse it, damage is not perfectly calculated
		const float d = ( ( vecLocal - vecDetonation ).Length( ) - kHeDamageOffset ) / kHeDamageFalloff;
		float flDamage = kHeDamageBase * std::exp( -d * d );

		flDamage = static_cast< float >( std::max( static_cast< int >( std::ceilf( CSGOArmor( flDamage, pLocal->m_ArmorValue( ) ) ) ), 0 ) );

		if( flDamage <= 0.f )
			return danger;

		danger.m_bSafe = false;
		danger.m_iDamage = static_cast< int >( flDamage );

		// * 0.75 because we want some tolerance incase this nade will do
		// significant damage to us
		danger.m_flSeverity = std::clamp< float >( flDamage / std::max( 1.f, std::floor( pLocal->m_iHealth( ) * 0.75f ) ), 0.f, 1.f );

		return danger;
	}

	if( m_iWeapIndex != WEAPON_MOLOTOV && m_iWeapIndex != WEAPON_INC )
		return danger;

	// no ground under the trajectory end means no puddle, so nothing to warn about yet
	if( !bFireReached )
		return danger;

	// molotovs thrown into smoke get extinguished on impact
	if( IsInsideSmoke( vecDetonation ) )
		return danger;

	// flames spread along the floor; a puddle a storey below us is not a threat
	if( std::fabsf( vecLocal.z - vecDetonation.z ) > kFireVerticalReach )
		return danger;

	const float flDistance = vecLocal.Distance( vecDetonation );
	if( flDistance > kFireRadius )
		return danger;

	danger.m_bSafe = false;
	danger.m_flSeverity = std::clamp< float >( 1.f - ( flDistance / kFireRadius ), 0.f, 1.f );

	return danger;
}

bool GrenadeWarning::GrenadeData_t::Draw( ) {
	if( m_Path.size( ) <= 1u
		|| g_pGlobalVars->curtime >= m_flExpireTime )
		return false;

	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return false;

	float flDistance = pLocal->m_vecOrigin( ).Distance( m_vecOrigin ) / 12;

	if( flDistance > 200.f )
		return false;

	// resolve the projectile through its handle; it may already be freed since we
	// deliberately keep predicted entries alive after the entity goes away.
	C_BaseEntity *pNadeEntity = m_bLocalPredicted ? nullptr : GetNadeEntity( );
	if( !m_bLocalPredicted && !pNadeEntity )
		return false;

	const bool bDrawTrail = m_bLocalPredicted || !pNadeEntity->IsDormant( );

	Vector2D vPrevScreen2D = { };
	Vector vFinalPoint = std::get< Vector >( m_Path.front( ) );
	auto bPrevScreen = Render::Engine::WorldToScreen( std::get< Vector >( m_Path.front( ) ), vPrevScreen2D );

	for( auto i = 1u; i < m_Path.size( ); ++i ) {
		auto path = m_Path[ i ];

		Vector2D vCurScreen = { };
		const auto bCurScreen = Render::Engine::WorldToScreen( std::get< Vector >( m_Path.at( i ) ), vCurScreen );

		if( bPrevScreen && bCurScreen && bDrawTrail
			&& ( g_Vars.esp.grenade_proximity_warning || g_Vars.esp.grenade_prediction ) ) {
			auto color = m_bLocalPredicted ? g_Vars.esp.grenade_prediction_color.ToRegularColor( ) : g_Vars.esp.grenade_proximity_warning_color.ToRegularColor( );

			// s/o esoterik polak maddie d3x dex boris templar wzn nave ko1n imitator raxer zbe etc etc
			// badster vice machete nitsuj wav kamay kolo arctosa searchy ingame1128 ducarii etc etc etc
			Render::Engine::Line( vPrevScreen2D, vCurScreen, color );

			if( m_bLocalPredicted ) {
				const bool bLastPoint = i == m_Path.size( ) - 1;
				if( path.second || bLastPoint ) {
					Vector2D vBounceScreen = { };
					if( Render::Engine::WorldToScreen( bLastPoint ? std::get< Vector >( m_Path.at( i ) ) : path.first, vBounceScreen ) ) {
						Render::Engine::Rect( vBounceScreen - 2, Vector2D{ 4, 4 }, bLastPoint ? Color( 255, 0, 0, color.a( ) ) : Color( 255, 255, 255, color.a( ) ) );
					}
				}
			}
		}

		vFinalPoint = std::get< Vector >( m_Path.at( i ) );
		vPrevScreen2D = vCurScreen;
		bPrevScreen = bCurScreen;
	}

	bool bReached = false;
	CGameTrace fireTrace;
	CTraceFilter fireFilter;
	fireFilter.pSkip = pNadeEntity;
	if( m_iWeapIndex == WEAPON_MOLOTOV || m_iWeapIndex == WEAPON_INC ) {
		Autowall::TraceLine( vFinalPoint + Vector( 0.f, 0.f, 10.f ), vFinalPoint - Vector( 0.f, 0.f, 119.8f ), MASK_SOLID, &fireFilter, &fireTrace );

		if( fireTrace.fraction != 1.f ) {
			bReached = true;

			vFinalPoint = fireTrace.endpos;
		}
	}

	if( m_bLocalPredicted ) {
		std::pair< float, C_CSPlayer * > target{ 0.f, nullptr };

		for( int i = 1; i <= g_pGlobalVars->maxClients; ++i ) {
			const auto player = C_CSPlayer::GetPlayerByIndex( i );
			if( !player )
				continue;

			if( player->IsDead( ) || player->m_bGunGameImmunity( ) || player->IsTeammate( pLocal ) || g_Visuals.player_fading_alpha.at( i ) <= 0.4f )
				continue;

			const Vector vecOrigin = ( player->IsDormant( ) && g_ExtendedVisuals.m_arrOverridePlayers.at( i ).m_eOverrideType != EOverrideType::ESP_NONE ) ?
				g_ExtendedVisuals.m_arrOverridePlayers.at( i ).m_vecOrigin
				: player->m_vecOrigin( );

			const Vector center = vecOrigin;

			const Vector delta = center - vFinalPoint;

			if( m_iWeapIndex == WEAPON_HEGRENADE ) {
				if( delta.Length( ) <= kHeDamageRadius ) {
					CGameTrace trace{ };
					CTraceFilter filter{ };
					filter.pSkip = player;

					g_pEngineTrace->TraceRay( Ray_t( vFinalPoint, center ), MASK_SHOT, ( ITraceFilter * )&filter, &trace );

					if( trace.hit_entity && trace.hit_entity == player ) {
						// TODO: make this perfect / reverse it
						// damage is not perfectly calculated
						const float d = ( ( vecOrigin - vFinalPoint ).Length( ) - kHeDamageOffset ) / kHeDamageFalloff;
						float flDamage = kHeDamageBase * std::exp( -d * d );
						flDamage = static_cast< float >( std::max( static_cast< int >( std::ceilf( CSGOArmor( flDamage, player->m_ArmorValue( ) ) ) ), 0 ) );

						if( flDamage > target.first ) {
							target.first = flDamage;
							target.second = player;
						}
					}
				}
			}
			else if( m_iWeapIndex == WEAPON_MOLOTOV || m_iWeapIndex == WEAPON_INC ) {
				// is within damage radius?
				if( delta.Length( ) > 131.f )
					continue;

				// flames don't climb, so a puddle well above/below them is harmless
				if( std::fabsf( vecOrigin.z - vFinalPoint.z ) > kFireVerticalReach )
					continue;

				// molotov landing in smoke gets extinguished
				if( IsInsideSmoke( vFinalPoint ) )
					continue;

				// hardcoded bullshit /shrug
				target.first = 10.f;
				target.second = player;
			}
		}

		if( target.second ) {
			Vector2D screen{ };
			if( Render::Engine::WorldToScreen( vFinalPoint, screen ) ) {
				if( m_iWeapIndex == WEAPON_MOLOTOV || m_iWeapIndex == WEAPON_INC ) {
					if( bReached )
						Render::Engine::esp_bold_wpn.string( screen.x, screen.y + 5,
														 Color( 255, 0, 0, 180 ),
														 std::string( XorStr( "SPREAD" ) ), Render::Engine::ALIGN_CENTER );
				}
				else if( m_iWeapIndex == WEAPON_HEGRENADE ) {
					Render::Engine::esp_bold_wpn.string( screen.x, screen.y + 5,
													 target.first >= target.second->m_iHealth( ) ? Color( 255, 200, 60, 180 ) : Color( 255, 0, 0, 180 ),
													 std::string( XorStr( "DMG: " ) + std::to_string( ( int )target.first ) ), Render::Engine::ALIGN_CENTER );
				}
			}
		}
	}

	// from here on it's all just warning stuff
	if( m_bLocalPredicted || !g_Vars.esp.grenade_proximity_warning )
		return true;

	const Vector center = pLocal->GetAbsOrigin( );

	// https://i.imgur.com/whidDo4.png not sure, prob not related?
	if( center.IsZero( ) )
		return true;

	const bool bIsFire = ( m_iWeapIndex == WEAPON_MOLOTOV || m_iWeapIndex == WEAPON_INC );

	// clamp the fuse readout: curtime can overshoot m_flExpireTime between the
	// expiry check and here, which used to render negative timers / bars.
	const float flTotalTime = TICKS_TO_TIME( m_iTick );
	const float flTime = std::max( 0.f, m_flExpireTime - g_pGlobalVars->curtime );
	const float flPercent = flTotalTime > 0.f ? std::clamp( flTime / flTotalTime, 0.f, 1.f ) : 0.f;

	const Danger_t danger = EvaluateDanger( pLocal, vFinalPoint, bReached );

	char nade_buffer[ 64 ]{ };
	std::snprintf( nade_buffer, sizeof( nade_buffer ),
				   bIsFire ? XorStr( "FIRE (%.1fs)" ) : XorStr( "FRAG (%.1fs)" ), flTime );

	char damage_buffer[ 64 ]{ };
	if( danger.m_iDamage > 0 ) {
		const bool bLethal = danger.m_iDamage >= pLocal->m_iHealth( );

		std::snprintf( damage_buffer, sizeof( damage_buffer ),
					   bLethal ? XorStr( "LETHAL (-%i HP)" ) : XorStr( "UNSAFE (-%i HP)" ), danger.m_iDamage );
	}
	else {
		std::snprintf( damage_buffer, sizeof( damage_buffer ), "%s", XorStr( "UNSAFE" ) );
	}

	// green when harmless, ramping to red as it gets closer to killing us
	const Color colWarning = danger.m_bSafe
		? Color( 150, 200, 60, 240 )
		: Color::Blend( Color( 255, 200, 60, 240 ), Color( 255, 0, 0, 240 ), danger.m_flSeverity );

	// anchor the hud to the real detonation point. for fire that's the traced
	// ground position, which the old code computed but then ignored in favour of
	// the last trajectory sample.
	Vector2D vecDetonationScreen{ };
	if( Render::Engine::WorldToScreen( vFinalPoint, vecDetonationScreen ) ) {
		// fuse timer bar
		Render::Engine::RectFilled( vecDetonationScreen - Vector2D( 30, -10 ) - 1, Vector2D( 60, 2 ) + 2, Color( 0, 0, 0, 180 ) );
		Render::Engine::RectFilled( vecDetonationScreen - Vector2D( 30, -10 ), Vector2D( 60 * flPercent, 2 ),
									g_Vars.esp.grenade_proximity_warning_color.ToRegularColor( ).OverrideAlpha( 255, true ) );

		// main text
		Render::Engine::esp_bold_wpn.string( vecDetonationScreen.x, vecDetonationScreen.y, Color( 255, 255, 255, 240 ), nade_buffer, Render::Engine::ALIGN_CENTER );

		if( !danger.m_bSafe )
			Render::Engine::esp_bold_wpn.string( vecDetonationScreen.x, vecDetonationScreen.y - 10, colWarning, damage_buffer, Render::Engine::ALIGN_CENTER );
	}

	DrawIndicator( pLocal, vFinalPoint, danger, flPercent );

	return true;
}

void GrenadeWarning::GrenadeData_t::DrawIndicator( C_CSPlayer *pLocal, const Vector &vecDetonation, const Danger_t &danger, float flPercent ) {
	if( !pLocal || danger.m_bSafe || danger.m_flSeverity <= 0.f ) {
		m_bIndicatorValid = false;
		return;
	}

	const auto vecScreenSize = Render::GetScreenSize( );
	const float flScreenWidth = vecScreenSize.x;
	const float flScreenHeight = vecScreenSize.y;

	if( flScreenWidth <= 0.f || flScreenHeight <= 0.f )
		return;

	// give some extra room for screen position to be off screen.
	const float flExtraRoom = flScreenWidth / 18.f;

	Vector2D screenPos{ };
	const bool bOnScreen = Render::Engine::WorldToScreen( vecDetonation, screenPos )
		&& screenPos.x >= -flExtraRoom
		&& screenPos.x <= ( flScreenWidth + flExtraRoom )
		&& screenPos.y >= -flExtraRoom
		&& screenPos.y <= ( flScreenHeight + flExtraRoom );

	// already visible, the trail and timer say everything
	if( bOnScreen ) {
		m_bIndicatorValid = false;
		return;
	}

	const std::string strIcon = g_Visuals.GetWeaponIcon( m_iWeapIndex );
	if( strIcon.empty( ) ) {
		m_bIndicatorValid = false;
		return;
	}

	Vector vLocalOrigin = pLocal->GetAbsOrigin( );
	if( pLocal->IsDead( ) && g_pInput.IsValid( ) )
		vLocalOrigin = g_pInput->m_vecCameraOffset;

	QAngle angViewAngles{ };
	g_pEngine->GetViewAngles( angViewAngles );

	const Vector2D vecScreenCenter = Vector2D( flScreenWidth * .5f, flScreenHeight * .5f );
	const float flAngleYaw = DEG2RAD( angViewAngles.y - Math::CalcAngle( vLocalOrigin, vecDetonation, true ).y - 90.f );

	// the more dangerous it is, the further out (closer to the screen edge) it sits
	const float flRadius = ( ( flScreenHeight - 60.f ) * 0.5f ) * std::clamp( 1.f - danger.m_flSeverity, 0.25f, 0.85f );

	const Vector2D vecWish = Vector2D(
		vecScreenCenter.x + flRadius * std::cos( flAngleYaw ) + 8.f,
		vecScreenCenter.y + flRadius * std::sin( flAngleYaw ) + 8.f
	);

	// smoothing state lives on this grenade, so several nades no longer fight
	// over one shared static like the old (disabled) implementation did.
	if( !m_bIndicatorValid ) {
		m_vecIndicator = vecWish;
		m_bIndicatorValid = true;
	}
	else {
		m_vecIndicator.x = ApproachSmooth( m_vecIndicator.x, vecWish.x, 15.f );
		m_vecIndicator.y = ApproachSmooth( m_vecIndicator.y, vecWish.y, 15.f );
	}

	const auto vecIconSize = Render::Engine::cs_large.size( strIcon );
	const float flIconWidth = static_cast< float >( vecIconSize.m_width );
	const float flIconHeight = static_cast< float >( vecIconSize.m_height );

	const float flCircleX = m_vecIndicator.x;
	const float flCircleY = m_vecIndicator.y - ( flIconHeight * 0.5f );

	Render::Engine::CircleFilled( flCircleX, flCircleY, 17.f, 130,
								  Color::Blend( Color( 0, 0, 0 ), Color( 255, 0, 0 ), danger.m_flSeverity ).OverrideAlpha( 200 ) );
	Render::Engine::Circle( flCircleX, flCircleY, 17.f, 130, Color( 0, 0, 0, 220 ) );

	const float flIconX = flCircleX - ( flIconWidth * 0.5f );
	const float flIconY = m_vecIndicator.y - flIconHeight;

	// faint icon underneath, then fill it back up from the bottom as the fuse burns
	Render::Engine::cs_large.string( flIconX, flIconY, Color( 255, 255, 255, 40 ), strIcon );

	const float flFillHeight = flIconHeight * flPercent;
	if( flFillHeight > 0.f ) {
		Render::Engine::SetClip( Vector2D( flIconX, m_vecIndicator.y - flFillHeight ), Vector2D( flIconWidth, flFillHeight ) );
		Render::Engine::cs_large.string( flIconX, flIconY, Color( 255, 255, 255, 230 ), strIcon );
		Render::Engine::ResetClip( );
	}
}

void GrenadeWarning::Prune( ) {
	auto &listNades = GetList( );

	// drop grenades that already went off, whose entity is gone, or that were left
	// behind by a map change / disconnect. the old code cleared the whole list once
	// nothing had been predicted for 5s, which also threw away live grenades.
	for( auto it = listNades.begin( ); it != listNades.end( ); ) {
		const bool bExpired = g_pGlobalVars->curtime >= it->second.m_flExpireTime;
		const bool bGone = !it->second.GetNadeEntity( );

		if( bExpired || bGone )
			it = listNades.erase( it );
		else
			++it;
	}

	// same for the grenade_thrown bookkeeping; this used to grow for the whole
	// session because nothing ever removed matched/expired entries.
	const float flNow = TICKS_TO_TIME( g_pGlobalVars->tickcount );

	m_vecEventNades.erase(
		std::remove_if( m_vecEventNades.begin( ), m_vecEventNades.end( ),
						[ flNow ] ( const auto &nade ) -> bool {
							return std::fabsf( flNow - std::get< 1 >( nade ) ) > 20.f;
						} ),
		m_vecEventNades.end( ) );
}

void GrenadeWarning::Run( C_BaseEntity *entity ) {
	auto &listNades = GetList( );

	// prune once per frame rather than per entity
	static int nLastPruneFrame = -1;
	if( nLastPruneFrame != g_pGlobalVars->framecount ) {
		nLastPruneFrame = g_pGlobalVars->framecount;

		Prune( );
	}

	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	if( !entity )
		return;

	// key on the ehandle value. the old code used the dereferenced entity pointer,
	// which is unstable across entity recycling and made the '0 means unmatched'
	// sentinel in m_vecEventNades ambiguous.
	const auto ulHandle = static_cast< unsigned long >( entity->GetRefEHandle( ).ToInt( ) );

	if( ( entity->IsDormant( ) && listNades.find( ulHandle ) == listNades.end( ) ) )
		return;

	if( !entity->IsDormant( ) ) {
		const auto pClientClass = entity->GetClientClass( );
		if( !pClientClass || pClientClass->m_ClassID != CMolotovProjectile && pClientClass->m_ClassID != CBaseCSGrenadeProjectile /*&& pClientClass->m_ClassID != CDecoyProjectile && pClientClass->m_ClassID != CSmokeGrenadeProjectile*/ )
			return;

		if( pClientClass->m_ClassID == CBaseCSGrenadeProjectile ) {
			const auto pModel = entity->GetModel( );
			if( !pModel )
				return;

			const auto pStudiomodel = g_pModelInfo->GetStudiomodel( pModel );
			if( !pStudiomodel || std::string_view( pStudiomodel->szName ).find( XorStr( "fraggrenade" ) ) == std::string::npos )
				return;
		}


		// i'm so sry
		//static auto m_nExplodeEffectTickBeginOffset = Engine::g_PropManager.GetOffset( XorStr( "DT_BaseCSGrenadeProjectile" ), XorStr( "m_nExplodeEffectTickBegin" ) );
		static auto m_hThrowerOffset = Engine::g_PropManager.GetOffset( XorStr( "DT_BaseCSGrenadeProjectile" ), XorStr( "m_hThrower" ) );
		static auto m_vInitialVelocityOffset = Engine::g_PropManager.GetOffset( XorStr( "DT_BaseCSGrenadeProjectile" ), XorStr( "m_vInitialVelocity" ) );

		//auto m_nExplodeEffectTickBegin = *( int * )( uintptr_t( entity ) + m_nExplodeEffectTickBeginOffset );
		auto m_hThrower = *( CBaseHandle * )( uintptr_t( entity ) + m_hThrowerOffset );
		auto m_vInitialVelocity = *( Vector * )( uintptr_t( entity ) + m_vInitialVelocityOffset );

		if( !m_hThrower.IsValid( ) || !m_hThrower.Get( ) || ( ( ( C_BaseEntity * )m_hThrower.Get( ) )->m_iTeamNum( ) == pLocal->m_iTeamNum( ) && ( ( C_BaseEntity * )m_hThrower.Get( ) )->EntIndex( ) != g_pEngine->GetLocalPlayer( ) ) ) {
			listNades.erase( ulHandle );

			// will only remove hegrenades
			// molotov/fire will stay coz the current scope will never be hit
			const auto it = std::find_if( m_vecEventNades.begin( ), m_vecEventNades.end( ), [ ulHandle ] ( const auto &it ) -> bool { return std::get<2>( it ) == ulHandle; } );
			if( it != m_vecEventNades.end( ) ) {
				m_vecEventNades.erase( it );
			}

			return;
		}

		// only re-simulate when the projectile actually received a new update.
		// the old code erased and rebuilt the entry on every frame, which meant a
		// full 60s physics prediction per grenade per frame.
		const float flSimulationTime = entity->m_flSimulationTime( );

		bool bCanShift = false;
		if( reinterpret_cast< C_CSPlayer * >( m_hThrower.Get( ) )->EntIndex( ) == pLocal->EntIndex( ) )
			bCanShift = g_TickbaseController.CanShift( false, true ) && g_TickbaseController.m_bBreakingLC && g_Vars.rage.exploit && g_Vars.rage.double_tap_bind.enabled;

		const int nShift = bCanShift ? g_TickbaseController.GetCorrection( ) : 0;

		const auto itExisting = listNades.find( ulHandle );
		const bool bNeedsPredict = itExisting == listNades.end( )
			|| itExisting->second.m_flPredictedSimTime != flSimulationTime
			|| itExisting->second.m_iPredictedOffset != nShift;

		if( bNeedsPredict ) {
			float flSpawnTime = 0.f;

			for( auto &a : m_vecEventNades ) {
				if( std::get<0>( a ) == ( ( C_BaseEntity * )m_hThrower.Get( ) )->EntIndex( ) ) {
					const float flDelta = std::fabsf( TICKS_TO_TIME( g_pGlobalVars->tickcount ) - std::get<1>( a ) );

					// flDelta is only for the initially decision, if this nade is the right one to take
					// as we don't clear m_vecEventNades it might take a wrong spawntime else (a too old one)
					// just filtering smh..
					if( ( std::get<2>( a ) == 0 && flDelta <= 10.f ) || std::get<2>( a ) == ulHandle ) {
						flSpawnTime = std::get<1>( a );

						std::get<2>( a ) = ulHandle;
					}
				}
			}

			int iWeaponIndex = WEAPON_HEGRENADE;
			switch( pClientClass->m_ClassID ) {
				case CMolotovProjectile:
					iWeaponIndex = WEAPON_MOLOTOV;
					break;
					//case CDecoyProjectile:
					//	iWeaponIndex = WEAPON_DECOY;
					//	break;
					//case CSmokeGrenadeProjectile:
					//	iWeaponIndex = WEAPON_SMOKEGRENADE;
					//	break;
			}

			const int nOffset = TIME_TO_TICKS( flSimulationTime ) - TIME_TO_TICKS( flSpawnTime ) + nShift;

			// preserve the smoothed indicator position across re-predictions so the
			// off-screen arrow doesn't snap every time the projectile updates
			Vector2D vecIndicator{ };
			bool bIndicatorValid = false;
			if( itExisting != listNades.end( ) ) {
				vecIndicator = itExisting->second.m_vecIndicator;
				bIndicatorValid = itExisting->second.m_bIndicatorValid;
			}

			auto &data = listNades[ ulHandle ] = GrenadeData_t( ( C_CSPlayer * )m_hThrower.Get( ),
				iWeaponIndex,
				entity->m_vecOrigin( ), reinterpret_cast< C_CSPlayer * >( entity )->m_vecVelocity( ),
				flSpawnTime,
				/*g_pGlobalVars->tickcount - TIME_TO_TICKS( flSpawnTime )*/
				nOffset, entity );

			data.m_flPredictedSimTime = flSimulationTime;
			data.m_iPredictedOffset = nShift;
			data.m_vecIndicator = vecIndicator;
			data.m_bIndicatorValid = bIndicatorValid;
		}
	}

	if( listNades.find( ulHandle ) != listNades.end( ) ) {
		// now draw it!
		listNades.at( ulHandle ).Draw( );
	}
}

void GrenadeWarning::PredictLocal( ) {
	// lazy way of resetting it all :) 
	m_data = { };

	if( !g_Vars.esp.grenade_prediction ) {
		m_data.m_Path.clear( );
		return;
	}

	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal || pLocal->IsDead( ) )
		return;

	const auto pWeapon = reinterpret_cast< C_WeaponCSBaseGun * >( pLocal->m_hActiveWeapon( ).Get( ) );
	if( !pWeapon )
		return;

	auto pWeaponData = pWeapon->GetCSWeaponData( );
	if( !pWeaponData.IsValid( ) )
		return;

	if( pWeaponData->m_iWeaponType != WEAPONTYPE_GRENADE )
		return;

	if( !pWeapon->m_bPinPulled( ) )
		return;

	//if( pWeapon->m_fThrowTime( ) > 0.f )
	//	return;

	//static auto m_iPrimaryAmmoType = Engine::g_PropManager.GetOffset( XorStr( "DT_BaseCombatWeapon" ), XorStr( "m_iPrimaryAmmoType" ) );
	//static auto m_iAmmo = Engine::g_PropManager.GetOffset( XorStr( "DT_BasePlayer" ), XorStr( "m_iAmmo" ) );

	//int iAmmoType = *reinterpret_cast< int * >( uintptr_t( pWeapon ) + m_iPrimaryAmmoType );
	//if( iAmmoType < 0 || iAmmoType > 31 )
	//	return;

	//int *pAmmo = reinterpret_cast< int * >( uintptr_t( pLocal ) + m_iAmmo );
	//if( !pAmmo )
	//	return;

	//if( pAmmo[ iAmmoType ] <= 0 )
	//	return;

	// setup stuff.
	m_data.m_pOwner = pLocal;
	m_data.m_iWeapIndex = pWeapon->m_iItemDefinitionIndex( );
	m_data.m_bLocalPredicted = true;

	QAngle angThrow{ };

	// get view angles (direction)
	g_pEngine->GetViewAngles( angThrow );

	float pitch = angThrow.pitch;

	if( pitch <= 90.0f ) {
		if( pitch < -90.0f ) {
			pitch += 360.0f;
		}
	}
	else {
		pitch -= 360.0f;
	}
	float a = pitch - ( 90.0f - fabs( pitch ) ) * 10.0f / 90.0f;
	angThrow.pitch = a;

	// get ThrowVelocity from weapon files.
	float flVel = pWeaponData->m_flThrowVelocity * 0.9f;

	// clipped to [ 15, 750 ]
	Math::Clamp( flVel, 15.f, 750.f );

	//clamp the throw strength ranges just to be sure
	float flClampedThrowStrength = pWeapon->m_flThrowStrength( );
	flClampedThrowStrength = std::clamp( flClampedThrowStrength, 0.0f, 1.0f );

	flVel *= Lerp( flClampedThrowStrength, 0.3f, 1.0f );

	Vector vForward, vRight, vUp;
	vForward = angThrow.ToVectors( &vRight, &vUp );

	// danke DucaRii, ich liebe dich
	// dieses code snippet hat mir so sehr geholfen https://cdn.discordapp.com/attachments/755873329151475845/762297342623088640/unknown.png
	// thanks DucaRii, you are the greatest
	// loveeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee
	// kochamy DucaRii
	auto vecSrc = pLocal->GetAbsOrigin( ) + pLocal->m_vecViewOffset( );
	float off = Lerp( flClampedThrowStrength, -12.f, 0.0f );
	vecSrc.z += off;

	// Game calls UTIL_TraceHull here with hull and assigns vecSrc tr.endpos
	CGameTrace tr;
	Vector vecDest = vecSrc;
	vecDest = ( vecDest + vForward * 22.0f );
	TraceHull( vecSrc, vecDest, { -2.f, -2.f, -2.f }, { 2.f, 2.f, 2.f }, MASK_SOLID | CONTENTS_CURRENT_90, pLocal, COLLISION_GROUP_NONE, &tr );

	// after the hull trace it moves 6 units back along vForward
	// vecSrc = tr.endpos - vForward * 6
	Vector vecBack = vForward; vecBack *= 6.0f;
	vecSrc = tr.endpos;
	vecSrc -= vecBack;

	// kurwa fix for anti-aim micromovements (c) NICO
	Vector vecThrow{ };

	auto velocity = pLocal->m_vecVelocity( );
	if( velocity.Length2D( ) > 3.4 ) {
		vecThrow = velocity;
	}
	else {
		vecThrow.Init( );
	}

	vecThrow *= 1.25f;
	vecThrow += ( vForward * flVel );

	bool bCanShift = g_TickbaseController.CanShift( false, true ) && g_TickbaseController.m_bBreakingLC && g_Vars.rage.exploit && g_Vars.rage.double_tap_bind.enabled;
	const int nOffset = bCanShift ? g_TickbaseController.GetCorrection( ) : 0;
	m_data.Predict( vecSrc, vecThrow, g_pGlobalVars->curtime, nOffset );
}

void GrenadeWarning::DrawLocal( ) {

	if( !g_Vars.esp.grenade_prediction ) {
		m_data.m_Path.clear( );
		return;
	}

	PredictLocal( );

	if( !m_data.Draw( ) ) {
		return;
	}

	m_data.m_Path.clear( );
}
