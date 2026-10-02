#include "Glow.hpp"
#include <fstream>
#include "Visuals.hpp"
#include "../../SDK/Classes/weapon.hpp"

Glow g_Glow;

void Glow::OnPostScreenEffects( ) {
	if( !g_pEngine->IsInGame( ) )
		return;

	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	for( auto i = 0; i < g_pGlowObjectManager->m_GlowObjectDefinitions.m_Size; i++ ) {
		auto &glowObject = g_pGlowObjectManager->m_GlowObjectDefinitions.m_Memory.m_pMemory[ i ];
		auto pEntity = ToCSPlayer( ( C_BaseEntity * )glowObject.m_pEntity );

		if( !&glowObject )
			continue;

		if( glowObject.IsUnused( ) )
			continue;

		if( !pEntity )
			continue;


		if( pEntity->IsDormant( ) || !pEntity->GetClientClass( ) )
			continue;

		auto color = Color_f{ };

		switch( pEntity->GetClientClass( )->m_ClassID ) {
			case CBaseCSGrenadeProjectile:
			case CMolotovProjectile:
			case CSmokeGrenadeProjectile:
			case CDecoyProjectile:
			case CInferno:
				if( g_Vars.esp.grenades_glow ) {
					color = g_Vars.esp.grenades_glow_color;

					glowObject.m_bRenderWhenOccluded = true;
					glowObject.m_nGlowStyle = 0;
					glowObject.m_vGlowColor = Vector4D( color.r, color.g, color.b, color.a );
					glowObject.m_bRenderWhenUnoccluded = false;
					continue;
				}
				break;
		}

		if( pEntity->IsWeapon( ) && g_Vars.esp.dropped_weapons_glow ) {
			auto pWeapon = reinterpret_cast< C_WeaponCSBaseGun * >( pEntity );

			if( pWeapon->m_iItemDefinitionIndex( ) > 0 && pWeapon->m_hOwnerEntity( ) == -1 ) {
				color = g_Vars.esp.dropped_weapons_glow_color;

				glowObject.m_bRenderWhenOccluded = true;
				glowObject.m_nGlowStyle = 0;
				glowObject.m_vGlowColor = Vector4D( color.r, color.g, color.b, color.a );
				glowObject.m_bRenderWhenUnoccluded = false;
				continue;
			}
		}

		if( pEntity->IsDead( ) )
			continue;

		if( pEntity->GetClientClass( )->m_ClassID == CCSPlayer ) {
			glowObject.m_bRenderWhenOccluded = false;
			color = Color_f( 0.f, 0.f, 0.f, 0.f );

			if( pEntity->EntIndex( ) == pLocal->EntIndex( ) ) {
				if( g_Vars.visuals_local.glow ) {
					color = g_Vars.visuals_local.glow_color;

					glowObject.m_bRenderWhenOccluded = true;
				}
			}
			else {
				C_CSPlayer *pDummyLocal = pLocal;
				bool bSpectatingPlayer = false;
				if( pLocal->IsDead( ) ) {
					if( pLocal->m_hObserverTarget( ).IsValid( ) ) {
						const auto pSpectatingPlayer = ( C_CSPlayer * )pLocal->m_hObserverTarget( ).Get( );
						if( pSpectatingPlayer ) {
							if( pLocal->m_iObserverMode( ) == 4 || pLocal->m_iObserverMode( ) == 5 ) {
								pDummyLocal = pSpectatingPlayer;
							}
						}
					}
				}

				const bool bIsEnemy = !pEntity->IsTeammate( pDummyLocal, false );

				if( bIsEnemy ) {
					if( g_Vars.visuals_enemy.glow ) {
						color = g_Vars.visuals_enemy.glow_color;

						color.a *= g_Visuals.player_fading_alpha[ pEntity->EntIndex( ) ];

						glowObject.m_bRenderWhenOccluded = true;
					}
				}
				else {
					// overwriting pDummyLocal from the defalt (pLocal)
					// to the spec target, apply default team glow
					if( pDummyLocal != pLocal ) {
						if( pDummyLocal->m_iTeamNum( ) == TEAM_CT ) {
							color[ 0 ] = 114 / 255.f;
							color[ 1 ] = 155 / 255.f;
							color[ 2 ] = 221 / 255.f;
						}
						else {
							color[ 0 ] = 224 / 255.f;
							color[ 1 ] = 175 / 255.f;
							color[ 2 ] = 86 / 255.f;
						}

						color[ 3 ] = 0.6f;

						glowObject.m_bRenderWhenOccluded = true;
					}
				}
			}
		}

		glowObject.m_nGlowStyle = 0;
		glowObject.m_vGlowColor = Vector4D( color.r, color.g, color.b, color.a );
		glowObject.m_bRenderWhenUnoccluded = false;
	}

	// HandlePlayerOutline( );
}

