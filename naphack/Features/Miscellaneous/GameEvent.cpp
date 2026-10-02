#include "GameEvent.hpp"
#include "../Visuals/EventLogger.hpp"
#include "../../pandora.hpp"
#include "../../Utils/extern/FnvHash.hpp"
#include "../../SDK/Classes/Player.hpp"
#include "../Rage/Animations.hpp"
#include "../../Libraries/BASS/API.h"
#include "../../Renderer/Textures/weaponicons.h"
#include <sstream>
#include "../../SDK/core.hpp"
#include "../Visuals/Models.hpp"
#include "../Rage/Resolver.hpp"
#include "../Rage/ShotHandling.hpp"
#pragma comment(lib,"Winmm.lib")
#include "../Rage/TickbaseShift.hpp"
#include "../Visuals/Hitmarker.hpp"
#include "AutomaticPurchase.hpp"
#include "../Rage/AntiAim.hpp"
#include "../Visuals/Visuals.hpp"
#include <fstream>
#include "GrenadeWarning.hpp"
#include "../Scripting/Scripting.hpp"
#include "../../Loader/Security/Security.hpp"
#include "SkinChanger.hpp"

#include "Movement.hpp"
#include "../Visuals/BulletTracer.hpp"

#define ADD_GAMEEVENT(n)  g_pGameEvent->AddListener(this, XorStr(#n), false)

