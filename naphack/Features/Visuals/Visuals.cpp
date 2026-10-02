#include "Visuals.hpp"
#include "../../pandora.hpp"
#include "../../Renderer/Render.hpp"
#include "../../SDK/Classes/weapon.hpp"
#include "../../SDK/Classes/WeaponInfo.hpp"
#include "../Rage/Resolver.hpp"
#include "../Rage/Ragebot.hpp"
#include "../Rage/Autowall.hpp"
#include "../../Utils/Threading/threading.h"
#include "../Rage/AntiAim.hpp"
#include "../Rage/ServerAnimations.hpp"
#include <ctime>
#include <iomanip>
#include "../Miscellaneous/Movement.hpp"
#include "../Miscellaneous/PlayerList.hpp"
#include "../Miscellaneous/GrenadeWarning.hpp"
#include "../Rage/TickbaseShift.hpp"
#include "../Miscellaneous/Communication.hpp"
#include "../Rage/EnginePrediction.hpp"

Visuals g_Visuals;
ExtendedVisuals g_ExtendedVisuals;

#define YRES(y)	( y  * ( ( float )Render::Engine::m_height / 480.0 ) )
void Visuals::SpreadCrosshair( ) {
	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	if( pLocal->IsDead( ) )
		return;

	auto pWeapon = ( C_WeaponCSBaseGun * )pLocal->m_hActiveWeapon( ).Get( );
	if( !pWeapon )
		return;

	if( !pWeapon->GetCSWeaponData( ).IsValid( ) )
		return;

	auto type = pWeapon->GetCSWeaponData( ).Xor( )->m_iWeaponType;

	if( g_Vars.esp.spread_crosshair == 0 )
		return;

	float fHalfFov = DEG2RAD( pLocal->GetFOV( ) ) * 0.5f;
	float flInaccuracy = pWeapon->GetInaccuracy( );
	float flSpread = pWeapon->GetSpread( );

	static auto cl_crosshairgap = g_pCVar->FindVar( XorStr( "cl_crosshairgap" ) );
	static auto cl_crosshairsize = g_pCVar->FindVar( XorStr( "cl_crosshairsize" ) );
	static auto cl_crosshairthickness = g_pCVar->FindVar( XorStr( "cl_crosshairthickness" ) );
	static auto cl_crosshair_dynamic_maxdist_splitratio = g_pCVar->FindVar( XorStr( "cl_crosshair_dynamic_maxdist_splitratio" ) );

	float fSpreadDistance = ( ( ( flInaccuracy + flSpread ) * 320.0f / tanf( fHalfFov ) ) ) + cl_crosshairgap->GetFloat( );;

	if( ( type == WEAPONTYPE_KNIFE && pWeapon->m_iItemDefinitionIndex( ) != WEAPON_ZEUS ) || type == WEAPONTYPE_C4 || type == WEAPONTYPE_GRENADE )
		fSpreadDistance = 0.f;

#if 1
	float flRadius = fSpreadDistance /*+ cl_crosshairgap->GetFloat( )*/;
	flRadius *= Render::Engine::m_height * ( 1.f / 480.f );
#else
	fSpreadDistance *= Render::Engine::m_height * ( 1.f / 480.f );
	fSpreadDistance += cl_crosshairgap->GetFloat( );

	int iBarThickness = MAX( 1, int( YRES( cl_crosshairthickness->GetFloat( ) ) ) );

	using GET_FISH_TALE = float( __thiscall * )( void * );
	auto flAccuracyFishtail = Memory::VCall<GET_FISH_TALE>( pWeapon, 337 )( this );

	int nFishTaleShift = ( flAccuracyFishtail * ( Render::Engine::m_height / 500.0f ) );
	int iBarSize = int( YRES( cl_crosshairsize->GetFloat( ) ) );
	int iInnerLeft = ( ( Render::GetScreenSize( ).x - fSpreadDistance - iBarThickness / 2 + nFishTaleShift ) - iBarSize );
	float flSplitRatio = cl_crosshair_dynamic_maxdist_splitratio->GetFloat( );



	int iBarSizeInner = ceil( ( float )iBarSize * ( 1.0f - flSplitRatio ) );
	int iBarSizeOuter = floor( ( float )iBarSize * flSplitRatio );

	// draw horizontal crosshair lines
	int iInnerLeft = ( ( Render::GetScreenSize( ).x / 2.f ) - fSpreadDistance - iBarThickness / 2 + nFishTaleShift ) - iBarSizeInner;
	int iInnerRight = iInnerLeft + 2 * ( fSpreadDistance + iBarSizeInner ) + iBarThickness + nFishTaleShift;
	int iOuterLeft = iInnerLeft - iBarSizeOuter;
	int iOuterRight = iInnerRight + iBarSizeOuter;
	int y0 = ( Render::GetScreenSize( ).y / 2.f ) - iBarThickness / 2;
	int y1 = y0 + iBarThickness;

	Render::Engine::Line( iOuterLeft, y0, iInnerLeft, y1, Color::Red( ) );

	float flRadius = fabsf( Render::GetScreenSize( ).x / 2 - iOuterLeft ) * 2.f;
#endif
	Vector2D vecPos = Render::GetScreenSize( ) / 2.f;
	static float flRadiusSmoothed;
	flRadiusSmoothed = GUI::Approach( flRadiusSmoothed, flRadius, g_pGlobalVars->frametime * 24.f );

	vecPos -= flRadiusSmoothed / 2.f;
	vecPos += 1;

	if( flRadiusSmoothed <= 0.f )
		return;

	Render::Engine::Texture( vecPos, Vector2D( flRadiusSmoothed, flRadiusSmoothed ), g_Vars.esp.spread_crosshair == 1 ? ETextures::SPREAD_REGULAR : ETextures::SPREAD_RAINBOW,
							 ( g_Vars.esp.spread_crosshair == 1 ? g_Vars.esp.spread_crosshair_color.ToRegularColor( ) : Color::White( ) ).OverrideAlpha(
								 g_Vars.esp.spread_crosshair == 1 ? 200 : 80 * ( g_Vars.esp.spread_crosshair_opacity / 100.f ), g_Vars.esp.spread_crosshair == 1 ) );

}

bool Visuals::SetupBoundingBox( C_CSPlayer *player, Box_t &box ) {
	// default mins/maxs
	static Vector vecDefaultMaxs = Vector( 16.f, 16.f, 72.f /*54.f when crouching, but whatever*/ );

	Vector vecRenderMaxs = player->GetCollideable( ) ? player->GetCollideable( )->OBBMaxs( ) : Vector( );

	if( player->IsDormant( ) ) {
		if( vecRenderMaxs.z < 54.f )
			vecRenderMaxs = vecDefaultMaxs;
	}

	if( vecRenderMaxs.IsZero( ) )
		return false;

	Vector pos = player->GetAbsOrigin( );
	Vector top = pos + Vector( 0, 0, vecRenderMaxs.z );

	Vector2D pos_screen, top_screen;

	if( !Render::Engine::WorldToScreen( pos, pos_screen ) ||
		!Render::Engine::WorldToScreen( top, top_screen ) )
		return false;

	box.x = int( top_screen.x - ( ( pos_screen.y - top_screen.y ) / 2 ) / 2 );
	box.y = int( top_screen.y );

	box.w = int( ( ( pos_screen.y - top_screen.y ) ) / 2 );
	box.h = int( ( pos_screen.y - top_screen.y ) );

	const bool out_of_fov = pos_screen.x + box.w + 20 < 0 || pos_screen.x - box.w - 20 > Render::Engine::m_width || pos_screen.y + 20 < 0 || pos_screen.y - box.h - 20 > Render::Engine::m_height;

	return !out_of_fov;
}

bool Visuals::SetupBoundingBoxCollision( C_CSPlayer *player, Box_t &box ) {
	// p[asted asf
	const Vector vecRenderMins = player->GetCollideable( ) ? player->GetCollideable( )->OBBMins( ) : Vector( );
	const Vector vecRenderMaxs = player->GetCollideable( ) ? player->GetCollideable( )->OBBMaxs( ) : Vector( );

	if( vecRenderMins.IsZero( ) || vecRenderMaxs.IsZero( ) )
		return false;

	Vector2D vecBoxes[ 8 ];
	matrix3x4_t &pCoordinateFrame = player->m_rgflCoordinateFrame( );

	Vector vecPoints[ ] = {
		Vector( vecRenderMins.x, vecRenderMins.y, vecRenderMins.z ),
		Vector( vecRenderMins.x, vecRenderMaxs.y, vecRenderMins.z ),
		Vector( vecRenderMaxs.x, vecRenderMaxs.y, vecRenderMins.z ),
		Vector( vecRenderMaxs.x, vecRenderMins.y, vecRenderMins.z ),
		Vector( vecRenderMaxs.x, vecRenderMaxs.y, vecRenderMaxs.z ),
		Vector( vecRenderMins.x, vecRenderMaxs.y, vecRenderMaxs.z ),
		Vector( vecRenderMins.x, vecRenderMins.y, vecRenderMaxs.z ),
		Vector( vecRenderMaxs.x, vecRenderMins.y, vecRenderMaxs.z )
	};

	for( int i = 0; i <= 7; i++ ) {
		Vector vecTransformed;
		Math::VectorTransform( vecPoints[ i ], pCoordinateFrame, vecTransformed );

		if( !Render::Engine::WorldToScreen( vecTransformed, vecBoxes[ i ] ) )
			return false;
	}

	Vector2D vecBoxArray[ ] = {
		vecBoxes[ 3 ],
		vecBoxes[ 5 ],
		vecBoxes[ 0 ],
		vecBoxes[ 4 ],
		vecBoxes[ 2 ],
		vecBoxes[ 1 ],
		vecBoxes[ 6 ],
		vecBoxes[ 7 ]
	};

	float left = vecBoxes[ 3 ].x, bottom = vecBoxes[ 3 ].y, right = vecBoxes[ 3 ].x, top = vecBoxes[ 3 ].y;

	for( int i = 0; i <= 7; i++ ) {
		if( left > vecBoxArray[ i ].x )
			left = vecBoxArray[ i ].x;

		if( bottom < vecBoxArray[ i ].y )
			bottom = vecBoxArray[ i ].y;

		if( right < vecBoxArray[ i ].x )
			right = vecBoxArray[ i ].x;

		if( top > vecBoxArray[ i ].y )
			top = vecBoxArray[ i ].y;
	}

	box.x = int( left );
	box.y = int( top );

	box.w = int( right - left );
	box.h = int( bottom - top );

	return true;
}

bool Visuals::IsValidPlayer( C_CSPlayer *entity ) {
	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return false;

	if( !entity )
		return false;

	if( !entity->IsPlayer( ) )
		return false;

	if( entity->IsDead( ) )
		return false;

	if( entity->EntIndex( ) == g_pEngine->GetLocalPlayer( ) )
		return false;

	if( entity->IsTeammate( pLocal ) )
		return false;

	if( g_PlayerList.GetSettings( entity->GetSteamID( ) ).m_bDisableVisuals )
		return false;

	return true;
}

bool Visuals::IsValidEntity( C_BaseEntity *entity ) {
	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return false;

	if( !entity )
		return false;

	if( entity->IsDormant( ) )
		return false;

	return true;
}

void Visuals::RenderBox( const Box_t &box, C_CSPlayer *entity ) {
	Render::Engine::Rect( box.x, box.y, box.w, box.h, DetermineVisualsColor( visuals_config->box_color.ToRegularColor( ).OverrideAlpha( 210, true ), entity ) );
	Render::Engine::Rect( box.x - 1, box.y - 1, box.w + 2, box.h + 2, Color( 0, 0, 0, 180 * player_fading_alpha.at( entity->EntIndex( ) ) ) );
	Render::Engine::Rect( box.x + 1, box.y + 1, box.w - 2, box.h - 2, Color( 0, 0, 0, 180 * player_fading_alpha.at( entity->EntIndex( ) ) ) );
}

void Visuals::RenderTopInfo( const Box_t &box, C_CSPlayer *entity ) {
	int offset = g_Vars.globals.m_bChangedDefaultNameFont ? 0 : 1;
	if( visuals_config->ping ) {
		if( ( g_pPlayerResource.IsValid( ) && ( *g_pPlayerResource.Xor( ) ) ) ) {
			int ping = ( *g_pPlayerResource.Xor( ) )->GetPlayerPing( entity->EntIndex( ) );
			float ping_clamped = std::clamp<float>( static_cast< float >( ping ) / ( g_Vars.sv_maxunlag->GetFloat( ) * 1000.0f ), 0.0f, 1 );

			if( ping_clamped >= 0.2f ) {
				offset += 5;

				const int bar_width = std::clamp<int>( ( box.w + 1 ) * ping_clamped, 0, box.w );

				Render::Engine::RectFilled( box.x - 1, box.y - 6, box.w + 2, 4, Color( 0, 0, 0, 180 * player_fading_alpha.at( entity->EntIndex( ) ) ) );
				Render::Engine::RectFilled( box.x, box.y - 5, bar_width, 2, DetermineVisualsColor( visuals_config->ping_color.ToRegularColor( ).OverrideAlpha( 210, true ), entity ) );
			}
		}
	}

	if( visuals_config->name ) {
		player_info_t info;
		if( g_pEngine->GetPlayerInfo( entity->EntIndex( ), &info ) ) {
			// yeah, performance isn't the best here
			std::string name( info.szName );

			if( name.find( XorStr( "\n" ) ) != std::string::npos ) {
				name = XorStr( "[BLANK]" );
			}

			else {
				if( name.length( ) > 10 ) {
					name.resize( 10 );
					name.append( XorStr( "..." ) );
				}
			}

			Render::Engine::esp_bold.string( box.x + ( box.w / 2 ), box.y - Render::Engine::esp_bold.m_size.m_height - offset,
											 DetermineVisualsColor( visuals_config->name_color.ToRegularColor( ).OverrideAlpha( 240, true ), entity ), name.data( ), Render::Engine::ALIGN_CENTER );
		}
	}
}

void Visuals::RenderHealth( const Box_t &box, C_CSPlayer *entity ) {
	const int bar_size = std::clamp( int( ( entity->m_iHealth( ) * box.h ) / entity->m_iMaxHealth( ) ), 0, box.h );

	Color color;

	if( entity->m_iHealth( ) >= 50 ) {
		float flHealth = std::clamp( entity->m_iHealth( ), 0, 50 );
		color = Color::Blend( Color( 160, 255, 0 ), Color( 207, 127, 0 ), 1.f - ( flHealth / 50.f ) );
	}
	else if( entity->m_iHealth( ) < 50 ) {
		float flHealth = std::clamp( entity->m_iHealth( ), 1, 50 );
		color = Color::Blend( Color( 207, 127, 0 ), Color( 222, 2, 0 ), 1.f - ( flHealth / 50.f ) );
	}

	if( g_Vars.visuals_enemy.health_color_override )
		color = g_Vars.visuals_enemy.health_color.ToRegularColor( );

	Render::Engine::RectFilled( box.x - 6, box.y - 1, 4, box.h + 2, Color( 0, 0, 0, 180 * player_fading_alpha.at( entity->EntIndex( ) ) ) );
	Render::Engine::RectFilled( box.x - 5, box.y + ( box.h - bar_size ), 2, bar_size, DetermineVisualsColor( color.OverrideAlpha( 210, true ), entity ) );

	// draw health amount when it's lethal damage
	if( entity->m_iHealth( ) <= 99 || entity->m_iHealth( ) > entity->m_iMaxHealth( ) ) {
		// note - michal;
		// either sprintf here, or add some nice formatting library
		// std::to_string is slow, could kill some frames when multiple people are being rendered
		// this could also be apparent on a much larger scale on lower-end pcs

		if( g_Vars.globals.m_bHealthBarStringLeft ) {
			Render::Engine::esp_pixel.string( box.x - Render::Engine::esp_pixel.m_size.m_width, box.y - 4, Color( 255, 255, 255, 180 * player_fading_alpha.at( entity->EntIndex( ) ) ), std::to_string( entity->m_iHealth( ) ), Render::Engine::ALIGN_RIGHT );
		}

		else {
			Render::Engine::esp_pixel.string( box.x - 4, box.y + ( box.h - bar_size ) - 8, Color( 255, 255, 255, 180 * player_fading_alpha.at( entity->EntIndex( ) ) ), std::to_string( entity->m_iHealth( ) ), Render::Engine::ALIGN_CENTER );
		}
	}
}

