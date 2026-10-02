#include "../Hooked.hpp"
#include "../../SDK/sdk.hpp"
#include "../../SDK/Displacement.hpp"
#include "../../SDK/Classes/Player.hpp"
#include "../../SDK/Classes/weapon.hpp"
#include "../../Features/Miscellaneous/SkinChanger.hpp"
#include "../../Features/Rage/Animations.hpp"
#include "../../Features/Rage/TickbaseShift.hpp"
#include "../../Features/Visuals/Models.hpp"

#include <intrin.h>
#include "../../Utils/Threading/threading.h"

namespace Hooked {
	void __fastcall DoExtraBoneProcessing( C_CSPlayer *ecx, void *edx, CStudioHdr *hdr, Vector *pos, Quaternion *rotations, matrix3x4_t *transforma, void *bone_list, void *ik_context ) {
		g_Vars.globals.szLastHookCalled = XorStr( "22" );
		if( !ecx )
			return;

		if( !ecx )
			return oDoExtraBoneProcessing( ecx, hdr, pos, rotations, transforma, bone_list, ik_context );

		// haha.
		return;
	}

	bool __fastcall SetupBones( void *ecx, void *edx, matrix3x4_t *matrix, int bone_count, int bone_mask, float time ) {
		if( !ecx )
			return oSetupBones( ecx, matrix, bone_count, bone_mask, time );

		// from renderable
		const auto pPlayer = reinterpret_cast< C_CSPlayer * >( uintptr_t( ecx ) - 0x4 );
		if( !pPlayer || pPlayer->EntIndex( ) <= 0 || pPlayer->EntIndex( ) > 64 )
			return oSetupBones( ecx, matrix, bone_count, bone_mask, time );

		const auto pLocal = C_CSPlayer::GetLocalPlayer( );
		if( !pLocal )
			return oSetupBones( ecx, matrix, bone_count, bone_mask, time );

		// those are attachments
		if( pPlayer->EntIndex( ) == pLocal->EntIndex( ) && bone_mask == 512 ) {
			matrix3x4_t pMatrix[ MAXSTUDIOBONES ];
			std::memcpy( pMatrix, pLocal->m_CachedBoneData( ).Base( ), pLocal->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );

			auto ret = true;
			if( g_Models.m_bAllowedBoneSetup ) {
				ret = oSetupBones( ecx, matrix, bone_count, bone_mask, time );

				std::memcpy( pLocal->m_CachedBoneData( ).Base( ), pMatrix, pLocal->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );
				pLocal->m_iMostRecentModelBoneCounter( ) = *( int * )Engine::Displacement.Data.m_uModelBoneCounter;
				pLocal->m_BoneAccessor( ).m_ReadableBones = pLocal->m_BoneAccessor( ).m_WritableBones = 0xFFFFFFFF;
				pLocal->m_flLastBoneSetupTime( ) = FLT_MAX;
			}
			else {
				// instead of building bones for the c4, we attachment heLp our LocalPlayer :-)
				if( const auto pStudioHdr = pLocal->m_pStudioHdr( ); pStudioHdr ) {
					using AttachmentHelperFn = void( __thiscall * )( C_BaseEntity *, CStudioHdr * );
					( ( AttachmentHelperFn )Engine::Displacement.Function.m_AttachmentHelper )( pLocal, pStudioHdr );
				}
			}


			return ret;
		}

		if( g_Vars.globals.m_bInBoneSetup || !pLocal || pLocal != pPlayer && pPlayer->m_iTeamNum( ) == pLocal->m_iTeamNum( ) || pLocal->IsDead( ) ) {
			return oSetupBones( ecx, matrix, bone_count, bone_mask, time );;
		}
		else {
			int nNewBoneCount = 128;

			if( bone_count <= nNewBoneCount ) {
				nNewBoneCount = bone_count;
			}

			if( matrix ) {
				if( nNewBoneCount > 0 )
					std::memcpy( matrix, pPlayer->m_CachedBoneData( ).Base( ), sizeof( matrix3x4_t ) * bone_count );

				return true;
			}
			else {
				return true;
			}
		}
		if( matrix && pPlayer->m_CachedBoneData( ).Base( ) ) {
			if( bone_count < pPlayer->m_CachedBoneData( ).Count( ) )
				return false;

			std::memcpy( matrix, pPlayer->m_CachedBoneData( ).Base( ), sizeof( matrix3x4_t ) * pPlayer->m_CachedBoneData( ).Count( ) );
		}

		return true;
	}

