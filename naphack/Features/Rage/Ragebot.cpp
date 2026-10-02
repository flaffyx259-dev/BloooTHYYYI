#include "../../pandora.hpp"

#include "Ragebot.hpp"
#include "../Rage/EnginePrediction.hpp"
#include "../../SDK/Classes/Player.hpp"
#include "../../SDK/Classes/weapon.hpp"
#include "../../SDK/Valve/CBaseHandle.hpp"
#include "../../Utils/InputSys.hpp"
#include "../../Renderer/Render.hpp"
#include "../../Utils/Threading/threading.h"
#include "ServerAnimations.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <thread>
#include "../../SDK/Displacement.hpp"
#include "Resolver.hpp"
#include "../Visuals/EventLogger.hpp"
#include "Resolver.hpp"
#include "ShotHandling.hpp"
#include "TickbaseShift.hpp"
#include "../visuals/visuals.hpp"
//#include "../Visuals/Chams.hpp"
#include "../Visuals/Models.hpp"

#include "../Scripting/Scripting.hpp"
#include "../Miscellaneous/Movement.hpp"

#include "../Rage/BoneSetup.hpp"

#include <sstream>

#include "../../Utils/Threading/shared_mutex.h"

#include "../Rage/AntiAim.hpp"
#include "../Miscellaneous/PlayerList.hpp"
#include "../Miscellaneous/Communication.hpp"
#include "../../Hooking/Hooked.hpp"
#include "FakeLag.hpp"

Aimbot g_Ragebot;

void Animations::AnimationEntry_t::SetupHitboxes( LagRecord_t *record, bool history ) {
	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	auto pWeapon = reinterpret_cast< C_WeaponCSBaseGun * >( pLocal->m_hActiveWeapon( ).Get( ) );
	if( !pWeapon ) {
		return;
	}

	// reset hitboxes.
	m_hitboxes.clear( );

	if( pWeapon->m_iItemDefinitionIndex( ) == WEAPON_ZEUS ) {
		// hitboxes for the zeus.
		m_hitboxes.push_back( { HITBOX_PELVIS, HitscanMode::PREFER } );
		return;
	}

	// prefer, always.
	if( g_Ragebot.m_AimbotInfo.m_pSettings->prefer_body_always )
		m_hitboxes.push_back( { HITBOX_PELVIS, HitscanMode::PREFER } );

	// prefer, lethal.
	if( g_Ragebot.m_AimbotInfo.m_pSettings->prefer_body_lethal )
		m_hitboxes.push_back( { HITBOX_PELVIS, HitscanMode::LETHAL } );

	// prefer, lethal x2.
	if( g_Ragebot.m_AimbotInfo.m_pSettings->prefer_body_lethal_x2 )
		m_hitboxes.push_back( { HITBOX_PELVIS, HitscanMode::LETHAL2 } );

	// prefer, fake.
	if( g_Ragebot.m_AimbotInfo.m_pSettings->prefer_body_fake && !record->m_bIsResolved )
		m_hitboxes.push_back( { HITBOX_PELVIS, HitscanMode::PREFER } );

	// prefer, in air.
	if( g_Ragebot.m_AimbotInfo.m_pSettings->prefer_body_air && !( record->m_fPredFlags & FL_ONGROUND ) )
		m_hitboxes.push_back( { HITBOX_CHEST, HitscanMode::PREFER } );

	// prefer, when doubletapping
	if( g_Ragebot.m_AimbotInfo.m_pSettings->prefer_body_exploit && ( g_TickbaseController.IsCharged( ) || g_TickbaseController.m_bTapShot ) )
		m_hitboxes.push_back( { HITBOX_PELVIS, HitscanMode::PREFER } );

	bool only{ false };
	bool only_air{ false };

	// we push in air first and don't check after that, because air forces chest (instead of pelvis), so we don't want to add anything besides that
	// otherwise it'll add both pelvis and chest (if one or more triggers)
	// only, in air.
	if( g_Ragebot.m_AimbotInfo.m_pSettings->force_body_air && !( record->m_fPredFlags & FL_ONGROUND ) ) {
		only_air = true;
		m_hitboxes.push_back( { HITBOX_CHEST, HitscanMode::PREFER } );
	}

	// air condition has been checked, return (need a different check then only, because it uses a different hitbox)
	if( only_air )
		return;

	// only, always.
	if( g_Ragebot.m_AimbotInfo.m_pSettings->force_body_always ) {
		only = true;
		m_hitboxes.push_back( { HITBOX_PELVIS, HitscanMode::PREFER } );
	}

	// only, health.
	if( g_Ragebot.m_AimbotInfo.m_pSettings->force_body_health && m_pEntity->m_iHealth( ) <= ( int )g_Ragebot.m_AimbotInfo.m_pSettings->force_body_health_min ) {
		only = true;
		m_hitboxes.push_back( { HITBOX_PELVIS, HitscanMode::PREFER } );
	}

	// only, fake.
	if( g_Ragebot.m_AimbotInfo.m_pSettings->force_body_fake && !record->m_bIsResolved ) {
		only = true;
		m_hitboxes.push_back( { HITBOX_PELVIS, HitscanMode::PREFER } );
	}

	// only, on peek.
	// trace this for all targets, unless our fps drops under tick rate - then only trace our current threat.
	const bool bLocalMoving = pLocal->m_vecVelocity( ).Length2D( ) > 0.1f || g_Movement.PressingMovementKeys( g_Ragebot.m_AimbotInfo.m_pCmd );
	const bool bLowFps = g_Ragebot.m_flFrameRateMultiplier > 0.2f;
	if( g_Ragebot.m_AimbotInfo.m_pSettings->force_body_peek && bLocalMoving && ( !( bLowFps && m_pEntity != g_AntiAim.GetBestPlayer( ) ) )) {
		// get target angle.
		Vector pLocalEyePos = g_Ragebot.m_AimbotInfo.m_vecEyePosition;
		Vector pTargetEyePos = record->m_vecEyePosition;
		QAngle angTargetAngle = Math::CalcAngle( pLocalEyePos, pTargetEyePos, false );

		// calculate right & left directions relative to the target angle.
		Vector vecLeftDir, vecRightDir;
		Math::AngleVectors( QAngle( 0, angTargetAngle.y + 90, 0 ), vecLeftDir );
		Math::AngleVectors( QAngle( 0, angTargetAngle.y - 90, 0 ), vecRightDir );

		// how far we want to trace, could have a slider for this but better to be forced.
		// set this bool to true to draw debug lines & damage.
		constexpr bool bDrawDebug = false;
		constexpr float flTraceDistance = 60;

		// trace from 3 positions.
		// our current eye pos, eye pos left & eye pos right.
		std::vector<Vector> positions( 3 );
		positions[ 0 ] = pLocalEyePos;
		positions[ 1 ] = { pLocalEyePos.x + vecLeftDir.x * flTraceDistance, pLocalEyePos.y + vecLeftDir.y * flTraceDistance, pLocalEyePos.z };
		positions[ 2 ] = { pLocalEyePos.x + vecRightDir.x * flTraceDistance, pLocalEyePos.y + vecRightDir.y * flTraceDistance, pLocalEyePos.z };

		// hitboxes we want to iterate over.
		std::map<int, Vector> vecHitboxPositions;

		if( g_Ragebot.m_AimbotInfo.m_pSettings->on_peek_chest ) {
			vecHitboxPositions[ HITBOX_LOWER_CHEST ] = record->m_vecLowerChestPosition;
			vecHitboxPositions[ HITBOX_CHEST ] = record->m_vecChestPosition;
		}

		if( g_Ragebot.m_AimbotInfo.m_pSettings->on_peek_pelvis )
			vecHitboxPositions[ HITBOX_PELVIS ] = record->m_vecPelvisPosition;

		if( g_Ragebot.m_AimbotInfo.m_pSettings->on_peek_stomach )
			vecHitboxPositions[ HITBOX_STOMACH ] = record->m_vecStomachPosition;

		// this will be overriden.
		int nHighestDamage = 0;

		// loop through our positions.
		for( int i = 0; i < positions.size( ); i++ ) {
			Vector vecTracePos = positions[ i ];

			// this variable will only be used for debugging, to see how much damage is done from each traced position.
			int nHighestItDamage = 0;

			// loop through how many hitboxes we want.
			for( const auto &hitbox : vecHitboxPositions ) {
				const Vector &vecEnemyHitboxPos = hitbox.second;

				// run simulated damage.
				Autowall::FireBulletData data{ };
				data.m_bPenetration = true;
				data.m_TargetPlayer = nullptr;
				data.m_Player = pLocal;
				data.m_vecStart = vecTracePos;

				data.m_vecDirection = vecEnemyHitboxPos - vecTracePos;
				data.m_vecDirection.Normalize( );

				Autowall::FireBullets( &data );

				int nDamage = static_cast< int >( data.m_flCurrentDamage );

				// new damage found is higher then our older highest damage, override it.
				if( nDamage > nHighestDamage )
					nHighestDamage = nDamage;

				// new damage is highest then our current iterated damage, override it.
				if( nDamage > nHighestItDamage )
					nHighestItDamage = nDamage;

				// draw debug.
				if( bDrawDebug ) {
					g_pDebugOverlay->AddTextOverlay( vecTracePos, g_pGlobalVars->interval_per_tick * 4.f, std::to_string( nHighestItDamage ).c_str( ) );
					g_pDebugOverlay->AddLineOverlay( pTargetEyePos, vecTracePos, 255, 255, 255, false, g_pGlobalVars->interval_per_tick * 4.f );
					g_pDebugOverlay->AddLineOverlay( vecTracePos, pLocalEyePos, 255, 255, 255, false, g_pGlobalVars->interval_per_tick * 4.f );
				}
			}
		}

		// highest damage is higher then enemy health, they are a lethal shot.
		if( nHighestDamage >= m_pEntity->m_iHealth( ) ) {
			only = true;

			if( g_Ragebot.m_AimbotInfo.m_pSettings->on_peek_chest ) {
				m_hitboxes.push_back( { HITBOX_LOWER_CHEST, HitscanMode::PREFER } );
				m_hitboxes.push_back( { HITBOX_CHEST, HitscanMode::PREFER } );
			}

			if( g_Ragebot.m_AimbotInfo.m_pSettings->on_peek_pelvis )
				m_hitboxes.push_back( { HITBOX_PELVIS, HitscanMode::PREFER } );

			if( g_Ragebot.m_AimbotInfo.m_pSettings->on_peek_stomach )
				m_hitboxes.push_back( { HITBOX_STOMACH, HitscanMode::PREFER } );
		}
	}

	// only, on key.
	if( g_Vars.rage.force_body_aim && g_Vars.rage.force_body_aim_key.enabled ) {
		only = true;
		m_hitboxes.push_back( { HITBOX_PELVIS, HitscanMode::PREFER } );
	}

	// only baim conditions have been met.
	// do not insert more hitboxes.
	if( only )
		return;

	// head.
	if( g_Ragebot.m_AimbotInfo.m_pSettings->hitbox_head ) {
		//-- removes like a bunch of hitpoints for some reason
		//if( record->m_bIsResolved )
		//	m_hitboxes.push_back( { HITBOX_HEAD, HitscanMode::PREFER } );
		//else
			m_hitboxes.push_back( { HITBOX_HEAD, HitscanMode::NORMAL } );
	}

	// upper chest.
	if( g_Ragebot.m_AimbotInfo.m_pSettings->hitbox_upper_chest ) {
		m_hitboxes.push_back( { HITBOX_UPPER_CHEST, HitscanMode::NORMAL } );
	}

	// chest.
	if( g_Ragebot.m_AimbotInfo.m_pSettings->hitbox_chest ) {
		m_hitboxes.push_back( { HITBOX_LOWER_CHEST, HitscanMode::NORMAL } );
		m_hitboxes.push_back( { HITBOX_CHEST, HitscanMode::NORMAL } );
	}

	// stomach.
	if( g_Ragebot.m_AimbotInfo.m_pSettings->hitbox_stomach ) {
		m_hitboxes.push_back( { HITBOX_PELVIS, HitscanMode::NORMAL } );
		m_hitboxes.push_back( { HITBOX_STOMACH, HitscanMode::NORMAL } );
	}

	// arms.
	if( g_Ragebot.m_AimbotInfo.m_pSettings->hitbox_arms ) {
		m_hitboxes.push_back( { HITBOX_LEFT_UPPER_ARM, HitscanMode::NORMAL } );
		m_hitboxes.push_back( { HITBOX_RIGHT_UPPER_ARM, HitscanMode::NORMAL } );
	}

	// legs.
	if( g_Ragebot.m_AimbotInfo.m_pSettings->hitbox_legs ) {
		m_hitboxes.push_back( { HITBOX_LEFT_THIGH, HitscanMode::NORMAL } );
		m_hitboxes.push_back( { HITBOX_RIGHT_THIGH, HitscanMode::NORMAL } );
		m_hitboxes.push_back( { HITBOX_LEFT_CALF, HitscanMode::NORMAL } );
		m_hitboxes.push_back( { HITBOX_RIGHT_CALF, HitscanMode::NORMAL } );
	}

	// feet.
	if( g_Ragebot.m_AimbotInfo.m_pSettings->hitbox_feet ) {
		m_hitboxes.push_back( { HITBOX_LEFT_FOOT, HitscanMode::NORMAL } );
		m_hitboxes.push_back( { HITBOX_RIGHT_FOOT, HitscanMode::NORMAL } );
	}
}

