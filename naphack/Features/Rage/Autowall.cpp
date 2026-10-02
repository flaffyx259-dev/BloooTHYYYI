#include "Autowall.hpp"
#include "../../SDK/Displacement.hpp"
#include "../../SDK/Classes/player.hpp"
#include "../../SDK/Classes/weapon.hpp"

#include "../../Loader/Security/Security.hpp"

#include <Virtualizer/C/VirtualizerSDK.h>

// CS_MASK_SHOOT | CONTENTS_HITBOX из cs_player_shared.cpp
constexpr uint32_t CS_MASK_SHOOT_HITBOX = MASK_SHOT | CONTENTS_GRATE;
static_assert( CS_MASK_SHOOT_HITBOX == 0x4600400B, "bullet trace mask differs from server" );

//inline int g_AUTOWALL_CALLS = 0;

// IsBreakableEntity
// https://github.com/ValveSoftware/source-sdk-2013/blob/master/sp/src/game/shared/obstacle_pushaway.cpp
bool Autowall::IsBreakable( C_BaseEntity *pEntity ) {
	if( !pEntity || pEntity->m_entIndex == 0 || !pEntity->GetCollideable( ) )
		return false;

	const ClientClass *pClientClass = pEntity->GetClientClass( );
	if( !pClientClass || !pClientClass->m_pNetworkName )
		return false;

	const char *szName = pClientClass->m_pNetworkName;

	// стёкла и func_breakable
	if( !strcmp( szName, "CBreakableSurface" ) || !strcmp( szName, "CBaseEntity" ) )
		return true;

	// на сервере у них DAMAGE_NO - пулей не ломаются. клиентский m_takedamage этого не знает.
	if( !strcmp( szName, "CPropDoorRotating" ) || !strcmp( szName, "CBaseDoor" ) || !strcmp( szName, "CDynamicProp" ) )
		return false;

	using fnIsBreakable = bool( __thiscall * )( C_BaseEntity * );
	return ( ( fnIsBreakable )Engine::Displacement.Function.m_uIsBreakable )( pEntity );
}

bool Autowall::IsArmored( C_CSPlayer *player, int nHitgroup ) {
	const bool bHasHelmet = player->m_bHasHelmet( );
	const bool bHasHeavyArmor = player->m_bHasHeavyArmor( );
	const float flArmorValue = player->m_ArmorValue( );

	bool v2; // dl
	bool result; // al

	v2 = 0;

	if( flArmorValue <= 0.f )
		return false;

	// server.dll 55 8B EC 32 D2
	switch( nHitgroup ) {
		case 0:
		case 2:
		case 3:
		case 4:
		case 5:
		case 8:
			result = 1;
			//if( *( this + 5941 ) )
			if( bHasHeavyArmor )
				result = 1;
			break;
		case 1:
			v2 = bHasHelmet;
			result = v2;
			//if( *( this + 5941 ) )
			if( bHasHeavyArmor )
				result = 1;
			break;
		default:
			result = v2;
			//if( *( this + 5941 ) )
			if( bHasHeavyArmor )
				result = 1;
			break;
	}

	return result;
}

// references CCSPlayer::TraceAttack and CCSPlayer::OnTakeDamage
float Autowall::ScaleDamage( C_CSPlayer *player, float flDamage, float flArmorRatio, int nHitgroup, C_CSPlayer *pOverride ) {
	if( !player )
		return -1.f;

	C_CSPlayer *pLocal = pOverride ? pOverride : C_CSPlayer::GetLocalPlayer( );

	if( !pLocal )
		return -1.f;

	C_WeaponCSBaseGun *pWeapon = ( C_WeaponCSBaseGun * )pLocal->m_hActiveWeapon( ).Get( );

	if( !pWeapon )
		return -1.f;

	const int nTeamNum = player->m_iTeamNum( );
	float flHeadDamageScale = nTeamNum == TEAM_CT ? g_Vars.mp_damage_scale_ct_head->GetFloat( ) : g_Vars.mp_damage_scale_t_head->GetFloat( );
	const float flBodyDamageScale = nTeamNum == TEAM_CT ? g_Vars.mp_damage_scale_ct_body->GetFloat( ) : g_Vars.mp_damage_scale_t_body->GetFloat( );

	const bool bIsArmored = IsArmored( player, nHitgroup );
	const bool bHasHeavyArmor = player->m_bHasHeavyArmor( );
	const bool bIsZeus = pWeapon->m_iItemDefinitionIndex( ) == WEAPON_ZEUS;

	const float flArmorValue = static_cast< float >( player->m_ArmorValue( ) );

	if( bHasHeavyArmor )
		flHeadDamageScale *= 0.5f;

	if( !bIsZeus ) {
		switch( nHitgroup ) {
			case Hitgroup_Head:
				flDamage = ( flDamage * 4.f ) * flHeadDamageScale;
				break;
			case Hitgroup_Stomach:
				flDamage = ( flDamage * 1.25f ) * flBodyDamageScale;
				break;
			case Hitgroup_Chest:
			case Hitgroup_LeftArm:
			case Hitgroup_RightArm:
			case Hitgroup_Gear:
				flDamage = flDamage * flBodyDamageScale;
				break;
			case Hitgroup_LeftLeg:
			case Hitgroup_RightLeg:
				flDamage = ( flDamage * 0.75f ) * flBodyDamageScale;
				break;
			default:
				break;
		}
	}

	// enemy have armor
	if( bIsArmored ) {
		float flArmorScale = 1.f;
		float flArmorBonusRatio = 0.5f;
		float flArmorRatioCalculated = flArmorRatio * 0.5f;
		float fDamageToHealth = 0.f;

		if( bHasHeavyArmor ) {
			flArmorRatioCalculated = flArmorRatio * 0.25f;
			flArmorBonusRatio = 0.33f;

			flArmorScale = 0.33f;

			fDamageToHealth = ( flDamage * flArmorRatioCalculated ) * 0.85f;
		}
		else {
			fDamageToHealth = flDamage * flArmorRatioCalculated;
		}

		float fDamageToArmor = ( flDamage - fDamageToHealth ) * ( flArmorScale * flArmorBonusRatio );

		// Does this use more armor than we have?
		if( fDamageToArmor > flArmorValue )
			fDamageToHealth = flDamage - ( flArmorValue / flArmorBonusRatio );

		flDamage = fDamageToHealth;
	}

	return std::floorf( flDamage );
}

