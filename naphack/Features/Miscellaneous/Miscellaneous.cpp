#include "Miscellaneous.hpp"
#include "../../SDK/variables.hpp"
#include "../../SDK/Classes/Player.hpp"
#include "../../SDK/Classes/weapon.hpp"

#include "../../SDK/Displacement.hpp"

#include "../../Loader/Exports.h"

Miscellaneous g_Misc;

void Miscellaneous::ThirdPerson( ) {
	C_CSPlayer *pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	// for whatever reason overrideview also gets called from the main menu.
	if( !g_pEngine->IsInGame( ) )
		return;

	// no need to do thirdperson
	//if( !g_Vars.misc.third_person ) {
	//	flTraceFraction = flInitialStartingValueOfThirdpersonAnimation;
	//	return;
	//}

	// reset this before doing anything
	g_Misc.m_flThirdpersonTransparency = 1.f;

	// check if we have a local player and he is alive.
	const bool bAlive = !pLocal->IsDead( );
	if( bAlive ) {
		C_WeaponCSBaseGun *pWeapon = ( C_WeaponCSBaseGun * )pLocal->m_hActiveWeapon( ).Get( );

		if( !pWeapon )
			return;

		auto pWeaponData = pWeapon->GetCSWeaponData( );
		if( !pWeaponData.IsValid( ) )
			return;

		if( pWeaponData->m_iWeaponType == WEAPONTYPE_GRENADE ) {
			if( g_Vars.esp.blur_on_grenades )
				g_Misc.m_flThirdpersonTransparency = g_Vars.esp.blur_in_scoped_value / 100.f;

			if( g_Vars.esp.first_person_on_nade ) {
				g_pInput->m_fCameraInThirdPerson = false;
				return;
			}
		}
	}

	bool bDummyThirdperson = g_Vars.esp.third_person && g_Vars.esp.third_person_bind.enabled;

	if( g_Vars.esp.third_person /*&& g_Vars.esp.first_person_dead*/ && !bAlive ) {
		bDummyThirdperson = true;
	}

	// camera should be in thirdperson.
	if( bDummyThirdperson ) {
		// if alive and not in thirdperson already switch to thirdperson.
		if( bAlive ) {
			if( !g_pInput->m_fCameraInThirdPerson )
				g_pInput->m_fCameraInThirdPerson = true;
		}
		// if dead and spectating in firstperson switch to thirdperson.
		else {
			if( pLocal->m_iObserverMode( ) == 4 ) {
				// if in thirdperson, switch to firstperson.
				// we need to disable thirdperson to spectate properly.
				if( g_pInput->m_fCameraInThirdPerson ) {
					g_pInput->m_fCameraInThirdPerson = false;
				}

				pLocal->m_iObserverMode( ) = 5;
			}
		}
	}

	// camera should be in firstperson.
	else if( g_pInput->m_fCameraInThirdPerson ) {
		g_pInput->m_fCameraInThirdPerson = false;
	}

	// if after all of this we are still in thirdperson.
	if( g_pInput->m_fCameraInThirdPerson && bAlive ) {
		// get camera angles.
		QAngle offset;
		g_pEngine->GetViewAngles( offset );

		// get our viewangle's forward directional vector.
		Vector forward;
		Math::AngleVectors( offset, forward );

		// setup thirdperson distance
		offset.z = g_pCVar->FindVar( XorStr( "cam_idealdist" ) )->GetFloat( );

		// fix camera position when fakeducking
		const Vector vecDuckOffset = pLocal->m_vecViewOffset( );

		// start pos.
		const Vector origin = pLocal->GetAbsOrigin( ) + vecDuckOffset;

		// setup trace filter and trace.
		CTraceFilterWorldAndPropsOnly filter;
		CGameTrace tr;

		g_pEngineTrace->TraceRay(
			Ray_t( origin, origin - ( forward * offset.z ), { -16.f, -16.f, -16.f }, { 16.f, 16.f, 16.f } ),
			MASK_NPCWORLDSTATIC,
			( ITraceFilter * )&filter,
			&tr
		);

		// override camera angles.
		g_pInput->m_vecCameraOffset = { offset.x, offset.y, offset.z * tr.fraction };

		// fix local player blinking on servers with sv_cheats set to 0
		pLocal->UpdateVisibilityAllEntities( );
	}
}