void Aimbot::init( ) {
	// clear old targets.
	m_targets.clear( );

	m_target = nullptr;
	m_aim = Vector{ };
	m_angle = Vector{ };
	m_damage = 0.f;
	m_record = {};
	m_stop = EAutoStopType::STOP_NONE;

	m_best_dist = std::numeric_limits< float >::max( );
	m_best_fov = 180.f + 1.f;
	m_best_damage = 0.f;
	m_best_hp = 100 + 1;
	m_best_lag = std::numeric_limits< float >::max( );
	m_best_height = std::numeric_limits< float >::max( );
}

Vector Aimbot::SetupEyePosition( QAngle ang ) {
	if( !m_AimbotInfo.m_pLocal )
		return { };

	ang.Clamp( );

	matrix3x4_t pBackupMatrix[ MAXSTUDIOBONES ];
	matrix3x4_t pAimbotMatrix[ MAXSTUDIOBONES ];
	std::memcpy( pBackupMatrix, m_AimbotInfo.m_pLocal->m_CachedBoneData( ).Base( ), m_AimbotInfo.m_pLocal->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );
	std::memcpy( pAimbotMatrix, g_ServerAnimations.m_uServerAnimations.m_pMatrix, sizeof g_ServerAnimations.m_uServerAnimations.m_pMatrix );

	const auto vecOrigin = m_AimbotInfo.m_pLocal->GetAbsOrigin( );
	for( size_t i = 0; i < MAXSTUDIOBONES; ++i ) for( auto n = 0; n <= 2; ++n )
		pAimbotMatrix[ i ].m[ n ][ 3 ] += vecOrigin[ n ];

	std::memcpy( m_AimbotInfo.m_pLocal->m_CachedBoneData( ).Base( ), pAimbotMatrix, m_AimbotInfo.m_pLocal->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );

	m_AimbotInfo.m_pLocal->ForceBoneCache( );

	const auto ret = m_AimbotInfo.m_pLocal->GetEyePosition( );

	std::memcpy( m_AimbotInfo.m_pLocal->m_CachedBoneData( ).Base( ), pBackupMatrix, m_AimbotInfo.m_pLocal->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );

	return ret;
}

CVariables::RAGE *Aimbot::GetRageSettings( ) {
	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return nullptr;

	if( pLocal->IsDead( ) ) {
		m_AimbotInfo.m_pWeapon = nullptr;
		m_AimbotInfo.m_pWeaponInfo = nullptr;
	}

	if( !m_AimbotInfo.m_pWeapon || !m_AimbotInfo.m_pWeaponInfo )
		return nullptr;

	CVariables::RAGE *current = nullptr;

	auto id = m_AimbotInfo.m_pWeapon->m_iItemDefinitionIndex( );

	// for now, I might do make aimbot also run on zeus
	if( id == WEAPON_ZEUS ) {
		m_AimbotInfo.m_bIsZeus = true;

		g_Vars.rage_zeus.auto_scope = false;
		g_Vars.rage_zeus.auto_stop = 0;
		g_Vars.rage_zeus.hitbox_neck = true;
		g_Vars.rage_zeus.hitbox_stomach = true;
		g_Vars.rage_zeus.hitbox_pelvis = true;
		g_Vars.rage_zeus.hitchance = 50.f;
		g_Vars.rage_zeus.minimal_damage = 105;
		g_Vars.rage_zeus.ensure_accuracy_strict = false;

		return &g_Vars.rage_zeus;
	}

	bool bSwappedWeapons = false;
	if( m_AimbotInfo.m_nLastWeaponType != m_AimbotInfo.m_pWeaponInfo->m_iWeaponType || m_AimbotInfo.m_nLastWeaponIndex != m_AimbotInfo.m_pWeapon->m_iItemDefinitionIndex( ) ) {
		bSwappedWeapons = true;
		m_AimbotInfo.m_nLastWeaponType = m_AimbotInfo.m_pWeaponInfo->m_iWeaponType;
		m_AimbotInfo.m_nLastWeaponIndex = m_AimbotInfo.m_pWeapon->m_iItemDefinitionIndex( );
	}

	switch( m_AimbotInfo.m_pWeaponInfo->m_iWeaponType ) {
		case WEAPONTYPE_PISTOL:
			if( id == WEAPON_REVOLVER || id == WEAPON_DEAGLE ) {
				if( id == WEAPON_REVOLVER ) {
					current = &g_Vars.rage_revolver;
				}
				else {
					current = &g_Vars.rage_deagle;
				}

				if( ( !g_Vars.globals.menuOpen || bSwappedWeapons ) && current->override_default_config ) {
					if( id == WEAPON_REVOLVER ) {
						g_Vars.globals.m_iCurrentRageGroup = 4;
					}
					else {
						g_Vars.globals.m_iCurrentRageGroup = 5;
					}
				}
			}
			else {
				current = &g_Vars.rage_pistols;
				if( ( !g_Vars.globals.menuOpen || bSwappedWeapons ) && current->override_default_config )
					g_Vars.globals.m_iCurrentRageGroup = 6;
			}
			break;
		case WEAPONTYPE_SNIPER_RIFLE:
			if( id == WEAPON_G3SG1 || id == WEAPON_SCAR20 ) {
				current = &g_Vars.rage_autosnipers;
				if( ( !g_Vars.globals.menuOpen || bSwappedWeapons ) && current->override_default_config )
					g_Vars.globals.m_iCurrentRageGroup = 1;
			}
			else {
				current = ( id == WEAPON_AWP ) ? &g_Vars.rage_awp : &g_Vars.rage_scout;

				if( ( !g_Vars.globals.menuOpen || bSwappedWeapons ) && current->override_default_config )
					g_Vars.globals.m_iCurrentRageGroup = ( id == WEAPON_AWP ) ? 3 : 2;
			}
			break;
		default:
			current = &g_Vars.rage_default;
			if( ( !g_Vars.globals.menuOpen || bSwappedWeapons ) )
				g_Vars.globals.m_iCurrentRageGroup = 0;
			break;
	}

	if( !current )
		return nullptr;

	if( !current->override_default_config ) {
		current = &g_Vars.rage_default;

		if( ( !g_Vars.globals.menuOpen || bSwappedWeapons ) )
			g_Vars.globals.m_iCurrentRageGroup = 0;
	}

	return current;
}

int Aimbot::GetMinimalDamage( C_CSPlayer *pEntity ) {
	if( !pEntity )
		return 1;

	auto GetHPDamage = [&] ( int nDamage ) {
		const int nPlayerHP = pEntity->m_iHealth( );
		return nDamage > 100 ? ( nPlayerHP + ( nDamage - 100 ) ) : nDamage;
	};

	int nConfigDamage = m_AimbotInfo.m_pSettings->minimal_damage;
	int returnDamage = GetHPDamage( m_AimbotInfo.m_pSettings->minimal_damage );

	if( g_Vars.rage.min_damage_override_key.enabled ) {
		nConfigDamage = m_AimbotInfo.m_pSettings->minimal_damage_override;
		returnDamage = GetHPDamage( m_AimbotInfo.m_pSettings->minimal_damage_override );
	}

	const float flMaxBodyDamage = Autowall::ScaleDamage( pEntity, m_AimbotInfo.m_pWeaponInfo->m_iWeaponDamage, m_AimbotInfo.m_pWeaponInfo->m_flArmorRatio, Hitgroup_Stomach ) - 1.f;

	if( m_AimbotInfo.m_pSettings->scale_damage_on_hp && nConfigDamage <= 100 )
		returnDamage = std::min( returnDamage, pEntity->m_iHealth( ) );

	if( returnDamage > flMaxBodyDamage && pEntity->m_iHealth( ) < flMaxBodyDamage * 0.5f ) {
		returnDamage = flMaxBodyDamage * 0.5f;
	}

	return returnDamage;
}

void Aimbot::CockRevolver( ) {
	if( m_AimbotInfo.m_pWeapon->m_iItemDefinitionIndex( ) != WEAPON_REVOLVER || g_TickbaseController.m_bShifting )
		return;

	if( !( m_AimbotInfo.m_pCmd->buttons & IN_RELOAD ) && m_AimbotInfo.m_pWeapon->m_iClip1( ) ) {
		float curtime = m_AimbotInfo.m_pLocal->m_nTickBase( ) * g_pGlobalVars->interval_per_tick;
		if( g_Vars.rage.double_tap_bind.enabled && g_TickbaseController.m_bBreakingLC && g_TickbaseController.m_bBreakLC && g_TickbaseController.CanShift( false, true ) ) {
			curtime -= TICKS_TO_TIME( g_TickbaseController.m_nLCShiftAmount );
		}

		m_AimbotInfo.m_pCmd->buttons &= ~IN_ATTACK2;

		if( m_AimbotInfo.m_pLocal->CanShoot( true ) ) {
			if( m_AimbotInfo.m_flCockTime <= curtime ) {
				if( m_AimbotInfo.m_pWeapon->m_flNextSecondaryAttack( ) <= curtime ) {
					m_AimbotInfo.m_flCockTime = curtime + 0.234375f;
				}
				else {
					m_AimbotInfo.m_pCmd->buttons |= IN_ATTACK2;
				}
			}
			else {
				m_AimbotInfo.m_pCmd->buttons |= IN_ATTACK;
			}
		}
		else {
			m_AimbotInfo.m_flCockTime = curtime + 0.234375f;
			m_AimbotInfo.m_pCmd->buttons &= ~IN_ATTACK;
		}
	}
}

