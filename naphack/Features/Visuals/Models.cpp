#include "Models.hpp"
#include "../../pandora.hpp"
#include <fstream>
#include "../../SDK/Classes/Player.hpp"
#include "../../SDK/Classes/IMaterialSystem.hpp"
#include "../../SDK/variables.hpp"
#include "../Rage/Animations.hpp"
#include "../../Hooking/hooked.hpp"
#include "../../SDK/displacement.hpp"
#include "../Rage/Ragebot.hpp"
#include "../Rage/Resolver.hpp"
#include "../Rage/AntiAim.hpp"
#include "../Rage/EnginePrediction.hpp"
#include "../Rage/ServerAnimations.hpp"
#include "../Rage/BoneSetup.hpp"
#include "../Miscellaneous/Miscellaneous.hpp"
#include "../Visuals/Visuals.hpp"
#include "../Visuals/Glow.hpp"

#include "../Scripting/Wrappers/material.h"
#include "../Scripting/Scripting.hpp"

#include <cstddef>
#include <string_view>
#include <iostream>
#include <string_view>
#include <string>

Models g_Models;

bool Models::InitialiseModelTypes( ) {
	if( m_bInitialised )
		return true;

	{
		std::ofstream( XorStr( "csgo\\materials\\nap_material.vmt" ) ) << XorStr( R"#("VertexLitGeneric"
			{
					"$basetexture"				"vgui/white_additive"
					"$ignorez"					"0"
					"$phong"					"0"
					"$BasemapAlphaPhongMask"    "1"
					"$phongexponent"			"255"
					"$normalmapalphaenvmask"	"1"
					"$envmap"					"env_cubemap"
					"$envmaptint"				"[0.0 0.0 0.0]"
					"$phongboost"				"1.0"
					"phongfresnelranges"		"[0.0 0.0 0.0]"
					"$nofog"					"1"
					"$model"					"1"
					"$nocull"					"0"
					"$selfillum"				"1"
					"$halflambert"				"1"
					"$znearer"					"0"
					"$flat"						"0"	
					"$rimlight"					"1"
					"$rimlightexponent"			"2"
					"$rimlightboost"			"0"
			}
			)#" );
	}
	m_pMaterial = g_pMaterialSystem->FindMaterial( XorStr( "nap_material" ), TEXTURE_GROUP_MODEL );
	if( !m_pMaterial || m_pMaterial == nullptr || m_pMaterial->IsErrorMaterial( ) )
		return false;

	{
		std::ofstream( XorStr( "csgo\\materials\\nap_shiny.vmt" ) ) << XorStr( R"#("VertexLitGeneric"
			{
					"$basetexture"				"vgui/white_additive"
					"$ignorez"					"0"
					"$phong"					"1"
					"$BasemapAlphaPhongMask"    "1"
					"$phongexponent"			"255"
					"$normalmapalphaenvmask"	"1"
					"$envmap"					"env_cubemap"
					"$envmaptint"				"[0.0 0.0 0.0]"
					"$phongboost"				"1.0"
					"phongfresnelranges"		"[0.0 0.0 0.0]"
					"$nofog"					"1"
					"$model"					"1"
					"$nocull"					"0"
					"$selfillum"				"1"
					"$halflambert"				"1"
					"$znearer"					"0"
					"$flat"						"0"	
					"$rimlight"					"1"
					"$rimlightexponent"			"2"
					"$rimlightboost"			"0"
			}
			)#" );
	}
	m_pShiny = g_pMaterialSystem->FindMaterial( XorStr( "nap_shiny" ), TEXTURE_GROUP_MODEL );
	if( !m_pShiny || m_pShiny == nullptr || m_pShiny->IsErrorMaterial( ) )
		return false;

	{
		std::ofstream( XorStr( "csgo\\materials\\nap_flat.vmt" ) ) << XorStr( R"#("UnlitGeneric"
			{
					"$basetexture"				"vgui/white_additive"
					"$ignorez"					"0"
					"$phong"					"1"
					"$BasemapAlphaPhongMask"    "1"
					"$phongexponent"			"15"
					"$normalmapalphaenvmask"	"1"
					"$envmap"					"env_cubemap"
					"$envmaptint"				"[0.0 0.0 0.0]"
					"$phongboost"				"[0.6 0.6 0.6]"
					"phongfresnelranges"		"[0.5 0.5 1.0]"
					"$nofog"					"1"
					"$model"					"1"
					"$nocull"					"0"
					"$selfillum"				"1"
					"$halflambert"				"1"
					"$znearer"					"0"
					"$flat"						"0"	
					"$rimlight"					"1"
					"$rimlightexponent"			"2"
					"$rimlightboost"			"0"
			}
			)#" );
	}
	m_pFlat = g_pMaterialSystem->FindMaterial( XorStr( "nap_flat" ), TEXTURE_GROUP_MODEL );
	if( !m_pFlat || m_pFlat == nullptr || m_pFlat->IsErrorMaterial( ) )
		return false;

	m_pGlow = g_pMaterialSystem->FindMaterial( XorStr( "dev/glow_armsrace" ), nullptr );
	if( !m_pGlow || m_pGlow == nullptr || m_pGlow->IsErrorMaterial( ) )
		return false;

	m_bInitialised = true;
	return true;
}

bool Models::ValidateEntity( C_CSPlayer *pEntity ) {
	if( !pEntity )
		return false;

	if( !g_pStudioRender.IsValid( ) )
		return false;

	if( !g_pEngine->IsInGame( ) )
		return false;

	auto pClientClass = pEntity->GetClientClass( );
	if( !pClientClass )
		return false;

	if( pClientClass->m_ClassID != CCSRagdoll ) {
		// entity is dead and it isn't a ragdoll
		if( pClientClass->m_ClassID == CCSPlayer && pEntity->IsDead( ) )
			return false;
	}

	auto bUsingGlow = [&] ( ) {
		if( !g_Vars.globals.m_bInPostScreenEffects )
			return false;

		if( !g_pStudioRender->m_pForcedMaterial )
			return g_pStudioRender->m_nForcedMaterialType == 2 || g_pStudioRender->m_nForcedMaterialType == 4;

		return strstr( g_pStudioRender->m_pForcedMaterial->GetName( ), XorStr( "dev/glow" ) ) != nullptr;
	};

	// allow game to render glow for us
	return !bUsingGlow( );
}