void Miscellaneous::Modulation( ) {
	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal || !g_pEngine->IsConnected( ) || !g_pEngine->IsInGame( ) ) {
		m_WallsColor = Color_f( 1.f, 1.f, 1.f, 1.f );
		m_SkyColor = Color_f( 1.f, 1.f, 1.f, 1.f );
		m_PropsColor = Color_f( 1.f, 1.f, 1.f, 1.f );

		return;
	}

	static bool bUpdateWorld = false;
	static bool bUpdateSky = false;
	static bool bUpdateProps = false;

	if( !g_Vars.esp.world_modulation ) {
		m_WallsColor = Color_f( 1.f, 1.f, 1.f, 1.f );
	}

	if( !g_Vars.esp.sky_modulation ) {
		m_SkyColor = Color_f( 1.f, 1.f, 1.f, 1.f );
	}

	if( !g_Vars.esp.prop_modulation ) {
		m_PropsColor = Color_f( 1.f, 1.f, 1.f, 1.f );
	}

	if( !g_Misc.m_vecStaticProps.empty( ) ) {
		if( ( m_PropsColor != g_Vars.esp.prop_modulation_color && g_Vars.esp.prop_modulation ) || ( !g_Vars.esp.prop_modulation && bUpdateProps ) ) {
			bUpdateProps = g_Vars.esp.prop_modulation;
			if( bUpdateProps ) {
				m_PropsColor = g_Vars.esp.prop_modulation_color;
			}

			for( auto &pStaticProp : g_Misc.m_vecStaticProps ) {
				if( !pStaticProp.IsValid( ) )
					continue;

				*( float * )( uintptr_t( pStaticProp.Xor( ) ) + 0xBC ) = m_PropsColor.r;
				*( float * )( uintptr_t( pStaticProp.Xor( ) ) + 0xC0 ) = m_PropsColor.g;
				*( float * )( uintptr_t( pStaticProp.Xor( ) ) + 0xC4 ) = m_PropsColor.b;
				*( float * )( uintptr_t( pStaticProp.Xor( ) ) + 0xC8 ) = m_PropsColor.a;

				if( auto m_pClientAlphaProperty = pStaticProp->m_pClientAlphaProperty( ); m_pClientAlphaProperty ) {
					m_pClientAlphaProperty->SetAlphaModulation( 255 * *( float * )( uintptr_t( pStaticProp.Xor( ) ) + 0xC8 ) );
				}

			}
		}
	}

	if( ( ( m_SkyColor != g_Vars.esp.sky_modulation_color && g_Vars.esp.sky_modulation ) || ( m_WallsColor != g_Vars.esp.world_modulation_color && g_Vars.esp.world_modulation ) ) || ( ( !g_Vars.esp.sky_modulation && bUpdateSky ) || ( !g_Vars.esp.world_modulation && bUpdateWorld ) ) ) {
		std::vector< IMaterial * > vecWorld, vecSky;
		const auto pInvalidMat = g_pMaterialSystem->InvalidMaterial( );
		for( auto i = g_pMaterialSystem->FirstMaterial( ); i != pInvalidMat; i = g_pMaterialSystem->NextMaterial( i ) ) {
			const auto pMaterial = g_pMaterialSystem->GetMaterial( i );
			if( !pMaterial || pMaterial->IsErrorMaterial( ) )
				continue;

			if( !pMaterial->GetName( ) )
				continue;

			//if( pMaterial->m_nFlags( ) & MATERIAL_VARS_IS_PRECACHED )
			//	continue;

			auto matGroup = pMaterial->GetTextureGroupName( );

			if( *matGroup == 'W' ) { // world textures
				if( matGroup[ 4 ] != 'd' )
					continue;

				vecWorld.push_back( pMaterial );
			}
			else if( *matGroup == 'S' ) { // staticprops & skybox
				auto thirdCharacter = matGroup[ 3 ];

				// skybox color..
				if( thirdCharacter == 'B' ) {
					vecSky.push_back( pMaterial );
				}
			}
		}

		if( g_Vars.esp.world_modulation ) {
			m_WallsColor = g_Vars.esp.world_modulation_color;
			bUpdateWorld = true;
		}
		else {
			bUpdateWorld = false;
		}

		if( g_Vars.esp.sky_modulation ) {
			m_SkyColor = g_Vars.esp.sky_modulation_color;
			bUpdateSky = true;
		}
		else {
			bUpdateSky = false;
		}

		for( auto &world : vecWorld ) {
			if( !world )
				continue;

			world->ColorModulate( m_WallsColor.r, m_WallsColor.g, m_WallsColor.b );
			world->AlphaModulate( m_WallsColor.a );

			//world->m_nFlags( ) &= ~MATERIAL_VARS_IS_PRECACHED;
		}

		for( auto &sky : vecSky ) {
			if( !sky )
				continue;

			sky->ColorModulate( m_SkyColor.r, m_SkyColor.g, m_SkyColor.b );
			sky->AlphaModulate( m_SkyColor.a );

			//world->m_nFlags( ) &= ~MATERIAL_VARS_IS_PRECACHED;
		}
	}
}

void Miscellaneous::PreserveKillfeed( ) {
	auto local = C_CSPlayer::GetLocalPlayer( );

	if( !local || !g_pEngine->IsInGame( ) || !g_pEngine->IsConnected( ) ) {
		return;
	}

	static auto bStatus = false;
	static float m_flSpawnTime = local->m_flSpawnTime( );

	auto bSet = false;
	if( m_flSpawnTime != local->m_flSpawnTime( ) || bStatus != g_Vars.misc.preserve_killfeed ) {
		bSet = true;
		bStatus = g_Vars.misc.preserve_killfeed;
		m_flSpawnTime = local->m_flSpawnTime( );
	}

	for( int i = 0; i < g_pDeathNotices->m_vecDeathNotices.Count( ); i++ ) {
		auto cur = &g_pDeathNotices->m_vecDeathNotices[ i ];
		if( !cur ) {
			continue;
		}

		if( local->IsDead( ) || bSet ) {
			if( cur->set != 1.f && !bSet ) {
				continue;
			}

			cur->m_flStartTime = g_pGlobalVars->curtime;
			cur->m_flStartTime -= local->m_iHealth( ) <= 0 ? 2.f : 7.5f;
			cur->set = 2.f;

			continue;
		}

		if( cur->set == 2.f ) {
			continue;
		}

		if( !bStatus ) {
			cur->set = 1.f;
			return;
		}

		if( cur->set == 1.f ) {
			continue;
		}

		if( cur->m_flLifeTimeModifier == 1.5f ) {
			cur->m_flStartTime = FLT_MAX;
		}

		cur->set = 1.f;
	}
}

