#pragma once

#include "sdk.hpp"

struct DllInitializeData {
	DWORD32 dwDataN;

	// C_BaseEntity
	struct {
		DWORD32 m_rgflCoordinateFrame;
		DWORD32 m_MoveType;
	} C_BaseEntity;

	// DT_BaseEntity
	struct {
		DWORD32 m_vecOrigin;
		DWORD32 m_flSimulationTime;
		DWORD32 m_fEffects;
		DWORD32 m_Particles;
		DWORD32 m_Collision;
		DWORD32 m_iEFlags;
		DWORD32 m_hOwnerEntity;
		DWORD32 moveparent;
		DWORD32 m_iTeamNum;
		DWORD32 m_nModelIndex;
		DWORD32 m_CollisionGroup;

		DWORD32 m_nForceBone;
		DWORD32 InvalidateBoneCache;
		DWORD32 m_flAnimTime;
	} DT_BaseEntity;

	// DT_BaseWeaponWorldModel
	struct {
		DWORD32 m_hCombatWeaponParent;
	} DT_BaseWeaponWorldModel;

	// C_BaseAnimating
	struct {

		DWORD32 m_bIsJiggleBonesEnabled;
		DWORD32 m_BoneAccessor;
		DWORD32 m_bShouldDraw;
		DWORD32 m_AnimOverlay;
		DWORD32 m_flLastBoneSetupTime;
		DWORD32 InvalidateBoneCache;
		DWORD32 m_CachedBoneData;
		DWORD32 m_pSecondBoneSnapshot;
		DWORD32 m_iOcclusionFlags;
		DWORD32 m_nCachedBonesPosition;
		DWORD32 m_pStudioHdr;
		DWORD32 BoneSnapshotsCall;
		DWORD32 m_iMostRecentModelBoneCounter;
		DWORD32 m_pFirstBoneSnapshot;
		DWORD32 m_pBoneMerge;
		DWORD32 CacheBoneData;
		DWORD32 m_iPrevBoneMask;
		DWORD32 m_iAccumulatedBoneMask;
		DWORD32 m_nCachedBonesRotation;
		DWORD32 m_pIk;
		DWORD32 m_iOcclusionFramecount;

	} C_BaseAnimating;

	// DT_BaseAnimating
	struct {

		DWORD32 m_bClientSideRagdoll;
		DWORD32 m_nSequence;
		DWORD32 m_flEncodedController;
		DWORD32 m_bClientSideAnimation;
		DWORD32 m_nHitboxSet;
		DWORD32 m_bIsLookingAtWeapon;
		DWORD32 m_flPoseParameter;
		DWORD32 m_flCycle;

	} DT_BaseAnimating;

	// DT_BaseCombatCharacter
	struct {
		DWORD32 m_flNextAttack;
		DWORD32 m_hActiveWeapon;
		DWORD32 m_hMyWearables;
		DWORD32 m_hMyWeapons;
	} DT_BaseCombatCharacter;

	// C_BasePlayer
	struct {
		DWORD32 relative_call;
		DWORD32 offset;
		DWORD32 m_pCurrentCommand;
		DWORD32 UpdateVisibilityAllEntities;
	} C_BasePlayer;

	// DT_BasePlayer
	struct {

		DWORD32 m_flFallVelocity;
		DWORD32 m_flDuckAmount;
		DWORD32 m_flDuckSpeed;
		DWORD32 m_iHealth;
		DWORD32 m_iDefaultFOV;
		DWORD32 m_fFlags;
		DWORD32 m_bSpotted;
		DWORD32 m_ubEFNoInterpParity;
		DWORD32 m_ubOldEFNoInterpParity;
		DWORD32 m_iObserverMode;
		DWORD32 m_lifeState;
		DWORD32 m_nTickBase;
		DWORD32 m_hGroundEntity;
		DWORD32 m_aimPunchAngle;
		DWORD32 m_aimPunchAngleVel;
		DWORD32 m_viewPunchAngle;
		DWORD32 pl;
		DWORD32 m_hObserverTarget;
		DWORD32 m_hViewModel;
		DWORD32 m_vecViewOffset;
		DWORD32 m_vecVelocity;
		DWORD32 m_vecBaseVelocity;
		DWORD32 m_vecLadderNormal;
	} DT_BasePlayer;