void Models::ApplyMaterial( CVariables::CHAMS *pChams, bool bIgnoreZ ) {
	// rest in piss;/
	if( !pChams ) {
		return;
	}

	// nope dont do rendering lal
	if( !pChams->enabled ) {
		return;
	}

	IMaterial *pMaterial = nullptr;
	switch( pChams->material ) {
		case EModelTypes::TYPE_DEFAULT:
			pMaterial = nullptr;
			break;
		case EModelTypes::TYPE_MATERIAL:
			pMaterial = pChams->shine > 0.f ? m_pShiny : m_pMaterial;
			break;
		case EModelTypes::TYPE_FLAT:
			pMaterial = m_pFlat;
			break;
		case EModelTypes::TYPE_GLOW:
			pMaterial = m_pGlow;
			break;
	}

	// rendering chams without a material
	// (just color tint), handle differently
	if( !pMaterial ) {
		// nothing to render (can't do invisible here)
		if( bIgnoreZ || !pChams->visible ) {
			return;
		}

		// color modulate this model
		g_pRenderView->SetColorModulation(
			pChams->visible_color.r * pChams->visible_color.a,
			pChams->visible_color.g * pChams->visible_color.a,
			pChams->visible_color.b * pChams->visible_color.a );

		return;
	}

	// determine what colour we're using
	const Color_f fColor = bIgnoreZ ? pChams->invisible_color : pChams->visible_color;

	// let game know we're using the material
	pMaterial->IncrementReferenceCount( );

	// rendering with material, apply our changes
	pMaterial->SetMaterialVarFlag( MATERIAL_VAR_IGNOREZ, bIgnoreZ );

	// apply our custom keyvalues/color modulation differently for glow
	if( pChams->material == EModelTypes::TYPE_GLOW ) {
		IMaterialVar *$envmaptint = pMaterial->FindVar( XorStr( "$envmaptint" ), nullptr );
		if( $envmaptint ) {
			$envmaptint->SetVecValue( pChams->glow_overlay_color.r,
									  pChams->glow_overlay_color.g,
									  pChams->glow_overlay_color.b );
		}

		IMaterialVar *$alpha = pMaterial->FindVar( XorStr( "$alpha" ), nullptr );
		if( $alpha ) {
			$alpha->SetFloatValue( pChams->glow_overlay_color.a );
		}

		IMaterialVar *$envmapfresnelminmaxexp = pMaterial->FindVar( XorStr( "$envmapfresnelminmaxexp" ), nullptr );
		if( $envmapfresnelminmaxexp != nullptr ) {
			$envmapfresnelminmaxexp->SetVecValue( 0.f, 1.f, std::clamp<float>( ( 100.0f - pChams->glow_overlay_opacity ) * 0.2f, 1.f, 20.f ) );
		}
	}
	// othwerise color modulate, apply our alpha and modify key values normally 
	else {
		pMaterial->ColorModulate( fColor.r, fColor.g, fColor.b );
		pMaterial->AlphaModulate( fColor.a );

		// 'reflectivity'
		IMaterialVar *$envmaptint = pMaterial->FindVar( XorStr( "$envmaptint" ), nullptr );
		if( $envmaptint ) {
			$envmaptint->SetVecValue( pChams->reflectivity_color.r * ( pChams->reflectivity / 100.f ),
									  pChams->reflectivity_color.g * ( pChams->reflectivity / 100.f ),
									  pChams->reflectivity_color.b * ( pChams->reflectivity / 100.f ) );
		}

		// 'shine'
		IMaterialVar *$phongboost = pMaterial->FindVar( XorStr( "$phongboost" ), nullptr );
		IMaterialVar *$phongexponent = pMaterial->FindVar( XorStr( "$phongexponent" ), nullptr );
		if( $phongboost && $phongexponent ) {
			$phongboost->SetFloatValue( pChams->shine / 100.f );
			$phongexponent->SetIntValue( ( int )( pChams->shine / 255.f ) );
		}

		// 'rim'
		IMaterialVar *$rimlightboost = pMaterial->FindVar( XorStr( "$rimlightboost" ), nullptr );
		if( $rimlightboost ) {
			float flRimBoost = pChams->rim / 100.f;
			if( pChams->rim < 1.f )
				flRimBoost = 0.f;

			if( pChams->rim > 99.f )
				flRimBoost = 1.f;

			$rimlightboost->SetFloatValue( flRimBoost );
		}
	}

	// fix color correction on certain maps
	for( int i = 0; i <= 2; ++i ) {
		g_pStudioRender->m_ColorMod[ i ] = 1.f;
	}

	// make sure the game uses our material
	g_pStudioRender->m_pForcedMaterial = pMaterial;
	g_pStudioRender->m_nForcedMaterialType = 0;

	// only force material override when needed (fix scout, awp scope etc.)
	if( m_bForceOverride ) {
		g_pModelRender->ForcedMaterialOverride( pMaterial );
	}
}

bool Models::IsRenderingChams( CVariables::CHAMS *pChams ) {
	// rest in piss;/
	if( !pChams ) {
		return false;
	}

	// nope dont do rendering lal
	if( !pChams->enabled ) {
		return false;
	}

	bool bRenderingChams = false;

	// let's not mess with future calls
	float flBackupColorModulation[ 3 ];
	g_pRenderView->GetColorModulation( flBackupColorModulation );

	// first render invisible
	if( pChams->invisible ) {
		ApplyMaterial( pChams, true );
		fnDrawModel( );

		bRenderingChams = true;
	}

	// then render visible
	if( pChams->visible ) {
		ApplyMaterial( pChams, false );
		fnDrawModel( );

		bRenderingChams = true;
	}

	// then finally we render our glow overlay
	if( pChams->glow_overlay ) {
		// make a copy of these chams settings
		// and force the material to glow, then
		// our material handler will render the glow
		const int nBackupMaterial = pChams->material;
		pChams->material = EModelTypes::TYPE_GLOW;

		ApplyMaterial( pChams, pChams->glow_overlay_through_walls );
		fnDrawModel( );

		// restore back to the material we had before
		pChams->material = nBackupMaterial;

		bRenderingChams = true;
	}

	// invalidate material for next call
	fnInvalidateMaterial( );

	// go back to what this was before
	g_pRenderView->SetColorModulation( flBackupColorModulation );

	// notify cheat if we're rendering smth
	return bRenderingChams;
}

