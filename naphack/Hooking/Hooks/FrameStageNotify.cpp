#include "../Hooked.hpp"
#include "../../Features/Rage/EnginePrediction.hpp"
#include "../../SDK/Displacement.hpp"
#include "../../SDK/sdk.hpp"
#include "../../SDK/Classes/Player.hpp"
#include "../../Features/Visuals/EventLogger.hpp"
#include "../../Renderer/Render.hpp"
#include "../../Utils/InputSys.hpp"
#include "../../SDK/Classes/Exploits.hpp"
#include "../../Utils/Threading/threading.h"
#include "../../Features/Rage/Animations.hpp"
#include "../../Features/Rage/Resolver.hpp"
#include "../../Features/Rage/ShotHandling.hpp"
#include "../../Features/Rage/TickbaseShift.hpp"
#include "../../Features/Visuals/Models.hpp"
#include "../../Features/Scripting/Scripting.hpp"
#include "../../Features/Rage/ServerAnimations.hpp"
#include "../../Features/Miscellaneous/Miscellaneous.hpp"
#include "../../Features/Miscellaneous/SkinChanger.hpp"
#include "../../Features/Visuals/WeatherController.hpp"
#include "../../Features/Rage/BoneSetup.hpp"
#include "../../Features/Miscellaneous/PlayerList.hpp"

//#define SERVER_HITBOXES 
//#define TEST_EXTRAP
// #define TEST_ANIMS

using CCLCMsg_VoiceData_ctor_t = void( __thiscall * )( void *msg );
using CCLCMsg_VoiceData_dtor_t = void( __thiscall * )( void *msg );
using protobuf_assign_t = void( __thiscall * )( void *ptr, void *data, size_t len );

void FuckYou( ) {
	static CCLCMsg_VoiceData_ctor_t CCLCMsg_VoiceData_ctor = ( CCLCMsg_VoiceData_ctor_t )Memory::Scan( XorStr( "engine.dll" ), XorStr( "56 57 8B F9 8D 4F 08 C7 07 ? ? ? ? E8 ? ? ? ? C7 07" ) );
	static CCLCMsg_VoiceData_dtor_t CCLCMsg_VoiceData_dtor = ( CCLCMsg_VoiceData_dtor_t )Memory::Scan( XorStr( "engine.dll" ), XorStr( "53 8B D9 56 8D 73 3C" ) );

	static uintptr_t tmp_protobuf_assign = Memory::Scan( XorStr( "engine.dll" ), XorStr( "E8 ? ? ? ? 83 ? ? ? ? 83 ? ? ? ? C7 44" ) );
	static protobuf_assign_t protobuf_assign = ( protobuf_assign_t )Memory::CallableFromRelative( tmp_protobuf_assign );

	INetChannel *nc;
	uint8_t      msg[ 104 ];

	nc = g_pEngine->GetNetChannelInfo( );
	if( !nc )
		return;

	static float flCrashTime = 0.f;

	if( g_Vars.globals.oppa ) {
		flCrashTime = g_pGlobalVars->realtime;
		g_Vars.globals.oppa = false;
	}

	if( flCrashTime <= 0.f )
		return;

	if( fabsf( flCrashTime - g_pGlobalVars->realtime ) > 10.f )
		return;

	CCLCMsg_VoiceData_ctor( &msg );
	protobuf_assign( &msg[ 8 ], nullptr, 0 );

	for( int i = 0; i < 512; ++i )
		nc->SendNetMsg( ( INetMessage * )&msg );

	nc->Transmit( );

	CCLCMsg_VoiceData_dtor( &msg );
}