void Miscellaneous::Clantag( ) {
	static bool bSetClantag = false;
	static auto fnClantagChanged = ( int( __fastcall * )( const char *, const char * ) ) Engine::Displacement.Function.m_uClanTagChange;

	auto SetClantag = [&] ( const char *ASD ) {
		fnClantagChanged( ASD, ASD );
	};

	if( !g_Vars.misc.clantag_changer ) {
		if( bSetClantag ) {
			fnClantagChanged( XorStr( "" ), XorStr( "" ) );
			bSetClantag = false;
		}

		return;
	}

	if( !g_pPrediction->GetUnpredictedGlobals( ) ) {
		if( bSetClantag ) {
			fnClantagChanged( XorStr( "" ), XorStr( "" ) );
			bSetClantag = false;
		}

		return;
	}

	bSetClantag = true;

	static int prevtime;
	static int prevval;

	if( int( g_pPrediction->GetUnpredictedGlobals( )->curtime * 3.1 ) != prevtime ) {
		if( prevval != int( g_pPrediction->GetUnpredictedGlobals( )->curtime * 3.1 ) % 31 ) {
			prevval = int( g_pPrediction->GetUnpredictedGlobals( )->curtime * 3.1 ) % 31;
			// 31 frames, played in ascending order:
			//   0 - 12  scroll out to the left, one character per frame
			//      13  blank
			//  14 - 26  scroll back in from the right
			//  27 - 28  drift left to the resting position
			//  29 - 30  unused, so the full tag holds for two frames
			// every frame is padded to exactly 15 characters, which is the most
			// csgo will accept without truncating it.
			switch( int( g_pPrediction->GetUnpredictedGlobals( )->curtime * 3.1 ) % 31 ) {
				case 28: { SetClantag( "blootoothYzio  " );  break; }
				case 27: { SetClantag( " blootoothYzio " );  break; }
				case 26: { SetClantag( "  blootoothYzio" );  break; }
				case 25: { SetClantag( "   blootoothYzi" );  break; }
				case 24: { SetClantag( "    blootoothYz" );  break; }
				case 23: { SetClantag( "     blootoothY" );  break; }
				case 22: { SetClantag( "      blootooth" );  break; }
				case 21: { SetClantag( "       blootoot" );  break; }
				case 20: { SetClantag( "        blootoo" );  break; }
				case 19: { SetClantag( "         blooto" );  break; }
				case 18: { SetClantag( "          bloot" );  break; }
				case 17: { SetClantag( "           bloo" );  break; }
				case 16: { SetClantag( "            blo" );  break; }
				case 15: { SetClantag( "             bl" );  break; }
				case 14: { SetClantag( "              b" );  break; }
				case 13: { SetClantag( "               " );  break; }
				case 12: { SetClantag( "o              " );  break; }
				case 11: { SetClantag( "io             " );  break; }
				case 10: { SetClantag( "zio            " );  break; }
				case 9: { SetClantag( "Yzio           " );  break; }
				case 8: { SetClantag( "hYzio          " );  break; }
				case 7: { SetClantag( "thYzio         " );  break; }
				case 6: { SetClantag( "othYzio        " );  break; }
				case 5: { SetClantag( "oothYzio       " );  break; }
				case 4: { SetClantag( "toothYzio      " );  break; }
				case 3: { SetClantag( "otoothYzio     " );  break; }
				case 2: { SetClantag( "ootoothYzio    " );  break; }
				case 1: { SetClantag( "lootoothYzio   " );  break; }
				case 0: { SetClantag( "blootoothYzio  " );  break; }
				default:;
			}
		}
	}

	prevtime = int( g_pPrediction->GetUnpredictedGlobals( )->curtime );
}