void Models::HandleHistoryMatrix( C_CSPlayer *pEntity ) {
	if( !pEntity ) {
		return;
	}

	// don't do shit
	if( !g_Vars.chams_backtrack.enabled ) {
		return;
	}

	// get all records, return if none found
	auto pEntry = g_Animations.GetAnimationEntry( pEntity->EntIndex( ) );
	if( !pEntry || pEntry->m_deqRecords.empty( ) ) {
		return;
	}

	// breaking lagcomp, don't draw
	if( g_Animations.BreakingTeleportDistance( pEntity->EntIndex( ) ) ) {
		return;
	}

	// only draw on current threat
	if( g_Vars.esp.backtrack_current_threat ) {
		// player is not our current threat, don't draw
		if( pEntity != g_AntiAim.GetBestPlayer( ) ) {
			return;
		}
	}

	// get oldest record
	LagRecord_t *pLast = nullptr;
	if( g_Vars.esp.backtrack_draw_last )
		pLast = &g_Animations.GetOldestRecord( pEntity->EntIndex( ) );

	// get ideal record
	LagRecord_t *pIdeal = nullptr;
	if( g_Vars.esp.backtrack_draw_ideal )
		pIdeal = &g_Resolver.FindIdealRecord( pEntry );

	// get lerped matrix
	matrix3x4_t pLerpedMatrix[ 128 ];
	if( g_Vars.esp.backtrack_draw_lerped )
		g_Animations.GetVisualMatrix( pEntity, pLerpedMatrix, false );

	// check if they're valid and we should use them
	const bool pLastValid = pLast && ( pLast->m_vecOrigin - pEntity->m_vecOrigin( ) ).Length( ) > 1.f;
	const bool bIdealValid = pIdeal ? ( pIdeal->m_vecOrigin - pEntity->m_vecOrigin( ) ).Length( ) > 1.f : false;
	const bool bLerpedValid = pLerpedMatrix;

	// get original backtrack colors
	const auto regularVisibleColor = g_Vars.chams_backtrack.visible_color;
	const auto regularHiddenColor = g_Vars.chams_backtrack.invisible_color;

	// get lerped backtrack color
	static float flMultiplier = 0.f;
	static bool bSwap = false;

	flMultiplier = GUI::Approach( flMultiplier, bSwap ? 1.f : 0.f, g_pGlobalVars->frametime * 1.4f );

	if( flMultiplier <= 0.01f || flMultiplier >= 0.99f )
		bSwap = !bSwap;

	Color visibleBlend = Color::Blend( g_Vars.chams_backtrack.visible_color.ToRegularColor( ), g_Vars.esp.second_lerped_color.ToRegularColor( ), flMultiplier );
	Color invisibleBlend = Color::Blend( g_Vars.chams_backtrack.invisible_color.ToRegularColor( ), g_Vars.esp.second_lerped_color.ToRegularColor( ), flMultiplier );
	Color idealBlend = Color::Blend( g_Vars.esp.backtrack_ideal_record_color.ToRegularColor( ), g_Vars.esp.second_lerped_color.ToRegularColor( ), flMultiplier );

	Color_f finalVisibleBlend = Color_f( visibleBlend.r( ), visibleBlend.g( ), visibleBlend.b( ), visibleBlend.a( ) );
	Color_f finalInvisibleBlend = Color_f( invisibleBlend.r( ), invisibleBlend.g( ), invisibleBlend.b( ), invisibleBlend.a( ) );
	Color_f finalIdealBlend = Color_f( idealBlend.r( ), idealBlend.g( ), idealBlend.b( ), idealBlend.a( ) );

	// don't render last & fdeal if we are already drawing all
	const bool bDontDraw = g_Vars.esp.backtrack_draw_all;

	// small lambda for handling chams
	auto fnRenderChams = [&] ( ) {
		// apply our material and render our model with the custom matrix
		if( IsRenderingChams( &g_Vars.chams_backtrack ) ) {
			// get rid of our custom matrix
			m_pCustomMatrix = nullptr;
		}
	};

	// last record
	if( g_Vars.esp.backtrack_draw_last && !bDontDraw ) {
		// draw oldest
		if( pLastValid ) {
			// if we want to blend it, override with blend color
			if( g_Vars.esp.lerp_visible )
				g_Vars.chams_backtrack.visible_color = finalVisibleBlend;

			if( g_Vars.esp.lerp_invisible )
				g_Vars.chams_backtrack.invisible_color = finalInvisibleBlend;

			// apply custom matrix
			m_pCustomMatrix = pLast->m_sAnims[ESides::SIDE_SERVER].m_pMatrix;

			// render chams
			fnRenderChams( );

			// restore to original colors
			if( g_Vars.esp.lerp_visible )
				g_Vars.chams_backtrack.visible_color = regularVisibleColor;

			if( g_Vars.esp.lerp_invisible )
				g_Vars.chams_backtrack.invisible_color = regularHiddenColor;
		}
	}

	// ideal record
	if( g_Vars.esp.backtrack_draw_ideal && !bDontDraw && pIdeal ) {
		// draw ideal
		if( bIdealValid ) {
			// restore to original colors
			if( g_Vars.esp.lerp_visible )
				g_Vars.chams_backtrack.visible_color = finalIdealBlend;

			if( g_Vars.esp.lerp_invisible )
				g_Vars.chams_backtrack.invisible_color = finalIdealBlend;

			// apply custom matrix
			m_pCustomMatrix = pIdeal->m_sAnims[ ESides::SIDE_SERVER ].m_pMatrix;

			// render chams
			fnRenderChams( );

			// restore to original colors
			if( g_Vars.esp.lerp_visible )
				g_Vars.chams_backtrack.visible_color = regularVisibleColor;

			if( g_Vars.esp.lerp_invisible )
				g_Vars.chams_backtrack.invisible_color = regularHiddenColor;
		}
	}

	// lerped record
	if( g_Vars.esp.backtrack_draw_lerped && !bDontDraw ) {
		// draw oldest
		if( bLerpedValid ) {
			// if we want to blend it, override with blend color
			if( g_Vars.esp.lerp_visible )
				g_Vars.chams_backtrack.visible_color = finalVisibleBlend;

			if( g_Vars.esp.lerp_invisible )
				g_Vars.chams_backtrack.invisible_color = finalInvisibleBlend;

			// apply custom matrix
			m_pCustomMatrix = pLerpedMatrix;

			// render chams
			fnRenderChams( );

			// restore to original colors
			if( g_Vars.esp.lerp_visible )
				g_Vars.chams_backtrack.visible_color = regularVisibleColor;

			if( g_Vars.esp.lerp_invisible )
				g_Vars.chams_backtrack.invisible_color = regularHiddenColor;
		}
	}

	// all records
	if( g_Vars.esp.backtrack_draw_all ) {
		// back up current alpha
		const float flAlpha1 = g_Vars.chams_backtrack.visible_color.a;
		const float flAlpha2 = g_Vars.chams_backtrack.invisible_color.a;
		const float flAlpha3 = g_Vars.chams_backtrack.glow_overlay_color.a;

		// loop through every record
		for( int it = 1; it <= pEntry->m_deqRecords.size( ); ++it ) {
			// single out record
			auto pRecord = &pEntry->m_deqRecords[ it - 1 ];
			if( !pRecord )
				continue;

			// record is not valid
			if( !pRecord->IsRecordValid( ) || pRecord->m_bInvalid || ( pRecord->m_vecOrigin - pEntity->m_vecOrigin( ) ).Length( ) < 2.5f )
				continue;

			// get fade alpha
			float flRecordAlpha = ( float )it / ( float )pEntry->m_deqRecords.size( );
			if( !flRecordAlpha )
				continue;

			// if we want to blend it, override with blend color
			if( g_Vars.esp.lerp_visible )
				g_Vars.chams_backtrack.visible_color = finalVisibleBlend;

			if( g_Vars.esp.lerp_invisible )
				g_Vars.chams_backtrack.invisible_color = finalInvisibleBlend;

			// override this
			if( g_Vars.esp.backtrack_manage_alpha ) {
				g_Vars.chams_backtrack.visible_color.a = flRecordAlpha;
				g_Vars.chams_backtrack.invisible_color.a = flRecordAlpha;
				g_Vars.chams_backtrack.glow_overlay_color.a = flRecordAlpha;
			}

			// apply custom matrix
			m_pCustomMatrix = pRecord->m_sAnims[ ESides::SIDE_SERVER ].m_pMatrix;

			// render chams
			fnRenderChams( );
		}

		// restore back to original alpha modulation
		if( g_Vars.esp.backtrack_manage_alpha ) {
			g_Vars.chams_backtrack.visible_color.a = flAlpha1;
			g_Vars.chams_backtrack.invisible_color.a = flAlpha2;
			g_Vars.chams_backtrack.glow_overlay_color.a = flAlpha3;
		}

		// restore to original colors
		if( g_Vars.esp.lerp_visible )
			g_Vars.chams_backtrack.visible_color = regularVisibleColor;

		if( g_Vars.esp.lerp_invisible )
			g_Vars.chams_backtrack.invisible_color = regularHiddenColor;
	}
}