std::deque<std::pair<C_CSPlayer *, int>> Aimbot::FindTargets( ) {
	std::deque<std::pair<C_CSPlayer *, int>> vecPlayers{ };

	// scan 8 players per tick normally
	int nScanLimit = g_Vars.rage.target_limit;

	// fps dipped, let's scan a bit less players
	if( g_Ragebot.m_flLimitTargets >= g_pGlobalVars->realtime ) {
		nScanLimit = 2;
	}

	const auto pCrosshairPlayer = g_AntiAim.GetBestPlayer( false );

	if( !pCrosshairPlayer )
		return vecPlayers;

	if( !pCrosshairPlayer->IsDead( ) && !pCrosshairPlayer->m_bGunGameImmunity( ) ) {

		if( !g_PlayerList.GetSettings( pCrosshairPlayer->GetSteamID( ) ).m_bAddToWhitelist ) {
			// already push this one.
			vecPlayers.push_back( { pCrosshairPlayer, pCrosshairPlayer->EntIndex( ) } );
		}
		else {
			if( pCrosshairPlayer->EntIndex( ) == m_AimbotInfo.m_iLastTarget )
				m_AimbotInfo.m_iLastTarget = 0;
		}
	}
	else {
		if( pCrosshairPlayer->EntIndex( ) == m_AimbotInfo.m_iLastTarget )
			m_AimbotInfo.m_iLastTarget = 0;
	}

	//C_CSPlayer *pLastTarget = nullptr;
	for( int i = 1; i <= g_pGlobalVars->maxClients; ++i ) {
		const auto player = C_CSPlayer::GetPlayerByIndex( i );
		if( !player )
			continue;

		if( player == pCrosshairPlayer )
			continue;

		if( player->IsDead( ) || player->m_bGunGameImmunity( ) || player->IsDormant( ) || g_PlayerList.GetSettings( player->GetSteamID( ) ).m_bAddToWhitelist ) {
			if( i == m_AimbotInfo.m_iLastTarget )
				m_AimbotInfo.m_iLastTarget = 0;

			continue;
		}

		if( player->IsTeammate( m_AimbotInfo.m_pLocal ) ) {
			if( i == m_AimbotInfo.m_iLastTarget )
				m_AimbotInfo.m_iLastTarget = 0;

			continue;
		}

		const auto it = std::find_if( m_vecLastSkippedPlayers.begin( ), m_vecLastSkippedPlayers.end( ), [&] ( const auto &pl ) -> bool {
			return pl.first && pl.first == player;
		} );

		if( it == m_vecLastSkippedPlayers.end( ) ) {
			vecPlayers.push_back( { player, i } );
		}
	}

	// sort them (get the best targets nearest to crosshair).
	std::sort( vecPlayers.begin( ), vecPlayers.end( ), [&] ( const std::pair<C_CSPlayer *, int> &a, const std::pair<C_CSPlayer *, int> &b ) -> bool {
		if( !a.first || !b.first )
			return false;

		return a.first->m_vecOrigin( ).Distance( pCrosshairPlayer->m_vecOrigin( ) ) < b.first->m_vecOrigin( ).Distance( pCrosshairPlayer->m_vecOrigin( ) );
	} );

	if( !m_AimbotInfo.m_bInPredicted ) {
		int nPushed = 0;
		for( int i = 0; i >= 0 && i < m_vecLastSkippedPlayers.size( ) && nPushed < g_Vars.rage.target_limit; ++i ) {
			auto entry = m_vecLastSkippedPlayers[ i ];
			if( !entry.first ) {
				m_vecLastSkippedPlayers.erase( m_vecLastSkippedPlayers.begin( ) + i );
				--i;
				continue;
			}

			if( entry.first->m_entIndex <= 0 || entry.first->m_entIndex > 64 ) {
				m_vecLastSkippedPlayers.erase( m_vecLastSkippedPlayers.begin( ) + i );
				--i;
				continue;
			}

			// for after fullupdate etc. [...]
			entry.first = reinterpret_cast< C_CSPlayer * >( g_pEntityList->GetClientEntity( entry.first->m_entIndex ) );

			if( !entry.first ) {
				m_vecLastSkippedPlayers.erase( m_vecLastSkippedPlayers.begin( ) + i );
				--i;
				continue;
			}

			if( entry.first->IsDead( ) || entry.first->m_bGunGameImmunity( ) || entry.first->IsDormant( ) ) {
				m_vecLastSkippedPlayers.erase( m_vecLastSkippedPlayers.begin( ) + i );
				--i;
				continue;
			}

			if( entry.first->IsTeammate( m_AimbotInfo.m_pLocal ) ) {
				m_vecLastSkippedPlayers.erase( m_vecLastSkippedPlayers.begin( ) + i );
				--i;
				continue;
			}

			// add it in the front.
			vecPlayers.push_front( { entry.first, entry.second } );

			// remember the amount we brought over.
			++nPushed;

			// delete it.
			m_vecLastSkippedPlayers.erase( m_vecLastSkippedPlayers.begin( ) + i );

			// fix loop.
			--i;
		}

		for( int i = g_Vars.rage.target_limit; i < vecPlayers.size( ); ++i ) {
			const auto entry = &vecPlayers[ i ];
			if( !entry )
				continue;

			const auto it = std::find_if( m_vecLastSkippedPlayers.begin( ), m_vecLastSkippedPlayers.end( ), [&] ( const auto &pl ) -> bool {
				return pl.first == entry->first;
			} );

			// not in it? we add it so we make sure to handle it next time!
			if( it == m_vecLastSkippedPlayers.end( ) ) {
				m_vecLastSkippedPlayers.push_back( { entry->first, entry->second } );
			}
		}
	}

	//if( pLastTarget != nullptr )
	//	vecPlayers.push_front( { pLastTarget, pLastTarget->m_entIndex } );

	if( vecPlayers.size( ) >= g_Vars.rage.target_limit ) {
		// resize to target_limit (target_limit targets only)
		vecPlayers.resize( g_Vars.rage.target_limit );
	}

	return vecPlayers;
}

void Aimbot::think( bool *bSendPacket, CUserCmd *pCmd ) {
	if( !pCmd || !bSendPacket )
		return;

	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	if( pLocal->IsDead( ) )
		return;

	// not the right place to do this but oh well
	if( g_Vars.menu.whitelist ) {
		if( !( pCmd->tick_count % 100 ) )
			g_Communication.SendUpdatePacket( );
	}

	// setup aimbot info variables
	m_AimbotInfo.m_pLocal = pLocal;
	m_AimbotInfo.m_pCmd = pCmd;
	m_AimbotInfo.m_pSendPacket = bSendPacket;

	QAngle angHead = { -3.5f, 0.f, 0.f };

	// apply recoil angle..
	angHead -= m_AimbotInfo.m_pLocal->m_aimPunchAngle( ) * g_Vars.weapon_recoil_scale->GetFloat( );

	// setup eyepos
	m_AimbotInfo.m_vecEyePosition = SetupEyePosition( angHead );

	if( !g_Vars.rage.enabled || !g_Vars.rage.key.enabled ) {
		return;
	}

	// do all startup routines.
	init( );

	auto pWeapon = reinterpret_cast< C_WeaponCSBaseGun * >( pLocal->m_hActiveWeapon( ).Get( ) );
	if( !pWeapon ) {
		return;
	}

	auto pWeaponInfo = pWeapon->GetCSWeaponData( );
	if( !pWeaponInfo.IsValid( ) ) {
		return;
	}

	m_AimbotInfo.m_pWeaponInfo = pWeaponInfo.Xor( );
	m_AimbotInfo.m_pWeapon = pWeapon;

	if( pWeapon->m_iItemDefinitionIndex( ) != WEAPON_ZEUS ) {
		if( pWeaponInfo->m_iWeaponType == WEAPONTYPE_KNIFE || pWeaponInfo->m_iWeaponType == WEAPONTYPE_GRENADE || pWeaponInfo->m_iWeaponType == WEAPONTYPE_C4 ) {
			return;
		}
	}

	// cache random values cuz valve random system cause performance issues
	if( !m_AimbotInfo.m_bInitialisedRandomSpread ) {
		for( auto i = 0; i <= 255; i++ ) {
			RandomSeed( i + 1 );

			m_AimbotInfo.m_uRandomSpread[ i ].flRand1 = RandomFloat( 0.0f, 1.0f );
			m_AimbotInfo.m_uRandomSpread[ i ].flRandPi1 = RandomFloat( 0.0f, DirectX::XM_2PI );
			m_AimbotInfo.m_uRandomSpread[ i ].flRand2 = RandomFloat( 0.0f, 1.0f );
			m_AimbotInfo.m_uRandomSpread[ i ].flRandPi2 = RandomFloat( 0.0f, DirectX::XM_2PI );
		}

		m_AimbotInfo.m_bInitialisedRandomSpread = true;
	}

	if( !m_AimbotInfo.m_bInitialisedRandomSpread ) {
		return;
	}

	m_AimbotInfo.m_bIsZeus = false;

	m_AimbotInfo.m_pSettings = GetRageSettings( );
	if( !m_AimbotInfo.m_pSettings )
		return;

	CockRevolver( );

	// no grenades or bomb.
	if( m_AimbotInfo.m_pWeaponInfo->m_iWeaponType == WEAPONTYPE_GRENADE || m_AimbotInfo.m_pWeaponInfo->m_iWeaponType == WEAPONTYPE_C4 )
		return;

	const bool bCanShoot = m_AimbotInfo.m_pLocal->CanShoot( );
	if( !bCanShoot ) {
		if( pWeapon->m_iItemDefinitionIndex( ) != WEAPON_REVOLVER )
			pCmd->buttons &= ~IN_ATTACK;
	}

	if( g_Vars.rage.exploit && g_Vars.rage.double_tap_bind.enabled && g_Vars.misc.autopeek && g_Vars.misc.autopeek_bind.enabled && g_TickbaseController.m_bTapShot && g_Vars.globals.m_bShotAutopeek ) {
		return;
	}

	if( g_Vars.rage.anti_aim_active && g_Vars.rage.anti_aim_fake_body && ( pLocal->m_fFlags( ) & FL_ONGROUND ) && pLocal->m_PlayerAnimState( )->m_flVelocityLengthXY < 0.1f ) {
		if( ( ( TICKS_TO_TIME( pLocal->m_nTickBase( ) ) + ( g_pGlobalVars->interval_per_tick * 2 ) ) > g_ServerAnimations.m_uServerAnimations.m_flLowerBodyRealignTimer ) ) {
			return;
		}
	}

	// GHETTO! :D
	// when switching weapon
	// break lc will fuck us up when
	// trying to aimbot in the first possible tick
	bool bNo = false;
	if( g_TickbaseController.IsCharged( ) ) {
		const bool bExploit = g_Vars.rage.exploit && ( g_Vars.rage.double_tap_bind.enabled || g_Vars.rage.hide_shots_bind.enabled );
		if( !g_TickbaseController.m_bTapShot && bExploit && ( ( g_TickbaseController.m_bBreakingLC && !g_TickbaseController.CanShift( true, true ) ) || g_TickbaseController.m_bDelayAimbot/*|| ( g_TickbaseController.m_bBreakLC && !g_TickbaseController.m_bBreakingLC )*/ ) ) {
			g_Visuals.AddDebugMessage( std::string( XorStr( "EXPLOIT_DELAY" ) ).append( XorStr( " -> L" ) + std::to_string( __LINE__ ) ) );
			bNo = true;
		}
	}

	// we have a normal weapon or a non cocking revolver
	// choke if its the processing tick.
	if( bCanShoot && !g_pClientState->m_nChokedCommands( ) && pWeapon->m_iItemDefinitionIndex( ) != WEAPON_REVOLVER ) {
		*bSendPacket = false;
		pCmd->buttons &= ~IN_ATTACK;

		return;
	}

	const bool bSkipForRevolver = m_AimbotInfo.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_REVOLVER && TICKS_TO_TIME( m_AimbotInfo.m_pLocal->m_nTickBase( ) ) >= m_AimbotInfo.m_pLocal->m_flNextAttack( ) && m_AimbotInfo.m_pWeapon->m_iClip1( ) > 0;
	const bool noAimbot = ( bNo ) || ( !bCanShoot && !bSkipForRevolver ) || g_Vars.globals.m_bShotWhileChoking;

	// we are not using between shots, and are not using fake walk autostop, we can return if we can't fire.
	if( !m_AimbotInfo.m_pSettings->auto_stop_between_shots && m_AimbotInfo.m_pSettings->auto_stop != 2 ) {
		// no point in aimbotting if we cannot fire this tick.
		if( !bCanShoot )
			return;

		if( noAimbot && ( ( g_TickbaseController.m_nTicksAfterUncharge >= 14 && !g_TickbaseController.IsCharged( ) ) || g_TickbaseController.m_nTicksAfterUncharge <= 3 ) )
			return;
	}

	// выстрел чокается, пакет уходит следующей командой ( RAX HandleShotChoke ),
	// поэтому прибытие = как будто выстрел уйдёт этим пакетом + 1 тик.
	if( g_Vars.globals.m_bShotWhileHiding )
		g_Animations.UpdateArrivalTick( false );
	else {
		g_Animations.UpdateArrivalTick( true );
		g_Animations.m_nArrivalTick += 1; // выстрел уходит следующей командой
	}

	auto vecPlayers = FindTargets( );
	if( !vecPlayers.empty( ) ) {
		// we don't want to return here, because we still want to prevent on-shot whilst manual firing if there are no targets (ran on apply)
		// return;

		if( m_AimbotInfo.m_iLastTarget > 0 && m_AimbotInfo.m_iLastTarget <= 64 ) {
			const auto player = C_CSPlayer::GetPlayerByIndex( m_AimbotInfo.m_iLastTarget );
			if( player ) {
				vecPlayers.push_back( { player, m_AimbotInfo.m_iLastTarget } );
			}
		}

		// setup bones for all valid targets.
		for( auto &[player, i] : vecPlayers ) {
			if( !player )

				if( !IsValidTarget( player ) ) {
					g_Visuals.vecAimpoints[ i ].clear( );
					g_Visuals.vecAimpointsSane[ i ].clear( );
					continue;
				}

			if( g_PlayerList.GetSettings( player->GetSteamID( ) ).m_bAddToWhitelist ) {
				g_Visuals.vecAimpoints[ i ].clear( );
				g_Visuals.vecAimpointsSane[ i ].clear( );
				continue;
			}

			if( g_Ragebot.m_arrNapUsers.at( player->EntIndex( ) ).first && !g_Vars.menu.whitelist_disable_key.enabled ) {
				g_Visuals.vecAimpoints[ i ].clear( );
				g_Visuals.vecAimpointsSane[ i ].clear( );
				continue;
			}

			Animations::AnimationEntry_t *data = g_Animations.GetAnimationEntry( i );
			if( !data ) {
				g_Visuals.vecAimpoints[ i ].clear( );
				g_Visuals.vecAimpointsSane[ i ].clear( );
				continue;
			}

			// store player as potential target this tick.
			m_targets.emplace_back( data );
		}
	}

	// scan available targets... if we even have any.
	find( );

	// no point in aimbotting if we cannot fire this tick.
	if( !bCanShoot )
		return;

	if( noAimbot && ( ( g_TickbaseController.m_nTicksAfterUncharge >= 14 && !g_TickbaseController.IsCharged( ) ) || g_TickbaseController.m_nTicksAfterUncharge <= 3 ) )
		return;

	// finally set data when shooting.
	apply( );
}