std::string Visuals::GetWeaponIcon( const int id ) {
	auto search = m_WeaponIconsMap.find( id );
	if( search != m_WeaponIconsMap.end( ) )
		return std::string( &search->second, 1 );

	return XorStr( "" );
}

void Visuals::RenderBottomInfo( const Box_t &box, C_CSPlayer *entity ) {
	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	/*if( !entity )
		return;*/

	const auto RoundToMultiple = [&] ( int in, int multiple ) {
		const auto ratio = static_cast< double >( in ) / multiple;
		const auto iratio = std::lround( ratio );
		return static_cast< int >( iratio * multiple );
	};

	const float flDistance = !pLocal->IsDead( ) ? pLocal->GetAbsOrigin( ).Distance( entity->m_vecOrigin( ) ) : ( pLocal->m_hObserverTarget( ).IsValid( ) && pLocal->m_hObserverTarget( ).Get( ) ) ? reinterpret_cast< C_CSPlayer * >( pLocal->m_hObserverTarget( ).Get( ) )->GetAbsOrigin( ).Distance( entity->m_vecOrigin( ) ) : 0.f;

	const float flMeters = flDistance * 0.0254f;
	const float flFeet = flMeters * 3.281f;

	std::string str = std::to_string( RoundToMultiple( flFeet, 5 ) ) + XorStr( " FT" );

	int offset = 3;
	if( visuals_config->lby_timer ) {
		if( auto data = g_Resolver.GetResolverData( entity->EntIndex( ) ); data.m_bPredictingBody && data.IsValidFloat( data.m_flNextBodyUpdate ) ) {
			float flAnimationTime = entity->m_flOldSimulationTime( ) + g_pGlobalVars->interval_per_tick;
			float flUpdateTime = g_Resolver.m_arrResolverData[ entity->EntIndex( ) ].m_flNextBodyUpdate - flAnimationTime;
			float flBoxMultiplier = ( /*1.1f -*/ flUpdateTime ) / 1.1f;

			animated_lby.at( entity->EntIndex( ) ) = GUI::Approach( animated_lby.at( entity->EntIndex( ) ), flBoxMultiplier, g_pGlobalVars->frametime * 10.f );
			if( flBoxMultiplier > animated_lby.at( entity->EntIndex( ) ) )
				animated_lby.at( entity->EntIndex( ) ) = flBoxMultiplier;

			animated_lby.at( entity->EntIndex( ) ) = std::clamp<float>( animated_lby.at( entity->EntIndex( ) ), 0.f, 1.f );

			const int box_width = std::clamp<int>( ( box.w + 1 ) * animated_lby.at( entity->EntIndex( ) ), 0, box.w );

			const auto bIsBreaking = g_Resolver.m_arrResolverData[ entity->EntIndex( ) ].m_eBodyState == BODY_BREAK;
			if( box_width > 1 && bIsBreaking ) {
				Render::Engine::RectFilled( box.x - 1, box.y + box.h + offset - 1, box.w + 2, 4, Color( 0, 0, 0, 180 * player_fading_alpha.at( entity->EntIndex( ) ) ) );
				Render::Engine::RectFilled( box.x, box.y + box.h + offset, box_width, 2, DetermineVisualsColor( visuals_config->lby_timer_color.ToRegularColor( ).OverrideAlpha( 210, true ), entity ) );

				// 4px height
				offset += 5;
			}
		}
	}

	auto pWeapon = reinterpret_cast< C_WeaponCSBaseGun * >( entity->m_hActiveWeapon( ).Get( ) );

	const bool bDormant = entity->IsDormant( );

	CCSWeaponInfo *pWeaponData = nullptr;
	if( !bDormant || ( bDormant && g_ExtendedVisuals.m_arrWeaponInfos[ entity->EntIndex( ) ].second == nullptr ) ) {
		if( pWeapon ) {
			g_ExtendedVisuals.m_arrWeaponInfos[ entity->EntIndex( ) ].first = pWeapon->m_iItemDefinitionIndex( );
			g_ExtendedVisuals.m_arrWeaponInfos[ entity->EntIndex( ) ].second = pWeaponData = pWeapon->GetCSWeaponData( ).Xor( );
		}
	}

	bool bDontDo = false;
	if( !pWeaponData ) {
		if( bDormant ) {
			pWeaponData = g_ExtendedVisuals.m_arrWeaponInfos[ entity->EntIndex( ) ].second;
		}

		if( !pWeaponData ) {
			bDontDo = true;
		}
	}

	if( !bDontDo ) {
		if( visuals_config->ammo && ( ( !bDormant && pWeapon && pWeaponData ) || ( bDormant && pWeaponData ) ) ) {
			const bool bMaxOutAmmo = bDormant; //&& last_non_dormant_weapon.at( entity->EntIndex( ) ) != pWeapon->m_iItemDefinitionIndex( );

			// don't render on knifes, zeus, etc
			bool bIsInvalidWeapon = pWeaponData->m_iWeaponType == WEAPONTYPE_GRENADE || pWeaponData->m_iWeaponType == WEAPONTYPE_KNIFE || pWeaponData->m_iWeaponType == WEAPONTYPE_C4 || pWeaponData->m_iMaxClip <= 0;
			if( !bIsInvalidWeapon ) {
				const int nCurrentClip = bMaxOutAmmo ? pWeaponData->m_iMaxClip : pWeapon->m_iClip1( );

				// also prevent division by zero, lol
				float flBoxMultiplier = ( float )nCurrentClip / pWeaponData->m_iMaxClip;

				bool bReloading = false;
				auto pReloadLayer = entity->m_AnimOverlay( ).Element( 1 );
				if( pReloadLayer.m_pOwner ) {
					const int nActivity = entity->GetSequenceActivity( pReloadLayer.m_nSequence );

					if( nActivity == 967 && pReloadLayer.m_flWeight != 0.f ) {
						// smooth out the ammo bar for reloading players
						flBoxMultiplier = pReloadLayer.m_flCycle;
						bReloading = true;
					}
				}

				const int box_width = std::clamp<int>( ( box.w + 1 ) * flBoxMultiplier, 0, box.w );

				Render::Engine::RectFilled( box.x - 1, box.y + box.h + offset - 1, box.w + 2, 4, Color( 0, 0, 0, 180 * player_fading_alpha.at( entity->EntIndex( ) ) ) );
				Render::Engine::RectFilled( box.x, box.y + box.h + offset, box_width, 2, DetermineVisualsColor( visuals_config->ammo_color.ToRegularColor( ).OverrideAlpha( 210, true ), entity ) );

				// ammo is less than 90% of the max ammo
				if( nCurrentClip > 0 && nCurrentClip <= int( std::floor( float( pWeaponData->m_iMaxClip ) * 0.9f ) ) && !bReloading ) {
					Render::Engine::esp_bold_wpn.string( box.x + box_width, ( box.y + box.h + offset ) - 3, DetermineVisualsColor( Color::White( ).OverrideAlpha( 210, true ), entity ),
														 std::to_string( nCurrentClip ), Render::Engine::ALIGN_CENTER );
				}

				// 4px height
				offset += 4;
			}
		}
	}

	if( !bDontDo ) {
		if( visuals_config->weapon && pWeaponData ) {
			// note - michal;
			// not the best code optimization-wise, again...
			// i'll end up leaving notes everywhere that i'll improve performance later on
			std::wstring localized = g_pLocalize->Find( pWeaponData->m_szHudName );
			std::string name( localized.begin( ), localized.end( ) );
			std::transform( name.begin( ), name.end( ), name.begin( ), g_Vars.globals.m_bWeaponUppercase ? ::toupper : ::tolower );

			Render::Engine::esp_bold_wpn.string( box.x + ( box.w / 2 ), box.y + box.h + offset - 1,
												 DetermineVisualsColor( visuals_config->weapon_color.ToRegularColor( ).OverrideAlpha( 210, true ), entity ), name.data( ), Render::Engine::ALIGN_CENTER );

			const int nAdditionalOffset = g_Vars.globals.m_bChangedDefaultWpnFont ? 0 : 1;
			offset += Render::Engine::esp_bold_wpn.m_size.m_height - nAdditionalOffset;
		}
	}

	if( visuals_config->weapon_icon && ( ( !bDormant && pWeapon ) || bDormant ) ) {
		Render::Engine::cs.string( box.x + ( box.w / 2 ), box.y + box.h + offset - 1,
								   DetermineVisualsColor( visuals_config->weapon_icon_color.ToRegularColor( ).OverrideAlpha( 210, true ), entity ), GetWeaponIcon( bDormant ? g_ExtendedVisuals.m_arrWeaponInfos[ entity->EntIndex( ) ].first : pWeapon->m_iItemDefinitionIndex( ) ), Render::Engine::ALIGN_CENTER );

		offset += Render::Engine::cs.m_size.m_height;
	}
}

void Visuals::RenderSideInfo( const Box_t &box, C_CSPlayer *entity ) {
	std::vector<std::pair<std::string, Color>> vec_flags{ };

	if( g_Ragebot.m_arrNapUsers.at( entity->EntIndex( ) ).first )
		vec_flags.emplace_back( XorStr( "SLEEPY" ), g_Vars.menu.whitelist_disable_key.enabled ? Color( 254, 74, 74 ) : Color( 110, 247, 89 ) );

	if( visuals_config->flag_exploit ) {
		if( auto pAnimData = g_Animations.GetAnimationEntry( entity->EntIndex( ) ); pAnimData ) {
			if( !pAnimData->m_deqRecords.empty( ) ) {
				const bool bExploiting = pAnimData->m_deqRecords.front( ).m_bShiftingTickbase;

				static std::array<float, 65> flLastExploitTime = { };
				if( bExploiting )
					flLastExploitTime[ entity->EntIndex( ) ] = g_pGlobalVars->realtime;

				if( bExploiting || flLastExploitTime[ entity->EntIndex( ) ] > 0.f ) {
					if( bExploiting || fabs( flLastExploitTime[ entity->EntIndex( ) ] - g_pGlobalVars->realtime ) <= TICKS_TO_TIME( 11 ) )
						vec_flags.emplace_back( XorStr( "X" ), Color( 255, 255, 255 ) );
				}
			}
		}
	}

	if( g_Vars.visuals_enemy.flag_money )
		vec_flags.emplace_back( std::string( XorStr( "$" ) ).append( std::to_string( entity->m_iAccount( ) ) ), Color( 155, 210, 100 ) );

	if( entity->m_ArmorValue( ) > 0 && g_Vars.visuals_enemy.flag_armor ) {
		if( entity->m_bHasHelmet( ) )
			vec_flags.emplace_back( XorStr( "HK" ), Color( 255, 255, 255 ) );
		else
			vec_flags.emplace_back( XorStr( "K" ), Color( 255, 255, 255 ) );
	}

	if( entity->m_bIsScoped( ) && g_Vars.visuals_enemy.flag_scope )
		vec_flags.emplace_back( XorStr( "SCOPED" ), Color( 0, 153, 204 ) );

	if( g_Vars.visuals_enemy.flag_utility ) {
		bool bShouldScanForBomb = g_Vars.mp_anyone_can_pickup_c4->GetBool( );
		if( entity->m_iTeamNum( ) == TEAM_TT && !bShouldScanForBomb )
			bShouldScanForBomb = true;

		if( bShouldScanForBomb ) {
			auto hPlayerWeapons = entity->m_hMyWeapons( );
			if( hPlayerWeapons && hPlayerWeapons->IsValid( ) ) {
				for( size_t i = 0; i < 48; ++i ) {
					auto hWeaponHandle = hPlayerWeapons[ i ];
					if( !hWeaponHandle.IsValid( ) )
						break;

					auto pWeapon = ( C_BaseCombatWeapon * )hWeaponHandle.Get( );
					if( !pWeapon )
						continue;

					if( pWeapon->m_Item( ).m_iItemDefinitionIndex( ) == WEAPON_C4 )
						vec_flags.emplace_back( XorStr( "BOMB" ), Color( 255, 100, 0 ) );
				}
			}
		}

		if( entity->m_bHasDefuser( ) ) {
			vec_flags.emplace_back( XorStr( "KIT" ), Color( 255, 255, 255 ) );
		}

		if( entity->m_bIsDefusing( ) ) {
			vec_flags.emplace_back( XorStr( "DEFUSE" ), Color( 255, 0, 0 ) );
		}
	}

	if( entity->IsReloading( ) && g_Vars.visuals_enemy.flag_reloading )
		vec_flags.emplace_back( XorStr( "RELOAD" ), Color( 0, 153, 204 ) );

	float m_flFlashBangTime = *( float * )( ( uintptr_t )entity + 0xA2E8 );
	if( m_flFlashBangTime > 0.f && g_Vars.visuals_enemy.flag_flashed )
		vec_flags.emplace_back( XorStr( "BLIND" ), Color( 0, 153, 204 ) );

	if( g_Vars.visuals_enemy.flag_ping && g_pPlayerResource.IsValid( ) && ( *g_pPlayerResource.Xor( ) ) ) {
		int ping = ( *g_pPlayerResource.Xor( ) )->GetPlayerPing( entity->EntIndex( ) );

		// lerp ping from 0 - 300
		const float ping_clamped = std::clamp<float>( static_cast< float >( ping / 300.f ), 0.0f, 1 );
		Color ping_flag_color = Color::Blend( Color( 138, 232, 84 ), Color( 245, 66, 66 ), ping_clamped );

		// above 300 is considered high, mark it red (final lerped color)
		if( ping >= 300 )
			ping_flag_color = Color( 245, 66, 66 );

		vec_flags.emplace_back( std::to_string( ping ) + " ms", ping_flag_color );
	}

	if( auto pAnimData = g_Animations.GetAnimationEntry( entity->EntIndex( ) ); pAnimData ) {
		if( !pAnimData->m_deqRecords.empty( ) ) {
			if( visuals_config->flag_resolver || visuals_config->flag_resolver_mode || visuals_config->flag_resolver_body || visuals_config->flag_debug ) {
				auto &resolverData = g_Resolver.m_arrResolverData.at( entity->EntIndex( ) );

				// this probably shouldn't be here, move it later (same with ragebot).
				auto ConfData = [&] ( EConfidence conf ) -> std::pair<std::string, Color> {
					switch( conf ) {
						case EConfidence::CONF_LOW:
							return { "LOW", Color( 255, 0, 0 ) };
							break;
						case EConfidence::CONF_MED:
							return { "MEDIUM", Color( 255, 150, 0 ) };
							break;
						case EConfidence::CONF_HIGH:
							return { "HIGH", Color( 150, 160, 0 ) };
							break;
						case EConfidence::CONF_VHIGH:
							return { "VERY HIGH", Color( 200, 255, 0 ) };
							break;
					}

					return { "?", Color( 255, 0, 0 ) };
				};

				// break data
				auto BreakData = [&] ( EBodyState state ) -> std::pair<std::string, Color> {
					switch( state ) {
						case EBodyState::BODY_BREAK:
							return { "BREAKING", Color( 255, 0, 0 ) };
							break;
						case EBodyState::BODY_DEFAULT:
							return { "DEFAULT", Color( 69, 186, 245 ) };
							break;
					}

					return { "?", Color( 255, 0, 0 ) };
				};

				if( pAnimData->m_deqRecords.size( ) ) {
					bool bFakingAngles = !pAnimData->m_deqRecords.front( ).m_bIsResolved && pAnimData->m_deqRecords.front( ).m_iResolverType != 2;
					// прогноз живёт в AnimationEntry_t::m_PredictedRecord, а не в m_deqRecords,
					// поэтому у front-записи m_bExtrapolated никогда не стоит - индикацию
					// берём из m_bPredictionValid. сверка тика обязательна: флаг переписывается
					// только при вызове ExtrapolateRecords, а он может не вызваться вообще -
					// без неё "LC" залипал бы до конца раунда.
					bool bExtrapolated = ( pAnimData->m_bPredictionValid
						&& pAnimData->m_nPredictedFromTick == pAnimData->m_deqRecords.front( ).m_nServerTick )
						|| pAnimData->m_deqRecords.front( ).m_bBrokeTeleportDst;
					bool bDelayed = pAnimData->m_bDelay;

					// resolver confidence & lagcomp
					if( visuals_config->flag_resolver ) {
						if( bExtrapolated )
							vec_flags.emplace_back( std::string( XorStr( "LC" ) ), bDelayed ? Color( 150, 200, 60 ) : Color( 240, 170, 50 ) );
						else
							vec_flags.emplace_back( ConfData( pAnimData->m_deqRecords.front( ).m_eConfidence ) );
					}

					// these are only in dev
				#if defined(DEV)
				// resolver mode in string
					if( visuals_config->flag_resolver_mode )
						vec_flags.emplace_back( std::string( pAnimData->m_deqRecords.front( ).m_szResolver.data( ) ), pAnimData->m_deqRecords.front( ).m_bIsResolved ? Color( 200, 255, 0 ) : Color( 100, 100, 100 ) );

					// only need these if they are standing
					if( visuals_config->flag_resolver_body && pAnimData->m_deqRecords.front( ).m_eResolverStage == RES_STAND ) {
						// additional, "break" data
						vec_flags.emplace_back( BreakData( g_Resolver.m_arrResolverData[ entity->EntIndex( ) ].m_eBodyState ) );

						// lower body yaw
						vec_flags.emplace_back( "LBY: " + std::to_string( pAnimData->m_deqRecords.front( ).m_flLowerBodyYawTarget ), Color( 255, 255, 255 ) );

						// lby diff check is the reason they're determined "default"
						if( g_Resolver.m_arrResolverData[ entity->EntIndex( ) ].m_bHasBeenStatic )
							vec_flags.emplace_back( "STATIC", Color( 255, 255, 255 ) );
					}

					if( visuals_config->flag_debug ) {
						vec_flags.emplace_back( std::to_string( pAnimData->m_deqRecords.front( ).m_vecPredVelocity.Length2D( ) ), Color( 255, 255, 255 ) );

						// проверка достижимости экстраполяции: если счётчик стоит на нуле,
						// фоллбэк в Ragebot не срабатывает ни разу, сколько бы кода там ни было
						vec_flags.emplace_back( "EXT: " + std::to_string( pAnimData->m_nPredictionHits ), Color( 240, 170, 50 ) );

					#if 1
						char buffer[ 128 ] = {};
						/*sprintf( buffer, "move playback: %.6f\n", pAnimData->m_deqRecords.front( ).m_pServerAnimOverlays[ ANIMATION_LAYER_MOVEMENT_MOVE ].m_flPlaybackRate );
						vec_flags.emplace_back( buffer, Color( 255, 255, 255 ) );

						sprintf( buffer, "move weight: %.6f\n", pAnimData->m_deqRecords.front( ).m_pServerAnimOverlays[ ANIMATION_LAYER_MOVEMENT_MOVE ].m_flWeight );
						vec_flags.emplace_back( buffer, Color( 255, 255, 255 ) );

						sprintf( buffer, "move cycle: %.6f\n", pAnimData->m_deqRecords.front( ).m_pServerAnimOverlays[ ANIMATION_LAYER_MOVEMENT_MOVE ].m_flCycle );
						vec_flags.emplace_back( buffer, Color( 255, 255, 255 ) );

						sprintf( buffer, "velo length: %.4f\n", pAnimData->m_deqRecords.front( ).m_vecPredVelocity.Length2D( ) );
						vec_flags.emplace_back( buffer, Color( 255, 255, 255 ) );*/

						auto &layer5 = pAnimData->m_deqRecords.front( ).m_sAnims[ ESides::SIDE_SERVER ].m_pServerAnimOverlays[ ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB ];
						auto &layer4 = pAnimData->m_deqRecords.front( ).m_sAnims[ ESides::SIDE_SERVER ].m_pServerAnimOverlays[ ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL ];

						if( pAnimData->m_deqRecords.front( ).m_bPotentialDesync ) {
							sprintf( buffer, "potential anim desync" );
							vec_flags.emplace_back( buffer, Color( 255, 255, 255 ) );
						}

						sprintf( buffer, "layer 5 act: %i\n", entity->GetSequenceActivity( layer5.m_nSequence ) );
						vec_flags.emplace_back( buffer, Color( 255, 255, 255 ) );

						sprintf( buffer, "layer 5 playback: %.6f\n", layer5.m_flPlaybackRate );
						vec_flags.emplace_back( buffer, Color( 255, 255, 255 ) );

						sprintf( buffer, "layer 5 cycle: %.6f\n", layer5.m_flCycle );
						vec_flags.emplace_back( buffer, Color( 255, 255, 255 ) );

						sprintf( buffer, "layer 5 weight: %.6f\n", layer5.m_flWeight );
						vec_flags.emplace_back( buffer, Color( 255, 255, 255 ) );

						sprintf( buffer, "layer 4 act: %i\n", entity->GetSequenceActivity( layer4.m_nSequence ) );
						vec_flags.emplace_back( buffer, Color( 255, 255, 255 ) );

						sprintf( buffer, "layer 4 playback: %.6f\n", layer4.m_flPlaybackRate );
						vec_flags.emplace_back( buffer, Color( 255, 255, 255 ) );

						sprintf( buffer, "layer 4 cycle: %.6f\n", layer4.m_flCycle );
						vec_flags.emplace_back( buffer, Color( 255, 255, 255 ) );

						sprintf( buffer, "layer 4 weight: %.6f\n", layer4.m_flWeight );
						vec_flags.emplace_back( buffer, Color( 255, 255, 255 ) );

						if( auto pState = entity->m_PlayerAnimState( ); pState ) {

							sprintf( buffer, "land anim multiplier: %.6f\n", pState->m_flLandAnimMultiplier );
							vec_flags.emplace_back( buffer, Color( 255, 255, 255 ) );
							
							sprintf( buffer, "duck additional: %.6f\n", pState->m_flDuckAdditional );
							vec_flags.emplace_back( buffer, Color( 255, 255, 255 ) );
							
							sprintf( buffer, "in air smooth: %.6f\n", pState->m_flInAirSmoothValue );
							vec_flags.emplace_back( buffer, Color( 255, 255, 255 ) );
							
							sprintf( buffer, "strafe change weight: %.6f\n", pState->m_flStrafeChangeWeight );
							vec_flags.emplace_back( buffer, Color( 255, 255, 255 ) );
						}
					#endif
					}

				#endif
					// vec_flags.emplace_back( std::string( XorStr( "LBY" ) ), bFakingAngles ? Color::Red( ) : Color( 150, 200, 60 ) );
				}
			}
		}
	}

	int offset{ 0 };
	for( auto flag : vec_flags ) {
		std::transform( flag.first.begin( ), flag.first.end( ), flag.first.begin( ), g_Vars.globals.m_bFlagsUppercase ? ::toupper : ::tolower );

		// draw the string
		Render::Engine::esp_pixel.string( box.x + box.w + 3, box.y - 3 + offset, DetermineVisualsColor( flag.second.OverrideAlpha( 240, true ), entity ), flag.first );

		// extend offset
		offset += Render::Engine::esp_pixel.m_size.m_height - g_Vars.globals.m_nFlagsOffset;
	}
}