	void __fastcall PostDataUpdate( void *ecx, void *edx, int updateType ) {
		g_Vars.globals.szLastHookCalled = XorStr( "25" );

		// from networkable
		const auto pEntity = reinterpret_cast< C_CSPlayer * >( uintptr_t( ecx ) - 0x8 );

		if( !pEntity || !ecx )
			return oPostDataUpdate( ecx, updateType );

		g_SkinChanger.m_bSkipCheck = true;

		auto pLocal = C_CSPlayer::GetLocalPlayer( );

		g_SkinChanger.m_bSkipCheck = false;

		if( pLocal && pEntity->m_entIndex && pEntity->IsPlayer( ) ) {
			const bool bIsLocalPlayer = pLocal->m_entIndex == pEntity->m_entIndex;
			if( bIsLocalPlayer ) {
				// High-ping защита на уровне движка: первое применение ДО
				// оригинального PostDataUpdate (значение переживёт декод),
				// второе — ПОСЛЕ (OnNetworkUpdate(false) внутри повторно
				// накатывает скины, перезатирая то, что прислал сервер).
				// Плюс FrameStageNotify делает то же самое на
				// START/END для ВСЕХ оружий + LockKnifeModels в RENDER_START.
				g_SkinChanger.OnNetworkUpdate( true );

				g_TickbaseController.m_bOverrideIsPaused = g_TickbaseController.m_bInCharge;

				oPostDataUpdate( ecx, updateType );

				g_TickbaseController.m_bOverrideIsPaused = false;

				g_SkinChanger.OnNetworkUpdate( false );
			}
			else {
				auto pAnimationEntry = g_Animations.GetAnimationEntry( pEntity->m_entIndex );
				if( pAnimationEntry ) {
					auto &alive_loop = pEntity->m_AnimOverlay( )[ ANIMATION_LAYER_ALIVELOOP ];

					if( pEntity->m_flSimulationTime( ) == pAnimationEntry->m_flOldSimulationTimeAlt ) {
						if( alive_loop.m_flCycle == pAnimationEntry->m_flPreviousLayer11CyclePostDataUpdate )
							pEntity->m_flSimulationTime( ) = pEntity->m_flOldSimulationTime( );
					}
					else {
						auto server_tick = g_pClientState->clock_drift_mgr.m_nServerTick;
						auto network_time_base = g_pGlobalVars->GetNetworkBase( server_tick, pEntity->m_entIndex );
						auto old_network_time_base = g_pGlobalVars->GetNetworkBase( pAnimationEntry->m_nServerTick, pEntity->m_entIndex );

						auto save_server_tick = ( TIME_TO_TICKS( pEntity->m_flSimulationTime( ) ) % 100 )
							|| alive_loop.m_flCycle != pAnimationEntry->m_flPreviousLayer11CyclePostDataUpdate
							|| network_time_base == old_network_time_base;

						if( save_server_tick )
							pAnimationEntry->m_nServerTick = server_tick;
						else {
							pAnimationEntry->m_flOldSimulationTimeAlt = pEntity->m_flSimulationTime( );
							pEntity->m_flSimulationTime( ) = pEntity->m_flOldSimulationTime( );
						}
					}

					pAnimationEntry->m_flPreviousLayer11CyclePostDataUpdate = alive_loop.m_flCycle;
				}

				oPostDataUpdate( ecx, updateType );
			}
		}
		else {
			oPostDataUpdate( ecx, updateType );
		}
	}
}