EAutoStopType Aimbot::ChooseStop( ) {
	const bool bInAir = ( !( m_AimbotInfo.m_pLocal->m_fFlags( ) & FL_ONGROUND ) || !( g_Prediction.get_initial_vars( )->flags & FL_ONGROUND ) || ( m_AimbotInfo.m_pCmd->buttons & IN_JUMP ) );
	const bool bCanPlayerFire = TICKS_TO_TIME( m_AimbotInfo.m_pLocal->m_nTickBase( ) ) >= m_AimbotInfo.m_pLocal->m_flNextAttack( );

	// we are pulling out our weapon, or are in air, don't autostop.
	if( bInAir || !bCanPlayerFire )
		return EAutoStopType::STOP_NONE;

	// slide walk.
	if( m_AimbotInfo.m_pSettings->auto_stop == 1 )
		return EAutoStopType::STOP_SLIDE;

	// fake walk.
	if( m_AimbotInfo.m_pSettings->auto_stop == 2 )
		return EAutoStopType::STOP_FAKE;

	return ( EAutoStopType )m_AimbotInfo.m_pSettings->auto_stop;
}

void Aimbot::find( ) {
	struct BestTarget_t {
		C_CSPlayer *player;
		Vector pos;
		float damage;
		LagRecord_t record;
		int health;
		mstudiobbox_t *bbox;
	};

	mstudiobbox_t *tmp_bbox = nullptr;
	Vector       tmp_pos;
	float        tmp_damage;
	BestTarget_t best;
	best.player = nullptr;
	best.damage = -1.f;
	best.pos = Vector{ };
	best.record = {};

	if( m_targets.empty( ) )
		return;

	// iterate all targets.
	for( const auto &t : m_targets ) {
		if( t->m_deqRecords.empty( ) )
			continue;

		// this player broke lagcomp.
		// his bones have been resetup by our lagcomp.
		// therfore now only the front record is valid.
		if( HandleLagcomp( t ) /*g_Animations.BreakingTeleportDistance( t->m_pEntity->EntIndex( ) ) */ ) {
			LagRecord_t front = t->m_deqRecords.front( );
			const bool bFrontUsable = front.m_pEntity != nullptr && !front.m_bInvalid && !front.m_bGunGameImmunity;

			// настоящая запись всегда имеет приоритет над прогнозом: сервер отмотает
			// lagcomp именно к ней. прогноз пробуем ТОЛЬКО если по front не вышло.
			// m_bDelay означает "лаг слишком велик, чтобы предсказать по этой записи"
			// ( HandleLagcomp ), поэтому при нём front даже не трогаем.
			bool bFound = false;

			if( bFrontUsable && !t->m_bDelay ) {
				t->SetupHitboxes( &front, false );

				// rip something went wrong..
				if( !t->m_hitboxes.empty( )
					&& t->GetBestAimPosition( tmp_pos, tmp_damage, &front ) && SelectTarget( &front, tmp_pos, tmp_damage ) ) {
					// if we made it so far, set shit.
					best.player = t->m_pEntity;
					best.pos = tmp_pos;
					best.damage = tmp_damage;
					best.record = front;
					best.health = t->m_pEntity->m_iHealth( );
					best.bbox = front.m_bbox;

					bFound = true;

					//g_Simulation.RenderBoundingBox( front.m_vecMins, front.m_vecMaxs, front.m_vecPredictedOrigin, g_pGlobalVars->interval_per_tick * 4.f, Color( 0, 150, 255 ) );
				}
			}

			// цель ломает lagcomp, а настоящая запись не дала цели ( или лаг слишком велик,
			// чтобы по ней стрелять ). ровно та ситуация, под которую экстраполяция и писалась:
			// предсказываем, где цель будет к моменту обработки нашей команды.
			// ExtrapolateRecords сама откажется работать по стоящей цели, при нулевом чоке и
			// когда новый настоящий апдейт обгонит нашу команду. вернула nullptr - подавляем
			// выстрел, как и раньше.
			// исключение: сервер стоит на нашей новейшей записи - прогноз не нужен,
			// если по front не вышло, ждём свежую запись, а не стреляем предсказание.
			if( !bFound && !t->IsNewestServerHead( ) ) {
				LagRecord_t *pPredicted = t->ExtrapolateRecords( );
				if( !pPredicted )
					continue;

				t->SetupHitboxes( pPredicted, false );
				if( t->m_hitboxes.empty( ) )
					continue;

				if( t->GetBestAimPosition( tmp_pos, tmp_damage, pPredicted ) && SelectTarget( pPredicted, tmp_pos, tmp_damage ) ) {
					best.player = t->m_pEntity;
					best.pos = tmp_pos;
					best.damage = tmp_damage;
					best.record = *pPredicted;
					best.health = t->m_pEntity->m_iHealth( );
					best.bbox = pPredicted->m_bbox;
				}
			}
		}
		// player did not break lagcomp.
		// history aim is possible at this point.
		else {
			LagRecord_t ideal = g_Resolver.FindIdealRecord( t, g_Ragebot.GetMinimalDamage( t->m_pEntity ) );
			if( !ideal.m_pEntity || ideal.m_pEntity == nullptr || ideal.m_bInvalid || ideal.m_bGunGameImmunity )
				continue;

			t->SetupHitboxes( &ideal, true );
			if( t->m_hitboxes.empty( ) )
				continue;

			// try to select best record as target.
			if( t->GetBestAimPosition( tmp_pos, tmp_damage, &ideal ) && SelectTarget( &ideal, tmp_pos, tmp_damage ) ) {
				// if we made it so far, set shit.
				best.player = t->m_pEntity;
				best.pos = tmp_pos;
				best.damage = tmp_damage;
				best.record = ideal;
				best.health = t->m_pEntity->m_iHealth( );
				best.bbox = ideal.m_bbox;

				// we're done here
				break;
			}

			// still haven't found a record, continue to scan
			LagRecord_t latest = g_Animations.GetLatestRecord( t->m_pEntity->EntIndex( ) );

			if( g_Vars.rage.wait_for_lby_flick && g_Vars.rage.wait_for_lby_flick_key.enabled ) {
				latest.m_pEntity = nullptr;

				for( auto &rec : t->m_deqRecords ) {
					if( !rec.IsRecordValid( ) || rec.m_bInvalid || rec.m_bGunGameImmunity )
						continue;

					if( rec.m_bLBYFlicked ) {
						latest = rec;
						latest.m_pEntity = rec.m_pEntity;
						break;
					}
				}
			}

			if( !latest.m_pEntity || !latest.IsRecordValid( ) || latest.m_bInvalid || latest.m_pEntity == nullptr || latest.m_flSimulationTime == ideal.m_flSimulationTime || latest.m_nServerTick == ideal.m_nServerTick || latest.m_bGunGameImmunity || &latest == &ideal )
				continue;

			t->SetupHitboxes( &latest, false );
			if( t->m_hitboxes.empty( ) )
				continue;

			// rip something went wrong..
			if( t->GetBestAimPosition( tmp_pos, tmp_damage, &latest ) && SelectTarget( &latest, tmp_pos, tmp_damage ) ) {
				// if we made it so far, set shit.
				best.player = t->m_pEntity;
				best.pos = tmp_pos;
				best.damage = tmp_damage;
				best.record = latest;
				best.health = t->m_pEntity->m_iHealth( );
				best.bbox = latest.m_bbox;

				// we're done here
				break;
			}

			// экстраполяции здесь нет намеренно. сюда мы попадаем, когда цель НЕ ломает
			// lagcomp, т.е. сервер гарантированно отмотает её к настоящей записи истории -
			// любой прогноз в этом случае строго хуже. фоллбэк на ExtrapolateRecords( )
			// живёт в if-ветке выше, где настоящих пригодных записей нет по построению.
		}
	}

	const bool bInAir = ( !( m_AimbotInfo.m_pLocal->m_fFlags( ) & FL_ONGROUND ) || !( g_Prediction.get_initial_vars( )->flags & FL_ONGROUND ) || ( m_AimbotInfo.m_pCmd->buttons & IN_JUMP ) );

	const bool bCanPlayerFire = TICKS_TO_TIME( m_AimbotInfo.m_pLocal->m_nTickBase( ) ) >= m_AimbotInfo.m_pLocal->m_flNextAttack( );
	const bool bCanShoot = m_AimbotInfo.m_pLocal->CanShoot( );

	const bool bSkipForRevolver = m_AimbotInfo.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_REVOLVER && bCanPlayerFire && m_AimbotInfo.m_pWeapon->m_iClip1( ) > 0;
	const bool bCanRevolverShoot = !bCanShoot && bSkipForRevolver;

	const bool noAimbot = ( !bCanShoot && !bSkipForRevolver ) || g_Vars.globals.m_bShotWhileChoking;

	// verify our target and set needed data.
	if( best.player && !( !best.record.m_pEntity || best.record.m_pEntity == nullptr || best.record.m_bInvalid || best.record.m_bGunGameImmunity ) ) {
		m_AimbotInfo.m_iLastTarget = best.player->EntIndex( );

		// return if we can't fire.
		if( !bCanRevolverShoot && ( ( !bCanShoot || noAimbot ) && m_AimbotInfo.m_pSettings->auto_stop_between_shots ) ) {
			// we still want to autostop between shots, so choose our autostop here.
			m_stop = ChooseStop( );
			return;
		}

		// calculate aim angle.
		Math::VectorAngles( best.pos - m_AimbotInfo.m_vecEyePosition, m_angle );

		// set member vars.
		m_target = best.player;
		m_aim = best.pos;
		m_damage = best.damage;
		m_record = best.record;
		m_health = best.health;
		m_bbox = best.bbox;

		// write data, needed for traces / etc.
		m_record.ApplyRecord( best.player );

		// set autostop.
		m_stop = ChooseStop( );

		// force fakewalk if we are using fakewalk autostop.
		g_Vars.globals.m_bForceFakewalk = m_AimbotInfo.m_pSettings->auto_stop == 2;

		bool bHitchance = CheckHitchance( m_target, QAngle( m_angle.x, m_angle.y, m_angle.z ) );
		bool bHitchanceOn = m_AimbotInfo.m_pSettings->hitchance > 0.f;
		bool bHit = bHitchanceOn && bHitchance;

		// if we can scope.
		bool bScopedWeapon = ( m_AimbotInfo.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_AUG || m_AimbotInfo.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_SG553 || m_AimbotInfo.m_pWeaponInfo->m_iWeaponType == WEAPONTYPE_SNIPER_RIFLE );
		bool bCanScope = !m_AimbotInfo.m_pLocal->m_bIsScoped( ) && bScopedWeapon;

		// auto socpe.
		if( m_AimbotInfo.m_pSettings->auto_scope && bCanScope && !bInAir && bHitchanceOn ) {
			m_AimbotInfo.m_pCmd->buttons |= IN_ATTACK2;

			return;
		}

		// attack.
		if( ( bHit || !bHitchanceOn ) && g_Vars.rage.auto_fire ) {
			// выстрел только НЕ первой командой пачки: первую анимирует сервер
			if( g_pClientState->m_nChokedCommands( ) < 1 )
				return; // подождём следующую команду

			m_AimbotInfo.m_pCmd->buttons |= IN_ATTACK;

			// сам выстрел чокаем ( угол не уйдёт ни в анимацию, ни в сетевые eye angles ),
			// отправляем следующей командой ( RAX HandleShotChoke ).
			if( g_pClientState->m_nChokedCommands( ) < g_Vars.sv_maxusrcmdprocessticks->GetInt( ) - 2 ) {
				*m_AimbotInfo.m_pSendPacket = false;
				m_bSendNextCommand = true;
			}

			if( g_TickbaseController.m_bTapShot ) {
				g_TickbaseController.m_bTapShot = false;
			}
		}
	}
}