std::string HitgroupToName__( const int hitgroup ) {
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

GameEvent g_GameEvent;

void GameEvent::Register( ) {
	ADD_GAMEEVENT( team_info );
	ADD_GAMEEVENT( team_score );
	ADD_GAMEEVENT( teamplay_broadcast_audio );
	ADD_GAMEEVENT( player_team );
	ADD_GAMEEVENT( player_class );
	ADD_GAMEEVENT( player_death );
	ADD_GAMEEVENT( player_say );
	ADD_GAMEEVENT( player_chat );
	ADD_GAMEEVENT( player_score );
	ADD_GAMEEVENT( player_spawn );
	ADD_GAMEEVENT( player_shoot );
	ADD_GAMEEVENT( player_use );
	ADD_GAMEEVENT( player_changename );
	ADD_GAMEEVENT( player_hintmessage );
	//ADD_GAMEEVENT( base_player_teleported );
	ADD_GAMEEVENT( game_init );
	ADD_GAMEEVENT( game_newmap );
	ADD_GAMEEVENT( game_start );
	ADD_GAMEEVENT( game_end );
	ADD_GAMEEVENT( round_start );
	ADD_GAMEEVENT( round_end );
	ADD_GAMEEVENT( game_message );
	ADD_GAMEEVENT( break_breakable );
	ADD_GAMEEVENT( break_prop );
	ADD_GAMEEVENT( entity_killed );
	ADD_GAMEEVENT( bonus_updated );
	ADD_GAMEEVENT( achievement_event );
	ADD_GAMEEVENT( achievement_increment );
	ADD_GAMEEVENT( physgun_pickup );
	ADD_GAMEEVENT( flare_ignite_npc );
	ADD_GAMEEVENT( helicopter_grenade_punt_miss );
	ADD_GAMEEVENT( user_data_downloaded );
	ADD_GAMEEVENT( ragdoll_dissolved );
	ADD_GAMEEVENT( hltv_changed_mode );
	ADD_GAMEEVENT( hltv_changed_target );
	ADD_GAMEEVENT( vote_ended );
	ADD_GAMEEVENT( vote_started );
	ADD_GAMEEVENT( vote_changed );
	ADD_GAMEEVENT( vote_passed );
	ADD_GAMEEVENT( vote_failed );
	ADD_GAMEEVENT( vote_cast );
	ADD_GAMEEVENT( vote_options );
	//ADD_GAMEEVENT( replay_saved );
	//ADD_GAMEEVENT( entered_performance_mode );
	//ADD_GAMEEVENT( browse_replays );
	//ADD_GAMEEVENT( replay_youtube_stats );
	ADD_GAMEEVENT( inventory_updated );
	ADD_GAMEEVENT( cart_updated );
	ADD_GAMEEVENT( store_pricesheet_updated );
	ADD_GAMEEVENT( gc_connected );
	ADD_GAMEEVENT( item_schema_initialized );
	ADD_GAMEEVENT( weapon_fire );
	ADD_GAMEEVENT( player_hurt );
	ADD_GAMEEVENT( player_connect_full );
	ADD_GAMEEVENT( item_purchase );
	ADD_GAMEEVENT( bullet_impact );
	ADD_GAMEEVENT( bomb_planted );
	ADD_GAMEEVENT( player_disconnect );
	ADD_GAMEEVENT( other_death );
	ADD_GAMEEVENT( bomb_beginplant );
	ADD_GAMEEVENT( bomb_abortplant );
	ADD_GAMEEVENT( bomb_defused );
	ADD_GAMEEVENT( bomb_exploded );
	ADD_GAMEEVENT( bomb_dropped );
	ADD_GAMEEVENT( bomb_pickup );
	ADD_GAMEEVENT( defuser_dropped );
	ADD_GAMEEVENT( defuser_pickup );
	ADD_GAMEEVENT( announce_phase_end );
	ADD_GAMEEVENT( cs_intermission );
	ADD_GAMEEVENT( bomb_begindefuse );
	ADD_GAMEEVENT( bomb_abortdefuse );
	ADD_GAMEEVENT( hostage_follows );
	ADD_GAMEEVENT( hostage_hurt );
	ADD_GAMEEVENT( hostage_killed );
	ADD_GAMEEVENT( hostage_rescued );
	ADD_GAMEEVENT( hostage_stops_following );
	ADD_GAMEEVENT( hostage_rescued_all );
	ADD_GAMEEVENT( hostage_call_for_help );
	ADD_GAMEEVENT( vip_escaped );
	ADD_GAMEEVENT( vip_killed );
	ADD_GAMEEVENT( player_radio );
	ADD_GAMEEVENT( bomb_beep );
	ADD_GAMEEVENT( weapon_fire_on_empty );
	ADD_GAMEEVENT( grenade_thrown );
	ADD_GAMEEVENT( weapon_outofammo );
	ADD_GAMEEVENT( weapon_reload );
	ADD_GAMEEVENT( weapon_zoom );
	ADD_GAMEEVENT( silencer_detach );
	ADD_GAMEEVENT( inspect_weapon );
	ADD_GAMEEVENT( weapon_zoom_rifle );
	ADD_GAMEEVENT( player_spawned );
	ADD_GAMEEVENT( item_pickup );
	//ADD_GAMEEVENT( item_pickup_failed );
	ADD_GAMEEVENT( item_remove );
	ADD_GAMEEVENT( ammo_pickup );
	ADD_GAMEEVENT( item_equip );
	ADD_GAMEEVENT( enter_buyzone );
	ADD_GAMEEVENT( exit_buyzone );
	ADD_GAMEEVENT( buytime_ended );
	ADD_GAMEEVENT( enter_bombzone );
	ADD_GAMEEVENT( exit_bombzone );
	ADD_GAMEEVENT( enter_rescue_zone );
	ADD_GAMEEVENT( exit_rescue_zone );
	ADD_GAMEEVENT( silencer_off );
	ADD_GAMEEVENT( silencer_on );
	ADD_GAMEEVENT( buymenu_open );
	ADD_GAMEEVENT( buymenu_close );
	ADD_GAMEEVENT( round_prestart );
	ADD_GAMEEVENT( round_poststart );
	ADD_GAMEEVENT( grenade_bounce );
	ADD_GAMEEVENT( hegrenade_detonate );
	ADD_GAMEEVENT( flashbang_detonate );
	ADD_GAMEEVENT( smokegrenade_detonate );
	ADD_GAMEEVENT( smokegrenade_expired );
	ADD_GAMEEVENT( molotov_detonate );
	ADD_GAMEEVENT( decoy_detonate );
	ADD_GAMEEVENT( decoy_started );
	ADD_GAMEEVENT( tagrenade_detonate );
	ADD_GAMEEVENT( inferno_startburn );
	ADD_GAMEEVENT( inferno_expire );
	ADD_GAMEEVENT( inferno_extinguish );
	ADD_GAMEEVENT( decoy_firing );
	ADD_GAMEEVENT( player_footstep );
	ADD_GAMEEVENT( player_jump );
	ADD_GAMEEVENT( player_blind );
	ADD_GAMEEVENT( player_falldamage );
	ADD_GAMEEVENT( door_moving );
	ADD_GAMEEVENT( round_freeze_end );
	ADD_GAMEEVENT( mb_input_lock_success );
	ADD_GAMEEVENT( mb_input_lock_cancel );
	ADD_GAMEEVENT( nav_blocked );
	ADD_GAMEEVENT( nav_generate );
	ADD_GAMEEVENT( player_stats_updated );
	ADD_GAMEEVENT( achievement_info_loaded );
	ADD_GAMEEVENT( spec_target_updated );
	ADD_GAMEEVENT( spec_mode_updated );
	ADD_GAMEEVENT( cs_game_disconnected );
	ADD_GAMEEVENT( cs_win_panel_round );
	ADD_GAMEEVENT( cs_win_panel_match );
	ADD_GAMEEVENT( cs_match_end_restart );
	ADD_GAMEEVENT( cs_pre_restart );
	ADD_GAMEEVENT( show_freezepanel );
	ADD_GAMEEVENT( hide_freezepanel );
	ADD_GAMEEVENT( freezecam_started );
	ADD_GAMEEVENT( player_avenged_teammate );
	ADD_GAMEEVENT( achievement_earned );
	ADD_GAMEEVENT( achievement_earned_local );
	ADD_GAMEEVENT( item_found );
	ADD_GAMEEVENT( items_gifted );
	ADD_GAMEEVENT( repost_xbox_achievements );
	ADD_GAMEEVENT( match_end_conditions );
	ADD_GAMEEVENT( round_mvp );
	ADD_GAMEEVENT( player_decal );
	ADD_GAMEEVENT( teamplay_round_start );
	ADD_GAMEEVENT( client_disconnect );
	ADD_GAMEEVENT( gg_player_levelup );
	ADD_GAMEEVENT( ggtr_player_levelup );
	ADD_GAMEEVENT( assassination_target_killed );
	ADD_GAMEEVENT( ggprogressive_player_levelup );
	ADD_GAMEEVENT( gg_killed_enemy );
	ADD_GAMEEVENT( gg_final_weapon_achieved );
	ADD_GAMEEVENT( gg_bonus_grenade_achieved );
	ADD_GAMEEVENT( switch_team );
	ADD_GAMEEVENT( gg_leader );
	ADD_GAMEEVENT( gg_team_leader );
	ADD_GAMEEVENT( gg_player_impending_upgrade );
	ADD_GAMEEVENT( write_profile_data );
	ADD_GAMEEVENT( trial_time_expired );
	ADD_GAMEEVENT( update_matchmaking_stats );
	ADD_GAMEEVENT( player_reset_vote );
	ADD_GAMEEVENT( enable_restart_voting );
	ADD_GAMEEVENT( sfuievent );
	ADD_GAMEEVENT( start_vote );
	ADD_GAMEEVENT( player_given_c4 );
	//ADD_GAMEEVENT( player_become_ghost );
	ADD_GAMEEVENT( gg_reset_round_start_sounds );
	ADD_GAMEEVENT( tr_player_flashbanged );
	//ADD_GAMEEVENT( tr_highlight_ammo );
	ADD_GAMEEVENT( tr_mark_complete );
	ADD_GAMEEVENT( tr_mark_best_time );
	ADD_GAMEEVENT( tr_exit_hint_trigger );
	ADD_GAMEEVENT( bot_takeover );
	ADD_GAMEEVENT( tr_show_finish_msgbox );
	ADD_GAMEEVENT( tr_show_exit_msgbox );
	ADD_GAMEEVENT( reset_player_controls );
	ADD_GAMEEVENT( jointeam_failed );
	ADD_GAMEEVENT( teamchange_pending );
	ADD_GAMEEVENT( material_default_complete );
	ADD_GAMEEVENT( cs_prev_next_spectator );
	ADD_GAMEEVENT( cs_handle_ime_event );
	ADD_GAMEEVENT( nextlevel_changed );
	ADD_GAMEEVENT( seasoncoin_levelup );
	ADD_GAMEEVENT( tournament_reward );
	ADD_GAMEEVENT( start_halftime );
	//ADD_GAMEEVENT( ammo_refill );
	//ADD_GAMEEVENT( parachute_pickup );
	//ADD_GAMEEVENT( parachute_deploy );
	//ADD_GAMEEVENT( dronegun_attack );
	//ADD_GAMEEVENT( drone_dispatched );
	//ADD_GAMEEVENT( loot_crate_visible );
	//ADD_GAMEEVENT( loot_crate_opened );
	//ADD_GAMEEVENT( open_crate_instr );
	//ADD_GAMEEVENT( smoke_beacon_paradrop );
	//ADD_GAMEEVENT( drone_cargo_detached );
	//ADD_GAMEEVENT( choppers_incoming_warning );
	//ADD_GAMEEVENT( firstbombs_incoming_warning );
	//ADD_GAMEEVENT( dz_item_interaction );
	//ADD_GAMEEVENT( snowball_hit_player_face );
}

void GameEvent::Shutdown( ) {
	g_pGameEvent->RemoveListener( this );
}

void GameEvent::FireGameEvent( IGameEvent *pEvent ) {
	if( !pEvent )
		return;

	auto event_hash = hash_32_fnv1a( pEvent->GetName( ) );

	g_Vars.globals.szLastHookCalled = XorStr( "g_e" );

#if defined(LUA_SCRIPTING)
	if( !g_pEngine->IsDrawingLoadingImage( ) && event_hash ) {
		Scripting::Script::DoCallback( event_hash, pEvent );
	}
#endif

	g_ShotHandling.GameEvent( pEvent );
	g_Hitmarker.GameEvent( pEvent );

	C_CSPlayer *pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !g_pEngine->IsInGame( ) )
		return;

	switch( event_hash ) {
		case hash_32_fnv1a_const( "weapon_fire" ):
		{
			if( !pLocal )
				return;

			auto enemy = pEvent->GetInt( XorStr( "userid" ) );
			auto enemy_index = g_pEngine->GetPlayerForUserID( enemy );
			if( enemy_index == pLocal->EntIndex( ) ) {
				g_Vars.globals.m_vecLastShotEyePosition = pLocal->GetEyePosition( );
			}
			break;
		}
		case hash_32_fnv1a_const( "bullet_impact" ):
		{
			if( !pLocal )
				return;

			const auto userid = pEvent->GetInt( XorStr( "userid" ) );
			const auto user_index = g_pEngine->GetPlayerForUserID( userid );

			const auto vecImpact = Vector( pEvent->GetFloat( XorStr( "x" ) ), pEvent->GetFloat( XorStr( "y" ) ), pEvent->GetFloat( XorStr( "z" ) ) );

			if( user_index == g_pEngine->GetLocalPlayer( ) ) {
				//printf( "\tarrived on server tick: %i [server]\n", g_pClientState->m_ClockDriftManager( ).m_nServerTick );

				if( g_Vars.esp.server_impacts ) {
					g_pDebugOverlay->AddBoxOverlay( vecImpact, { -1.5f, -1.5f, -1.5f }, { 1.5f, 1.5f, 1.5f }, { }, g_Vars.esp.server_impacts_color.r * 255, g_Vars.esp.server_impacts_color.g * 255, g_Vars.esp.server_impacts_color.b * 255, g_Vars.esp.server_impacts_color.a * 255, g_Vars.sv_showimpacts_time->GetFloat( ) );
				}

				if( g_Vars.esp.bullet_tracer_local ) {
					g_BeamEffects.Add( { g_pGlobalVars->realtime, g_Vars.globals.m_vecLastShotEyePosition, vecImpact, g_Vars.esp.bullet_tracer_local_color.ToRegularColor( ), pLocal->EntIndex( ), pLocal->m_nTickBase( ), false } );
				}
			}
			else {
				const auto player = C_CSPlayer::GetPlayerByIndex( user_index );
				if( player && !player->IsDormant( ) && !player->IsTeammate( pLocal ) ) {
					//g_AntiAim.m_pIntersectPlayer = player;
					//g_AntiAim.m_vecIntersectImpact.emplace_back( vecImpact );

					if( g_Vars.esp.bullet_tracer_enemy ) {
						g_BeamEffects.Add( { g_pGlobalVars->realtime,player->GetEyePosition( false, true ), vecImpact, g_Vars.esp.bullet_tracer_enemy_color.ToRegularColor( ), player->EntIndex( ), player->m_nTickBase( ), false } );
					}
				}
			}

			break;
		}
		case hash_32_fnv1a_const( "round_start" ):
		{
			// hehe. :)
			//g_protection.check_round( );

			g_Vars.globals.m_bGameOver = false;

			g_AutomaticPurchase.GameEvent( );

			/*if( !g_AntiAim.m_vecIntersectImpact.empty( ) ) {
				g_AntiAim.m_vecIntersectImpact.clear( );
				g_AntiAim.m_pIntersectPlayer = nullptr;
				g_AntiAim.m_bForceInvert = false;
			}*/

			for( int i = 1; i <= 64; ++i ) {
				g_Vars.globals.m_bPlantingC4[ i ] = false;

				g_Resolver.m_arrResolverData.at( i ).Reset( );

				g_Visuals.player_fading_alpha.at( i ) = 0.f;
				g_ExtendedVisuals.m_arrOverridePlayers.at( i ).m_eOverrideType = EOverrideType::ESP_NONE;

				g_Ragebot.m_arrNapUsers.at( i ).first = false;
				g_Ragebot.m_arrNapUsers.at( i ).second = "";

				const auto player = C_CSPlayer::GetPlayerByIndex( i );
				if( !player )
					continue;

				if( player->m_iTeamNum( ) != TEAM_CT && player->m_iTeamNum( ) != TEAM_TT )
					continue;

				if( player->IsTeammate( pLocal ) )
					continue;

				if( player->IsDead( ) )
					continue;

				// reset health...
				player->m_iHealth( ) = 100;
			}

			break;
		}
		case hash_32_fnv1a_const( "round_poststart" ):
		{

			for( int i = 1; i <= g_pGlobalVars->maxClients; ++i ) {
				g_Vars.globals.m_bPlantingC4[ i ] = false;

				g_Resolver.m_arrResolverData.at( i ).Reset( );

				g_Visuals.player_fading_alpha.at( i ) = 0.f;
				g_ExtendedVisuals.m_arrOverridePlayers.at( i ).m_eOverrideType = EOverrideType::ESP_NONE;

				const auto player = C_CSPlayer::GetPlayerByIndex( i );
				if( !player )
					continue;

				if( player->m_iTeamNum( ) != TEAM_CT && player->m_iTeamNum( ) != TEAM_TT )
					continue;

				if( player->IsTeammate( pLocal ) )
					continue;

				if( player->IsDead( ) )
					continue;

				// reset health...
				player->m_iHealth( ) = 100;
			}

			break;
		}
		case hash_32_fnv1a_const( "player_spawn" ):
		{
			auto enemy = pEvent->GetInt( XorStr( "userid" ) );
			auto enemy_index = g_pEngine->GetPlayerForUserID( enemy );

			/*if( enemy_index && enemy_index <= 64 )
				g_Chams.m_bDrewDeadChams[ enemy_index ] = false;*/

			if( enemy_index == g_pEngine->GetLocalPlayer( ) ) {
				g_Movement.m_vecAutoPeekPos = Vector( );
				g_AutomaticPurchase.GameEvent( );

				g_Vars.globals.m_bGameOver = false;

				// Скинчейнджер high-ping: один cl_fullupdate при спавне, чтобы
				// сервер переслал все DataTable и скины легли корректно.
				g_SkinChanger.OnJoinServer( );
			}

			g_Visuals.vecAimpoints[ enemy_index ].clear( );
			g_Visuals.vecAimpointsSane[ enemy_index ].clear( );

			break;
		}
		case hash_32_fnv1a_const( "game_start" ):
		{
			// Подключение к серверу: форсим полную пересылку таблиц один раз.
			g_SkinChanger.OnJoinServer( );
			break;
		}
		case hash_32_fnv1a_const( "cs_intermission" ):
		{
			g_Vars.globals.m_bGameOver = true;

			break;
		}
		case hash_32_fnv1a_const( "player_spawned" ): // idk
		{
			auto enemy = pEvent->GetInt( XorStr( "userid" ) );
			auto enemy_index = g_pEngine->GetPlayerForUserID( enemy );

			/*if( enemy_index && enemy_index <= 64 )
				g_Chams.m_bDrewDeadChams[ enemy_index ] = false;*/

			g_Visuals.vecAimpoints[ enemy_index ].clear( );
			g_Visuals.vecAimpointsSane[ enemy_index ].clear( );

			break;
		}
		case hash_32_fnv1a_const( "player_death" ):
		{

			auto enemy = pEvent->GetInt( XorStr( "userid" ) );
			auto attacker = pEvent->GetInt( XorStr( "attacker" ) );
			auto enemy_index = g_pEngine->GetPlayerForUserID( enemy );
			auto attacker_index = g_pEngine->GetPlayerForUserID( attacker );

			for( int i = 1; i <= 64; ++i ) {
				if( i == enemy_index ) {
					g_Vars.globals.m_bPlantingC4[ i ] = false;

					g_Resolver.m_arrResolverData.at( i ).Reset( );
				}
			}

			g_ExtendedVisuals.m_arrSoundPlayers.at( enemy_index ).m_flLastNonDormantTime = 0.f;

			g_Visuals.vecAimpoints[ enemy_index ].clear( );
			g_Visuals.vecAimpointsSane[ enemy_index ].clear( );

			if( pLocal && enemy_index != pLocal->EntIndex( ) && attacker_index == pLocal->EntIndex( ) ) {
				const std::string weaponName = pEvent->GetString( XorStr( "weapon" ) );
				if( weaponName.find( XorStr( "knife" ) ) != std::string::npos || weaponName.find( XorStr( "bayonet" ) ) != std::string::npos ) {
					if( g_SkinChanger.m_icon_overrides.find( weaponName ) == g_SkinChanger.m_icon_overrides.end( ) )
						break;

					const auto overrideName = g_SkinChanger.m_icon_overrides.at( weaponName );
					if( !overrideName.empty( ) ) {
						pEvent->SetString( XorStr( "weapon" ), overrideName.data( ) );
					}
				}
			}

			break;
		}
		case hash_32_fnv1a_const( "item_equip" ):
		{
			if( !g_Vars.visuals_enemy.dormant )
				return;

			auto enemy = pEvent->GetInt( XorStr( "userid" ) );
			auto enemy_index = g_pEngine->GetPlayerForUserID( enemy );

			if( enemy_index == g_pEngine->GetLocalPlayer( ) )
				return;

			if( enemy_index < 0 || ( enemy_index && enemy_index > 64 ) )
				return;

			auto wep_type = pEvent->GetInt( XorStr( "weptype" ) );
			auto weap = pEvent->GetInt( XorStr( "defindex" ) );

			const auto pNewWeaponData = g_pWeaponSystem->GetWeaponInfo( weap );

			g_ExtendedVisuals.m_arrWeaponInfos[ enemy_index ].first = weap;
			g_ExtendedVisuals.m_arrWeaponInfos[ enemy_index ].second = pNewWeaponData ? pNewWeaponData : nullptr;

			// this is fine, but sometimes pWeapon will be nullptr, therefore it won't render the weapon
			//auto pWeapon = reinterpret_cast< C_WeaponCSBaseGun* >( player->m_hActiveWeapon( ).Get( ) );
			//if( pWeapon ) {
			//	//pWeapon->m_iItemDefinitionIndex( ) = weap;

			//	// idk what other shit to save
			//	/*if( pNewWeaponData && pWeapon->GetCSWeaponData( ).IsValid( ) ) {
			//		pWeapon->GetCSWeaponData( )->m_iMaxClip = pNewWeaponData->m_iMaxClip;
			//		pWeapon->GetCSWeaponData( )->m_iWeaponType = wep_type; //pNewWeaponData->m_iWeaponType;
			//		pWeapon->GetCSWeaponData( )->m_szHudName = pNewWeaponData->m_szHudName; // lol idk
			//		pWeapon->GetCSWeaponData( )->m_szWeaponName = pNewWeaponData->m_szWeaponName; // lol idk
			//	}*/
			//}
			break;
		}
		case hash_32_fnv1a_const( "item_purchase" ):
		{
			if( !g_Vars.misc.event_buy )
				return;

			std::string item = pEvent->GetString( XorStr( "weapon" ) );
			if( item == XorStr( "weapon_unknown" ) )
				return;

			if( item.find( "weapon_" ) != std::string::npos )
				item.replace( 0, 7, "" );
			else if( item.find( "item_" ) != std::string::npos )
				item.replace( 0, 5, "" );

			auto enemy = pEvent->GetInt( XorStr( "userid" ) );
			auto enemy_index = g_pEngine->GetPlayerForUserID( enemy );

			if( enemy_index < 0 || ( enemy_index && enemy_index > 64 ) )
				return;

			auto player = C_CSPlayer::GetPlayerByIndex( enemy_index );

			if( !player || player->IsTeammate( pLocal ) || enemy_index == g_pEngine->GetLocalPlayer( ) )
				return;

			player_info_t info;
			if( !g_pEngine->GetPlayerInfo( enemy_index, &info ) )
				return;

			std::string playerName{ info.szName };

			if( playerName.find( XorStr( "\n" ) ) != std::string::npos ) {
				playerName = XorStr( "[BLANK]" );
			}

			char damage_buffer[ 512 ] = {};
			sprintf( damage_buffer, XorStr( "%s bought %s\n" ),
					 playerName.data( ),
					 item.data( ) );

			// std::to_string( it->m_ShotData.m_iShotID )
			g_EventLog.PushEvent( damage_buffer, Color_f( 255, 255, 255 ), true, "", false );

			g_pCVar->ConsoleColorPrintf( Color( 200, 255, 0, 255 ), XorStr( "[ PURCHASE ] " ) );
			g_pCVar->ConsoleColorPrintf( Color( 255, 229, 204, 255 ), XorStr( "%s " ), playerName.data( ) );
			g_pCVar->ConsoleColorPrintf( Color( 255, 255, 255, 255 ), XorStr( "bought " ) );
			g_pCVar->ConsoleColorPrintf( Color( 173, 216, 230, 255 ), XorStr( "%s\n" ), item.data( ) );
			break;
		}
		case hash_32_fnv1a_const( "player_blind" ):
		{
			if( !pLocal )
				return;

			auto enemy = pEvent->GetInt( XorStr( "userid" ) );
			auto attacker = pEvent->GetInt( XorStr( "attacker" ) );

			auto enemy_index = g_pEngine->GetPlayerForUserID( enemy );
			auto attacker_index = g_pEngine->GetPlayerForUserID( attacker );

			if( enemy_index < 0 || ( enemy_index && enemy_index > 64 ) )
				return;

			auto player = C_CSPlayer::GetPlayerByIndex( enemy_index );

			if( !player || player->IsTeammate( pLocal ) || enemy_index == g_pEngine->GetLocalPlayer( ) )
				return;

			if( enemy_index == pLocal->EntIndex( ) || attacker_index != pLocal->EntIndex( ) )
				return;

			if( g_Vars.esp.hitsound_type > 0 ) {

				uint32_t hitsound = 0;

				switch( g_Vars.esp.hitsound_type ) {
					case 1:
						hitsound = BASS::stream_sounds.fatality_hitsound;
						break;
					case 2:
						g_pEngineSound->EmitAmbientSound( XorStr( "buttons/arena_switch_press_02.wav" ), ( g_Vars.esp.hitsound_volume / 100.f ) );
						break;
					case 3:
						hitsound = BASS::stream_sounds.bubble_hitsound;
						break;
				}

				if( hitsound != 0 ) {
					BASS_SET_VOLUME( hitsound, ( g_Vars.esp.hitsound_volume / 100.f ) );
					BASS_ChannelPlay( hitsound, true );
				}

				if( g_Vars.esp.hitsound_type == 4 && !g_Vars.globals.m_vecHitsounds.empty( ) ) {
					// DIRECTORY XD!!!
					int idx = g_Vars.esp.hitsound_custom;
					if( idx >= g_Vars.globals.m_vecHitsounds.size( ) )
						idx = g_Vars.globals.m_vecHitsounds.size( ) - 1;
					else if( idx < 0 )
						idx = 0;

					std::string curfile = g_Vars.globals.m_vecHitsounds[ idx ];
					if( !curfile.empty( ) ) {
						std::string dir = GetHitsoundsDirectory( ).append( curfile );

						auto ReadWavFileIntoMemory = [ & ] ( std::string fname, BYTE **pb, DWORD *fsize ) {
							std::ifstream f( fname, std::ios::binary );

							f.seekg( 0, std::ios::end );
							int lim = f.tellg( );
							*fsize = lim;

							*pb = new BYTE[ lim ];
							f.seekg( 0, std::ios::beg );

							f.read( ( char * )*pb, lim );

							f.close( );
						};

						DWORD dwFileSize;
						BYTE *pFileBytes;
						ReadWavFileIntoMemory( dir.data( ), &pFileBytes, &dwFileSize );

						// danke anarh1st47, ich liebe dich
						// dieses code snippet hat mir so sehr geholfen https://i.imgur.com/ybWTY2o.png
						// thanks anarh1st47, you are the greatest
						// loveeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee
						// kochamy anarh1st47
						auto modify_volume = [ & ] ( BYTE *bytes ) {
							int offset = 0;
							for( int i = 0; i < dwFileSize / 2; i++ ) {
								if( bytes[ i ] == 'd' && bytes[ i + 1 ] == 'a'
									&& bytes[ i + 2 ] == 't' && bytes[ i + 3 ] == 'a' ) {
									offset = i;
									break;
								}
							}

							if( !offset )
								return;

							BYTE *pDataOffset = ( bytes + offset );
							DWORD dwNumSampleBytes = *( DWORD * )( pDataOffset + 4 );
							DWORD dwNumSamples = dwNumSampleBytes / 2;

							SHORT *pSample = ( SHORT * )( pDataOffset + 8 );
							for( DWORD dwIndex = 0; dwIndex < dwNumSamples; dwIndex++ ) {
								SHORT shSample = *pSample;
								shSample = ( SHORT )( shSample * ( g_Vars.esp.hitsound_volume / 100.f ) );
								*pSample = shSample;
								pSample++;
								if( ( ( BYTE * )pSample ) >= ( bytes + dwFileSize - 1 ) )
									break;
							}
						};

						if( pFileBytes ) {
							modify_volume( pFileBytes );
							PlaySoundA( ( LPCSTR )pFileBytes, NULL, SND_MEMORY | SND_ASYNC );
						}
					}
				}
			}
			break;
		}
		case hash_32_fnv1a_const( "player_hurt" ):
		{
			if( !pLocal )
				return;

			auto enemy = pEvent->GetInt( XorStr( "userid" ) );
			auto attacker = pEvent->GetInt( XorStr( "attacker" ) );
			auto remaining_health = pEvent->GetString( XorStr( "health" ) );
			auto dmg_to_health = pEvent->GetInt( XorStr( "dmg_health" ) );
			auto hitgroup = pEvent->GetInt( XorStr( "hitgroup" ) );

			auto enemy_index = g_pEngine->GetPlayerForUserID( enemy );
			auto attacker_index = g_pEngine->GetPlayerForUserID( attacker );

			if( enemy_index < 0 || ( enemy_index && enemy_index > 64 ) )
				return;

			// printf( "player_hurt time bro: %.4f\n", g_pGlobalVars->curtime );

			ResolverData_t::PlayerHurtInfo_t hurtInfo;
			hurtInfo.m_nHitgroup = hitgroup;
			hurtInfo.m_flTime = g_pGlobalVars->curtime;
			g_Resolver.m_arrResolverData.at( enemy_index ).m_vecPlayerHurtInfo.push_back( hurtInfo );
		ResolverData_t::TrimOld( g_Resolver.m_arrResolverData.at( enemy_index ).m_vecPlayerHurtInfo, 32 );

			auto player = C_CSPlayer::GetPlayerByIndex( enemy_index );

			//if( auto data = g_AnimationSystem.GetAnimationData( enemy_index ); data ) {
			//	data->m_flSimulationTimeAtTimeOfHit = data->m_deqRecords.size( ) ? data->m_deqRecords.front( ).m_flSimulationTime : player->m_flSimulationTime( );
			//	data->m_nHitgroupAtTimeOfHit = hitgroup;
			//}

			if( g_Vars.visuals_enemy.dormant ) {
				if( player ) {
					if( player->IsDormant( ) && !player->IsTeammate( pLocal ) ) {
						//	g_Visuals.animated_health.at( enemy_index ) = GUI::Approach( g_Visuals.animated_health.at( enemy_index ), float( std::stoi( remaining_health ) ) + 0.1f, g_pGlobalVars->frametime * 10.f );
						//	g_Visuals.animated_health.at( enemy_index ) = std::clamp( g_Visuals.animated_health.at( enemy_index ), 0.f, 100.f );


						player->m_iHealth( ) = std::stoi( remaining_health );
					}
				}
			}

			player_info_t attacker_info;
			player_info_t enemy_info;

			if( g_pEngine->GetPlayerInfo( attacker_index, &attacker_info ) && g_pEngine->GetPlayerInfo( enemy_index, &enemy_info ) ) {
				if( !pLocal )
					return;

				if( attacker_index != g_pEngine->GetLocalPlayer( ) ) {
					if( enemy_index == pLocal->m_entIndex ) {
						if( g_Vars.misc.event_harm ) {
							std::stringstream msg;

							std::string playerName{ attacker_info.szName };

							if( playerName.find( XorStr( "\n" ) ) != std::string::npos ) {
								playerName = XorStr( "[BLANK]" );
							}

							auto damage = dmg_to_health;
							damage = std::clamp( damage, 0, 100 );
							const int green = 255 - damage * 2.55;
							const int red = damage * 2.55;

							char damage_buffer[ 512 ] = {};
							sprintf( damage_buffer, XorStr( "-%d in %s by %s\n" ),
									 dmg_to_health,
									 HitgroupToName__( hitgroup ).data( ),
									 playerName.data( ) );

							// std::to_string( it->m_ShotData.m_iShotID )
							g_EventLog.PushEvent( damage_buffer, Color_f( 255, 255, 255 ), true, "", false );

							g_pCVar->ConsoleColorPrintf( Color( 255, 255, 154, 255 ), XorStr( "[ HURT ] " ) );
							g_pCVar->ConsoleColorPrintf( Color( red, green, 0, 255 ), XorStr( "-%d " ), dmg_to_health );
							g_pCVar->ConsoleColorPrintf( Color( 255, 255, 255, 255 ), XorStr( "in " ) );
							g_pCVar->ConsoleColorPrintf( Color( 255, 204, 204, 255 ), XorStr( "%s " ), HitgroupToName__( hitgroup ).data( ) );
							g_pCVar->ConsoleColorPrintf( Color( 255, 255, 255, 255 ), XorStr( "by " ) );
							g_pCVar->ConsoleColorPrintf( Color( 255, 229, 204, 255 ), XorStr( "%s\n" ), playerName.data( ) );
						}

					}
				}
				else {
					if( g_Vars.esp.hitsound_type > 0 ) {

						uint32_t hitsound = 0;

						switch( g_Vars.esp.hitsound_type ) {
							case 1:
								hitsound = BASS::stream_sounds.fatality_hitsound;
								break;
							case 2:
								g_pEngineSound->EmitAmbientSound( XorStr( "buttons/arena_switch_press_02.wav" ), ( g_Vars.esp.hitsound_volume / 100.f ) );
								break;
							case 3:
								hitsound = BASS::stream_sounds.bubble_hitsound;
								break;
						}

						if( hitsound != 0 ) {
							BASS_SET_VOLUME( hitsound, ( g_Vars.esp.hitsound_volume / 100.f ) );
							BASS_ChannelPlay( hitsound, true );
						}

						if( g_Vars.esp.hitsound_type == 4 && !g_Vars.globals.m_vecHitsounds.empty( ) ) {
							// DIRECTORY XD!!!
							int idx = g_Vars.esp.hitsound_custom;
							if( idx >= g_Vars.globals.m_vecHitsounds.size( ) )
								idx = g_Vars.globals.m_vecHitsounds.size( ) - 1;
							else if( idx < 0 )
								idx = 0;

							std::string curfile = g_Vars.globals.m_vecHitsounds[ idx ];
							if( !curfile.empty( ) ) {
								std::string dir = GetHitsoundsDirectory( ).append( curfile );

								auto ReadWavFileIntoMemory = [ & ] ( std::string fname, BYTE **pb, DWORD *fsize ) {
									std::ifstream f( fname, std::ios::binary );

									f.seekg( 0, std::ios::end );
									int lim = f.tellg( );
									*fsize = lim;

									*pb = new BYTE[ lim ];
									f.seekg( 0, std::ios::beg );

									f.read( ( char * )*pb, lim );

									f.close( );
								};

								DWORD dwFileSize;
								BYTE *pFileBytes;
								ReadWavFileIntoMemory( dir.data( ), &pFileBytes, &dwFileSize );

								// danke anarh1st47, ich liebe dich
								// dieses code snippet hat mir so sehr geholfen https://i.imgur.com/ybWTY2o.png
								// thanks anarh1st47, you are the greatest
								// loveeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee
								// kochamy anarh1st47
								auto modify_volume = [ & ] ( BYTE *bytes ) {
									int offset = 0;
									for( int i = 0; i < dwFileSize / 2; i++ ) {
										if( bytes[ i ] == 'd' && bytes[ i + 1 ] == 'a'
											&& bytes[ i + 2 ] == 't' && bytes[ i + 3 ] == 'a' ) {
											offset = i;
											break;
										}
									}

									if( !offset )
										return;

									BYTE *pDataOffset = ( bytes + offset );
									DWORD dwNumSampleBytes = *( DWORD * )( pDataOffset + 4 );
									DWORD dwNumSamples = dwNumSampleBytes / 2;

									SHORT *pSample = ( SHORT * )( pDataOffset + 8 );
									for( DWORD dwIndex = 0; dwIndex < dwNumSamples; dwIndex++ ) {
										SHORT shSample = *pSample;
										shSample = ( SHORT )( shSample * ( g_Vars.esp.hitsound_volume / 100.f ) );
										*pSample = shSample;
										pSample++;
										if( ( ( BYTE * )pSample ) >= ( bytes + dwFileSize - 1 ) )
											break;
									}
								};

								if( pFileBytes ) {
									modify_volume( pFileBytes );
									PlaySoundA( ( LPCSTR )pFileBytes, NULL, SND_MEMORY | SND_ASYNC );
								}
							}
						}
					}
				}
			}
			break;
		}
		case hash_32_fnv1a_const( "grenade_thrown" ):
		{
			const int nThrowerIndex = g_pEngine->GetPlayerForUserID( pEvent->GetInt( XorStr( "userid" ) ) );

			const float flLatency = g_pEngine->GetNetChannelInfo( ) ? g_pEngine->GetNetChannelInfo( )->GetLatency( FLOW_INCOMING ) : 0.f;

			// it seems to be perfect now, just still not quite sure if we should account latency or not..
			const float flSpawnTime = TICKS_TO_TIME( g_pGlobalVars->tickcount ) - flLatency;

			g_GrenadeWarning.m_vecEventNades.push_back( { nThrowerIndex, flSpawnTime, 0 } );

			break;
		}
		case hash_32_fnv1a_const( "bomb_beginplant" ):
		{
			auto enemy = pEvent->GetInt( XorStr( "userid" ) );

			auto enemy_index = g_pEngine->GetPlayerForUserID( enemy );

			if( enemy_index < 0 || ( enemy_index && enemy_index > 64 ) )
				return;

			g_Vars.globals.m_bPlantingC4[ enemy_index ] = true;
			break;
		}
		case hash_32_fnv1a_const( "bomb_abortplant" ):
		{
			auto enemy = pEvent->GetInt( XorStr( "userid" ) );

			auto enemy_index = g_pEngine->GetPlayerForUserID( enemy );

			if( enemy_index < 0 || ( enemy_index && enemy_index > 64 ) )
				return;

			g_Vars.globals.m_bPlantingC4[ enemy_index ] = false;
			break;
		}
		case hash_32_fnv1a_const( "bomb_planted" ):
		case hash_32_fnv1a_const( "bomb_defused" ):
		case hash_32_fnv1a_const( "bomb_exploded" ):
		case hash_32_fnv1a_const( "bomb_dropped" ):
		case hash_32_fnv1a_const( "bomb_pickup" ):
		{
			for( int i = 1; i <= 64; ++i )
				g_Vars.globals.m_bPlantingC4[ i ] = false;

			break;
		}
		case hash_32_fnv1a_const( "player_connect_full" ):
		{
			// Вход на сервер: cl_fullupdate один раз, даже если ignore_radio выкл.
			{
				auto m_pEntityJoin = pEvent->GetInt( XorStr( "userid" ) );
				if( m_pEntityJoin ) {
					auto join_idx = g_pEngine->GetPlayerForUserID( m_pEntityJoin );
					if( join_idx == g_pEngine->GetLocalPlayer( ) )
						g_SkinChanger.OnJoinServer( );
				}
			}

			if( !pLocal )
				return;

			if( !g_Vars.misc.ignore_radio )
				return;

			auto m_pEntity = pEvent->GetInt( XorStr( "userid" ) );
			if( !m_pEntity )
				return;

			auto player_index = g_pEngine->GetPlayerForUserID( m_pEntity );
			if( player_index < 0 || ( player_index && player_index > 64 ) )
				return;

			auto local_index = pLocal->EntIndex( );
			if( local_index < 0 || ( local_index && local_index > 64 ) )
				return;
			
			if( player_index == pLocal->EntIndex( ) )
				g_pEngine->ClientCmd_Unrestricted( "ignorerad" );

			break;
		}
	}

}

int GameEvent::GetEventDebugID( void ) {
	return 42;
}