void Autowall::TraceLine( const Vector &start, const Vector &end, uint32_t mask, ITraceFilter *ignore, CGameTrace *ptr ) {
	Ray_t ray;
	ray.Init( start, end );
	g_pEngineTrace->TraceRay( ray, mask, ignore, ptr );
}

__forceinline float DistanceToRay( const Vector &vecPosition, const Vector &vecRayStart, const Vector &vecRayEnd, float *flAlong = NULL, Vector *vecPointOnRay = NULL ) {
	Vector vecTo = vecPosition - vecRayStart;
	Vector vecDir = vecRayEnd - vecRayStart;
	float flLength = vecDir.Normalize( );

	float flRangeAlong = DotProduct( vecDir, vecTo );
	if( flAlong ) {
		*flAlong = flRangeAlong;
	}

	float flRange;

	if( flRangeAlong < 0.0f ) {
		// off start point
		flRange = -vecTo.Length( );

		if( vecPointOnRay ) {
			*vecPointOnRay = vecRayStart;
		}
	}
	else if( flRangeAlong > flLength ) {
		// off end point
		flRange = -( vecPosition - vecRayEnd ).Length( );

		if( vecPointOnRay ) {
			*vecPointOnRay = vecRayEnd;
		}
	}
	else { // within ray bounds
		Vector vecOnRay = vecRayStart + vecDir * flRangeAlong;
		flRange = ( vecPosition - vecOnRay ).Length( );

		if( vecPointOnRay ) {
			*vecPointOnRay = vecOnRay;
		}
	}

	return flRange;
}


void Autowall::ClipTraceToPlayer( const Vector vecAbsStart, const Vector &vecAbsEnd, uint32_t iMask, ITraceFilter *pFilter, CGameTrace *pGameTrace, C_CSPlayer *player ) {
	if( !player )
		return;

	if( pFilter && !pFilter->ShouldHitEntity( player, iMask ) )
		return;

	constexpr float flMaxRange = 60.0f, flMinRange = 0.0f;
	float flSmallestFraction = pGameTrace->fraction;

	ICollideable *pCollideble = player->GetCollideable( );

	if( !pCollideble )
		return;

	// get bounding box
	const Vector vecObbMins = pCollideble->OBBMins( );
	const Vector vecObbMaxs = pCollideble->OBBMaxs( );
	const Vector vecObbCenter = ( vecObbMaxs + vecObbMins ) / 2.f;

	// calculate world space center
	const Vector vecPosition = vecObbCenter + player->m_vecOrigin( );

	Ray_t Ray;
	Ray.Init( vecAbsStart, vecAbsEnd );

	// calculate distance to ray
	const float flRange = DistanceToRay( vecPosition, vecAbsStart, vecAbsEnd );

	if( flRange < 0.0f || flRange > 60.0f )
		return;

	CGameTrace playerTrace;
	g_pEngineTrace->ClipRayToEntity( Ray, iMask, player, &playerTrace );
	if( playerTrace.fraction < flSmallestFraction ) {
		*pGameTrace = playerTrace;
		flSmallestFraction = pGameTrace->fraction;
	}
}