bool Aimbot::can_hit_hitbox( const Vector start, const Vector end ) {
	const auto studio_box = m_bbox;

	if( !studio_box )
		return false;

	Vector min, max;

	const auto is_capsule = studio_box->m_flRadius != -1.f;

	if( is_capsule ) {
		Math::VectorTransform( studio_box->bbmin, m_record.m_sAnims[ ESides::SIDE_SERVER ].m_pMatrix[ studio_box->bone ], min );
		Math::VectorTransform( studio_box->bbmax, m_record.m_sAnims[ ESides::SIDE_SERVER ].m_pMatrix[ studio_box->bone ], max );
		//const auto dist = Math::segment_to_segment( start, end, min, max );

		//if( dist < studio_box->m_flRadius )
		//	return true;
	}

	if( !is_capsule ) {
		Math::VectorTransform( Math::vector_rotate( studio_box->bbmin, studio_box->m_angAngles ), m_record.m_sAnims[ ESides::SIDE_SERVER ].m_pMatrix[ studio_box->bone ], min );
		Math::VectorTransform( Math::vector_rotate( studio_box->bbmax, studio_box->m_angAngles ), m_record.m_sAnims[ ESides::SIDE_SERVER ].m_pMatrix[ studio_box->bone ], max );

		Math::vector_i_transform( start, m_record.m_sAnims[ ESides::SIDE_SERVER ].m_pMatrix[ studio_box->bone ], min );
		Math::vector_i_rotate( end, m_record.m_sAnims[ ESides::SIDE_SERVER ].m_pMatrix[ studio_box->bone ], max );

		if( Math::intersect_line_with_bb( min, max, studio_box->bbmin, studio_box->bbmax ) )
			return true;
	}

	return false;
}

__forceinline bool IsValidHitgroup( int index ) {
	if( ( index >= Hitgroup_Head && index <= Hitgroup_RightLeg ) || index == Hitgroup_Gear )
		return true;

	return false;
}

static std::vector<std::tuple<float, float, float>> precomputed_seeds = {};
__forceinline void build_seed_table( ) {
	if( !precomputed_seeds.empty( ) )
		return;

	for( auto i = 0; i <= 255; i++ ) {
		RandomSeed( i + 1 );

		const auto pi_seed = RandomFloat( 0.f, DirectX::XM_2PI );

		precomputed_seeds.emplace_back( RandomFloat( 0.f, 1.f ),
										sin( pi_seed ), cos( pi_seed ) );
	}
}

bool Aimbot::CheckHitchance( C_CSPlayer *player, const QAngle &angle ) {
	auto networked_vars = g_Prediction.get_networked_vars( m_AimbotInfo.m_pCmd->command_number );


	size_t total_hits{};
	const auto needed_hits{ static_cast< size_t >( ( ( float )m_AimbotInfo.m_pSettings->hitchance * 255 / 100.f ) ) };

	const auto inaccuracy = networked_vars->inaccuracy;
	const auto spread = networked_vars->spread;
	const auto start{ m_AimbotInfo.m_vecEyePosition };

	auto *studio_model = g_pModelInfo->GetStudiomodel( player->GetModel( ) );
	if( !studio_model )
		return false;

	auto *hitbox_set = studio_model->pHitboxSet( player->m_nHitboxSet( ) );
	Vector mins{}, maxs{};

	auto *hitbox = m_bbox;

	if( ( g_Prediction.ideal_inaccuracy + 0.0005f ) >= networked_vars->inaccuracy ) {
		return true;
	}

	matrix3x4_t bone_transform;
	memcpy( &bone_transform, &m_record.m_sAnims[ ESides::SIDE_SERVER ].m_pMatrix[ hitbox->bone ], sizeof( matrix3x4_t ) );
	if( !hitbox->m_angAngles.IsZero( ) ) {
		matrix3x4_t temp;

		Math::AngleMatrix( hitbox->m_angAngles, temp );
		Math::ConcatTransforms( bone_transform, temp, bone_transform );
	}

	Vector vMin, vMax;
	Math::VectorTransform( hitbox->bbmin, bone_transform, vMin );
	Math::VectorTransform( hitbox->bbmax, bone_transform, vMax );

	Vector forward{}, right{}, up{};

	// get needed directional vectors.
	Math::AngleVectors( angle, forward, right, up );

	for( auto i = 0; i <= 255; i++ ) {
		const auto weapon_spread = m_AimbotInfo.m_pWeapon->CalculateSpread( i, inaccuracy, spread, false );

		Vector dir = ( forward + ( right * weapon_spread.x ) + ( up * weapon_spread.y ) ).Normalized( );

		const auto end = start + ( dir * m_AimbotInfo.m_pWeaponInfo->m_flWeaponRange );

		float m1, m2;

		if( m_AimbotInfo.m_pWeaponInfo->m_iWeaponType == WEAPONTYPE_SNIPER_RIFLE && ( m_AimbotInfo.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_G3SG1 || m_AimbotInfo.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_SCAR20 ) ) {
			CGameTrace tr{ };
			g_pEngineTrace->ClipRayToEntity( Ray_t( m_AimbotInfo.m_vecEyePosition, end ), MASK_SHOT, player, &tr );
			if( tr.hit_entity == player )
				total_hits++;
		}

		else {
			if( hitbox->m_flRadius == -1.f ) {
				CGameTrace tr;
				if( Autowall::ClipRayToHitbox( Ray_t( m_AimbotInfo.m_vecEyePosition, end ), m_bbox, m_record.m_sAnims[ ESides::SIDE_SERVER ].m_pMatrix[ m_bbox->bone ], tr ) >= 0 )
					total_hits++;
			}
			else {
				float dist = Math::segment_to_segment( m_AimbotInfo.m_vecEyePosition, end, vMin, vMax, m1, m2 );

				if( dist <= hitbox->m_flRadius * hitbox->m_flRadius ) {
					total_hits++;
				}
			}
		}

		if( total_hits >= needed_hits )
			return true;

		if( ( ( 255 - i ) + total_hits ) < needed_hits )
			return false;
	}

	return false;

#if 0
	// calculate hitchance.
	if( bHitchanceHits ) {
		flCalculatedChance = static_cast< float >( bHitchanceHits ) / static_cast< float >( MAX_INTERSECTIONS );
	}
	else {
		flCalculatedChance = 0.f;
	}

	if( !m_AimbotInfo.m_bIsZeus && ( ( flCalculatedChance >= flHitchance ) ) ) {
		// this might be not perfected, testing needed.
		float flAccuracyBoostChance = flHitchance;

		int nBoostHits{ };
		int nBoostDamage{ };

		bool bFailedBoost = false;

		int nValidSeeds{ };

		int nMaxPenetrations = 0;
		bool bSkipBoosts = false;
		for( int i = 0; i < MAX_INTERSECTIONS; ++i ) {
			if( ( !arrSeedData.at( i ).iSeed && i != 0 ) || !arrSeedData.at( i ).bIntersected )
				continue;

			const Vector vecEnd = arrSeedData.at( i ).vecEnd;
			++nValidSeeds;

			penetration::PenetrationInput_t in;

			in.m_start = g_Ragebot.m_AimbotInfo.m_vecEyePosition;
			in.m_damage = 2;
			in.m_can_pen = true;
			in.m_target = player;
			in.m_from = g_Ragebot.m_AimbotInfo.m_pLocal;
			in.m_pos = vecEnd;

			penetration::PenetrationOutput_t out;
			penetration::run( &in, &out );

			if( out.m_pen_count >= 3 ) {
				++nMaxPenetrations;
			}

			if( nMaxPenetrations >= int( bHitchanceHits * 0.25 ) ) {
				bSkipBoosts = true;
				break;
			}

			if( out.m_damage > 1.f ) {
				++nBoostHits;
				nBoostDamage += out.m_damage;
			}

			float flCalculatedBoostChance = static_cast< float >( nBoostHits ) / static_cast< float >( nValidSeeds );
			const float flBoostHitchance = static_cast< float >( nBoostDamage ) / static_cast< float >( nValidSeeds );
			int nMitigatedMinDamage = std::max( g_Ragebot.GetMinimalDamage( player ), static_cast< int >( m_damage / 3 ) );

			if( flBoostHitchance >= ( nMitigatedMinDamage * ( flHitchance ) ) && flCalculatedBoostChance >= flAccuracyBoostChance ) {
				break;
			}

			if( nBoostHits ) {
				if( static_cast< float >( nBoostHits + MAX_INTERSECTIONS - i ) / static_cast< float >( MAX_INTERSECTIONS ) < flAccuracyBoostChance ) {
					break;
				}
			}
		}

		// restore to regular behaviour.
		m_AimbotInfo.m_iOverrideMinDamage = 0;

		float flCalculatedBoostChance = static_cast< float >( nBoostHits ) / static_cast< float >( nValidSeeds );

		const float flBoostHitchance = static_cast< float >( nBoostDamage ) / static_cast< float >( nValidSeeds );
		int nMitigatedMinDamage = std::max( g_Ragebot.GetMinimalDamage( player ), static_cast< int >( m_damage / 3 ) );

		const bool bPassed = flBoostHitchance >= ( nMitigatedMinDamage * ( flHitchance ) );
		const bool bPassedBoost = flCalculatedBoostChance >= flAccuracyBoostChance;

		if( !bSkipBoosts ) {
			return bPassed && bPassedBoost;
		} // the else case is fucked tbh but it's a somewhat optimisation for some spots
	}
#endif
}

void VectorRotate( Vector in1, const matrix3x4_t &in2, Vector *out ) {
	float x = DotProduct( in1, in2[ 0 ] );
	float y = DotProduct( in1, in2[ 1 ] );
	float z = DotProduct( in1, in2[ 2 ] );

	out->x = x;
	out->y = y;
	out->z = z;
}