#if defined(SERVER_HITBOXES) || defined(TEST_ANIMS)|| defined(TEST_EXTRAP)
void DrawServerHitboxes( int index ) {
	auto GetPlayerByIndex = [ ] ( int index ) -> C_CSPlayer * { //i dont need this shit func for anything else so it can be lambda
		typedef C_CSPlayer *( __fastcall *player_by_index )( int );
		static auto nIndex = reinterpret_cast< player_by_index >( Memory::Scan( XorStr( "server.dll" ), XorStr( "85 C9 7E 2A A1" ) ) );

		if( !nIndex )
			return false;

		return nIndex( index );
	};

	static auto DrawServerHitboxesFn = Memory::Scan( XorStr( "server.dll" ), XorStr( "55 8B EC 81 EC ? ? ? ? 53 56 8B 35 ? ? ? ? 8B D9 57 8B CE" ) );
	auto flDuration = -1.f;
	PVOID pEntity = nullptr;

	pEntity = GetPlayerByIndex( index );
	if( !pEntity )
		return;

	using GetBaseAnimatingServer = void *( __thiscall * )( void * );
	auto pBaseAnimating = Memory::VCall< GetBaseAnimatingServer >( pEntity, 54 )( pEntity );

	if( pBaseAnimating ) {
		using FlushBoneCache = void( __thiscall * )( void * );
		Memory::VCall< FlushBoneCache >( pBaseAnimating, 221 )( pBaseAnimating );
	}

	__asm {
		pushad
		movss xmm1, flDuration
		push 1 // 0 - colored, 1 - blue
		mov ecx, pEntity
		call DrawServerHitboxesFn
		popad
	}
}
#endif