void Visuals::RenderDroppedWeapons( C_BaseEntity *entity ) {
	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	auto pWeapon = reinterpret_cast< C_WeaponCSBaseGun * >( entity );
	if( !pWeapon )
		return;

	if( !pWeapon->m_iItemDefinitionIndex( ) || pWeapon->m_hOwnerEntity( ) != -1 )
		return;

	auto pWeaponData = pWeapon->GetCSWeaponData( );
	if( !pWeaponData.IsValid( ) )
		return;

	Vector2D screen_position{ };
	if( !Render::Engine::WorldToScreen( pWeapon->GetAbsOrigin( ), screen_position ) )
		return;

	auto clientClass = entity->GetClientClass( );
	if( !clientClass )
		return;

	auto bIsC4 = clientClass->m_ClassID == CC4;

	// note - michal;
	// not the best code optimization-wise, again...
	// i'll end up leaving notes everywhere that i'll improve performance later on
	std::wstring localized = g_pLocalize->Find( pWeaponData->m_szHudName );
	std::string name( localized.begin( ), localized.end( ) );
	std::transform( name.begin( ), name.end( ), name.begin( ), g_Vars.globals.m_bWeaponUppercase ? ::toupper : ::tolower );

	if( name.empty( ) )
		return;

	// LOL
	float distance = !pLocal->IsDead( ) ? pLocal->GetAbsOrigin( ).Distance( entity->m_vecOrigin( ) ) : ( pLocal->m_hObserverTarget( ).IsValid( ) && pLocal->m_hObserverTarget( ).Get( ) ) ? reinterpret_cast< C_CSPlayer * >( pLocal->m_hObserverTarget( ).Get( ) )->GetAbsOrigin( ).Distance( entity->m_vecOrigin( ) ) : 0.f;

	const auto clamped_distance = std::clamp<float>( distance - 300.f, 0.f, 360.f );
	float alpha = bIsC4 ? 180.f : 180.f - ( clamped_distance * 0.5f );

	if( alpha < 0.f )
		return;

	if( g_Vars.esp.dropped_weapons || ( bIsC4 && g_Vars.esp.draw_c4_2d ) ) {
		Render::Engine::esp_bold_wpn.string( screen_position.x, screen_position.y,
											 ( bIsC4 && g_Vars.esp.draw_c4_2d ) ? Color( 150, 200, 60 ).OverrideAlpha( 180, true ) : g_Vars.esp.dropped_weapons_color.ToRegularColor( ).OverrideAlpha( alpha, true ),
											 ( bIsC4 && g_Vars.esp.draw_c4_2d ) ? XorStr( "C4" ) : name.data( ) );
	}

	if( g_Vars.esp.dropped_weapons_ammo && !bIsC4 ) {
		const auto clip = pWeapon->m_iClip1( );
		if( clip > 0 ) {
			const auto text_size = Render::Engine::esp_bold_wpn.size( name );
			const auto max_clip = pWeaponData->m_iMaxClip;

			auto width = text_size.m_width;
			width *= clip;

			// even tho max_clip should never be 0, better safe..
			if( max_clip )
				width /= max_clip;

			// outline
			Render::Engine::RectFilled( screen_position + Vector2D( 0, 9 ), Vector2D( text_size.m_width + 1, 4 ), Color( 0.f, 0.f, 0.f, alpha ) );

			// actual bar
			Render::Engine::RectFilled( screen_position + Vector2D( 1, 10 ), Vector2D( width - 1, 2 ), g_Vars.esp.dropped_weapons_ammo_color.ToRegularColor( ).OverrideAlpha( alpha ) );

			// draw bullets in clip
			if( clip <= static_cast< int >( max_clip * 0.34 ) ) {
				Render::Engine::esp_bold_wpn.string( screen_position.x + width, screen_position.y + 8, Color::White( ).OverrideAlpha( alpha ), std::to_string( clip ) );
			}
		}
	}
}