bool Animations::AnimationEntry_t::SetupHitboxPoints( LagRecord_t *record, matrix3x4_t *pMatrix, int index, std::vector< std::pair<mstudiobbox_t *, std::pair<Vector, bool>> > &points ) {
	// reset points.
	points.clear( );

	const model_t *model = m_pEntity->GetModel( );
	if( !model )
		return false;

	studiohdr_t *hdr = g_pModelInfo->GetStudiomodel( model );
	if( !hdr )
		return false;

	mstudiohitboxset_t *set = hdr->pHitboxSet( m_pEntity->m_nHitboxSet( ) );
	if( !set )
		return false;

	mstudiobbox_t *pHitbox = set->pHitbox( index );
	if( !pHitbox )
		return false;

	bool bDoDynamicScale = false;

	// get hitbox scales.
	float pointScale = g_Ragebot.m_AimbotInfo.m_pSettings->pointscale / 100.f;

	if( index == HITBOX_HEAD )
		pointScale = g_Ragebot.m_AimbotInfo.m_pSettings->headscale / 100.f;

	// do le dynamic
	if( pointScale == 0.f ) {
		bDoDynamicScale = true;

		// max pointscale, then we can scale down
		pointScale = 0.8f;
	}

	// big inair fix.
	if( !( record->m_fPredFlags & FL_ONGROUND ) )
		pointScale *= 0.5f;

	// по прогнозу стреляем только в центр хитбокса. порядок строк НАМЕРЕННЫЙ: проверка
	// динамического масштаба ( pointScale == 0.f выше ) стоит раньше, поэтому обнуление
	// здесь не включает bDoDynamicScale, а именно отключает multipoint. у прогноза нет
	// реального снапшота, к которому сервер отмотает, - края хитбокса там недостоверны.
	if( record->m_bExtrapolated )
		pointScale = 0.f;

	if( bDoDynamicScale ) {
		bool bDecreasing = false;

		// scale down by 60%
		if( record->m_vecPredVelocity.Length2D( ) > 0.1f ) {
			pointScale *= 0.4f;
			bDecreasing = true;
		}

		// scale down based on inaccuracy percentage
		auto pPredictedData = g_Prediction.get_networked_vars( g_Ragebot.m_AimbotInfo.m_pCmd->command_number );
		if( pPredictedData ) {
			pointScale *= ( g_Prediction.ideal_inaccuracy / pPredictedData->inaccuracy );
			// always clamp or it will go out of bounds
			//if( bDecreasing )
				//pointScale = std::clamp( pointScale, 0.3f, 0.8f );
		}

		// scale down by velocity modifier (if we get hit)
		if( g_Ragebot.m_AimbotInfo.m_pLocal->m_flVelocityModifier( ) < 1.f &&
			( g_Ragebot.m_AimbotInfo.m_pLocal->m_vecVelocity( ).Length2D( ) > 0.1f || g_Movement.PressingMovementKeys( g_Ragebot.m_AimbotInfo.m_pCmd ) ) &&
			g_Ragebot.m_AimbotInfo.m_pLocal->m_fFlags( ) & FL_ONGROUND ) {
			pointScale *= g_Ragebot.m_AimbotInfo.m_pLocal->m_flVelocityModifier( );
		}
	}

	// clamp pointscale to be sure it wont exceed our limit
	pointScale = std::clamp( pointScale, 0.f, 0.8f );

	Vector vecCenter = ( pHitbox->bbmax + pHitbox->bbmin ) * 0.5f;
	const auto vecCenterTransformed = vecCenter.Transform( pMatrix[ pHitbox->bone ] );

	const Vector vecMin = pHitbox->bbmin.Transform( pMatrix[ pHitbox->bone ] );
	const Vector vecMax = pHitbox->bbmax.Transform( pMatrix[ pHitbox->bone ] );

	Vector vecDelta = ( vecMax - vecMin ) * 0.5f;
	float flHitboxLength = vecDelta.Length( );

	const auto angDelta = vecDelta.Normalized( );
	matrix3x4_t matSpace;
	matSpace.AngleMatrix( { angDelta.x, angDelta.y, angDelta.z } );

	if( !g_Ragebot.m_AimbotInfo.m_bDidSetupMatrix ) {
		g_Ragebot.m_AimbotInfo.m_matSpaceRotation.AngleMatrix( QAngle( 0, 0, 1 ) );
		g_Ragebot.m_AimbotInfo.m_bDidSetupMatrix = true;
	}

	Vector vecSourcePoint = Vector( 0, 0, ( flHitboxLength * 0.7f * ( 1 + ( pointScale * 3.f ) ) ) );
	Vector vecSourcePointNoHeight = Vector( 0, 0, flHitboxLength * 0.7f );

	if( index == Hitboxes::HITBOX_HEAD ) {
		vecSourcePoint = Vector( 0, 0, ( flHitboxLength * 0.9f * ( 1 + ( pointScale * 3.f ) ) ) );
		vecSourcePointNoHeight = Vector( 0, 0, flHitboxLength * 0.25f );
	}

	const QAngle angAway = Math::CalcAngle( vecCenterTransformed, g_Ragebot.m_AimbotInfo.m_vecEyePosition );

	Vector vecForward, vecRight, vecUp;
	Math::AngleVectors( angAway, vecForward, vecRight, vecUp );

	float flDistanceToCenter = g_Ragebot.m_AimbotInfo.m_vecEyePosition.Distance( vecCenterTransformed );

	const Vector vecLeftPoint = vecCenterTransformed + vecRight * flDistanceToCenter;
	const Vector vecRightPoint = vecCenterTransformed - vecRight * flDistanceToCenter;

	auto SetupPoint = [&] ( Vector vecStartPosition, bool bUseHeight = false, float flPointScaleOverride = -1.f ) -> Vector {
		if( flPointScaleOverride != -1.f ) {
			if( index == Hitboxes::HITBOX_HEAD ) {
				vecSourcePoint = Vector( 0, 0, ( flHitboxLength * 0.9f * ( 1 + ( flPointScaleOverride * 3.f ) ) ) );
				vecSourcePointNoHeight = Vector( 0, 0, flHitboxLength * 0.25f );
			}
			else {
				vecSourcePoint = Vector( 0, 0, ( flHitboxLength * 0.7f * ( 1 + ( flPointScaleOverride * 3.f ) ) ) );
				vecSourcePointNoHeight = Vector( 0, 0, flHitboxLength * 0.7f );
			}
		}
		else {
			if( index == Hitboxes::HITBOX_HEAD ) {
				vecSourcePoint = Vector( 0, 0, ( flHitboxLength * 0.9f * ( 1 + ( pointScale * 3.f ) ) ) );
				vecSourcePointNoHeight = Vector( 0, 0, flHitboxLength * 0.25f );
			}
			else {
				vecSourcePoint = Vector( 0, 0, ( flHitboxLength * 0.7f * ( 1 + ( pointScale * 3.f ) ) ) );
				vecSourcePointNoHeight = Vector( 0, 0, flHitboxLength * 0.7f );
			}
		}

		auto vecPointDirection = vecCenterTransformed - vecStartPosition;
		vecPointDirection.Normalize( );

		auto flLength = vecPointDirection.Length2D( );

		float flDistanceToEdge = pHitbox->m_flRadius / flLength;
		Vector vecEndPoint = ( bUseHeight ? vecSourcePoint : vecSourcePointNoHeight ) + ( vecPointDirection * flDistanceToEdge );
		if( vecEndPoint.z > flHitboxLength ) {
			vecEndPoint.z -= flHitboxLength;
			vecEndPoint = vecEndPoint.Normalized( ) * pHitbox->m_flRadius;
			vecEndPoint.z += flHitboxLength;
		}
		else if( vecEndPoint.z < 0 ) {
			vecEndPoint = vecEndPoint.Normalized( ) * pHitbox->m_flRadius;
		}

		vecEndPoint = ( ( vecEndPoint - ( bUseHeight ? vecSourcePoint : vecSourcePointNoHeight ) ) * ( flPointScaleOverride != -1.f ? flPointScaleOverride : pointScale ) ) + ( bUseHeight ? vecSourcePoint : vecSourcePointNoHeight );

		VectorRotate( vecEndPoint, g_Ragebot.m_AimbotInfo.m_matSpaceRotation, &vecEndPoint );
		VectorRotate( vecEndPoint, matSpace, &vecEndPoint );

		vecEndPoint += vecCenterTransformed;

		return vecEndPoint;
	};

	auto AddPoint = [&] ( mstudiobbox_t *pHitbox, Vector vecPoint, bool bCenter ) -> void {
		points.push_back( { pHitbox, {vecPoint, bCenter} } );
	};

	// capsules
	if( pHitbox->m_flRadius != -1.0f ) {
		// add center points first
		AddPoint( pHitbox, SetupPoint( g_Ragebot.m_AimbotInfo.m_vecEyePosition, false, 0.f ), true );

		// then we can add multipoints
		if( pointScale >= 0.1f ) {
			// head multipoints
			if( index == Hitboxes::HITBOX_HEAD && g_Ragebot.m_AimbotInfo.m_pSettings->multipoint_head ) {
				AddPoint( pHitbox, SetupPoint( g_Ragebot.m_AimbotInfo.m_vecEyePosition, true ), false );

				// left / right
				AddPoint( pHitbox, SetupPoint( vecLeftPoint ), false );
				AddPoint( pHitbox, SetupPoint( vecRightPoint ), false );

				// left / right top
				AddPoint( pHitbox, SetupPoint( vecLeftPoint, true ), false );
				AddPoint( pHitbox, SetupPoint( vecRightPoint, true ), false );
			}
			else if( index == Hitboxes::HITBOX_STOMACH && g_Ragebot.m_AimbotInfo.m_pSettings->multipoint_stomach ) {
				// AddPoint( pHitbox, SetupPoint( g_Ragebot.m_AimbotInfo.m_vecEyePosition, true ), false );

				// left / right
				AddPoint( pHitbox, SetupPoint( vecLeftPoint ), false );
				AddPoint( pHitbox, SetupPoint( vecRightPoint ), false );
			}
			else if( index == Hitboxes::HITBOX_CHEST  ) {
				if( g_Ragebot.m_AimbotInfo.m_pSettings->multipoint_upper_chest )
					AddPoint( pHitbox, SetupPoint( g_Ragebot.m_AimbotInfo.m_vecEyePosition, true ), false );

				// left / right
				if( g_Ragebot.m_AimbotInfo.m_pSettings->multipoint_chest ) {

					AddPoint( pHitbox, SetupPoint( vecLeftPoint ), false );
					AddPoint( pHitbox, SetupPoint( vecRightPoint ), false );
				}
			}
			else if( ( index == Hitboxes::HITBOX_LEFT_UPPER_ARM || index == Hitboxes::HITBOX_RIGHT_UPPER_ARM ) && g_Ragebot.m_AimbotInfo.m_pSettings->multipoint_arms ) {
				Vector vecElbow{ pHitbox->bbmax.x + pHitbox->m_flRadius, vecCenter.y, vecCenter.z };
				AddPoint( pHitbox, vecElbow.Transform( pMatrix[ pHitbox->bone ] ), false );
			}

			if( g_Ragebot.m_AimbotInfo.m_pSettings->multipoint_legs ) {
				if( index == Hitboxes::HITBOX_RIGHT_THIGH || index == Hitboxes::HITBOX_LEFT_THIGH ) {
					Vector vecHalfBottom{ pHitbox->bbmax.x - ( pHitbox->m_flRadius * ( 0.5f * pointScale ) ), pHitbox->bbmax.y, pHitbox->bbmax.z };
					AddPoint( pHitbox, vecHalfBottom.Transform( pMatrix[ pHitbox->bone ] ), false );
				}
				else if( index == Hitboxes::HITBOX_RIGHT_CALF || index == Hitboxes::HITBOX_LEFT_CALF ) {
					Vector vecHalfTop{ pHitbox->bbmax.x - ( pHitbox->m_flRadius * ( 0.5f * pointScale ) ), pHitbox->bbmax.y, pHitbox->bbmax.z };
					AddPoint( pHitbox, vecHalfTop.Transform( pMatrix[ pHitbox->bone ] ), false );
				}
			}
		}
	}
	// boxes (feet, hands)
	else {
		// for feet, we have to do this magic shit or else our points will end up in the wrong positions
		// for example underground or above the heel or some shit
		// before fix: https://i.imgur.com/CBuGUi1.png
		// after fix : https://i.imgur.com/VhFW14O.png

		using AngleMatrix_t = void( __fastcall * )( const QAngle &, matrix3x4_t & );
		static AngleMatrix_t AngleMatrixFn = reinterpret_cast< AngleMatrix_t >( Memory::CallableFromRelative( Memory::Scan( XorStr( "client.dll" ), XorStr( "E8 ? ? ? ? 8B 07 89 46 0C" ) ) ) );

		matrix3x4_t matRotated, matFoot;
		AngleMatrixFn( pHitbox->m_angAngles, matRotated );
		Math::ConcatTransforms( pMatrix[ pHitbox->bone ], matRotated, matFoot );

		auto CorrectPoint = [&] ( Vector vecFootPoint ) -> Vector {
			vecFootPoint = { vecFootPoint.Dot( matFoot[ 0 ] ), vecFootPoint.Dot( matFoot[ 1 ] ), vecFootPoint.Dot( matFoot[ 2 ] ) };
			vecFootPoint += Vector( matFoot.m[ 0 ][ 3 ], matFoot.m[ 1 ][ 3 ], matFoot.m[ 2 ][ 3 ] );

			return vecFootPoint;
		};

		// first add center
		AddPoint( pHitbox, CorrectPoint( vecCenter ), true );

		// then we can add multipoints
		if( pointScale >= 0.1f ) {
			if( g_Ragebot.m_AimbotInfo.m_pSettings->multipoint_feet && ( index == Hitboxes::HITBOX_LEFT_FOOT || index == Hitboxes::HITBOX_RIGHT_FOOT ) ) {
				Vector vecFirst{ vecCenter.x + ( ( pHitbox->bbmax.x - vecCenter.x ) * pointScale ), vecCenter.y, vecCenter.z };
				AddPoint( pHitbox, CorrectPoint( vecFirst ), false );

				Vector vecSecond{ vecCenter.x + ( ( pHitbox->bbmin.x - vecCenter.x ) * pointScale ), vecCenter.y, vecCenter.z };
				AddPoint( pHitbox, CorrectPoint( vecSecond ), false );
			}
		}
	}

	// sort center points first
	std::sort( points.begin( ), points.end( ), [&] ( const std::pair<mstudiobbox_t *, std::pair<Vector, bool>> &a, const std::pair<mstudiobbox_t *, std::pair<Vector, bool>> &b ) {
		return ( int )a.second.second > ( int )b.second.second;
	} );

	return true;
}