void Models::HandleBodyUpdateMatrix( C_CSPlayer *pEntity ) {
	if( !pEntity ) {
		return;
	}

	matrix3x4_t pBodyUpdateMatrix[ MAXSTUDIOBONES ];

	// handle local body update separately
	if( pEntity->EntIndex( ) == g_pEngine->GetLocalPlayer( ) ) {

		// don't do shit
		if( !g_Vars.chams_body_update_local.enabled ) {
			return;
		}

		// 400ms fade out
		g_ServerAnimations.m_uBodyAnimations.m_flBodyAlpha -= ( 1.f / 0.4f ) * g_pGlobalVars->frametime;

		const float flAlpha1 = g_Vars.chams_body_update_local.visible_color.a;
		const float flAlpha2 = g_Vars.chams_body_update_local.invisible_color.a;
		const float flAlpha3 = g_Vars.chams_body_update_local.glow_overlay_color.a;

		// fade out the matrix lel
		g_Vars.chams_body_update_local.visible_color.a *= g_ServerAnimations.m_uBodyAnimations.m_flBodyAlpha;
		g_Vars.chams_body_update_local.invisible_color.a *= g_ServerAnimations.m_uBodyAnimations.m_flBodyAlpha;
		g_Vars.chams_body_update_local.glow_overlay_color.a *= g_ServerAnimations.m_uBodyAnimations.m_flBodyAlpha;

		// apply custom matrix
		m_pCustomMatrix = g_ServerAnimations.m_uBodyAnimations.m_pMatrix;

		// apply our material and render our model with the custom matrix
		if( IsRenderingChams( &g_Vars.chams_body_update_local ) ) {
			// get rid of our custom matrix
			m_pCustomMatrix = nullptr;
		}

		// restore back to original alpha modulation
		g_Vars.chams_body_update_local.visible_color.a = flAlpha1;
		g_Vars.chams_body_update_local.invisible_color.a = flAlpha2;
		g_Vars.chams_body_update_local.glow_overlay_color.a = flAlpha3;
	}

	// enemy body update chams
	// don't do shit
	if( !g_Vars.chams_body_update_enemy.enabled ) {
		return;
	}

	// nothing to draw
	if( !g_Animations.GetVisualMatrix( pEntity, pBodyUpdateMatrix, true ) ) {
		return;
	}

	// 400ms fade out
	m_flLeAlpha.at( pEntity->EntIndex( ) ) -= ( 1.f / 0.4f ) * g_pGlobalVars->frametime;

	const float flAlpha1 = g_Vars.chams_body_update_enemy.visible_color.a;
	const float flAlpha2 = g_Vars.chams_body_update_enemy.invisible_color.a;
	const float flAlpha3 = g_Vars.chams_body_update_enemy.glow_overlay_color.a;

	// fade out the matrix lel
	g_Vars.chams_body_update_enemy.visible_color.a *= m_flLeAlpha.at( pEntity->EntIndex( ) );
	g_Vars.chams_body_update_enemy.invisible_color.a *= m_flLeAlpha.at( pEntity->EntIndex( ) );
	g_Vars.chams_body_update_enemy.glow_overlay_color.a *= m_flLeAlpha.at( pEntity->EntIndex( ) );

	// apply custom matrix
	m_pCustomMatrix = pBodyUpdateMatrix;

	// apply our material and render our model with the custom matrix
	if( IsRenderingChams( &g_Vars.chams_body_update_enemy ) ) {
		// get rid of our custom matrix
		m_pCustomMatrix = nullptr;
	}

	// restore back to original alpha modulation
	g_Vars.chams_body_update_enemy.visible_color.a = flAlpha1;
	g_Vars.chams_body_update_enemy.invisible_color.a = flAlpha2;
	g_Vars.chams_body_update_enemy.glow_overlay_color.a = flAlpha3;
}