void Visuals::RenderNades( C_BaseEntity *entity ) {
	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	if( !entity )
		return;

	// yeah nigga I grab it here? got a problem, huh?
	//static auto m_nExplodeEffectTickBeginOffsetz = Engine::g_PropManager.GetOffset( XorStr( "DT_BaseCSGrenadeProjectile" ), XorStr( "m_nExplodeEffectTickBegin" ) );

	auto client_class = entity->GetClientClass( );
	if( !client_class )
		return;

	std::string grenade{ };

	// CInferno does not have a model...
	if( client_class->m_ClassID != CInferno ) {
		auto model = entity->GetModel( );
		if( !model )
			return;

		std::string model_name = g_pModelInfo->GetModelName( model );
		if( model_name.empty( ) )
			return;

		const auto smoke = reinterpret_cast< C_SmokeGrenadeProjectile * >( entity );
		switch( client_class->m_ClassID ) {
			case CBaseCSGrenadeProjectile:
				//if( *( int * )( uintptr_t( entity ) + m_nExplodeEffectTickBeginOffsetz ) ) {
				//	return;
				//}

				// seriously, just why?
				// this game is so shit
				if( model_name.find( XorStr( "fraggrenade" ) ) != std::string::npos ) {
					grenade = XorStr( "frag" );
				}
				else {
					grenade = XorStr( "flash" );
				}
				break;
			case CMolotovProjectile:
				grenade = XorStr( "fire" );
				break;
			case CSmokeGrenadeProjectile:
				grenade = XorStr( "smoke" );

				// apparently m_bDidSmokeEffect doesn't seem to work?
				if( smoke ) {
					const auto spawn_time = TICKS_TO_TIME( smoke->m_nSmokeEffectTickBegin( ) );
					const auto time = ( spawn_time + C_SmokeGrenadeProjectile::GetExpiryTime( ) ) - g_pGlobalVars->curtime;
					const auto factor = time / C_SmokeGrenadeProjectile::GetExpiryTime( );

					if( factor > 0.0f ) {
						grenade.clear( );
					}
				}

				break;
			case CDecoyProjectile:
				grenade = XorStr( "decoy" );
				break;
		}
	}

	Vector2D vScreenPosition{ };
	if( !Render::Engine::WorldToScreen( entity->GetAbsOrigin( ), vScreenPosition ) )
		return;

	Vector vecFromOrigin = pLocal->GetAbsOrigin( );
	if( pLocal->IsDead( ) ) {
		if( pLocal->m_hObserverTarget( ).IsValid( ) && pLocal->m_hObserverTarget( ).Get( ) && pLocal->m_iObserverMode( ) != /*OBS_MODE_ROAMING*/6 ) {
			vecFromOrigin = reinterpret_cast< C_CSPlayer * >( pLocal->m_hObserverTarget( ).Get( ) )->GetAbsOrigin( );
		}
	}

	float flDistance = vecFromOrigin.Distance( entity->m_vecOrigin( ) );
	/*const auto flClampedDistance = std::clamp<float>( flDistance - 300.f, 0.f, 360.f );
	float flAlpha = std::clamp( ( 180.f - ( flClampedDistance * 0.5f ) ) / 180.f, 0.f, 1.f );*/

	// draw nade string..
	if( !grenade.empty( ) && client_class->m_ClassID != CInferno && g_Vars.esp.grenades ) {
		Render::Engine::esp_bold.string( vScreenPosition.x, vScreenPosition.y,
			Color::White( ), grenade, Render::Engine::ALIGN_CENTER );
	}

	static const auto vSize = Vector2D( 70.f, 4.f );
	const auto vScaledPosition = Vector2D( vScreenPosition.x - vSize.x * 0.5, vScreenPosition.y - vSize.y * 0.5 );

	if( entity->GetClientClass( )->m_ClassID == CInferno ) {
		C_Inferno *pInferno = reinterpret_cast< C_Inferno * >( entity );
		C_CSPlayer *pOwner = ( C_CSPlayer * )entity->m_hOwnerEntity( ).Get( );

		if( pOwner ) {
			bool bDraw = true;
			if( auto pLocal = C_CSPlayer::GetLocalPlayer( ); pLocal ) {
				if( pOwner->m_iTeamNum( ) == pLocal->m_iTeamNum( ) && pOwner->EntIndex( ) != g_pEngine->GetLocalPlayer( ) ) {
					if( g_Vars.mp_friendlyfire->GetInt( ) != 1 ) {
						bDraw = false;
					}
				}
			}

			const Vector vecOrigin = pInferno->GetAbsOrigin( );
			Vector2D vScreenOrigin = Vector2D( );

			if( Render::Engine::WorldToScreen( vecOrigin, vScreenOrigin ) && bDraw ) {
				const auto flSpawnTime = pInferno->m_flSpawnTime( );
				const auto flTime = ( ( flSpawnTime + C_Inferno::GetExpiryTime( ) ) - g_pGlobalVars->curtime );

				if( flTime > 0.05f ) {
					Vector vecMin, vecMax;
					if( entity->GetClientRenderable( ) )
						entity->GetClientRenderable( )->GetRenderBounds( vecMin, vecMax );

					auto flRadius = ( vecMax - vecMin ).Length2D( ) * 0.5f;
					Vector vecBoundOrigin = Vector( ( vecMin.x + vecMax.x ) * 0.5f, ( vecMin.y + vecMax.y ) * 0.5f, vecMin.z + 5 ) + vecOrigin;
					constexpr int ACCURACY = 128;
					const float flStep = DirectX::XM_2PI / ACCURACY;
					for( float a = 0.0f; a < DirectX::XM_2PI; a += flStep ) {
						float a_c, a_s, as_c, as_s;
						DirectX::XMScalarSinCos( &a_s, &a_c, a );
						DirectX::XMScalarSinCos( &as_s, &as_c, a + flStep );

						Vector startPos = Vector( a_c * flRadius + vecBoundOrigin.x, a_s * flRadius + vecBoundOrigin.y, vecBoundOrigin.z );
						Vector endPos = Vector( as_c * flRadius + vecBoundOrigin.x, as_s * flRadius + vecBoundOrigin.y, vecBoundOrigin.z );


						Vector2D start2d, end2d, boundorigin2D;
						if( Render::Engine::WorldToScreen( startPos, start2d ) && Render::Engine::WorldToScreen( endPos, end2d ) && Render::Engine::WorldToScreen( vecBoundOrigin, boundorigin2D ) ) {
							if( g_Vars.esp.grenades_radius && g_Vars.esp.grenades_radius_smoke )
								Render::Engine::Line( start2d, end2d, g_Vars.esp.grenades_radius_fire_color.ToRegularColor( ).OverrideAlpha( 255 /** flAlpha*/, true ) );
						}
					}

					if( g_Vars.esp.grenades ) {
						const float flMagic = std::clamp( std::clamp( flTime, 0.f, C_Inferno::GetExpiryTime( ) ) / ( C_Inferno::GetExpiryTime( ) ), 0.f, 1.f );

						char time_buffer[ 128 ] = {};
						sprintf( time_buffer, XorStr( "FIRE (%.1fS)" ), flTime );

						// c4 timer bar
						Render::Engine::RectFilled( vScreenOrigin - Vector2D( 30, -10 ) - 1, Vector2D( 60, 2 ) + 2, Color( 0.f, 0.f, 0.f, 180 /** flAlpha*/ ) );
						Render::Engine::RectFilled( vScreenOrigin - Vector2D( 30, -10 ), Vector2D( 60 * flMagic, 2 ), g_Vars.esp.grenades_color.ToRegularColor( ).OverrideAlpha( 255 /** flAlpha*/, true ) );

						// main text
						Render::Engine::esp_bold_wpn.string( vScreenOrigin.x, vScreenOrigin.y, Color( 255, 255, 255, 240 /** flAlpha*/ ), time_buffer, Render::Engine::ALIGN_CENTER );
					}
				}
			}
		}
	}

	C_SmokeGrenadeProjectile *pSmokeEffect = reinterpret_cast< C_SmokeGrenadeProjectile * >( entity );
	if( pSmokeEffect->GetClientClass( )->m_ClassID == CSmokeGrenadeProjectile ) {
		const Vector origin = pSmokeEffect->GetAbsOrigin( );
		Vector2D screen_origin = Vector2D( );

		if( Render::Engine::WorldToScreen( origin, screen_origin ) ) {
			const auto spawn_time = TICKS_TO_TIME( pSmokeEffect->m_nSmokeEffectTickBegin( ) );
			const auto time = ( spawn_time + C_SmokeGrenadeProjectile::GetExpiryTime( ) ) - g_pGlobalVars->curtime;

			static const auto size = Vector2D( 70.f, 4.f );

			if( time > 0.05f ) {
				auto radius = 144.f;

				const int accuracy = 128;
				const float step = DirectX::XM_2PI / accuracy;
				for( float a = 0.0f; a < DirectX::XM_2PI; a += step ) {
					float a_c, a_s, as_c, as_s;
					DirectX::XMScalarSinCos( &a_s, &a_c, a );
					DirectX::XMScalarSinCos( &as_s, &as_c, a + step );

					Vector startPos = Vector( a_c * radius + origin.x, a_s * radius + origin.y, origin.z + 5 );
					Vector endPos = Vector( as_c * radius + origin.x, as_s * radius + origin.y, origin.z + 5 );

					Vector2D start2d, end2d;
					if( Render::Engine::WorldToScreen( startPos, start2d ) && Render::Engine::WorldToScreen( endPos, end2d ) ) {
						if( g_Vars.esp.grenades_radius && g_Vars.esp.grenades_radius_smoke )
							Render::Engine::Line( start2d, end2d, g_Vars.esp.grenades_radius_smoke_color.ToRegularColor( ).OverrideAlpha( 255 /** flAlpha*/, true ) );
					}
				}

				if( g_Vars.esp.grenades ) {
					const float flMagic = std::clamp( std::clamp( time, 0.f, C_SmokeGrenadeProjectile::GetExpiryTime( ) ) / ( C_SmokeGrenadeProjectile::GetExpiryTime( ) ), 0.f, 1.f );

					char time_buffer[ 128 ] = {};
					sprintf( time_buffer, XorStr( "SMOKE (%.1fS)" ), time );

					// c4 timer bar
					Render::Engine::RectFilled( screen_origin - Vector2D( 30, -10 ) - 1, Vector2D( 60, 2 ) + 2, Color( 0.f, 0.f, 0.f, 180 /** flAlpha*/ ) );
					Render::Engine::RectFilled( screen_origin - Vector2D( 30, -10 ), Vector2D( 60 * flMagic, 2 ), g_Vars.esp.grenades_color.ToRegularColor( ).OverrideAlpha( 255 /** flAlpha*/, true ) );

					// main text
					Render::Engine::esp_bold_wpn.string( screen_origin.x, screen_origin.y, Color( 255, 255, 255, 240 /** flAlpha*/ ), time_buffer, Render::Engine::ALIGN_CENTER );
				}
				if( vecSmokesOrigin.find( entity->EntIndex( ) ) == vecSmokesOrigin.end( ) ) {
					vecSmokesOrigin.insert( { entity->EntIndex( ), pSmokeEffect->GetAbsOrigin( ) } );
				}

			}
			else {
				if( vecSmokesOrigin.size( ) ) {
					if( vecSmokesOrigin.find( entity->EntIndex( ) ) != vecSmokesOrigin.end( ) ) {
						vecSmokesOrigin.erase( entity->EntIndex( ) );
					}
				}
			}
		}
	}
}
std::string Visuals::GetBombSite( C_PlantedC4 *entity ) {
	if( !g_pPlayerResource.IsValid( ) || !( *g_pPlayerResource.Xor( ) ) )
		return XorStr( "Error while finding bombsite..." );

	const auto &origin = entity->GetAbsOrigin( );

	// gosh I hate dereferencing it here!
	const auto &bomb_a = ( *g_pPlayerResource.Xor( ) )->m_bombsiteCenterA( );
	const auto &bomb_b = ( *g_pPlayerResource.Xor( ) )->m_bombsiteCenterB( );

	const auto dist_a = origin.Distance( bomb_a );
	const auto dist_b = origin.Distance( bomb_b );

	return dist_a < dist_b ? XorStr( "A" ) : XorStr( "B" );
}

void Visuals::RenderObjectives( C_BaseEntity *entity ) {
	if( !entity )
		return;

	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	auto client_class = entity->GetClientClass( );
	if( !client_class )
		return;

	if( client_class->m_ClassID == CPlantedC4 ) {
		const auto plantedc4 = reinterpret_cast< C_PlantedC4 * >( entity );

		if( !plantedc4 )
			return;

		const float c4timer = g_Vars.mp_c4timer->GetFloat( );

		// note - nico;
		// I don't know if we should clamp this to mp_c4timer->GetFloat( )
		// if the mp_c4timer changes while the c4 is planted to something lower than the remaining time
		// it will clamp it.. (this should never really happen, but yeah)
		const float time = std::clamp( plantedc4->m_flC4Blow( ) - g_pGlobalVars->curtime, 0.f, c4timer );

		if( time && !plantedc4->m_bBombDefused( ) ) {
			// SUPREMACY SUPREMACY SUPREMACY SUPREMACY SUPREMACY SUPREMACY SUPREMACY SUPREMACY SUPREMACY SUPREMACY 

			CGameTrace tr;
			CTraceFilter filter;
			auto explosion_origin = plantedc4->GetAbsOrigin( );
			auto explosion_origin_adjusted = explosion_origin;
			explosion_origin_adjusted.z += 8.f;

			// setup filter and do first trace.
			filter.pSkip = plantedc4;

			g_pEngineTrace->TraceRay(
				Ray_t( explosion_origin_adjusted, explosion_origin_adjusted + Vector( 0.f, 0.f, -40.f ) ),
				MASK_SOLID,
				&filter,
				&tr
			);

			// pull out of the wall a bit.
			if( tr.fraction != 1.f )
				explosion_origin = tr.endpos + ( tr.plane.normal * 0.6f );

			// this happens inside CCSGameRules::RadiusDamage.
			explosion_origin.z += 1.f;

			// set all other vars.
			auto m_planted_c4_explosion_origin = explosion_origin;

			auto dst = pLocal->WorldSpaceCenter( );
			auto to_target = m_planted_c4_explosion_origin - dst;
			auto dist = to_target.Length( );

			// calculate the bomb damage based on our distance to the C4's explosion.
			float range_damage = 500.f * std::exp( ( dist * dist ) / ( ( ( ( 500.f * 3.5f ) / 3.f ) * -2.f ) * ( ( 500.f * 3.5f ) / 3.f ) ) );

			static auto scale_damage = [ ] ( float damage, int armor_value ) {
				float new_damage, armor;

				if( armor_value > 0 ) {
					new_damage = damage * 0.5f;
					armor = ( damage - new_damage ) * 0.5f;

					if( armor > ( float )armor_value ) {
						armor = ( float )armor_value * 2.f;
						new_damage = damage - armor;
					}

					damage = new_damage;
				}

				return std::max( 0, ( int )std::floor( damage ) );
			};

			// now finally, scale the damage based on our armor (if we have any).
			float final_damage = scale_damage( range_damage, pLocal->m_ArmorValue( ) );

			// we can clamp this in range 0-10, it can't be higher than 10, lol!
			const float remaining_defuse_time = std::clamp( plantedc4->m_flDefuseCountDown( ) - g_pGlobalVars->curtime, 0.f, 10.f );

			const float factor_c4 = time / ( ( c4timer != 0.f ) ? c4timer : 40.f );
			const float factor_defuse = remaining_defuse_time / 10.f;

			char time_buf[ 128 ] = { };
			sprintf( time_buf, XorStr( "%.2fs"/* " - %.1fs"*/ ), time );

			char dmg_buf[ 128 ] = { };
			sprintf( dmg_buf, XorStr( "-%d HP" ), int( final_damage ) );

			char defuse_buf[ 128 ] = { };
			sprintf( defuse_buf, XorStr( "%.1fs" ), remaining_defuse_time );

			// compute bombsite string
			const auto bomb_site = GetBombSite( plantedc4 ).append( time_buf );

			if( g_Vars.esp.draw_c4_2d ) {
				const auto screen = Render::GetScreenSize( );
				static const auto size = Vector2D( 160.f, 3.f );

				const float width_c4 = size.x * factor_c4;

				Color site_color = Color( 150, 200, 60, 220 );
				if( time <= 10.f ) {
					site_color = Color( 255, 255, 185, 220 );

					if( time <= 5.f ) {
						site_color = Color( 255, 0, 0, 220 );
					}
				}

				// is this thing being defused?
				if( plantedc4->m_bBeingDefused( ) ) {
					const float width_defuse = size.y * factor_defuse;

					// background
					Render::Engine::RectFilled( Vector2D( 0, 0 ), Vector2D( 20, screen.y ), Color( 0, 0, 0, 100 ) );

					// defuse timer bar
					int height = ( screen.y - 2 ) * factor_defuse;
					Render::Engine::RectFilled( Vector2D( 1, 1 + ( int )abs( screen.y - height ) ), Vector2D( 18, height ), Color( 30, 170, 30 ) );
				}

				// draw bomb site string
				//if( time > 0.f )
				//	Render::Engine::esp_indicator.string( 8, 8, site_color, bomb_site, Render::Engine::ALIGN_LEFT );

				//if( final_damage > 0 )
				//	Render::Engine::esp_indicator.string( 8, 36,
				//										  final_damage >= pLocal->m_iHealth( ) ? Color( 255, 0, 0 ) : Color( 255, 255, 185 ), final_damage >= pLocal->m_iHealth( ) ? XorStr( "FATAL" ) : dmg_buf, Render::Engine::ALIGN_LEFT );
			}

			if( g_Vars.esp.draw_c4_3d ) {
				static const auto size = Vector2D( 80.f, 3.f );

				Vector2D screen{ };
				if( Render::Engine::WorldToScreen( entity->GetAbsOrigin( ), screen ) ) {
					const float width_c4 = size.x * factor_c4;

					// draw bomb site string
					Render::Engine::esp_bold_wpn.string( screen.x, screen.y - Render::Engine::esp_pixel.m_size.m_height - 1, Color( 255, 255, 255 ).OverrideAlpha( 180, true ), XorStr( "C4" ), Render::Engine::ALIGN_CENTER );

					// draw bomb remaining time
					if( time > 0.f )
						Render::Engine::esp_bold_wpn.string( screen.x, screen.y - Render::Engine::esp_pixel.m_size.m_height + 8, Color( 255, 0, 0 ).OverrideAlpha( 180, true ), time_buf, Render::Engine::ALIGN_CENTER );
				}
			}
		}
	}
}

void Visuals::RenderArrows( C_BaseEntity *entity ) {
	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	auto RotateArrow = [ ] ( std::array< Vector2D, 3 > &points, float rotation ) {
		const auto vecPointsCenter = ( points.at( 0 ) + points.at( 1 ) + points.at( 2 ) ) / 3;
		for( auto &point : points ) {
			point -= vecPointsCenter;

			const auto temp_x = point.x;
			const auto temp_y = point.y;

			const auto theta = DEG2RAD( rotation );
			const auto c = cos( theta );
			const auto s = sin( theta );

			point.x = temp_x * c - temp_y * s;
			point.y = temp_x * s + temp_y * c;

			point += vecPointsCenter;
		}
	};

	const float flWidth = Render::GetScreenSize( ).x;
	const float flHeight = Render::GetScreenSize( ).y;
	if( !entity || !entity->IsPlayer( ) || entity == pLocal || entity->m_iTeamNum( ) == pLocal->m_iTeamNum( ) )
		return;

	static float alpha[ 65 ];
	static bool plus_or_minus[ 65 ];
	if( alpha[ entity->EntIndex( ) ] <= 5 || alpha[ entity->EntIndex( ) ] >= 255 )
		plus_or_minus[ entity->EntIndex( ) ] = !plus_or_minus[ entity->EntIndex( ) ];

	alpha[ entity->EntIndex( ) ] += plus_or_minus[ entity->EntIndex( ) ] ? ( 255.f / 1.f * g_pGlobalVars->frametime ) : -( 255.f / 1.f * g_pGlobalVars->frametime );
	alpha[ entity->EntIndex( ) ] = std::clamp<float>( alpha[ entity->EntIndex( ) ], 5.f, 255.f );

	Vector2D vecScreenPos;
	const bool bWorldToScreened = Render::Engine::WorldToScreen( entity->GetAbsOrigin( ), vecScreenPos );

	// give some extra room for screen position to be off screen.
	const float flExtraRoomX = flWidth / 18.f;
	const float flExtraRoomY = flHeight / 18.f;

	if( !bWorldToScreened
		|| vecScreenPos.x < -flExtraRoomX
		|| vecScreenPos.x >( flWidth + flExtraRoomX )
		|| vecScreenPos.y < -flExtraRoomY
		|| vecScreenPos.y >( flHeight + flExtraRoomY ) ) {
		QAngle angViewAngles;
		g_pEngine.Xor( )->GetViewAngles( angViewAngles );

		const Vector2D vecScreenCenter = Vector2D( flWidth * .5f, flHeight * .5f );

		Vector vecFromOrigin = pLocal->GetAbsOrigin( );
		if( pLocal->IsDead( ) ) {
			if( pLocal->m_hObserverTarget( ).IsValid( ) && pLocal->m_hObserverTarget( ).Get( ) && pLocal->m_iObserverMode( ) != /*OBS_MODE_ROAMING*/6 ) {
				vecFromOrigin = reinterpret_cast< C_CSPlayer * >( pLocal->m_hObserverTarget( ).Get( ) )->GetAbsOrigin( );
			}
		}

		const float flAngle = ( angViewAngles.y - Math::CalcAngle( vecFromOrigin, entity->GetAbsOrigin( ), true ).y - 90 );
		const float flAngleYaw = DEG2RAD( flAngle );

		// note - michal;
		// when the day comes, i'll eventuall make this dynamic so that we can 
		// choose the distance and the size in pixels of the arrow, but this looks nice for now
		const float flNewPointX = ( vecScreenCenter.x + ( ( ( flWidth - 60.f ) / 2 ) * ( visuals_config->view_arrows_distance / 100.0f ) ) * cos( flAngleYaw ) ) + 8.f;
		const float flNewPointY = ( vecScreenCenter.y + ( ( ( flHeight - 60.f ) / 2 ) * ( visuals_config->view_arrows_distance / 100.0f ) ) * sin( flAngleYaw ) ) + 8.f;

		std::array< Vector2D, 3 >vecPoints{
			Vector2D( flNewPointX - visuals_config->view_arrows_size, flNewPointY - visuals_config->view_arrows_size ),
			Vector2D( flNewPointX + visuals_config->view_arrows_size, flNewPointY ),
			Vector2D( flNewPointX - visuals_config->view_arrows_size, flNewPointY + visuals_config->view_arrows_size ) };

		RotateArrow( vecPoints, flAngle );

		std::array< Vertex_t, 3 > uVertices{
			Vertex_t( vecPoints.at( 0 ) ),
			Vertex_t( vecPoints.at( 1 ) ),
			Vertex_t( vecPoints.at( 2 ) ) };

		static int nTextureID;
		if( !g_pSurface->IsTextureIDValid( nTextureID ) )
			nTextureID = g_pSurface.Xor( )->CreateNewTextureID( true );

		Color clr = visuals_config->view_arrows_color.ToRegularColor( ).OverrideAlpha( ( entity->IsDormant( ) ? 100 : 255 ) * player_fading_alpha.at( entity->EntIndex( ) ), true );

		// fill
		g_pSurface.Xor( )->DrawSetColor( clr.r( ), clr.g( ), clr.b( ), clr.a( ) * ( alpha[ entity->EntIndex( ) ] / 255.f ) );
		g_pSurface.Xor( )->DrawSetTexture( nTextureID );
		g_pSurface.Xor( )->DrawTexturedPolygon( 3, uVertices.data( ) );
	}
}