void Miscellaneous::RemoveSmoke( ) {
	if( !g_pEngine->IsInGame( ) )
		return;

	static uint32_t *pSmokeCount = nullptr;
	if( !pSmokeCount ) {
	#ifndef DEV
		pSmokeCount = *reinterpret_cast< uint32_t ** > ( Engine::Displacement.Data.m_uSmokeCount );
	#else
		pSmokeCount = *reinterpret_cast< uint32_t ** > ( Engine::Displacement.Data.m_uSmokeCount );
	#endif 
	}

	if( g_Vars.esp.remove_smoke ) {
		*pSmokeCount = 0;
	}

	static IMaterial *pSmokeGrenade = nullptr;
	if( !pSmokeGrenade ) {
		pSmokeGrenade = g_pMaterialSystem->FindMaterial( XorStr( "particle/vistasmokev1/vistasmokev1_smokegrenade" ), nullptr );
	}

	static IMaterial *pSmokeFire = nullptr;
	if( !pSmokeFire ) {
		pSmokeFire = g_pMaterialSystem->FindMaterial( XorStr( "particle/vistasmokev1/vistasmokev1_fire" ), nullptr );
	}

	static IMaterial *pSmokeDust = nullptr;
	if( !pSmokeDust ) {
		pSmokeDust = g_pMaterialSystem->FindMaterial( XorStr( "particle/vistasmokev1/vistasmokev1_emods_impactdust" ), nullptr );
	}

	static IMaterial *pSmokeMods = nullptr;
	if( !pSmokeMods ) {
		pSmokeMods = g_pMaterialSystem->FindMaterial( XorStr( "particle/vistasmokev1/vistasmokev1_emods" ), nullptr );
	}

	if( !pSmokeGrenade || !pSmokeFire || !pSmokeDust || !pSmokeMods ) {
		return;
	}

	pSmokeGrenade->SetMaterialVarFlagSane( MATERIAL_VAR_NO_DRAW, g_Vars.esp.remove_smoke );
	// pSmokeGrenade->IncrementReferenceCount( );

	pSmokeFire->SetMaterialVarFlag( MATERIAL_VAR_NO_DRAW, g_Vars.esp.remove_smoke );
	// pSmokeFire->IncrementReferenceCount( );

	pSmokeDust->SetMaterialVarFlag( MATERIAL_VAR_NO_DRAW, g_Vars.esp.remove_smoke );
	// pSmokeDust->IncrementReferenceCount( );

	pSmokeMods->SetMaterialVarFlag( MATERIAL_VAR_NO_DRAW, g_Vars.esp.remove_smoke );
	// pSmokeMods->IncrementReferenceCount( );

	static auto mat_postprocess_enable = g_pCVar->FindVar( XorStr( "mat_postprocess_enable" ) );
	mat_postprocess_enable->SetValueInt( ( int )( !g_Vars.esp.remove_post_proccesing ) );

	// always remove the scope blur, so you can have post processing with no scope blur
	//if( !g_Vars.esp.remove_post_proccesing )
	//	return;

	enum PostProcessParameterNames_t {
		PPPN_FADE_TIME = 0,
		PPPN_LOCAL_CONTRAST_STRENGTH,
		PPPN_LOCAL_CONTRAST_EDGE_STRENGTH,
		PPPN_VIGNETTE_START,
		PPPN_VIGNETTE_END,
		PPPN_VIGNETTE_BLUR_STRENGTH,
		PPPN_FADE_TO_BLACK_STRENGTH,
		PPPN_DEPTH_BLUR_FOCAL_DISTANCE,
		PPPN_DEPTH_BLUR_STRENGTH,
		PPPN_SCREEN_BLUR_STRENGTH,
		PPPN_FILM_GRAIN_STRENGTH,

		POST_PROCESS_PARAMETER_COUNT
	};

	struct PostProcessParameters_t {
		PostProcessParameters_t( ) {
			memset( m_flParameters, 0, sizeof( m_flParameters ) );
			m_flParameters[ PPPN_VIGNETTE_START ] = 0.8f;
			m_flParameters[ PPPN_VIGNETTE_END ] = 1.1f;
		}

		float m_flParameters[ POST_PROCESS_PARAMETER_COUNT ];
	};

#ifndef DEV
	const static auto PostProcessParameters = *reinterpret_cast< PostProcessParameters_t ** >( Engine::Displacement.Data.m_PostProcessParameters );
#else
	const static auto PostProcessParameters = *reinterpret_cast< PostProcessParameters_t ** >( Engine::Displacement.Data.m_PostProcessParameters );
#endif

	if( PostProcessParameters->m_flParameters[ PPPN_VIGNETTE_BLUR_STRENGTH ] != 0.f )
		PostProcessParameters->m_flParameters[ PPPN_VIGNETTE_BLUR_STRENGTH ] = 0.f;

	if( PostProcessParameters->m_flParameters[ PPPN_SCREEN_BLUR_STRENGTH ] != 0.f )
		PostProcessParameters->m_flParameters[ PPPN_SCREEN_BLUR_STRENGTH ] = 0.f;
}

