#include "ShotHandling.hpp"
#include "Resolver.hpp"
#include "../Visuals/EventLogger.hpp"
#include "TickbaseShift.hpp"
#include "../Visuals/Models.hpp"
#include "../Visuals/Models.hpp"

ShotHandling g_ShotHandling;

std::string HitgroupToName_( const int hitgroup ) {
	switch( hitgroup ) {
		case Hitgroup_Head:
			return XorStr( "HEAD" );
		case Hitgroup_LeftLeg:
		case Hitgroup_RightLeg:
			return XorStr( "LEG" );
		case Hitgroup_LeftArm:
		case Hitgroup_RightArm:
			return XorStr( "ARM" );
		case Hitgroup_Stomach:
			return XorStr( "STOMACH" );
		default:
			return XorStr( "BODY" );
	}
}

// distance from a point to the record's collision hull. 0.f when the point is inside it.
// used to tell apart "bullet hit world geometry" from "bullet hit the enemy, just not
// where we aimed" - both of those look identical to the raw distance comparison.
static float DistanceToRecordHull( const LagRecord_t &record, const Vector &point ) {
	const Vector vecMins = record.m_vecOrigin + record.m_vecMins;
	const Vector vecMaxs = record.m_vecOrigin + record.m_vecMaxs;

	Vector vecDelta{ 0.f, 0.f, 0.f };

	for( int i = 0; i < 3; ++i ) {
		if( point[ i ] < vecMins[ i ] )
			vecDelta[ i ] = vecMins[ i ] - point[ i ];
		else if( point[ i ] > vecMaxs[ i ] )
			vecDelta[ i ] = point[ i ] - vecMaxs[ i ];
	}

	return vecDelta.Length( );
}

std::string ShotHandling::HitgroupToString( int hitgroup ) {
	switch( hitgroup ) {
		case Hitgroup_Generic:
			return XorStr( "generic" );
		case Hitgroup_Head:
			return XorStr( "head" );
		case Hitgroup_Chest:
			return XorStr( "chest" );
		case Hitgroup_Stomach:
			return XorStr( "stomach" );
		case Hitgroup_LeftArm:
			return XorStr( "left arm" );
		case Hitgroup_RightArm:
			return XorStr( "right arm" );
		case Hitgroup_LeftLeg:
			return XorStr( "left leg" );
		case Hitgroup_RightLeg:
			return XorStr( "right leg" );
		case Hitgroup_Neck:
			return XorStr( "neck" );
	}
	return XorStr( "generic" );
}