void Models::HandleDormantMatrix( C_CSPlayer *pEntity ) {
	if( !pEntity ) {
		return;
	}

	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal ) {
		return;
	}

	// not using behind wall chams, or dormant chams
	if( !g_Vars.chams_enemy.invisible || !g_Vars.visuals_enemy.dormant_chams )
		return;

	const Color_f fBackupInvisibleColor = g_Vars.chams_enemy.invisible_color;
	const Color_f fBackupVisibleColor = g_Vars.chams_enemy.visible_color;

	g_Vars.chams_enemy.invisible_color = Color_f( 1.f, 1.f, 1.f, 0.5f * g_Visuals.player_fading_alpha[ pEntity->EntIndex( ) ] );
	g_Vars.chams_enemy.visible_color = Color_f( 0.f, 0.f, 0.f, 0.f );

	bool bCreatedDormantMatrix = false;
	bool bDrawFatalityMatrix = false;
	alignas( 16 ) matrix3x4_t pDormant[ MAXSTUDIOBONES ];

	const float flReceiveTimeDelta = fabs( g_ExtendedVisuals.m_arrOverridePlayers.at( pEntity->EntIndex( ) ).m_flReceiveTime - g_pGlobalVars->realtime );

	bDrawFatalityMatrix = g_ExtendedVisuals.m_arrSoundPlayers.at( pEntity->EntIndex( ) ).m_flLastNonDormantTime > 0.f &&
		fabsf( g_ExtendedVisuals.m_arrSoundPlayers.at( pEntity->EntIndex( ) ).m_flLastNonDormantTime - g_pGlobalVars->realtime ) < 1.f;


	if( /*!bDrawFatalityMatrix &&*/
		g_ExtendedVisuals.m_arrOverridePlayers.at( pEntity->EntIndex( ) ).m_flReceiveTime != FLT_MAX &&
		g_ExtendedVisuals.m_arrOverridePlayers[ pEntity->EntIndex( ) ].m_eOverrideType != ESP_NONE &&
		flReceiveTimeDelta < 10.f ) {

		static std::array<bool, 65> m_bAscendingLastFrame, m_bSetLayer;
		static std::array<float, 65> m_flFakeSimulationTime, m_flFakePrevSimulationTime;
		static std::array<int, 65> m_fPreviousFlags;
		static std::array<int, 65> m_nTicksSameOrigin;
		static std::array<float, 65> m_flLastReceiveTime;

		if( g_ExtendedVisuals.m_arrOverridePlayers[ pEntity->EntIndex( ) ].m_flServerTime != 0.f ) {
			m_flFakePrevSimulationTime.at( pEntity->EntIndex( ) ) = m_flFakeSimulationTime.at( pEntity->EntIndex( ) );
			m_flFakeSimulationTime.at( pEntity->EntIndex( ) ) = g_ExtendedVisuals.m_arrOverridePlayers[ pEntity->EntIndex( ) ].m_flServerTime;
		}
		else {
			if( g_ExtendedVisuals.m_arrOverridePlayers.at( pEntity->EntIndex( ) ).m_vecLastOrigin !=
				g_ExtendedVisuals.m_arrOverridePlayers.at( pEntity->EntIndex( ) ).m_vecOrigin ) {
				// since m_vecOrigin updates on simtime change,
				// simtime will update at the same time origin does
				m_flFakePrevSimulationTime.at( pEntity->EntIndex( ) ) = m_flFakeSimulationTime.at( pEntity->EntIndex( ) );
				m_flFakeSimulationTime.at( pEntity->EntIndex( ) ) = g_pGlobalVars->curtime;

				m_nTicksSameOrigin[ pEntity->EntIndex( ) ] = 0;
			}

			if( g_ExtendedVisuals.m_arrOverridePlayers.at( pEntity->EntIndex( ) ).m_vecOrigin == g_ExtendedVisuals.m_arrOverridePlayers.at( pEntity->EntIndex( ) ).m_vecLastOrigin ) {
				if( g_ExtendedVisuals.m_arrOverridePlayers.at( pEntity->EntIndex( ) ).m_flReceiveTime != m_flLastReceiveTime[ pEntity->EntIndex( ) ] ) {
					m_nTicksSameOrigin[ pEntity->EntIndex( ) ]++;
				}

				m_flLastReceiveTime[ pEntity->EntIndex( ) ] = g_ExtendedVisuals.m_arrOverridePlayers.at( pEntity->EntIndex( ) ).m_flReceiveTime;
			}

			if( m_nTicksSameOrigin[ pEntity->EntIndex( ) ] > 2 ) {
				m_flFakeSimulationTime.at( pEntity->EntIndex( ) ) = g_pGlobalVars->curtime;
				m_flFakePrevSimulationTime.at( pEntity->EntIndex( ) ) = m_flFakeSimulationTime.at( pEntity->EntIndex( ) ) - TICKS_TO_TIME( 1 );
			}
		}

		const Vector vecBackupVelocity = pEntity->m_vecVelocity( );
		const Vector vecBackupAbsVelocity = pEntity->GetAbsVelocity( );

		if( m_flFakeSimulationTime.at( pEntity->EntIndex( ) ) != m_flFakePrevSimulationTime.at( pEntity->EntIndex( ) ) ) {

			// get our mimic time difference
			float flTimeDifference = std::max( g_pGlobalVars->interval_per_tick, m_flFakeSimulationTime.at( pEntity->EntIndex( ) ) - m_flFakePrevSimulationTime.at( pEntity->EntIndex( ) ) );

			// compute a rough velocity from our two origins
			Vector vecPredictedVelocity = (
				g_ExtendedVisuals.m_arrOverridePlayers.at( pEntity->EntIndex( ) ).m_vecOrigin -
				g_ExtendedVisuals.m_arrOverridePlayers.at( pEntity->EntIndex( ) ).m_vecLastOrigin ) * ( 1.0f / flTimeDifference );


			Vector src3D, dst3D;
			CGameTrace tr;
			Ray_t ray;
			CTraceFilter filter;

			filter.pSkip = pEntity;
			src3D = g_ExtendedVisuals.m_arrOverridePlayers.at( pEntity->EntIndex( ) ).m_vecOrigin + Vector( 0, 0, 1 );
			dst3D = src3D - Vector( 0, 0, 10 );
			ray.Init( src3D, dst3D );

			g_pEngineTrace->TraceRay( ray, MASK_PLAYERSOLID, &filter, &tr );

			// trace didn't hit anything
			if( tr.fraction == 1.f /*&& vecPredictedVelocity.z != 0.f*/ ) {
				// we're not on ground
				pEntity->m_fFlags( ) &= ~FL_ONGROUND;

				vecPredictedVelocity.z -= g_Vars.sv_gravity->GetFloat( ) * g_pGlobalVars->interval_per_tick;

				// previous origin was higher than current origin,
				// this player must be falling
				if( g_ExtendedVisuals.m_arrOverridePlayers.at( pEntity->EntIndex( ) ).m_vecLastOrigin.z > g_ExtendedVisuals.m_arrOverridePlayers.at( pEntity->EntIndex( ) ).m_vecOrigin.z ) {
					// if the player was ascending before, this means that the 
					// player is now beginning to fall, instead of forcing jump_fall
					// to 1, we should continue approaching it instead 
					if( m_bAscendingLastFrame.at( pEntity->EntIndex( ) ) ) {
						if( !m_bSetLayer.at( pEntity->EntIndex( ) ) ) {
							pEntity->m_AnimOverlay( )[ 4 ].m_nSequence = 14;
							pEntity->m_AnimOverlay( )[ 4 ].m_flPlaybackRate = pEntity->GetLayerSequenceCycleRate( &pEntity->m_AnimOverlay( )[ 4 ], 14 );
							pEntity->m_AnimOverlay( )[ 4 ].m_flCycle = 0.0f;
							pEntity->m_AnimOverlay( )[ 4 ].m_flWeight = 0.0f;

							m_bSetLayer.at( pEntity->EntIndex( ) ) = true;
						}
					}
					// this player wasn't ascending before, this can only
					// mean that he fell from something. force jump_fall to 1
					else {
						if( !m_bSetLayer.at( pEntity->EntIndex( ) ) ) {
							pEntity->m_AnimOverlay( )[ 4 ].m_nSequence = 14;
							pEntity->m_AnimOverlay( )[ 4 ].m_flPlaybackRate = pEntity->GetLayerSequenceCycleRate( &pEntity->m_AnimOverlay( )[ 4 ], 14 );
							pEntity->m_AnimOverlay( )[ 4 ].m_flCycle = 0.0f;
							pEntity->m_AnimOverlay( )[ 4 ].m_flWeight = 0.0f;

							m_bSetLayer.at( pEntity->EntIndex( ) ) = true;
						}
					}
				}

				// current origin is higher that previous origin,
				// this player must be ascending
				if( g_ExtendedVisuals.m_arrOverridePlayers.at( pEntity->EntIndex( ) ).m_vecOrigin.z > g_ExtendedVisuals.m_arrOverridePlayers.at( pEntity->EntIndex( ) ).m_vecLastOrigin.z ) {
					// note that the player did indeed jump, and didn't fall from something
					m_bAscendingLastFrame.at( pEntity->EntIndex( ) ) = true;

					if( !m_bSetLayer.at( pEntity->EntIndex( ) ) ) {
						pEntity->m_AnimOverlay( )[ 4 ].m_nSequence = 16;
						pEntity->m_AnimOverlay( )[ 4 ].m_flPlaybackRate = pEntity->GetLayerSequenceCycleRate( &pEntity->m_AnimOverlay( )[ 4 ], 16 );
						pEntity->m_AnimOverlay( )[ 4 ].m_flCycle = 0.0f;
						pEntity->m_AnimOverlay( )[ 4 ].m_flWeight = 0.0f;

						m_bSetLayer.at( pEntity->EntIndex( ) ) = true;
					}
				}
			}
			// trace hit smth, we must be on ground
			else {
				if( m_bSetLayer.at( pEntity->EntIndex( ) ) ) {
					pEntity->m_AnimOverlay( )[ 5 ].m_nSequence = 22;
					pEntity->m_AnimOverlay( )[ 5 ].m_flPlaybackRate = pEntity->GetLayerSequenceCycleRate( &pEntity->m_AnimOverlay( )[ 4 ], 22 );
					pEntity->m_AnimOverlay( )[ 5 ].m_flCycle = 0.0f;
					pEntity->m_AnimOverlay( )[ 5 ].m_flWeight = 0.0f;

					// we;re on ground DON'T DO !!
					pEntity->m_AnimOverlay( )[ 4 ].m_flPlaybackRate = 0.f;
					pEntity->m_AnimOverlay( )[ 4 ].m_flCycle = 0.0f;
					pEntity->m_AnimOverlay( )[ 4 ].m_flWeight = 0.0f;

					m_bSetLayer.at( pEntity->EntIndex( ) ) = false;
				}

				pEntity->m_fFlags( ) |= FL_ONGROUND;

				// cant go this fast when ducking,
				// let's account for it ..
				if( pEntity->m_flDuckAmount( ) > 0.f && vecPredictedVelocity.Length2D( ) > 115.f ) {
					pEntity->m_flDuckAmount( ) = 0.f;
					pEntity->m_fFlags( ) &= ~FL_DUCKING;
				}
			}

			if( g_ExtendedVisuals.m_arrOverridePlayers.at( pEntity->EntIndex( ) ).m_vecOrigin ==
				g_ExtendedVisuals.m_arrOverridePlayers.at( pEntity->EntIndex( ) ).m_vecLastOrigin ) {
				vecPredictedVelocity.Init( );
			}

			pEntity->m_vecVelocity( ) = vecPredictedVelocity;
			pEntity->SetAbsVelocity( vecPredictedVelocity );


			// update simulation time
			pEntity->m_flSimulationTime( ) = m_flFakeSimulationTime.at( pEntity->EntIndex( ) );
		}

		// update animations for this player and render a 
		// render matrix with our fresh predicted data
		g_Animations.UpdatePlayerSimple( pEntity );

		g_BoneSetup.m_vecCustomOrigin = g_ExtendedVisuals.m_arrOverridePlayers.at( pEntity->EntIndex( ) ).m_vecOrigin;
		bCreatedDormantMatrix = g_BoneSetup.SetupBonesRebuild( pEntity, pDormant, 128, 0x7FF00, pEntity->m_flSimulationTime( ), BoneSetupFlags::UseCustomOutput );
		g_BoneSetup.m_vecCustomOrigin.Init( );

		bDrawFatalityMatrix = false;

		pEntity->m_vecVelocity( ) = vecBackupVelocity;
		pEntity->SetAbsVelocity( vecBackupAbsVelocity );

		m_fPreviousFlags.at( pEntity->EntIndex( ) ) = pEntity->m_fFlags( );
	}
	else {
		bCreatedDormantMatrix = false;

		if( g_ExtendedVisuals.m_arrSoundPlayers.at( pEntity->EntIndex( ) ).m_bValidSound ) {
			g_BoneSetup.m_vecCustomOrigin = g_ExtendedVisuals.m_arrSoundPlayers.at( pEntity->EntIndex( ) ).m_vecOrigin;
			bCreatedDormantMatrix = g_BoneSetup.SetupBonesRebuild( pEntity, pDormant, 128, 0x7FF00, pEntity->m_flSimulationTime( ), BoneSetupFlags::UseCustomOutput );
			g_BoneSetup.m_vecCustomOrigin.Init( );
		}

	}

	if( !pDormant )
		bCreatedDormantMatrix = false;

	if( bDrawFatalityMatrix ) {
		g_Vars.chams_enemy.invisible_color = fBackupInvisibleColor;
		g_Vars.chams_enemy.visible_color = fBackupVisibleColor;

		// imitate our non-dormant chams rendering
		{
			bool bRenderingChams = false;

			// let's not mess with future calls
			float flBackupColorModulation[ 3 ];
			g_pRenderView->GetColorModulation( flBackupColorModulation );

			// first render invisible
			if( g_Vars.chams_enemy.invisible ) {
				ApplyMaterial( &g_Vars.chams_enemy, true );
				HandleRebuiltDrawModel( pEntity, bCreatedDormantMatrix ? pDormant : pEntity->m_CachedBoneData( ).Base( ) );

				bRenderingChams = true;
			}

			// then render visible
			if( g_Vars.chams_enemy.visible ) {
				ApplyMaterial( &g_Vars.chams_enemy, false );
				HandleRebuiltDrawModel( pEntity, bCreatedDormantMatrix ? pDormant : pEntity->m_CachedBoneData( ).Base( ) );
			}

			// then finally we render our glow overlay
			if( g_Vars.chams_enemy.glow_overlay ) {
				// make a copy of these chams settings
				// and force the material to glow, then
				// our material handler will render the glow
				const int nBackupMaterial = g_Vars.chams_enemy.material;
				g_Vars.chams_enemy.material = EModelTypes::TYPE_GLOW;

				ApplyMaterial( &g_Vars.chams_enemy, g_Vars.chams_enemy.glow_overlay_through_walls );
				HandleRebuiltDrawModel( pEntity, bCreatedDormantMatrix ? pDormant : pEntity->m_CachedBoneData( ).Base( ) );

				// restore back to the material we had before
				g_Vars.chams_enemy.material = nBackupMaterial;
			}

			// invalidate material for next call
			fnInvalidateMaterial( );

			// go back to what this was before
			g_pRenderView->SetColorModulation( flBackupColorModulation );
		}

		return;
	}

	// nothing to draw !!
	if( !bCreatedDormantMatrix || g_Vars.chams_enemy.invisible_color.a <= 0.f ) {
		g_Vars.chams_enemy.invisible_color = fBackupInvisibleColor;
		g_Vars.chams_enemy.visible_color = fBackupVisibleColor;
		return;
	}

	ApplyMaterial( &g_Vars.chams_enemy, true );
	HandleRebuiltDrawModel( pEntity, bCreatedDormantMatrix ? pDormant : pEntity->m_CachedBoneData( ).Base( ) );

	// invalidate material for next call
	fnInvalidateMaterial( );

	g_Vars.chams_enemy.invisible_color = fBackupInvisibleColor;
	g_Vars.chams_enemy.visible_color = fBackupVisibleColor;
}