void Miscellaneous::SkyBoxChanger( ) {
	static int iOldSky = 0;

	if( !g_pEngine->IsInGame( ) || !g_pEngine->IsConnected( ) ) {
		iOldSky = 0;
		return;
	}

	const auto pLocal = C_CSPlayer::GetLocalPlayer( );

	if( !pLocal )
		return;

	static auto fnLoadNamedSkys = ( void( __fastcall * )( const char * ) )Engine::Displacement.Function.m_uLoadNamedSkys;
	static ConVar *sv_skyname = g_pCVar->FindVar( XorStr( "sv_skyname" ) );
	static ConVar *r_3dsky = g_pCVar->FindVar( XorStr( "r_3dsky" ) );
	static ConVar *fog_enableskybox = g_pCVar->FindVar( XorStr( "fog_enableskybox" ) );
	static ConVar *fog_override = g_pCVar->FindVar( XorStr( "fog_override" ) );
	static ConVar *fog_enable = g_pCVar->FindVar( XorStr( "fog_enable" ) );

	if( r_3dsky ) {
		if( r_3dsky->fnChangeCallback.m_Size != 0 ) {
			r_3dsky->fnChangeCallback.m_Size = 0;
		}
	}

	static bool bReset = false;
	if( fog_enableskybox ) {
		if( fog_enableskybox->fnChangeCallback.m_Size != 0 ) {
			fog_enableskybox->fnChangeCallback.m_Size = 0;
		}

		if( fog_enableskybox->GetInt( ) != 0 && g_Vars.esp.remove_fog ) {
			fog_enableskybox->SetValue( 0 );
			bReset = true;
		}
		else {
			if( fog_enableskybox->GetInt( ) != 1 && !g_Vars.esp.remove_fog && bReset ) {
				fog_enableskybox->SetValue( 1 );
			}
		}
	}

	if( fog_override ) {
		if( fog_override->fnChangeCallback.m_Size != 0 ) {
			fog_override->fnChangeCallback.m_Size = 0;
		}

		if( fog_override->GetInt( ) != 1 && g_Vars.esp.remove_fog ) {
			fog_override->SetValue( 1 );
		}
		else {
			if( fog_override->GetInt( ) != 0 && !g_Vars.esp.remove_fog && bReset ) {
				fog_override->SetValue( 0 );
			}
		}
	}

	if( fog_enable ) {
		if( fog_enable->fnChangeCallback.m_Size != 0 ) {
			fog_enable->fnChangeCallback.m_Size = 0;
		}

		if( fog_enable->GetInt( ) != 0 && g_Vars.esp.remove_fog ) {
			fog_enable->SetValue( 0 );
		}
		else {
			if( fog_enable->GetInt( ) != 1 && !g_Vars.esp.remove_fog && bReset ) {
				fog_enable->SetValue( 1 );

				bReset = false;
			}
		}
	}

	if( sv_skyname && r_3dsky ) {
		static std::string skybox = g_Vars.esp.sky_changer_name;
		if( skybox != g_Vars.esp.sky_changer_name ) {
			if( g_Vars.esp.sky_changer == 15 )
				iOldSky = 0;

			skybox = g_Vars.esp.sky_changer_name;
		}

		if( iOldSky != g_Vars.esp.sky_changer ) {

			iOldSky = g_Vars.esp.sky_changer;

			switch( g_Vars.esp.sky_changer ) {
				case 0:
					fnLoadNamedSkys( sv_skyname->GetString( ) );
					break;
				case 1:
					fnLoadNamedSkys( XorStr( "cs_baggage_skybox_" ) );
					break;
				case 2:
					fnLoadNamedSkys( XorStr( "cs_tibet" ) );
					break;
				case 3:
					fnLoadNamedSkys( XorStr( "embassy" ) );
					break;
				case 4:
					fnLoadNamedSkys( XorStr( "italy" ) );
					break;
				case 5:
					fnLoadNamedSkys( XorStr( "jungle" ) );
					break;
				case 6:
					fnLoadNamedSkys( XorStr( "nukeblank" ) );
					break;
				case 7:
					fnLoadNamedSkys( XorStr( "office" ) );
					break;
				case 8:
					fnLoadNamedSkys( XorStr( "sky_csgo_cloudy01" ) );
					break;
				case 9:
					fnLoadNamedSkys( XorStr( "sky_csgo_night02" ) );
					break;
				case 10:
					fnLoadNamedSkys( XorStr( "sky_csgo_night02b" ) );
					break;
				case 11:
					fnLoadNamedSkys( XorStr( "sky_dust" ) );
					break;
				case 12:
					fnLoadNamedSkys( XorStr( "sky_venice" ) );
					break;
				case 13:
					fnLoadNamedSkys( XorStr( "vertigo" ) );
					break;
				case 14:
					fnLoadNamedSkys( XorStr( "vietnam" ) );
					break;
				case 15:
					fnLoadNamedSkys( g_Vars.esp.sky_changer_name.data( ) );
					break;
				default:
					break;
			}

			if( g_Vars.esp.sky_changer != 0 ) {
				// fuck 3d sky !!
				if( r_3dsky->GetInt( ) != 0 ) {
					r_3dsky->SetValue( 0 );
				}
			}
			else {
				r_3dsky->SetValue( 1 );
			}

			// force sky color update
			m_SkyColor = Color_f( 1.f, 1.f, 1.f, 1.f );
		}
	}
}

void Miscellaneous::ForceCrosshair( ) {
	C_CSPlayer *pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal ) {
		return;
	}

	static auto weapon_debug_spread_show = g_pCVar->FindVar( XorStr( "weapon_debug_spread_show" ) );
	if( !weapon_debug_spread_show ) {
		return;
	}

	static bool bReset = false;
	if( !g_Vars.esp.force_sniper_crosshair ) {
		if( bReset ) {
			weapon_debug_spread_show->SetValue( 0 );
			bReset = false;
		}

		return;
	}

	const int nCrosshairValue = pLocal->m_bIsScoped( ) ? 0 : 3;
	if( weapon_debug_spread_show->GetInt( ) != nCrosshairValue )
		weapon_debug_spread_show->SetValue( nCrosshairValue );

	bReset = true;
}

void Miscellaneous::OverrideFOV( CViewSetup *vsView ) {
	C_CSPlayer *pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal ) {
		return;
	}

	C_CSPlayer *pIntendedEnemy = pLocal;
	if( pLocal->EntIndex( ) == g_pEngine->GetLocalPlayer( ) ) {
		if( pLocal->IsDead( ) ) {
			if( pLocal->m_hObserverTarget( ).IsValid( ) ) {
				const auto pSpectator = ( C_CSPlayer * )pLocal->m_hObserverTarget( ).Get( );
				if( pSpectator ) {
					if( pLocal->m_iObserverMode( ) == 4 || pLocal->m_iObserverMode( ) == 5 )
						pIntendedEnemy = pSpectator;
				}
			}
		}
	}

	if( !pIntendedEnemy ) {
		return;
	}

	auto pWeapon = ( C_WeaponCSBaseGun * )( pIntendedEnemy->m_hActiveWeapon( ).Get( ) );
	if( !pWeapon ) {
		return;
	}

	// we want to set our fov to the slider value, let's see
	// what our fov is right now and see how much we have left
	// till our wish fov

	float flFovLeft = g_Vars.misc.override_fov - vsView->fov;
	float flOverrideFovScope = g_Vars.misc.override_fov_scope;

	if( pLocal->IsDead( ) )
		flOverrideFovScope = 0.f;

	if( g_Vars.esp.remove_scope_zoom && pWeapon->m_zoomLevel( ) == 1 )
		flOverrideFovScope = 0.f;

	if( pIntendedEnemy->m_bIsScoped( ) ) {
		flFovLeft *= ( ( 100.f - flOverrideFovScope ) / 100.f );
	}

	vsView->fov += flFovLeft;
}