void ShotHandling::ProcessShots( ) {
	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	if( m_vecFilteredShots.empty( ) ) {
		return;
	}

	for( auto it = m_vecFilteredShots.begin( ); it != m_vecFilteredShots.end( ); ) {
		// closest impact to the eye position - this is where the bullet actually stopped.
		// never back( ), which is the FURTHEST penetrated surface.
		const Vector *pNearestImpact = it->GetNearestImpact( );

		// no impacts yet. leave the shot pending so the timeout path in GameEvent can
		// classify it instead of erasing it here without any reason at all.
		if( !pNearestImpact && it->m_vecDamage.empty( ) ) {
			it = next( it );
			continue;
		}

		// just for optimizations
		bool bDead = false;

		const auto player = C_CSPlayer::GetPlayerByIndex( it->m_ShotData.m_iTargetIndex );
		if( player ) {
			if( player->IsDead( ) ) {
				bDead = true;
			}
		}

		const bool weHit = it->m_vecDamage.size( );

		if( weHit ) {
			for( const auto &hit : it->m_vecDamage ) {
				if( hit.m_iTargetIndex != it->m_ShotData.m_iTargetIndex )
					continue;

				// попали туда, куда целились - гипотеза рабочая. иначе угол был неверный, прогресс не трогаем
				if( hit.m_iHitgroup != it->m_ShotData.m_iTraceHitgroup )
					continue;

				auto &resolverData = g_Resolver.m_arrResolverData.at( it->m_ShotData.m_iTargetIndex );
				resolverData.ResetMissedShots( it->m_ShotData.m_pRecord.m_eResolverStage );
				break;
			}

			for( int n = 0; n < it->m_vecDamage.size( ); ++n ) {
				auto hurt_info = it->m_vecDamage[ n ];

				// we hit something else but wanted head and we were considered accurate?
				// let's increment.
				//if( hurt_info.m_iHitgroup != it->m_ShotData.m_iTraceHitgroup && it->m_ShotData.m_iTraceHitgroup == Hitgroup_Head && it->m_ShotData.m_iHitchance == 100 && !it->m_ShotData.m_pRecord.m_vecVelocity.z ) {
				//	// g_EventLog.PushEvent( XorStr( "hit improperly resolved shot" ), Color_f( 0.69f, 0.69f, 0.69f ), false, std::to_string( it->m_ShotData.m_iShotID ) );

				//	g_Resolver.m_arrResolverData.at( it->m_ShotData.m_iTargetIndex ).Increment( it->m_ShotData.m_pRecord.m_eResolverStage, it->m_ShotData.m_pRecord.m_iResolverType );
				//}

				// handle deduction resolver. a hurt event can arrive without any
				// bullet_impact, and the deduction pass has no ray to trace then.
				// вычёркивать есть смысл, только если попали в НАШУ цель, но НЕ в тот хитгруп, куда целились
				if( pNearestImpact &&
					hurt_info.m_iTargetIndex == it->m_ShotData.m_iTargetIndex &&
					it->m_ShotData.m_iTraceHitgroup >= 0 &&
					hurt_info.m_iHitgroup != it->m_ShotData.m_iTraceHitgroup ) {
					ImpactInfo_t missInfo;
					missInfo.m_bDealtDamage = true;
					missInfo.m_vecStart = it->m_ShotData.m_vecStart;
					missInfo.m_vecEnd = *pNearestImpact;
					missInfo.m_iExpectedHitgroup = it->m_ShotData.m_iTraceHitgroup;

					g_Resolver.OnBulletImpact( &it->m_ShotData.m_pRecord, &missInfo );
				}

				if( g_Vars.misc.event_dmg ) {
					std::stringstream msg;

					player_info_t enemy_info{ };
					if( g_pEngine->GetPlayerInfo( hurt_info.m_iTargetIndex, &enemy_info ) ) {
						std::stringstream msg;

						// ye lol maybe we hit other player with this shoot (too)
						std::string playerName{ enemy_info.szName };
						if( playerName.find( XorStr( "\n" ) ) != std::string::npos ) {
							playerName = XorStr( "[BLANK]" );
						}

						auto damage = hurt_info.m_iDamage;
						damage = std::clamp( damage, 0, 100 );
						const int green = 255 - damage * 2.55;
						const int red = damage * 2.55;

						char damage_buffer[ 512 ] = {};
						sprintf( damage_buffer, XorStr( "did -%d in %s to %s\n" ),
								 hurt_info.m_iDamage,
								 HitgroupToName_( hurt_info.m_iHitgroup ).data( ),
								 playerName.data( ) );

						// std::to_string( it->m_ShotData.m_iShotID )
						g_EventLog.PushEvent( damage_buffer, Color_f( 255, 255, 255 ), true, "", false );

						g_pCVar->ConsoleColorPrintf( Color( 205, 92, 92, 255 ), XorStr( "[ DAMAGE ] " ) );
						g_pCVar->ConsoleColorPrintf( Color( red, green, 0, 255 ), XorStr( "-%d " ), hurt_info.m_iDamage );
						g_pCVar->ConsoleColorPrintf( Color( 255, 255, 255, 255 ), XorStr( "in " ) );
						g_pCVar->ConsoleColorPrintf( Color( 255, 204, 204, 255 ), XorStr( "%s " ), HitgroupToName_( hurt_info.m_iHitgroup ).data( ) );
						g_pCVar->ConsoleColorPrintf( Color( 255, 255, 255, 255 ), XorStr( "to " ) );
						g_pCVar->ConsoleColorPrintf( Color( 255, 229, 204, 255 ), XorStr( "%s\n" ), playerName.data( ) );
					}
				}
			}

		}
		else {
			// intersection with resolved matrix
			// when true we missed due to resolver
			const bool bHit = bDead ? false : TraceShot( &*it );

			std::stringstream reason{ };

			if( !bDead ) {
				// handle miss logs here
				const float flPointDistance = it->m_ShotData.m_vecStart.Distance( it->m_ShotData.m_vecEnd );

				auto &resolverData = g_Resolver.m_arrResolverData.at( it->m_ShotData.m_iTargetIndex );

				// handle deduction resolver
				ImpactInfo_t missInfo;
				missInfo.m_bDealtDamage = false;
				missInfo.m_vecStart = it->m_ShotData.m_vecStart;
				// путь пули целиком, включая прострелы - берём самый дальний импакт
				Vector vecFurthest = *pNearestImpact;
				float flFurthest = 0.f;
				for( const auto &impact : it->m_vecImpacts ) {
					const float flDist = it->m_ShotData.m_vecStart.Distance( impact );
					if( flDist > flFurthest ) {
						flFurthest = flDist;
						vecFurthest = impact;
					}
				}
				missInfo.m_vecEnd = vecFurthest;
				missInfo.m_iExpectedHitgroup = it->m_ShotData.m_iTraceHitgroup;

				g_Resolver.OnBulletImpact( &it->m_ShotData.m_pRecord, &missInfo );

				// resolver feedback, shared by the "resolver" and "animations" branches.
				// only ever called once per shot.
				auto fnCountResolverMiss = [ & ] ( ) {
					bool bOverriding = g_Vars.rage.resolver;
					if( !g_Vars.rage.resolver_override_key.enabled )
						bOverriding = false;

					// don't count misses when overriding
					if( bOverriding )
						return;

					resolverData.IncrementMissedShots( it->m_ShotData.m_pRecord.m_eResolverStage );

					// blame the hypothesis the record actually used. matching on
					// m_iResolverType 12/15 only worked in OnPlayerStand - the trial
					// resolver reports 78 for every post-miss angle, so these two
					// counters were never updated under resolver_trial.
					if( it->m_ShotData.m_pRecord.m_bUsedFreestand ) {
						resolverData.m_bMissedFreestand = true;
					}

					if( it->m_ShotData.m_pRecord.m_bUsedSameLBY ) {
						if( resolverData.m_nMissedBody == -1 )
							resolverData.m_nMissedBody = 0;

						resolverData.m_nMissedBody++;

						// не попали по LBY "по таймеру" -> это брейкер с тем же LBY, перестаём ему верить
						resolverData.m_bBodyTimerFailed = true;
					}
				};

				// bullet_impact приходит на КАЖДУЮ поверхность входа: ближайший импакт - это первая
				// стенка, даже если пуля её пробила. реально пуля остановилась в самом дальнем.
				const float flStopDistance = flFurthest;
				const bool bStoppedShort = flPointDistance > flStopDistance + 8.f;

				// была ли пуля хоть где-то рядом с целью
				float flHullDistance = FLT_MAX;
				for( const auto &impact : it->m_vecImpacts )
					flHullDistance = std::min( flHullDistance, DistanceToRecordHull( it->m_ShotData.m_pRecord, impact ) );

				CGameTrace trWorld{ };
				CTraceFilterWorldOnly worldFilter;
				g_pEngineTrace->TraceRay( Ray_t( it->m_ShotData.m_vecStart, it->m_ShotData.m_vecEnd ), MASK_SHOT, &worldFilter, &trWorld );

				const bool bWorldBlocked = trWorld.fraction < 0.999f;
				const float flWallDist = it->m_ShotData.m_vecStart.Distance( trWorld.endpos );

				const bool bReachedTarget = flHullDistance <= 32.f;

				// умерла в стене: остановилась раньше точки прицела, и до места остановки
				// на пути есть геометрия мира. пробила стенку и улетела дальше - это НЕ world.
				const bool bDiedInWall = bStoppedShort && bWorldBlocked && flWallDist <= flStopDistance + 8.f;

				if( bDiedInWall ) {
					// Реальная геометрия: наш автовол посчитал, что пройдёт, а сервер нет.
					reason << XorStr( "world" );
				}
				else if( bHit ) {
					// Направление верное (по серверной матрице попали бы), а урона нет.
					fnCountResolverMiss( );
					reason << XorStr( "resolver" );
				}
				else if( bReachedTarget ) {
					// Долетело до тушки, но мимо: угол резолва неверный, а не «окклюжн».
					// Раньше это называлось occlusion (target) и мисс резолву не засчитывался.
					fnCountResolverMiss( );
					reason << XorStr( "resolver" );
				}
				else if( it->m_ShotData.m_pRecord.m_bIsResolved ) {
					// Полный пролёт при правдоподобном угле: разъехался анимстейт.
					reason << XorStr( "animations" );

					if( !it->m_ShotData.m_bTapShot ) {
						fnCountResolverMiss( );
					}
				}
				else {
					// Путь чист или прошла навылет, остановилась в пустоте: разброс.
					reason << XorStr( "spread" );
				}

				if( it->m_ShotData.m_bHadPredError ) {
					if( !reason.str( ).size( ) )
						reason << XorStr( "prediction" );
				}
			}
			else {
				reason << XorStr( "enemy death" );
			}

			ReportMiss( *it, reason.str( ) );
		}

		it = m_vecFilteredShots.erase( it );
	}
}