void Visuals::Spectators( std::vector< std::string > spectators ) {
	C_CSPlayer *pLocal = C_CSPlayer::GetLocalPlayer( );
	if( spectators.empty( ) )
		return;

	if( pLocal->m_iObserverMode( ) == /*OBS_MODE_ROAMING*/6 )
		return;

	int nOffset = 6;

	if( g_Vars.menu.watermark )
		nOffset += Render::Engine::watermark.size( "A" ).m_height + 2;

	for( size_t i{ }; i < spectators.size( ); ++i ) {
		auto msg = spectators[ i ];
		auto width = Render::Engine::watermark.size( msg ).m_width;
		auto height = Render::Engine::watermark.size( msg ).m_height + 6;

		Render::Engine::watermark.string( Render::GetScreenSize( ).x - 8 - width, nOffset + ( height * i ), Color( 255, 255, 255, 220 ), msg );
	}
}

void Visuals::RenderSkeleton( C_CSPlayer *entity ) {
	if( !entity ) {
		return;
	}

	auto model = entity->GetModel( );
	if( !model ) {
		return;
	}

	auto *hdr = g_pModelInfo->GetStudiomodel( model );
	if( !hdr ) {
		return;
	}

	if( entity->IsDead( ) ) {
		return;
	}

	if( entity->IsDormant( ) ) {
		return;
	}

	if( g_Visuals.player_fading_alpha[ entity->EntIndex( ) ] <= 0.05f || g_ExtendedVisuals.m_arrSoundPlayers.at( entity->EntIndex( ) ).m_bValidSound ) {
		return;
	}

	auto data = g_Animations.GetAnimationEntry( entity->EntIndex( ) );
	if( !data ) {
		return;
	}

	Vector2D bone1, bone2;
	matrix3x4_t uRenderMatrix[ 128 ];
	bool bFound = false;
	// since we now have a seperate option for history skeleton, this is not needed
	//for( auto &rec : data->m_deqRecords ) {
	//	if( !rec.IsRecordValid( ) || rec.m_bInvalid )
	//		continue;

	//	// found ideal record (render)
	//	if( rec.m_nIdeality == 2 ) {
	//		std::memcpy( uRenderMatrix, rec.m_pMatrix, sizeof( rec.m_pMatrix ) );
	//		bFound = true;
	//		break;
	//	}
	//}

	for( int i = 0; i < hdr->numbones; ++i ) {
		auto pBone = hdr->pBone( i );
		if( !pBone )
			continue;

		if( ( pBone->flags & BONE_USED_BY_HITBOX ) == 0 || pBone->parent < 0 )
			continue;

		auto GetBonePos = [&] ( int n ) -> Vector {
			return Vector(
				( bFound ? uRenderMatrix : entity->m_CachedBoneData( ).m_Memory.m_pMemory )[ n ][ 0 ][ 3 ],
				( bFound ? uRenderMatrix : entity->m_CachedBoneData( ).m_Memory.m_pMemory )[ n ][ 1 ][ 3 ],
				( bFound ? uRenderMatrix : entity->m_CachedBoneData( ).m_Memory.m_pMemory )[ n ][ 2 ][ 3 ]
			);
		};

		if( !Render::Engine::WorldToScreen( GetBonePos( i ), bone1 ) || !Render::Engine::WorldToScreen( GetBonePos( pBone->parent ), bone2 ) ) {
			continue;
		}

		Render::Engine::Line( { bone1.x, bone1.y }, { bone2.x, bone2.y }, DetermineVisualsColor( g_Vars.visuals_enemy.skeleton_color.ToRegularColor( ).OverrideAlpha( 210, true ), entity ) );
	}
}

void Visuals::RenderHistorySkeleton( C_CSPlayer *entity ) {
	if( !entity ) {
		return;
	}

	auto model = entity->GetModel( );
	if( !model ) {
		return;
	}

	auto *hdr = g_pModelInfo->GetStudiomodel( model );
	if( !hdr ) {
		return;
	}

	if( entity->IsDead( ) ) {
		return;
	}

	if( entity->IsDormant( ) ) {
		return;
	}

	if( g_Visuals.player_fading_alpha[ entity->EntIndex( ) ] <= 0.05f || g_ExtendedVisuals.m_arrSoundPlayers.at( entity->EntIndex( ) ).m_bValidSound ) {
		return;
	}

	auto data = g_Animations.GetAnimationEntry( entity->EntIndex( ) );
	if( !data ) {
		return;
	}

	matrix3x4_t pLerpedMatrix[ 128 ];
	g_Animations.GetVisualMatrix( entity, pLerpedMatrix, false );

	if( g_Vars.visuals_enemy.history_skeleton_all ) {
		Vector2D bone1, bone2;
		const float flAlpha = g_Vars.visuals_enemy.history_skeleton_color.a;
		for( auto &rec : data->m_deqRecords ) {
			if( !rec.IsRecordValid( ) || rec.m_bInvalid || ( rec.m_vecOrigin - entity->m_vecOrigin( ) ).Length( ) < 2.5f )
				continue;

			for( int i = 0; i < hdr->numbones; ++i ) {
				auto pBone = hdr->pBone( i );
				if( !pBone )
					continue;

				// get fade alpha
				float flRecordAlpha = ( float )i / ( float )data->m_deqRecords.size( );
				if( !flRecordAlpha )
					continue;

				// use updated alpha
				if( g_Vars.visuals_enemy.history_skeleton_all_manage_alpha )
					g_Vars.visuals_enemy.history_skeleton_color.a = flRecordAlpha;

				if( ( pBone->flags & BONE_USED_BY_HITBOX ) == 0 || pBone->parent < 0 )
					continue;

				auto GetBonePos = [&] ( int n ) -> Vector {
					return Vector(
						( rec.m_sAnims[ ESides::SIDE_SERVER ].m_pMatrix )[ n ][ 0 ][ 3 ],
						( rec.m_sAnims[ ESides::SIDE_SERVER ].m_pMatrix )[ n ][ 1 ][ 3 ],
						( rec.m_sAnims[ ESides::SIDE_SERVER ].m_pMatrix )[ n ][ 2 ][ 3 ]
					);
				};

				if( !Render::Engine::WorldToScreen( GetBonePos( i ), bone1 ) || !Render::Engine::WorldToScreen( GetBonePos( pBone->parent ), bone2 ) ) {
					continue;
				}

				Render::Engine::Line( { bone1.x, bone1.y }, { bone2.x, bone2.y }, DetermineVisualsColor( g_Vars.visuals_enemy.history_skeleton_color.ToRegularColor( ).OverrideAlpha( 210, true ), entity ) );

				// restore back to original alpha modulation
				if( g_Vars.visuals_enemy.history_skeleton_all_manage_alpha )
					g_Vars.visuals_enemy.history_skeleton_color.a = flAlpha;
			}
		}
	}
	// repeated code on if statement w/e works
	else {
		Vector2D bone1_l, bone2_l;
		for( int i = 0; i < hdr->numbones; ++i ) {
			auto pBone = hdr->pBone( i );
			if( !pBone )
				continue;

			if( ( pBone->flags & BONE_USED_BY_HITBOX ) == 0 || pBone->parent < 0 )
				continue;

			auto GetBonePos = [&] ( int n ) -> Vector {
				return Vector(
					( pLerpedMatrix )[ n ][ 0 ][ 3 ],
					( pLerpedMatrix )[ n ][ 1 ][ 3 ],
					( pLerpedMatrix )[ n ][ 2 ][ 3 ]
				);
			};

			if( !Render::Engine::WorldToScreen( GetBonePos( i ), bone1_l ) || !Render::Engine::WorldToScreen( GetBonePos( pBone->parent ), bone2_l ) ) {
				continue;
			}

			Render::Engine::Line( { bone1_l.x, bone1_l.y }, { bone2_l.x, bone2_l.y }, DetermineVisualsColor( g_Vars.visuals_enemy.history_skeleton_color.ToRegularColor( ).OverrideAlpha( 210, true ), entity ) );
		}
	}
}

void Visuals::HandlePlayerVisuals( C_CSPlayer *entity ) {
	// do view arrows before any other visuals
	// this is so the bounding box check doesnt interfere with em
	if( visuals_config->view_arrows ) {
		RenderArrows( entity );
	}

	// force engine radar.
	if( g_Vars.esp.ingame_radar )
		entity->m_bSpotted( ) = true;

	Box_t box;
	const bool bPassedSetup = g_Vars.globals.m_bUseCollisionBoundingBoxes ? SetupBoundingBoxCollision( entity, box ) : SetupBoundingBox( entity, box );

	if( !bPassedSetup )
		return;

	if( visuals_config->box ) {
		RenderBox( box, entity );
	}

	if( visuals_config->health ) {
		RenderHealth( box, entity );
	}

	if( visuals_config->skeleton ) {
		RenderSkeleton( entity );
	}

	if( visuals_config->history_skeleton ) {
		RenderHistorySkeleton( entity );
	}

	RenderTopInfo( box, entity );
	RenderBottomInfo( box, entity );
	RenderSideInfo( box, entity );
}

void Visuals::HandleWorldVisuals( C_BaseEntity *entity ) {
	auto client_class = entity->GetClientClass( );
	if( !client_class )
		return;

	if( g_Vars.esp.dropped_weapons || g_Vars.esp.dropped_weapons_ammo ) {
		if( entity->IsWeapon( ) ) {
			RenderDroppedWeapons( entity );
		}
	}

	switch( client_class->m_ClassID ) {
		case CBaseCSGrenadeProjectile:
		case CMolotovProjectile:
		case CSmokeGrenadeProjectile:
		case CDecoyProjectile:
		case CInferno:
			if( g_Vars.esp.grenades || g_Vars.esp.grenades_radius ) {
				RenderNades( entity );
			}
			break;
		case CC4:
		case CPlantedC4:
			RenderObjectives( entity );
			break;
	}

	// already found an entity we are in range of?
	if( !g_Vars.globals.m_bInsideFireRange ) {
		// ok this maths is a huge meme and can prolly be cleaned up LMFAO
		if( client_class->m_ClassID == CInferno ) {
			const auto pLocal = C_CSPlayer::GetLocalPlayer( );
			if( pLocal ) {
				Vector mins, maxs, nmins, nmaxs;
				if( entity->GetClientRenderable( ) )
					entity->GetClientRenderable( )->GetRenderBounds( mins, maxs );

				auto vecLocalAbs = pLocal->GetAbsOrigin( ).ToVector2D( );
				const auto &vecAbsOrigin = entity->GetAbsOrigin( );

				C_CSPlayer *pOwner = ( C_CSPlayer * )entity->m_hOwnerEntity( ).Get( );
				bool bIsLethal = true;
				if( pOwner ) {
					if( pOwner->m_iTeamNum( ) == pLocal->m_iTeamNum( ) && pOwner->EntIndex( ) != g_pEngine->GetLocalPlayer( ) ) {
						if( g_Vars.mp_friendlyfire->GetInt( ) != 1 )
							bIsLethal = false;
					}
				}

				nmins = mins;
				nmaxs = maxs;
				nmins.x *= -1.f;
				nmaxs.x *= -1.f;

				maxs += vecAbsOrigin;
				mins += vecAbsOrigin;
				nmins += vecAbsOrigin;
				nmaxs += vecAbsOrigin;

				bool inBounds = bIsLethal && ( vecLocalAbs >= mins.ToVector2D( ) && vecLocalAbs <= maxs.ToVector2D( ) ) || ( vecLocalAbs >= nmins.ToVector2D( ) && vecLocalAbs <= nmaxs.ToVector2D( ) );

				g_Vars.globals.m_bInsideFireRange = inBounds;
			}
		}
	}
}

Color Visuals::DetermineVisualsColor( Color regular, C_CSPlayer *entity ) {
	/*if( g_ExtendedVisuals.m_arrSoundPlayers.at( entity->EntIndex( ) ).m_flLastNonDormantTime > 0.f &&
		fabsf( g_ExtendedVisuals.m_arrSoundPlayers.at( entity->EntIndex( ) ).m_flLastNonDormantTime - g_pGlobalVars->realtime ) > 1.f )
	{*/
	if( entity->IsDormant( ) && g_Vars.visuals_enemy.dormant ) {
		return Color( 210, 210, 210, regular.a( ) * player_fading_alpha.at( entity->EntIndex( ) ) );
	}
	//}

	Color cRetColor{ regular };
	cRetColor.RGBA[ 3 ] *= player_fading_alpha.at( entity->EntIndex( ) );

	return cRetColor;
}

bool IsAimingAtPlayerThroughPenetrableWall( C_CSPlayer *local, C_WeaponCSBaseGun *pWeapon ) {
	auto weaponInfo = pWeapon->GetCSWeaponData( );
	if( !weaponInfo.IsValid( ) )
		return -1.0f;

	QAngle view_angles;
	g_pEngine->GetViewAngles( view_angles );

	Autowall::FireBulletData data;

	data.m_Player = local;
	data.m_TargetPlayer = nullptr;
	data.m_bPenetration = true;
	data.m_vecStart = local->GetEyePosition( );
	data.m_vecDirection = view_angles.ToVectors( );
	data.m_flMaxLength = data.m_vecDirection.Normalize( );
	data.m_WeaponData = weaponInfo.Xor( );
	data.m_flCurrentDamage = static_cast< float >( weaponInfo->m_iWeaponDamage );

	return Autowall::FireBullets( &data ) >= 1.f;
}