// доля гипотез ног ( сервер + 3 стороны ), в которых луч на точку задевает ТОТ ЖЕ хитбокс.
// 1.0 = попадём при любом footyaw в пределах eye ± 58 ( RAX CheckIntersection ).
float Aimbot::GetPointSafety( LagRecord_t *pRecord, const Vector &vecStart, const Vector &vecPoint, int nHitbox ) {
	C_CSPlayer *pEntity = pRecord->m_pEntity;
	if( !pEntity )
		return 0.f;

	Vector vecDir = vecPoint - vecStart;
	vecDir.Normalize( );
	const Vector vecEnd = vecStart + vecDir * 8192.f;

	const int nBones = pEntity->m_CachedBoneData( ).Count( );
	if( nBones <= 0 || !pEntity->m_CachedBoneData( ).Base( ) )
		return 0.f;

	matrix3x4_t pBackup[ MAXSTUDIOBONES ];
	std::memcpy( pBackup, pEntity->m_CachedBoneData( ).Base( ), nBones * sizeof( matrix3x4_t ) );

	static constexpr ESides arrSides[ ] = { ESides::SIDE_SERVER, ESides::SIDE_MIDDLE, ESides::SIDE_OPPOSITEL, ESides::SIDE_OPPOSITER };

	int nHits = 0, nTotal = 0;
	for( const ESides eSide : arrSides ) {
		// сторона не считалась ( запись без предыдущей ) - не учитываем ни за, ни против
		if( eSide != ESides::SIDE_SERVER && !pRecord->m_sAnims[ eSide ].m_bUpdated )
			continue;

		std::memcpy( pEntity->m_CachedBoneData( ).Base( ), pRecord->m_sAnims[ eSide ].m_pMatrix, nBones * sizeof( matrix3x4_t ) );
		pEntity->ForceBoneCache( );

		CGameTrace tr;
		g_pEngineTrace->ClipRayToEntity( Ray_t( vecStart, vecEnd ), MASK_SHOT, pEntity, &tr );

		++nTotal;
		if( tr.hit_entity == pEntity && tr.hitbox == nHitbox )
			++nHits;
	}

	std::memcpy( pEntity->m_CachedBoneData( ).Base( ), pBackup, nBones * sizeof( matrix3x4_t ) );
	pEntity->ForceBoneCache( );

	return nTotal ? float( nHits ) / float( nTotal ) : 0.f;
}

bool Animations::AnimationEntry_t::GetBestAimPosition( Vector &aim, float &damage, LagRecord_t *record ) {
	bool                  done, pen;
	float                 dmg, pendmg;
	HitscanData_t         scan;
	float                 flBestSafety = 0.f;
	std::vector< std::pair<mstudiobbox_t *, std::pair<Vector, bool>> > points;

	// get player hp.
	int hp = std::min( 100, m_pEntity->m_iHealth( ) );

	if( g_Ragebot.m_AimbotInfo.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_ZEUS ) {
		dmg = pendmg = hp;
		pen = true;
	}

	else {
		dmg = pendmg = g_Ragebot.GetMinimalDamage( m_pEntity );
		pen = true;
	}

	// do this here instead so it's dependant on the record, not entity (ground flag) 
	if( g_Ragebot.m_AimbotInfo.m_pSettings->lower_air_minimal_damage && dmg >= 100 && !( record->m_fFlags & FL_ONGROUND ) && m_pEntity->m_iHealth( ) >= 92 ) {
		dmg = 60.f;
	}

	// write all data of this record l0l.
	// record->cache( );
	record->ApplyRecord( m_pEntity );

	// iterate hitboxes.
	for( const auto &it : m_hitboxes ) {
		done = false;

		// setup points on hitbox.
		if( !SetupHitboxPoints( record, record->m_sAnims[ ESides::SIDE_SERVER ].m_pMatrix, it.m_index, points ) )
			continue;

		// iterate points on hitbox.
		for( const auto &point : points ) {
			penetration::PenetrationInput_t in;

			in.m_start = g_Ragebot.m_AimbotInfo.m_vecEyePosition;
			in.m_damage = dmg;
			in.m_can_pen = true;
			in.m_target = m_pEntity;
			in.m_from = g_Ragebot.m_AimbotInfo.m_pLocal;
			in.m_pos = point.second.first;

			g_Visuals.vecAimpoints[ m_pEntity->EntIndex( ) ].push_back( point.second.first );

			// ignore mindmg.
			// -- probably not a very good idea.
			//if( it.m_mode == HitscanMode::LETHAL || it.m_mode == HitscanMode::LETHAL2 )
			//	in.m_damage = 1.f;

			penetration::PenetrationOutput_t out;

			// we can hit p!
			if( penetration::run( &in, &out ) ) {
				// nope we did not hit head..
				if( it.m_index == HITBOX_HEAD && out.m_hitgroup != Hitgroup_Head )
					continue;

				// доля гипотез ног, в которых луч на точку задевает ТОТ ЖЕ хитбокс
				const float flSafety = g_Ragebot.GetPointSafety( record, g_Ragebot.m_AimbotInfo.m_vecEyePosition, point.second.first, it.m_index );

				// низкая уверенность резолвера: только точки, которые попадают при ЛЮБЫХ ногах
				if( record->m_eConfidence == EConfidence::CONF_LOW && flSafety < 1.f )
					continue;

				// prefered hitbox, just stop now.
				if( it.m_mode == HitscanMode::PREFER )
					done = true;

				// this hitbox requires lethality to get selected, if that is the case.
				// we are done, stop now.
				else if( it.m_mode == HitscanMode::LETHAL && out.m_damage >= dmg && out.m_damage >= m_pEntity->m_iHealth( ) )
					done = true;

				// 2 shots will be sufficient to kill.
				else if( it.m_mode == HitscanMode::LETHAL2 && out.m_damage >= dmg && ( out.m_damage * 2.f ) >= m_pEntity->m_iHealth( ) )
					done = true;

				// this hitbox has normal selection, it needs to have more damage.
				else if( it.m_mode == HitscanMode::NORMAL ) {
					// we did more damage.
					if( out.m_damage > scan.m_damage
						// при равном уроне выбирай точку корпуса с flSafety == 1.f
						|| ( out.m_damage == scan.m_damage && flSafety >= 1.f && it.m_index != HITBOX_HEAD && flBestSafety < 1.f ) ) {
						// save new best data.
						scan.m_damage = out.m_damage;
						scan.m_pos = point.second.first;
						record->m_bbox = point.first;
						flBestSafety = flSafety;

						// if the first point is lethal
						// screw the other ones.
						if( point.second == points.front( ).second && out.m_damage >= m_pEntity->m_iHealth( ) )
							break;
					}
				}

				// we found a preferred / lethal hitbox.
				if( done ) {
					// save new best data.
					scan.m_damage = out.m_damage;
					scan.m_pos = point.second.first;
					record->m_bbox = point.first;
					flBestSafety = flSafety;
					break;
				}
			}
		}

		// ghetto break out of outer loop.
		if( done )
			break;
	}

	// we found something that we can damage.
	// set out vars.
	if( scan.m_damage > 0.f ) {
		aim = scan.m_pos;
		damage = scan.m_damage;
		return true;
	}

	return false;
}

bool Aimbot::SelectTarget( LagRecord_t *record, const Vector &aim, float damage ) {
	// damage is enough to kill the target, choose this.
	if( damage >= record->m_pEntity->m_iHealth( ) )
		return true;

	// otherwise, base it on distance.
	float dist = ( record->m_vecPredOrigin - m_AimbotInfo.m_vecEyePosition ).Length( );
	if( dist < m_best_dist ) {
		m_best_dist = dist;
		return true;
	}

	// to:do, have something where if there are 2 targets, and one of them is jumping; it will target the one on ground (since they are more of a threat).

	return false;
}