void Miscellaneous::PerformConvarRelated( ) {
	static auto con_filter_text = g_pCVar->FindVar( "con_filter_text" );
	static auto con_filter_text_out = g_pCVar->FindVar( "con_filter_text_out" );
	static auto con_filter_enable = g_pCVar->FindVar( "con_filter_enable" );
	static auto contimes = g_pCVar->FindVar( "contimes" );

	static bool filter = false;
	if( g_Vars.misc.filter_console && !filter ) {
		con_filter_text->SetValue( " " );
		con_filter_text_out->SetValue( " " );
		con_filter_enable->SetValue( 2 );

		g_pEngine->ClientCmd_Unrestricted( "clear" );
		filter = true;
	}

	if( !g_Vars.misc.filter_console && filter ) {
		con_filter_text->SetValue( "" );
		con_filter_text_out->SetValue( "" );
		con_filter_enable->SetValue( 0 );

		g_pEngine->ClientCmd_Unrestricted( "clear" );
		filter = false;
	}

	UnlockConVars( );

	if( g_pEngine->GetNetChannelInfo( ) ) {
		static bool bJoinedDicks = false;
		const bool bIsDicks = strstr( g_pEngine->GetNetChannelInfo( )->GetAddress( ), XorStr( "91.229.114.127" ) );
		if( !bJoinedDicks && bIsDicks ) {
			g_Vars.sv_maxunlag->SetValueFloat( 0.6f );
			bJoinedDicks = true;
		}

		if( bJoinedDicks && !bIsDicks ) {
			g_Vars.sv_maxunlag->SetValueFloat( 1.0f );
			bJoinedDicks = false;
		}

		static bool bJoinedMBot = false;
		const bool bIsMBot = strstr( g_pEngine->GetNetChannelInfo( )->GetAddress( ), XorStr( "74.91.119.53" ) );
		if( !bJoinedMBot && bIsMBot ) {
			g_Vars.sv_maxunlag->SetValueFloat( 0.3f );
			bJoinedMBot = true;
		}

		if( bJoinedMBot && !bIsMBot ) {
			g_Vars.sv_maxunlag->SetValueFloat( 1.0f );
			bJoinedMBot = false;
		}
	}

	// bypass anti-cl_lagcomp 0 plugins
	if( g_Vars.misc.cl_lagcomp_bypass && g_Vars.cl_lagcompensation ) {
		if( !( g_Vars.cl_lagcompensation->nFlags & ( 1 << 29 ) ) ) {
			// FCVAR_SERVER_CANNOT_QUERY
			g_Vars.cl_lagcompensation->nFlags |= ( 1 << 29 );
		}
	}
	else {
		if( g_Vars.cl_lagcompensation ) {
			if( g_Vars.cl_lagcompensation->nFlags & ( 1 << 29 ) ) {
				g_Vars.cl_lagcompensation->nFlags &= ~( 1 << 29 );
			}
		}
	}

	if( g_Vars.cl_pred_optimize->GetInt( ) != 0 ) {
		g_Vars.cl_pred_optimize->SetValueInt( 0 );
	}


	// rate 786432 

	static auto rate = g_pCVar->FindVar( XorStr( "rate" ) );
	if( rate && rate->GetInt( ) != 786432 ) {
		rate->SetValueInt( 786432 );
	}

	if( g_Vars.cl_extrapolate ) {
		if( g_Vars.cl_extrapolate->GetInt( ) != 0 ) {
			g_Vars.cl_extrapolate->SetValue( 0 );
		}
	}

	// fix chams lightning
	static auto mat_force_tonemap_scale = g_pCVar->FindVar( XorStr( "mat_force_tonemap_scale" ) );

	if( mat_force_tonemap_scale ) {
		if( mat_force_tonemap_scale->fnChangeCallback.m_Size != 0 ) {
			mat_force_tonemap_scale->fnChangeCallback.m_Size = 0;
		}

		if( mat_force_tonemap_scale->GetFloat( ) != 1.f ) {
			mat_force_tonemap_scale->SetValueFloat( 1.f );
		}
	}

	static auto sleep_when_meeting_framerate = g_pCVar->FindVar( XorStr( "sleep_when_meeting_framerate" ) );

	if( sleep_when_meeting_framerate ) {
		if( sleep_when_meeting_framerate->fnChangeCallback.m_Size != 0 ) {
			sleep_when_meeting_framerate->fnChangeCallback.m_Size = 0;
		}

		if( sleep_when_meeting_framerate->GetInt( ) != 0 ) {
			sleep_when_meeting_framerate->SetValueInt( 0 );
		}
	}

	// improve networking :nerd:
	//static auto net_earliertempents = g_pCVar->FindVar( XorStr( "net_earliertempents" ) );
	//static auto cl_ignorepackets = g_pCVar->FindVar( XorStr( "cl_ignorepackets" ) );

	//// https://github.com/perilouswithadollarsign/cstrike15_src/blob/f82112a2388b841d72cb62ca48ab1846dfcc11c8/engine/cl_main.cpp#L509-L517
	//if( net_earliertempents && cl_ignorepackets ) {
	//	if( net_earliertempents->fnChangeCallback.m_Size != 0 ) {
	//		net_earliertempents->fnChangeCallback.m_Size = 0;
	//	}

	//	if( cl_ignorepackets->fnChangeCallback.m_Size != 0 ) {
	//		cl_ignorepackets->fnChangeCallback.m_Size = 0;
	//	}

	//	net_earliertempents->SetValueInt( 1 );
	//	cl_ignorepackets->SetValueInt( 0 );
	//}

	//static auto cl_pred_doresetlatch = g_pCVar->FindVar( XorStr( "cl_pred_doresetlatch" ) );
	//if( cl_pred_doresetlatch ) {
	//	// FUCK THIS
	//	if( cl_pred_doresetlatch->GetBool( ) ) {
	//		cl_pred_doresetlatch->SetValueInt( 0 );
	//	}
	//}

	// sunset mode
	static bool bDisable = true;
	if( g_Vars.esp.sunset_mode ) {
		if( bDisable ) {
			if( g_Vars.cl_csm_max_shadow_dist->GetFloat( ) != 650.f )
				bDisable = false;

			if( g_Vars.cl_csm_rot_x->GetFloat( ) != 15 )
				bDisable = false;
		}

		if( !bDisable ) {
			// sucks but w/e
			g_Vars.cl_csm_shadows->SetValue( 1 );

			// https://github.com/perilouswithadollarsign/cstrike15_src/blob/f82112a2388b841d72cb62ca48ab1846dfcc11c8/game/client/c_env_cascade_light.cpp#L1475
			// i played around with these until it looked nice
			g_Vars.cl_csm_max_shadow_dist->SetValue( 650.f );
			g_Vars.cl_csm_rot_override->SetValue( 1 );

			g_Vars.cl_csm_rot_x->SetValue( 15 );
			g_Vars.cl_csm_rot_y->SetValue( 0 );
			g_Vars.cl_csm_rot_z->SetValue( 0 );

			bDisable = true;
		}
	}
	else {
		if( bDisable ) {
			g_Vars.cl_csm_max_shadow_dist->SetValue( 0.f );
			g_Vars.cl_csm_rot_override->SetValue( 0 );

			bDisable = false;
		}
	}

	// ambient lightning
	static bool bReset = false;
	if( g_Vars.esp.ambient_ligtning ) {
		bReset = false;
		if( g_Vars.mat_ambient_light_r != nullptr && g_Vars.mat_ambient_light_g != nullptr && g_Vars.mat_ambient_light_b != nullptr ) {
			const float flAlpha = g_Vars.esp.ambient_ligtning_color.a;

			if( g_Vars.mat_ambient_light_r->GetFloat( ) != ( g_Vars.esp.ambient_ligtning_color.r ) * flAlpha )
				g_Vars.mat_ambient_light_r->SetValueFloat( ( g_Vars.esp.ambient_ligtning_color.r ) * flAlpha );

			if( g_Vars.mat_ambient_light_g->GetFloat( ) != ( g_Vars.esp.ambient_ligtning_color.g ) * flAlpha )
				g_Vars.mat_ambient_light_g->SetValueFloat( ( g_Vars.esp.ambient_ligtning_color.g ) * flAlpha );

			if( g_Vars.mat_ambient_light_b->GetFloat( ) != ( g_Vars.esp.ambient_ligtning_color.b ) * flAlpha )
				g_Vars.mat_ambient_light_b->SetValueFloat( ( g_Vars.esp.ambient_ligtning_color.b ) * flAlpha );
		}
	}
	else {
		if( !bReset ) {
			if( g_Vars.mat_ambient_light_r != nullptr && g_Vars.mat_ambient_light_g != nullptr && g_Vars.mat_ambient_light_b != nullptr ) {

				g_Vars.mat_ambient_light_r->SetValueFloat( 0.f );
				g_Vars.mat_ambient_light_g->SetValueFloat( 0.f );
				g_Vars.mat_ambient_light_b->SetValueFloat( 0.f );
			}

			bReset = true;
		}
	}

	// fog modulation
	static bool bResetFog = false;
	static bool bFirstSet = true;
	if( g_Vars.esp.fog_modulation ) {
		bResetFog = false;
		if( g_Vars.fog_override != nullptr && g_Vars.fog_start != nullptr && g_Vars.fog_startskybox != nullptr && g_Vars.fog_end != nullptr && g_Vars.fog_endskybox != nullptr && g_Vars.fog_maxdensity != nullptr && g_Vars.maxdensityskybox != nullptr && g_Vars.fog_color != nullptr && g_Vars.fog_colorskybox != nullptr ) {
			// override
			if( g_Vars.fog_override->GetInt( ) != 1 )
				g_Vars.fog_override->SetValue( 1 );

			// start
			if( g_Vars.fog_start->GetFloat( ) != g_Vars.esp.fog_start_s )
				g_Vars.fog_start->SetValueFloat( g_Vars.esp.fog_start_s );

			if( g_Vars.fog_startskybox->GetFloat( ) != g_Vars.esp.fog_start_s )
				g_Vars.fog_startskybox->SetValueFloat( g_Vars.esp.fog_start_s );

			// end
			if( g_Vars.fog_end->GetFloat( ) != g_Vars.esp.fog_end_s )
				g_Vars.fog_end->SetValueFloat( g_Vars.esp.fog_end_s );

			if( g_Vars.fog_endskybox->GetFloat( ) != g_Vars.esp.fog_end_s )
				g_Vars.fog_endskybox->SetValueFloat( g_Vars.esp.fog_end_s );

			// density
			if( g_Vars.fog_maxdensity->GetFloat( ) != g_Vars.esp.fog_modulation_color.a )
				g_Vars.fog_maxdensity->SetValueFloat( g_Vars.esp.fog_modulation_color.a );

			if( g_Vars.maxdensityskybox->GetFloat( ) != g_Vars.esp.fog_modulation_color.a )
				g_Vars.maxdensityskybox->SetValueFloat( g_Vars.esp.fog_modulation_color.a );

			// color
			char colbuf[ 128 ] = {};
			sprintf( colbuf, XorStr( "%i %i %i" ),
					 int( g_Vars.esp.fog_modulation_color.r * 255.f ),
					 int( g_Vars.esp.fog_modulation_color.g * 255.f ),
					 int( g_Vars.esp.fog_modulation_color.b * 255.f ) );

			static Color_f oldColor = g_Vars.esp.fog_modulation_color;
			const Color_f curColor = g_Vars.esp.fog_modulation_color;

			if( oldColor != curColor || bFirstSet ) {
				g_Vars.fog_color->SetValue( colbuf );
				g_Vars.fog_colorskybox->SetValue( colbuf );
				oldColor = curColor;
				bFirstSet = false;
			}
		}
	}
	else {
		if( !bResetFog ) {
			if( g_Vars.fog_override != nullptr )
				g_Vars.fog_override->SetValue( 0 );

			bResetFog = true;
			bFirstSet = true;
		}
	}

	// fps fix when tabbed out
	if( g_Vars.engine_no_focus_sleep && g_Vars.engine_no_focus_sleep->GetInt( ) != 0 )
		g_Vars.engine_no_focus_sleep->SetValue( 0 );

	// jiggling attachments roflrofl
	if( g_Vars.r_jiggle_bones ) {
		if( g_Vars.r_jiggle_bones->fnChangeCallback.m_Size != 0 ) {
			g_Vars.r_jiggle_bones->fnChangeCallback.m_Size = 0;
		}

		if( g_Vars.r_jiggle_bones->GetInt( ) != 0 ) {
			g_Vars.r_jiggle_bones->SetValue( 0 );
		}
	}

}