void Models::HandleDormantMatrices( ) {
	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !g_pEngine->IsConnected( ) || !g_pEngine->IsInGame( ) || !pLocal ) {
		m_vecAimbotMatrix.clear( );
		return;
	}

	for( int i = 1; i <= 64; ++i ) {
		auto pEntity = C_CSPlayer::GetPlayerByIndex( i );
		if( !pEntity ) {
			continue;
		}

		if( pEntity->IsDead( ) ) {
			g_ExtendedVisuals.m_arrSoundPlayers.at( i ).m_flLastNonDormantTime = 0.f;
			g_ExtendedVisuals.m_arrSoundPlayers.at( i ).m_iReceiveTime = FLT_MAX;
			continue;
		}

		if( pEntity->IsTeammate( pLocal ) ) {
			continue;
		}

		// not dormant, can't handle
		if( !pEntity->IsDormant( ) ) {
			continue;
		}

		// nothing to render :-/
		if( g_Visuals.player_fading_alpha.at( i ) <= 0.f ) {
			continue;
		}

		// render our chams for this player
		HandleDormantMatrix( pEntity );
	}
}

void Models::HandleRebuiltDrawModel( C_CSPlayer *pEntity, matrix3x4_t *pMatrix ) {
	if( !pEntity )
		return;

	if( !g_Vars.r_drawmodelstatsoverlay )
		return;

	static int m_nSkin = SDK::Memory::FindInDataMap( pEntity->GetPredDescMap( ), XorStr( "m_nSkin" ) );
	static int m_nBody = SDK::Memory::FindInDataMap( pEntity->GetPredDescMap( ), XorStr( "m_nBody" ) );

	auto renderable = pEntity->GetClientRenderable( );
	if( !renderable )
		return;

	auto model = pEntity->GetModel( );
	if( !model )
		return;

	auto hdr = *( studiohdr_t ** )( pEntity->m_pStudioHdr( ) );
	if( !hdr )
		return;

	DrawModelResults_t results;
	DrawModelInfo_t info;
	info.m_bStaticLighting = true;
	info.m_pStudioHdr = hdr;
	info.m_pHardwareData = g_pMDLCache->GetHardwareData( model->studio );
	info.m_Skin = *( int * )( uintptr_t( pEntity ) + m_nSkin );
	info.m_Body = *( int * )( uintptr_t( pEntity ) + m_nBody );
	info.m_HitboxSet = pEntity->m_nHitboxSet( );
	info.m_pClientEntity = renderable;
	info.m_Lod = 0;
	info.m_pColorMeshes = nullptr;

	bool bShadowDepth = ( 0x1 & STUDIO_SHADOWDEPTHTEXTURE ) != 0;

	// Don't do decals if shadow depth mapping...
	info.m_Decals = bShadowDepth ? 0 : StudioDecalHandle_t( );

	// Sets up flexes
	float *pFlexWeights = NULL;
	float *pFlexDelayedWeights = NULL;

	int overlayVal = g_Vars.r_drawmodelstatsoverlay->GetInt( );
	int drawFlags = 0;

	if( bShadowDepth ) {
		drawFlags |= STUDIORENDER_DRAW_OPAQUE_ONLY;
		drawFlags |= STUDIORENDER_SHADOWDEPTHTEXTURE;
	}

	if( overlayVal && !bShadowDepth ) {
		drawFlags |= STUDIORENDER_DRAW_GET_PERF_STATS;
	}

	drawFlags |= STUDIORENDER_DRAW_STATIC_LIGHTING;
	drawFlags |= STUDIORENDER_DRAW_NO_SHADOWS;

	auto pFinalMatrix = pMatrix ? pMatrix : pEntity->m_CachedBoneData( ).Base( );

	// lol dude i was legit scrolling thru the model render class
	// and saw this function with 1 param (origin) and thought to myself
	// hmm yes its look good let's call maybe) and it fixed the lightning
	// shit brah thank you RNG gods (i dont think we can call it multiple times doe)
	g_pModelRender->SetupLighting( pEntity->GetAbsOrigin( ) + Vector( 0, 0, 2 ) );

	const float flBackupAlphaMod = g_pStudioRender->m_AlphaMod;
	g_pStudioRender->m_AlphaMod = 1.f;
	g_pStudioRender->DrawModel( &results, &info, pFinalMatrix, pFlexWeights, pFlexDelayedWeights, pEntity->GetAbsOrigin( ), drawFlags );
	g_pStudioRender->m_AlphaMod = flBackupAlphaMod;
}