std::string HitgroupToName( const int hitgroup ) {
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

// will only use more accurate one for shot logs, will be more usefull
std::string HitgroupToString( int hitgroup ) {
	switch( hitgroup ) {
		case Hitgroup_Generic:
			return XorStr( "GENERIC" );
		case Hitgroup_Head:
			return XorStr( "HEAD" );
		case Hitgroup_Chest:
			return XorStr( "CHEST" );
		case Hitgroup_Stomach:
			return XorStr( "STOMACH" );
		case Hitgroup_LeftArm:
			return XorStr( "LEFT ARM" );
		case Hitgroup_RightArm:
			return XorStr( "RIGHT ARM" );
		case Hitgroup_LeftLeg:
			return XorStr( "LEFT LEG" );
		case Hitgroup_RightLeg:
			return XorStr( "RIGHT LEG" );
		case Hitgroup_Neck:
			return XorStr( "NECK" );
	}
	return XorStr( "GENERIC" );
}


void Aimbot::apply( ) {
	bool attack, attack2;

	// attack states.
	attack = ( m_AimbotInfo.m_pCmd->buttons & IN_ATTACK );
	//attack2 = ( m_AimbotInfo.m_pWeapon->m_iItemDefinitionIndex( ) == WEAPON_REVOLVER && m_AimbotInfo.m_pCmd->buttons & IN_ATTACK2 );

	// ensure we're attacking.
	if( attack /*|| attack2*/ ) {
		//	m_record.m_bAttemptedShot = true;

		if( !g_Vars.globals.m_bFakeWalking ) {
			// RAX HandleShotChoke: выстрел — средняя команда пачки, не первая и не последняя.
			// сам выстрел чокаем ( угол не уйдёт ни в анимацию, ни в сетевые eye angles ),
			// пакет уйдёт следующей командой через m_bSendNextCommand ( выставляется ниже ).
			if( !g_Vars.globals.m_bShotWhileHiding &&
				g_pClientState->m_nChokedCommands( ) < g_Vars.sv_maxusrcmdprocessticks->GetInt( ) - 2 )
				*m_AimbotInfo.m_pSendPacket = false;
			else if( !g_Vars.globals.m_bShotWhileHiding )
				*m_AimbotInfo.m_pSendPacket = g_Vars.rage.exploit && g_Vars.rage.double_tap_bind.enabled;
		}

		if( m_target ) {
			// make sure to aim at un-interpolated data.
			// do this so BacktrackEntity selects the exact record.
			if( m_record.m_pEntity != nullptr && !m_record.m_bBrokeTeleportDst ) {
				// округляем лерп ВВЕРХ: цель сервера никогда не уходит ниже simtime записи
				const int nLerpTicks = static_cast< int >( std::ceil( g_Animations.m_fLerpTime / g_pGlobalVars->interval_per_tick - 0.001f ) );
				m_AimbotInfo.m_pCmd->tick_count = TIME_TO_TICKS( m_record.m_flSimulationTime ) + nLerpTicks;
			}

			// set angles to target.
			m_AimbotInfo.m_pCmd->viewangles = QAngle( m_angle.x, m_angle.y, m_angle.z );

			// if not silent aim, apply the viewangles.
			if( !g_Vars.rage.silent_aim )
				g_pEngine->SetViewAngles( QAngle( m_angle.x, m_angle.y, m_angle.z ) );
		}

		// nospread.
		//if( g_menu.main.aimbot.nospread.get( ) && g_menu.main.config.mode.get( ) == 1 )
		//	NoSpread( );

		// norecoil.
		//if( g_menu.main.aimbot.norecoil.get( ) )
		m_AimbotInfo.m_pCmd->viewangles -= m_AimbotInfo.m_pLocal->m_aimPunchAngle( ) * g_Vars.weapon_recoil_scale->GetFloat( );

		// store fired shot.
		{
			// if we got here, let's increment the current shotid
			++m_AimbotInfo.m_iShotID;

			player_info_t info;
			if( g_pEngine->GetPlayerInfo( m_record.m_nEntIndex, &info ) ) {
				std::string playerName{ info.szName };

				if( playerName.find( XorStr( "\n" ) ) != std::string::npos ) {
					playerName = XorStr( "[BLANK]" );
				}

				if( playerName.length( ) > 10 ) {
					playerName.resize( 10 );
					playerName.append( XorStr( "..." ) );
				}

				auto fnBoolInfo = [&] ( bool x )-> std::pair<const char *, Color> {
					if( x ) {
						return { "TRUE", Color( 200, 255, 0 ) };
					}

					return { "FALSE", Color( 100, 100, 100 ) };
				};

				auto ConfData = [&] ( EConfidence conf ) -> std::pair<std::string, Color> {
					switch( conf ) {
						case EConfidence::CONF_LOW:
							return { "LOW", Color( 255,0,0 ) };
							break;
						case EConfidence::CONF_MED:
							return { "MEDIUM", Color( 255, 150,0 ) };
							break;
						case EConfidence::CONF_HIGH:
							return { "HIGH", Color( 150, 160, 0 ) };
							break;
						case EConfidence::CONF_VHIGH:
							return { "VERY HIGH", Color( 200, 255, 0 ) };
							break;
					}

					return { "?", Color( 255,0,0 ) };
				};

				auto damage = ( int )m_damage;
				damage = std::clamp( damage, 0, 100 );
				const int green = 255 - damage * 2.55;
				const int red = damage * 2.55;


				auto LBY1info = fnBoolInfo( m_record.m_bLBYFlicked );
				auto LCinfo = fnBoolInfo( m_record.m_bBrokeTeleportDst );
				auto XTPinfo = fnBoolInfo( m_record.m_bExtrapolated );
				auto CONFinfo = ConfData( m_record.m_eConfidence );
				//auto LINEARinfo = fnBoolInfo( m_record.m_bLikelyLinear );

				// std::to_string( it->m_ShotData.m_iShotID )
				g_pCVar->ConsoleColorPrintf( Color( 255, 105, 181, 255 ), XorStr( "[ AIMBOT ] " ) );
				g_pCVar->ConsoleColorPrintf( Color( 255, 255, 255, 255 ), XorStr( "shot at " ) );
				g_pCVar->ConsoleColorPrintf( Color( 255, 229, 204, 255 ), XorStr( "%s " ), playerName.data( ) );
				g_pCVar->ConsoleColorPrintf( Color( 255, 255, 255, 255 ), XorStr( "in " ) );
				g_pCVar->ConsoleColorPrintf( Color( 255, 204, 204, 255 ), XorStr( "%s " ), HitgroupToString( m_bbox->group ).data( ) );
				g_pCVar->ConsoleColorPrintf( Color( 255, 255, 255, 255 ), XorStr( "for " ) );
				g_pCVar->ConsoleColorPrintf( Color( red, green, 0, 255 ), XorStr( "%d " ), ( int )m_damage );
				g_pCVar->ConsoleColorPrintf( Color( 100, 100, 255, 255 ), XorStr( "[ " ) );
				g_pCVar->ConsoleColorPrintf( Color( 255, 255, 255, 255 ), XorStr( "LBY: " ) );
				g_pCVar->ConsoleColorPrintf( LBY1info.second, XorStr( "%s " ), LBY1info.first );
				g_pCVar->ConsoleColorPrintf( Color( 255, 255, 255, 255 ), XorStr( "LC: " ) );
				g_pCVar->ConsoleColorPrintf( LCinfo.second, XorStr( "%s " ), LCinfo.first );
				g_pCVar->ConsoleColorPrintf( Color( 255, 255, 255, 255 ), XorStr( "EXTRAP: " ) );
				g_pCVar->ConsoleColorPrintf( XTPinfo.second, XorStr( "%s " ), XTPinfo.first );
				g_pCVar->ConsoleColorPrintf( Color( 255, 255, 255, 255 ), XorStr( "SPEED: " ) );
				g_pCVar->ConsoleColorPrintf( Color( 100, 100, 100 ), XorStr( "%.2f " ), m_record.m_vecPredVelocity.Length2D( ) );
				g_pCVar->ConsoleColorPrintf( Color( 255, 255, 255, 255 ), XorStr( "RES: " ) );
				std::transform( m_record.m_szResolver.begin( ), m_record.m_szResolver.end( ), m_record.m_szResolver.begin( ), ::toupper );
				g_pCVar->ConsoleColorPrintf( m_record.m_bIsResolved ? Color( 200, 255, 0 ) : Color( 100, 100, 100 ), XorStr( "%s (%i) " ), m_record.m_szResolver.data( ), m_record.m_iResolverType );
				g_pCVar->ConsoleColorPrintf( Color( 255, 255, 255, 255 ), XorStr( "CONF: " ) );
				g_pCVar->ConsoleColorPrintf( CONFinfo.second, XorStr( "%s " ), CONFinfo.first.data( ) );
				g_pCVar->ConsoleColorPrintf( Color( 255, 255, 255, 255 ), XorStr( "SHOT: " ) );
				g_pCVar->ConsoleColorPrintf( Color( 100, 100, 100 ), XorStr( "%i " ), m_AimbotInfo.m_iShotID % 100 );
				g_pCVar->ConsoleColorPrintf( Color( 255, 255, 255, 255 ), XorStr( "RECORD: " ) );
				g_pCVar->ConsoleColorPrintf( Color( 100, 100, 100 ), XorStr( "%i " ), m_record.m_nIdeality );
				if( !m_record.m_bExtrapolated ) {
					g_pCVar->ConsoleColorPrintf( Color( 255, 255, 255, 255 ), XorStr( "BT: " ) );
					g_pCVar->ConsoleColorPrintf( Color( 100, 100, 100 ), XorStr( "%i " ), TIME_TO_TICKS( m_target->m_flSimulationTime( ) ) - TIME_TO_TICKS( m_record.m_flSimulationTime ) );
				}
				if( m_record.m_bBrokeTeleportDst ) {
					g_pCVar->ConsoleColorPrintf( Color( 255, 255, 255, 255 ), XorStr( "LAG: " ) );
					g_pCVar->ConsoleColorPrintf( Color( 100, 100, 100 ), XorStr( "%i " ), m_record.m_nChokedTicks );
				}
				g_pCVar->ConsoleColorPrintf( Color( 100, 100, 255, 255 ), XorStr( "]\n" ) );
				ShotInformation_t shot;
				shot.m_szName = std::move( playerName );
				shot.m_iPredictedDamage = m_damage;
				shot.m_iEnemyHealth = m_health;
				shot.m_iMinimalDamage = g_Ragebot.GetMinimalDamage( m_record.m_pEntity );
				//shot.m_iHitchance = pPoint->m_iHitchance;
				//shot.m_iTraceHitgroup = pPoint->m_iTraceHitgroup;
				shot.m_flTime = g_pGlobalVars->realtime;
				shot.m_iTargetIndex = m_record.m_nEntIndex;
				shot.m_bTapShot = g_TickbaseController.m_bTapShot;
				shot.m_vecStart = m_AimbotInfo.m_vecEyePosition;
				shot.m_pHitbox = m_bbox;
				shot.m_pRecord = m_record;
				shot.m_vecEnd = m_aim;
				shot.m_iHistoryTicks = 0;
				shot.m_bHadPredError = false; // happened a few times, always had a pred error then
				shot.m_szFlags = {};
				shot.m_iShotID = m_AimbotInfo.m_iShotID % 100;
				// keep the unwrapped counter too, event matching needs a unique id.
				shot.m_iUniqueShotID = m_AimbotInfo.m_iShotID;
				shot.m_szFiredLog = {};

				g_ShotHandling.RegisterShot( shot );

				m_AimbotInfo.m_iLastTarget = m_record.m_nEntIndex;
			}
		}

		g_FakeLag.m_iAwaitingChoke = 1;
		m_bSendNextCommand = m_AimbotInfo.m_pWeapon->m_iItemDefinitionIndex( ) != WEAPON_REVOLVER;
	}
}

bool Aimbot::HandleLagcomp( Animations::AnimationEntry_t *data ) {
	// reset this.
	data->m_bDelay = false;

	// player is not breaking lagcomp, we can still shoot at history records.
	// return false to allow this.
	if( !g_Animations.BreakingTeleportDistance( data->m_pEntity->EntIndex( ) ) )
		return false;

	// сервер стоит на нашей новейшей записи к моменту обработки команды:
	// стреляем по m_deqRecords[ 0 ] без задержки и без экстраполяции.
	// m_bDelay уже сброшен выше, front валиден по построению IsNewestServerHead.
	if( data->IsNewestServerHead( ) )
		return true;

	// we are not using wait fake-lag correction, just shoot when possible.
	if( g_Ragebot.m_AimbotInfo.m_pSettings->fake_lag_correction != 1 )
		return true;

	// get net channel info.
	INetChannel *pNetChannel = g_pEngine->GetNetChannelInfo( );
	if( !pNetChannel )
		return true;

	// get lag record.
	LagRecord_t *pRecord = &data->m_deqRecords[ 0 ];

	// start at current server tick.
	const int nStartTick = g_pClientState->m_ClockDriftManager( ).m_nServerTick + 1;

	// try to predict when our shot will be registered.
	int nArrivalTick = nStartTick;

	// account for latency.
	const float flOutgoingLatency = pNetChannel->GetLatency( FLOW_OUTGOING );
	int nServerLatencyTicks = TIME_TO_TICKS( flOutgoingLatency );
	int nClientLatencyTicks = TIME_TO_TICKS( pNetChannel->GetLatency( FLOW_INCOMING ) );
	nArrivalTick += nServerLatencyTicks;

	// account for our current choke cycle.
	int nChokeCycle = std::max( g_FakeLag.m_iAwaitingChoke, 0 ) - g_pClientState->m_nChokedCommands( );
	if( g_FakeLag.m_iAwaitingChoke > 1 )
		nArrivalTick += std::max( nChokeCycle, 0 );

	// get the delta in ticks between the last server net update
	// and the net update on which we created this record.
	int updatedelta = g_pClientState->m_ClockDriftManager( ).m_nServerTick - pRecord->m_nServerTick;

	if( !updatedelta ) {
		return true;
	}

	const int receive_tick = std::abs( nArrivalTick - TIME_TO_TICKS( pRecord->m_flSimulationTime ) );
	int nChokedSafe = pRecord->m_nChokedTicks;
	if( nChokedSafe <= 0 )
		nChokedSafe = 1;

	// too much lag to predict, exit and delay shot
	if( receive_tick / nChokedSafe > 19 ) {
		data->m_bDelay = true;

		return true;
	}

	const float adjusted_arrive_tick = TIME_TO_TICKS( flOutgoingLatency + g_pGlobalVars->realtime - pRecord->m_flCreationTime );

	// too much lag to predict, exit and delay shot
	if( adjusted_arrive_tick - pRecord->m_nChokedTicks >= 0 ) {
		data->m_bDelay = true;

		return true;
	}

	return true;
}

void Aimbot::NoSpread( ) {
	//bool    attack2;
	//vec3_t  spread, forward, right, up, dir;

	//// revolver state.
	//attack2 = ( g_cl.m_weapon_id == REVOLVER && ( g_cl.m_cmd->m_buttons & IN_ATTACK2 ) );

	//// get spread.
	//spread = g_cl.m_weapon->CalculateSpread( g_cl.m_cmd->m_random_seed, attack2 );

	//// compensate.
	//g_cl.m_cmd->m_view_angles -= { -math::rad_to_deg( std::atan( spread.length_2d( ) ) ), 0.f, math::rad_to_deg( std::atan2( spread.x, spread.y ) ) };
}