void Autowall::ClipTraceToPlayers( const Vector &vecAbsStart, const Vector &vecAbsEnd, uint32_t iMask, ITraceFilter *pFilter, CGameTrace *pGameTrace, float flMaxRange, float flMinRange ) {
	float flSmallestFraction = pGameTrace->fraction;

	Vector vecDelta( vecAbsEnd - vecAbsStart );
	const float flDelta = vecDelta.Normalize( );

	Ray_t Ray;
	Ray.Init( vecAbsStart, vecAbsEnd );

	for( int i = 1; i <= g_pGlobalVars->maxClients; ++i ) {
		C_CSPlayer *pPlayer = C_CSPlayer::GetPlayerByIndex( i );
		if( !pPlayer || pPlayer->IsDormant( ) || pPlayer->IsDead( ) )
			continue;

		if( pFilter && !pFilter->ShouldHitEntity( pPlayer, iMask ) )
			continue;

		auto pCollideble = pPlayer->GetCollideable( );
		if( !pCollideble )
			continue;

		// get bounding box
		const Vector vecObbMins = pCollideble->OBBMins( );
		const Vector vecObbMaxs = pCollideble->OBBMaxs( );
		const Vector vecObbCenter = ( vecObbMaxs + vecObbMins ) / 2.f;

		// calculate world space center
		const Vector vecPosition = vecObbCenter + pPlayer->GetAbsOrigin( );

		// calculate distance to ray
		const float flRange = DistanceToRay( vecPosition, vecAbsStart, vecAbsEnd );

		// one player outside the corridor must not abort the whole loop - Valve's
		// UTIL_ClipTraceToPlayers uses continue here. with `return` any closer
		// occluding player after this one was never clipped at all.
		if( flRange < flMinRange || flRange > flMaxRange )
			continue;

		CGameTrace playerTrace;
		g_pEngineTrace->ClipRayToEntity( Ray, iMask, pPlayer, &playerTrace );
		if( playerTrace.fraction < flSmallestFraction ) {
			// we shortened the ray - save off the trace
			*pGameTrace = playerTrace;
			flSmallestFraction = playerTrace.fraction;
		}
	}
}

bool Autowall::TraceToExit( CGameTrace *pEnterTrace, Vector vecStartPos, Vector vecDirection, CGameTrace *pExitTrace, C_CSPlayer *player ) {
	if( !player )
		return false;

	constexpr float flMaxDistance = 90.f, flStepSize = 4.f;
	float flCurrentDistance = 0.f;

	int iFirstContents = 0;

	do {
		// Add extra distance to our ray
		flCurrentDistance += flStepSize;

		// Multiply the direction vector to the distance so we go outwards, add our position to it.
		Vector vecEnd = vecStartPos + ( vecDirection * flCurrentDistance );

		// the literals here were exactly MASK_SHOT (0x4600400B) and MASK_SHOT_HULL
		// (0x600400B), which is what CCSPlayer::FireBullet uses. spelled out now.
		const int iPointContents = g_pEngineTrace->GetPointContents( vecEnd, CS_MASK_SHOOT_HITBOX );

		if( !iFirstContents )
			iFirstContents = iPointContents;

		if( !( iPointContents & MASK_SHOT_HULL ) || ( ( iPointContents & CONTENTS_HITBOX ) && iPointContents != iFirstContents ) ) {
			//Let's setup our end position by deducting the direction by the extra added distance
			Vector vecStart = vecEnd - ( vecDirection * flStepSize );

			// this gets a bit more complicated and expensive when we have to deal with displacements
			TraceLine( vecEnd, vecStart, CS_MASK_SHOOT_HITBOX, nullptr, pExitTrace );

			// lol we were missing this before..
			static ConVar *sv_clip_penetration_traces_to_players = g_pCVar->FindVar( XorStr( "sv_clip_penetration_traces_to_players" ) );
			if( sv_clip_penetration_traces_to_players ) {
				if( sv_clip_penetration_traces_to_players->GetBool( ) ) {
					uint32_t ClipTraceToPlayerFilter_[ 4 ] = { *reinterpret_cast< uint32_t * > ( Engine::Displacement.Function.m_TraceFilterSimple ), uint32_t( player ), 0, 0 };

					ClipTraceToPlayers( vecStart, vecEnd, CS_MASK_SHOOT_HITBOX, reinterpret_cast< CTraceFilter * >( ClipTraceToPlayerFilter_ ), pExitTrace, 60.f, -60.f );
				}
			}

			// we hit an ent's hitbox, do another trace.
			if( pExitTrace->startsolid && pExitTrace->surface.flags & SURF_HITBOX ) {

				uint32_t filter_[ 4 ] = { *reinterpret_cast< uint32_t * > ( Engine::Displacement.Function.m_TraceFilterSimple ), 0, 0, 0 };
				filter_[ 1 ] = reinterpret_cast< uint32_t >( pExitTrace->hit_entity );

				// do another trace, but skip the player to get the actual exit surface.
				// this has to run from the OUTER point back towards the entry point
				// (Ray( end, start ) in FireBullet); starting inside the solid at
				// vecStartPos just came back startsolid and lost the penetration.
				TraceLine( vecEnd, vecStartPos, MASK_SHOT_HULL, reinterpret_cast< ITraceFilter * >( filter_ ), pExitTrace );

				if( pExitTrace->DidHit( ) && !pExitTrace->startsolid ) {
					return true;
				}

				continue;
			}

			//Can we hit? Is the wall solid?
			if( pExitTrace->DidHit( ) && !pExitTrace->startsolid ) {
				const bool bStartIsNoDraw = ( pEnterTrace->surface.flags & SURF_NODRAW );
				const bool bExitIsNoDraw = ( pExitTrace->surface.flags & SURF_NODRAW );

				if( bExitIsNoDraw ) {
					if( IsBreakable( ( C_BaseEntity * )pEnterTrace->hit_entity ) && IsBreakable( ( C_BaseEntity * )pExitTrace->hit_entity ) )
						return true;
				}

				if( ( !bExitIsNoDraw || ( bStartIsNoDraw && bExitIsNoDraw ) )
					&& pExitTrace->plane.normal.Dot( vecDirection ) <= 1.f ) {
					return true;
				}

				continue;
			}

			if( !pExitTrace->DidHit( ) || pExitTrace->startsolid ) {
				if( pEnterTrace->DidHitNonWorldEntity( ) && IsBreakable( ( C_BaseEntity * )pEnterTrace->hit_entity ) ) {
					// if we hit a breakable, make the assumption that we broke it if we can't find an exit (hopefully..)
					// копируем В выходную трассу, а не переставляем указатель
					*pExitTrace = *pEnterTrace;
					pExitTrace->endpos = vecStartPos + vecDirection;
					return true;
				}
			}
		}
		// max pen distance is 90 units.
	} while( flCurrentDistance <= flMaxDistance );

	return false;
}

