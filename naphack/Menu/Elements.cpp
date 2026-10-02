#include <filesystem>
#include "../Features/Scripting/Scripting.hpp"
#include "Elements.h"
#include "Framework/GUI.h"
#include "../Features/Miscellaneous/KitParser.hpp"
#include "../Utils/Config.hpp"
#include "../Features/Visuals/EventLogger.hpp"
#include "../pandora.hpp"
#include "../Features/Visuals/Visuals.hpp"
#include "../Features/Miscellaneous/PlayerList.hpp"
#include "../Utils/base64.h"
#include "../Features/Rage/Ragebot.hpp"

void testingg( std::function<std::string( float testtest )> adasd ) {
	auto asdas1 = adasd( 1.f );
	auto asdas2 = adasd( 2.f );

	//printf( "%s %s\n", asdas1.data( ), asdas2.data( ) );
}

bool on_cfg_load_gloves, on_cfg_load_knives;

#undef MENU_DEV

namespace Menu {

	void DrawMenu( ) {
		//testingg( [ & ] ( float bruh ) { return bruh == 1.f ? "one" : "two"; } );

	#if defined(LUA_SCRIPTING)

		GUI::ctx->allow_tooltip = false;
		// let's make sure everything is ready first..
		if( GUI::ctx->setup ) {
			Scripting::init( );
		}
	#endif

		if( GUI::Form::BeginWindow( XorStr( "blootoothYzio" ) ) || GUI::ctx->setup ) {
		#ifdef MENU_DEV

			if( GUI::Form::BeginTab( "aimbot", "rage" ) ) {

				GUI::Group::BeginGroup( "aimbot", Vector2D( 50, 100 ) );
				{
					static bool checkbox1, checkbox2;
					static bool checkbox3, checkbox4;
					static bool checkbox5, checkbox6;
					static bool aaaa = true, bbbb;
					static bool cccc = true, dddd, eeee = true;
					static int list;
					static int drop;
					static int slider1;
					static float slider2;
					static Color_f color1, color2, color3;
					static hotkey_t hotkeyl, hotkey2;
					GUI::Controls::Checkbox( "checkbox 1", &checkbox1 );
					GUI::Controls::Checkbox( "checkbox 2", &checkbox2 );
					GUI::Controls::Dropdown( "dropdown 1", { "aaaa","bbbb","cccc","dddd","eeee" }, &drop );

					static std::vector<MultiItem_t> multi = {
						{"aaaa", &aaaa },
						{"bbbb", &bbbb },
						{"cccc", &cccc },
						{"dddd", &dddd },
						{"eeee", &eeee },
					};
					GUI::Controls::MultiDropdown( "multi-dropdown 1", multi );
					GUI::Controls::Label( "label 1" );
					GUI::Controls::Checkbox( "checkbox 3", &checkbox3 );
					GUI::Controls::ColorPicker( "colorpicker 1", &color1, true, false, 0 );
					GUI::Controls::ColorPicker( "colorpicker 2", &color2, true, false, 1 );
					GUI::Controls::ColorPicker( "colorpicker 3", &color3, true, true );
					GUI::Controls::Button( "button 1", [&] ( ) { } );
					GUI::Controls::Listbox( "listbox 1", { "aaaa", "bbbb", "cccc", "dddd", "eeee", "ffff", "gggg", "hhhh", "iiii", "jjjj", "kkkk" }, &list, true, 7 );
					GUI::Controls::Slider( "slider 1", &slider1, 0.f, 100.f, "%d%%" );
					GUI::Controls::Slider( "slider 2", &slider2, 0.f, 100.f, "%.1f" );
					GUI::Controls::Checkbox( "checkbox 4", &checkbox4 );
					GUI::Controls::Checkbox( "checkbox 5", &checkbox5 );
					GUI::Controls::Checkbox( "checkbox 6", &checkbox6 );
					GUI::Controls::Hotkey( "hotkey 1", &hotkeyl );
					GUI::Controls::Hotkey( "hotkey 2", &hotkey2, true );

				}
				GUI::Group::EndGroup( );

				GUI::Group::BeginGroup( "triggerbot", Vector2D( 50, 50 ) );
				GUI::Group::EndGroup( );

				GUI::Group::BeginGroup( "nigger", Vector2D( 50, 50 ) );
				GUI::Group::EndGroup( );
			}
			if( GUI::Form::BeginTab( "players", "esp" ) ) {

			}
			if( GUI::Form::BeginTab( "world", "style" ) ) {

			}
			if( GUI::Form::BeginTab( "misc", "etc" ) ) {

			}
			if( GUI::Form::BeginTab( "scripts", "lua" ) ) {

			}
			if( GUI::Form::BeginTab( "home", "as n4pper" ) ) {

			}
		#else
			// unset so we don't fuck up things..
			GUI::ctx->CurrentWeaponGroup = XorStr( "" );
			if( GUI::Form::BeginTab( XorStr( "aimbot" ), XorStr( "rage" ) ) ) {
				if( GUI::Form::BeginSubTab( XorStr( "Ragebot" ) ) ) {
					// :p
					if( !g_Vars.rage_default.override_default_config )
						g_Vars.rage_default.override_default_config = true;

					CVariables::RAGE *rage = nullptr;
					auto RageControls = [&] ( int group ) {
						if( !rage )
							return;

						// main shit
						if( group == 0 ) {
							auto bruh2 = GUI::ctx->enabled;
							GUI::ctx->enabled = bruh2 && rage->override_default_config;

							std::vector<MultiItem_t> hitboxes{
								{ XorStr( "Head" ), &rage->hitbox_head },
								//{ XorStr( "Neck" ), &rage->hitbox_neck },
								{ XorStr( "Upper chest" ), &rage->hitbox_upper_chest },
								{ XorStr( "Chest" ), &rage->hitbox_chest },
								//{ XorStr( "Lower chest" ), &rage->hitbox_lower_chest },
								{ XorStr( "Stomach" ), &rage->hitbox_stomach },
								//{ XorStr( "Pelvis" ), &rage->hitbox_pelvis },
								{ XorStr( "Arms" ), &rage->hitbox_arms },
								{ XorStr( "Legs" ), &rage->hitbox_legs },
								{ XorStr( "Feet" ), &rage->hitbox_feet },
							};

							std::vector<MultiItem_t> multipoints{
								{ XorStr( "Head" ), &rage->multipoint_head },
								{ XorStr( "Upper chest" ), &rage->multipoint_upper_chest },
								{ XorStr( "Chest" ), &rage->multipoint_chest },
								//{ XorStr( "Lower Chest" ), &rage->multipoint_lower_chest },
								{ XorStr( "Stomach" ), &rage->multipoint_stomach },
								{ XorStr( "Arms" ), &rage->multipoint_arms },
								{ XorStr( "Legs" ), &rage->multipoint_legs },
								{ XorStr( "Feet" ), &rage->multipoint_feet },
							};

							GUI::Controls::MultiDropdown( XorStr( "Hitboxes" ), hitboxes );
							GUI::Controls::MultiDropdown( XorStr( "Multipoints" ), multipoints );

							//GUI::Controls::Dropdown( XorStr( "Multipoint mode" ), { XorStr( "Default" ), XorStr( "High" ), XorStr( "Intensive" ) }, &rage->multipoint_style );

							auto GetPsDisplay = [&] ( float ps ) -> std::string {
								return ps == 0.f ? XorStr( "dynamic" ) : XorStr( "%.0f%%" );
							};

							if( rage->multipoint_head || GUI::ctx->setup )
								GUI::Controls::Slider( XorStr( "Headscale" ), &rage->headscale, 0.f, 80.f, GetPsDisplay( rage->headscale ) );

							if( rage->multipoint_chest || rage->multipoint_stomach || rage->multipoint_arms || rage->multipoint_legs || rage->multipoint_feet || GUI::ctx->setup )
								GUI::Controls::Slider( XorStr( "Pointscale" ), &rage->pointscale, 0.f, 80.f, GetPsDisplay( rage->pointscale ) );

							auto GetDmgDisplay = [&] ( int dmg ) -> std::string {
								return dmg > 100 ? ( std::string( XorStr( "hp + " ) ).append( std::string( std::to_string( dmg - 100 ) ) ) ) : std::string( XorStr( "%d" ) );
							};

							GUI::Controls::Slider( XorStr( "minimal damage" ), &rage->minimal_damage, 1, 150, GetDmgDisplay( rage->minimal_damage ), 1 );

							GUI::Controls::Slider( XorStr( "minimal damage override" ), &rage->minimal_damage_override, 1, 150, GetDmgDisplay( rage->minimal_damage_override ) );
							GUI::Controls::Hotkey( XorStr( "minimal damage override key" ), &g_Vars.rage.min_damage_override_key );

							if( rage->minimal_damage <= 100 || GUI::ctx->setup )
								GUI::Controls::Checkbox( XorStr( "scale damage by health" ), &rage->scale_damage_on_hp, { {XorStr( "The cheat will clamp your minimum damage at the player's health" ) }, { XorStr( "clamp your minimum damage" ) } } );

							if( rage == &g_Vars.rage_scout || GUI::ctx->setup ) {
								GUI::Controls::Checkbox( XorStr( "fiske's magical scout" ), &rage->lower_air_minimal_damage );
							}
						}
						else if( group == 1 ) {
							auto bruh2 = GUI::ctx->enabled;
							GUI::ctx->enabled = bruh2 && rage->override_default_config;

							GUI::Controls::Slider( XorStr( "minimum hitchance" ), &rage->hitchance, 0, 100, XorStr( "%d%%" ) );

							GUI::Controls::Checkbox( XorStr( "extended backtrack" ), &g_Vars.misc.extended_backtrack );

							if( GUI::Controls::Checkbox( XorStr( "ping spike" ), &g_Vars.misc.ping_spike ) || GUI::ctx->setup ) {
								GUI::Controls::Hotkey( XorStr( "ping spike key" ), &g_Vars.misc.ping_spike_key );

								const int flMaxSpike = ( int )std::clamp( g_Vars.sv_maxunlag->GetFloat( ) * 1000.f, 0.f, 800.f );
								GUI::Controls::Slider( XorStr( "ping spike amount" ), &g_Vars.misc.ping_spike_amount, 50, flMaxSpike, XorStr( "%dms" ), 1 );
							}

							GUI::Controls::Dropdown( XorStr( "Accuracy boost" ), { XorStr( "Regular" ), XorStr( "High" ), XorStr( "Extreme" ) }, &rage->accuracy_boost );
							GUI::Controls::Dropdown( XorStr( "Fake-lag correction" ), { XorStr( "Off" ), /*XorStr( "predict" ),*/ XorStr( "wait" ) }, &rage->fake_lag_correction );
							//if( rage->fake_lag_correction == 1 || GUI::ctx->setup )
							//	GUI::Controls::Checkbox( XorStr( "prefer linear prediction" ), &g_Vars.rage.linear_lel );

							/*if( rage != &g_Vars.rage_awp && rage != &g_Vars.rage_scout ) {
								std::string hitchanceDisplay = rage->hitchance_dt == 0.f ? XorStr( "Auto" ) : XorStr( "%.0f%%" );
								GUI::Controls::Slider( XorStr( "Double tap hitchance" ), &rage->hitchance_dt, 0.f, 100.f, hitchanceDisplay, 1, { XorStr( "Hitchance of the second shot, when double tap enabled" ), XorStr( "second shot" ) } );
							}*/

							if( GUI::Controls::Dropdown( XorStr( "auto stop" ), { XorStr( "Off" ), XorStr( "slide" ), XorStr( "fakewalk" ) }, &rage->auto_stop ) ) {
								if( rage->auto_stop == 1 || GUI::ctx->setup )
									GUI::Controls::Checkbox( XorStr( "auto stop between shots" ), &rage->auto_stop_between_shots );

								if( rage->auto_stop == 2 || GUI::ctx->setup )
									GUI::Controls::Checkbox( XorStr( "lock fake walk on movement" ), &rage->lock_fakewalk_on_movement );
							}

							if( rage == &g_Vars.rage_default || rage == &g_Vars.rage_autosnipers || rage == &g_Vars.rage_scout || rage == &g_Vars.rage_awp ) {
								GUI::Controls::Checkbox( XorStr( "auto scope" ), &rage->auto_scope );
							}

							std::vector<MultiItem_t> prefer_body{
								{ XorStr( "Always" ), &rage->prefer_body_always },
								{ XorStr( "Lethal" ), &rage->prefer_body_lethal },
								{ XorStr( "Lethal x2" ), &rage->prefer_body_lethal_x2 },
								{ XorStr( "Fake angles" ), &rage->prefer_body_fake },
								{ XorStr( "In air" ), &rage->prefer_body_air },
								//{ XorStr( "Local exploit" ), &rage->prefer_body_exploit },
							};

							std::vector<MultiItem_t> force_body{
								{ XorStr( "Always" ),		&rage->force_body_always },
								{ XorStr( "Health" ),		&rage->force_body_health },
								{ XorStr( "Fake angles" ),	&rage->force_body_fake },
								{ XorStr( "In air" ),		&rage->force_body_air },
								{ XorStr( "On peek" ),		&rage->force_body_peek },
								{ XorStr( "On key" ),		&g_Vars.rage.force_body_aim },
							};

							GUI::Controls::MultiDropdown( XorStr( "Prefer body-aim" ), prefer_body );
							GUI::Controls::MultiDropdown( XorStr( "Force body-aim" ), force_body );

							if( g_Vars.rage.force_body_aim || GUI::ctx->setup )
								GUI::Controls::Hotkey( XorStr( "Force body-aim key" ), &g_Vars.rage.force_body_aim_key );

							if( rage->force_body_health || GUI::ctx->setup )
								GUI::Controls::Slider( XorStr( "Minimum body-aim health" ), &rage->force_body_health_min, 1, 100, XorStr( "%dhp" ) );

							std::vector<MultiItem_t> on_peek_hitboxes{
								{ XorStr( "Chest" ),		&rage->on_peek_chest },
								{ XorStr( "Pelvis" ),	&rage->on_peek_pelvis },
								{ XorStr( "Stomach" ),		&rage->on_peek_stomach }
							};

							if( rage->force_body_peek || GUI::ctx->setup )
								GUI::Controls::MultiDropdown( XorStr( "On peek hitboxes" ), on_peek_hitboxes );
						}
					};

				#pragma region rage_setup 

					if( GUI::ctx->setup ) {
						for( int i = 0; i <= 10; i++ ) {
							switch( i ) {
								case 0:
									rage = &g_Vars.rage_default;
									GUI::ctx->CurrentWeaponGroup = XorStr( "general" );
									break;
								case 1:
									rage = &g_Vars.rage_autosnipers;
									GUI::ctx->CurrentWeaponGroup = XorStr( "auto-sniper" );
									break;
								case 2:
									rage = &g_Vars.rage_scout;
									GUI::ctx->CurrentWeaponGroup = XorStr( "scout" );
									break;
								case 3:
									rage = &g_Vars.rage_awp;
									GUI::ctx->CurrentWeaponGroup = XorStr( "awp" );
									break;
								case 4:
									rage = &g_Vars.rage_revolver;
									GUI::ctx->CurrentWeaponGroup = XorStr( "revolver" );
									break;
								case 5:
									rage = &g_Vars.rage_deagle;
									GUI::ctx->CurrentWeaponGroup = XorStr( "deagle" );
									break;
								case 6:
									rage = &g_Vars.rage_pistols;
									GUI::ctx->CurrentWeaponGroup = XorStr( "pistolss" );
									break;
							}

							GUI::ctx->CurrentGroup = XorStr( "targetting" );

							// General
							RageControls( 0 );

							GUI::ctx->CurrentGroup = XorStr( "accuracy" );

							// Accuracy
							RageControls( 1 );
						}
					}
				#pragma endregion

					// yeah no.
					GUI::ctx->CurrentGroup = XorStr( "" );

					// https://streamable.com/v8rdh2
					static int nLastRageGroup = g_Vars.globals.m_iCurrentRageGroup;
					if( nLastRageGroup != g_Vars.globals.m_iCurrentRageGroup ) {
						GUI::ctx->FocusedID = GUI::ctx->DropdownInfo.HashedID = GUI::ctx->HotkeyInfo.HashedID = GUI::ctx->MultiDropdownInfo.HashedID = 0;
						nLastRageGroup = g_Vars.globals.m_iCurrentRageGroup;
					}

					/*GUI::Group::BeginGroup( XorStr( "rage options" ), Vector2D( 50, 15.5 ) );
					{
						static int asd;
						GUI::Controls::Dropdown( XorStr( "##asdsa" ), { XorStr("show aimbot options" ),XorStr( "show anti-aim options" ) }, &asd );
					}
					GUI::Group::EndGroup( );*/

					// 0 rage, 1 antiaim
					static int nDisplay = 0;

					GUI::Group::BeginGroup( XorStr( "general" ), Vector2D( 50, 35/*30*/ ) );
					{
						if( nDisplay == 0 || GUI::ctx->setup ) {
							GUI::ctx->CurrentWeaponGroup = XorStr( "rage" );

							if( GUI::Controls::Checkbox( XorStr( "Enabled##rage" ), &g_Vars.rage.enabled ) ) {
								GUI::Controls::Hotkey( XorStr( "Enabled key##rage" ), &g_Vars.rage.key );
							}
						}

						if( nDisplay == 1 || GUI::ctx->setup ) {
							GUI::ctx->CurrentWeaponGroup = XorStr( "anti-aim" );

							GUI::Controls::Checkbox( XorStr( "Enabled##aa" ), &g_Vars.rage.anti_aim_active );
						}

						GUI::Controls::Dropdown( XorStr( "##asdsa" ), { XorStr( "show aimbot options" ),XorStr( "show anti-aim options" ) }, &nDisplay );

						if( nDisplay == 0 || GUI::ctx->setup ) {
							GUI::ctx->CurrentWeaponGroup = XorStr( "rage" );

							auto bruh2 = GUI::ctx->enabled;
							GUI::ctx->enabled = g_Vars.rage.enabled;

							GUI::Controls::Checkbox( XorStr( "auto fire" ), &g_Vars.rage.auto_fire );
							GUI::Controls::Checkbox( XorStr( "silent aim" ), &g_Vars.rage.silent_aim );
							GUI::Controls::Checkbox( XorStr( "visual silent aimbot" ), &g_Vars.rage.visual_silent_aim );

							if( GUI::Controls::Checkbox( XorStr( "anti-aim resolver" ), &g_Vars.rage.resolver ) || GUI::ctx->setup ) {
								GUI::Controls::Hotkey( XorStr( "anti-aim resolver override key" ), &g_Vars.rage.resolver_override_key );
								GUI::Controls::Checkbox( XorStr( "use trial resolver" ), &g_Vars.rage.resolver_trial );
								if( g_Vars.rage.resolver_trial || GUI::ctx->setup ) {
									GUI::Controls::Checkbox( XorStr( "deduct bruteforce" ), &g_Vars.rage.deduct_bruteforce );
								}
							}

							if( GUI::Controls::Checkbox( XorStr( "wait for lby flick" ), &g_Vars.rage.wait_for_lby_flick ) || GUI::ctx->setup )
								GUI::Controls::Hotkey( XorStr( "wait for lby flick key" ), &g_Vars.rage.wait_for_lby_flick_key );

							GUI::Controls::Slider( XorStr( "target scan limit" ), &g_Vars.rage.target_limit, 1, 32 );

							GUI::ctx->enabled = bruh2;
						}

						if( nDisplay == 1 || GUI::ctx->setup ) {
							// unset so we don't fuck up things..
							GUI::ctx->CurrentWeaponGroup = XorStr( "anti-aim" );

							auto bruh2 = GUI::ctx->enabled;
							GUI::ctx->enabled = g_Vars.rage.anti_aim_active;

							GUI::Controls::Checkbox( XorStr( "at players" ), &g_Vars.rage.anti_aim_at_players );
							GUI::Controls::Dropdown( XorStr( "base pitch" ), { XorStr( "none" ), XorStr( "down" ), XorStr( "up" ), XorStr( "zero" ) }, &g_Vars.rage.anti_aim_pitch );

							GUI::ctx->enabled = bruh2;
						}

					}
					GUI::Group::EndGroup( );

					// rage tab
					if( nDisplay == 0 || GUI::ctx->setup ) {
						switch( g_Vars.globals.m_iCurrentRageGroup ) {
							case 0:
								rage = &g_Vars.rage_default;
								GUI::ctx->CurrentWeaponGroup = XorStr( "default" );
								break;
							case 1:
								rage = &g_Vars.rage_autosnipers;
								GUI::ctx->CurrentWeaponGroup = XorStr( "auto-sniper" );
								break;
							case 2:
								rage = &g_Vars.rage_scout;
								GUI::ctx->CurrentWeaponGroup = XorStr( "scout" );
								break;
							case 3:
								rage = &g_Vars.rage_awp;
								GUI::ctx->CurrentWeaponGroup = XorStr( "awp" );
								break;
							case 4:
								rage = &g_Vars.rage_revolver;
								GUI::ctx->CurrentWeaponGroup = XorStr( "revolver" );
								break;
							case 5:
								rage = &g_Vars.rage_deagle;
								GUI::ctx->CurrentWeaponGroup = XorStr( "deagle" );
								break;
							case 6:
								rage = &g_Vars.rage_pistols;
								GUI::ctx->CurrentWeaponGroup = XorStr( "pistols" );
								break;
						}


						auto bruh2 = GUI::ctx->enabled;
						GUI::ctx->enabled = g_Vars.rage.enabled;

						GUI::Group::BeginGroup( XorStr( "targetting" ), Vector2D( 50, /*51*/65 ) );
						{
							RageControls( 0 );
						}
						GUI::Group::EndGroup( );

						GUI::ctx->enabled = g_Vars.rage.enabled;

						GUI::Group::BeginGroup( XorStr( "accuracy" ), Vector2D( 50, 78 ) );
						{
							RageControls( 1 );
						}
						GUI::Group::EndGroup( );

						GUI::Group::BeginGroup( XorStr( "weapon config" ), Vector2D( 50, 22 ) );
						{
							auto bruh = GUI::ctx->enabled;
							GUI::ctx->enabled = bruh && rage != &g_Vars.rage_default;
							GUI::Controls::Checkbox( XorStr( "override weapon config" ), &rage->override_default_config );
							GUI::ctx->enabled = bruh;
							GUI::Controls::Dropdown( XorStr( "##weapon config" ), {
									XorStr( "general" ), XorStr( "auto-sniper" ), XorStr( "scout" ), XorStr( "awp" ),
									XorStr( "revolver" ), XorStr( "deagle" ), XorStr( "pistols" ) }, &g_Vars.globals.m_iCurrentRageGroup );
						}
						GUI::Group::EndGroup( );

						GUI::ctx->enabled = bruh2;
					}
					// anti aim tab

					if( nDisplay == 1 || GUI::ctx->setup ) {
						// unset so we don't fuck up things..
						GUI::ctx->CurrentWeaponGroup = XorStr( "" );

						auto bruh2 = GUI::ctx->enabled;
						GUI::ctx->enabled = g_Vars.rage.anti_aim_active;

						GUI::Group::BeginGroup( XorStr( "fake-lag options" ), Vector2D( 50, /*51*/65 ) );
						{
							GUI::Controls::Checkbox( XorStr( "enabled##fakelag" ), &g_Vars.rage.fake_lag );
							GUI::Controls::Dropdown( XorStr( "fake-lag type" ), { XorStr( "factor" ), XorStr( "dynamic" ), XorStr( "fluctuate" )/*, XorStr( "Adaptive" )*/ }, &g_Vars.rage.fake_lag_type );
							GUI::Controls::Slider( XorStr( "fake-lag amount" ), &g_Vars.rage.fake_lag_amount, 1, std::clamp( g_Vars.sv_maxusrcmdprocessticks->GetInt( ) - 1, 0, 62 ) );

							if( g_Vars.rage.fake_lag_type != 3 || GUI::ctx->setup ) {
								std::vector< MultiItem_t > fakelag_triggers{
									{ XorStr( "while moving" ), &g_Vars.rage.fake_lag_moving },
									{ XorStr( "while jumping" ), &g_Vars.rage.fake_lag_air },
									{ XorStr( "while uncrouching" ), &g_Vars.rage.fake_lag_unduck },
									{ XorStr( "while peeking" ), &g_Vars.rage.fake_lag_peeking },
									// { XorStr( "while shooting" ), &g_Vars.rage.fake_lag_firing }
								};
								GUI::Controls::MultiDropdown( XorStr( "fake-lag conditions" ), fakelag_triggers );
							}
						}
						GUI::Group::EndGroup( );

						GUI::ctx->enabled = g_Vars.rage.anti_aim_active;

						GUI::Group::BeginGroup( XorStr( "yaw options" ), Vector2D( 50, 100 ) );
						{
							GUI::Controls::Dropdown( XorStr( "base yaw" ), {
							XorStr( "none" ), XorStr( "180" ), XorStr( "180 z" ), XorStr( "avoid air" ) }, &g_Vars.rage.anti_aim_yaw );

							GUI::Controls::Slider( XorStr( "base yaw additive" ), &g_Vars.rage.anti_aim_base_yaw_additive, -180, 180, XorStr( "%d" ), 1 );

							if( g_Vars.rage.anti_aim_yaw > 0 || GUI::ctx->setup )
								if( GUI::Controls::Checkbox( XorStr( "base yaw add jitter" ), &g_Vars.rage.anti_aim_yaw_add_jitter ) || GUI::ctx->setup ) {
									GUI::Controls::Slider( XorStr( "base yaw jitter distance" ), &g_Vars.rage.anti_aim_yaw_jitter, 0, 180, XorStr( "%d" ), 1 );
								}

							GUI::Controls::Dropdown( XorStr( "fake yaw" ), { XorStr( "none" ), XorStr( "default" ), XorStr( "opposite" ), XorStr( "jitter" ), XorStr( "spin" ), XorStr( "random" ), XorStr( "lby (no leak)" ) }, &g_Vars.rage.anti_aim_fake_yaw );
							if( g_Vars.rage.anti_aim_fake_yaw == 2 || GUI::ctx->setup ) {
								GUI::Controls::Slider( XorStr( "fake yaw additive" ), &g_Vars.rage.anti_aim_fake_yaw_relative, -180, 180, XorStr( "%d" ), 1 );
							}

							if( g_Vars.rage.anti_aim_fake_yaw == 3 || GUI::ctx->setup ) {
								GUI::Controls::Slider( XorStr( "fake yaw jitter distance" ), &g_Vars.rage.anti_aim_fake_yaw_jitter, 1, 180, XorStr( "%d" ), 1 );
							}

							//GUI::Controls::Checkbox( XorStr( "twist on stand" ), &g_Vars.rage.anti_aim_twist );

							GUI::Controls::Checkbox( XorStr( "edge yaw" ), &g_Vars.rage.anti_aim_edge );
							GUI::Controls::Hotkey( XorStr( "edge yaw key" ), &g_Vars.rage.anti_aim_edge_key );

							GUI::Controls::Checkbox( XorStr( "auto yaw" ), &g_Vars.rage.anti_aim_freestand );
							GUI::Controls::Hotkey( XorStr( "auto yaw key" ), &g_Vars.rage.anti_aim_freestand_key );

							GUI::Controls::Checkbox( XorStr( "body yaw" ), &g_Vars.rage.anti_aim_fake_body );
							if( g_Vars.rage.anti_aim_fake_body || GUI::ctx->setup ) {
								if( !g_Vars.rage.anti_aim_fake_body_hide || GUI::ctx->setup )
									GUI::Controls::Dropdown( XorStr( "body yaw side" ), { XorStr( "left" ), XorStr( "right" ) }, &g_Vars.rage.anti_aim_fake_body_side );

								GUI::Controls::Slider( XorStr( "body yaw delta" ), &g_Vars.rage.anti_aim_fake_body_amount, 36, 180, XorStr( "%d" ), 1 );

								GUI::Controls::Checkbox( XorStr( "force balance adjust" ), &g_Vars.rage.anti_aim_fake_body_balance );
								GUI::Controls::Checkbox( XorStr( "hide lby flick" ), &g_Vars.rage.anti_aim_fake_body_hide );
							}

							std::vector<MultiItem_t> desync_land = {
								{XorStr( "initial land" ), &g_Vars.rage.anti_aim_desync_land_first },
								{XorStr( "force on key" ), &g_Vars.rage.anti_aim_desync_land_force },
							};

							GUI::Controls::MultiDropdown( XorStr( "desync land" ), desync_land );
							if( g_Vars.rage.anti_aim_desync_land_force || GUI::ctx->setup )
								GUI::Controls::Hotkey( XorStr( "desync land key" ), &g_Vars.rage.anti_aim_desync_land_key );

							/*if( g_Vars.rage.anti_aim_desync_land_first || g_Vars.rage.anti_aim_desync_land_force || GUI::ctx->setup )
								GUI::Controls::Slider( XorStr( "desync land offset" ), &g_Vars.rage.anti_aim_desync_land_delta, -180, 180, "%d" );*/

							/*GUI::Controls::Dropdown( XorStr( "yaw flick exploit" ), { XorStr( "none" ), XorStr( "regular" ) }, &g_Vars.rage.anti_aim_fake_flick );
							GUI::Controls::Hotkey( XorStr( "yaw flick exploit key" ), &g_Vars.rage.anti_aim_fake_flick_key );*/

							std::vector<MultiItem_t> distortion = {
								{XorStr( "while standing" ), &g_Vars.rage.anti_aim_distortion_stand },
								{XorStr( "while moving" ), &g_Vars.rage.anti_aim_distortion_move },
								{XorStr( "while jumping" ), &g_Vars.rage.anti_aim_distortion_air },
								{XorStr( "while crouching" ), &g_Vars.rage.anti_aim_distortion_crouch }
							};

							GUI::Controls::MultiDropdown( XorStr( "yaw distortion" ), distortion );
							if( GUI::ctx->setup || g_Vars.rage.anti_aim_distortion_stand || g_Vars.rage.anti_aim_distortion_move || g_Vars.rage.anti_aim_distortion_air || g_Vars.rage.anti_aim_distortion_crouch ) {
								GUI::Controls::Hotkey( XorStr( "yaw distortion key" ), &g_Vars.rage.anti_aim_distortion_key );

								GUI::Controls::Dropdown( XorStr( "distortion side" ), { XorStr( "dynamic" ), XorStr( "left" ), XorStr( "right" ) }, &g_Vars.rage.anti_aim_distortion_side );
								GUI::Controls::Checkbox( XorStr( "distortion disable on manual" ), &g_Vars.rage.anti_aim_distortion_disable_on_manual );
								GUI::Controls::Checkbox( XorStr( "distortion hide flick" ), &g_Vars.rage.anti_aim_distortion_hide_flick );
								GUI::Controls::Checkbox( XorStr( "distortion hide last move" ), &g_Vars.rage.anti_aim_distortion_hide_moving );
							}

							GUI::Controls::Hotkey( XorStr( "lock yaw in place" ), &g_Vars.rage.anti_aim_lock_angle_key, true );
							GUI::Controls::Hotkey( XorStr( "yaw override left" ), &g_Vars.rage.anti_aim_manual_left_key, true );
							GUI::Controls::Hotkey( XorStr( "yaw override backwards" ), &g_Vars.rage.anti_aim_manual_back_key, true );
							GUI::Controls::Hotkey( XorStr( "yaw override right" ), &g_Vars.rage.anti_aim_manual_right_key, true );
							GUI::Controls::Hotkey( XorStr( "yaw override forward" ), &g_Vars.rage.anti_aim_manual_forward_key, true );

							GUI::Controls::Checkbox( XorStr( "disable yaw override in air" ), &g_Vars.rage.disable_anti_aim_manual_air );

							//if( GUI::Controls::Checkbox( XorStr( "anti-aim indicator" ), &g_Vars.rage.anti_aim_indi ) || GUI::ctx->setup )
							//	GUI::Controls::ColorPicker( XorStr( "anti-aim indicator color" ), &g_Vars.rage.anti_aim_indi_color );
						}
						GUI::Group::EndGroup( );

						GUI::ctx->enabled = bruh2;
					}

					// unset so we don't fuck up things..
					GUI::ctx->CurrentWeaponGroup = XorStr( "" );
					//#endif
				}

				// unset so we don't fuck up things..
				GUI::ctx->CurrentWeaponGroup = XorStr( "" );
			}

			// unset so we don't fuck up things..
			GUI::ctx->CurrentWeaponGroup = XorStr( "" );

			if( GUI::Form::BeginTab( XorStr( "players" ), XorStr( "esp" ) ) ) {
				if( GUI::Form::BeginSubTab( XorStr( "General" ) ) ) {
					GUI::Group::BeginGroup( XorStr( "visuals" ), Vector2D( 50, 100 ) );
					{
						CVariables::PLAYER_VISUALS *visuals = &g_Vars.visuals_enemy;

						if( visuals ) {
							if( GUI::Controls::Checkbox( XorStr( "Bounding box" ), &visuals->box ) || GUI::ctx->setup )
								GUI::Controls::ColorPicker( XorStr( "Box color" ), &visuals->box_color );

							if( GUI::Controls::Checkbox( XorStr( "Dormant" ), &visuals->dormant ) || GUI::ctx->setup ) {
								/*if( GUI::Controls::Checkbox( XorStr( "Dormant color" ), &visuals->dormant_color_custom ) || GUI::ctx->setup )
									GUI::Controls::ColorPicker( XorStr( "Dormant color picker " ), &visuals->dormant_color );*/

							}

							if( GUI::Controls::Checkbox( XorStr( "POV arrows" ), &visuals->view_arrows ) || GUI::ctx->setup ) {
								GUI::Controls::ColorPicker( XorStr( "POV arrows color" ), &visuals->view_arrows_color );

								GUI::Controls::Slider( XorStr( "Arrows distance" ), &visuals->view_arrows_distance, 15.f, 100.f, XorStr( "%.0f%%" ) );
								GUI::Controls::Slider( XorStr( "Arrows size" ), &visuals->view_arrows_size, 4, 24, XorStr( "%dpx" ) );
							}

							if( GUI::Controls::Checkbox( XorStr( "Health" ), &visuals->health ) || GUI::ctx->setup ) {
								if( GUI::Controls::Checkbox( XorStr( "Health color override" ), &visuals->health_color_override ) || GUI::ctx->setup )
									GUI::Controls::ColorPicker( XorStr( "Health color override color" ), &visuals->health_color );
							}

							if( GUI::Controls::Checkbox( XorStr( "Name" ), &visuals->name ) || GUI::ctx->setup )
								GUI::Controls::ColorPicker( XorStr( "Name color" ), &visuals->name_color );

							if( GUI::Controls::Checkbox( XorStr( "Skeleton" ), &visuals->skeleton ) || GUI::ctx->setup ) {
								GUI::Controls::ColorPicker( XorStr( "Skeleton color" ), &visuals->skeleton_color/*, true, 0, { XorStr( "Regular color" ), XorStr( "Regular" ) }*/ );
								//GUI::Controls::Cog( XorStr( "Skeleton outline color" ), nullptr, &visuals->skeleton_outline_color, true, 14, { XorStr( "Outline color" ), XorStr( "Outline" ) } );
							}

							if( GUI::Controls::Checkbox( XorStr( "History skeleton" ), &visuals->history_skeleton ) || GUI::ctx->setup ) {
								GUI::Controls::ColorPicker( XorStr( "History skeleton color" ), &visuals->history_skeleton_color );
								GUI::Controls::Checkbox( XorStr( "Draw all records" ), &visuals->history_skeleton_all );
								GUI::Controls::Checkbox( XorStr( "Manage alpha" ), &visuals->history_skeleton_all_manage_alpha );
							}

							if( GUI::Controls::Checkbox( XorStr( "Ammo" ), &visuals->ammo ) || GUI::ctx->setup )
								GUI::Controls::ColorPicker( XorStr( "Ammo color" ), &visuals->ammo_color );

							if( GUI::Controls::Checkbox( XorStr( "LBY timer" ), &visuals->lby_timer ) || GUI::ctx->setup )
								GUI::Controls::ColorPicker( XorStr( "LBY timer color" ), &visuals->lby_timer_color );

							if( GUI::Controls::Checkbox( XorStr( "Ping" ), &visuals->ping ) || GUI::ctx->setup )
								GUI::Controls::ColorPicker( XorStr( "Ping color" ), &visuals->ping_color );

							if( GUI::Controls::Checkbox( XorStr( "weapon text" ), &visuals->weapon ) || GUI::ctx->setup ) {
								GUI::Controls::ColorPicker( XorStr( "weapon text color" ), &visuals->weapon_color );
							}

							if( GUI::Controls::Checkbox( XorStr( "Weapon icon" ), &visuals->weapon_icon ) || GUI::ctx->setup )
								GUI::Controls::ColorPicker( XorStr( "weapon icon color" ), &visuals->weapon_icon_color );

							//GUI::Controls::Checkbox( XorStr( "Ignore skin name" ), &visuals->weapon_ignore_skin );

							if( GUI::Controls::Checkbox( XorStr( "Glow" ), &visuals->glow ) || GUI::ctx->setup )
								GUI::Controls::ColorPicker( XorStr( "Glow color" ), &visuals->glow_color );


							if( GUI::Controls::Checkbox( XorStr( "show aimbot points" ), &g_Vars.rage.visualize_aimpoints ) || GUI::ctx->setup )
								GUI::Controls::ColorPicker( XorStr( "show aimbot points color" ), &g_Vars.rage.visualize_aimpoints_clr );

							/*if( GUI::Controls::Checkbox( XorStr( "Player outline" ), &visuals->outline ) || GUI::ctx->setup )
								GUI::Controls::Cog( XorStr( "Player outline color" ), nullptr, &visuals->outline_color );*/

							std::vector< MultiItem_t > flags{
								{ XorStr( "armor" ), &visuals->flag_armor },
								{ XorStr( "money" ), &visuals->flag_money },
								{ XorStr( "zoom" ), &visuals->flag_scope },
								{ XorStr( "reload" ), &visuals->flag_reloading },
								{ XorStr( "flash" ), &visuals->flag_flashed },
								{ XorStr( "utilities" ), &visuals->flag_utility },
								{ XorStr( "exploit" ), &visuals->flag_exploit },
								{ XorStr( "latency" ), &visuals->flag_ping },
								{ XorStr( "resolver" ), &visuals->flag_resolver }
							};

						#if defined(DEV)
							flags.push_back( { XorStr( "resolver mode" ), &visuals->flag_resolver_mode } );
							flags.push_back( { XorStr( "body data" ), &visuals->flag_resolver_body } );
							flags.push_back( { XorStr( "debug" ), &visuals->flag_debug } );
						#endif

							/*	#if defined(DEV) || defined(BETA_MODE)
									flags.push_back( { XorStr( "Resolver" ), &visuals->flag_resolver } );
								#endif*/

							GUI::Controls::MultiDropdown( XorStr( "Flags" ), flags );
						}
					}
					GUI::Group::EndGroup( );

					GUI::Group::BeginGroup( XorStr( "player models" ), Vector2D( 50, 50 ) );
					{
						std::vector<std::string> materials = {
							XorStr( "default" ), XorStr( "material" ), XorStr( "flat" )
						};

						CVariables::CHAMS *pChams = nullptr;

						static int nGroup = 0;
						GUI::Controls::Dropdown( XorStr( "model options" ), { XorStr( "enemies" ), XorStr( "teammates" ), XorStr( "local" ), XorStr( "backtrack" ),
												 XorStr( "body update (enemy)" ),
												 XorStr( "body update (local)" ),
												 XorStr( "aimbot" ), XorStr( "ragdolls" ) }, &nGroup );

						switch( nGroup ) {
							case 0: pChams = &g_Vars.chams_enemy; break;
							case 1: pChams = &g_Vars.chams_teammates; break;
							case 2: pChams = &g_Vars.chams_local; break;
							case 3: 
								pChams = &g_Vars.chams_backtrack; 
								GUI::ctx->CurrentWeaponGroup = XorStr( "backtrack" ); 
								break;
							case 4: pChams = &g_Vars.chams_body_update_enemy; break;
							case 5: pChams = &g_Vars.chams_body_update_local; break;
							case 6: pChams = &g_Vars.chams_aimbot; break;
							case 7: pChams = &g_Vars.chams_ragdolls; break;
						}

						if( pChams ) {
							GUI::Controls::Checkbox( XorStr( "model material" ), &pChams->enabled );

							auto b = GUI::ctx->enabled;
							GUI::ctx->enabled = pChams->enabled;
							{
								GUI::Controls::Dropdown( XorStr( "##model material" ), materials, &pChams->material );

								const bool bNoMaterial = pChams->material == 0;

								bool bDrawIT = false;
								if( GUI::Controls::Checkbox( XorStr( "visible model" ), &pChams->visible ) ) {
									GUI::Controls::ColorPicker( XorStr( "visible model color##enemies" ), &pChams->visible_color );
									bDrawIT = true;
								}

								if( pChams->material > 0 || GUI::ctx->setup ) {
									if( GUI::Controls::Checkbox( XorStr( "invisible model" ), &pChams->invisible ) ) {
										GUI::Controls::ColorPicker( XorStr( "invisible model color" ), &pChams->invisible_color );
										bDrawIT = true;
									}
								}

								if( ( bDrawIT || GUI::ctx->setup ) && pChams->material > 0 ) {
									GUI::Controls::Slider( XorStr( "model reflectivity" ), &pChams->reflectivity, 0.f, 100.f, XorStr( "%0.0f%%" ) );
									GUI::Controls::ColorPicker( XorStr( "model reflectivity color" ), &pChams->reflectivity_color );
									GUI::Controls::Slider( XorStr( "model shine" ), &pChams->shine, 0.f, 100.f, XorStr( "%0.0f%%" ) );
									GUI::Controls::Slider( XorStr( "model rim" ), &pChams->rim, 0.f, 100.f, XorStr( "%0.0f%%" ) );
								}

								//GUI::Controls::Slider( XorStr( "model rim" ), &g_Vars.globals.test, 0.f, 1000.f, XorStr( "%0.0f%%" ) );

								if( GUI::Controls::Checkbox( XorStr( "model glow overlay##enemies" ), &pChams->glow_overlay ) || GUI::ctx->setup ) {
									GUI::Controls::ColorPicker( XorStr( "model glow overlay color##enemies" ), &pChams->glow_overlay_color );

									GUI::Controls::Checkbox( XorStr( "model glow pass through walls##enemies" ), &pChams->glow_overlay_through_walls );

									GUI::Controls::Slider( XorStr( "model glow overlay opacity##enemies" ), &pChams->glow_overlay_opacity, 1.f, 100.f, XorStr( "%0.0f%%" ) );
								}

							}
							GUI::ctx->enabled = b;

							if( pChams == &g_Vars.chams_enemy || GUI::ctx->setup ) {
								if( g_Vars.chams_enemy.invisible )
									GUI::Controls::Checkbox( XorStr( "dormant chams" ), &g_Vars.visuals_enemy.dormant_chams );
							}

							if( pChams == &g_Vars.chams_backtrack || GUI::ctx->setup ) {
								std::vector< MultiItem_t > draw_records{
									{ XorStr( "last" ), &g_Vars.esp.backtrack_draw_last },
									{ XorStr( "ideal" ), &g_Vars.esp.backtrack_draw_ideal },
									{ XorStr( "lerped" ), &g_Vars.esp.backtrack_draw_lerped },
									{ XorStr( "all" ), &g_Vars.esp.backtrack_draw_all }
								};

								GUI::Controls::Checkbox( XorStr( "only draw on current threat" ), &g_Vars.esp.backtrack_current_threat );

								GUI::Controls::MultiDropdown( XorStr( "draw records" ), draw_records );
								if( g_Vars.esp.backtrack_draw_ideal && !g_Vars.esp.backtrack_draw_all || GUI::ctx->setup )
									GUI::Controls::ColorPicker( XorStr( "ideal record color" ), &g_Vars.esp.backtrack_ideal_record_color );

								if( g_Vars.esp.backtrack_draw_all || GUI::ctx->setup ) {
									GUI::Controls::Checkbox( XorStr( "manage alpha" ), &g_Vars.esp.backtrack_manage_alpha );
								}

								std::vector< MultiItem_t > lerp_colors{
									{ XorStr( "visible" ), &g_Vars.esp.lerp_visible },
									{ XorStr( "invisible" ), &g_Vars.esp.lerp_invisible },
									// { XorStr( "lerped" ), &g_Vars.esp.lerp_lerped }
								};

								GUI::Controls::MultiDropdown( XorStr( "lerp colors" ), lerp_colors );
								if( g_Vars.esp.lerp_visible || g_Vars.esp.lerp_invisible || g_Vars.esp.lerp_lerped )
									GUI::Controls::ColorPicker( XorStr( "second lerped color" ), &g_Vars.esp.second_lerped_color );
							}

							if( pChams == &g_Vars.chams_aimbot || GUI::ctx->setup ) {
								GUI::Controls::Dropdown( XorStr( "model capsules" ), { XorStr( "none" ), XorStr( "hitbox only" ), XorStr( "full capsule" ) }, &g_Vars.esp.target_capsules, 10 );
								if( g_Vars.esp.target_capsules > 0 || GUI::ctx->setup )
									GUI::Controls::ColorPicker( XorStr( "model capsules color" ), &g_Vars.esp.target_capsules_color );

								GUI::Controls::Slider( XorStr( "model render duration" ), &g_Vars.esp.chams_hitmatrix_duration, 1.f, 10.f, XorStr( "%.1fs" ) );
							}

							if( pChams == &g_Vars.chams_local || GUI::ctx->setup ) {
								if( GUI::Controls::Checkbox( XorStr( "model glow" ), &g_Vars.visuals_local.glow ) || GUI::ctx->setup )
									GUI::Controls::ColorPicker( XorStr( "model glow color##Local color" ), &g_Vars.visuals_local.glow_color );

								GUI::Controls::Checkbox( XorStr( "disable material when scoped" ), &g_Vars.visuals_local.disable_material );

								std::vector<MultiItem_t> model_blend{
									{ XorStr( "when scoped" ), &g_Vars.esp.blur_in_scoped },
									{ XorStr( "on grenades" ), &g_Vars.esp.blur_on_grenades },
								};

								GUI::Controls::MultiDropdown( XorStr( "model blend" ), model_blend );

								if( g_Vars.esp.blur_in_scoped || g_Vars.esp.blur_on_grenades || GUI::ctx->setup )
									GUI::Controls::Slider( XorStr( "model blend amount" ), &g_Vars.esp.blur_in_scoped_value, 0.0f, 100.f, XorStr( "%0.f%%" ) );
							}

						}
					}
					GUI::Group::EndGroup( );

					GUI::Group::BeginGroup( XorStr( "other models" ), Vector2D( 50, 50 ) );
					{
						std::vector<std::string> materials = {
							XorStr( "default" ), XorStr( "material" ), XorStr( "flat" )
						};

						CVariables::CHAMS *pChams = nullptr;

						static int nGroup = 0;
						GUI::Controls::Dropdown( XorStr( "model options##other" ), { XorStr( "weapons" ), XorStr( "hands" ), XorStr( "attachments (local)" ), XorStr( "attachments (enemy)" ) }, &nGroup );
						switch( nGroup ) {
							case 0: pChams = &g_Vars.chams_weapons; break;
							case 1: pChams = &g_Vars.chams_arms; break;
							case 2: pChams = &g_Vars.chams_attachments_local; break;
							case 3: pChams = &g_Vars.chams_attachments_enemy; break;
						}

						if( pChams ) {
							GUI::Controls::Checkbox( XorStr( "model material##other" ), &pChams->enabled );

							auto b = GUI::ctx->enabled;
							GUI::ctx->enabled = pChams->enabled;
							{
								GUI::Controls::Dropdown( XorStr( "##model material##other" ), materials, &pChams->material );

								const bool bNoMaterial = pChams->material == 0;

								bool bDrawIT = false;
								if( GUI::Controls::Checkbox( XorStr( "visible model##other" ), &pChams->visible ) ) {
									GUI::Controls::ColorPicker( XorStr( "visible model color##enemies##other" ), &pChams->visible_color );
									bDrawIT = true;
								}

								if( pChams->material > 0 ) {
									if( GUI::Controls::Checkbox( XorStr( "invisible model##other" ), &pChams->invisible ) ) {
										GUI::Controls::ColorPicker( XorStr( "invisible model color##other" ), &pChams->invisible_color );
										bDrawIT = true;
									}
								}

								if( ( bDrawIT || GUI::ctx->setup ) && pChams->material > 0 ) {
									GUI::Controls::Slider( XorStr( "model reflectivity##other" ), &pChams->reflectivity, 0.f, 100.f, XorStr( "%0.0f%%" ) );
									GUI::Controls::ColorPicker( XorStr( "model reflectivity color##other" ), &pChams->reflectivity_color );
									GUI::Controls::Slider( XorStr( "model shine##other" ), &pChams->shine, 0.f, 100.f, XorStr( "%0.0f%%" ) );
									GUI::Controls::Slider( XorStr( "model rim##other" ), &pChams->rim, 0.f, 100.f, XorStr( "%0.0f%%" ) );
								}

								//GUI::Controls::Slider( XorStr( "model rim" ), &g_Vars.globals.test, 0.f, 1000.f, XorStr( "%0.0f%%" ) );

								if( GUI::Controls::Checkbox( XorStr( "model glow overlay##enemies##other" ), &pChams->glow_overlay ) || GUI::ctx->setup ) {
									GUI::Controls::ColorPicker( XorStr( "model glow overlay color##enemies##other" ), &pChams->glow_overlay_color );

									GUI::Controls::Checkbox( XorStr( "model glow overlay pass through##enemies##other" ), &pChams->glow_overlay_through_walls );

									GUI::Controls::Slider( XorStr( "model glow overlay opacity##enemies##other" ), &pChams->glow_overlay_opacity, 1.f, 100.f, XorStr( "%0.0f%%" ) );
								}

							}
							GUI::ctx->enabled = b;

						}
					}
					GUI::Group::EndGroup( );
				}
			}

			if( GUI::Form::BeginTab( XorStr( "world" ), XorStr( "style" ) ) ) {

				GUI::Group::BeginGroup( XorStr( "general" ), Vector2D( 50, 100 ) );
				{

					std::vector< MultiItem_t > dropped_wpn{
						{ XorStr( "glow" ), &g_Vars.esp.dropped_weapons_glow },
						{ XorStr( "text" ), &g_Vars.esp.dropped_weapons },
						{ XorStr( "ammo" ), &g_Vars.esp.dropped_weapons_ammo },
					};

					GUI::Controls::MultiDropdown( XorStr( "Dropped weapons" ), dropped_wpn );
					int lel = 0;
					if( g_Vars.esp.dropped_weapons ) {
						GUI::Controls::ColorPicker( XorStr( "Dropped weapons color" ), &g_Vars.esp.dropped_weapons_color, true, false, lel );
						lel++;
					}

					if( g_Vars.esp.dropped_weapons_ammo ) {
						GUI::Controls::ColorPicker( XorStr( "Dropped weapons ammo color" ), &g_Vars.esp.dropped_weapons_ammo_color, true, false, lel );
						lel++;
					}

					if( g_Vars.esp.dropped_weapons_glow ) {
						GUI::Controls::ColorPicker( XorStr( "Dropped weapons glow color" ), &g_Vars.esp.dropped_weapons_glow_color, true, false, lel );
						lel++;
					}

					std::vector< MultiItem_t > grenade_lel = {
						{ XorStr( "glow" ), &g_Vars.esp.grenades_glow },
						{ XorStr( "text" ), &g_Vars.esp.grenades },
						{ XorStr( "radius" ), &g_Vars.esp.grenades_radius }
					};

					GUI::Controls::MultiDropdown( XorStr( "Grenades" ), grenade_lel );
					int lul = 0;
					if( g_Vars.esp.grenades_glow ) {
						GUI::Controls::ColorPicker( XorStr( "Grenades glow color" ), &g_Vars.esp.grenades_glow_color, true, false, lul );
						lul++;
					}

					if( g_Vars.esp.grenades ) {
						GUI::Controls::ColorPicker( XorStr( "Grenades color" ), &g_Vars.esp.grenades_color, true, false, lul );
						lul++;
					}
				
					std::vector< MultiItem_t > grenade_radius = {
						{ XorStr( "smoke" ), &g_Vars.esp.grenades_radius_smoke },
						{ XorStr( "fire" ), &g_Vars.esp.grenades_radius_fire }
					};

					if( g_Vars.esp.grenades_radius ) {
						GUI::Controls::MultiDropdown( XorStr( "Grenade radius" ), grenade_radius );

						int lul = 0;
						if( g_Vars.esp.grenades_radius_smoke ) {
							GUI::Controls::ColorPicker( XorStr( "Smoke radius color" ), &g_Vars.esp.grenades_radius_smoke_color, true, false, lul );
							lul++;
						}

						if( g_Vars.esp.grenades_radius_fire ) {
							GUI::Controls::ColorPicker( XorStr( "Fire radius color" ), &g_Vars.esp.grenades_radius_fire_color, true, false, lul );
							lul++;
						}
					}

					// hostage, ...
					std::vector< MultiItem_t > objectives{
						{ XorStr( "screen (2d)" ), &g_Vars.esp.draw_c4_2d },
						{ XorStr( "world (3d)" ), &g_Vars.esp.draw_c4_3d },
					};

					GUI::Controls::MultiDropdown( XorStr( "bomb esp" ), objectives );

					std::vector<MultiItem_t> removals{
						{ XorStr( "smoke grenades" ), &g_Vars.esp.remove_smoke },
						{ XorStr( "flashbang grenades" ), &g_Vars.esp.remove_flash },
						{ XorStr( "scope lines" ), &g_Vars.esp.remove_scope },
						{ XorStr( "fog smoke" ), &g_Vars.esp.remove_fog },
						{ XorStr( "post processing" ), &g_Vars.esp.remove_post_proccesing },
						{ XorStr( "recoil shake" ), &g_Vars.esp.remove_recoil_shake },
						{ XorStr( "recoil punch" ), &g_Vars.esp.remove_recoil_punch },
						{ XorStr( "ignore first scope" ), &g_Vars.esp.remove_scope_zoom },
					};
					GUI::Controls::MultiDropdown( XorStr( "visual removals" ), removals );

					std::vector<MultiItem_t> modulations{
						{ XorStr( "full bright" ), &g_Vars.esp.fullbright_modulation },
						{ XorStr( "world color" ), &g_Vars.esp.world_modulation },
						{ XorStr( "props color" ), &g_Vars.esp.prop_modulation },
						{ XorStr( "skybox color" ), &g_Vars.esp.sky_modulation },
						{ XorStr( "ambient lighting" ), &g_Vars.esp.ambient_ligtning },
						{ XorStr( "fog modulation" ), &g_Vars.esp.fog_modulation }
						//{ XorStr( "Weather controller" ), &g_Vars.esp.weather_controller },
					};

					GUI::Controls::MultiDropdown( XorStr( "world modulations" ), modulations );

					int nOffset = 0;

					if( g_Vars.esp.world_modulation || GUI::ctx->setup ) {
						GUI::Controls::ColorPicker( XorStr( "world color#clr" ), &g_Vars.esp.world_modulation_color, true, false, nOffset++ );
					}

					if( g_Vars.esp.prop_modulation || GUI::ctx->setup ) {
						GUI::Controls::ColorPicker( XorStr( "props color#clr" ), &g_Vars.esp.prop_modulation_color, true, false, nOffset++ );
					}

					if( g_Vars.esp.sky_modulation || GUI::ctx->setup ) {
						GUI::Controls::ColorPicker( XorStr( "skybox color#clr" ), &g_Vars.esp.sky_modulation_color, true, false, nOffset++ );
					}

					if( g_Vars.esp.ambient_ligtning || GUI::ctx->setup ) {
						GUI::Controls::ColorPicker( XorStr( "ambient lighting color#clr" ), &g_Vars.esp.ambient_ligtning_color, true, false, nOffset++ );
					}

					if( g_Vars.esp.fog_modulation || GUI::ctx->setup ) {
						GUI::Controls::ColorPicker( XorStr( "fog modulation color#clr" ), &g_Vars.esp.fog_modulation_color, true, false, nOffset++ );

						GUI::Controls::Slider( XorStr( "start fog" ), &g_Vars.esp.fog_start_s, 0, 15000.f, XorStr( "%.0f%%" ) );
						GUI::Controls::Slider( XorStr( "end fog" ), &g_Vars.esp.fog_end_s, 0, 15000.f, XorStr( "%.0f%%" ) );
					}

					GUI::Controls::Dropdown( XorStr( "skybox changer" ), {
						XorStr( "Default" ),
						XorStr( "cs_baggage_skybox_" ),
						XorStr( "cs_tibet" ),
						XorStr( "embassy" ),
						XorStr( "italy" ),
						XorStr( "jungle" ),
						XorStr( "nukeblank" ),
						XorStr( "office" ),
						XorStr( "sky_csgo_cloudy01" ),
						XorStr( "sky_csgo_night02" ),
						XorStr( "sky_csgo_night02b" ),
						XorStr( "sky_dust" ),
						XorStr( "sky_venice" ),
						XorStr( "vertigo" ),
						XorStr( "vietnam" ),
						XorStr( "custom" ),
											 }, &g_Vars.esp.sky_changer );

					if( g_Vars.esp.sky_changer == 15 ) {
						GUI::Controls::Textbox( XorStr( "skybox name" ), &g_Vars.esp.sky_changer_name, 24 );
					}

					GUI::Controls::Checkbox( XorStr( "force engine radar" ), &g_Vars.esp.ingame_radar );

					if( GUI::Controls::Checkbox( XorStr( "custom aspect ratio" ), &g_Vars.esp.aspect_ratio ) || GUI::ctx->setup ) {
						GUI::Controls::Slider( XorStr( "aspect ratio" ), &g_Vars.esp.aspect_ratio_value, 0.02f, 5.f, XorStr( "%.2f" ), 0.01f );
					}

				}
				GUI::Group::EndGroup( );

				GUI::Group::BeginGroup( XorStr( "effects" ), Vector2D( 50, 100 ) );
				{
					std::vector<MultiItem_t> indicators{
						{ XorStr( "show anti-aim lby" ), &g_Vars.esp.indicator_antiaim_lby },
						{ XorStr( "show anti-aim side" ), &g_Vars.esp.indicator_antiaim_side },
						{ XorStr( "show lag comp break" ), &g_Vars.esp.indicator_lagcomp },
						{ XorStr( "show crosshair" ), &g_Vars.esp.indicator_crosshair },
						{ XorStr( "show ping spike" ), &g_Vars.esp.indicator_pingspike },
						{ XorStr( "show target limit" ), &g_Vars.rage.anti_aim_fps },
					};

					std::vector<MultiItem_t> crosshair_indicators{
						{ XorStr( "show min dmg override" ), &g_Vars.esp.crosshair_indicator_mindmg },
						{ XorStr( "show wait for lby" ), &g_Vars.esp.crosshair_indicator_lby },
						{ XorStr( "show override" ), &g_Vars.esp.crosshair_indicator_override },
						{ XorStr( "show force body aim" ), &g_Vars.esp.crosshair_indicator_body },
						{ XorStr( "show ping spike" ), &g_Vars.esp.crosshair_indicator_ping }
					};

					GUI::Controls::MultiDropdown( XorStr( "indicators" ), indicators );

					if( g_Vars.esp.indicator_antiaim_side || GUI::ctx->setup )
						GUI::Controls::ColorPicker( XorStr( "anti-aim indicator color" ), &g_Vars.esp.indicator_antiaim_side_color );

					if( g_Vars.esp.indicator_crosshair || GUI::ctx->setup )
						GUI::Controls::MultiDropdown( XorStr( "crosshair indicators" ), crosshair_indicators );

					GUI::Controls::Slider( XorStr( "override fov" ), &g_Vars.misc.override_fov, 60.f, 150.f, XorStr( "%.0f%%" ) );
					GUI::Controls::Slider( XorStr( "scale zoom fov" ), &g_Vars.misc.override_fov_scope, 0.f, 100.f, XorStr( "%.0f%%" ) );
					GUI::Controls::Checkbox( XorStr( "show viewmodel in scope" ), &g_Vars.misc.show_viewmodel_in_scope );

					if( GUI::Controls::Checkbox( XorStr( "third person" ), &g_Vars.esp.third_person ) || GUI::ctx->setup ) {
						GUI::Controls::Hotkey( XorStr( "third person key" ), &g_Vars.esp.third_person_bind );
					}

					GUI::Controls::Checkbox( XorStr( "force crosshair" ), &g_Vars.esp.force_sniper_crosshair );
					GUI::Controls::Checkbox( XorStr( "autowall indicator" ), &g_Vars.esp.autowall_crosshair );

					GUI::Controls::Dropdown( XorStr( "show spectators" ), { XorStr( "off" ), XorStr( "retro" ), XorStr( "classic" ) }, &g_Vars.esp.spectators );

					int nImpactColorOffset = 0;
					if( GUI::Controls::Checkbox( XorStr( "bullet impacts" ), &g_Vars.esp.server_impacts ) || GUI::ctx->setup ) {
						GUI::Controls::ColorPicker( XorStr( "Server bullet impacts color" ), &g_Vars.esp.server_impacts_color, true, false, nImpactColorOffset++ );
						GUI::Controls::ColorPicker( XorStr( "Client bullet impacts color" ), &g_Vars.esp.client_impacts_color, true, false, nImpactColorOffset++ );
					}

					GUI::Controls::Dropdown( XorStr( "spread crosshair" ), { XorStr( "none" ), XorStr( "gradient" ), XorStr( "rainbow" ) }, &g_Vars.esp.spread_crosshair );
					if( g_Vars.esp.spread_crosshair == 1 )
						GUI::Controls::ColorPicker( XorStr( "spread crosshair color" ), &g_Vars.esp.spread_crosshair_color );
					else if( g_Vars.esp.spread_crosshair == 2 )
						GUI::Controls::Slider( XorStr( "spread crosshair opacity" ), &g_Vars.esp.spread_crosshair_opacity, 1.f, 100.f, XorStr( "%.0f%%" ) );

					std::vector<MultiItem_t> bullet_tracers{
						{ XorStr( "show local" ), &g_Vars.esp.bullet_tracer_local },
						{ XorStr( "show enemies" ), &g_Vars.esp.bullet_tracer_enemy },
					};

					GUI::Controls::MultiDropdown( XorStr( "bullet beams" ), bullet_tracers );
					if( g_Vars.esp.bullet_tracer_local )
						GUI::Controls::ColorPicker( XorStr( "Local bullet tracer color" ), &g_Vars.esp.bullet_tracer_local_color, true, false, 0 );

					if( g_Vars.esp.bullet_tracer_enemy )
						GUI::Controls::ColorPicker( XorStr( "Enemy bullet tracer color" ), &g_Vars.esp.bullet_tracer_enemy_color, true, false, int( g_Vars.esp.bullet_tracer_local ) );

					if( g_Vars.esp.bullet_tracer_local || g_Vars.esp.bullet_tracer_enemy || GUI::ctx->setup ) {
						GUI::Controls::Slider( XorStr( "bullet beams duration" ), &g_Vars.esp.bullet_tracer_life, 1, 10, XorStr( "%ds" ) );
					}

					if( GUI::Controls::Checkbox( XorStr( "grenade prediction" ), &g_Vars.esp.grenade_prediction ) || GUI::ctx->setup )
						GUI::Controls::ColorPicker( XorStr( "grenade prediction color" ), &g_Vars.esp.grenade_prediction_color );

					if( GUI::Controls::Checkbox( XorStr( "grenade warning" ), &g_Vars.esp.grenade_proximity_warning ) || GUI::ctx->setup )
						GUI::Controls::ColorPicker( XorStr( "grenade warning color" ), &g_Vars.esp.grenade_proximity_warning_color );

					std::vector<MultiItem_t> hitmarker{
							{ XorStr( "screen (2d)" ), &g_Vars.esp.vizualize_hitmarker },
							{ XorStr( "world (3d)" ), &g_Vars.esp.visualize_hitmarker_world },
					};

					GUI::Controls::Checkbox( XorStr( "show damage" ), &g_Vars.esp.visualize_damage );
					GUI::Controls::MultiDropdown( XorStr( "hit marker" ), hitmarker );

					GUI::Controls::Dropdown( XorStr( "hit sound type" ), { XorStr( "none" ), XorStr( "default" ), XorStr( "arena switch" ), XorStr( "bubble" ), XorStr( "custom" ) }, &g_Vars.esp.hitsound_type );
					if( g_Vars.esp.hitsound_type > 0 ) {
						if( g_Vars.esp.hitsound_type == 4 || GUI::ctx->setup ) {
							const auto GetSounds = [ ] ( ) {
								std::string dir = GetHitsoundsDirectory( );

								for( auto &file_path : std::filesystem::directory_iterator( dir ) ) {
									if( !file_path.path( ).string( ).empty( ) ) {
										if( file_path.path( ).string( ).find( XorStr( ".wav" ) ) != std::string::npos ) {
											g_Vars.globals.m_vecHitsounds.emplace_back( file_path.path( ).string( ).erase( 0, dir.length( ) ) );
										}
									}
								}
							};


							if( g_Vars.globals.m_vecHitsounds.empty( ) || ( g_Vars.globals.m_iTick % 500 ) == 0 ) {
								GetSounds( );
							}

							if( g_Vars.globals.m_vecHitsounds.empty( ) ) {
								GUI::Controls::Dropdown( XorStr( "available hit sounds" ), { XorStr( "none" ) }, &g_Vars.esp.hitsound_custom );
							}
							else {
								GUI::Controls::Dropdown( XorStr( "available hit sounds" ), g_Vars.globals.m_vecHitsounds, &g_Vars.esp.hitsound_custom );
							}
						}

						GUI::Controls::Slider( XorStr( "hit sound volume" ), &g_Vars.esp.hitsound_volume, 1.f, 100.f, XorStr( "%.0f%%" ) );
					}
				}
				GUI::Group::EndGroup( );
			}

			if( GUI::Form::BeginTab( XorStr( "misc" ), XorStr( "etc" ) ) ) {
				if( GUI::Form::BeginSubTab( XorStr( "General" ) ) ) {
					GUI::Group::BeginGroup( XorStr( "General" ), Vector2D( 50, 100 ) );
					{
						if( GUI::Controls::Checkbox( XorStr( "extended killfeed" ), &g_Vars.misc.preserve_killfeed ) ) {
							// note - alpha;
							// maybe make a dropdown, with options such as:
							// "extend", "preserve" where if u have extend 
							// you can choose how long, and if u have preserve
							// just force this to FLT_MAX or smth? idk.
							g_Vars.misc.preserve_killfeed_time = 300.f;
						}
						GUI::Controls::Checkbox( XorStr( "Clantag spammer" ), &g_Vars.misc.clantag_changer );

						GUI::Controls::Checkbox( XorStr( "Unlock inventory" ), &g_Vars.misc.unlock_inventory );
						GUI::Controls::Checkbox( XorStr( "Filter console" ), &g_Vars.misc.filter_console );
						GUI::Controls::Checkbox( XorStr( "Ignore radio" ), &g_Vars.misc.ignore_radio );

						// rotates the in-game radar (and the local blip arrow)
						// to the real yaw, in first and third person.
						GUI::Controls::Checkbox( XorStr( "rotate radar to real yaw" ), &g_Vars.misc.radar_real_yaw );

						std::vector<MultiItem_t> logs{
							{ XorStr( "Damage given" ), &g_Vars.misc.event_dmg },
							{ XorStr( "Damage taken" ), &g_Vars.misc.event_harm },
							{ XorStr( "Purchases" ), &g_Vars.misc.event_buy },
						};

						GUI::Controls::MultiDropdown( XorStr( "notifications" ), logs );


						GUI::Controls::Checkbox( XorStr( "auto buy" ), &g_Vars.misc.autobuy_enabled );
						if( g_Vars.misc.autobuy_enabled || GUI::ctx->setup ) {
							std::vector<std::string> first_weapon_str = {
								XorStr( "none" ),
								XorStr( "auto-sniper" ),
								XorStr( "scout" ),
								XorStr( "awp" ),
							};

							std::vector<std::string> second_weapon_str = {
								XorStr( "none" ),
								XorStr( "dualies" ),
								XorStr( "revolver / deagle" ),
								XorStr( "five-seven / tec9" ),
							};

							std::vector<MultiItem_t> other_weapon_conditions = {
								{ XorStr( "armor" ), &g_Vars.misc.autobuy_armor },
								{ XorStr( "flashbang" ), &g_Vars.misc.autobuy_flashbang },
								{ XorStr( "grenade" ), &g_Vars.misc.autobuy_hegrenade },
								{ XorStr( "molotov" ), &g_Vars.misc.autobuy_molotovgrenade },
								{ XorStr( "smoke" ), &g_Vars.misc.autobuy_smokegreanade },
								//{ XorStr( "Decoy" ), &g_Vars.misc.autobuy_decoy },
								{ XorStr( "zeus" ), &g_Vars.misc.autobuy_zeus },
								{ XorStr( "defuser" ), &g_Vars.misc.autobuy_defusekit },
							};

							GUI::Controls::Dropdown( XorStr( "auto buy primary" ), first_weapon_str, &g_Vars.misc.autobuy_first_weapon );
							GUI::Controls::Dropdown( XorStr( "auto buy secondary" ), second_weapon_str, &g_Vars.misc.autobuy_second_weapon );
							GUI::Controls::MultiDropdown( XorStr( "auto buy utilities" ), other_weapon_conditions );
						}

						//GUI::Controls::Checkbox( XorStr( "Unlock hidden convars" ), &g_Vars.misc.unlocked_hidden_cvars );
						//GUI::Controls::Checkbox( XorStr( "Unlock lagcomp restrictions" ), &g_Vars.misc.cl_lagcomp_bypass, { XorStr( "Bypasses cl_lagcompensation restrictions caused by plugins" ), XorStr( "Bypasses cl_lagcompensation restrictions" ) } );
						//GUI::Controls::Checkbox( XorStr( "Suppress server messages" ), &g_Vars.misc.disable_server_messages, { XorStr( "Suppresses server messages (e.g. ads)" ), XorStr( "Suppresses" ) } );
					}
					GUI::Group::EndGroup( );

					GUI::Group::BeginGroup( XorStr( "Movement" ), Vector2D( 50, 100 ) );
					{
						GUI::Controls::Checkbox( XorStr( "Bunny hop" ), &g_Vars.misc.bunnyhop );
						if( GUI::Controls::Checkbox( XorStr( "Auto strafe" ), &g_Vars.misc.autostrafer ) ) {
							GUI::Controls::Checkbox( XorStr( "wasd key strafer" ), &g_Vars.misc.autostrafer_wasd );
							GUI::Controls::Slider( XorStr( "Auto strafe smoothing" ), &g_Vars.misc.autostrafer_smooth, 0.f, 100.f, XorStr( "%.0f%%" ) );
						}

						#if defined(DEV)
							GUI::Controls::Checkbox( XorStr( "Infinite duck stamina" ), &g_Vars.misc.fastduck );
						#endif

						GUI::Controls::Checkbox( XorStr( "Quick stop" ), &g_Vars.misc.quickstop );
						if( GUI::Controls::Checkbox( XorStr( "Edge jump" ), &g_Vars.misc.edge_jump ) ) {
							GUI::Controls::Hotkey( XorStr( "Edge jump key" ), &g_Vars.misc.edge_jump_key );
						}


						GUI::Controls::Dropdown( XorStr( "leg movement" ), { XorStr( "default" ), XorStr( "never slide" ), XorStr( "force slide" ) }, &g_Vars.misc.leg_movement );
						if( GUI::Controls::Checkbox( XorStr( "fake walk" ), &g_Vars.misc.slow_walk ) )
							GUI::Controls::Hotkey( XorStr( "fake walk key" ), &g_Vars.misc.slow_walk_bind );

						if( GUI::Controls::Checkbox( XorStr( "quick peek assist" ), &g_Vars.misc.autopeek ) ) {
							GUI::Controls::Hotkey( XorStr( "quick peek assist key##key" ), &g_Vars.misc.autopeek_bind );

							GUI::Controls::Dropdown( XorStr( "quick peek assist retreat" ), { XorStr( "retreat after shooting" ),XorStr( "always retreat" ) }, &g_Vars.misc.autopeek_retrack );
							GUI::Controls::ColorPicker( XorStr( "Auto peek color##key" ), &g_Vars.misc.autopeek_color, true, 0 );
						}
					}
					GUI::Group::EndGroup( );
				}

				if( GUI::Form::BeginSubTab( XorStr( "Skins" ) ) && !GUI::ctx->setup ) {

				}
			}


		#if defined(LUA_SCRIPTING)
			static int SELECTED_SCRIPT;
			static std::vector<std::string> SCRIPT_LIST;
			static bool INIT_SCRIPTS = true;

			bool LUA_REINT = false;
			if( INIT_SCRIPTS || ( GetTickCount( ) % 1000 ) == 0 ) {
				SCRIPT_LIST = Scripting::GetScripts( );
				INIT_SCRIPTS = false;
				LUA_REINT = true;
			}

			// let's loop through all scripts
			for( auto &script : SCRIPT_LIST ) {
				bool found = false;

				for( int i = 0; i < g_Vars.m_loaded_luas.m_children.size( ); ++i ) {
					auto it = ( CVariables::lua_scripts_data * )g_Vars.m_loaded_luas.m_children[ i ];

					if( it ) {
						if( it->m_script_name == script ) {
							// let's not add it again..
							found = true;
						}
					}
				}

				// ooooo, not in g_Vars.m_loaded_luas yet?
				if( !found ) {
					auto index = g_Vars.m_loaded_luas.AddEntry( );
					auto entry = g_Vars.m_loaded_luas[ index ];

					entry->SetName( script );

					// let's set the name!
					entry->m_script_name = script;
					entry->m_is_cloud = false;
				}
			}

			if( !g_Vars.m_loaded_luas.m_children.empty( ) ) {
				for( int i = 0; i < g_Vars.m_loaded_luas.m_children.size( ); ++i ) {
					auto it = ( CVariables::lua_scripts_data * )g_Vars.m_loaded_luas.m_children[ i ];

					if( it ) {
						bool found = false;

						for( auto &script : SCRIPT_LIST ) {
							// hahahahhahahahaa
							if( script == it->m_script_name ) {
								found = true;
							}
						}

						// we did not find this anymore (user deleted script?)
						if( !found ) {
							// ya YEET
							g_Vars.m_loaded_luas.m_children.erase( g_Vars.m_loaded_luas.m_children.begin( ) + i );
						}
					}
				}
			}
		#endif

		#if defined(LUA_SCRIPTING)
			static bool bDisplayMessage = true;
			if( GUI::Form::BeginTab( XorStr( "scripts" ), XorStr( "lua" ) ) && !GUI::ctx->setup ) {
				if( GUI::Form::BeginSubTab( XorStr( "Local" ) ) ) {
					GUI::Group::BeginGroup( XorStr( "Scripts" ), Vector2D( 50, 100 ) );
					{
						GUI::Controls::Checkbox( XorStr( "Allow unsafe scripts" ), &g_Vars.menu.unsafe_scripts, { XorStr( "Allow FFI library to be used within scripts." ), XorStr( "Allow FFI library" ) } );

						GUI::Controls::Button( XorStr( "Reload scripts" ), [&] ( ) -> void {
							Scripting::ReloadActiveScripts( );
						} );

						GUI::Controls::Label( XorStr( "script list" ) );
						GUI::Controls::Luabox( XorStr( "script list#box" ), SCRIPT_LIST, &SELECTED_SCRIPT, true, 6 );

						if( LUA_REINT ) {
							if( SELECTED_SCRIPT >= SCRIPT_LIST.size( ) )
								SELECTED_SCRIPT = SCRIPT_LIST.size( ) - 1;

							if( SELECTED_SCRIPT < 0 )
								SELECTED_SCRIPT = 0;
						}

						if( !SCRIPT_LIST.empty( ) ) {
							auto current_script = SCRIPT_LIST.at( SELECTED_SCRIPT );

							CVariables::lua_scripts_data *script = nullptr;
							// I couldn't think about a better way to do this, oh well STD::FIND_IF (kek)??
							// fuck std
							for( int i = 0; i < g_Vars.m_loaded_luas.m_children.size( ); ++i ) {
								auto it = ( CVariables::lua_scripts_data * )g_Vars.m_loaded_luas.m_children[ i ];

								if( it != nullptr ) {
									if( it->m_script_name == current_script ) {
										script = it;
									}
								}
							}

							//bool loaded = Scripting::Script::IsScriptLoaded( current_script.append( XorStr( ".lua" ) ), false );
							//std::string state = loaded ? XorStr( "active" ) : XorStr( "inactive" );

							//GUI::Controls::Label( XorStr( "Status: " ) + state );

							// even tho this should never happen, I'm better ultra safe...
							if( script != nullptr ) {
								GUI::Controls::Checkbox( XorStr( "Load with config" ), &script->m_load );
							}

							// append it !!
							current_script.append( XorStr( ".lua" ) );

							GUI::Controls::Button( XorStr( "Load script" ), [&] ( ) {
								std::string xd = XorStr( "" );
								Scripting::Script::LoadScript( current_script, xd );

								GUI::ctx->FocusedID = 0;
							} );

							GUI::Controls::Button( XorStr( "Unload script" ), [&] ( ) {
								Scripting::Script::UnloadScript( current_script );

								GUI::ctx->FocusedID = 0;
							} );
						}

						GUI::Controls::Button( XorStr( "Open script folder" ), [&] ( ) {
							Scripting::OpenFolder( );
						} );

						GUI::Group::EndGroup( );
					}

					GUI::Group::BeginGroup( XorStr( "lua elements" ), Vector2D( 50, 100 ) );
					{
						if( !SCRIPT_LIST.empty( ) ) {
							auto current_script = SCRIPT_LIST.at( SELECTED_SCRIPT );
							current_script.append( XorStr( ".lua" ) );

							bool loaded = Scripting::Script::IsScriptLoaded( current_script );

							// TODO: cleanup/rewrite (it looks so fucking ugly...)
							if( !Scripting::states.empty( ) && loaded ) {
								for( auto states : Scripting::states ) {
									// current selected script?
									if( states.first == current_script ) {
										// state valid?
										if( states.second ) {
											// do we have fucking items?
											if( !states.second->ui_items.empty( ) ) {
												int nLastWasCOLOR = 0;
												for( auto &item : states.second->ui_items ) {
													if( item.second ) {
														auto type = item.second->get_type( );
														auto xd = XorStr( "##" ) + states.first + std::to_string( type ) + XorStr( "_lua" );

														if( !item.second->visible )
															continue;

														if( type != Scripting::itemType_t::COLORPICKER )
															nLastWasCOLOR = 0;

														switch( type ) {
															case Scripting::itemType_t::CHECKBOX:
															{
																auto chkbox = ( Scripting::checkboxItem * )item.second;
																if( chkbox ) {
																	GUI::Controls::Checkbox( item.first + xd, chkbox->value );
																}

																break;
															}
															case Scripting::itemType_t::DROPDOWN:
															{
																auto drpdwn = ( Scripting::dropdownItem * )item.second;
																if( drpdwn ) {
																	GUI::Controls::Dropdown( item.first + xd, drpdwn->items, drpdwn->value );
																}
																break;
															}
															case Scripting::itemType_t::MULTIDROPDOWN:
															{
																auto mltidrpdwn = ( Scripting::multidropdownItem * )item.second;
																if( mltidrpdwn ) {
																	GUI::Controls::MultiDropdown( item.first + xd, *mltidrpdwn->items );
																}
																break;
															}
															case Scripting::itemType_t::COLORPICKER:
															{
																auto clr = ( Scripting::colorpickerItem * )item.second;
																if( clr ) {
																	GUI::Controls::ColorPicker( item.first + xd, clr->color, true, true, 0 );
																	++nLastWasCOLOR;
																}
																break;
															}
															case Scripting::itemType_t::HOTKEY:
															{
																auto htk = ( Scripting::hotkeyItem * )item.second;
																if( htk ) {
																	GUI::Controls::Hotkey( item.first + xd, htk->hotkey, false );
																}
																break;
															}
															case Scripting::itemType_t::SLIDER:
															{
																auto slder = ( Scripting::sliderItem * )item.second;
																if( slder ) {
																	GUI::Controls::Slider( item.first + xd, slder->value, slder->min, slder->max );
																}
																break;
															}
															case Scripting::itemType_t::SLIDER_FLOAT:
															{
																auto slder = ( Scripting::sliderFloatItem * )item.second;
																if( slder ) {
																	GUI::Controls::Slider( item.first + xd, slder->value, slder->min, slder->max );
																}
																break;
															}
															case Scripting::itemType_t::LABEL:
															{
																GUI::Controls::Label( item.first + xd );
																break;
															}
															case Scripting::itemType_t::BUTTON:
															{
																auto btn = ( Scripting::buttonItem * )item.second;
																if( btn ) {
																	GUI::Controls::Button( item.first, [&] ( ) {
																		for( auto &func : btn->get_functions( ) ) {
																			sol::safe_function_result result = func.call( );

																			if( result.valid( ) )
																				continue;

																			sol::error err = result;
																			g_EventLog.PushEvent( err.what( ), Color_f( 1.f, 0.f, 0.f ), true, XorStr( "LUA" ) );
																		}
																	} );
																}
																break;
															}
															case Scripting::itemType_t::TEXTBOX:
															{
																auto txtbox = ( Scripting::textBoxItem * )item.second;
																if( txtbox ) {
																	GUI::Controls::Textbox( item.first + xd, txtbox->value );
																}
																break;
															}
														}
													}
												}
											}
											else {

											}
										}
									}
								}
							}
							else {

							}
						}
						else {
							GUI::Controls::Label( XorStr( "no scripts" ) );
						}

						GUI::Group::EndGroup( );
					}
				}
			}
			else {
				bDisplayMessage = true;
			}
		#endif

			if( GUI::Form::BeginTab( XorStr( "home" ), std::string( XorStr( "as " ) ).append( get_string_from_array( g_Vars.globals.user_info.the_array ) ) ) ) {
				if( GUI::Form::BeginSubTab( XorStr( "General" ) ) ) {

					GUI::Group::BeginGroup( XorStr( "Configs" ), Vector2D( 50, 100 ) );
					{

						static std::vector<std::string> CFG_LIST;
						static bool INITIALISE_CONFIGS = true;
						bool CONFIGS_REINT = false;
						if( INITIALISE_CONFIGS || ( GetTickCount( ) % 1000 ) == 0 ) {
							CFG_LIST = ConfigManager::GetConfigs( );
							INITIALISE_CONFIGS = false;
							CONFIGS_REINT = true;
						}

						static std::string config_name;
						GUI::Controls::Textbox( XorStr( "config name" ), &config_name, 26 );
						GUI::Controls::Listbox( XorStr( "config list" ),
												( CFG_LIST.empty( ) ? std::vector<std::string>{XorStr( "none" )} : CFG_LIST ), &g_Vars.globals.m_iSelectedConfig, true, 4 );

						if( CONFIGS_REINT ) {
							if( g_Vars.globals.m_iSelectedConfig >= CFG_LIST.size( ) )
								g_Vars.globals.m_iSelectedConfig = CFG_LIST.size( ) - 1;

							if( g_Vars.globals.m_iSelectedConfig < 0 )
								g_Vars.globals.m_iSelectedConfig = 0;
						}

						if( !CFG_LIST.empty( ) ) {
							GUI::Controls::Button( XorStr( "Save config#save" ), [&] ( ) {
								if( g_Vars.globals.m_iSelectedConfig <= CFG_LIST.size( ) && g_Vars.globals.m_iSelectedConfig >= 0 ) {
									ConfigManager::SaveConfig( CFG_LIST.at( g_Vars.globals.m_iSelectedConfig ) );

									GUI::ctx->SliderInfo.LastChangeTime.clear( );
									GUI::ctx->SliderInfo.PreviewAnimation.clear( );
									GUI::ctx->SliderInfo.PreviousAmount.clear( );
									GUI::ctx->SliderInfo.ShouldChangeValue.clear( );
									GUI::ctx->SliderInfo.ValueTimer.clear( );
									GUI::ctx->SliderInfo.ValueAnimation.clear( );

									g_EventLog.PushEvent( XorStr( "saved config" ), g_Vars.menu.ascent, true, XorStr( "CONFIG" ) );
								}
							} );

							GUI::Controls::Button( XorStr( "Load config" ), [&] ( ) {
								if( g_Vars.globals.m_iSelectedConfig <= CFG_LIST.size( ) && g_Vars.globals.m_iSelectedConfig >= 0 ) {
									ConfigManager::ResetConfig( );

									ConfigManager::LoadConfig( CFG_LIST.at( g_Vars.globals.m_iSelectedConfig ) );

								#if defined(LUA_SCRIPTING)
									Scripting::UnloadActiveScripts( );
								#endif

									g_Vars.m_global_skin_changer.m_update_skins = true;
									g_Vars.m_global_skin_changer.m_update_gloves = true;

									GUI::ctx->SliderInfo.LastChangeTime.clear( );
									GUI::ctx->SliderInfo.PreviewAnimation.clear( );
									GUI::ctx->SliderInfo.PreviousAmount.clear( );
									GUI::ctx->SliderInfo.ShouldChangeValue.clear( );
									GUI::ctx->SliderInfo.ValueTimer.clear( );
									GUI::ctx->SliderInfo.ValueAnimation.clear( );

								#if defined(LUA_SCRIPTING)
									// who needs nice looking code, lol?
									for( int i = 0; i < g_Vars.m_loaded_luas.m_children.size( ); ++i ) {
										auto it = ( CVariables::lua_scripts_data * )g_Vars.m_loaded_luas.m_children[ i ];

										if( it != nullptr ) {
											if( it->m_load ) {

												auto temp = it->m_script_name;
												std::string xd = XorStr( "" );
												Scripting::Script::LoadScript( temp.append( XorStr( ".lua" ) ), xd );

											}
										}
									}
								#endif

									on_cfg_load_knives = on_cfg_load_gloves = true;

									// reset keybinds .
									for( auto &key : g_keybinds ) {
										key->enabled = false;
									}

									// update skin colors
									auto &skin_data = g_Vars.m_skin_changer;
									CVariables::skin_changer_data *skin = nullptr;
									for( size_t i = 0u; i < skin_data.Size( ); ++i ) {
										skin_data[ i ]->m_set_color = true;
									}

									g_EventLog.PushEvent( XorStr( "loaded config" ), g_Vars.menu.ascent, true, XorStr( "CONFIG" ) );
									//g_EventLog.PushEvent( XorStr( "loaded config" ), Color_f( 1.f, 1.f, 1.f ), true, XorStr("config") );
								}
							} );

							if( ( g_Vars.globals.m_iSelectedConfig != 0 && CFG_LIST.at( 0 ) == XorStr( "default" ) ) || ( g_Vars.globals.m_iSelectedConfig >= 0 && CFG_LIST.at( 0 ) != XorStr( "default" ) ) )
								GUI::Controls::Button( XorStr( "Delete config#delete" ), [&] ( ) {
								if( g_Vars.globals.m_iSelectedConfig <= CFG_LIST.size( ) && g_Vars.globals.m_iSelectedConfig >= 0 ) {
									ConfigManager::RemoveConfig( CFG_LIST.at( g_Vars.globals.m_iSelectedConfig ) );
									CFG_LIST = ConfigManager::GetConfigs( );

									g_EventLog.PushEvent( XorStr( "deleted config" ), g_Vars.menu.ascent, true, XorStr( "CONFIG" ) );
								}
							} );
						}

						GUI::Controls::Button( XorStr( "Create config" ), [&] ( ) {
							if( config_name.empty( ) )
								return;

							ConfigManager::CreateConfig( config_name );
							CFG_LIST = ConfigManager::GetConfigs( );

							g_EventLog.PushEvent( XorStr( "created config" ), g_Vars.menu.ascent, true, XorStr( "CONFIG" ) );
						} );

						GUI::Controls::Button( XorStr( "Open config folder" ), [&] ( ) {
							ConfigManager::OpenConfigFolder( );
						} );

						//GUI::Controls::Button( XorStr( "Reset config" ), [ & ] ( ) {
						//	if( g_Vars.globals.m_iSelectedConfig <= CFG_LIST.size( ) && g_Vars.globals.m_iSelectedConfig >= 0 ) {
						//		ConfigManager::ResetConfig( );

						//		g_Vars.m_global_skin_changer.m_update_skins = true;
						//		g_Vars.m_global_skin_changer.m_update_gloves = true;

						//		on_cfg_load_knives = on_cfg_load_gloves = true;

						//		// [...]
						//		for( auto &key : g_keybinds ) {
						//			key->enabled = false;
						//		}
						//	}
						//} );

					//#ifdef DEV
					//	GUI::Controls::Button( XorStr( "Unload cheat" ), [ & ] ( ) {
					//		//if( bHovered )
					//		SetCursor( LoadCursor( NULL, IDC_ARROW ) );

					//		g_Vars.globals.hackUnload = true;
					//	} );
					//#endif
						GUI::Group::EndGroup( );
					}

					GUI::Group::BeginGroup( XorStr( "Global settings" ), Vector2D( 50, 100 ) );
					{
						#if defined(DEV)
							GUI::Controls::Button( "force server restart", [&] ( ) { g_Vars.globals.oppa = true; } );

							GUI::Controls::Button( "blootoothYzio", [ & ]( ) {
								for( int i = 1; i <= 64; ++i ) {
									if( !g_Ragebot.m_arrNapUsers.at( i ).first ) {
										continue;
									}

									const auto pEntity = C_CSPlayer::GetPlayerByIndex( i );
									if( !pEntity ) {
										continue;
									}

									player_info_t info;
									if( !g_pEngine->GetPlayerInfo( pEntity->EntIndex( ), &info ) ) {
										continue;
									}

									std::string name = info.szName;
									std::string napUser = g_Ragebot.m_arrNapUsers.at( i ).second;

									// remove the last character (invalid)
									if( !napUser.empty( ) && napUser.back( ) < 0 ) {
										napUser.pop_back( );
									}

									g_pCVar->ConsoleColorPrintf( Color( 110, 247, 89, 255 ), XorStr( "[ USER ] " ) );

									g_pCVar->ConsoleColorPrintf( Color( 255, 204, 204, 255 ), XorStr( "%s " ), napUser.c_str( ) );
									g_pCVar->ConsoleColorPrintf( Color( 255, 255, 255, 255 ), XorStr( "is napping as " ) );
									g_pCVar->ConsoleColorPrintf( Color( 255, 204, 204, 255 ), XorStr( "%s\n" ), name.c_str( ) );
								}
								} );
						#endif

						GUI::Controls::Hotkey( XorStr( "Menu key#MenuKey" ), &g_Vars.menu.key, true );

						GUI::Controls::ColorPicker( XorStr( "Menu accent color" ), &g_Vars.menu.ascent, false, true );

						GUI::Controls::Checkbox( XorStr( "Watermark" ), &g_Vars.menu.watermark );

						if( GUI::Controls::Checkbox( XorStr( "Whitelist nap users" ), &g_Vars.menu.whitelist ) || GUI::ctx->setup )
							GUI::Controls::Hotkey( XorStr( "Disable whitelist nap users key" ), &g_Vars.menu.whitelist_disable_key );

						GUI::Controls::Checkbox( XorStr( "Show skinchanger" ), &g_Vars.misc.show_skinchanger );

						static bool bOldSkinchangerWindow = g_Vars.misc.show_skinchanger;
						if( bOldSkinchangerWindow != g_Vars.misc.show_skinchanger ) {

							// focus the window when we open it
							if( g_Vars.misc.show_skinchanger ) {
								GUI::pSkinchanger->flLastActivity = g_pGlobalVars->realtime;
							}

							bOldSkinchangerWindow = g_Vars.misc.show_skinchanger;
						}


					#if 0
						GUI::Controls::ColorPicker( XorStr( "test bro 1" ), &g_Vars.globals.testtt1, false, true );
						GUI::Controls::ColorPicker( XorStr( "test bro 2" ), &g_Vars.globals.testtt2, false, true );
						GUI::Controls::ColorPicker( XorStr( "test bro 3" ), &g_Vars.globals.testtt3, false, true );
						GUI::Controls::ColorPicker( XorStr( "test bro 4" ), &g_Vars.globals.testtt4, false, true );
						GUI::Controls::Button( "do it", [ ] ( ) {
							g_Vars.m_global_skin_changer.m_update_skins = true;
							g_Vars.globals.dotest = true;
					} );
					#endif
						/*std::vector<MultiItem_t> vecPopupIgnores = {
							{XorStr( "Save config" ), &g_Vars.menu.ignore_save_popup },
							{XorStr( "Load config" ), &g_Vars.menu.ignore_load_popup},
							{XorStr( "Delete config" ), &g_Vars.menu.ignore_delete_popup},
							{XorStr( "Reset config" ), &g_Vars.menu.ignore_reset_popup},
						};*/

						//GUI::Controls::MultiDropdown( XorStr( "Ignore confirmation popups" ), vecPopupIgnores );

						//GUI::Controls::Dropdown( XorStr( "Menu DPI scale" ), { XorStr( "100% (Default)" ), XorStr( "125%" ), XorStr( "150%" ), XorStr( "175%" ), XorStr( "200%" ) }, &g_Vars.menu.dpi_menu );

					#if defined(DEV) || defined(BETA_MODE)
						//GUI::Controls::Checkbox( XorStr( "Hide aimbot history log" ), &g_Vars.misc.undercover_log );
					#endif

						/*		GUI::Controls::Checkbox( XorStr( "Lock menu layout" ), &g_Vars.menu.lock_layout );
								GUI::Controls::Button( XorStr( "Reset menu layout" ), [ & ] ( ) {
									g_Vars.menu.size_x = 610.f;
									g_Vars.menu.size_y = 415.f;
								} );*/

						GUI::Group::EndGroup( );
				}
			}
		}


	#if /*defined(DEV) || defined(BETA_MODE)*/0
		if( GUI::Form::BeginTab( ETextures::PROFILE, XorStr( "Players" ) ) ) {
			if( GUI::Form::BeginTab( "Player list", XorStr( "Players" ) ) ) {
				static int nPlayerListOption = 0;

				auto vecConnectedPlayers = g_PlayerList.GetConnectedPlayers( );
				GUI::ctx->enabled = !( !g_pEngine->IsConnected( ) || vecConnectedPlayers.empty( ) );

				GUI::Group::BeginGroup( XorStr( "Players" ), Vector2D( 50, 100 ) );
				{
					GUI::Controls::Listbox( XorStr( "##Playerlist" ),
											vecConnectedPlayers.size( ) ?
											vecConnectedPlayers :
											std::vector<std::string>{ }, &nPlayerListOption, false, 12 );

					GUI::Controls::Button( XorStr( "Reset all" ), [&] ( ) { g_PlayerList.ClearPlayerData( ); } );

					if( vecConnectedPlayers.empty( ) ) {
						g_PlayerList.ClearPlayerData( );
						nPlayerListOption = 0;
					}
				}
				GUI::Group::EndGroup( );

				GUI::Group::BeginGroup( XorStr( "Adjustments" ), Vector2D( 50, 100 ) );
				{
					if( nPlayerListOption > vecConnectedPlayers.size( ) - 1 )
						nPlayerListOption = 0;

					GUI::ctx->enabled = !( !g_pEngine->IsConnected( ) || vecConnectedPlayers.empty( ) );

					auto szSelectedPlayer = ( /*bSomethingWentWrong ||*/ vecConnectedPlayers.empty( ) || !GUI::ctx->enabled ) ? XorStr( "" ) : vecConnectedPlayers[ nPlayerListOption ];

					__int64 nSteamID = !GUI::ctx->enabled ? 69 : 1;
					if( g_PlayerList.GetPlayerData( ).size( ) )
						for( auto data : g_PlayerList.GetPlayerData( ) ) {
							nSteamID = data.first;

							if( data.second.m_szPlayerName == szSelectedPlayer ) {
								break;
							}
						}

					// meme entry (in order to show controls when player-list is disabled)
					if( nSteamID == 69 ) {
						if( g_PlayerList.m_vecPlayerData.find( 69 ) == g_PlayerList.m_vecPlayerData.end( ) ) {
							PlayerList::PlayerListInfo_t uEntry;
							uEntry.m_nPlayerHandle = CBaseHandle( 69 );
							uEntry.m_szPlayerName = XorStr( "Deez1337Nuts" );

							g_PlayerList.m_vecPlayerData.insert( { 69, uEntry } );
						}
					}

					if( g_PlayerList.m_vecPlayerData.find( nSteamID ) != g_PlayerList.m_vecPlayerData.end( ) ) {
						auto &listOptions = g_PlayerList.m_vecPlayerData.at( nSteamID );

						GUI::Controls::Checkbox( std::string( XorStr( "Add to whitelist##" ) ).append( szSelectedPlayer ), &listOptions.m_bAddToWhitelist );
						GUI::Controls::Checkbox( std::string( XorStr( "Disable visuals##" ) ).append( szSelectedPlayer ), &listOptions.m_bDisableVisuals );
						GUI::Controls::Checkbox( std::string( XorStr( "High priority##" ) ).append( szSelectedPlayer ), &listOptions.m_bHighPriority );
						GUI::Controls::Slider( std::string( XorStr( "Override extrapolation amount##" ) ).append( szSelectedPlayer ), &listOptions.m_iExtrapolationOverride, 0, 17 );

						//GUI::Controls::Dropdown( std::string( XorStr( "Force yaw##" ) ).append( szSelectedPlayer ), { XorStr( "-" ), XorStr( "Static" ), XorStr( "Away from me" ), XorStr( "Lower body yaw" ), XorStr( "Nearest enemy" ), XorStr( "Average enemy" ) }, &listOptions.m_iForceYaw );
						//GUI::Controls::Slider( std::string( XorStr( "##yaw" ) ).append( szSelectedPlayer ), &listOptions.m_flForcedYaw, -180.f, 180.f, XorStr( "%.0f°" ), 1, true );

						//GUI::Controls::Checkbox( std::string( XorStr( "Force pitch##" ) ).append( szSelectedPlayer ), &listOptions.m_bForcePitch );
						//GUI::Controls::Slider( std::string( XorStr( "##pitch" ) ).append( szSelectedPlayer ), &listOptions.m_flForcedPitch, -89.f, 89.f, XorStr( "%.0f°" ), 1, true );

						// todo - maxwell; find out what was in this dropdown...
						std::vector<MultiItem_t> correction{
							{ XorStr( "Disable anti-aim resolver" ), &listOptions.m_bDisableResolver },
							//{ XorStr( "Disable body prediction" ), &listOptions.m_bDisableBodyPred },
						};
						GUI::Controls::MultiDropdown( std::string( XorStr( "Override anti-aim correction##" ) ).append( szSelectedPlayer ), correction );

						//GUI::Controls::Checkbox( std::string( XorStr( "Edge correction##" ) ).append( szSelectedPlayer ), &listOptions.m_bEdgeCorrection );
						//GUI::Controls::Dropdown( std::string( XorStr( "Override prefer body aim##" ) ).append( szSelectedPlayer ), { XorStr( "-" ), XorStr( "Fake angles" ), XorStr( "Always" ), XorStr( "Aggressive" ), XorStr( "High inaccuracy" ) }, &listOptions.m_iOverridePreferBaim );
						//GUI::Controls::Dropdown( std::string( XorStr( "Override accuracy boost##" ) ).append( szSelectedPlayer ), { XorStr( "-" ), XorStr( "Low" ), XorStr( "Medium" ), XorStr( "High" ) }, &listOptions.m_iOverrideAccuracyBoost );

						GUI::Controls::Button( XorStr( "Apply to all" ), [&] ( ) {
							for( auto &allEntries : g_PlayerList.m_vecPlayerData ) {
								// no need
								if( allEntries.first == nSteamID )
									continue;

								// we don't want to modify these
								const auto szBackupName = allEntries.second.m_szPlayerName;
								const auto nBackupIndex = allEntries.second.m_nPlayerHandle;

								// copy over this player's options to everyone
								std::memcpy( &allEntries.second, &listOptions, sizeof( PlayerList::PlayerListInfo_t ) );

								// restore back backed up variables
								allEntries.second.m_szPlayerName = szBackupName;
								allEntries.second.m_nPlayerHandle = nBackupIndex;
							}
						} );
					}

					// no need to reset it here tbh
					GUI::ctx->enabled = true;
				}
				GUI::Group::EndGroup( );
	}
		#endif
		#endif

			GUI::Form::EndWindow( XorStr( "" ) );

			// yeh
			if( GUI::ctx->setup ) {
				for( auto &child : g_Vars.m_children ) {
					child->Save( );

					auto json = child->GetJson( );
					g_Vars.m_json_default_cfg[ std::to_string( child->GetName( ) ) ] = ( json );
				}

				g_EventLog.PushEvent( XorStr( "loaded default config" ), g_Vars.menu.ascent, true, XorStr( "CONFIG" ) );
			}

			// we not in setup anymore (menu code ran once)
			GUI::ctx->setup = false;
}
	}

#include <limits>
	void DrawSkinchanger( ) {
		GUI::ctx->animation = g_Vars.globals.m_bLoadDefaultConfig ? std::numeric_limits<float>::min( ) : 1.f;
		GUI::ctx->animation *= GUI::pMenu->animation;

		if( g_Vars.globals.m_bLoadDefaultConfig ) {
			ConfigManager::ResetConfig( );

			ConfigManager::LoadConfig( XorStr( "default" ) );

			g_Vars.m_global_skin_changer.m_update_skins = true;
			g_Vars.m_global_skin_changer.m_update_gloves = true;

			on_cfg_load_knives = on_cfg_load_gloves = true;

			// reset keybinds .
			for( auto &key : g_keybinds ) {
				key->enabled = false;
			}

			// update skin colors
			auto &skin_data = g_Vars.m_skin_changer;
			CVariables::skin_changer_data *skin = nullptr;
			for( size_t i = 0u; i < skin_data.Size( ); ++i ) {
				skin_data[ i ]->m_set_color = true;
			}

		#if defined(LUA_SCRIPTING)
			// who needs nice looking code, lol?
			for( int i = 0; i < g_Vars.m_loaded_luas.m_children.size( ); ++i ) {
				auto it = ( CVariables::lua_scripts_data * )g_Vars.m_loaded_luas.m_children[ i ];

				if( it != nullptr ) {
					if( it->m_load ) {

						auto temp = it->m_script_name;
						std::string xd = XorStr( "" );
						Scripting::Script::LoadScript( temp.append( XorStr( ".lua" ) ), xd );

					}
				}
			}
		#endif
		}

		if( GUI::Form::BeginWindow( XorStr( "skinchanger" ) ) ) {

			GUI::Group::BeginGroup( XorStr( "knife changer" ), Vector2D( 50, 22 ) );
			{
				std::vector<std::string> knifes;
				GUI::Controls::Checkbox( XorStr( "override knife##knife" ), &g_Vars.m_global_skin_changer.m_knife_changer );

				if( g_KitParser.vecKnifeNames.at( g_Vars.m_global_skin_changer.m_knife_vector_idx ).definition_index != g_Vars.m_global_skin_changer.m_knife_idx ) {
					auto it = std::find_if( g_KitParser.vecKnifeNames.begin( ), g_KitParser.vecKnifeNames.end( ), [&] ( const WeaponName_t &a ) {
						return a.definition_index == g_Vars.m_global_skin_changer.m_knife_idx;
					} );

					if( on_cfg_load_knives ) {
						if( it != g_KitParser.vecKnifeNames.end( ) )
							g_Vars.m_global_skin_changer.m_knife_vector_idx = std::distance( g_KitParser.vecKnifeNames.begin( ), it );

						on_cfg_load_knives = false;
					}
				}

				static bool init_knife_names = false;
				for( int i = 0; i < g_KitParser.vecKnifeNames.size( ); ++i ) {
					auto currentKnife = g_KitParser.vecKnifeNames[ i ];
					knifes.push_back( currentKnife.name );
				}

				static int bruh = g_Vars.m_global_skin_changer.m_knife_vector_idx;
				static bool changed_knife_smth = g_Vars.m_global_skin_changer.m_knife_changer;

				if( bruh != g_Vars.m_global_skin_changer.m_knife_vector_idx || changed_knife_smth != g_Vars.m_global_skin_changer.m_knife_changer ) {
					g_Vars.m_global_skin_changer.m_update_skins = true;
					g_Vars.m_global_skin_changer.m_update_gloves = true;

					changed_knife_smth = g_Vars.m_global_skin_changer.m_knife_changer;
					bruh = g_Vars.m_global_skin_changer.m_knife_vector_idx;
				}

				if( !knifes.empty( ) ) {
					GUI::Controls::Dropdown( XorStr( "knife model" ), knifes, &g_Vars.m_global_skin_changer.m_knife_vector_idx );
					g_Vars.m_global_skin_changer.m_knife_idx = g_KitParser.vecKnifeNames[ g_Vars.m_global_skin_changer.m_knife_vector_idx ].definition_index;
				}

				knifes.clear( );
			}
			GUI::Group::EndGroup( );

			GUI::Group::BeginGroup( XorStr( "glove changer" ), Vector2D( 50, 78 ) );
			{
				for( int i = 0; i < g_KitParser.vecWeapons.size( ); ++i ) {
					auto whatevertheFUCK = g_KitParser.vecWeapons[ i ];

					if( g_Vars.globals.m_iWeaponIndex == whatevertheFUCK.id )
						g_Vars.globals.m_iWeaponIndexSkins = i;
				}

				static bool bOldGlove = g_Vars.m_global_skin_changer.m_glove_changer;
				static int bruh = g_Vars.m_global_skin_changer.m_gloves_vector_idx;
				if( GUI::Controls::Checkbox( XorStr( "override gloves##glove" ), &g_Vars.m_global_skin_changer.m_glove_changer ) ) {

					if( g_KitParser.vecGloveNames.at( g_Vars.m_global_skin_changer.m_gloves_vector_idx ).definition_index != g_Vars.m_global_skin_changer.m_gloves_idx ) {
						auto it = std::find_if( g_KitParser.vecGloveNames.begin( ), g_KitParser.vecGloveNames.end( ), [&] ( const WeaponName_t &a ) {
							return a.definition_index == g_Vars.m_global_skin_changer.m_gloves_idx;
						} );

						if( on_cfg_load_gloves ) {
							if( it != g_KitParser.vecGloveNames.end( ) )
								g_Vars.m_global_skin_changer.m_gloves_vector_idx = std::distance( g_KitParser.vecGloveNames.begin( ), it );
						}
					}

					static std::vector<std::string> gloves;
					for( int i = 0; i < g_KitParser.vecGloveNames.size( ); ++i ) {
						auto whatevertheFUCK = g_KitParser.vecGloveNames[ i ];
						std::string bruh = whatevertheFUCK.name;
						std::transform( bruh.begin( ), bruh.end( ), bruh.begin( ), ::tolower );

						gloves.push_back( bruh.data( ) );
					}

					if( !gloves.empty( ) ) {
						GUI::Controls::Dropdown( XorStr( "glove model" ), gloves, &g_Vars.m_global_skin_changer.m_gloves_vector_idx );
						g_Vars.m_global_skin_changer.m_gloves_idx = g_KitParser.vecGloveNames[ g_Vars.m_global_skin_changer.m_gloves_vector_idx ].definition_index;

						static std::vector<std::string> paint_kits;

						int currentGloveID = 0;
						for( int i = 0; i < g_KitParser.vecWeapons.size( ); ++i ) {
							auto currentGlove = g_KitParser.vecWeapons[ i ];

							if( currentGlove.id == g_Vars.m_global_skin_changer.m_gloves_idx ) {
								currentGloveID = i;
							}
						}

						auto &skin_data = g_Vars.m_skin_changer;
						CVariables::skin_changer_data *skin = nullptr;
						for( size_t i = 0u; i < skin_data.Size( ); ++i ) {
							skin = skin_data[ i ];
							if( skin->m_definition_index == g_Vars.m_global_skin_changer.m_gloves_idx ) {
								break;
							}
						}

						if( currentGloveID != 0 && skin != nullptr ) {
							static int iOldKit = skin->m_paint_kit_index;
							static int iOldKit2 = skin->m_paint_kit_no_filter;

							auto &current_weapon = g_KitParser.vecWeapons[ currentGloveID ];

							if( on_cfg_load_gloves ) {
								auto &kit = current_weapon.m_kits[ skin->m_paint_kit_index ];
								if( kit.id != skin->m_paint_kit ) {
									auto it = std::find_if( current_weapon.m_kits.begin( ), current_weapon.m_kits.end( ), [skin] ( paint_kit &a ) {
										return a.id == skin->m_paint_kit;
									} );

									if( it != current_weapon.m_kits.end( ) )
										skin->m_paint_kit_index = std::distance( current_weapon.m_kits.begin( ), it );

									skin->m_paint_kit = current_weapon.m_kits[ skin->m_paint_kit_index ].id;
								}

								on_cfg_load_gloves = false;
							}

							skin->m_filter_paint_kits = true;

							GUI::Controls::Checkbox( XorStr( "filter by gloves" ), &skin->m_filter_paint_kits );
							//if( skin->m_filter_paint_kits ) {
							static std::vector<std::string> paint_kits;
							// rebuilding this was ~3 heap allocations per kit per frame
							// (by-value kit copy, lowercase string, push_back). the kit
							// list only changes when the selected glove does.
							static int iLastKitsGlove = -1;
							if( iLastKitsGlove != currentGloveID ) {
								iLastKitsGlove = currentGloveID;
								paint_kits.clear( );

								for( int i = 0; i < current_weapon.m_kits.size( ); ++i ) {
									std::string bruh = current_weapon.m_kits[ i ].name.data( );
									std::transform( bruh.begin( ), bruh.end( ), bruh.begin( ), ::tolower );

									paint_kits.push_back( bruh );
								}
							}

							if( !paint_kits.empty( ) ) {
								GUI::Controls::Listbox( XorStr( "glove paint kit" ), paint_kits, &skin->m_paint_kit_index, true, 7 );
							}
							//}
							/*else {
								if( !g_Vars.globals.m_vecGloveKits.empty( ) ) {
									GUI::Controls::Listbox( XorStr( "glove paint kit" ), g_Vars.globals.m_vecGloveKits, &skin->m_paint_kit_no_filter, true, 7 );
								}
							}*/

							float flOldWear = skin->m_wear;
							float flOldSeed = skin->m_seed;
							GUI::Controls::Slider( XorStr( "paint kit quality#Glove" ), &skin->m_wear, 1.f, 100.f, XorStr( "%.0f%%" ) );
							GUI::Controls::Slider( XorStr( "paint kit seed#Glove" ), &skin->m_seed, 1, 1000, XorStr( "%d" ) );

							if( flOldWear != skin->m_wear || flOldSeed != skin->m_seed ) {
								g_Vars.m_global_skin_changer.m_update_gloves = true;
								flOldWear = skin->m_wear;
								flOldSeed = skin->m_seed;
							}

							skin->m_paint_kit = current_weapon.m_kits[ skin->m_paint_kit_index ].id;
							skin->m_enabled = true;

							if( iOldKit != skin->m_paint_kit_index || iOldKit2 != skin->m_paint_kit_no_filter ) {
								g_Vars.m_global_skin_changer.m_update_gloves = true;
								iOldKit = skin->m_paint_kit_index;
								iOldKit2 = skin->m_paint_kit_no_filter;
							}

							paint_kits.clear( );
						}
					}

					gloves.clear( );
				}

				if( bruh != g_Vars.m_global_skin_changer.m_gloves_vector_idx || bOldGlove != g_Vars.m_global_skin_changer.m_glove_changer ) {
					g_Vars.m_global_skin_changer.m_update_gloves = true;

					bruh = g_Vars.m_global_skin_changer.m_gloves_vector_idx;
					bOldGlove = g_Vars.m_global_skin_changer.m_glove_changer;
				}

				GUI::Group::EndGroup( );
			}

			GUI::Group::BeginGroup( XorStr( "skin changer" ), Vector2D( 50, 100 ) );
			{
				g_Vars.m_global_skin_changer.m_active = true;

				auto &current_weapon = g_KitParser.vecWeapons[ /*weapon_id*/g_Vars.globals.m_iWeaponIndexSkins == -1 ? 0 : g_Vars.globals.m_iWeaponIndexSkins ];
				auto idx = g_Vars.globals.m_iWeaponIndex == -1 ? 7 : g_Vars.globals.m_iWeaponIndex;

				auto &skin_data = g_Vars.m_skin_changer;
				CVariables::skin_changer_data *skin = nullptr;
				for( size_t i = 0u; i < skin_data.Size( ); ++i ) {
					skin = skin_data[ i ];
					if( skin->m_definition_index == idx ) {
						break;
					}
				}

				static auto nOldWeapon = idx;

				if( skin ) {
					GUI::Controls::Checkbox( XorStr( "enabled##Skins" ), &skin->m_enabled );

					bool bOldStatrak = skin->m_stat_trak;
					GUI::Controls::Checkbox( XorStr( "stattrak" ), &skin->m_stat_trak );

					float flOldWear = skin->m_wear;
					float flOldSeed = skin->m_seed;
					GUI::Controls::Slider( XorStr( "weapon paint kit quality" ), &skin->m_wear, 1.f, 100.f, XorStr( "%.0f%%" ) );
					GUI::Controls::Slider( XorStr( "weapon paint kit seed" ), &skin->m_seed, 1, 1000, XorStr( "%d" ) );

					if( GUI::Controls::Checkbox( XorStr( "change weapon paint kit" ), &skin->m_change_paint_kit ) ) {
						if( GUI::Controls::Checkbox( XorStr( "change weapon paint kit color" ), &skin->m_custom_color ) ) {
							GUI::Controls::Label( XorStr( "weapon paint kit color" ) );
							GUI::Controls::ColorPicker( XorStr( "weapon paint kit color 1" ), &skin->color_1, false, false, 0 );
							GUI::Controls::ColorPicker( XorStr( "weapon paint kit color 2" ), &skin->color_2, false, false, 1 );
							GUI::Controls::ColorPicker( XorStr( "weapon paint kit color 3" ), &skin->color_3, false, false, 2 );
							GUI::Controls::ColorPicker( XorStr( "weapon paint kit color 4" ), &skin->color_4, false, false, 3 );

							GUI::Controls::Button( XorStr( "set default paint kit colors" ), [ & ]( ) -> void {
								skin->m_set_color = true;
								skin->m_reset_color = true;
							} );
						}

						if( GUI::Controls::Checkbox( XorStr( "change weapon paint kit phong" ), &skin->m_change_phong ) ) {
							GUI::Controls::Slider( XorStr( "phong exponent" ), &skin->m_phong_exponent, 1.f, 255.f, XorStr( "%.0f%%" ) );
							GUI::Controls::Slider( XorStr( "phong albedo boost" ), &skin->m_phong_albedo_boost, 1.f, 255.f, XorStr( "%.0f%%" ) );
							GUI::Controls::Slider( XorStr( "phong intensity" ), &skin->m_phong_intensity, 1.f, 255.f, XorStr( "%.0f%%" ) );
						}
					}

					float el_hash = float( skin->m_change_paint_kit ) +
						skin->color_1.r + skin->color_1.g + skin->color_1.b +
						skin->color_2.r + skin->color_2.g + skin->color_2.b +
						skin->color_3.r + skin->color_3.g + skin->color_3.b +
						skin->color_4.r + skin->color_4.g + skin->color_4.b + float( skin->m_custom_color )
						+ skin->m_phong_exponent + skin->m_phong_albedo_boost + skin->m_phong_intensity + float( skin->m_change_phong );
					static float old_hash = el_hash;

					if( old_hash != el_hash ) {
						if( GUI::ctx->ColorPickerInfo.HashedID == 0 ) {
							skin->m_set_color = true;
							old_hash = el_hash;
						}
					}

					//GUI::Controls::Checkbox( XorStr( "Custom color" ), &skin->m_custom_color );
					//if( skin->m_custom_color ) {
					//	int nOffset = 0;
					//	GUI::Controls::ColorPicker( std::string( XorStr( "Color 1" ) ).append( XorStr( "##" ) ).append( std::to_string( idx ) ), &skin->color_1, false );
					//	GUI::Controls::ColorPicker( std::string( XorStr( "Color 2" ) ).append( XorStr( "##" ) ).append( std::to_string( idx ) ), &skin->color_2, false, ++nOffset * 24 );
					//	GUI::Controls::ColorPicker( std::string( XorStr( "Color 3" ) ).append( XorStr( "##" ) ).append( std::to_string( idx ) ), &skin->color_3, false, ++nOffset * 24 );
					//	GUI::Controls::ColorPicker( std::string( XorStr( "Color 4" ) ).append( XorStr( "##" ) ).append( std::to_string( idx ) ), &skin->color_4, false, ++nOffset * 24 );
					//}

					//GUI::Controls::Button( XorStr( "Update skin" ), [ & ]( ) {
					//	g_Vars.m_global_skin_changer.m_update_skins = true;
					//} );

					if( flOldWear != skin->m_wear || flOldSeed != skin->m_seed || bOldStatrak != skin->m_stat_trak ) {
						g_Vars.m_global_skin_changer.m_update_skins = true;
						flOldWear = skin->m_wear;
						flOldSeed = skin->m_seed;

						if( bOldStatrak && !skin->m_stat_trak ) {
							g_Vars.globals.m_bForceFullUpdate = true;
						}

						bOldStatrak = skin->m_stat_trak;
					}

					GUI::Controls::Checkbox( XorStr( "filter paint kit by weapon" ), &skin->m_filter_paint_kits );

					if( skin->m_filter_paint_kits ) {
						auto &kit = current_weapon.m_kits[ skin->m_paint_kit_index ];
						if( kit.id != skin->m_paint_kit ) {
							auto it = std::find_if( current_weapon.m_kits.begin( ), current_weapon.m_kits.end( ), [skin] ( paint_kit &a ) {
								return a.id == skin->m_paint_kit;
							} );

							if( it != current_weapon.m_kits.end( ) )
								skin->m_paint_kit_index = std::distance( current_weapon.m_kits.begin( ), it );
						}
					}

					static int bruh1 = skin->m_paint_kit_index;
					static int bruh2 = skin->m_paint_kit_no_filter;

					if( skin->m_filter_paint_kits ) {
						static std::vector<std::string> paint_kits;
						// rebuilding this was ~3 heap allocations per kit per frame
						// (by-value kit copy, lowercase string, push_back). the kit
						// list only changes when the selected weapon does.
						static int iLastKitsWeapon = -1;
						if( iLastKitsWeapon != g_Vars.globals.m_iWeaponIndexSkins ) {
							iLastKitsWeapon = g_Vars.globals.m_iWeaponIndexSkins;
							paint_kits.clear( );

							for( int i = 0; i < current_weapon.m_kits.size( ); ++i ) {
								std::string bruh = current_weapon.m_kits[ i ].name.data( );
								std::transform( bruh.begin( ), bruh.end( ), bruh.begin( ), ::tolower );

								paint_kits.push_back( bruh );
							}
						}

						if( !paint_kits.empty( ) ) {
							GUI::Controls::Listbox( XorStr( "weapon paint kit" ), paint_kits, &skin->m_paint_kit_index, true, 13 );
						}
					}
					else {
						if( !g_Vars.globals.m_vecPaintKits.empty( ) ) {
							GUI::Controls::Listbox( XorStr( "weapon paint kit" ), g_Vars.globals.m_vecPaintKits, &skin->m_paint_kit_no_filter, true, 13 );
						}
					}

					if( idx != nOldWeapon ) {
						nOldWeapon = idx;
						bruh1 = skin->m_paint_kit_index;
						bruh2 = skin->m_paint_kit_no_filter;
					}

					if( ( bruh1 != skin->m_paint_kit_index ) || ( bruh2 != skin->m_paint_kit_no_filter ) ) {
						g_Vars.m_global_skin_changer.m_update_skins = true;

						if( skin->m_paint_kit_index == 0 )
							g_Vars.m_global_skin_changer.m_update_gloves = true;

						bruh1 = skin->m_paint_kit_index;
						bruh2 = skin->m_paint_kit_no_filter;
					}

					skin->m_paint_kit = skin->m_filter_paint_kits ? current_weapon.m_kits[ skin->m_paint_kit_index ].id : skin->m_paint_kit_no_filter;

					//skin->m_enabled = true;
				}

				GUI::Group::EndGroup( );
			}

			GUI::Form::EndWindow( XorStr( "skinchanger" ) );
		}

		g_Vars.globals.m_bLoadDefaultConfig = false;
	}

	void Draw( ) {
		static int index = 0;

		static bool init = false;
		if( !init ) {
			GUI::pMenu->flLastActivity = -1.f;
			GUI::pSkinchanger->flLastActivity = 1.f;
			init = true;
		}

		// add main window
		Windows_t _this;
		_this.m_pContext = GUI::pMenu;
		_this.m_fnRender = DrawMenu;
		m_vecWindows.push_back( _this );

		if( g_Vars.misc.show_skinchanger || g_Vars.globals.m_bLoadDefaultConfig ) {
			// add skins window
			_this.m_pContext = GUI::pSkinchanger;
			_this.m_fnRender = DrawSkinchanger;
			m_vecWindows.push_back( _this );
		}

		// nothing else to sort
		if( m_vecWindows.size( ) > 1 ) {
			// sort windows in order of latest activity
			std::sort( m_vecWindows.begin( ), m_vecWindows.end( ),
					   [ ] ( const Windows_t &a, const Windows_t &b ) {
				return a.m_pContext->flLastActivity < b.m_pContext->flLastActivity;
			} );
		}

		// render windows now
		for( auto &wnd : m_vecWindows ) {
			GUI::SetContext( wnd.m_pContext );
			wnd.m_fnRender( );
		}

		// everyting renderer, we can clear for next frame
		m_vecWindows.clear( );

		// lol 
		if( g_Vars.globals.menuOpen ) {
			if( InputHelper::Down( VK_LCONTROL ) ) {
				if( InputHelper::Pressed( 'S' ) ) {
					static std::vector<std::string> CFG_LIST;
					CFG_LIST = ConfigManager::GetConfigs( );

					if( g_Vars.globals.m_iSelectedConfig < CFG_LIST.size( ) && g_Vars.globals.m_iSelectedConfig >= 0 ) {
						ConfigManager::SaveConfig( CFG_LIST.at( g_Vars.globals.m_iSelectedConfig ) );

						GUI::ctx->SliderInfo.LastChangeTime.clear( );
						GUI::ctx->SliderInfo.PreviewAnimation.clear( );
						GUI::ctx->SliderInfo.PreviousAmount.clear( );
						GUI::ctx->SliderInfo.ShouldChangeValue.clear( );
						GUI::ctx->SliderInfo.ValueTimer.clear( );
						GUI::ctx->SliderInfo.ValueAnimation.clear( );

						g_EventLog.PushEvent( XorStr( "saved config" ), g_Vars.menu.ascent, true, XorStr( "CONFIG" ) );
					}
				}
			}
		}

		// we not in setup anymore (menu code ran once)
		GUI::ctx->setup = false;
		GUI::pMenu->setup = false;
	}
}