void ShotHandling::RegisterShot( ShotInformation_t &shot ) {
	m_vecShots.emplace_back( ).m_ShotData = shot;
}

void ShotHandling::ReportMiss( const ShotEvents_t &shot, const std::string &szReason ) {
	const std::string szText = szReason.empty( ) ? XorStr( "unknown" ) : szReason;

	char damage_buffer[ 512 ] = {};
	sprintf( damage_buffer, XorStr( "missed due to %s\n" ), szText.data( ) );

	g_EventLog.PushEvent( damage_buffer, Color_f( 255, 255, 255 ), true, "", false );

	g_pCVar->ConsoleColorPrintf( Color( 51, 171, 249, 255 ), XorStr( "[ MISS ] " ) );
	g_pCVar->ConsoleColorPrintf( Color( 255, 204, 204, 255 ), XorStr( "missed due to %s" ), szText.data( ) );
	g_pCVar->ConsoleColorPrintf( Color( 100, 100, 255, 255 ), XorStr( " [ " ) );
	g_pCVar->ConsoleColorPrintf( Color( 255, 255, 255, 255 ), XorStr( "SHOT: " ) );
	g_pCVar->ConsoleColorPrintf( Color( 100, 100, 100 ), XorStr( "%i " ), shot.m_ShotData.m_iShotID % 100 );
	g_pCVar->ConsoleColorPrintf( Color( 100, 100, 255, 255 ), XorStr( "]\n" ) );
}