void Models::HandleAimbotMatrices( ) {
	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !g_pEngine->IsConnected( ) || !g_pEngine->IsInGame( ) || !pLocal ) {
		m_vecAimbotMatrix.clear( );
		return;
	}

	if( m_vecAimbotMatrix.empty( ) ) {
		return;
	}

	if( !g_pStudioRender.IsValid( ) ) {
		return;
	}

	auto it = m_vecAimbotMatrix.begin( );
	while( it != m_vecAimbotMatrix.end( ) ) {
		if( !it->m_pMatrix || !it->m_pEntity ) {
			++it;
			continue;
		}

		const auto flDelta = g_pGlobalVars->realtime - it->m_flTime;
		if( g_pGlobalVars->realtime >= it->m_flTime ) {
			// fade out at the same rate esp fades out when players die
			it->m_flAlpha = GUI::Approach( it->m_flAlpha, 0.f, g_pGlobalVars->frametime * 10.f );
		}

		if( it->m_flAlpha <= 0.0f ) {
			it = m_vecAimbotMatrix.erase( it );
			continue;
		}

		// in case player leaves, we regrab it to make sure it's valid.
		it->m_pEntity = C_CSPlayer::GetPlayerByIndex( it->m_nEntIndex );
		if( !it->m_pEntity ) {
			it = m_vecAimbotMatrix.erase( it );
			continue;
		}

		if( !it->m_pEntity || !it->m_pEntity->EntIndex( ) || it->m_pEntity->EntIndex( ) > 64 ) {
			it = m_vecAimbotMatrix.erase( it );
			continue;
		}

		const float flBackupVisibleAlpha = g_Vars.chams_aimbot.visible_color.a;
		const float flBackupInvisibleAlpha = g_Vars.chams_aimbot.invisible_color.a;
		const float flBackupGlowAlpha = g_Vars.chams_aimbot.glow_overlay_color.a;

		// fade out when needed
		g_Vars.chams_aimbot.visible_color.a *= it->m_flAlpha;
		g_Vars.chams_aimbot.invisible_color.a *= it->m_flAlpha;
		g_Vars.chams_aimbot.glow_overlay_color.a *= it->m_flAlpha;

		// first render invisible
		if( g_Vars.chams_aimbot.invisible ) {
			ApplyMaterial( &g_Vars.chams_aimbot, true );
			HandleRebuiltDrawModel( it->m_pEntity, it->m_pMatrix );
		}

		// then render visible
		if( g_Vars.chams_aimbot.visible ) {
			ApplyMaterial( &g_Vars.chams_aimbot, false );
			HandleRebuiltDrawModel( it->m_pEntity, it->m_pMatrix );
		}

		// then finally we render our glow overlay
		if( g_Vars.chams_aimbot.glow_overlay ) {
			// make a copy of these chams settings
			// and force the material to glow, then
			// our material handler will render the glow
			const int nBackupMaterial = g_Vars.chams_aimbot.material;
			g_Vars.chams_aimbot.material = EModelTypes::TYPE_GLOW;

			ApplyMaterial( &g_Vars.chams_aimbot, true );
			HandleRebuiltDrawModel( it->m_pEntity, it->m_pMatrix );

			// restore back to the material we had before
			g_Vars.chams_aimbot.material = nBackupMaterial;
		}

		// restore back to our original colors
		g_Vars.chams_aimbot.visible_color.a = flBackupVisibleAlpha;
		g_Vars.chams_aimbot.invisible_color.a = flBackupInvisibleAlpha;
		g_Vars.chams_aimbot.glow_overlay_color.a = flBackupGlowAlpha;

		// invalidate material for next call
		fnInvalidateMaterial( );

		++it;
	}
}

void Models::AddAimbotMatrix( C_CSPlayer *pEntity, matrix3x4_t *pMatrix ) {
	if( !pEntity || !pMatrix )
		return;

	auto &pShot = m_vecAimbotMatrix.emplace_back( );

	pShot.m_pEntity = pEntity;
	pShot.m_nEntIndex = pEntity->EntIndex( );
	pShot.m_flTime = g_pGlobalVars->realtime + g_Vars.esp.chams_hitmatrix_duration;
	pShot.m_flAlpha = 1.f;

	std::memcpy( pShot.m_pMatrix, pMatrix, pEntity->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );
}