void __fastcall Hooked::FrameStageNotify( void *ecx, void *edx, ClientFrameStage_t stage ) {
	g_Vars.globals.szLastHookCalled = XorStr( "9" );
	g_Vars.globals.m_eLastStage = stage;
	FuckYou( );

	//g_Prediction.m_nLastFrameStage = stage;
	g_Vars.globals.m_iTick = g_pGlobalVars->tickcount;

	const auto pLocal = reinterpret_cast< C_CSPlayer * >( g_pEntityList->GetClientEntity( g_pEngine->GetLocalPlayer( ) ) );

	if( !g_Vars.globals.m_bInCreateMove && g_Vars.globals.m_bForceFullUpdate && stage == FRAME_NET_UPDATE_POSTDATAUPDATE_START && pLocal && !pLocal->IsDead( ) && g_pEngine->IsInGame( ) && !g_pEngine->IsDrawingLoadingImage( ) ) {
		g_pClientState->m_nDeltaTick( ) = -1;

		g_Prediction.reset( );

		g_TickbaseController.m_pWeapon = nullptr;
		g_TickbaseController.m_pWeaponData = nullptr;

		// reset it to not end up using invalid data :)
		for( int i = 1; i <= 64; ++i ) {
			g_Vars.globals.m_bPlantingC4[ i ] = false;

			const auto pAnimData = g_Animations.GetAnimationEntry( i );
			if( !pAnimData )
				continue;

			pAnimData->m_deqRecords.clear( );
			pAnimData->m_pEntity = nullptr;
		}

		oFrameStageNotify( ecx, stage );

		g_Vars.globals.m_bForceFullUpdate = false;
		return;
	}

	// Скинчейнджер с защитой от сети (high-ping stability, 2018 source):
	// - POSTDATAUPDATE_START: первое применение до сетевых PostDataUpdate.
	// - POSTDATAUPDATE_END: второе применение после перезаписи сервером.
	// - RENDER_START: жёсткая блокировка модели ножа каждый кадр.
	// Порядок важен: движок вызывает PostDataUpdate для всех сущностей МЕЖДУ
	// START и END, поэтому двойное применение гарантирует fallback до рендера
	// даже на пинге 200+, где DataTable-апдейты приходят чаще/позже.
	if( stage == FRAME_NET_UPDATE_POSTDATAUPDATE_START && pLocal && !pLocal->IsDead( )
		&& g_pEngine->IsInGame( ) && !g_pEngine->IsDrawingLoadingImage( ) ) {
		g_SkinChanger.OnFrameStageNotify( stage );
	}

	if( static int bPatchLimit = 0; !bPatchLimit++ ) {
		auto CL_SendMoveMaxCommands = Memory::Scan( XorStr( "engine.dll" ), XorStr( "B8 ? ? ? ? 3B F0 0F 4F F0 89 5D FC" ) ) + 1;

		unsigned long lpflOldProtect = 0;

		VirtualProtect( ( void * )CL_SendMoveMaxCommands, 4, PAGE_EXECUTE_READWRITE, &lpflOldProtect );
		printf( "%i -> 17\n", *( int * )CL_SendMoveMaxCommands );
		*( int * )CL_SendMoveMaxCommands = 17; // lel
		VirtualProtect( ( void * )CL_SendMoveMaxCommands, 4, lpflOldProtect, &lpflOldProtect );
	}

	// CAN I DO THIS?LOL
	if( !pLocal ) {
		// crigne
		if( !g_pEngine->IsInGame( ) ) {
			static ConVar *cl_predict = g_pCVar->FindVar( XorStr( "cl_predict" ) );
			if( cl_predict->fnChangeCallback.m_Size != 0 ) {
				cl_predict->fnChangeCallback.m_Size = 0;
			}

			if( !cl_predict->GetBool( ) ) {
				cl_predict->SetValueInt( 1 );
			}

			if( g_Vars.cl_interp->GetFloat( ) != 0.015625f ) {
				g_Vars.cl_interp->SetValueFloat( 0.015625f );
			}

			if( g_Vars.cl_interp_ratio->GetInt( ) != 1 ) {
				g_Vars.cl_interp_ratio->SetValueInt( 1 );
			}

			g_WeatherController.UpdateWeather( );
			g_Misc.Modulation( );
			g_Misc.SkyBoxChanger( );
			g_Misc.UnlockConVars( );
			g_Models.HandleAimbotMatrices( );
			g_WeatherController.ReleasePrecipitationEntity( );
			//g_SkinChanger.DestroyGlove( );
			g_Animations.OnFrameStageNotify( );
			g_PlayerList.UpdatePlayerList( );
		}

		//g_Ragebot.m_vecLastSkippedPlayers.clear( );

		g_Prediction.reset( );

		return oFrameStageNotify( ecx, stage );
	}


#if defined(LUA_SCRIPTING)
	if( !g_pEngine->IsDrawingLoadingImage( ) && pLocal ) {
		Scripting::Script::DoCallback( hash_32_fnv1a_const( XorStr( "pre_frame_stage" ) ), stage );
	}
#endif

	if( g_Vars.sv_showimpacts_time ) {
		struct client_hit_verify_t {
			Vector vecPosition;
			float flTime;
			float flExpiration;
		};

		static auto nLastCount = 0;
		auto &uClientImpactList = *reinterpret_cast< CUtlVector< client_hit_verify_t >* >( uintptr_t( pLocal ) + Engine::Displacement.C_CSPlayer.m_utlClientImpactList );

		for( auto i = uClientImpactList.Count( ); i > nLastCount; i-- ) {
			// we want the rest of the code to always run
			// so we don't draw old impacts when we enable this after shooting a couple.
			if( g_Vars.esp.server_impacts ) {
				g_pDebugOverlay->AddBoxOverlay( uClientImpactList[ i - 1 ].vecPosition, { -1.5f, -1.5f, -1.5f }, { 1.5f, 1.5f, 1.5f }, { }, g_Vars.esp.client_impacts_color.r * 255, g_Vars.esp.client_impacts_color.g * 255, g_Vars.esp.client_impacts_color.b * 255, g_Vars.esp.client_impacts_color.a * 255, g_Vars.sv_showimpacts_time->GetFloat( ) );
			}
		}

		if( uClientImpactList.Count( ) != nLastCount )
			nLastCount = uClientImpactList.Count( );
	}

	g_Misc.PerformConvarRelated( );

	// reset whitelist when toggling the checkbox (don't think there are menu callbacks)
	static bool bOldWhitelistValue = g_Vars.menu.whitelist;
	const bool bCurrWhiteListValue = g_Vars.menu.whitelist;
	if( bOldWhitelistValue != bCurrWhiteListValue ) {
		for( int i = 1; i <= 64; ++i ) {
			g_Ragebot.m_arrNapUsers.at( i ).first = false;
			g_Ragebot.m_arrNapUsers.at( i ).second = "";
		}

		bOldWhitelistValue = bCurrWhiteListValue;
	}

	// idk where to move this to its ugly here

	static const auto ppGameRulesProxy = *reinterpret_cast< CCSGameRules *** >( Engine::Displacement.Data.m_uGameRulesProxy );
	if( *ppGameRulesProxy ) {
		g_pGameRules = *ppGameRulesProxy;
	}

	if( g_pEngine->IsInGame( ) ) {
		if( g_pClientState->m_nDeltaTick( ) == -1 ) {
			//g_SkinChanger.DestroyGlove( true );
			return oFrameStageNotify( ecx, stage );
		}
	}

	if( stage == FRAME_RENDER_START ) {
		/*if( pLocal ) {
			pLocal->SetAbsAngles( QAngle( 0.f, g_ServerAnimations.m_uServerAnimations.m_flFootYaw, 0.f ) );
		}*/

		if( auto pState = pLocal->m_PlayerAnimState( ); pState && pLocal && pLocal->m_AnimOverlay( ).Base( ) && pLocal->m_AnimOverlay( ).Count( ) > 0 && g_ServerAnimations.m_pCmd ) {
			std::memcpy( g_ServerAnimations.m_uVisualAnimations.m_pPoseParameters.data( ), pLocal->m_flPoseParameter( ), sizeof( float ) * 20 );
			matrix3x4_t pBackupMatrix[ MAXSTUDIOBONES ];
			std::memcpy( pBackupMatrix, pLocal->m_CachedBoneData( ).Base( ), pLocal->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );
			QAngle angAbsAngles = pLocal->GetAbsAngles( );

			CCSGOPlayerAnimState pStateBackup{};
			std::memcpy( &pStateBackup, pState, sizeof( CCSGOPlayerAnimState ) );

			pLocal->SetAbsAngles( QAngle( 0.f, g_ServerAnimations.m_uServerAnimations.m_flFootYaw, 0.f ) );

			std::array<C_AnimationLayer, 13> pAnimOverlays;
			std::memcpy( pAnimOverlays.data( ), pLocal->m_AnimOverlay( ).Base( ), 13 * sizeof( C_AnimationLayer ) );

			// setup ground and flag stuff
			static bool bOnGround = false;
			bool bWasOnGround = bOnGround;
			bOnGround = ( pLocal->m_fFlags( ) & 1 );

			bool v68 = bWasOnGround && !bOnGround;
			if( v68 && g_ServerAnimations.m_pCmd->buttons & IN_JUMP ) {
				auto v94 = &g_ServerAnimations.m_uVisualAnimations.m_pAnimOverlays.data( )[ 4 ];
				auto v95 = pState->SelectSequenceFromActivityModifier( 985 );

				v94->m_nSequence = v95;
				v94->m_flPlaybackRate = pLocal->GetLayerSequenceCycleRate( v94, v95 );
				v94->m_flCycle = 0.0f;
				v94->m_flWeight = 0.0f;

				g_ServerAnimations.m_uVisualAnimations.m_pAnimOverlays.data( )[ 4 ] = *v94;
			}

			std::memcpy( pLocal->m_AnimOverlay( ).Base( ), g_ServerAnimations.m_uVisualAnimations.m_pAnimOverlays.data( ), 13 * sizeof( C_AnimationLayer ) );

			// pLocal->m_AnimOverlay( )[ animstate_layer_t::ANIMATION_LAYER_LEAN ].m_flWeight = 0.f;

			//if( pState->m_bLanding && ( g_Prediction.get_initial_vars( )->flags & FL_ONGROUND ) && ( pLocal->m_fFlags( ) & FL_ONGROUND ) ) {
			//	float flPitch = -10.f;

			//	// -10.f pitch
			//	pLocal->m_flPoseParameter( )[ AIM_BLEND_CROUCH_IDLE ] = ( flPitch + 90.f ) / 180.f;
			//}

			c_bone_builder pVisualBones;
			pVisualBones.store( pLocal, g_ServerAnimations.m_uVisualAnimations.m_pMatrix, 0x7FF00 );
			pVisualBones.setup( );

			std::memcpy( pLocal->m_AnimOverlay( ).Base( ), pAnimOverlays.data( ), 13 * sizeof( C_AnimationLayer ) );

			for( int i = 0; i < MAXSTUDIOBONES; ++i ) for( int n = 0; n <= 2; ++n )
				g_ServerAnimations.m_uVisualAnimations.m_pMatrix[ i ].m[ n ][ 3 ] -= pLocal->GetAbsOrigin( )[ n ];

			pLocal->SetAbsAngles( angAbsAngles );
			std::memcpy( pState, &pStateBackup, sizeof( CCSGOPlayerAnimState ) );
			std::memcpy( pLocal->m_CachedBoneData( ).Base( ), pBackupMatrix, pLocal->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );
			std::memcpy( pLocal->m_flPoseParameter( ), g_ServerAnimations.m_uVisualAnimations.m_pPoseParameters.data( ), sizeof( float ) * 20 );
		}

		// rotate the in-game radar (local blip arrow) to the REAL yaw.
		//
		// firstperson problem: the stock radar blip reads pl.v_angle
		// (local viewangles, written via SetLocalViewAngles / prediction),
		// not m_angEyeAngles. anti-aim leaves the fake there, so the radar
		// showed FAKE in firstperson while thirdperson worked (orbit camera
		// is immune to SetLocalViewAngles, so forcing real there was free).
		// fix (UC 277713 pattern): swap v_angle yaw to REAL strictly for
		// the render window - set it in RENDER_START so the radar draws the
		// server truth, restore the FAKE back in RENDER_END so the camera /
		// prediction keep desyncing. we show REAL, never fake.
		// - NAPAS chokes real and sends fake (DoFakeYaw on send), hence the
		//   real angles are read off choked commands. with fakelag off
		//   nothing chokes, so fall back to the wish (engine) angles instead
		//   of freezing on a stale value.
		if( g_Vars.misc.radar_real_yaw && pLocal && !pLocal->IsDead( ) ) {
			QAngle angRadar{ };
			const bool bFreshReal = g_ServerAnimations.m_bHasRadarAngles
				&& ( g_pGlobalVars->realtime - g_ServerAnimations.m_flLastChokeTime ) < 0.5f;

			if( bFreshReal )
				angRadar = g_ServerAnimations.m_angRadarAngles;
			else
				g_pEngine->GetViewAngles( angRadar );

			const float flRealYaw = angRadar.y;

			// m_angRotation is a prediction datamap field, not a netvar,
			// so there is no offset for it in Displacement. resolve it
			// through the pred desc map; the scoreboard radar reads this one.
			// retry until found (first RENDER_START can run with dead local).
			static unsigned int uAngRotationOffset = 0u;
			if( !uAngRotationOffset ) {
				if( const auto pMap = pLocal->GetPredDescMap( ) )
					uAngRotationOffset = Memory::FindInDataMap( pMap, XorStr( "m_angRotation" ) );
			}

			// thirdperson (vader.tech pattern): the orbit camera is immune
			// here, so aligning the radar map to the real yaw is free.
			if( g_pInput->m_fCameraInThirdPerson ) {
				// drop a stale firstperson swap if the view switched mid-frame.
				g_ServerAnimations.m_bRadarVAngleSwapped = false;

				pLocal->m_angEyeAngles( ).y = flRealYaw;

				if( uAngRotationOffset ) {
					// QAngle, we only want .y
					*( float * )( uintptr_t( pLocal ) + uAngRotationOffset + sizeof( float ) ) = flRealYaw;
				}

				g_pPrediction->SetLocalViewAngles( angRadar );
			}
			else {
				// firstperson: ONLY pl.v_angle yaw goes to REAL for the render
				// window - that is what the HUD radar blip reads.
				// hands/viewmodel follow the eye/camera, so m_angEyeAngles
				// and abs angles are deliberately left alone: hands stay
				// static where YOU look, radar logic never touches them.
				// pitch/roll stay fake too - radar is 2D, only yaw matters.
				// save once per window (guard double START without END).
				if( !g_ServerAnimations.m_bRadarVAngleSwapped ) {
					g_ServerAnimations.m_angRadarSavedVAngle = pLocal->pl( ).v_angle;
					g_ServerAnimations.m_flRadarSavedRotationYaw = uAngRotationOffset
						? *( float * )( uintptr_t( pLocal ) + uAngRotationOffset + sizeof( float ) )
						: flRealYaw;
					g_ServerAnimations.m_bRadarVAngleSwapped = true;
				}

				// pure real for the CalcView delta (see header comment).
				g_ServerAnimations.m_flRadarSwapRealYaw = flRealYaw;

				pLocal->pl( ).v_angle.y = flRealYaw;

				// scoreboard radar path (prediction field, invisible in
				// world - does not move hands/camera). restored in RENDER_END.
				if( uAngRotationOffset ) {
					*( float * )( uintptr_t( pLocal ) + uAngRotationOffset + sizeof( float ) ) = flRealYaw;
				}
			}
		}

		g_WeatherController.UpdateWeather( );
		g_Misc.Modulation( );

		// this could be shit performance wise
		g_PlayerList.UpdatePlayerList( );

		if( g_pEngine->IsConnected( ) ) {
			g_Misc.Clantag( );
			g_Misc.RemoveVisualEffects( );

		#if defined(SERVER_HITBOXES) || defined(TEST_ANIMS)|| defined(TEST_EXTRAP)

		#if defined(SERVER_HITBOXES)
			if( g_pInput->m_fCameraInThirdPerson ) {
				DrawServerHitboxes( pLocal->EntIndex( ) );
			}
		#endif

		#if defined(TEST_ANIMS) || defined(TEST_EXTRAP)
			for( int i = 1; i <= 64; ++i ) {
				if( i == g_pEngine->GetLocalPlayer( ) )
					continue;

				auto pEntity = C_CSPlayer::GetPlayerByIndex( i );
				if( !pEntity )
					continue;

				DrawServerHitboxes( i );

			#ifdef TEST_ANIMS
				auto pAnimation = g_Animations.GetAnimationEntry( i );
				if( pAnimation && !pAnimation->m_deqRecords.empty( ) ) {

					/*if( pAnimation->m_deqRecords.front( ).m_iChosenMatrix == 1 )
						pEntity->DrawHitboxMatrix( pAnimation->m_deqRecords.front( ).m_sAnims[ ESides::SIDE_OPPOSITER ].m_pMatrix, nullptr, g_pGlobalVars->interval_per_tick / 4.f, Color( 255, 0, 0, 100 ) );
					else if( pAnimation->m_deqRecords.front( ).m_iChosenMatrix == 2 )
						pEntity->DrawHitboxMatrix( pAnimation->m_deqRecords.front( ).m_sAnims[ ESides::SIDE_OPPOSITEL ].m_pMatrix, nullptr, g_pGlobalVars->interval_per_tick / 4.f, Color( 0, 255, 0, 100 ) );
					else if( pAnimation->m_deqRecords.front( ).m_iChosenMatrix == 3 )
						pEntity->DrawHitboxMatrix( pAnimation->m_deqRecords.front( ).m_sAnims[ ESides::SIDE_MIDDLE ].m_pMatrix, nullptr, g_pGlobalVars->interval_per_tick / 4.f, Color( 255, 255, 0, 100 ) );*/

					//	pEntity->DrawHitboxMatrix( pAnimation->m_deqRecords.front( ).m_sAnims[ ESides::SIDE_OPPOSITEL ].m_pMatrix, nullptr, g_pGlobalVars->interval_per_tick / 4.f, Color( 0, 255, 0, 100 ) );
					//	pEntity->DrawHitboxMatrix( pAnimation->m_deqRecords.front( ).m_sAnims[ ESides::SIDE_OPPOSITER ].m_pMatrix, nullptr, g_pGlobalVars->interval_per_tick / 4.f, Color( 255, 0, 0, 100 ) );
					pEntity->DrawHitboxMatrix( pAnimation->m_deqRecords.front( ).m_sAnims[ ESides::SIDE_SERVER ].m_pMatrix, nullptr, g_pGlobalVars->interval_per_tick / 4.f );
				}

			#endif	
			}
		#endif

		#endif
		}
	}

	if( stage == FRAME_NET_UPDATE_END ) {
		g_pEngine->FireEvents( );
	}

	g_Prediction.fix_viewmodel( stage );

	// firstperson radar fix, part 2: the frame is drawn, give the FAKE back.
	// rendering (radar included) happened between RENDER_START and RENDER_END,
	// so it already saw REAL. restore before the original END handler to keep
	// prediction / camera / desync on fake. if the feature got toggled off,
	// died or disconnected mid-window, still restore to avoid a stuck real.
	// NOTE: eye/abs are never touched in firstperson on purpose - hands stay
	// static where you look, only v_angle (+scoreboard rotation) is swapped.
	if( stage == FRAME_RENDER_END && g_ServerAnimations.m_bRadarVAngleSwapped ) {
		if( pLocal && !pLocal->IsDead( ) && g_pEngine->IsInGame( ) ) {
			pLocal->pl( ).v_angle = g_ServerAnimations.m_angRadarSavedVAngle;

			static unsigned int uAngRotationRestoreOffset = 0u;
			if( !uAngRotationRestoreOffset ) {
				if( const auto pMap = pLocal->GetPredDescMap( ) )
					uAngRotationRestoreOffset = Memory::FindInDataMap( pMap, XorStr( "m_angRotation" ) );
			}

			if( uAngRotationRestoreOffset ) {
				*( float * )( uintptr_t( pLocal ) + uAngRotationRestoreOffset + sizeof( float ) ) = g_ServerAnimations.m_flRadarSavedRotationYaw;
			}
		}

		g_ServerAnimations.m_bRadarVAngleSwapped = false;
	}

	oFrameStageNotify( ecx, stage );

#if defined(LUA_SCRIPTING)
	if( !g_pEngine->IsDrawingLoadingImage( ) && pLocal ) {
		Scripting::Script::DoCallback( hash_32_fnv1a_const( XorStr( "post_frame_stage" ) ), stage );
	}
#endif

	// NEVER MOVE THIS, IT'S PERFECT HERE.
	if( stage == FRAME_NET_UPDATE_END ) {
		// g_Prediction.ApplyCompressionNetvars( );

		g_Animations.OnFrameStageNotify( );

	#ifdef TEST_EXTRAP

		for( int i = 1; i <= 64; ++i ) {
			if( i == g_pEngine->GetLocalPlayer( ) )
				continue;

			auto pEntity = C_CSPlayer::GetPlayerByIndex( i );
			if( !pEntity )
				continue;

			auto pData = g_AnimationSystem.GetAnimationData( i );
			if( !pData )
				continue;

			if( pEntity->m_iTeamNum( ) == pLocal->m_iTeamNum( ) )
				continue;

			auto pNetChannel = g_pEngine->GetNetChannelInfo( );
			if( !pNetChannel )
				continue;

			//LagRecord_t *record = &pData->m_deqRecords[ 0 ];
			//LagRecord_t *extrap = record;

			/*int nServerLatencyTicks = TIME_TO_TICKS( pNetChannel->GetLatency( FLOW_OUTGOING ) );
			int nArrivalTick = g_pClientState->m_ClockDriftManager( ).m_nServerTick + 1 + nServerLatencyTicks;

			if( pEntity->m_flSimulationTime( ) != pEntity->m_flOldSimulationTime( ) ) {
				printf( "predicted next arrival tick: %i", nArrivalTick );
				printf( "update simulation time tick: %i", g_pClientState->m_ClockDriftManager( ).m_nServerTick );
			}*/
		}

	#endif

		g_pEngine->FireEvents( );

		g_ShotHandling.ProcessShots( );
	}

	if( stage == FRAME_RENDER_START ) {
		if( pLocal ) {
			using AttachmentHelper_t = void( __thiscall * )( C_CSPlayer *, CStudioHdr * );
			static AttachmentHelper_t AttachmentHelperFn = ( AttachmentHelper_t )Engine::Displacement.Function.m_AttachmentHelper;

			if( const auto pStudioHdr = pLocal->m_pStudioHdr( ); pStudioHdr ) {
				AttachmentHelperFn( pLocal, pStudioHdr );
			}
		}
	}

}