void Miscellaneous::UnlockConVars( ) {
	struct ModifiedConVar_t {
		ConVar *m_pCVar;
		bool m_bUnlockedFirst = false;
		bool m_bUnlockedSecond = false;
	};

	static std::vector<ModifiedConVar_t> vecModifiedConVars{ };
	static bool bUnlockedOnLoad = false;

	// unlock hidden cvars
	if( !bUnlockedOnLoad ) {
		auto p = **reinterpret_cast< ConVar *** >( ( uint32_t )g_pCVar.Xor( ) + 0x34 );
		for( ConVar *c = p->pNext; c != nullptr; c = c->pNext ) {
			ModifiedConVar_t temp{ };
			temp.m_pCVar = c;

			if( c->nFlags & ( 1 << 4 ) ) {
				c->nFlags &= ~( 1 << 4 );

				temp.m_bUnlockedFirst = true;
			}
			if( c->nFlags & ( 1 << 1 ) ) {
				c->nFlags &= ~( 1 << 1 );

				temp.m_bUnlockedSecond = true;
			}

			if( temp.m_bUnlockedFirst || temp.m_bUnlockedSecond ) {
				vecModifiedConVars.push_back( temp );
			}
		}

		bUnlockedOnLoad = true;
	}

#if 0
	if( bUnlockedOnLoad && !g_Vars.misc.unlocked_hidden_cvars ) {
		for( auto &modified : vecModifiedConVars ) {
			if( !modified.m_pCVar )
				continue;

			if( modified.m_bUnlockedFirst ) {
				modified.m_pCVar->nFlags |= ( 1 << 4 );
			}

			if( modified.m_bUnlockedSecond ) {
				modified.m_pCVar->nFlags |= ( 1 << 1 );
			}
		}

		bUnlockedOnLoad = false;

		// clear it :)
		vecModifiedConVars.clear( );
	}
#endif
}