bool Autowall::HandleBulletPenetration( FireBulletData *data ) {
	if( !data )
		return true;

	int iEnterMaterial = data->m_EnterSurfaceData->game.material;
	const int nPenetrationSystem = g_Vars.sv_penetration_type->GetInt( );

	bool bSolidSurf = ( ( data->m_EnterTrace.contents >> 3 ) & CONTENTS_SOLID );
	bool bLightSurf = ( ( data->m_EnterTrace.surface.flags >> 7 ) & SURF_LIGHT );
	bool bContentsGrate = data->m_EnterTrace.contents & CONTENTS_GRATE;
	bool bNoDrawSurf = !!( data->m_EnterTrace.surface.flags & ( SURF_NODRAW ) ); // this is valve code :D!

	// check if bullet can penetrarte another entity
	if( data->m_iPenetrationCount == 0 &&
		!bContentsGrate &&
		!bNoDrawSurf &&
		iEnterMaterial != CHAR_TEX_GRATE &&
		iEnterMaterial != CHAR_TEX_GLASS )
		return true; // no, stop

	// if we hit a grate with iPenetration == 0, stop on the next thing we hit
	if( data->m_WeaponData->m_flPenetration <= 0.f || data->m_iPenetrationCount == 0 )
		return true;

	// find exact penetration exit
	CGameTrace ExitTrace = { };
	if( !TraceToExit( &data->m_EnterTrace, data->m_EnterTrace.endpos, data->m_vecDirection, &ExitTrace, data->m_Player ) ) {
		// ended in solid
		if( ( g_pEngineTrace->GetPointContents( data->m_EnterTrace.endpos, MASK_SHOT_HULL ) & MASK_SHOT_HULL ) == 0 )
			return true;
	}

	const surfacedata_t *pExitSurfaceData = g_pPhysicsSurfaceProps->GetSurfaceData( ExitTrace.surface.surfaceProps );

	if( !pExitSurfaceData )
		return true;

	const float flEnterPenetrationModifier = data->m_EnterSurfaceData->game.flPenetrationModifier;
	const float flExitPenetrationModifier = pExitSurfaceData->game.flPenetrationModifier;
	const float flExitDamageModifier = pExitSurfaceData->game.flDamageModifier;

	int iExitMaterial = pExitSurfaceData->game.material;

	//if( !data->m_bPenetration ) {
	//	printf( "%s %i | ", data->m_EnterTrace.surface.name, iEnterMaterial );
	//	printf( "%s %i\n", ExitTrace.surface.name, iExitMaterial );
	//}

	// https://www.unknowncheats.me/forum/counterstrike-global-offensive/474608-autowall-vs-de_cache-door.html
	if( hash_32_fnv1a( g_pEngine->GetLevelName( ) ) == hash_32_fnv1a_const( XorStr( "maps\\de_cache.bsp" ) ) && iEnterMaterial == iExitMaterial ) {
		// i hope xor'd const hashes are fine
		if( ExitTrace.hit_entity && hash_32_fnv1a( ExitTrace.hit_entity->GetClientClass( )->m_pNetworkName ) == hash_32_fnv1a_const( XorStr( "CPropDoorRotating" ) ) ) {
			iEnterMaterial = iExitMaterial = CHAR_TEX_WOOD;
		}
	}

	float flDamageModifier = 0.f;
	float flPenetrationModifier = 0.f;

	// percent of total damage lost automatically on impacting a surface
	flDamageModifier = 0.16f;
	flPenetrationModifier = ( flEnterPenetrationModifier + flExitPenetrationModifier ) * 0.5f;

	// new penetration method
	if( nPenetrationSystem == 1 ) {
		if( iEnterMaterial == CHAR_TEX_GRATE || iEnterMaterial == CHAR_TEX_GLASS ) {
			flDamageModifier = 0.05f;
			flPenetrationModifier = 3.0f;
		}
		else if( bSolidSurf || bLightSurf ) {
			flDamageModifier = 0.16f;
			flPenetrationModifier = 1.0f;
		}
		// for some weird reason some community servers have ff_damage_reduction_bullets > 0 but ff_damage_bullet_penetration == 0
		// so yeah, no shooting through teammates :)
		else if( iEnterMaterial == CHAR_TEX_FLESH && ( data->m_Player->IsTeammate( ( C_CSPlayer * )( data->m_EnterTrace.hit_entity ) ) ) &&
				 g_Vars.ff_damage_reduction_bullets->GetFloat( ) == 0.f ) {
			//Look's like you aren't shooting through your teammate today
			const float ffDamage = g_Vars.ff_damage_bullet_penetration->GetFloat( );
			if( ffDamage == 0.f )
				return true;

			//Let's shoot through teammates and get kicked for teamdmg! Whatever, atleast we did damage to the enemy. I call that a win.
			flPenetrationModifier = ffDamage;
			flDamageModifier = 0.16f;
		}
		else {
			// percent of total damage lost automatically on impacting a surface
			flDamageModifier = 0.16f;
			flPenetrationModifier = ( flEnterPenetrationModifier + flExitPenetrationModifier ) * 0.5f;
		}

		// if enter & exit point is wood we assume this is 
		// a hollow crate and give a penetration bonus
		if( iEnterMaterial == iExitMaterial ) {
			if( iExitMaterial == CHAR_TEX_WOOD || iExitMaterial == CHAR_TEX_CARDBOARD )
				flPenetrationModifier = 3.f;
			else if( iExitMaterial == CHAR_TEX_PLASTIC )
				flPenetrationModifier = 2.f;
		}

		// calculate damage  
		float flTraceDistance = ( ExitTrace.endpos - data->m_EnterTrace.endpos ).Length( );
		float flPenetrationMod = std::max( 0.0f, 1.0f / flPenetrationModifier );

		flTraceDistance = ( ( flTraceDistance * flTraceDistance ) * flPenetrationMod ) * 0.041666667f;

		auto flLostDamage = ( data->m_flCurrentDamage * flDamageModifier )
			+ std::max( 0.0f, 3.0f / data->m_WeaponData->m_flPenetration * 1.25f ) * ( flPenetrationMod * 3.0f )
			+ flTraceDistance;

		const float flClampedLostDamage = fmaxf( flLostDamage, 0.f );

		if( flClampedLostDamage > data->m_flCurrentDamage )
			return true;

		// reduce damage power each time we hit something other than a grate
		if( flClampedLostDamage > 0.0f )
			data->m_flCurrentDamage -= flClampedLostDamage;

		// do we still have enough damage to deal?
		if( data->m_flCurrentDamage < 1.0f )
			return true;

		// у Valve flDistance/flCurrentDistance передаются по значению, поэтому
		// "m_flMaxLength = ( m_flMaxLength - m_flTraceLength ) * 0.5" не имеет эффекта.
		// оставляем путь накопление: FireBullets сам считает остаток как дальность минус путь.
		data->m_flTraceLength += ( ExitTrace.endpos - data->m_EnterTrace.endpos ).Length( );

		// penetration was successful
		// setup new start end parameters for successive trace
		data->m_vecStart = ExitTrace.endpos;
		--data->m_iPenetrationCount;
		return false;
	}
	else {
		// since some railings in de_inferno are CONTENTS_GRATE but CHAR_TEX_CONCRETE, we'll trust the
		// CONTENTS_GRATE and use a high damage modifier.
		if( bContentsGrate || bNoDrawSurf ) {
			// If we're a concrete grate (TOOLS/TOOLSINVISIBLE texture) allow more penetrating power.
			flPenetrationModifier = 1.0f;
			flDamageModifier = 0.99f;
		}
		else {
			if( flExitPenetrationModifier < flPenetrationModifier ) {
				flPenetrationModifier = flExitPenetrationModifier;
			}
			if( flExitDamageModifier < flDamageModifier ) {
				flDamageModifier = flExitDamageModifier;
			}
		}

		// if enter & exit point is wood or metal we assume this is 
		// a hollow crate or barrel and give a penetration bonus
		if( iEnterMaterial == iExitMaterial ) {
			if( iExitMaterial == CHAR_TEX_WOOD ||
				iExitMaterial == CHAR_TEX_METAL ) {
				flPenetrationModifier *= 2;
			}
		}

		float flTraceDistance = ( ExitTrace.endpos - data->m_EnterTrace.endpos ).Length( );

		// check if bullet has enough power to penetrate this distance for this material
		if( flTraceDistance > ( data->m_WeaponData->m_flPenetration * flPenetrationModifier ) )
			return true; // bullet hasn't enough power to penetrate this distance

		// reduce damage power each time we hit something other than a grate
		data->m_flCurrentDamage *= flDamageModifier;

		// у Valve деления дальности пополам нет (см. выше), только накопление пути.
		data->m_flTraceLength += ( ExitTrace.endpos - data->m_EnterTrace.endpos ).Length( );

		// penetration was successful
		// setup new start end parameters for successive trace
		data->m_vecStart = ExitTrace.endpos;
		--data->m_iPenetrationCount;
		return false;
	}
}