	struct {
		DWORD32 m_bombsiteCenterB;
		DWORD32 m_bombsiteCenterA;
	} DT_CSPlayerResource;

	// C_CSPlayer
	struct {
		DWORD32 m_PlayerAnimState;
		DWORD32 m_flSpawnTime;
		DWORD32 m_utlClientImpactList;
		DWORD32 m_bUseNewAnimState;

	} C_CSPlayer;

	// DT_CSPlayer 
	struct {
		DWORD32 m_iMatchStats_Deaths;
		DWORD32 m_iMatchStats_HeadShotKills;
		DWORD32 m_iMoveState;
		DWORD32 m_bWaitForNoAttack;
		DWORD32 m_bCustomPlayer;
		DWORD32 m_iPlayerState;
		DWORD32 m_bIsDefusing;
		DWORD32 m_hCarriedHostage;
		DWORD32 m_nSurvivalTeam;
		DWORD32 m_bHasHelmet;
		DWORD32 m_bHasHeavyArmor;
		DWORD32 m_bIsWalking;
		DWORD32 m_flStamina;
		DWORD32 m_flThirdpersonRecoil;
		DWORD32 m_iAccount;
		DWORD32 m_iShotsFired;
		DWORD32 m_flFlashDuration;
		DWORD32 m_flLowerBodyYawTarget;
		DWORD32 m_flVelocityModifier;
		DWORD32 m_bGunGameImmunity;
		DWORD32 m_flHealthShotBoostExpirationTime;
		DWORD32 m_iMatchStats_Kills;
		DWORD32 m_bHasDefuser;
		DWORD32 m_bIsPlayerGhost;
		DWORD32 m_iFOV;
		DWORD32 m_angEyeAngles;
		DWORD32 m_vecPlayerPatchEconIndices;
		DWORD32 m_hRagdoll;
		DWORD32 m_bInNoDefuseArea;
		DWORD32 m_ArmorValue;
		DWORD32 m_bScoped;
		DWORD32 m_bDuckOverride;
	} DT_CSPlayer;

	// DT_CSRagdoll
	struct {
		DWORD32 m_hPlayer;
	} DT_CSRagdoll;

	// DT_FogController
	struct {
		DWORD32 m_fog_enable;
	} DT_FogController;

	// DT_Precipitation
	struct {
		DWORD32 m_nPrecipType;
	} DT_Precipitation;

	// DT_BaseCombatWeapon
	struct {
	
		DWORD32 m_iItemDefinitionIndex;
		DWORD32 m_iWorldDroppedModelIndex;
		DWORD32 m_iViewModelIndex;
		DWORD32 m_CustomMaterials;
		DWORD32 m_bCustomMaterialInitialized;
		DWORD32 m_hWeaponWorldModel;
		DWORD32 m_iWorldModelIndex;
		DWORD32 m_flNextPrimaryAttack;
		DWORD32 m_iPrimaryReserveAmmoCount;
		DWORD32 m_flNextSecondaryAttack;
		DWORD32 m_hOwner;
		DWORD32 m_iClip1;

	} DT_BaseCombatWeapon;

	// DT_WeaponCSBase
	struct {
		DWORD32 m_weaponMode;
		DWORD32 m_flRecoilIndex;
		DWORD32 m_fLastShotTime;
		DWORD32 m_fAccuracyPenalty;
		DWORD32 m_flPostponeFireReadyTime;
	} DT_WeaponCSBase;

	// DT_WeaponCSBaseGun
	struct {
		DWORD32 m_fNextBurstShot;
		DWORD32 m_zoomLevel;
		DWORD32 m_iBurstShotsRemaining;
	} DT_WeaponCSBaseGun;