C_BaseEntity *Models::GetMoveParentEntity( C_BaseEntity *pEntity ) {
	if( !pEntity )
		return false;

	if( !pEntity->moveparent( ).Get( ) || !pEntity->moveparent( ).IsValid( ) )
		return false;

	return ( ( C_BaseEntity * )pEntity->moveparent( ).Get( ) );
}

bool Models::AttachmentBelongsToPlayer( C_BaseEntity *pEntity, C_CSPlayer *pOwnerEntity ) {
	if( !pEntity )
		return false;

	if( !pOwnerEntity )
		return false;

	auto pAttachmentParent = GetMoveParentEntity( pEntity );
	if( !pAttachmentParent )
		return false;

	return pAttachmentParent->EntIndex( ) == pOwnerEntity->EntIndex( );
}

void Models::HandleModels( void *ecx, IMatRenderContext *pMatRenderContext, DrawModelState_t &uDrawModelState, ModelRenderInfo_t &uRenderInfo, matrix3x4_t *pMatrix ) {
	// setup our functions that we'll be using to render our models
	fnDrawModel = std::move( [&] ( ) {
		return Hooked::oDrawModelExecute( ecx, pMatRenderContext, uDrawModelState, uRenderInfo, m_pCustomMatrix ? m_pCustomMatrix : pMatrix );
	} );

	fnSetAlpha = std::move( [&] ( float flAlpha ) {
		g_pStudioRender->m_AlphaMod = flAlpha;
		g_pRenderView->SetBlend( flAlpha );
	} );

	fnInvalidateMaterial = std::move( [&] ( ) {
		g_pStudioRender->m_pForcedMaterial = nullptr;
		g_pStudioRender->m_nForcedMaterialType = 0;
	} );

	if( uRenderInfo.flags & 0x40000000 || !uRenderInfo.pRenderable || !uRenderInfo.pRenderable->GetIClientUnknown( ) ) {
		return fnDrawModel( );
	}

	// failed to initialise materials
	if( !InitialiseModelTypes( ) ) {
		return fnDrawModel( );
	}

	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal ) {
		return fnDrawModel( );
	}

	auto pEntity = ( C_CSPlayer * )uRenderInfo.pRenderable->GetIClientUnknown( )->GetBaseEntity( );
	if( !ValidateEntity( pEntity ) ) {
		return fnDrawModel( );
	}

	// check for dormancy on players 
	if( pEntity->IsPlayer( ) && pEntity->IsDormant( ) ) {
		return fnDrawModel( );
	}

	// first, let's handle rendering of players
	if( pEntity->IsPlayer( ) ) {
		// let's handle local player first
		if( pEntity->EntIndex( ) == g_pEngine->GetLocalPlayer( ) ) {
			// apply blend stuff to our model if needed
			{
				const float flBlendMultiplier = g_pInput->m_fCameraInThirdPerson ? g_Misc.m_flThirdpersonTransparency : 0.f;
				const float flAlpha = ( pLocal->m_bIsScoped( ) && g_Vars.esp.blur_in_scoped ) && g_pInput->m_fCameraInThirdPerson ?
					( ( g_Vars.esp.blur_in_scoped_value / 100.f ) * flBlendMultiplier )
					: flBlendMultiplier;

				fnSetAlpha( flAlpha );
			}

			if( !( g_Vars.visuals_local.disable_material && pLocal->m_bIsScoped( ) ) ) {
				// render the original model when needed
				// else, we will draw our own chams
				if( !IsRenderingChams( &g_Vars.chams_local ) ) {
					// first render the original model
					m_pCustomMatrix = nullptr;
					fnDrawModel( );
					fnInvalidateMaterial( );
				}

				// then handle rendering of our custom matrix above our original model
				HandleBodyUpdateMatrix( pEntity );
				m_pCustomMatrix = nullptr;

				return;
			}
		}

		const bool bTeammate = pEntity->IsTeammate( pLocal );

		// now let's handle enemies
		if( !bTeammate ) {
			// let's first handle rendering of our 'custom matrices'
			HandleHistoryMatrix( pEntity );
			HandleBodyUpdateMatrix( pEntity );

			// reset our custom matrix for sanity reasons
			m_pCustomMatrix = nullptr;

			// yep, we wanna draw enemy chams.
			if( IsRenderingChams( &g_Vars.chams_enemy ) ) {
				return;
			}
		}

		if( bTeammate ) {
			// yep, we wanna draw teammate chams.
			if( IsRenderingChams( &g_Vars.chams_teammates ) ) {
				return;
			}
		}
	}
	// now we can handle non-player entities (weapons, arms, attachments, ragdolls etc)
	else {
		auto pClientClass = pEntity->GetClientClass( );
		if( pClientClass ) {

			// handle ragdolls 
			if( pClientClass->m_ClassID == CCSRagdoll ) {
				if( !pEntity->IsTeammate( pLocal ) ) {
					// yep, we wanna draw ragdoll chams.
					if( IsRenderingChams( &g_Vars.chams_ragdolls ) ) {
						return;
					}
				}
			}
			// handle weapons
			else if( pClientClass->m_ClassID == CPredictedViewModel ) {
				m_bForceOverride = true;

				// yep, we wanna draw weapon chams.
				if( IsRenderingChams( &g_Vars.chams_weapons ) ) {
					if( m_bForceOverride )
						g_pModelRender->ForcedMaterialOverride( nullptr );

					m_bForceOverride = false;
					return;
				}
			}
			// handle hands
			else if( pClientClass->m_ClassID == CBaseAnimating &&
					 uDrawModelState.m_pStudioHdr && std::string( uDrawModelState.m_pStudioHdr->szName ).find( XorStr( "stattrack" ) ) == std::string::npos ) {
				m_bForceOverride = true;
				// yep, we wanna draw hand chams.
				if( IsRenderingChams( &g_Vars.chams_arms ) ) {
					if( m_bForceOverride )
						g_pModelRender->ForcedMaterialOverride( nullptr );

					m_bForceOverride = false;
					return;
				}
			}
		}

		// handle rendering of local attachments
		if( AttachmentBelongsToPlayer( pEntity, pLocal ) ) {
			// yep, we wanna draw these.
			if( IsRenderingChams( &g_Vars.chams_attachments_local ) ) {
				return;
			}
		}

		// handle rendering of enemy attachments
		// check if the parent of this attachment is a player and isn't on our team
		C_CSPlayer *pAttachmentParent = reinterpret_cast< C_CSPlayer * >( GetMoveParentEntity( pEntity ) );
		if( pAttachmentParent &&
			pAttachmentParent->IsPlayer( ) &&
			!pAttachmentParent->IsTeammate( pLocal ) ) {
			// yep, we wanna draw these.
			if( IsRenderingChams( &g_Vars.chams_attachments_enemy ) ) {
				return;
			}
		}
	}

	// if this is still somehow valid here,
	// let's ensure we don't use it
	m_pCustomMatrix = nullptr;

	// if we have reached here, our chams are disabled for this entity.
	// finish up and render the original model
	fnDrawModel( );
	fnInvalidateMaterial( );
}