float Autowall::FireBullets( FireBulletData *data ) {
	if( !g_pEngine->IsInGame( ) || !g_pEngine->IsConnected( ) )
		return -1.f;

	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return -1.f;

	if( pLocal->IsDead( ) )
		return -1.f;

	constexpr float rayExtension = 40.f;

	if( !data )
		return -1.f;

	//This gets set in FX_Firebullets to 4 as a pass-through value.
	//CS:GO has a maximum of 4 surfaces a bullet can pass-through before it 100% stops.
	//Excerpt from Valve: https://steamcommunity.com/sharedfiles/filedetails/?id=275573090
	//"The total number of surfaces any bullet can penetrate in a single flight is capped at 4." -CS:GO Official

	if( !data->m_Weapon ) {
		data->m_Weapon = reinterpret_cast< C_WeaponCSBaseGun * >( data->m_Player->m_hActiveWeapon( ).Get( ) );
	}

	if( data->m_Weapon && !data->m_WeaponData ) {
		data->m_WeaponData = data->m_Weapon->GetCSWeaponData( ).Xor( );
	}

	if( !data->m_Weapon || !data->m_WeaponData )
		return -1.f;

	data->m_flTraceLength = 0.f;
	data->m_flCurrentDamage = static_cast< float >( data->m_WeaponData->m_iWeaponDamage );

	data->m_flMaxLength = data->m_WeaponData->m_flWeaponRange;

	// m_bInAwall is global and affects every trace in the frame, so it must be cleared
	// on every exit path. a scoped guard does that even if an early return is added later.
	struct InAwallGuard {
		InAwallGuard( void *pTargetCollideable ) {
			g_Vars.globals.m_bInAwall = true;
			g_Vars.globals.m_pAwallTargetCollideable = pTargetCollideable;
		}
		~InAwallGuard( ) {
			g_Vars.globals.m_bInAwall = false;
			g_Vars.globals.m_pAwallTargetCollideable = nullptr;
		}
	} inAwallGuard( data->m_TargetPlayer ? ( void * )data->m_TargetPlayer->GetCollideable( ) : nullptr );

	//++g_AUTOWALL_CALLS;

	// we got damage and the power of 4 (headshot) is still in our min dmg bounds?
	float flScaledDamage = 0.f;

	IClientEntity *lastPlayerHit = NULL;
	while( data->m_flCurrentDamage > 0.f && ( data->m_flCurrentDamage * 4.f ) >= data->m_nMinimumDamage && ( flScaledDamage == 0.f || flScaledDamage >= data->m_nMinimumDamage ) ) {
		if( !data )
			break;

		if( !pLocal || pLocal->IsDead( ) )
			break;

		if( !data->m_Weapon || !data->m_WeaponData || !data->m_Player )
			break;

		// remaining bullet range, measured from the FULL weapon range each iteration.
		// mutating m_flMaxLength in place while m_flTraceLength kept accumulating meant
		// the first segment got subtracted again on every later segment, so from the
		// third penetration on the bullet ran out of range earlier than on the server.
		// CCSPlayer::FireBullet: vecEnd = start + dir * (range - flCurrentDistance).
		const float flRemainingRange = data->m_flMaxLength - data->m_flTraceLength;
		if( flRemainingRange <= 0.f )
			break;

		// create end point of bullet
		Vector vecEnd = data->m_vecStart + data->m_vecDirection * flRemainingRange;

		uint32_t filter_1[ 5 ] = { *reinterpret_cast< uint32_t * > ( Engine::Displacement.Function.m_TraceFilterSkipTwoEntities ), 0, 0, 0 };
		filter_1[ 1 ] = reinterpret_cast< uint32_t >( lastPlayerHit );
		filter_1[ 4 ] = uint32_t( data->m_Player );

		TraceLine( data->m_vecStart, vecEnd, CS_MASK_SHOOT_HITBOX, reinterpret_cast< ITraceFilter * >( filter_1 ), &data->m_EnterTrace );

		uint32_t filter_[ 5 ] = { *reinterpret_cast< uint32_t * > ( Engine::Displacement.Function.m_TraceFilterSkipTwoEntities ), 0, 0, 0 };
		filter_[ 1 ] = reinterpret_cast< uint32_t >( data->m_EnterTrace.hit_entity );
		filter_[ 4 ] = uint32_t( data->m_Player );

		// create extended end point
		Vector vecEndExtended = vecEnd + data->m_vecDirection * rayExtension;

		// Check for player hitboxes extending outside their collision bounds
		if( data->m_TargetPlayer && !data->m_TargetPlayer->IsDead( ) ) {
			// clip trace to one player
			ClipTraceToPlayer( data->m_vecStart, vecEndExtended, CS_MASK_SHOOT_HITBOX, reinterpret_cast< ITraceFilter * >( filter_ ), &data->m_EnterTrace, data->m_TargetPlayer );
		}
		else {
			ClipTraceToPlayers( data->m_vecStart, vecEndExtended, CS_MASK_SHOOT_HITBOX, reinterpret_cast< ITraceFilter * >( filter_ ), &data->m_EnterTrace );
		}

		if( data->m_EnterTrace.fraction == 1.f )
			break;  // we didn't hit anything, stop tracing shoot

		// calculate the damage based on the distance the bullet traveled.
		// flCurrentDistance += trace.fraction * (range - flCurrentDistance)
		data->m_flTraceLength += data->m_EnterTrace.fraction * flRemainingRange;

		// let's make our damage drops off the further away the bullet is.
		data->m_flCurrentDamage *= powf( data->m_WeaponData->m_flRangeModifier, data->m_flTraceLength * 0.002f );

		C_CSPlayer *pHittedPlayer = ToCSPlayer( ( C_BasePlayer * )data->m_EnterTrace.hit_entity );

		const int nHitGroup = data->m_Weapon->m_iItemDefinitionIndex( ) == WEAPON_ZEUS ? Hitgroup_Generic : data->m_EnterTrace.hitgroup;
		const bool bHitgroupIsValid = ( nHitGroup >= Hitgroup_Generic && nHitGroup <= Hitgroup_Neck ) || nHitGroup == Hitgroup_Gear;
		const bool bTargetIsValid = !data->m_TargetPlayer || ( pHittedPlayer != nullptr && pHittedPlayer->m_entIndex == data->m_TargetPlayer->m_entIndex );
		if( pHittedPlayer != nullptr ) {
			lastPlayerHit = pHittedPlayer->m_entIndex <= g_pGlobalVars->maxClients && pHittedPlayer->m_entIndex > 0 ? data->m_EnterTrace.hit_entity : nullptr;

			if( bTargetIsValid && bHitgroupIsValid && pHittedPlayer->m_entIndex <= g_pGlobalVars->maxClients && pHittedPlayer->m_entIndex > 0 ) {
				data->m_vecPenetratedPositions.push_back( data->m_EnterTrace.endpos );

				data->m_flCurrentDamage = ScaleDamage( pHittedPlayer, data->m_flCurrentDamage, data->m_WeaponData->m_flArmorRatio, data->m_iHitgroup == -1 ? nHitGroup : data->m_iHitgroup, data->m_Player );
				data->m_iHitgroup = nHitGroup;

				// InAwallGuard clears the global flag on the way out
				return data->m_flCurrentDamage;
			}
		}

		bool bCanPenetrate = data->m_bPenetration;
		if( !data->m_bPenetration )
			bCanPenetrate = data->m_EnterTrace.contents & CONTENTS_WINDOW;

		if( !bCanPenetrate )
			break;


		data->m_EnterSurfaceData = g_pPhysicsSurfaceProps->GetSurfaceData( data->m_EnterTrace.surface.surfaceProps );

		if( !data->m_EnterSurfaceData )
			break;

		// check if we reach penetration distance, no more penetrations after that
		// or if our modifier is super low, just stop the bullet
		if( ( data->m_flTraceLength > 3000.f && data->m_WeaponData->m_flPenetration > 0.f ) ||
			data->m_EnterSurfaceData->game.flPenetrationModifier < 0.1f ) {
			data->m_iPenetrationCount = 0;
			break;
		}

		bool bIsBulletStopped = HandleBulletPenetration( data );
		if( bIsBulletStopped )
			break;

		data->m_vecPenetratedPositions.push_back( data->m_EnterTrace.endpos );

		flScaledDamage = ScaleDamage( data->m_TargetPlayer, data->m_flCurrentDamage, data->m_WeaponData->m_flArmorRatio, data->m_iHitgroup, data->m_Player );
	}

	data->m_flCurrentDamage = -1.f;
	return -1.f;
}