void Miscellaneous::RemoveVisualEffects( ) {
	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	//if( g_Vars.esp.skip_occulusion ) {
		//for( int i = 1; i <= g_pGlobalVars->maxClients; ++i ) {
		//	const auto player = C_CSPlayer::GetPlayerByIndex( i );
		//	if( !player )
		//		continue;

		//	if( player->IsDead( ) || player->IsDormant( ) )
		//		continue;

		//	auto pRenderable = player->GetClientRenderable( );
		//	if( !pRenderable )
		//		continue;

		//	// we disable occlusion for local player by setting translucency type to RENDERABLE_IS_TRANSLUCENT
		//	// in frame_render_start, but that removes the rendering of our shadow, so set it to opaque here
		//	g_pClientLeafSystem->SetTranslucencyType( pRenderable->RenderHandle( ), RENDERABLE_IS_TRANSLUCENT );
		//}
	//}

	auto angAimPunch = pLocal->m_aimPunchAngle( ) * g_Vars.weapon_recoil_scale->GetFloat( ) * g_Vars.view_recoil_tracking->GetFloat( );
	if( g_Vars.esp.remove_recoil_shake ) {
		pLocal->pl( ).v_angle -= pLocal->m_viewPunchAngle( );
	}

	if( g_Vars.esp.remove_recoil_punch ) {
		pLocal->pl( ).v_angle -= angAimPunch;
	}

	pLocal->pl( ).v_angle.Normalize( );

	if( g_Vars.esp.remove_flash ) {
		pLocal->m_flFlashDuration( ) = 0.f;
	}

	// clean
	RemoveSmoke( );
	SkyBoxChanger( );
}

bool Miscellaneous::PrecacheModel( const char *model ) {
	auto pCacheTable = g_pNetworkStringTableContainer->FindTable( XorStr( "modelprecache" ) );
	if( !pCacheTable )
		return false;

	if( !g_pModelInfo->FindOrLoadModel( model ) )
		return false;

	return pCacheTable->AddString( false, model ) != -1;
}