bool yurr( C_CSPlayer *local, C_WeaponCSBaseGun *pWeapon ) {
	auto weaponInfo = pWeapon->GetCSWeaponData( );
	if( !weaponInfo.IsValid( ) )
		return false;

	QAngle view_angles;
	g_pEngine->GetViewAngles( view_angles );

	Autowall::FireBulletData data;

	data.m_iPenetrationCount = 4;
	data.m_Player = local;
	data.m_TargetPlayer = nullptr;
	data.m_vecStart = local->GetEyePosition( );
	data.m_vecDirection = view_angles.ToVectors( );
	data.m_flMaxLength = data.m_vecDirection.Normalize( );
	data.m_WeaponData = weaponInfo.Xor( );
	data.m_flTraceLength = 0.0f;
	data.m_flCurrentDamage = static_cast< float >( weaponInfo->m_iWeaponDamage );

	Vector end = data.m_vecStart + data.m_vecDirection * weaponInfo->m_flWeaponRange;

	CTraceFilter filter;
	filter.pSkip = local;

	Autowall::TraceLine( data.m_vecStart, end, MASK_SHOT_HULL | CONTENTS_HITBOX, &filter, &data.m_EnterTrace );

	data.m_flTraceLength += data.m_flMaxLength * data.m_EnterTrace.fraction;
	data.m_flCurrentDamage *= powf( weaponInfo->m_flRangeModifier, data.m_flTraceLength * 0.002f );
	data.m_EnterSurfaceData = g_pPhysicsSurfaceProps->GetSurfaceData( data.m_EnterTrace.surface.surfaceProps );

	return !Autowall::HandleBulletPenetration( &data );
};

void Visuals::PenetrationCrosshair( ) {
	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	if( !g_Vars.esp.autowall_crosshair ) {
		return;
	}

	if( pLocal->IsDead( ) ) {
		return;
	}

	C_WeaponCSBaseGun *pWeapon = ( C_WeaponCSBaseGun * )pLocal->m_hActiveWeapon( ).Get( );
	if( !pWeapon )
		return;

	if( !pWeapon->GetCSWeaponData( ).IsValid( ) )
		return;

	auto type = pWeapon->GetCSWeaponData( ).Xor( )->m_iWeaponType;

	if( type == WEAPONTYPE_KNIFE || type == WEAPONTYPE_C4 || type == WEAPONTYPE_GRENADE )
		return;

	static float flDamage = 0.f;
	static bool bAim = false;

	if( g_pGlobalVars->tickcount % 10 ) {
		flDamage = yurr( pLocal, pWeapon );
		bAim = IsAimingAtPlayerThroughPenetrableWall( pLocal, pWeapon );
	}

	auto screen = Render::GetScreenSize( ) / 2;
	bool aim = bAim && flDamage;
	Color color = aim ? ( Color( 255, 255, 0, 255 ) ) : ( flDamage ? Color( 0, 255, 0, 255 ) : Color( 255, 0, 0, 255 ) );

	Render::Engine::RectFilled( screen.x - 1, screen.y - 1, 3, 3, color );
}

void Visuals::RenderManual( ) {
	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	if( !g_pEngine->IsInGame( ) || !g_pEngine->IsConnected( ) || pLocal->IsDead( ) )
		return;

	//if( g_AntiAim.GetSide( ) == SIDE_MAX )
	//	return;

	if( !g_Vars.rage.anti_aim_manual || !g_Vars.rage.anti_aim_manual_arrows )
		return;

	auto vecScreenCenter = Render::GetScreenSize( ) / 2;

	// modify these
	const int nPaddingFromCenter{ g_Vars.rage.anti_aim_manual_arrows_spacing }; // { 60 }
	const int nSizeInPixels{ g_Vars.rage.anti_aim_manual_arrows_size }; // { 22 };

	// do not fuckign touch these
	const int nSize2{ nSizeInPixels / 2 };

	std::array<Vector2D, 9> vecArrows = {
		Vector2D( vecScreenCenter.x - nPaddingFromCenter, vecScreenCenter.y - nSize2 ), // left arrow top
		Vector2D( vecScreenCenter.x - nPaddingFromCenter, vecScreenCenter.y + nSize2 ), // left arrow bottom
		Vector2D( vecScreenCenter.x - ( nPaddingFromCenter + nSizeInPixels ), vecScreenCenter.y ), // left arrow middle

		Vector2D( vecScreenCenter.x + nPaddingFromCenter, vecScreenCenter.y - nSize2 ), // right arrow top
		Vector2D( vecScreenCenter.x + nPaddingFromCenter, vecScreenCenter.y + nSize2 ), // right arrow bottom
		Vector2D( vecScreenCenter.x + ( nPaddingFromCenter + nSizeInPixels ), vecScreenCenter.y ), // right arrow middle

		Vector2D( vecScreenCenter.x - nSize2, vecScreenCenter.y + nPaddingFromCenter ), // bottom arrow left
		Vector2D( vecScreenCenter.x + nSize2, vecScreenCenter.y + nPaddingFromCenter ), // bottom arrow right
		Vector2D( vecScreenCenter.x, vecScreenCenter.y + ( nPaddingFromCenter + nSizeInPixels ) ), // bottom arrow middle
	};

	const auto eSide = g_AntiAim.GetSide( );
	Render::Engine::FilledTriangle( vecArrows.at( 0 ), vecArrows.at( 1 ), vecArrows.at( 2 ), eSide == SIDE_LEFT ? g_Vars.rage.anti_aim_manual_arrows_color.ToRegularColor( ) : Color::Black( ).OverrideAlpha( 150 ) );
	Render::Engine::FilledTriangle( vecArrows.at( 3 ), vecArrows.at( 4 ), vecArrows.at( 5 ), eSide == SIDE_RIGHT ? g_Vars.rage.anti_aim_manual_arrows_color.ToRegularColor( ) : Color::Black( ).OverrideAlpha( 150 ) );
	Render::Engine::FilledTriangle( vecArrows.at( 6 ), vecArrows.at( 7 ), vecArrows.at( 8 ), eSide == SIDE_BACK ? g_Vars.rage.anti_aim_manual_arrows_color.ToRegularColor( ) : Color::Black( ).OverrideAlpha( 150 ) );
}

void Visuals::AutopeekIndicator( ) {
	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	if( !g_pEngine->IsInGame( ) || !g_pEngine->IsConnected( ) || pLocal->IsDead( ) )
		return;

	if( g_Movement.m_vecAutoPeekPos.IsZero( ) )
		return;

	Render::Engine::WorldCircle( g_Movement.m_vecAutoPeekPos, 15.f, Color( 0, 0, 0, 0 ), g_Vars.misc.autopeek_color.ToRegularColor( ) );
}

struct soundsss_t {
	int user_data;
	int sound_source;
	int ent_channel;
	c_sfx_table *sfx;
	Vector origin;
};

void Visuals::Draw( ) {
	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	if( !g_pEngine->IsInGame( ) || !g_pEngine->IsConnected( ) )
		return;

#if 0
	if( !g_Vars.globals.m_vecSounds.empty( ) ) {
		for( size_t i{ 0 }; i < g_Vars.globals.m_vecSounds.size( ); i++ ) {
			auto &sound = g_Vars.globals.m_vecSounds.at( i );

			if( sound.m_ack ) {
				g_Vars.globals.m_vecSounds.erase( g_Vars.globals.m_vecSounds.begin( ) + i );
				continue;
			}

			auto entity = ( C_CSPlayer * )g_pEntityList->GetClientEntity( sound.m_sound_idx );
			if( !entity ) {
				g_Vars.globals.m_vecSounds.erase( g_Vars.globals.m_vecSounds.begin( ) + i );
				continue;
			}

			auto player = ( C_CSPlayer * )entity;
			if( player && !player->IsPlayer( ) ) {
				if( entity->IsWeapon( ) )
					player = ( C_CSPlayer * )( g_pEntityList->GetClientEntityFromHandle( entity->m_hOwnerEntity( ) ) );
			}

			if( player && player->IsPlayer( ) && !player->IsTeammate( pLocal ) ) {
				if( g_ExtendedVisuals.m_arrOverridePlayers.at( player->EntIndex( ) ).m_eOverrideType <= EOverrideType::ESP_SHARED ) {
					g_ExtendedVisuals.m_arrOverridePlayers.at( player->EntIndex( ) ).m_eOverrideType = EOverrideType::ESP_SHARED;
					g_ExtendedVisuals.m_arrOverridePlayers.at( player->EntIndex( ) ).m_flReceiveTime = sound.m_sound_time;
					g_ExtendedVisuals.m_arrOverridePlayers.at( player->EntIndex( ) ).m_vecLastOrigin = g_ExtendedVisuals.m_arrOverridePlayers.at( player->EntIndex( ) ).m_vecOrigin;
					g_ExtendedVisuals.m_arrOverridePlayers.at( player->EntIndex( ) ).m_vecOrigin = sound.m_sound_origin;
				}
			}

			sound.m_ack = true;
}
	}
#endif

	visuals_config = &g_Vars.visuals_enemy;

	if( !visuals_config )
		return;

	if( g_Vars.esp.remove_scope ) {
		const auto pWeapon = reinterpret_cast< C_WeaponCSBaseGun * >( pLocal->m_hActiveWeapon( ).Get( ) );
		if( pWeapon ) {
			auto pWeaponData = pWeapon->GetCSWeaponData( );
			if( pWeaponData.IsValid( ) ) {
				if( pLocal->m_bIsScoped( ) && pWeaponData->m_iWeaponType == WEAPONTYPE_SNIPER_RIFLE ) {

					const auto screen = Render::GetScreenSize( );

					int w = screen.x,
						h = screen.y,
						x = w / 2,
						y = h / 2,
						size = g_Vars.cl_crosshair_sniper_width->GetInt( );

					if( size > 1 ) {
						x -= ( size / 2 );
						y -= ( size / 2 );
					}

					Render::Engine::RectFilled( 0, y, w, size, Color::Black( ) );
					Render::Engine::RectFilled( x, 0, size, h, Color::Black( ) );
				}
			}
		}
	}

	RenderManual( );
	DrawWatermark( );
	PenetrationCrosshair( );
	AutopeekIndicator( );

	g_ExtendedVisuals.Adjust( );

	g_Vars.globals.m_bInsideFireRange = false;

	std::vector< std::string > spectators{ };
	const auto local_observer = pLocal->m_hObserverTarget( );
	C_CSPlayer *spec = nullptr;
	if( local_observer.IsValid( ) ) {
		spec = ( C_CSPlayer * )g_pEntityList->GetClientEntityFromHandle( local_observer );
	}

	if( g_Vars.rage.visualize_aimpoints )
		for( int i = 1; i <= 64; ++i ) {
			if( g_ExtendedVisuals.m_arrOverridePlayers.at( i ).m_bSus ) {
				g_ExtendedVisuals.m_arrOverridePlayers.at( i ).m_flReceiveTime = g_pGlobalVars->realtime;
				g_ExtendedVisuals.m_arrOverridePlayers.at( i ).m_bSus = false;
			}

			if( pLocal->IsDead( ) ) {
				vecAimpoints[ i ].clear( );
				vecAimpointsSane[ i ].clear( );
				break;
			}

			if( !vecAimpointsSane[ i ].empty( ) ) {
				for( auto &p : vecAimpointsSane[ i ] ) {
					Vector2D pos;
					if( Render::Engine::WorldToScreen( p, pos ) ) {
						//Render::Engine::Rect( pos - Vector2D( 0, 1 ), Vector2D( 1, 3 ), Color::White( ) );
						//Render::Engine::Rect( pos - Vector2D( 1, 0 ), Vector2D( 3, 1 ), Color::White( ) );

						Render::Engine::Rect( Vector2D( pos.x, pos.y - 1 ), Vector2D( 1, 3 ), g_Vars.rage.visualize_aimpoints_clr.ToRegularColor( ) );
						Render::Engine::Rect( Vector2D( pos.x - 1, pos.y ), Vector2D( 3, 1 ), g_Vars.rage.visualize_aimpoints_clr.ToRegularColor( ) );
					}
				}
			}
		}

	int x = 0;
	// main entity loop
	for( int i = 1; i <= g_pEntityList->GetHighestEntityIndex( ); ++i ) {
		auto entity = reinterpret_cast< C_BaseEntity * >( g_pEntityList->GetClientEntity( i ) );
		if( !entity ) {

			// reset if entity got invalid
			if( i <= 64 ) {
				g_ExtendedVisuals.m_arrWeaponInfos[ i ].first = 0;
				g_ExtendedVisuals.m_arrWeaponInfos[ i ].second = nullptr;
			}

			continue;
		}

		// let's not even bother if we are out of range
		if( i <= 64 ) {
			// convert entity to csplayer
			const auto player = ToCSPlayer( entity );

			if( player->IsTeammate( pLocal ) ) {
				bool bAllowPacketSendForThisEntity = !( g_pGlobalVars->tickcount % 4 );

				// hahahha this is so fucked
				static Vector vecPreviousOrigin[ 65 ];
				if( vecPreviousOrigin[ i ] != player->GetAbsOrigin( ) ) {
					bAllowPacketSendForThisEntity = true;
					vecPreviousOrigin[ i ] = player->GetAbsOrigin( );
				}
			}

			// so that it doesn't require a 2nd fucking loop..
			if( player && g_Vars.esp.spectators > 0 ) {
				player_info_t info;
				if( g_pEngine->GetPlayerInfo( i, &info ) ) {
					if( !player->IsDormant( ) && player->IsDead( ) && player->EntIndex( ) != pLocal->EntIndex( ) ) {
						std::string playerName{ info.szName };

						if( playerName.find( XorStr( "\n" ) ) != std::string::npos ) {
							playerName = XorStr( "[BLANK]" );
						}

						if( g_Vars.esp.spectators == 1 ) {
							auto observer = player->m_hObserverTarget( );
							if( observer.IsValid( ) ) {
								auto target = reinterpret_cast< C_CSPlayer * >( g_pEntityList->GetClientEntityFromHandle( observer ) );
								if( target ) {
									player_info_t info2;
									if( g_pEngine->GetPlayerInfo( target->EntIndex( ), &info2 ) ) {
										std::string playerName2{ info2.szName };

										if( playerName2.find( XorStr( "\n" ) ) != std::string::npos ) {
											playerName2 = XorStr( "[BLANK]" );
										}

										/*if( playerName2.length( ) > 10 ) {
											playerName2.resize( 10 );
											playerName2.append( "..." );
										}*/

										char buf1[ 255 ];
										char buf2[ 255 ];
										char buf3[ 255 ];
										sprintf_s( buf1, "%s", playerName.data( ) );
										sprintf_s( buf2, " -> " );
										sprintf_s( buf3, "%s", playerName2.data( ) );

										auto size1 = Render::Engine::speclist.size( buf1 );
										auto size2 = Render::Engine::speclist.size( buf2 );
										auto size3 = Render::Engine::speclist.size( buf3 );

										Color clrPlayer = player->m_iTeamNum( ) == TEAM_CT ? Color( 114, 155, 221, 220 ) : Color( 224, 175, 86, 220 );
										Color clrTarget = target->EntIndex( ) == pLocal->EntIndex( ) ? Color( 0, 255, 0, 220 ) : ( target->m_iTeamNum( ) == TEAM_CT ? Color( 114, 155, 221, 220 ) : Color( 224, 175, 86, 220 ) );

										Render::Engine::speclist.string( 20, 295 + ( 16 * x ), clrPlayer, buf1 );
										Render::Engine::speclist.string( 20 + size1.m_width, 295 + ( 16 * x ), Color( 224, 175, 86, 220 ), buf2 );
										Render::Engine::speclist.string( 20 + size1.m_width + size2.m_width, 295 + ( 16 * x ), clrTarget, buf3 );

										x++;
									}

								}

							}

						}
						else {
							if( playerName.length( ) > 10 ) {
								playerName.resize( 10 );
								playerName.append( "..." );
							}

							if( pLocal->IsDead( ) ) {
								auto observer = player->m_hObserverTarget( );
								if( local_observer.IsValid( ) && observer.IsValid( ) ) {
									auto target = reinterpret_cast< C_CSPlayer * >( g_pEntityList->GetClientEntityFromHandle( observer ) );

									if( target == spec && spec ) {
										spectators.push_back( playerName.substr( 0, 24 ) );
									}
								}
							}
							else {
								if( player->m_hObserverTarget( ) == pLocal ) {
									spectators.push_back( playerName.substr( 0, 24 ) );
								}
							}
						}
					}
				}
			}

			// handle player visuals
			if( IsValidPlayer( player ) ) {
				std::array<std::pair<Vector, bool>, 65> m_arrManualRestore;

				bool bShowPlayerManualOverride = false;

				if( player->IsDormant( ) ) {
					vecAimpoints[ i ].clear( );
					vecAimpointsSane[ i ].clear( );

					if( g_ExtendedVisuals.m_arrOverridePlayers.at( i ).m_eOverrideType != EOverrideType::ESP_NONE ) {
						m_arrManualRestore.at( i ) = std::make_pair( player->m_vecOrigin( ), true );

						const float flReceiveTimeDelta = fabs( g_ExtendedVisuals.m_arrOverridePlayers.at( i ).m_flReceiveTime - g_pGlobalVars->realtime );

						if( g_ExtendedVisuals.m_arrOverridePlayers.at( i ).m_flReceiveTime != FLT_MAX )
							bShowPlayerManualOverride = flReceiveTimeDelta < 10.f;

						player->m_vecOrigin( ) = g_ExtendedVisuals.m_arrOverridePlayers.at( i ).m_vecOrigin;
						player->SetAbsOrigin( g_ExtendedVisuals.m_arrOverridePlayers.at( i ).m_vecOrigin );
					}
				}
				else {
					g_ExtendedVisuals.m_arrSoundPlayers.at( i ).m_iReceiveTime = 0;
					m_arrManualRestore.at( i ) = std::make_pair( Vector( ), false );
					g_ExtendedVisuals.m_arrOverridePlayers.at( i ).Reset( );
				}

				if( visuals_config->dormant ) {
					// not dormant?
					if( !player->IsDormant( ) || g_ExtendedVisuals.m_arrSoundPlayers.at( i ).m_bValidSound || ( player->IsDormant( ) && bShowPlayerManualOverride ) ) {
						if( g_ExtendedVisuals.m_arrSoundPlayers.at( i ).m_bValidSound || ( player->IsDormant( ) && bShowPlayerManualOverride ) ) {
							// set full alpha if this was a sound based update
							player_fading_alpha.at( i ) = 1.f;
						}
						else {
							// increase alpha.
							player_fading_alpha.at( i ) += ( 5.f ) * g_pGlobalVars->frametime;
						}
					}
					else {
						if( player_fading_alpha.at( i ) < 0.6f ) {
							// decrease alpha FAST.
							player_fading_alpha.at( i ) -= ( 1.f ) * g_pGlobalVars->frametime;
						}
						else {
							// decrease alpha.
							player_fading_alpha.at( i ) -= ( 0.05f ) * g_pGlobalVars->frametime;
						}
					}
				}
				else {
					if( !player->IsDormant( ) ) {
						// increase alpha.
						player_fading_alpha.at( i ) += ( 5.f ) * g_pGlobalVars->frametime;
					}
					else {
						// decrease alpha FAST.
						player_fading_alpha.at( i ) -= ( 1.f ) * g_pGlobalVars->frametime;
					}
				}

				// now clamp it
				player_fading_alpha.at( i ) = std::clamp( player_fading_alpha.at( i ), 0.f, 1.0f );

				HandlePlayerVisuals( player );

				if( m_arrManualRestore.at( i ).second ) {
					player->m_vecOrigin( ) = m_arrManualRestore.at( i ).first;
					player->SetAbsOrigin( m_arrManualRestore.at( i ).first );
				}
			}
		}

		// handle world visuals
		if( IsValidEntity( entity ) ) {
			HandleWorldVisuals( entity );
		}

		g_GrenadeWarning.Run( entity );
	}

	if( g_Vars.esp.spectators == 1 )
		Render::Engine::speclist.string( 5, 275, Color( 255, 255, 255, 220 ), XorStr( "spectators" ) );
	else
		Spectators( spectators );

	g_ExtendedVisuals.Restore( );

	SpreadCrosshair( );
}