int Autowall::ClipRayToHitbox( const Ray_t &ray, mstudiobbox_t *pBox, matrix3x4_t &matrix, CGameTrace &tr ) {
	tr.fraction = 1.f;
	tr.startsolid = false;

	return reinterpret_cast< int( __fastcall * )( const Ray_t &, mstudiobbox_t *, matrix3x4_t &, CGameTrace & ) >( Engine::Displacement.Function.m_ClipRayToHitbox )( ray, pBox, matrix, tr );
}

float Autowall::GetPenetrationDamage( C_CSPlayer *pLocal, C_WeaponCSBaseGun *pWeapon ) {
	if( !pLocal || !pWeapon )
		return -1.0f;

	auto weaponInfo = pWeapon->GetCSWeaponData( );
	if( !weaponInfo.IsValid( ) )
		return -1.0f;

	Autowall::FireBulletData data;

	data.m_iPenetrationCount = 4;
	data.m_Player = pLocal;
	data.m_TargetPlayer = nullptr;

	QAngle view_angles;
	g_pEngine->GetViewAngles( view_angles );
	data.m_vecStart = pLocal->GetEyePosition( );
	data.m_vecDirection = view_angles.ToVectors( );
	data.m_flMaxLength = data.m_vecDirection.Normalize( );
	data.m_WeaponData = weaponInfo.Xor( );
	data.m_flTraceLength = 0.0f;

	data.m_flCurrentDamage = static_cast< float >( weaponInfo->m_iWeaponDamage );

	CTraceFilter filter;
	filter.pSkip = pLocal;

	Vector end = data.m_vecStart + data.m_vecDirection * weaponInfo->m_flWeaponRange;

	Autowall::TraceLine( data.m_vecStart, end, MASK_SHOT_HULL | CONTENTS_HITBOX, &filter, &data.m_EnterTrace );
	Autowall::ClipTraceToPlayers( data.m_vecStart, end + data.m_vecDirection * 40.0f, MASK_SHOT_HULL | CONTENTS_HITBOX, &filter, &data.m_EnterTrace );
	if( data.m_EnterTrace.fraction == 1.f )
		return -1.0f;

	data.m_flTraceLength += data.m_flMaxLength * data.m_EnterTrace.fraction;
	if( data.m_flMaxLength != 0.0f && data.m_flTraceLength >= data.m_flMaxLength )
		return data.m_flCurrentDamage;

	data.m_flCurrentDamage *= powf( weaponInfo->m_flRangeModifier, data.m_flTraceLength * 0.002f );
	data.m_EnterSurfaceData = g_pPhysicsSurfaceProps->GetSurfaceData( data.m_EnterTrace.surface.surfaceProps );

	C_BasePlayer *hit_player = static_cast< C_BasePlayer * >( data.m_EnterTrace.hit_entity );
	bool can_do_damage = ( data.m_EnterTrace.hitgroup >= Hitgroup_Head && data.m_EnterTrace.hitgroup <= Hitgroup_Gear );
	bool hit_target = !data.m_TargetPlayer || hit_player == data.m_TargetPlayer;
	if( can_do_damage && hit_player && hit_player->EntIndex( ) <= g_pGlobalVars->maxClients && hit_player->EntIndex( ) > 0 && hit_target ) {
		if( pWeapon && pWeapon->m_iItemDefinitionIndex( ) == WEAPON_ZEUS )
			return ( data.m_flCurrentDamage * 0.9f );

		if( pWeapon->m_iItemDefinitionIndex( ) == WEAPON_ZEUS ) {
			data.m_EnterTrace.hitgroup = Hitgroup_Generic;
		}

		data.m_flCurrentDamage = Autowall::ScaleDamage( ( C_CSPlayer * )hit_player, data.m_flCurrentDamage, weaponInfo->m_flArmorRatio, data.m_EnterTrace.hitgroup );
		return data.m_flCurrentDamage;
	};

	if( data.m_flTraceLength > 3000.0f && weaponInfo->m_flPenetration > 0.f || 0.1f > data.m_EnterSurfaceData->game.flPenetrationModifier )
		return -1.0f;

	if( Autowall::HandleBulletPenetration( &data ) )
		return -1.0f;

	return data.m_flCurrentDamage;
};