	// DT_BaseCSGrenade
	struct {
		DWORD32 m_flThrowStrength;
		DWORD32 m_bPinPulled;
		DWORD32 m_fThrowTime;
	} DT_BaseCSGrenade;

	// DT_BaseAttributableItem
	struct {
		
		DWORD32 m_nFallbackStatTrak;
		DWORD32 m_iEntityLevel;
		DWORD32 m_iEntityQuality;
		DWORD32 m_OriginalOwnerXuidHigh;
		DWORD32 m_OriginalOwnerXuidLow;
		DWORD32 m_bInitialized;
		DWORD32 m_iAccountID;
		DWORD32 m_nFallbackPaintKit;
		DWORD32 m_iItemDefinitionIndex;
		DWORD32 m_Item;
		DWORD32 m_iItemIDLow;
		DWORD32 m_iItemIDHigh;
		DWORD32 m_nFallbackSeed;
		DWORD32 m_flFallbackWear;
		DWORD32 m_szCustomName;
	} DT_BaseAttributableItem;

	// DT_BaseViewModel
	struct {
		DWORD32 m_nSequence;
		DWORD32 m_hOwner;
		DWORD32 m_hWeapon;
	} DT_BaseViewModel;

	// DT_SmokeGrenadeProjectile
	struct {
		DWORD32 m_SmokeParticlesSpawned;
		DWORD32 m_nSmokeEffectTickBegin;
		DWORD32 m_bDidSmokeEffect;
	} DT_SmokeGrenadeProjectile;

	// DT_PlantedC4
	struct {
		DWORD32 m_bBombDefused;
		DWORD32 m_flC4Blow;
		DWORD32 m_flDefuseCountDown;
	} DT_PlantedC4;

	// CClientState
	struct {	
		DWORD32 m_nLastOutgoingCommand;
		DWORD32 m_nLastCommandAck;
		DWORD32 m_bIsHLTV;
		DWORD32 m_nMaxClients;
		DWORD32 m_nDeltaTick;
		DWORD32 m_nChokedCommands;
	} CClientState;

	// CBoneMergeCache
	struct {
		DWORD32 m_nConstructor;
		DWORD32 m_nUpdateCache;
		DWORD32 m_CopyToFollow;
		DWORD32 m_CopyFromFollow;
		DWORD32 m_nInit;

	} CBoneMergeCache;

	// CIKContext
	struct {
		DWORD32 m_nUpdateTargets;
		DWORD32 m_nConstructor;
		DWORD32 m_nDestructor;
		DWORD32 m_nInit;
		DWORD32 m_nSolveDependencies;
	} CIKContext;

	// CBoneSetup
	struct {
		DWORD32 AccumulatePose;
		DWORD32 CalcAutoplaySequences;
		DWORD32 CalcBoneAdj;
		DWORD32 InitPose;
	} CBoneSetup;

	// IPrediction
	struct {
		DWORD32 m_nCommandsPredicted;
	} IPrediction;

	struct {
	
		DWORD32 m_uCenterPrint;
		DWORD32 m_uHostFrameTicks;
	
