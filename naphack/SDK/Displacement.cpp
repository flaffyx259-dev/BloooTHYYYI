#include "Displacement.hpp"
#include "Classes/PropManager.hpp"
#include "../Utils/extern/FnvHash.hpp"

DllInitializeData Engine::Displacement{ };

namespace Engine {
	__forceinline uintptr_t CallableFromRelative( DWORD nAddress ) {
		return nAddress + *( DWORD * )( nAddress + 1 ) + 5;
	}

	void Create( ) {
		auto image_vstdlib = GetModuleHandleA( XorStr( "vstdlib.dll" ) );
		auto image_client = ( std::uintptr_t )GetModuleHandleA( XorStr( "client.dll" ) );
		auto image_engine = ( std::uintptr_t )GetModuleHandleA( XorStr( "engine.dll" ) );
		auto image_server = ( std::uintptr_t )GetModuleHandleA( XorStr( "server.dll" ) );
		auto image_shaderapidx9 = ( std::uintptr_t )GetModuleHandleA( XorStr( "shaderapidx9.dll" ) );

		// TODO: datamap
		Displacement.C_BaseEntity.m_MoveType = g_PropManager.GetOffset( XorStr( "DT_BaseEntity" ), XorStr( "m_nRenderMode" ) ) + 1;
		Displacement.C_BaseEntity.m_rgflCoordinateFrame = g_PropManager.GetOffset( XorStr( "DT_BaseEntity" ), XorStr( "m_CollisionGroup" ) ) - 0x30;

		Displacement.DT_BaseEntity.m_iTeamNum = g_PropManager.GetOffset( XorStr( "DT_BaseEntity" ), XorStr( "m_iTeamNum" ) );
		Displacement.DT_BaseEntity.m_vecOrigin = g_PropManager.GetOffset( XorStr( "DT_BaseEntity" ), XorStr( "m_vecOrigin" ) );
		Displacement.DT_BaseEntity.m_flSimulationTime = g_PropManager.GetOffset( XorStr( "DT_BaseEntity" ), XorStr( "m_flSimulationTime" ) );
		Displacement.DT_BaseEntity.m_fEffects = g_PropManager.GetOffset( XorStr( "DT_BaseEntity" ), XorStr( "m_fEffects" ) );
		Displacement.DT_BaseEntity.m_iEFlags = Displacement.DT_BaseEntity.m_fEffects - 0x8;
		Displacement.DT_BaseEntity.m_hOwnerEntity = g_PropManager.GetOffset( XorStr( "DT_BaseEntity" ), XorStr( "m_hOwnerEntity" ) );
		Displacement.DT_BaseEntity.moveparent = g_PropManager.GetOffset( XorStr( "DT_BaseEntity" ), XorStr( "moveparent" ) );
		Displacement.DT_BaseEntity.m_nModelIndex = g_PropManager.GetOffset( XorStr( "DT_BaseEntity" ), XorStr( "m_nModelIndex" ) );
		Displacement.DT_BaseEntity.m_Collision = g_PropManager.GetOffset( XorStr( "DT_BaseEntity" ), XorStr( "m_Collision" ) );
		Displacement.DT_BaseEntity.m_Particles = g_PropManager.GetOffset( XorStr( "DT_BaseEntity" ), XorStr( "m_Particles" ) );
		Displacement.DT_BaseEntity.m_CollisionGroup = g_PropManager.GetOffset( XorStr( "DT_BaseEntity" ), XorStr( "m_CollisionGroup" ) );
		Displacement.DT_BaseEntity.m_flAnimTime = g_PropManager.GetOffset( XorStr( "DT_BaseEntity" ), XorStr( "m_flAnimTime" ) );

		Displacement.DT_BaseWeaponWorldModel.m_hCombatWeaponParent = g_PropManager.GetOffset( XorStr( "DT_BaseWeaponWorldModel" ), XorStr( "m_hCombatWeaponParent" ) );

		auto m_nForceBone = g_PropManager.GetOffset( XorStr( "DT_BaseAnimating" ), XorStr( "m_nForceBone" ) );
		auto m_hLightingOrigin = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_hLightingOrigin" ) );

	#ifdef DEV
		Displacement.C_BaseAnimating.InvalidateBoneCache = Memory::Scan( image_client, XorStr( "80 3D ? ? ? ? ? 74 16 A1 ? ? ? ? 48 C7 81" ) );

	#else
		Displacement.C_BaseAnimating.InvalidateBoneCache = Memory::Scan( image_client, XorStr( "80 3D ? ? ? ? ? 74 16 A1 ? ? ? ? 48 C7 81" ) );

	#endif		
		Displacement.C_BaseAnimating.m_BoneAccessor = m_nForceBone + 0x1C;
		Displacement.C_BaseAnimating.m_iPrevBoneMask = m_nForceBone + 0x10;
		Displacement.C_BaseAnimating.m_iAccumulatedBoneMask = m_nForceBone + 0x14;
		Displacement.C_BaseAnimating.m_bIsJiggleBonesEnabled = m_hLightingOrigin - 0x18;
		Displacement.C_BaseAnimating.m_iOcclusionFramecount = 0xA30;
		Displacement.C_BaseAnimating.m_iOcclusionFlags = 0xA28;
		Displacement.C_BaseAnimating.m_iMostRecentModelBoneCounter = *( int * )( Displacement.C_BaseAnimating.InvalidateBoneCache + 0x1B );
		Displacement.C_BaseAnimating.m_flLastBoneSetupTime = *( int * )( Displacement.C_BaseAnimating.InvalidateBoneCache + 0x11 );

	#ifdef DEV
		Displacement.C_BaseAnimating.m_CachedBoneData = *( int * )( Memory::Scan( image_client, XorStr( "FF B7 ? ? ? ? 52" ) ) + 2 ) + 0x4;

	#else
		Displacement.C_BaseAnimating.m_CachedBoneData = *( int * )( Memory::Scan( image_client, XorStr( "FF B7 ? ? ? ? 52" ) ) + 2 ) + 0x4;

	#endif
	#ifdef DEV
		Displacement.C_BaseAnimating.m_AnimOverlay = *( int * )( Memory::Scan( image_client, XorStr( "8B 89 ? ? ? ? 8D 0C D1" ) ) + 2 );

	#else
		Displacement.C_BaseAnimating.m_AnimOverlay = *( int * )( Memory::Scan( image_client, XorStr( "8B 89 ? ? ? ? 8D 0C D1" ) ) + 2 );

	#endif

	#ifdef DEV
		auto BoneSnapshotsCall = Memory::Scan( image_client, XorStr( "8D 8F ? ? ? ? 6A 01 C7 87" ) ); // 8D 8F ? ? ? ? 6A 01 C7 87

	#else
		auto BoneSnapshotsCall = Memory::Scan( image_client, XorStr( "8D 8F ? ? ? ? 6A 01 C7 87" ) ); // 8D 8F ? ? ? ? 6A 01 C7 87

	#endif

		Displacement.C_BaseAnimating.m_pFirstBoneSnapshot = *( int * )( BoneSnapshotsCall + 0x2 );
		Displacement.C_BaseAnimating.m_pSecondBoneSnapshot = *( int * )( BoneSnapshotsCall + 0x1B );


	#ifdef DEV
		auto CacheBoneData = Memory::Scan( image_client, XorStr( "8D 87 ? ? ? ? 50 E8 ? ? ? ? 8B 06" ) );

	#else
		auto CacheBoneData = Memory::Scan( image_client, XorStr( "8D 87 ? ? ? ? 50 E8 ? ? ? ? 8B 06" ) );

	#endif		
		Displacement.C_BaseAnimating.m_nCachedBonesPosition = *( int * )( CacheBoneData + 0x2 ) + 0x4;
		Displacement.C_BaseAnimating.m_nCachedBonesRotation = *( int * )( CacheBoneData + 0x25 ) + 0x4;

	#ifdef DEV
		Displacement.C_BaseAnimating.m_pStudioHdr = *( int * )( Memory::Scan( image_client, XorStr( "8B B7 ? ? ? ? 89 74 24 20" ) ) + 0x2 ) + 0x4;

	#else
		Displacement.C_BaseAnimating.m_pStudioHdr = *( int * )( Memory::Scan( image_client, XorStr( "8B B7 ? ? ? ? 89 74 24 20" ) ) + 0x2 ) + 0x4;

	#endif
	#ifdef DEV
		Displacement.C_BaseAnimating.m_bShouldDraw = *( int * )( Memory::Scan( image_client, XorStr( "FF 15 ? ? ? ? 80 BE ? ? ? ? ? 0F 84" ) ) + 0x8 );

	#else
		Displacement.C_BaseAnimating.m_bShouldDraw = *( int * )( Memory::Scan( image_client, XorStr( "FF 15 ? ? ? ? 80 BE ? ? ? ? ? 0F 84" ) ) + 0x8 );

	#endif
	#ifdef DEV
		Displacement.C_BaseAnimating.m_pBoneMerge = *( int * )( Memory::Scan( image_client, XorStr( "89 86 ? ? ? ? E8 ? ? ? ? FF 75 08" ) ) + 2 );

	#else
		Displacement.C_BaseAnimating.m_pBoneMerge = *( int * )( Memory::Scan( image_client, XorStr( "89 86 ? ? ? ? E8 ? ? ? ? FF 75 08" ) ) + 2 );

	#endif
	#ifdef DEV
		Displacement.C_BaseAnimating.m_pIk = *( int * )( Memory::Scan( image_client, XorStr( "8B 8F ? ? ? ? 89 4C 24 1C" ) ) + 2 ) + 4;

	#else
		Displacement.C_BaseAnimating.m_pIk = *( int * )( Memory::Scan( image_client, XorStr( "8B 8F ? ? ? ? 89 4C 24 1C" ) ) + 2 ) + 4;

	#endif
		Displacement.DT_BaseAnimating.m_bClientSideAnimation = g_PropManager.GetOffset( XorStr( "DT_BaseAnimating" ), XorStr( "m_bClientSideAnimation" ) );
		Displacement.DT_BaseAnimating.m_bClientSideRagdoll = g_PropManager.GetOffset( XorStr( "DT_BaseAnimating" ), XorStr( "m_bClientSideRagdoll" ) );
		Displacement.DT_BaseAnimating.m_flPoseParameter = g_PropManager.GetOffset( XorStr( "DT_BaseAnimating" ), XorStr( "m_flPoseParameter" ) );
		Displacement.DT_BaseAnimating.m_nHitboxSet = g_PropManager.GetOffset( XorStr( "DT_BaseAnimating" ), XorStr( "m_nHitboxSet" ) );
		Displacement.DT_BaseAnimating.m_bIsLookingAtWeapon = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_bIsLookingAtWeapon" ) );
		Displacement.DT_BaseAnimating.m_flCycle = g_PropManager.GetOffset( XorStr( "DT_BaseAnimating" ), XorStr( "m_flCycle" ) );
		Displacement.DT_BaseAnimating.m_nSequence = g_PropManager.GetOffset( XorStr( "DT_BaseAnimating" ), XorStr( "m_nSequence" ) );
		Displacement.DT_BaseAnimating.m_nSequence = g_PropManager.GetOffset( XorStr( "DT_BaseAnimating" ), XorStr( "m_nSequence" ) );
		Displacement.DT_BaseAnimating.m_flEncodedController = g_PropManager.GetOffset( XorStr( "DT_BaseAnimating" ), XorStr( "m_flEncodedController" ) );

		Displacement.DT_BaseCombatCharacter.m_hActiveWeapon = g_PropManager.GetOffset( XorStr( "DT_BaseCombatCharacter" ), XorStr( "m_hActiveWeapon" ) );
		Displacement.DT_BaseCombatCharacter.m_flNextAttack = g_PropManager.GetOffset( XorStr( "DT_BaseCombatCharacter" ), XorStr( "m_flNextAttack" ) );
		Displacement.DT_BaseCombatCharacter.m_hMyWeapons = g_PropManager.GetOffset( XorStr( "DT_BaseCombatCharacter" ), XorStr( "m_hMyWeapons" ) ) / 2;
		Displacement.DT_BaseCombatCharacter.m_hMyWearables = g_PropManager.GetOffset( XorStr( "DT_BaseCombatCharacter" ), XorStr( "m_hMyWearables" ) );


		Displacement.C_BasePlayer.m_pCurrentCommand = 0x3314; // https://i.imgur.com/ErliX2j.png

		auto relative_call = Memory::Scan( image_client, XorStr( "E8 ? ? ? ? 83 7D ? ? 7C ? A1 ? ? ? ? FF 75" ), false );

		auto offset = *( uintptr_t * )( relative_call + 0x1 );
		Displacement.C_BasePlayer.UpdateVisibilityAllEntities = ( DWORD32 )( relative_call + 5 + offset );

		Displacement.DT_BasePlayer.m_aimPunchAngle = g_PropManager.GetOffset( XorStr( "DT_BasePlayer" ), XorStr( "m_aimPunchAngle" ) );
		Displacement.DT_BasePlayer.m_aimPunchAngleVel = g_PropManager.GetOffset( XorStr( "DT_BasePlayer" ), XorStr( "m_aimPunchAngleVel" ) );
		Displacement.DT_BasePlayer.m_viewPunchAngle = g_PropManager.GetOffset( XorStr( "DT_BasePlayer" ), XorStr( "m_viewPunchAngle" ) );
		Displacement.DT_BasePlayer.m_vecViewOffset = g_PropManager.GetOffset( XorStr( "DT_BasePlayer" ), XorStr( "m_vecViewOffset[0]" ) );
		Displacement.DT_BasePlayer.m_vecVelocity = g_PropManager.GetOffset( XorStr( "DT_BasePlayer" ), XorStr( "m_vecVelocity[0]" ) );
		Displacement.DT_BasePlayer.m_vecBaseVelocity = g_PropManager.GetOffset( XorStr( "DT_BasePlayer" ), XorStr( "m_vecBaseVelocity" ) );
		Displacement.DT_BasePlayer.m_vecLadderNormal = g_PropManager.GetOffset( XorStr( "DT_BasePlayer" ), XorStr( "m_vecLadderNormal" ) );
		Displacement.DT_BasePlayer.m_flFallVelocity = g_PropManager.GetOffset( XorStr( "DT_BasePlayer" ), XorStr( "m_flFallVelocity" ) );
		Displacement.DT_BasePlayer.m_flDuckAmount = g_PropManager.GetOffset( XorStr( "DT_BasePlayer" ), XorStr( "m_flDuckAmount" ) );
		Displacement.DT_BasePlayer.m_flDuckSpeed = g_PropManager.GetOffset( XorStr( "DT_BasePlayer" ), XorStr( "m_flDuckSpeed" ) );
		Displacement.DT_BasePlayer.m_lifeState = g_PropManager.GetOffset( XorStr( "DT_BasePlayer" ), XorStr( "m_lifeState" ) );
		Displacement.DT_BasePlayer.m_nTickBase = g_PropManager.GetOffset( XorStr( "DT_BasePlayer" ), XorStr( "m_nTickBase" ) );
		Displacement.DT_BasePlayer.m_iHealth = g_PropManager.GetOffset( XorStr( "DT_BasePlayer" ), XorStr( "m_iHealth" ) );
		Displacement.DT_BasePlayer.m_iDefaultFOV = g_PropManager.GetOffset( XorStr( "DT_BasePlayer" ), XorStr( "m_iDefaultFOV" ) );
		Displacement.DT_BasePlayer.m_fFlags = g_PropManager.GetOffset( XorStr( "DT_BasePlayer" ), XorStr( "m_fFlags" ) );
		Displacement.DT_BasePlayer.m_bSpotted = g_PropManager.GetOffset( XorStr( "DT_BaseEntity" ), XorStr( "m_bSpotted" ) );
		Displacement.DT_BasePlayer.m_iObserverMode = g_PropManager.GetOffset( XorStr( "DT_BasePlayer" ), XorStr( "m_iObserverMode" ) );
		Displacement.DT_BasePlayer.pl = g_PropManager.GetOffset( XorStr( "DT_BasePlayer" ), XorStr( "pl" ) );
		Displacement.DT_BasePlayer.m_hObserverTarget = g_PropManager.GetOffset( XorStr( "DT_BasePlayer" ), XorStr( "m_hObserverTarget" ) );
		Displacement.DT_BasePlayer.m_hViewModel = g_PropManager.GetOffset( XorStr( "DT_BasePlayer" ), XorStr( "m_hViewModel[0]" ) );
		Displacement.DT_BasePlayer.m_ubEFNoInterpParity = g_PropManager.GetOffset( XorStr( "DT_BasePlayer" ), XorStr( "m_ubEFNoInterpParity" ) );

	#ifdef DEV
		Displacement.DT_BasePlayer.m_ubOldEFNoInterpParity = *( int * )( Memory::Scan( image_client, XorStr( "8A 87 ? ? ? ? 8D 5F F8" ) ) + 2 ) + 8;

	#else
		Displacement.DT_BasePlayer.m_ubOldEFNoInterpParity = *( int * )( Memory::Scan( image_client, XorStr( "8A 87 ? ? ? ? 8D 5F F8" ) ) + 2 ) + 8;

	#endif		
		Displacement.DT_BasePlayer.m_hGroundEntity = g_PropManager.GetOffset( XorStr( "DT_BasePlayer" ), XorStr( "m_hGroundEntity" ) );

		Displacement.DT_CSGameRulesProxy.m_bIsValveDS = g_PropManager.GetOffset( XorStr( "DT_CSGameRulesProxy" ), XorStr( "m_bIsValveDS" ) );
		Displacement.DT_CSGameRulesProxy.m_bFreezePeriod = g_PropManager.GetOffset( XorStr( "DT_CSGameRulesProxy" ), XorStr( "m_bFreezePeriod" ) );

		Displacement.DT_CSPlayerResource.m_bombsiteCenterA = g_PropManager.GetOffset( XorStr( "DT_CSPlayerResource" ), XorStr( "m_bombsiteCenterA" ) );
		Displacement.DT_CSPlayerResource.m_bombsiteCenterB = g_PropManager.GetOffset( XorStr( "DT_CSPlayerResource" ), XorStr( "m_bombsiteCenterB" ) );


	#ifdef DEV
		Displacement.C_CSPlayer.m_PlayerAnimState = *( int * )( Memory::Scan( image_client, XorStr( "8B 8E ? ? ? ? 85 C9 74 3E" ) ) + 2 );

	#else
		Displacement.C_CSPlayer.m_PlayerAnimState = *( int * )( Memory::Scan( image_client, XorStr( "8B 8E ? ? ? ? 85 C9 74 3E" ) ) + 2 );

	#endif

	#ifdef DEV
		Displacement.C_CSPlayer.m_flSpawnTime = *( int * )( Memory::Scan( image_client, XorStr( "89 86 ? ? ? ? E8 ? ? ? ? 80 BE" ) ) + 2 );

	#else
		Displacement.C_CSPlayer.m_flSpawnTime = *( int * )( Memory::Scan( image_client, XorStr( "89 86 ? ? ? ? E8 ? ? ? ? 80 BE" ) ) + 2 );

	#endif
	#ifdef DEV
		Displacement.C_CSPlayer.m_utlClientImpactList = *( int * )( Memory::Scan( image_client, XorStr( "8D 8F ? ? ? ? F3 0F 10 84 24" ) ) + 2 );
	#else
		Displacement.C_CSPlayer.m_utlClientImpactList = *( int * )( Memory::Scan( image_client, XorStr( "8D 8F ? ? ? ? F3 0F 10 84 24" ) ) + 2 );
	#endif

		Displacement.DT_CSPlayer.m_angEyeAngles = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_angEyeAngles[0]" ) );
		Displacement.DT_CSPlayer.m_nSurvivalTeam = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_nSurvivalTeam" ) );
		Displacement.DT_CSPlayer.m_bHasHelmet = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_bHasHelmet" ) );
		Displacement.DT_CSPlayer.m_bHasHeavyArmor = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_bHasHeavyArmor" ) );
		Displacement.DT_CSPlayer.m_ArmorValue = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_ArmorValue" ) );
		Displacement.DT_CSPlayer.m_bScoped = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_bIsScoped" ) );
		Displacement.DT_CSPlayer.m_bIsWalking = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_bIsWalking" ) );
		Displacement.DT_CSPlayer.m_flThirdpersonRecoil = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_flThirdpersonRecoil" ) );
		Displacement.DT_CSPlayer.m_iAccount = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_iAccount" ) );
		Displacement.DT_CSPlayer.m_iShotsFired = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_iShotsFired" ) );
		Displacement.DT_CSPlayer.m_flFlashDuration = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_flFlashDuration" ) );
		Displacement.DT_CSPlayer.m_flLowerBodyYawTarget = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_flLowerBodyYawTarget" ) );
		Displacement.DT_CSPlayer.m_flVelocityModifier = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_flVelocityModifier" ) );
		Displacement.DT_CSPlayer.m_flStamina = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_flStamina" ) );
		Displacement.DT_CSPlayer.m_bDuckOverride = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_bDuckOverride" ) );
		Displacement.DT_CSPlayer.m_bGunGameImmunity = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_bGunGameImmunity" ) );
		Displacement.DT_CSPlayer.m_flHealthShotBoostExpirationTime = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_flHealthShotBoostExpirationTime" ) );
		Displacement.DT_CSPlayer.m_iMatchStats_Kills = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_iMatchStats_Kills" ) );
		Displacement.DT_CSPlayer.m_iMatchStats_Deaths = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_iMatchStats_Deaths" ) );
		Displacement.DT_CSPlayer.m_iMatchStats_HeadShotKills = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_iMatchStats_HeadShotKills" ) );
		Displacement.DT_CSPlayer.m_iMoveState = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_iMoveState" ) );
		Displacement.DT_CSPlayer.m_bWaitForNoAttack = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_bWaitForNoAttack" ) );

	#ifdef DEV
		Displacement.DT_CSPlayer.m_bCustomPlayer = *( int * )( Memory::Scan( image_client, XorStr( "80 BF ? ? ? ? ? 0F 84 ? ? ? ? 83 BF ? ? ? ? ? 74 7C" ) ) + 2 ); //80 BF ? ? ? ? ? 0F 84 ? ? ? ? 83 BF ? ? ? ? ? 74 7C

	#else
		Displacement.DT_CSPlayer.m_bCustomPlayer = *( int * )( Memory::Scan( image_client, XorStr( "80 BF ? ? ? ? ? 0F 84 ? ? ? ? 83 BF ? ? ? ? ? 74 7C" ) ) + 2 ); //80 BF ? ? ? ? ? 0F 84 ? ? ? ? 83 BF ? ? ? ? ? 74 7C

	#endif		
		Displacement.DT_CSPlayer.m_iPlayerState = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_iPlayerState" ) );
		Displacement.DT_CSPlayer.m_bIsDefusing = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_bIsDefusing" ) );
		Displacement.DT_CSPlayer.m_hCarriedHostage = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_hCarriedHostage" ) );
		Displacement.DT_CSPlayer.m_bHasDefuser = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_bHasDefuser" ) );
		Displacement.DT_CSPlayer.m_iFOV = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_iFOV" ) );
		Displacement.DT_CSPlayer.m_bIsPlayerGhost = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_bIsPlayerGhost" ) );
		Displacement.DT_CSPlayer.m_vecPlayerPatchEconIndices = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_vecPlayerPatchEconIndices" ) );
		Displacement.DT_CSPlayer.m_hRagdoll = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_hRagdoll" ) );
		Displacement.DT_CSPlayer.m_bInNoDefuseArea = g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_bInNoDefuseArea" ) );

		Displacement.DT_CSRagdoll.m_hPlayer = g_PropManager.GetOffset( XorStr( "DT_CSRagdoll" ), XorStr( "m_hPlayer" ) );

		Displacement.DT_FogController.m_fog_enable = g_PropManager.GetOffset( XorStr( "DT_FogController" ), XorStr( "m_fog.enable" ) );
		Displacement.DT_Precipitation.m_nPrecipType = g_PropManager.GetOffset( XorStr( "DT_Precipitation" ), XorStr( "m_nPrecipType" ) );

		Displacement.DT_BaseCombatWeapon.m_flNextPrimaryAttack = g_PropManager.GetOffset( XorStr( "DT_BaseCombatWeapon" ), XorStr( "m_flNextPrimaryAttack" ) );
		Displacement.DT_BaseCombatWeapon.m_flNextSecondaryAttack = g_PropManager.GetOffset( XorStr( "DT_BaseCombatWeapon" ), XorStr( "m_flNextSecondaryAttack" ) );
		Displacement.DT_BaseCombatWeapon.m_hOwner = g_PropManager.GetOffset( XorStr( "DT_BaseCombatWeapon" ), XorStr( "m_hOwner" ) );
		Displacement.DT_BaseCombatWeapon.m_iClip1 = g_PropManager.GetOffset( XorStr( "DT_BaseCombatWeapon" ), XorStr( "m_iClip1" ) );
		Displacement.DT_BaseCombatWeapon.m_iPrimaryReserveAmmoCount = g_PropManager.GetOffset( XorStr( "DT_BaseCombatWeapon" ), XorStr( "m_iPrimaryReserveAmmoCount" ) );
		Displacement.DT_BaseCombatWeapon.m_iItemDefinitionIndex = g_PropManager.GetOffset( XorStr( "DT_BaseCombatWeapon" ), XorStr( "m_iItemDefinitionIndex" ) );
		Displacement.DT_BaseCombatWeapon.m_hWeaponWorldModel = g_PropManager.GetOffset( XorStr( "DT_BaseCombatWeapon" ), XorStr( "m_hWeaponWorldModel" ) );
		Displacement.DT_BaseCombatWeapon.m_iWorldModelIndex = g_PropManager.GetOffset( XorStr( "DT_BaseCombatWeapon" ), XorStr( "m_iWorldModelIndex" ) );
		Displacement.DT_BaseCombatWeapon.m_iWorldDroppedModelIndex = g_PropManager.GetOffset( XorStr( "DT_BaseCombatWeapon" ), XorStr( "m_iWorldDroppedModelIndex" ) );
		Displacement.DT_BaseCombatWeapon.m_iViewModelIndex = g_PropManager.GetOffset( XorStr( "DT_BaseCombatWeapon" ), XorStr( "m_iViewModelIndex" ) );


	#ifdef DEV
		Displacement.DT_BaseCombatWeapon.m_CustomMaterials = ( *( int * )( Memory::Scan( image_client, XorStr( "83 BE ? ? ? ? ? 7F 67" ) ) + 0x2 ) ) - 12;

	#else
		Displacement.DT_BaseCombatWeapon.m_CustomMaterials = ( *( int * )( Memory::Scan( image_client, XorStr( "83 BE ? ? ? ? ? 7F 67" ) ) + 0x2 ) ) - 12;

	#endif
	#ifdef DEV
		Displacement.DT_BaseCombatWeapon.m_bCustomMaterialInitialized = *( int * )( Memory::Scan( image_client, XorStr( "C6 86 ? ? ? ? ? FF 50 04" ) ) + 0x2 );

	#else
		Displacement.DT_BaseCombatWeapon.m_bCustomMaterialInitialized = *( int * )( Memory::Scan( image_client, XorStr( "C6 86 ? ? ? ? ? FF 50 04" ) ) + 0x2 );

	#endif
		Displacement.DT_WeaponCSBase.m_flRecoilIndex = g_PropManager.GetOffset( XorStr( "DT_WeaponCSBase" ), XorStr( "m_flRecoilIndex" ) );
		Displacement.DT_WeaponCSBase.m_weaponMode = g_PropManager.GetOffset( XorStr( "DT_WeaponCSBase" ), XorStr( "m_weaponMode" ) );
		Displacement.DT_WeaponCSBase.m_flPostponeFireReadyTime = g_PropManager.GetOffset( XorStr( "DT_WeaponCSBase" ), XorStr( "m_flPostponeFireReadyTime" ) );
		Displacement.DT_WeaponCSBase.m_fLastShotTime = g_PropManager.GetOffset( XorStr( "DT_WeaponCSBase" ), XorStr( "m_fLastShotTime" ) );
		Displacement.DT_WeaponCSBase.m_fAccuracyPenalty = g_PropManager.GetOffset( XorStr( "DT_WeaponCSBase" ), XorStr( "m_fAccuracyPenalty" ) );

		Displacement.DT_WeaponCSBaseGun.m_zoomLevel = g_PropManager.GetOffset( XorStr( "DT_WeaponCSBaseGun" ), XorStr( "m_zoomLevel" ) );
		Displacement.DT_WeaponCSBaseGun.m_iBurstShotsRemaining = g_PropManager.GetOffset( XorStr( "DT_WeaponCSBaseGun" ), XorStr( "m_iBurstShotsRemaining" ) );
		Displacement.DT_WeaponCSBaseGun.m_fNextBurstShot = g_PropManager.GetOffset( XorStr( "DT_WeaponCSBaseGun" ), XorStr( "m_fNextBurstShot" ) );

		Displacement.DT_BaseCSGrenade.m_bPinPulled = g_PropManager.GetOffset( XorStr( "DT_BaseCSGrenade" ), XorStr( "m_bPinPulled" ) );
		Displacement.DT_BaseCSGrenade.m_fThrowTime = g_PropManager.GetOffset( XorStr( "DT_BaseCSGrenade" ), XorStr( "m_fThrowTime" ) );
		Displacement.DT_BaseCSGrenade.m_flThrowStrength = g_PropManager.GetOffset( XorStr( "DT_BaseCSGrenade" ), XorStr( "m_flThrowStrength" ) );

		Displacement.DT_BaseAttributableItem.m_flFallbackWear = g_PropManager.GetOffset( XorStr( "DT_BaseAttributableItem" ), XorStr( "m_flFallbackWear" ) );
		Displacement.DT_BaseAttributableItem.m_nFallbackPaintKit = g_PropManager.GetOffset( XorStr( "DT_BaseAttributableItem" ), XorStr( "m_nFallbackPaintKit" ) );
		Displacement.DT_BaseAttributableItem.m_nFallbackSeed = g_PropManager.GetOffset( XorStr( "DT_BaseAttributableItem" ), XorStr( "m_nFallbackSeed" ) );
		Displacement.DT_BaseAttributableItem.m_nFallbackStatTrak = g_PropManager.GetOffset( XorStr( "DT_BaseAttributableItem" ), XorStr( "m_nFallbackStatTrak" ) );
		Displacement.DT_BaseAttributableItem.m_OriginalOwnerXuidHigh = g_PropManager.GetOffset( XorStr( "DT_BaseAttributableItem" ), XorStr( "m_OriginalOwnerXuidHigh" ) );
		Displacement.DT_BaseAttributableItem.m_OriginalOwnerXuidLow = g_PropManager.GetOffset( XorStr( "DT_BaseAttributableItem" ), XorStr( "m_OriginalOwnerXuidLow" ) );
		Displacement.DT_BaseAttributableItem.m_szCustomName = g_PropManager.GetOffset( XorStr( "DT_BaseAttributableItem" ), XorStr( "m_szCustomName" ) );
		Displacement.DT_BaseAttributableItem.m_bInitialized = g_PropManager.GetOffset( XorStr( "DT_BaseAttributableItem" ), XorStr( "m_bInitialized" ) );
		Displacement.DT_BaseAttributableItem.m_iAccountID = g_PropManager.GetOffset( XorStr( "DT_BaseAttributableItem" ), XorStr( "m_iAccountID" ) );
		Displacement.DT_BaseAttributableItem.m_iEntityLevel = g_PropManager.GetOffset( XorStr( "DT_BaseAttributableItem" ), XorStr( "m_iEntityLevel" ) );
		Displacement.DT_BaseAttributableItem.m_iEntityQuality = g_PropManager.GetOffset( XorStr( "DT_BaseAttributableItem" ), XorStr( "m_iEntityQuality" ) );
		Displacement.DT_BaseAttributableItem.m_iItemDefinitionIndex = g_PropManager.GetOffset( XorStr( "DT_BaseAttributableItem" ), XorStr( "m_iItemDefinitionIndex" ) );
		Displacement.DT_BaseAttributableItem.m_Item = g_PropManager.GetOffset( XorStr( "DT_BaseAttributableItem" ), XorStr( "m_Item" ) );
		Displacement.DT_BaseAttributableItem.m_iItemIDLow = g_PropManager.GetOffset( XorStr( "DT_BaseAttributableItem" ), XorStr( "m_iItemIDLow" ) );
		Displacement.DT_BaseAttributableItem.m_iItemIDHigh = g_PropManager.GetOffset( XorStr( "DT_BaseAttributableItem" ), XorStr( "m_iItemIDHigh" ) );

		Displacement.DT_BaseViewModel.m_hOwner = g_PropManager.GetOffset( XorStr( "DT_BaseViewModel" ), XorStr( "m_hOwner" ) );
		Displacement.DT_BaseViewModel.m_hWeapon = g_PropManager.GetOffset( XorStr( "DT_BaseViewModel" ), XorStr( "m_hWeapon" ) );
		Displacement.DT_BaseViewModel.m_nSequence = g_PropManager.GetOffset( XorStr( "DT_BaseViewModel" ), XorStr( "m_nSequence" ) );

		Displacement.DT_SmokeGrenadeProjectile.m_nSmokeEffectTickBegin = g_PropManager.GetOffset( XorStr( "DT_SmokeGrenadeProjectile" ), XorStr( "m_nSmokeEffectTickBegin" ) );
		Displacement.DT_SmokeGrenadeProjectile.m_bDidSmokeEffect = g_PropManager.GetOffset( XorStr( "DT_SmokeGrenadeProjectile" ), XorStr( "m_bDidSmokeEffect" ) );

	#ifdef DEV
		Displacement.CBoneMergeCache.m_nConstructor = Memory::Scan( image_client, XorStr( "56 8B F1 0F 57 C0 C7 86" ) );

	#else
		Displacement.CBoneMergeCache.m_nConstructor = Memory::Scan( image_client, XorStr( "56 8B F1 0F 57 C0 C7 86" ) );

	#endif
	#ifdef DEV
		Displacement.CBoneMergeCache.m_nInit = CallableFromRelative( Memory::Scan( image_client, XorStr( "E8 ? ? ? ? FF 75 08 8B 8E" ) ) );

	#else
		Displacement.CBoneMergeCache.m_nInit = CallableFromRelative( Memory::Scan( image_client, XorStr( "E8 ? ? ? ? FF 75 08 8B 8E" ) ) );

	#endif
	#ifdef DEV
		Displacement.CBoneMergeCache.m_nUpdateCache = CallableFromRelative( Memory::Scan( image_client, XorStr( "E8 ? ? ? ? 83 7E 10 00 74 64" ) ) );

	#else
		Displacement.CBoneMergeCache.m_nUpdateCache = CallableFromRelative( Memory::Scan( image_client, XorStr( "E8 ? ? ? ? 83 7E 10 00 74 64" ) ) );

	#endif
	#ifdef DEV
		Displacement.CBoneMergeCache.m_CopyToFollow = CallableFromRelative( Memory::Scan( image_client, XorStr( "E8 ? ? ? ? 8B 87 ? ? ? ? 8D 8C 24 ? ? ? ? 8B 7C 24 18" ) ) );

	#else
		Displacement.CBoneMergeCache.m_CopyToFollow = CallableFromRelative( Memory::Scan( image_client, XorStr( "E8 ? ? ? ? 8B 87 ? ? ? ? 8D 8C 24 ? ? ? ? 8B 7C 24 18" ) ) );

	#endif
	#ifdef DEV
		Displacement.CBoneMergeCache.m_CopyFromFollow = CallableFromRelative( Memory::Scan( image_client, XorStr( "E8 ? ? ? ? F3 0F 10 45 ? 8D 84 24" ) ) );

	#else
		Displacement.CBoneMergeCache.m_CopyFromFollow = CallableFromRelative( Memory::Scan( image_client, XorStr( "E8 ? ? ? ? F3 0F 10 45 ? 8D 84 24" ) ) );

	#endif

	#ifdef DEV
		Displacement.CIKContext.m_nConstructor = Memory::Scan( image_client, XorStr( "56 8B F1 6A 00 6A 00 C7" ) );

	#else
		Displacement.CIKContext.m_nConstructor = Memory::Scan( image_client, XorStr( "56 8B F1 6A 00 6A 00 C7" ) );

	#endif
	#ifdef DEV
		Displacement.CIKContext.m_nDestructor = Memory::Scan( image_client, XorStr( "56 8B F1 57 8D 8E ? ? ? ? E8 ? ? ? ? 8D 8E ? ? ? ? E8 ? ? ? ? 83 BE" ) );

	#else
		Displacement.CIKContext.m_nDestructor = Memory::Scan( image_client, XorStr( "56 8B F1 57 8D 8E ? ? ? ? E8 ? ? ? ? 8D 8E ? ? ? ? E8 ? ? ? ? 83 BE" ) );

	#endif
	#ifdef DEV
		Displacement.CIKContext.m_nInit = Memory::Scan( image_client, XorStr( "55 8B EC 83 EC 08 8B 45 08 56 57 8B F9 8D 8F" ) );

	#else
		Displacement.CIKContext.m_nInit = Memory::Scan( image_client, XorStr( "55 8B EC 83 EC 08 8B 45 08 56 57 8B F9 8D 8F" ) );

	#endif
	#ifdef DEV
		Displacement.CIKContext.m_nUpdateTargets = Memory::Scan( image_client, XorStr( "55 8B EC 83 E4 F0 81 EC ? ? ? ? 33 D2" ) );

	#else
		Displacement.CIKContext.m_nUpdateTargets = Memory::Scan( image_client, XorStr( "55 8B EC 83 E4 F0 81 EC ? ? ? ? 33 D2" ) );

	#endif
	#ifdef DEV
		Displacement.CIKContext.m_nSolveDependencies = Memory::Scan( image_client, XorStr( "55 8B EC 83 E4 F0 81 EC ? ? ? ? 8B 81" ) );

	#else
		Displacement.CIKContext.m_nSolveDependencies = Memory::Scan( image_client, XorStr( "55 8B EC 83 E4 F0 81 EC ? ? ? ? 8B 81" ) );

	#endif

	#ifdef DEV
		Displacement.CBoneSetup.InitPose = Memory::Scan( image_client, XorStr( "55 8B EC 83 EC 10 53 8B D9 89 55 F8 56 57 89 5D F4 8B 0B 89 4D F0" ) );

	#else
		Displacement.CBoneSetup.InitPose = Memory::Scan( image_client, XorStr( "55 8B EC 83 EC 10 53 8B D9 89 55 F8 56 57 89 5D F4 8B 0B 89 4D F0" ) );

	#endif
	#ifdef DEV
		Displacement.CBoneSetup.AccumulatePose = Memory::Scan( image_client, XorStr( "55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? A1" ) );

	#else
		Displacement.CBoneSetup.AccumulatePose = Memory::Scan( image_client, XorStr( "55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? A1" ) );

	#endif
	#ifdef DEV
		Displacement.CBoneSetup.CalcAutoplaySequences = Memory::Scan( image_client, XorStr( "55 8B EC 83 EC 10 53 56 57 8B 7D 10 8B D9 F3 0F 11 5D" ) );

	#else
		Displacement.CBoneSetup.CalcAutoplaySequences = Memory::Scan( image_client, XorStr( "55 8B EC 83 EC 10 53 56 57 8B 7D 10 8B D9 F3 0F 11 5D" ) );

	#endif
	#ifdef DEV
		Displacement.CBoneSetup.CalcBoneAdj = Memory::Scan( image_client, XorStr( "55 8B EC 83 E4 F8 81 EC ? ? ? ? 8B C1 89 54 24 04 89 44 24 2C 56 57 8B 00" ) );

	#else
		Displacement.CBoneSetup.CalcBoneAdj = Memory::Scan( image_client, XorStr( "55 8B EC 83 E4 F8 81 EC ? ? ? ? 8B C1 89 54 24 04 89 44 24 2C 56 57 8B 00" ) );

	#endif
		Displacement.IPrediction.m_nCommandsPredicted = 0x1C;

		// jmp patterns is not very reliable

	#ifdef DEV
		auto CL_Predict = Memory::Scan( image_engine, XorStr( "75 30 8B 87" ) );

	#else
		auto CL_Predict = Memory::Scan( image_engine, XorStr( "75 30 8B 87" ) );

	#endif
	#ifdef DEV
		auto CL_Move = Memory::Scan( image_engine, XorStr( "74 0F 80 BF" ) );

	#else
		auto CL_Move = Memory::Scan( image_engine, XorStr( "74 0F 80 BF" ) );
	#endif	

		Displacement.CClientState.m_nLastCommandAck = *( int * )( CL_Predict + 0x20 );
		Displacement.CClientState.m_nDeltaTick = *( int * )( CL_Predict + 0x10 );
		Displacement.CClientState.m_nLastOutgoingCommand = *( int * )( CL_Predict + 0xA );
		Displacement.CClientState.m_nChokedCommands = *( int * )( CL_Predict + 0x4 );
		Displacement.CClientState.m_bIsHLTV = *( int * )( CL_Move + 0x4 );
		Displacement.CClientState.m_nMaxClients = 0x0388;

		Displacement.DT_PlantedC4.m_flC4Blow = g_PropManager.GetOffset( XorStr( "DT_PlantedC4" ), XorStr( "m_flC4Blow" ) );
		Displacement.DT_PlantedC4.m_flDefuseCountDown = g_PropManager.GetOffset( XorStr( "DT_PlantedC4" ), XorStr( "m_flDefuseCountDown" ) );
		Displacement.DT_PlantedC4.m_bBombDefused = g_PropManager.GetOffset( XorStr( "DT_PlantedC4" ), XorStr( "m_bBombDefused" ) );

	#ifdef DEV
		Displacement.Data.m_uMoveHelper = **( std::uintptr_t ** )( Memory::Scan( image_client, XorStr( "8B 0D ? ? ? ? 8B 46 08 68" ) ) + 2 );

	#else
		Displacement.Data.m_uMoveHelper = **( std::uintptr_t ** )( Memory::Scan( image_client, XorStr( "8B 0D ? ? ? ? 8B 46 08 68" ) ) + 2 );

	#endif
	#ifdef DEV
		Displacement.Data.m_uInput = *( std::uintptr_t * )( Memory::Scan( image_client, XorStr( "B9 ? ? ? ? F3 0F 11 04 24 FF 50 10" ) ) + 1 );

	#else
		Displacement.Data.m_uInput = *( std::uintptr_t * )( Memory::Scan( image_client, XorStr( "B9 ? ? ? ? F3 0F 11 04 24 FF 50 10" ) ) + 1 );

	#endif		
		Displacement.Data.m_uModelBoneCounter = *( std::uintptr_t * )( Displacement.C_BaseAnimating.InvalidateBoneCache + 0xA );

	#ifdef DEV
		Engine::Displacement.Function.m_ClipRayToHitbox = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 83 E4 F8 F3 0F 10 42" ) );
	#else
		Engine::Displacement.Function.m_ClipRayToHitbox = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 83 E4 F8 F3 0F 10 42" ) );
	#endif

	#ifdef DEV
		Displacement.Data.m_uClientSideAnimationList = *( std::uintptr_t * )( Memory::Scan( image_client, XorStr( "A1 ? ? ? ? F6 44 F0 04 01 74 0B" ) ) + 1 );

	#else
		Displacement.Data.m_uClientSideAnimationList = *( std::uintptr_t * )( Memory::Scan( image_client, XorStr( "A1 ? ? ? ? F6 44 F0 04 01 74 0B" ) ) + 1 );

	#endif
	#ifdef DEV
		Displacement.Data.m_uGlowObjectManager = *( std::uintptr_t * )( Memory::Scan( image_client, XorStr( "0F 11 05 ? ? ? ? 83 C8 01" ) ) + 3 );

	#else
		Displacement.Data.m_uGlowObjectManager = *( std::uintptr_t * )( Memory::Scan( image_client, XorStr( "0F 11 05 ? ? ? ? 83 C8 01" ) ) + 3 );

	#endif
	#ifdef DEV
		Displacement.Data.m_uRenderBeams = ( std::uintptr_t )( Memory::Scan( image_client, XorStr( "A1 ? ? ? ? FF 10 A1 ? ? ? ? B9" ) ) + 0x1 );

	#else
		Displacement.Data.m_uRenderBeams = ( std::uintptr_t )( Memory::Scan( image_client, XorStr( "A1 ? ? ? ? FF 10 A1 ? ? ? ? B9" ) ) + 0x1 );

	#endif

	#ifdef DEV
		Displacement.Data.m_uSmokeCount = ( std::uintptr_t )( Memory::Scan( image_client, XorStr( "A3 ? ? ? ? 57 8B CB" ) ) + 0x1 );
	#else
		Displacement.Data.m_uSmokeCount = ( std::uintptr_t )( Memory::Scan( image_client, XorStr( "A3 ? ? ? ? 57 8B CB" ) ) + 0x1 );
	#endif

	#ifdef DEV
		Engine::Displacement.Data.m_PostProcessParameters = ( std::uintptr_t )Memory::Scan( XorStr( "client.dll" ), XorStr( "0F 11 05 ? ? ? ? 0F 10 87" ) ) + 3;
	#else
		Engine::Displacement.Data.m_PostProcessParameters = ( std::uintptr_t )Memory::Scan( XorStr( "client.dll" ), XorStr( "0F 11 05 ? ? ? ? 0F 10 87" ) ) + 3;
	#endif	

	#ifdef DEV
		Engine::Displacement.Function.m_UpdateAllViewmodelAddons = ( std::uintptr_t )Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 83 E4 ? 83 EC ? 53 8B D9 56 57 8B 03 FF 90 ? ? ? ? 8B F8 89 7C 24 ? 85 FF 0F 84 ? ? ? ? 8B 17 8B CF" ) );

	#else
		Engine::Displacement.Function.m_UpdateAllViewmodelAddons = ( std::uintptr_t )Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 83 E4 ? 83 EC ? 53 8B D9 56 57 8B 03 FF 90 ? ? ? ? 8B F8 89 7C 24 ? 85 FF 0F 84 ? ? ? ? 8B 17 8B CF" ) );
	#endif

	#ifdef DEV
		Engine::Displacement.Data.m_uUnknownPredValue = Memory::Scan( XorStr( "client.dll" ), XorStr( "83 3D ? ? ? ? ? 7E 1C" ) ) + 2;
	#else
		Engine::Displacement.Data.m_uUnknownPredValue = Memory::Scan( XorStr( "client.dll" ), XorStr( "83 3D ? ? ? ? ? 7E 1C" ) ) + 2;
	#endif

	#ifdef DEV
		Displacement.Data.m_uCenterPrint = ( std::uintptr_t )( Memory::Scan( image_client, XorStr( "8B 35 ? ? ? ? 8D 4C 24 20" ) ) + 0x2 );

	#else
		Displacement.Data.m_uCenterPrint = ( std::uintptr_t )( Memory::Scan( image_client, XorStr( "8B 35 ? ? ? ? 8D 4C 24 20" ) ) + 0x2 );

	#endif
	#ifdef DEV
		Displacement.Data.m_uHostFrameTicks = ( Memory::Scan( image_engine, XorStr( "03 05 ? ? ? ? 83 CF 10" ) ) + 2 );

	#else
		Displacement.Data.m_uHostFrameTicks = ( Memory::Scan( image_engine, XorStr( "03 05 ? ? ? ? 83 CF 10" ) ) + 2 );

	#endif
	#ifdef DEV
		Displacement.Data.m_uServerGlobals = Memory::Scan( image_server, XorStr( "8B 15 ? ? ? ? 33 C9 83 7A 18 01" ) ) + 0x2;

	#else
		Displacement.Data.m_uServerGlobals = Memory::Scan( image_server, XorStr( "8B 15 ? ? ? ? 33 C9 83 7A 18 01" ) ) + 0x2;

	#endif
	#ifdef DEV
		Displacement.Data.m_uServerPoseParameters = Memory::Scan( image_server, XorStr( "8D 87 ? ? ? ? 89 46 08 C7 46 ? ? ? ? ? EB 06" ) ) + 0x2;

	#else
		Displacement.Data.m_uServerPoseParameters = Memory::Scan( image_server, XorStr( "8D 87 ? ? ? ? 89 46 08 C7 46 ? ? ? ? ? EB 06" ) ) + 0x2;;

	#endif
	#ifdef DEV
		Displacement.Data.m_uServerAnimState = Memory::Scan( image_server, XorStr( "8B 8F ? ? ? ? 85 C9 74 06 56" ) ) + 2;

	#else
		Displacement.Data.m_uServerAnimState = Memory::Scan( image_server, XorStr( "8B 8F ? ? ? ? 85 C9 74 06 56" ) ) + 2;

	#endif
	#ifdef DEV
		Displacement.Data.m_D3DDevice = Memory::Scan( image_shaderapidx9, XorStr( "A1 ? ? ? ? 50 8B 08 FF 51 0C" ) ) + 1;

	#else
		Displacement.Data.m_D3DDevice = Memory::Scan( image_shaderapidx9, XorStr( "A1 ? ? ? ? 50 8B 08 FF 51 0C" ) ) + 1;

	#endif
	#ifdef DEV
		Displacement.Data.m_uTicksAllowed = Memory::Scan( image_server, XorStr( "FF 86 ? ? ? ? 8B CE 89 86" ) ) + 2;

	#else
		Displacement.Data.m_uTicksAllowed = Memory::Scan( image_server, XorStr( "FF 86 ? ? ? ? 8B CE 89 86" ) ) + 2;

	#endif
	#ifdef DEV
		Displacement.Data.m_uHudElement = Memory::Scan( image_client, XorStr( "B9 ? ? ? ? E8 ? ? ? ? 8B 5D 08" ) ) + 1;

	#else
		Displacement.Data.m_uHudElement = Memory::Scan( image_client, XorStr( "B9 ? ? ? ? E8 ? ? ? ? 8B 5D 08" ) ) + 1;

	#endif
	#ifdef DEV
		Displacement.Data.m_uListLeavesInBoxReturn = Memory::Scan( image_client, XorStr( "FF 52 18 8B 7D 08 8B" ) ) + 3;

	#else
		Displacement.Data.m_uListLeavesInBoxReturn = Memory::Scan( image_client, XorStr( "FF 52 18 8B 7D 08 8B" ) ) + 3;

	#endif
	#ifdef DEV
		Displacement.Data.s_bAllowExtrapolation = Memory::Scan( image_client, XorStr( "A2 ? ? ? ? 8B 45 E8" ) ) + 1;

	#else
		Displacement.Data.s_bAllowExtrapolation = Memory::Scan( image_client, XorStr( "A2 ? ? ? ? 8B 45 E8" ) ) + 1;

	#endif
	#ifdef DEV
		Displacement.Data.m_FireBulletsReturn = Memory::Scan( image_client, XorStr( "3B 3D ? ? ? ? 75 4C" ) );

	#else
		Displacement.Data.m_FireBulletsReturn = Memory::Scan( image_client, XorStr( "3B 3D ? ? ? ? 75 4C" ) );

	#endif
	#ifdef DEV
		//Displacement.Data.m_SoundService = Memory::Scan( image_engine, XorStr( "B9 ? ? ? ? 80 65 FC FE 6A 00" ) );

	#else
		//Displacement.Data.m_SoundService  =  Memory::GetAddress( loader_hash( XorStr( "B9 ? ? ? ? 80 65 FC FE 6A 00" ) ) );

	#endif
	#ifdef DEV
		Displacement.Data.m_InterpolateServerEntities = Memory::Scan( image_client, XorStr( "55 8B EC 83 EC 1C 8B 0D ? ? ? ? 53 56" ) ); // xref CVProfile::EnterScope(g_VProfCurrentProfile, XorStr("C_BaseEntity::InterpolateServerEntities"), 0, XorStr("Interpolation"), 0, 4);

	#else
		Displacement.Data.m_InterpolateServerEntities = Memory::Scan( image_client, XorStr( "55 8B EC 83 EC 1C 8B 0D ? ? ? ? 53 56" ) ); // xref CVProfile::EnterScope(g_VProfCurrentProfile, XorStr("C_BaseEntity::InterpolateServerEntities"), 0, XorStr("Interpolation"), 0, 4);

	#endif

	#ifdef DEV
		Displacement.Data.m_uPredictionRandomSeed = *( std::uintptr_t * )( Memory::Scan( image_client, XorStr( "8B 0D ? ? ? ? BA ? ? ? ? E8 ? ? ? ? 83 C4" ) ) + 2 );

	#else
		Displacement.Data.m_uPredictionRandomSeed = *( std::uintptr_t * )( Memory::Scan( image_client, XorStr( "8B 0D ? ? ? ? BA ? ? ? ? E8 ? ? ? ? 83 C4" ) ) + 2 );

	#endif
	#ifdef DEV
		Displacement.Data.m_uPredictionPlayer = *( std::uintptr_t * )( Memory::Scan( image_client, XorStr( "89 ? ? ? ? ? F3 0F 10 48 20" ) ) + 2 );

	#else
		Displacement.Data.m_uPredictionPlayer = *( std::uintptr_t * )( Memory::Scan( image_client, XorStr( "89 ? ? ? ? ? F3 0F 10 48 20" ) ) + 2 );

	#endif

		Displacement.Data.m_SendNetMsg = Memory::Scan( image_engine, XorStr( "55 8B EC 56 8B F1 8B 86 ? ? ? ? 85 C0 74 24 48 83 F8 02 77 2C 83 BE ? ? ? ? ? 8D 8E ? ? ? ? 74 06 32 C0 84 C0 EB 10 E8 ? ? ? ? 84 C0 EB 07 83 BE ? ? ? ? ? 0F 94 C0 84 C0 74 07 B0 01 5E 5D C2 0C 00" ), false ); // xref volume || ConVarRef %s doesn't point to an existing ConVar\

	#ifdef DEV
		Displacement.Data.m_ResetContentsCache = Memory::Scan( image_client, XorStr( "56 8D 51 3C BE" ) );

	#else
		Displacement.Data.m_ResetContentsCache = Memory::Scan( image_client, XorStr( "56 8D 51 3C BE" ) );


	#endif
	#ifdef DEV
		Displacement.Data.m_ProcessInterpolatedList = Memory::Scan( image_client, XorStr( "0F B7 05 ? ? ? ? 3D ? ? ? ? 74 3F" ) ); // xref C_BaseEntity::InterpolateServerEntities

	#else
		Displacement.Data.m_ProcessInterpolatedList = Memory::Scan( image_client, XorStr( "0F B7 05 ? ? ? ? 3D ? ? ? ? 74 3F" ) ); // xref C_BaseEntity::InterpolateServerEntities

	#endif
	#ifdef DEV
		Displacement.Data.CheckReceivingListReturn = Memory::Scan( image_engine, XorStr( "8B 1D ? ? ? ? 85 C0 74 16 FF B6" ) );

	#else
		Displacement.Data.CheckReceivingListReturn = Memory::Scan( image_engine, XorStr( "8B 1D ? ? ? ? 85 C0 74 16 FF B6" ) );

	#endif
	#ifdef DEV
		Displacement.Data.ReadSubChannelDataReturn = Memory::Scan( image_engine, XorStr( "85 C0 74 12 53 FF 75 0C 68 ? ? ? ? FF 15 ? ? ? ? 83 C4 0C" ) );

	#else
		Displacement.Data.ReadSubChannelDataReturn = Memory::Scan( image_engine, XorStr( "85 C0 74 12 53 FF 75 0C 68 ? ? ? ? FF 15 ? ? ? ? 83 C4 0C" ) );

	#endif
	#ifdef DEV
		Displacement.Data.SendDatagram = Memory::Scan( image_engine, XorStr( "55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? 56 57 8B F9 89 7C 24 18" ) ); // 55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? 56 57 8B F9 89 7C 24 18

	#else
		Displacement.Data.SendDatagram = Memory::Scan( image_engine, XorStr( "55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? 56 57 8B F9 89 7C 24 18" ) ); // 55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? 56 57 8B F9 89 7C 24 18

	#endif
	#ifdef DEV
		Displacement.Data.ProcessPacket = Memory::Scan( image_engine, XorStr( "55 8B EC 83 E4 C0 81 EC ? ? ? ? 53 56 57 8B 7D 08 8B D9" ) );

	#else
		Displacement.Data.ProcessPacket = Memory::Scan( image_engine, XorStr( "55 8B EC 83 E4 C0 81 EC ? ? ? ? 53 56 57 8B 7D 08 8B D9" ) );

	#endif
		Displacement.Function.m_uRandomSeed = ( std::uintptr_t )( GetProcAddress( image_vstdlib, XorStr( "RandomSeed" ) ) );
		Displacement.Function.m_uRandomFloat = ( std::uintptr_t )( GetProcAddress( image_vstdlib, XorStr( "RandomFloat" ) ) );
		Displacement.Function.m_uRandomInt = ( std::uintptr_t )( GetProcAddress( image_vstdlib, XorStr( "RandomInt" ) ) );

	#ifdef DEV
		Engine::Displacement.Data.m_uRetScopeArc = Memory::Scan( XorStr( "client.dll" ), XorStr( "6A 00 FF 50 3C 8B 0D ? ? ? ? FF B7" ) ) + 5;
	#else
		Engine::Displacement.Data.m_uRetScopeArc = Memory::Scan( XorStr( "client.dll" ), XorStr( "6A 00 FF 50 3C 8B 0D ? ? ? ? FF B7" ) ) + 5;
	#endif

	#ifdef DEV
		Engine::Displacement.Data.m_uMaintainSequence = reinterpret_cast< void * >( Memory::Scan( XorStr( "client.dll" ), XorStr( "84 C0 74 17 8B 87" ) ) );
	#else
		Engine::Displacement.Data.m_uMaintainSequence = reinterpret_cast< void * >( Memory::Scan( XorStr( "client.dll" ), XorStr( "84 C0 74 17 8B 87" ) ) );
	#endif

	#ifdef DEV
		Engine::Displacement.Function.m_uLookupSequence = Memory::CallableFromRelative( Memory::Scan( XorStr( "client.dll" ), XorStr( "E8 ? ? ? ? 5E 83 F8 FF" ) ) );
		Engine::Displacement.Data.m_uViewMatrix = Memory::Scan( XorStr( "client.dll" ), XorStr( "0F 10 05 ? ? ? ? 8D 85 ? ? ? ? B9" ) );
		Engine::Displacement.Data.m_uRetExtrapolation = Memory::Scan( XorStr( "client.dll" ), XorStr( "FF D0 A1 ? ? ? ? B9 ? ? ? ? D9 1D ? ? ? ? FF 50 34 85 C0 74 22 8B 0D" ) ) + 0x29;
		Engine::Displacement.Data.m_uRetSetupVelocity = Memory::Scan( XorStr( "client.dll" ), XorStr( "84 C0 75 38 8B 0D ? ? ? ? 8B 01 8B 80" ) );
		Engine::Displacement.Data.m_uRetAccumulateLayers = Memory::Scan( XorStr( "client.dll" ), XorStr( "84 C0 75 0D F6 87" ) );
		Engine::Displacement.Data.m_uAnimLayer = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 57 8B F9 8B 97 ? ? ? ? 85 D2" ) );
		Engine::Displacement.Data.m_uAnimLayer = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 57 8B F9 8B 97 ? ? ? ? 85 D2" ) );
		//Engine::Displacement.Function.m_uNextThink = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 56 57 8B F9 8B B7 E8" ) );
		Engine::Displacement.Function.m_GetItemName = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 56 8B F1 E8 ? ? ? ? 8B 8E" ) );

		// ref: CPrediction::ProcessMovement  (*(void (__thiscall **)(_DWORD *))(*player + 552))(player);
		Engine::Displacement.Data.m_ThinkOffset = *( int * )( Memory::Scan( XorStr( "client.dll" ), XorStr( "FF 90 ? ? ? ? FF 35 ? ? ? ? 8B 4C 24 3C" ) ) + 2 ) / 4;
		Engine::Displacement.Data.m_PreThinkOffset = *( int * )( Memory::Scan( XorStr( "client.dll" ), XorStr( "FF 90 ? ? ? ? 8B 86 ? ? ? ? 83 F8 FF" ) ) + 2 ) / 4;

		// int __usercall GetSequenceLinearMotion@<eax>(int a1@<edx>, _DWORD *a2@<ecx>, int a3, _DWORD *a4)
		// it fastcall, but edx and ecx swaped
		// xref: Bad pstudiohdr in GetSequenceLinearMotion()!\n | Bad sequence (%i out of %i max) in GetSequenceLinearMotion() for model '%s'!\n
		Engine::Displacement.Function.m_GetSequenceLinearMotion = Memory::CallableFromRelative( Memory::Scan( XorStr( "client.dll" ), XorStr( "E8 ? ? ? ? F3 0F 10 4D ? 83 C4 08 F3 0F 10 45 ? F3 0F 59 C0" ) ) );
		Engine::Displacement.Function.m_GetWeaponPrefix = Memory::Scan( XorStr( "client.dll" ), XorStr( "53 56 8B F1 57 33 FF 8B 4E ? 8B 01" ) );
		//Engine::Displacement.Function.m_GetWeaponPrefix = Memory::Scan( XorStr( "client.dll" ), XorStr( "53 56 57 8B F9 33 F6 8B 4F 60 8B 01 FF 90 2C 04 00 00 89 47 64" ) );
	#else
		Engine::Displacement.Function.m_uLookupSequence = Memory::CallableFromRelative( Memory::Scan( XorStr( "client.dll" ), XorStr( "E8 ? ? ? ? 5E 83 F8 FF" ) ) );
		Engine::Displacement.Data.m_uViewMatrix = Memory::Scan( XorStr( "client.dll" ), XorStr( "0F 10 05 ? ? ? ? 8D 85 ? ? ? ? B9" ) );
		Engine::Displacement.Data.m_uRetExtrapolation = Memory::Scan( XorStr( "client.dll" ), XorStr( "FF D0 A1 ? ? ? ? B9 ? ? ? ? D9 1D ? ? ? ? FF 50 34 85 C0 74 22 8B 0D" ) ) + 0x29;
		Engine::Displacement.Data.m_uRetSetupVelocity = Memory::Scan( XorStr( "client.dll" ), XorStr( "84 C0 75 38 8B 0D ? ? ? ? 8B 01 8B 80" ) );
		Engine::Displacement.Data.m_uRetAccumulateLayers = Memory::Scan( XorStr( "client.dll" ), XorStr( "84 C0 75 0D F6 87" ) );
		Engine::Displacement.Data.m_uAnimLayer = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 57 8B F9 8B 97 ? ? ? ? 85 D2" ) );
		Engine::Displacement.Data.m_uAnimLayer = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 57 8B F9 8B 97 ? ? ? ? 85 D2" ) );
		//Engine::Displacement.Function.m_uNextThink = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 56 57 8B F9 8B B7 E8" ) );
		Engine::Displacement.Function.m_GetItemName = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 56 8B F1 E8 ? ? ? ? 8B 8E" ) );

		// ref: CPrediction::ProcessMovement  (*(void (__thiscall **)(_DWORD *))(*player + 552))(player);
		Engine::Displacement.Data.m_ThinkOffset = *( int * )( Memory::Scan( XorStr( "client.dll" ), XorStr( "FF 90 ? ? ? ? FF 35 ? ? ? ? 8B 4C 24 3C" ) ) + 2 ) / 4;
		Engine::Displacement.Data.m_PreThinkOffset = *( int * )( Memory::Scan( XorStr( "client.dll" ), XorStr( "FF 90 ? ? ? ? 8B 86 ? ? ? ? 83 F8 FF" ) ) + 2 ) / 4;

		// int __usercall GetSequenceLinearMotion@<eax>(int a1@<edx>, _DWORD *a2@<ecx>, int a3, _DWORD *a4)
		// it fastcall, but edx and ecx swaped
		// xref: Bad pstudiohdr in GetSequenceLinearMotion()!\n | Bad sequence (%i out of %i max) in GetSequenceLinearMotion() for model '%s'!\n
		Engine::Displacement.Function.m_GetSequenceLinearMotion = Memory::CallableFromRelative( Memory::Scan( XorStr( "client.dll" ), XorStr( "E8 ? ? ? ? F3 0F 10 4D ? 83 C4 08 F3 0F 10 45 ? F3 0F 59 C0" ) ) );
		Engine::Displacement.Function.m_GetWeaponPrefix = Memory::Scan( XorStr( "client.dll" ), XorStr( "53 56 8B F1 57 33 FF 8B 4E ? 8B 01" ) );
		//Engine::Displacement.Function.m_GetWeaponPrefix = Memory::Scan( XorStr( "client.dll" ), XorStr( "53 56 57 8B F9 33 F6 8B 4F 60 8B 01 FF 90 2C 04 00 00 89 47 64" ) );
	#endif

	#ifdef DEV
		Engine::Displacement.Data.m_uGameRulesProxy = Memory::Scan( XorStr( "client.dll" ), XorStr( "8B 0D ? ? ? ? 85 C0 74 0A 8B 01 FF 50 78 83 C0 54" ) ) + 2;
	#else
		Engine::Displacement.Data.m_uGameRulesProxy = Memory::Scan( XorStr( "client.dll" ), XorStr( "8B 0D ? ? ? ? 85 C0 74 0A 8B 01 FF 50 78 83 C0 54" ) ) + 2;
	#endif	

	#ifdef DEV
		//Engine::Displacement.Data.m_uRetLoadout = Memory::Scan( XorStr( "client.dll" ), XorStr( "84 C0 75 05 B0 01 5F" ) );
	#else
		//Engine::Displacement.Data.m_uRetLoadout = Memory::GetAddress( loader_hash( XorStr( "84 C0 75 05 B0 01 5F" ) ) );
	#endif


	#ifdef DEV
		Displacement.Function.m_CLReadPackets = Memory::Scan( XorStr( "engine.dll" ), XorStr( "53 8A D9 8B 0D ? ? ? ? 56 57 8B B9" ) );
	#else
		Displacement.Function.m_CLReadPackets = Memory::Scan( XorStr( "engine.dll" ), XorStr( "53 8A D9 8B 0D ? ? ? ? 56 57 8B B9" ) );
	#endif

	#ifdef DEV
		Engine::Displacement.Function.m_LoadFromBufferFunc = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 83 E4 F8 83 EC 34 53 8B 5D 0C 89" ) );
	#else
		Engine::Displacement.Function.m_LoadFromBufferFunc = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 83 E4 F8 83 EC 34 53 8B 5D 0C 89" ) );
	#endif

	#ifdef DEV
		Engine::Displacement.Data.m_UpdateCollisionBoundsIndex = *( int * )( Memory::Scan( XorStr( "client.dll" ), XorStr( "FF 90 ? ? ? ? 83 3D ? ? ? ? ? 7E 1C" ) ) + 2 ) / 4;
	#else
		Engine::Displacement.Data.m_UpdateCollisionBoundsIndex = *( int * )( Memory::Scan( XorStr( "client.dll" ), XorStr( "FF 90 ? ? ? ? 83 3D ? ? ? ? ? 7E 1C" ) ) + 2 ) / 4;
	#endif

	#ifdef DEV
		Engine::Displacement.Function.m_InitKeyValuesFunc = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 51 33 C0 C7 45" ) );
	#else
		Engine::Displacement.Function.m_InitKeyValuesFunc = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 51 33 C0 C7 45" ) );
	#endif

	#ifndef DEV
		Engine::Displacement.Data.m_uEnableInvalidateBoneCache = Memory::Scan( XorStr( "client.dll" ), XorStr( "C6 05 ? ? ? ? ? 89 47 70" ) ) + 2;
	#else
		Engine::Displacement.Data.m_uEnableInvalidateBoneCache = Memory::Scan( XorStr( "client.dll" ), XorStr( "C6 05 ? ? ? ? ? 89 47 70" ) ) + 2;
	#endif	

	#ifndef DEV
		Engine::Displacement.Data.m_uAbsRecomp = Memory::Scan( XorStr( "client.dll" ), XorStr( "C6 05 ? ? ? ? ? 88 81 ? ? ? ? F3 0F 10 4E" ) ) + 2;
	#else
		Engine::Displacement.Data.m_uAbsRecomp = Memory::Scan( XorStr( "client.dll" ), XorStr( "C6 05 ? ? ? ? ? 88 81 ? ? ? ? F3 0F 10 4E" ) ) + 2;
	#endif


	#ifdef DEV
		Engine::Displacement.Data.m_uRandomOffset = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 81 EC 88 00 00 00 56 8B F1" ) );
	#else
		Engine::Displacement.Data.m_uRandomOffset = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 81 EC 88 00 00 00 56 8B F1" ) );
	#endif

	#ifdef DEV
		Engine::Displacement.Data.m_FallVelocity = *( int * )( Memory::Scan( XorStr( "client.dll" ), XorStr( "C7 87 ? ? ? ? ? ? ? ? 83 BF ? ? ? ? ? 75 0C" ) ) + 2 );
	#else
		Engine::Displacement.Data.m_FallVelocity = *( int * )( Memory::Scan( XorStr( "client.dll" ), XorStr( "C7 87 ? ? ? ? ? ? ? ? 83 BF ? ? ? ? ? 75 0C" ) ) + 2 );
	#endif

	#ifdef DEV
		Engine::Displacement.Data.m_SetSequenceIndex = *( int * )( Memory::Scan( XorStr( "client.dll" ), XorStr( "FF 90 ? ? ? ? 8B 07 8B CF FF 90 ? ? ? ? 8B CF" ) ) + 2 ) / 4;
	#else
		Engine::Displacement.Data.m_SetSequenceIndex = *( int * )( Memory::Scan( XorStr( "client.dll" ), XorStr( "FF 90 ? ? ? ? 8B 07 8B CF FF 90 ? ? ? ? 8B CF" ) ) + 2 ) / 4;
	#endif

	#ifdef DEV
		Displacement.Function.m_uSetAbsOrigin = Memory::Scan( image_client, XorStr( "55 8B EC 83 E4 F8 51 53 56 57 8B F1 E8" ) );

	#else
		Displacement.Function.m_uSetAbsOrigin = Memory::Scan( image_client, XorStr( "55 8B EC 83 E4 F8 51 53 56 57 8B F1 E8" ) );
	#endif

	#ifdef DEV
		Displacement.Function.m_ConstructVoiceMsg = Memory::Scan( XorStr( "engine.dll" ), XorStr( "56 57 8B F9 8D 4F 08 C7 07 ? ? ? ? E8 ? ? ? ? C7" ) );

	#else
		Displacement.Function.m_ConstructVoiceMsg = Memory::Scan( XorStr( "engine.dll" ), XorStr( "56 57 8B F9 8D 4F 08 C7 07 ? ? ? ? E8 ? ? ? ? C7" ) );
	#endif

	#ifdef DEV
		Displacement.Function.m_uSetAbsAngles = Memory::Scan( image_client, XorStr( "55 8B EC 83 E4 F8 83 EC 64 53 56 57 8B F1 E8" ) );

	#else
		Displacement.Function.m_uSetAbsAngles = Memory::Scan( image_client, XorStr( "55 8B EC 83 E4 F8 83 EC 64 53 56 57 8B F1 E8" ) );

	#endif
	#ifdef DEV
		Displacement.Function.m_uIsBreakable = Memory::Scan( image_client, XorStr( "55 8B EC 51 56 8B F1 85 F6 74 68" ) ); //xref

	#else
		Displacement.Function.m_uIsBreakable = Memory::Scan( image_client, XorStr( "55 8B EC 51 56 8B F1 85 F6 74 68" ) ); //xref

	#endif
	#ifdef DEV
		//Displacement.Function.m_uClearHudWeaponIcon = Memory::CallableFromRelative( Memory::Scan( image_client, XorStr( "E8 ? ? ? ? 8B F0 C6 44 24 ? ? C6 44 24" ) ) );

	#else
		//Displacement.Function.m_uClearHudWeaponIcon = Memory::CallableFromRelative( Memory::GetAddress( loader_hash( XorStr( "E8 ? ? ? ? 8B F0 C6 44 24 ? ? C6 44 24" ) ) ) );

	#endif
	#ifdef DEV
		Displacement.Function.m_uLoadNamedSkys = Memory::Scan( image_engine, XorStr( "55 8B EC 81 EC ? ? ? ? 56 57 8B F9 C7 45" ) );

	#else
		Displacement.Function.m_uLoadNamedSkys = Memory::Scan( image_engine, XorStr( "55 8B EC 81 EC ? ? ? ? 56 57 8B F9 C7 45" ) );

	#endif

	#ifdef DEV
		Engine::Displacement.Data.m_uRetScopeClear = Memory::Scan( XorStr( "client.dll" ), XorStr( "0F 82 ? ? ? ? FF 50 3C" ) ) + 0x9;
	#else
		Engine::Displacement.Data.m_uRetScopeClear = Memory::Scan( XorStr( "client.dll" ), XorStr( "0F 82 ? ? ? ? FF 50 3C" ) ) + 0x9;
	#endif

	#ifdef DEV
		Engine::Displacement.Data.m_uRetScopeLens = Memory::Scan( XorStr( "client.dll" ), XorStr( "FF 50 3C 8B 4C 24 20" ) ) + 3;
	#else
		Engine::Displacement.Data.m_uRetScopeLens = Memory::Scan( XorStr( "client.dll" ), XorStr( "FF 50 3C 8B 4C 24 20" ) ) + 3;
	#endif

	#ifdef DEV
		Engine::Displacement.Data.m_uRetScopeBlur = Memory::Scan( XorStr( "client.dll" ), XorStr( "FF B7 ? ? ? ? 8B 01 FF 90 ? ? ? ? 8B 4C 24 24" ) ) - 0x6;
	#else
		Engine::Displacement.Data.m_uRetScopeBlur = Memory::Scan( XorStr( "client.dll" ), XorStr( "FF B7 ? ? ? ? 8B 01 FF 90 ? ? ? ? 8B 4C 24 24" ) ) - 0x6;
	#endif

	#ifdef DEV
		Displacement.Function.m_uResetAnimState = Memory::Scan( image_client, XorStr( "56 6A 01 68 ? ? ? ? 8B F1" ) );

	#else
		Displacement.Function.m_uResetAnimState = Memory::Scan( image_client, XorStr( "56 6A 01 68 ? ? ? ? 8B F1" ) );

	#endif
	#ifdef DEV
		Displacement.Function.m_uUpdateAnimState = Memory::Scan( image_client, XorStr( "55 8B EC 83 E4 F8 83 EC 18 56 57 8B F9 F3 0F 11 54 24" ) );

	#else
		Displacement.Function.m_uUpdateAnimState = Memory::Scan( image_client, XorStr( "55 8B EC 83 E4 F8 83 EC 18 56 57 8B F9 F3 0F 11 54 24" ) );

	#endif
	#ifdef DEV
		Displacement.Function.m_uClanTagChange = Memory::Scan( image_engine, XorStr( "53 56 57 8B DA 8B F9 FF 15" ) );

	#else
		Displacement.Function.m_uClanTagChange = Memory::Scan( image_engine, XorStr( "53 56 57 8B DA 8B F9 FF 15" ) );

	#endif
	#ifdef DEV
		Displacement.Function.m_uGetSequenceActivity = Memory::Scan( image_client, XorStr( "55 8B EC 83 7D 08 FF 56 8B F1 74 3D" ) );

	#else
		Displacement.Function.m_uGetSequenceActivity = Memory::Scan( image_client, XorStr( "55 8B EC 83 7D 08 FF 56 8B F1 74 3D" ) );

	#endif
	#ifdef DEV
		Displacement.Function.m_uInvalidatePhysics = Memory::Scan( image_client, XorStr( "55 8B EC 83 E4 F8 83 EC 0C 53 8B 5D 08 8B C3 56 83 E0 04" ) );

	#else
		Displacement.Function.m_uInvalidatePhysics = Memory::Scan( image_client, XorStr( "55 8B EC 83 E4 F8 83 EC 0C 53 8B 5D 08 8B C3 56 83 E0 04" ) );

	#endif

	#ifdef DEV
		Displacement.Function.m_uCreateAnimState = Memory::Scan( image_client, XorStr( "55 8B EC 56 8B F1 B9 ? ? ? ? C7 46" ) );

	#else
		Displacement.Function.m_uCreateAnimState = Memory::Scan( image_client, XorStr( "55 8B EC 56 8B F1 B9 ? ? ? ? C7 46" ) );

	#endif

	#ifdef DEV
		Displacement.Function.m_uPostThinkVPhysics = CallableFromRelative( Memory::Scan( image_client, XorStr( "E8 ? ? ? ? 8B CF E8 ? ? ? ? 8B 06" ) ) );

	#else
		Displacement.Function.m_uPostThinkVPhysics = CallableFromRelative( Memory::Scan( image_client, XorStr( "E8 ? ? ? ? 8B CF E8 ? ? ? ? 8B 06" ) ) );

	#endif
	#ifdef DEV
		Displacement.Function.m_SimulatePlayerSimulatedEntities = Memory::Scan( image_client, XorStr( "56 8B F1 57 8B BE ? ? ? ? 83 EF 01 78 72 90 8B 86" ) );

	#else
		Displacement.Function.m_SimulatePlayerSimulatedEntities = Memory::Scan( image_client, XorStr( "56 8B F1 57 8B BE ? ? ? ? 83 EF 01 78 72 90 8B 86" ) );

	#endif
	#ifdef DEV
		Displacement.Function.m_uImplPhysicsRunThink = Memory::Scan( image_client, XorStr( "55 8B EC 83 EC 10 53 56 57 8B F9 8B 87 ? ? ? ? C1 E8 16" ) );

	#else
		Displacement.Function.m_uImplPhysicsRunThink = Memory::Scan( image_client, XorStr( "55 8B EC 83 EC 10 53 56 57 8B F9 8B 87 ? ? ? ? C1 E8 16" ) );

	#endif
	#ifdef DEV
	//	Displacement.Function.m_uClearDeathNotices = Memory::Scan( image_client, XorStr( "55 8B EC 83 EC 0C 53 56 8B 71 58" ) ); //55 8B EC 83 EC 0C 53 56 8B 71 58 33 DB 57 89 5D F8 8B 4E 04 8B 01 FF 90

	#else
		//Displacement.Function.m_uClearDeathNotices = Memory::GetAddress( loader_hash( XorStr( "55 8B EC 83 EC 0C 53 56 8B 71 58" ) ) ); //55 8B EC 83 EC 0C 53 56 8B 71 58 33 DB 57 89 5D F8 8B 4E 04 8B 01 FF 90

	#endif
	#ifdef DEV
		Displacement.Function.m_uSetTimeout = Memory::Scan( image_engine, XorStr( "55 8B EC 80 7D 0C 00 F3 0F 10 4D" ) );

	#else
		Displacement.Function.m_uSetTimeout = Memory::Scan( image_engine, XorStr( "55 8B EC 80 7D 0C 00 F3 0F 10 4D" ) );

	#endif
	#ifdef DEV
		Displacement.Function.m_uFindHudElement = Memory::Scan( image_client, XorStr( "55 8B EC 53 8B 5D 08 56 57 8B F9 33 F6 39 77 28" ) );

	#else
		Displacement.Function.m_uFindHudElement = Memory::Scan( image_client, XorStr( "55 8B EC 53 8B 5D 08 56 57 8B F9 33 F6 39 77 28" ) );

	#endif
	#ifdef DEV
		Displacement.Function.m_SetCollisionBounds = Memory::Scan( image_client, XorStr( "53 8B DC 83 EC 08 83 E4 F8 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 83 EC 10 56 57 8B 7B" ) );

	#else
		Displacement.Function.m_SetCollisionBounds = Memory::Scan( image_client, XorStr( "53 8B DC 83 EC 08 83 E4 F8 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 83 EC 10 56 57 8B 7B" ) );

	#endif
	#ifdef DEV
		Displacement.Function.m_MD5PseudoRandom = Memory::Scan( image_client, XorStr( "55 8B EC 83 E4 F8 83 EC 70 6A 58 8D 44 24 1C 89 4C 24 08 6A 00 50" ) );

	#else
		Displacement.Function.m_MD5PseudoRandom = Memory::Scan( image_client, XorStr( "55 8B EC 83 E4 F8 83 EC 70 6A 58 8D 44 24 1C 89 4C 24 08 6A 00 50" ) );

	#endif
	#ifdef DEV
		Displacement.Function.m_WriteUsercmd = Memory::Scan( image_client, XorStr( "55 8B EC 83 E4 F8 51 53 56 8B D9 8B 0D" ) );

	#else
		Displacement.Function.m_WriteUsercmd = Memory::Scan( image_client, XorStr( "55 8B EC 83 E4 F8 51 53 56 8B D9 8B 0D" ) );

	#endif
	#ifdef DEV
		Displacement.Function.m_StdStringAssign = Memory::Scan( image_engine, XorStr( "55 8B EC 53 8B 5D 08 56 8B F1 85 DB 74 57 8B 4E 14 83 F9 10 72 04 8B 06 EB 02" ) );

	#else
		Displacement.Function.m_StdStringAssign = Memory::Scan( image_engine, XorStr( "55 8B EC 53 8B 5D 08 56 8B F1 85 DB 74 57 8B 4E 14 83 F9 10 72 04 8B 06 EB 02" ) );

	#endif
	#ifdef DEV
		Displacement.Function.m_pPoseParameter = Memory::Scan( image_client, XorStr( "55 8B EC 8B 45 08 57 8B F9 8B 4F 04 85 C9 75 15 8B" ) );

	#else
		Displacement.Function.m_pPoseParameter = Memory::Scan( image_client, XorStr( "55 8B EC 8B 45 08 57 8B F9 8B 4F 04 85 C9 75 15 8B" ) );

	#endif
	#ifdef DEV
		Displacement.Function.m_AttachmentHelper = Memory::Scan( image_client, XorStr( "55 8B EC 83 EC 48 53 8B 5D 08 89 4D F4" ) );

	#else
		Displacement.Function.m_AttachmentHelper = Memory::Scan( image_client, XorStr( "55 8B EC 83 EC 48 53 8B 5D 08 89 4D F4" ) );

	#endif
	#ifdef DEV
		Displacement.Function.m_LockStudioHdr = CallableFromRelative( Memory::Scan( image_client, XorStr( "E8 ? ? ? ? 8D 47 FC" ) ) );

	#else
		Displacement.Function.m_LockStudioHdr = CallableFromRelative( Memory::Scan( image_client, XorStr( "E8 ? ? ? ? 8D 47 FC" ) ) );

	#endif
	#ifdef DEV
		Displacement.Function.m_LineGoesThroughSmoke = Memory::Scan( image_client, XorStr( "55 8B EC 83 EC 08 8B 15 ? ? ? ? 0F 57 C0" ) );

	#else
		Displacement.Function.m_LineGoesThroughSmoke = Memory::Scan( image_client, XorStr( "55 8B EC 83 EC 08 8B 15 ? ? ? ? 0F 57 C0" ) );

	#endif
	#ifdef DEV
		Displacement.Function.m_RunSimulation = CallableFromRelative( Memory::Scan( image_client, XorStr( "E8 ? ? ? ? A1 ? ? ? ? F3 0F 10 45 ? F3 0F 11 40" ) ) );

	#else
		Displacement.Function.m_RunSimulation = CallableFromRelative( Memory::Scan( image_client, XorStr( "E8 ? ? ? ? A1 ? ? ? ? F3 0F 10 45 ? F3 0F 11 40" ) ) );

	#endif
	#ifdef DEV
		Displacement.Function.m_TraceFilterSimple = Memory::Scan( image_client, XorStr( "55 8B EC 83 E4 F0 83 EC 7C 56 52" ) ) + 0x3D; //xref : offset ?_7CTraceFilterSimple@@6B@ ; const CTraceFilterSimple::`vftable' 

	#else
		Displacement.Function.m_TraceFilterSimple = Memory::Scan( image_client, XorStr( "55 8B EC 83 E4 F0 83 EC 7C 56 52" ) ) + 0x3D; //xref : offset ?_7CTraceFilterSimple@@6B@ ; const CTraceFilterSimple::`vftable' 

	#endif	

		Displacement.Function.m_TraceFilterSkipTwoEntities = Memory::Scan( image_client, XorStr( "C7 45 ? ? ? ? ? 89 45 E4 8B 01" ), false ) + 0x3;
	}

	bool CreateDisplacement( void *reserved ) {
		Create( );

		return true;
	}
}