ShotEvents_t *ShotHandling::MatchShotByImpact( const Vector &vecImpact ) {
	if( m_vecFilteredShots.empty( ) )
		return nullptr;

	// single shot in flight: nothing to disambiguate.
	if( m_vecFilteredShots.size( ) == 1 )
		return &m_vecFilteredShots.back( );

	// the impact lies on (or very near) the firing ray of the shot that produced it,
	// so pick the shot whose ray passes closest to it. ties go to the oldest shot,
	// since impacts arrive in firing order.
	ShotEvents_t *pBest = nullptr;
	float flBestDistance = FLT_MAX;

	for( auto &shot : m_vecFilteredShots ) {
		const float flDistance = shot.DistanceToRay( vecImpact );
		if( flDistance < flBestDistance ) {
			flBestDistance = flDistance;
			pBest = &shot;
		}
	}

	return pBest ? pBest : &m_vecFilteredShots.back( );
}

ShotEvents_t *ShotHandling::MatchShotByTarget( int nTargetIndex ) {
	if( m_vecFilteredShots.empty( ) )
		return nullptr;

	if( m_vecFilteredShots.size( ) == 1 )
		return &m_vecFilteredShots.back( );

	// prefer the oldest pending shot aimed at this player that has no damage yet -
	// hurt events arrive in firing order, one per bullet.
	ShotEvents_t *pFallback = nullptr;

	for( auto &shot : m_vecFilteredShots ) {
		if( shot.m_ShotData.m_iTargetIndex != nTargetIndex )
			continue;

		if( shot.m_vecDamage.empty( ) )
			return &shot;

		if( !pFallback )
			pFallback = &shot;
	}

	return pFallback ? pFallback : &m_vecFilteredShots.back( );
}