		DWORD32 m_uRenderBeams;
		DWORD32 m_uSmokeCount;
		DWORD32 m_InterpolateServerEntities;
		DWORD32 m_SendNetMsg;
		DWORD32 m_D3DDevice;
		DWORD32 m_SoundService;
		DWORD32 m_uMoveHelper;
		DWORD32 m_uInput;
		DWORD32 m_uGlobalVars;
		DWORD32 m_uPredictionRandomSeed;
		DWORD32 m_uPredictionPlayer;
		DWORD32 m_uModelBoneCounter;
		DWORD32 m_uClientSideAnimationList;
		DWORD32 m_uServerGlobals;
		DWORD32 m_uServerPoseParameters;
		DWORD32 m_uServerAnimState;
		DWORD32 m_uAnimLayer;
		DWORD32 m_uTicksAllowed;
		DWORD32 m_uHudElement;
		DWORD32 m_uListLeavesInBoxReturn;
		DWORD32 s_bAllowExtrapolation;
		DWORD32 m_FireBulletsReturn;
		DWORD32 m_uRandomOffset;
		DWORD32 m_uEnableInvalidateBoneCache;
		DWORD32 m_uAbsRecomp;
		DWORD32 m_uGlowObjectManager;
		DWORD32 m_uRetScopeLens;
		DWORD32 m_uRetScopeClear;
		DWORD32 m_uGameRulesProxy;
		DWORD32 m_uRetLoadout;
		DWORD32 m_uRetSetupVelocity;
		DWORD32 m_uRetAccumulateLayers;
		DWORD32 m_uRetExtrapolation;
		DWORD32 m_uViewMatrix;
		DWORD32 m_ResetContentsCache;
		DWORD32 m_ProcessInterpolatedList;
		DWORD32 CheckReceivingListReturn;
		DWORD32 ReadSubChannelDataReturn;
		DWORD32 SendDatagram;
		DWORD32 ProcessPacket;
		DWORD32 m_uRetScopeBlur;	
		DWORD32 m_PostProcessParameters;
		DWORD32 m_uUnknownPredValue;
		DWORD32 m_uRetScopeArc;

		// just faster like this
		

		int m_SetSequenceIndex;
		int m_ThinkOffset;
		int m_PreThinkOffset;
		void *m_uMaintainSequence;
		int m_FallVelocity;
		int m_UpdateCollisionBoundsIndex;
	} Data;

	struct {
		
		DWORD32 m_uCreateAnimState;
		DWORD32 m_uResetAnimState;
		DWORD32 m_AttachmentHelper;
		DWORD32 m_GetSequenceLinearMotion;
	
		DWORD32 m_RunSimulation;
		DWORD32 m_uSetTimeout;
		DWORD32 m_uFindHudElement;
		DWORD32 m_uUpdateAnimState;
		DWORD32 m_uPostThinkVPhysics;
		DWORD32 m_SimulatePlayerSimulatedEntities;
		DWORD32 m_uNextThink;
		DWORD32 m_uImplPhysicsRunThink;
		DWORD32 m_uClanTagChange;
		DWORD32 m_uGetSequenceActivity;
		DWORD32 m_uInvalidatePhysics;
		DWORD32 m_uClearDeathNotices;
		DWORD32 m_GetItemName;
		DWORD32 m_GetWeaponPrefix;
		DWORD32 m_uRandomInt;
		DWORD32 m_LoadFromBufferFunc;
		DWORD32 m_InitKeyValuesFunc;
		DWORD32 m_ClipRayToHitbox;
		DWORD32 m_uRandomSeed;
		DWORD32 m_uRandomFloat;
		DWORD32 m_uLoadNamedSkys;
		DWORD32 m_uSetAbsOrigin;
		DWORD32 m_StdStringAssign;
		DWORD32 m_UpdateAllViewmodelAddons;
		DWORD32 m_CLReadPackets;
		DWORD32 m_SetLocalReady;
		DWORD32 m_pPoseParameter;

		DWORD32 m_uLookupSequence;
		DWORD32 m_SetCollisionBounds;
		DWORD32 m_MD5PseudoRandom;
		DWORD32 m_WriteUsercmd;
		DWORD32 m_ConstructVoiceMsg;
		DWORD32 m_uSetAbsAngles;
		DWORD32 m_TraceFilterSimple;
		DWORD32 m_TraceFilterSkipTwoEntities;
	
		DWORD32 m_uIsBreakable;
		DWORD32 m_uClearHudWeaponIcon;
		DWORD32 m_LockStudioHdr;
		DWORD32 m_LineGoesThroughSmoke;

	} Function;

	struct {
		DWORD32 m_bFreezePeriod;
		DWORD32 m_bIsValveDS;
	} DT_CSGameRulesProxy;
};

namespace Engine {
	extern DllInitializeData Displacement;

	bool CreateDisplacement( void *reserved );
}