void Visuals::DrawWatermark( ) {
	if( g_Vars.menu.watermark ) {
		char watermarkBuf[ 512 ] = {};

		std::string date = __DATE__;
		std::transform( date.begin( ), date.end( ), date.begin( ), ::tolower );

		if( g_pEngine->IsConnected( ) && g_pEngine->IsInGame( ) )
			sprintf( watermarkBuf, XorStr( "blootoothYzio | lt %.2f + %.2f | %s" ), g_Animations.m_flOutgoingLatency, g_Animations.m_flIncomingLatency, date.data( ) );
		else
			sprintf( watermarkBuf, XorStr( "blootoothYzio | not connected | %s" ), date.data( ) );

		auto watermarkSize = Render::Engine::watermark.size( watermarkBuf );
		Render::Engine::watermark.string( Render::Engine::m_width - watermarkSize.m_width - 5, 5, Color( 245, 245, 245, 235 ), watermarkBuf );
	}

	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	if( pLocal->IsDead( ) ) {
		return;
	}

	C_WeaponCSBaseGun *pWeapon = ( C_WeaponCSBaseGun * )pLocal->m_hActiveWeapon( ).Get( );
	if( !pWeapon )
		return;

	if( !pWeapon->GetCSWeaponData( ).IsValid( ) )
		return;

	struct Indicator_t { Color color; std::string text; };
	std::vector< Indicator_t > indicators{ };

	static bool bRenderIndicator = false;
	if( ( g_Vars.misc.extended_backtrack || ( g_Vars.misc.ping_spike && g_Vars.misc.ping_spike_key.enabled ) ) && g_Vars.esp.indicator_pingspike ) {
		bRenderIndicator = true;
	}

	static bool bRenderIndicatorChair = false;
	if( g_Vars.misc.ping_spike && g_Vars.misc.ping_spike_key.enabled && g_Vars.esp.crosshair_indicator_ping ) {
		bRenderIndicatorChair = true;
	}

	if( g_Vars.rage.anti_aim_fps && g_Ragebot.m_flLimitTargets >= g_pGlobalVars->realtime ) {
		if( g_Ragebot.m_flFrameRateMultiplier > 0.2f ) {
			Indicator_t ind{ };
			ind.color = Color::Blend( Color::Red( ), Color( 150, 200, 60 ), g_Ragebot.m_flFrameRateMultiplier );
			ind.text = XorStr( "FPS" );
			indicators.push_back( ind );
		}
	}

	if( bRenderIndicator || bRenderIndicatorChair ) {
		auto netchannel = Encrypted_t<INetChannel>( g_pEngine->GetNetChannelInfo( ) );
		if( netchannel.IsValid( ) ) {
			const float flFakePing = netchannel->GetLatency( FLOW_INCOMING ) * 1000.f;
			const float flRealPing = netchannel->GetLatency( FLOW_OUTGOING ) * 1000.f;
			const float flTargetLatency = g_Vars.misc.extended_backtrack ? 200.f : 50.f;
			const bool bDisableIndicator = fabs( flFakePing - flRealPing ) <= 50.f && !( g_Vars.misc.extended_backtrack || ( g_Vars.misc.ping_spike && g_Vars.misc.ping_spike_key.enabled ) && g_Vars.esp.indicator_pingspike );
			const bool bDisableIndicatorChair = fabs( flFakePing - flRealPing ) <= flTargetLatency && !( ( g_Vars.misc.ping_spike && g_Vars.misc.ping_spike_key.enabled ) && g_Vars.esp.crosshair_indicator_ping );

			if( bRenderIndicator ) {
				int nTargetPing = g_Vars.misc.ping_spike && g_Vars.misc.ping_spike_key.enabled ? g_Vars.misc.ping_spike_amount : g_Vars.misc.extended_backtrack;
				float flCorrect = std::clamp<float>( ( ( nTargetPing / 1000.f ) - netchannel->GetLatency( FLOW_OUTGOING ) - g_Animations.m_fLerpTime ) * 1000.f, 0.f, 1000.f );
				const float flPingMultiplier = std::clamp<float>( flFakePing / flCorrect, 0.f, 1.f );

				Indicator_t ind{ };
				ind.color = Color::Blend( Color::Red( ), Color( 160, 255, 0 ), flPingMultiplier );
				ind.text = g_Vars.misc.ping_spike && g_Vars.misc.ping_spike_key.enabled ? XorStr( "SPIKE" ) : XorStr( "EXT" );
				indicators.push_back( ind );

				// we disabled ping spike etc, but our ping obviously wont instantly reach our old ping.

				// check for ping multiplier between our ping and wish ping, if it's low enough and we aren't pingspiking
				// anymore we can disable the indicator. until then, show the user the current ping "spike" amount.
				if( bDisableIndicator ) {
					bRenderIndicator = false;
				}
			}

			// just disable this one aswell if ur only using extended backtrack, no need to show ur ping since it's always on
			if( bRenderIndicatorChair && bDisableIndicatorChair ) {
				bRenderIndicatorChair = false;
			}
		}
	}

	// we disabled the menu element, don't want this at all.. just force disalbe
	if( bRenderIndicator && !g_Vars.esp.indicator_pingspike )
		bRenderIndicator = false;

	if( bRenderIndicatorChair && !g_Vars.esp.crosshair_indicator_ping )
		bRenderIndicatorChair = false;

	if( g_Vars.rage.anti_aim_active && g_Vars.esp.indicator_antiaim_side ) {
		Indicator_t ind{ };
		ind.color = /*g_AntiAim.m_bDistorting ? Color( 100, 100, 255 ) :*/ g_Vars.esp.indicator_antiaim_side_color.ToRegularColor( ).OverrideAlpha( 255 );

		bool bHasText = false;

		if( g_Vars.rage.anti_aim_lock_angle_key.enabled ) {
			ind.text = XorStr( "LOCK" );
			bHasText = true;
		}
		else {
			bHasText = true;
			if( g_Vars.rage.anti_aim_manual_right_key.enabled ) {
				ind.text = XorStr( "RIGHT" );
			}
			else if( g_Vars.rage.anti_aim_manual_left_key.enabled ) {
				ind.text = XorStr( "LEFT" );
			}
			else if( g_Vars.rage.anti_aim_manual_back_key.enabled ) {
				ind.text = XorStr( "BACK" );
			}
			else if( g_Vars.rage.anti_aim_manual_forward_key.enabled ) {
				ind.text = XorStr( "FORWARD" );
			}
			else {
				bHasText = false;
			}
		}

		// no manual override / lock is active, so report the automatic state.
		// note; "AUTO" used to be shown here unconditionally, which only meant
		// "no manual override" and said nothing about auto yaw actually running.
		if( !bHasText ) {
			// mirror the exact gates from AntiAim::Think so the indicator can
			// never disagree with the logic that picks the angle.
			const bool bOnGround = ( pLocal->m_fFlags( ) & FL_ONGROUND ) != 0;

			const bool bCanFreestand = bOnGround && g_Vars.rage.anti_aim_freestand
				&& ( g_Vars.rage.anti_aim_freestand_key.enabled || g_Vars.rage.anti_aim_freestand_key.key == 0 );

			const bool bCanEdge = bOnGround && g_Vars.rage.anti_aim_edge
				&& ( g_Vars.rage.anti_aim_edge_key.enabled || g_Vars.rage.anti_aim_edge_key.key == 0 );

			if( g_AntiAim.m_bEdgeYawApplied ) {
				// edge yaw beats auto yaw when a wall is detected.
				ind.color = Color( 160, 255, 0 );
				ind.text = XorStr( "EDGE" );
			}
			else if( bCanFreestand || bCanEdge ) {
				// enabled; green once an angle was actually applied this tick,
				// red while there is no solution and we fall back to base yaw.
				ind.color = g_AntiAim.m_bAutoYawApplied ? Color( 160, 255, 0 ) : Color( 255, 0, 0 );
				ind.text = XorStr( "AUTO" );
			}
			else {
				// auto yaw is off entirely; keep the old "no manual override"
				// signal, but don't overload the "AUTO" string for it.
				ind.text = XorStr( "STATIC" );
			}

			bHasText = true;
		}

		if( bHasText )
			indicators.push_back( ind );
	}

	if( g_Vars.rage.fake_lag && g_Vars.esp.indicator_lagcomp ) {
		Indicator_t ind{ };
		ind.color = g_ServerAnimations.m_uServerAnimations.m_bBreakingTeleportDst ? Color( 160, 255, 0 ) : Color( 255, 0, 0 );
		ind.text = XorStr( "LC" );

		if( g_ServerAnimations.m_uServerAnimations.m_bBreakingTeleportDst || pLocal->m_vecVelocity( ).Length2D( ) > 260.f )
			indicators.push_back( ind );
	}

	bool bPurple = false;
	const float change = std::abs( Math::AngleNormalize( g_ServerAnimations.m_uServerAnimations.m_flLowerBodyYawTarget - g_ServerAnimations.m_uServerAnimations.m_flEyeYaw ) );
	if( g_Vars.rage.anti_aim_fake_body && g_Vars.esp.indicator_antiaim_lby ) {
		if( ( g_AntiAim.m_bHidingLBYFlick && pLocal->m_fFlags( ) & FL_ONGROUND && g_Prediction.get_unpredicted_vars( )->flags & FL_ONGROUND ) )
			bPurple = true;

		if( g_AntiAim.m_bLandDesynced && change > 35.f )
			bPurple = true;

		Indicator_t ind{ };
		ind.color = bPurple ? Color( 100, 100, 255 ) : ( ( change > 35.f ) ? Color( 160, 255, 0 ) : Color( 255, 0, 0 ) );
		ind.text = XorStr( "LBY" );
		indicators.push_back( ind );
	}

	// temp
	/*auto GetRotatedPos( Vector start, const float rotation, const float distance ) -> Vector {
		const auto rad = DEG2RAD( rotation );
		start.x += cosf( rad ) * distance;
		start.y += sinf( rad ) * distance;

		return start;
	};

	Vector2D screen1, screen2;
	if( Render::Engine::WorldToScreen( pLocal->GetAbsOrigin( ), screen1 ) ) {
		if( Render::Engine::WorldToScreen( GetRotatedPos( origin, g_ServerAnimations.m_uServerAnimations.m_flEyeYaw, 25.f ), screen2 ) ) {
			Render::Engine::Line( screen1.x, screen1.y, screen2.x, screen2.y, Color( 255, 255, 255 ) );
			Render::Engine::esp_bold.string( screen2.x, screen2.y, Color::White( ), "real" );
		}

		if( Render::Engine::WorldToScreen( GetRotatedPos( pLocal->GetAbsOrigin( ), g_ServerAnimations.m_uServerAnimations.m_flLowerBodyYawTarget, 25.f ), screen2 ) ) {
			Render::Engine::Line( screen1.x, screen1.y, screen2.x, screen2.y, Color( 255, 255, 255 ) );
			Render::Engine::esp_bold.string( screen2.x, screen2.y, Color::White( ), "lby" );
		}
	}*/

	CVariables::RAGE *current = nullptr;

	std::vector<std::string> vecCrosshairIndicators;

	// call me gay
	if( pLocal && g_Vars.esp.crosshair_indicator_mindmg ) {
		auto pWeapon = reinterpret_cast< C_WeaponCSBaseGun * >( pLocal->m_hActiveWeapon( ).Get( ) );
		if( pWeapon ) {
			auto pWeaponInfo = pWeapon->GetCSWeaponData( );
			if( pWeaponInfo.IsValid( ) ) {
				if( pWeapon ) {
					auto id = pWeapon->m_iItemDefinitionIndex( );
					switch( pWeaponInfo->m_iWeaponType ) {
						case WEAPONTYPE_PISTOL:
							if( id == WEAPON_REVOLVER || id == WEAPON_DEAGLE ) {
								if( id == WEAPON_REVOLVER ) {
									current = &g_Vars.rage_revolver;
								}
								else {
									current = &g_Vars.rage_deagle;
								}
							}
							else {
								current = &g_Vars.rage_pistols;
							}
							break;
						case WEAPONTYPE_SNIPER_RIFLE:
							if( id == WEAPON_G3SG1 || id == WEAPON_SCAR20 ) {
								current = &g_Vars.rage_autosnipers;
							}
							else {
								current = ( id == WEAPON_AWP ) ? &g_Vars.rage_awp : &g_Vars.rage_scout;
							}
							break;
						default:
							current = &g_Vars.rage_default;
							break;
					}

					if( !current->override_default_config ) {
						current = &g_Vars.rage_default;
					}
				}
				else {
					current = &g_Vars.rage_default;
				}


				if( g_Vars.rage.min_damage_override_key.enabled ) {
					// current->minimal_damage_override
					auto GetDmgDisplay = [&] ( int dmg ) -> std::string {
						return dmg > 100 ? ( std::string( XorStr( "HP+" ) ).append( std::string( std::to_string( dmg - 100 ) ) ) ) : std::to_string( dmg );
					};

					vecCrosshairIndicators.push_back( std::string( "DMG: " ).append( GetDmgDisplay( current->minimal_damage_override ).data( ) ) );
				}
			}
		}
	}

	if( g_Vars.esp.crosshair_indicator_lby && g_Vars.rage.wait_for_lby_flick && g_Vars.rage.wait_for_lby_flick_key.enabled ) {
		vecCrosshairIndicators.push_back( XorStr( "FLICK" ) );
	}

	if( g_Vars.esp.crosshair_indicator_body && g_Vars.rage.force_body_aim && g_Vars.rage.force_body_aim_key.enabled ) {
		vecCrosshairIndicators.push_back( XorStr( "BAIM" ) );
	}

	if( g_Vars.esp.crosshair_indicator_override && g_Vars.rage.resolver && g_Vars.rage.resolver_override_key.enabled ) {
		vecCrosshairIndicators.push_back( XorStr( "OVERRIDE" ) );
	}

	if( bRenderIndicatorChair ) {
		int nCurrentPing = int( ( g_Animations.m_flOutgoingLatency + g_Animations.m_flIncomingLatency ) * 1000.f );
		vecCrosshairIndicators.push_back( std::string( "PING: " ).append( std::to_string( nCurrentPing ).data( ) ) );
	}

	if( vecCrosshairIndicators.size( ) ) {
		for( auto i = 0; i < vecCrosshairIndicators.size( ); ++i ) {
			if( g_Vars.esp.indicator_crosshair )
				Render::Engine::esp_pixel.string( Render::Engine::m_width / 2.f + 8.f, Render::Engine::m_height / 2.f + 8.f + ( 10 * i ),
												  Color( 255, 255, 255, 180 ), vecCrosshairIndicators[ i ].data( ) );
		}
	}

	if( indicators.empty( ) )
		return;

	const int nConstant = ( Render::Engine::m_height / 2.f );

	// iterate and draw indicators.
	for( size_t i{ }; i < indicators.size( ); ++i ) {
		auto &indicator = indicators[ i ];

		Render::Engine::esp_indicator.string( 11, nConstant + ( 22 * i ), Color( 0, 0, 0, 60 ), indicator.text );
		Render::Engine::esp_indicator.string( 12, nConstant - 1 + ( 22 * i ), Color( 0, 0, 0, 60 ), indicator.text );
		Render::Engine::esp_indicator.string( 13, nConstant + ( 22 * i ), Color( 0, 0, 0, 60 ), indicator.text );
		Render::Engine::esp_indicator.string( 12, nConstant + 1 + ( 22 * i ), Color( 0, 0, 0, 60 ), indicator.text );
		Render::Engine::esp_indicator.string( 13, nConstant + 1 + ( 22 * i ), Color( 0, 0, 0, 80 ), indicator.text );
		Render::Engine::esp_indicator.string( 11, nConstant + 1 + ( 22 * i ), Color( 0, 0, 0, 60 ), indicator.text );
		Render::Engine::esp_indicator.string( 11, nConstant + ( 22 * i ), Color( 0, 0, 0, 30 ), indicator.text );
		Render::Engine::esp_indicator.string( 12, nConstant + ( 22 * i ), indicator.color, indicator.text );
	}

	if( pLocal->m_fFlags( ) & FL_ONGROUND && g_Vars.rage.anti_aim_fake_body && g_Vars.esp.indicator_antiaim_lby && g_Prediction.get_unpredicted_vars( )->flags & FL_ONGROUND &&
		( change > 35.f || bPurple ) && ( pLocal->m_vecVelocity( ).Length2D( ) <= 0.1f || g_Vars.globals.m_bFakeWalking ) ) {
		int nWidth = Render::Engine::esp_indicator.size( XorStr( "LBY" ) ).m_width;

		float flUpdateTime = g_ServerAnimations.m_uRenderAnimations.m_flLowerBodyRealignTimer - g_pGlobalVars->realtime;
		float flBoxMultiplier = ( /*1.1f -*/ flUpdateTime ) / 1.1f;
		flBoxMultiplier = std::clamp<float>( flBoxMultiplier, 0.f, 1.f );

		if( flBoxMultiplier > 0.f ) {
			Render::Engine::RectFilled( Vector2D( 11, nConstant + ( 22 * indicators.size( ) ) - 3 ), Vector2D( nWidth, 4 ), Color( 0, 0, 0, 120 ) );
			Render::Engine::RectFilled( Vector2D( 11, nConstant + ( 22 * indicators.size( ) ) - 3 ) + 1, Vector2D( ( nWidth - 2 ) * flBoxMultiplier, 2 ),
										bPurple ? Color( 100, 100, 255 ) : Color( 160, 255, 0 ) );
		}
	}

	if( debug_messages_sane.empty( ) ) {
		return;
	}

	Vector2D screen = Render::GetScreenSize( );
	for( int i = 0; i < debug_messages_sane.size( ); ++i ) {
		auto msg = debug_messages_sane[ i ];
		auto width = Render::Engine::esp_bold.size( msg ).m_width;
		auto height = Render::Engine::esp_bold.size( msg ).m_height;

		Render::Engine::menu_regular.string( screen.x - 9 - width - 60, 2 + ( height * i ), Color( 255, 255, 255, 200 ), msg );
	}
}