void Glow::ApplyPlayerOutline( C_CSPlayer *pEntity, Color_f flColor, std::function<void( )> oRenderModel ) {
	return;
	if( !g_pEngine->IsInGame( ) )
		return;

	if( !pEntity )
		return;

	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	auto pRenderContext = g_pMaterialSystem->GetRenderContext( );
	if( !pRenderContext )
		return;

	static IMaterial *pWireframeMaterial = g_pMaterialSystem->FindMaterial( XorStr( "debug/debugdrawflat" ), nullptr );
	if( !pWireframeMaterial )
		return;

	if( !g_Vars.visuals_enemy.outline && !g_Vars.visuals_local.outline )
		return;

	// force wireframe and ignorez
	pWireframeMaterial->SetMaterialVarFlagSane( MATERIAL_VAR_WIREFRAME, true );
	pWireframeMaterial->SetMaterialVarFlagSane( MATERIAL_VAR_IGNOREZ, true );

	// setup stencil render states
	ShaderStencilState_t uStencilState;
	uStencilState.m_bEnable = true;
	uStencilState.m_nReferenceValue = 1;
	uStencilState.m_PassOp = 3;
	uStencilState.m_FailOp = 1;
	uStencilState.m_ZFailOp = 3;
	uStencilState.m_CompareFunc = 8;
	uStencilState.m_nTestMask = 0xFFFFFFFF;
	uStencilState.m_nWriteMask = 0xFFFFFFFF;
	pRenderContext->SetStencilState( uStencilState );

	// force the regular model to be 0 alpha
	g_pRenderView->SetBlend( 0.f );
	g_pStudioRender->m_AlphaMod = 0.f;

	uStencilState.m_CompareFunc = 8;
	pRenderContext->SetStencilState( uStencilState );

	// render the "0 alpha model", which gets rid of the inner wireframe
	// and only leaves a nice outline
	// g_Chams.DrawModelEx( pEntity );
	oRenderModel( );

	// no clue what the fuck this shit d oes
	uStencilState.m_CompareFunc = 6;
	pRenderContext->SetStencilState( uStencilState );

	g_pStudioRender->m_AlphaMod = 1.f;
	g_pRenderView->SetBlend( 1.f );

	pWireframeMaterial->ColorModulate( flColor.r, flColor.g, flColor.b );

	// fade out the outline
	pWireframeMaterial->AlphaModulate( flColor.a * ( ( pEntity->EntIndex( ) == g_pEngine->GetLocalPlayer( ) ) ? 1.f : g_Visuals.player_fading_alpha[ pEntity->EntIndex( ) ] ) );

	// force wireframe material for the outline
	g_pModelRender->ForcedMaterialOverride( pWireframeMaterial );

	// render the outline
	oRenderModel( );

	// invalidate material
	g_pModelRender->ForcedMaterialOverride( nullptr );

	// force 1.f alpha modulation, as well as blend here
	g_pStudioRender->m_AlphaMod = 1.f;
	g_pRenderView->SetBlend( 1.f );

	uStencilState.m_bEnable = false;
	pRenderContext->SetStencilState( uStencilState );
}