__forceinline bool IsValidHitgroup( int index ) {
	if( ( index >= Hitgroup_Head && index <= Hitgroup_LeftLeg ) || index == Hitgroup_Gear )
		return true;

	return false;
}

bool penetration::run( PenetrationInput_t *in, PenetrationOutput_t *out ) {
	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return false;

	C_WeaponCSBaseGun *weapon;
	CCSWeaponInfo *weapon_info;
	Vector start;

	Autowall::FireBulletData data;

	// if we are tracing from our local player perspective.
	if( in->m_from->EntIndex( ) == pLocal->EntIndex( ) ) {
		weapon = ( C_WeaponCSBaseGun * )pLocal->m_hActiveWeapon( ).Get( );
		weapon_info = weapon->GetCSWeaponData( ).Xor( );
		start = in->m_start;
	}

	// not local player.
	else {
		weapon = ( C_WeaponCSBaseGun * )in->m_from->m_hActiveWeapon( ).Get( );
		if( !weapon || weapon == nullptr )
			return false;

		// get weapon info.
		weapon_info = weapon->GetCSWeaponData( ).Xor( );
		if( !weapon_info || weapon_info == nullptr )
			return false;

		// set trace start.
		start = in->m_from->GetEyePosition( );
	}

	// get direction to end point.
	Vector dir = ( in->m_pos - start ).Normalized( );

	data.m_Player = in->m_from;
	data.m_TargetPlayer = in->m_target;
	data.m_bPenetration = true;
	data.m_vecStart = start;
	data.m_vecDirection = dir;
	data.m_flMaxLength = weapon_info ? weapon_info->m_flWeaponRange : -1337.f;
	data.m_WeaponData = weapon_info;
	data.m_flCurrentDamage = weapon_info ? static_cast< float >( weapon_info->m_iWeaponDamage ) : -1338.f;

	auto flDamage = Autowall::FireBullets( &data );

	// set result data for when we hit a player.
	out->m_pen_count = data.m_iPenetrationCount;
	out->m_pen = data.m_iPenetrationCount != 4;
	out->m_hitgroup = data.m_iHitgroup;
	out->m_damage = flDamage;
	out->m_target = data.m_TargetPlayer;

	return flDamage >= in->m_damage;
}