bool Visuals::VisualSilentAimbot( CViewSetup *vsView ) {
	if( !vsView )
		return false;

	// onetap-style "visualize silent angles": only makes sense with silent aim.
	if( !g_Vars.rage.visual_silent_aim )
		return false;

	if( !g_Vars.rage.silent_aim )
		return false;

	if( !g_Vars.rage.enabled )
		return false;

	if( !g_pEngine->IsConnected( ) || !g_pEngine->IsInGame( ) )
		return false;

	// thirdperson has no viewmodel.
	if( g_pInput->m_fCameraInThirdPerson )
		return false;

	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal || pLocal->IsDead( ) )
		return false;

	const auto pViewModel = ( C_BaseViewModel * )pLocal->m_hViewModel( ).Get( );
	if( !pViewModel )
		return false;

	// no aimbot target -> nothing to visualize, leave viewmodel alone.
	if( !g_Ragebot.m_target )
		return false;

	// m_angle is set in Ragebot::find() for the current target.
	// use the same final server angle as apply() sends:
	// m_angle minus recoil (aimpunch * weapon_recoil_scale).
	QAngle angSilent( g_Ragebot.m_angle.x, g_Ragebot.m_angle.y, g_Ragebot.m_angle.z );
	angSilent -= pLocal->m_aimPunchAngle( ) * g_Vars.weapon_recoil_scale->GetFloat( );

	// keep it sane (anti-untrusted style clamp).
	angSilent.x = std::clamp( Math::AngleNormalize( angSilent.x ), -89.f, 89.f );
	angSilent.y = Math::AngleNormalize( angSilent.y );
	angSilent.z = 0.f;

	// viewmodel-aimbot technique (UC 308885):
	// rotate only the viewmodel entity, camera (vsView) stays untouched.
	// result: screen stays (silent), but gun/hands snap to the target -
	// exactly like onetap "visualize silent angles".
	pViewModel->SetAbsAngles( angSilent );

	return true;
}

void Visuals::AddDebugMessage( std::string msg ) {
	bool dont = false;

	if( debug_messages.size( ) ) {
		for( int i = 0; i < debug_messages.size( ); ++i ) {
			auto msgs = debug_messages[ i ];

			if( msgs.find( msg ) != std::string::npos )
				dont = true;
		}
	}

	if( dont ) {
		return;
	}

	debug_messages.push_back( msg );
}

void ExtendedVisuals::NormalizeSound( C_CSPlayer *player, SndInfo_t &sound ) {
	if( !player || !&sound || !sound.m_pOrigin )
		return;

	Vector src3D, dst3D;
	CGameTrace tr;
	Ray_t ray;
	CTraceFilter filter;

	filter.pSkip = player;
	src3D = ( *sound.m_pOrigin ) + Vector( 0, 0, 1 );
	dst3D = src3D - Vector( 0, 0, 100 );
	ray.Init( src3D, dst3D );

	g_pEngineTrace->TraceRay( ray, MASK_PLAYERSOLID, &filter, &tr );

	// step = (tr.fraction < 0.20)
	// shot = (tr.fraction > 0.20)
	// stand = (tr.fraction > 0.50)
	// crouch = (tr.fraction < 0.50)

	// Player stuck, idk how this happened
	if( tr.allsolid ) {
		m_arrSoundPlayers.at( player->EntIndex( ) ).m_iReceiveTime = -1;

		m_arrSoundPlayers.at( player->EntIndex( ) ).m_vecOrigin = *sound.m_pOrigin;
		m_arrSoundPlayers.at( player->EntIndex( ) ).m_nFlags = player->m_fFlags( );
	}
	else {
		m_arrSoundPlayers.at( player->EntIndex( ) ).m_vecOrigin = ( tr.fraction < 0.97 ? tr.endpos : *sound.m_pOrigin );
		m_arrSoundPlayers.at( player->EntIndex( ) ).m_nFlags = player->m_fFlags( );
		m_arrSoundPlayers.at( player->EntIndex( ) ).m_nFlags |= ( tr.fraction < 0.50f ? FL_DUCKING : 0 ) | ( tr.fraction != 1 ? FL_ONGROUND : 0 );
		m_arrSoundPlayers.at( player->EntIndex( ) ).m_nFlags &= ( tr.fraction > 0.50f ? ~FL_DUCKING : 0 ) | ( tr.fraction == 1 ? ~FL_ONGROUND : 0 );
	}
}

// todo - maxwell; fix this. https://i.imgur.com/T5q222e.png
bool ExtendedVisuals::ValidateSound( SndInfo_t &sound ) {
	if( !sound.m_bFromServer )
		return false;

	for( int i = 0; i < m_vecSoundBuffer.Count( ); i++ ) {
		const SndInfo_t &cachedSound = m_vecSoundBuffer[ i ];
		// was this sound already cached/processed?
		if( cachedSound.m_nGuid == sound.m_nGuid ) {
			return false;
		}
	}

	return true;
}

void ExtendedVisuals::Adjust( ) {
	if( !g_Vars.visuals_enemy.dormant )
		return;

	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	for( int i = 1; i <= g_pGlobalVars->maxClients; ++i ) {
		C_CSPlayer *player = C_CSPlayer::GetPlayerByIndex( i );
		if( !player ) {
			continue;
		}

		if( g_ExtendedVisuals.m_arrOverridePlayers.at( i ).m_eOverrideType == EOverrideType::ESP_SHARED )
			continue;

		// only grab info from dead players who are spectating others
		if( !player->IsDead( ) ) {
			continue;
		}

		// we can grab info from this spectator
		if( player->IsDormant( ) ) {
			continue;
		}

		// this player is spectating someone
		if( player->m_hObserverTarget( ).IsValid( ) ) {
			// grab the player who is being spectated (pSpectatee) by the spectator (player)
			const auto pSpectatee = ( C_CSPlayer * )player->m_hObserverTarget( ).Get( );
			if( pSpectatee && pSpectatee->EntIndex( ) <= 64 ) {
				// if the guy that's being spectated is dormant, set his origin
				// to the spectators origin (and ofc set dormancy state for esp to draw him)
				if( pSpectatee->IsDormant( ) ) {
					// make sure they're actually spectating them instead of flying around or smth
					if( player->m_iObserverMode( ) == 4 || player->m_iObserverMode( ) == 5 ) {
						m_arrOverridePlayers.at( pSpectatee->EntIndex( ) ).m_eOverrideType = EOverrideType::ESP_SPECTATOR;

						//std::memcpy( m_arrOverridePlayers.at( pSpectatee->EntIndex( ) ).m_flPoseParameters, player->m_flPoseParameter( ), sizeof( player->m_flPoseParameter( ) ) );
						m_arrOverridePlayers.at( pSpectatee->EntIndex( ) ).m_vecLastOrigin = m_arrOverridePlayers.at( pSpectatee->EntIndex( ) ).m_vecOrigin;
						m_arrOverridePlayers.at( pSpectatee->EntIndex( ) ).m_vecOrigin = player->m_vecOrigin( );
						m_arrOverridePlayers.at( pSpectatee->EntIndex( ) ).m_flReceiveTime = g_pGlobalVars->realtime;
						m_arrOverridePlayers.at( pSpectatee->EntIndex( ) ).m_flServerTime = player->m_flSimulationTime( );
					}
				}
			}
		}
	}

	CUtlVector<SndInfo_t> m_vecCurSoundList;
	g_pEngineSound->GetActiveSounds( m_vecCurSoundList );

	// No active sounds.
	if( !m_vecCurSoundList.Count( ) )
		return;

	for( int i = 0; i < m_vecCurSoundList.Count( ); i++ ) {
		SndInfo_t &sound = m_vecCurSoundList[ i ];
		if( sound.m_nSoundSource == 0 || // World
			sound.m_nSoundSource > 64 )   // Most likely invalid
			continue;

		C_CSPlayer *player = ( C_CSPlayer * )g_pEntityList->GetClientEntity( sound.m_nSoundSource );

		if( !player || !sound.m_pOrigin || !player->IsPlayer( ) || player == pLocal || player->IsTeammate( pLocal ) || sound.m_pOrigin->IsZero( ) )
			continue;

		// we have a valid spectator dormant player entry
		if( m_arrOverridePlayers.at( player->EntIndex( ) ).m_eOverrideType != EOverrideType::ESP_NONE ) {
			// no need to do anything else for this player
			continue;
		}

		// if( !ValidateSound( sound ) )
		// 	continue;

		NormalizeSound( player, sound );

		m_arrSoundPlayers.at( player->EntIndex( ) ).Override( sound );
	}

	for( int i = 1; i <= g_pGlobalVars->maxClients; ++i ) {
		const auto player = C_CSPlayer::GetPlayerByIndex( i );
		if( !player || player->IsDead( ) || !player->IsDormant( ) ) {
			// notify visuals that this target is officially not dormant..
			m_arrSoundPlayers.at( i ).m_bValidSound = false;
			m_arrOverridePlayers.at( i ).m_eOverrideType = EOverrideType::ESP_NONE;
			if( player && !player->IsDormant( ) ) {
				m_arrOverridePlayers.at( i ).m_vecOrigin = player->m_vecOrigin( );
				m_arrSoundPlayers.at( i ).m_flLastNonDormantTime = g_pGlobalVars->realtime;
			}

			continue;
		}

		constexpr int EXPIRE_DURATION = 1000;
		auto &soundPlayer = m_arrSoundPlayers.at( player->EntIndex( ) );

		bool bSoundExpired = GetTickCount( ) - soundPlayer.m_iReceiveTime > EXPIRE_DURATION;
		if( bSoundExpired ) {
			m_arrSoundPlayers.at( i ).m_bValidSound = false;
			continue;
		}

		// first backup the player
		SoundPlayer backupPlayer;
		backupPlayer.m_iIndex = player->m_entIndex;
		backupPlayer.m_nFlags = player->m_fFlags( );
		backupPlayer.m_vecOrigin = player->m_vecOrigin( );
		backupPlayer.m_vecAbsOrigin = player->GetAbsOrigin( );

		m_vecRestorePlayers.emplace_back( backupPlayer );

		// notify visuals that this target is officially dormant but we found a sound..
		soundPlayer.m_bValidSound = true;

		// set stuff accordingly.
		player->m_fFlags( ) = soundPlayer.m_nFlags;
		player->m_vecOrigin( ) = soundPlayer.m_vecOrigin;
		player->SetAbsOrigin( soundPlayer.m_vecOrigin );
	}

	// copy sounds (cache)
	m_vecSoundBuffer = m_vecCurSoundList;
}

void ExtendedVisuals::Restore( ) {
	if( m_vecRestorePlayers.empty( ) )
		return;

	for( auto &restorePlayer : m_vecRestorePlayers ) {
		auto player = C_CSPlayer::GetPlayerByIndex( restorePlayer.m_iIndex );
		if( !player || player->IsDormant( ) )
			continue;

		player->m_fFlags( ) = restorePlayer.m_nFlags;
		player->m_vecOrigin( ) = restorePlayer.m_vecOrigin;
		player->SetAbsOrigin( restorePlayer.m_vecAbsOrigin );

		m_arrSoundPlayers.at( restorePlayer.m_iIndex ).m_bValidSound = false;
	}

	m_vecRestorePlayers.clear( );
}