void ShotHandling::GameEvent( IGameEvent *pEvent ) {
	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	//if( m_vecShots.empty( ) && m_vecFilteredShots.empty( ) )
	//	return;

	if( !m_vecShots.empty( ) ) {
		const float flTimeout = g_Animations.m_flIncomingLatency + g_Animations.m_flOutgoingLatency + 0.55f;

		for( auto it = m_vecShots.begin( ); it != m_vecShots.end( ); ) {
			const float flAge = std::fabsf( it->m_ShotData.m_flTime - g_pGlobalVars->realtime );

			// shouldve registered by now, and it never did - the server never
			// acknowledged the shot at all.
			if( flAge >= flTimeout && it->m_vecImpacts.empty( ) && !it->m_ShotData.m_bMatched ) {
				ReportMiss( *it, pLocal->IsDead( ) ? XorStr( "local death" ) : XorStr( "server rejection" ) );

				// only drop the shot that actually expired. clearing the whole deque
				// here threw away every other in-flight shot along with it.
				it = m_vecShots.erase( it );
				continue;
			}

			if( flAge >= 1.f || it->m_ShotData.m_bMatched ) {
				it = m_vecShots.erase( it );
			}
			else {
				it = next( it );
			}
		}
	}

	// filtered shots that the server acknowledged (weapon_fire matched) but never
	// produced a single impact for. these used to be erased without any
	// classification at all in ProcessShots.
	if( !m_vecFilteredShots.empty( ) ) {
		const float flTimeout = g_Animations.m_flIncomingLatency + g_Animations.m_flOutgoingLatency + 0.55f;

		for( auto it = m_vecFilteredShots.begin( ); it != m_vecFilteredShots.end( ); ) {
			if( it->m_vecImpacts.empty( ) && it->m_vecDamage.empty( )
				&& std::fabsf( it->m_ShotData.m_flTime - g_pGlobalVars->realtime ) >= flTimeout ) {
				ReportMiss( *it, pLocal->IsDead( ) ? XorStr( "local death" ) : XorStr( "no impact" ) );

				it = m_vecFilteredShots.erase( it );
				continue;
			}

			it = next( it );
		}
	}

	switch( hash_32_fnv1a( pEvent->GetName( ) ) ) {
		case hash_32_fnv1a_const( "weapon_fire" ):
		{
			if( g_pEngine->GetPlayerForUserID( pEvent->GetInt( XorStr( "userid" ) ) ) != pLocal->EntIndex( ) || m_vecShots.empty( ) )
				return;

			if( !g_TickbaseController.IsCharged( ) && g_TickbaseController.m_nTicksAfterUncharge >= 14 ) {
				g_TickbaseController.m_nTicksAfterUncharge = 14;
			}

			const auto it = std::find_if( m_vecShots.begin( ), m_vecShots.end( ), [ & ] ( const ShotEvents_t &shot ) -> bool {
				return !shot.m_ShotData.m_bMatched;
			} );

			if( it == m_vecShots.end( ) )
				return;

			it->m_ShotData.m_bMatched = true;

			m_vecFilteredShots.emplace_back( *it );

			break;
		}
		case hash_32_fnv1a_const( "player_hurt" ):
		{
			auto index = g_pEngine->GetPlayerForUserID( pEvent->GetInt( XorStr( "userid" ) ) );
			auto target = C_CSPlayer::GetPlayerByIndex( index );
			auto attacker = g_pEngine->GetPlayerForUserID( pEvent->GetInt( XorStr( "attacker" ) ) );
			if( !target || target == pLocal || target->IsTeammate( pLocal ) || attacker != pLocal->EntIndex( ) )
				return;

			if( m_vecFilteredShots.empty( ) ) {
				if( g_Vars.misc.event_dmg ) {
					std::stringstream str{ };
					player_info_t enemy_info;
					if( g_pEngine->GetPlayerInfo( index, &enemy_info ) ) {
						std::string playerName{ enemy_info.szName };

						if( playerName.find( XorStr( "\n" ) ) != std::string::npos ) {
							playerName = XorStr( "[BLANK]" );
						}

						auto damage = pEvent->GetInt( XorStr( "dmg_health" ) );
						damage = std::clamp( damage, 0, 100 );
						const int green = 255 - damage * 2.55;
						const int red = damage * 2.55;

						char damage_buffer[ 512 ] = {};
						sprintf( damage_buffer, XorStr( "did -%d in %s to %s\n" ),
								 pEvent->GetInt( XorStr( "dmg_health" ) ),
								 HitgroupToName_( pEvent->GetInt( XorStr( "hitgroup" ) ) ).data( ),
								 playerName.data( ) );

						// std::to_string( it->m_ShotData.m_iShotID )
						g_EventLog.PushEvent( damage_buffer, Color_f( 255, 255, 255 ), true, "", false );

						g_pCVar->ConsoleColorPrintf( Color( 205, 92, 92, 255 ), XorStr( "[ DAMAGE ] " ) );
						g_pCVar->ConsoleColorPrintf( Color( red, green, 0, 255 ), XorStr( "-%d " ), pEvent->GetInt( XorStr( "dmg_health" ) ) );
						g_pCVar->ConsoleColorPrintf( Color( 255, 255, 255, 255 ), XorStr( "in " ) );
						g_pCVar->ConsoleColorPrintf( Color( 255, 204, 204, 255 ), XorStr( "%s " ), HitgroupToName_( pEvent->GetInt( XorStr( "hitgroup" ) ) ).data( ) );
						g_pCVar->ConsoleColorPrintf( Color( 255, 255, 255, 255 ), XorStr( "to " ) );
						g_pCVar->ConsoleColorPrintf( Color( 255, 229, 204, 255 ), XorStr( "%s\n" ), playerName.data( ) );
					}
				}
			}
			else {
				ShotEvents_t::player_hurt_t hurt{ };
				hurt.m_iDamage = pEvent->GetInt( XorStr( "dmg_health" ) );
				hurt.m_iHealth = pEvent->GetInt( XorStr( "health" ) );
				hurt.m_iTargetIndex = target->EntIndex( );
				hurt.m_iHitgroup = pEvent->GetInt( XorStr( "hitgroup" ) );

				if( auto pShot = MatchShotByTarget( hurt.m_iTargetIndex ); pShot )
					pShot->m_vecDamage.push_back( hurt );
			}

			break;
		}
		case hash_32_fnv1a_const( "bullet_impact" ):
		{
			if( m_vecFilteredShots.empty( ) )
				return;

			if( g_pEngine->GetPlayerForUserID( pEvent->GetInt( XorStr( "userid" ) ) ) != pLocal->EntIndex( ) )
				return;

			const Vector vecImpact{ pEvent->GetFloat( XorStr( "x" ) ), pEvent->GetFloat( XorStr( "y" ) ), pEvent->GetFloat( XorStr( "z" ) ) };

			auto pBack = MatchShotByImpact( vecImpact );
			if( !pBack )
				return;

			pBack->m_vecImpacts.emplace_back( vecImpact );
			if( !pBack->m_ShotData.m_bLogged ) {
				{
					// g_EventLog.PushEvent( pBack->m_ShotData.m_szFiredLog, Color_f( 0.8f, 0.8f, 0.8f ), false, std::to_string( m_vecFilteredShots.back( ).m_ShotData.m_iShotID ) );
					pBack->m_ShotData.m_bLogged = true;


					if( !pBack->m_ShotData.m_pRecord.m_pEntity )
						break;

					pBack->m_ShotData.m_pRecord.m_pEntity = C_CSPlayer::GetPlayerByIndex( pBack->m_ShotData.m_pRecord.m_pEntity->m_entIndex );
					if( !pBack->m_ShotData.m_pRecord.m_pEntity )
						break;

					if( g_Vars.esp.target_capsules > 0 ) {
						if( pBack->m_ShotData.m_pRecord.m_sAnims[ ESides::SIDE_SERVER ].m_pMatrix )
							pBack->m_ShotData.m_pRecord.m_pEntity->DrawHitboxMatrix( pBack->m_ShotData.m_pRecord.m_sAnims[ ESides::SIDE_SERVER ].m_pMatrix,
																					 g_Vars.esp.target_capsules == 1 ? pBack->m_ShotData.m_pHitbox : nullptr );
					}

					if( g_Vars.chams_aimbot.enabled ) {
						if( pBack->m_ShotData.m_pRecord.m_sAnims[ ESides::SIDE_SERVER ].m_pMatrix )
							g_Models.AddAimbotMatrix( pBack->m_ShotData.m_pRecord.m_pEntity, pBack->m_ShotData.m_pRecord.m_sAnims[ ESides::SIDE_SERVER ].m_pMatrix );
					}
				}

				// TODO: lua callback
			}

			break;
		}
	}
}

bool ShotHandling::TraceShot( ShotEvents_t *shot ) {
	if( !shot )
		return false;

	if( !shot->m_vecImpacts.size( ) )
		return false;

	if( !shot->m_ShotData.m_pHitbox || shot->m_ShotData.m_pHitbox == nullptr )
		return false;

	if( !shot->m_ShotData.m_pRecord.m_pEntity || shot->m_ShotData.m_pRecord.m_pEntity == nullptr )
		return false;

	if( !shot->m_ShotData.m_pRecord.m_pEntity->m_entIndex || shot->m_ShotData.m_pRecord.m_pEntity->m_entIndex > 64 )
		return false;

	const auto pMatrix = shot->m_ShotData.m_pRecord.m_sAnims[ ESides::SIDE_SERVER ].m_pMatrix;
	if( !pMatrix )
		return false;

	shot->m_ShotData.m_pRecord.m_pEntity = C_CSPlayer::GetPlayerByIndex( shot->m_ShotData.m_pRecord.m_pEntity->m_entIndex );

	int nCount = 0;
	for( auto &impact : shot->m_vecImpacts ) {
		CGameTrace tr{ };

		auto vecDirection = ( impact - shot->m_ShotData.m_vecStart ).Normalized( );
		auto vecServerImpact = shot->m_ShotData.m_vecStart + ( vecDirection * shot->m_ShotData.m_vecStart.Distance( shot->m_ShotData.m_pRecord.m_vecOrigin ) * 3.123456f );

		auto nRet = Autowall::ClipRayToHitbox( Ray_t( shot->m_ShotData.m_vecStart, vecServerImpact ), shot->m_ShotData.m_pHitbox, pMatrix[ shot->m_ShotData.m_pHitbox->bone ], tr );
		if( nRet >= 0 )
			++nCount;
	}


	bool bFirstReturn = nCount >= std::max( 1, int( shot->m_vecImpacts.size( ) * 0.66 ) );
	if( bFirstReturn )
		return true;

	// if bFirstReturn return false, let's try verify to see if we maybe missed due to resolver (but it didnt think we did at first)

	matrix3x4_t bone_transform;
	memcpy( &bone_transform, &shot->m_ShotData.m_pRecord.m_sAnims[ 0 ].m_pMatrix[ shot->m_ShotData.m_pHitbox->bone ], sizeof( matrix3x4_t ) );
	if( !shot->m_ShotData.m_pHitbox->m_angAngles.IsZero( ) ) {
		matrix3x4_t temp;

		Math::AngleMatrix( shot->m_ShotData.m_pHitbox->m_angAngles, temp );
		Math::ConcatTransforms( bone_transform, temp, bone_transform );
	}

	Vector vMin, vMax;
	Math::VectorTransform( shot->m_ShotData.m_pHitbox->bbmin, bone_transform, vMin );
	Math::VectorTransform( shot->m_ShotData.m_pHitbox->bbmax, bone_transform, vMax );

	// reset this shit back to 0
	nCount = 0;
	for( auto &impact : shot->m_vecImpacts ) {
		CGameTrace tr{ };
		
		auto vecDirection = ( impact - shot->m_ShotData.m_vecStart ).Normalized( );
		auto vecServerImpact = shot->m_ShotData.m_vecStart + ( vecDirection * shot->m_ShotData.m_vecStart.Distance( shot->m_ShotData.m_pRecord.m_vecOrigin ) * 3.123456f );

		float m1, m2;
		float dist = Math::segment_to_segment( shot->m_ShotData.m_vecStart, vecServerImpact, vMin, vMax, m1, m2 );

		if( dist <= shot->m_ShotData.m_pHitbox->m_flRadius * shot->m_ShotData.m_pHitbox->m_flRadius ) {
			nCount++;
		}
	}

	// xd
	if( nCount >= std::max( 1, int( shot->m_vecImpacts.size( ) * 0.66 ) ) ) {
		g_EventLog.PushEvent( XorStr( "corrected spread detection" ), Color_f( 255, 255, 255 ), true );
		return true;
	}

	return false;
}