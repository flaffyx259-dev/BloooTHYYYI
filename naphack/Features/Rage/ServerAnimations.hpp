#pragma once
#include "../../SDK/sdk.hpp"
#include "../Rage/Animations.hpp"
#include <optional>

class ServerAnimations {
	struct AnimationInfo_t {
		std::array<C_AnimationLayer, 13> m_pAnimOverlays;
		std::array<float, 20> m_pPoseParameters;

		alignas(16) Vector m_vecBonePos[ 128 ];
		alignas( 16 ) Quaternion m_quatBoneRot[ 128 ];

		float m_flFootYaw;
		float m_flEyeYaw;
		float m_flLowerBodyYawTarget;
		float m_flLowerBodyRealignTimer;
		float m_flSpawnTime;

		bool m_bBreakingTeleportDst;
		Vector m_vecLastOrigin;

		CCSGOPlayerAnimState m_pAnimState;
		bool m_bInitializedAnimState;

		QAngle m_angUpdateAngles;

		bool m_bRealignBreaker;

		bool m_bFirstFlick = false;
		bool m_bWaitForBreaker = false;

		bool m_bDoingPreFlick = false;
		bool m_bDoingRealFlick = false;

		float m_flBodyAlpha;

		bool m_bSetupBones;
		alignas( 16 ) matrix3x4_t m_pMatrix[ MAXSTUDIOBONES ];
	};

	void SetLayerInactive( C_AnimationLayer *pLayer, int idx );

	void SetLayerSequence( C_CSPlayer *pEntity, C_AnimationLayer *pLayer, int32_t activity, /*CUtlVector<uint16_t> modifiers,*/ int nOverrideSequence = -1 );

	QAngle angThirdPersonAngles;

	bool m_bCompute = false;

public:
	bool HandleLayerSeparately( int nLayer );

	CUserCmd *m_pCmd;

	// last real viewangles. NAPAS chokes real and sends fake
	// (AntiAim::Think runs DoFakeYaw on send), so the real angles are the
	// choked commands - the opposite of the 2018 reference builds.
	QAngle m_angRadarAngles;
	bool m_bHasRadarAngles = false;
	// realtime of the last choked command. with fakelag off nothing ever
	// chokes, so a stale real would freeze the radar - the caller falls
	// back to the wish (engine) angles when this goes cold.
	float m_flLastChokeTime = 0.f;

	// firstperson radar fix: pl.v_angle drives the stock radar blip, so we
	// swap its yaw to REAL for the render window (RENDER_START) and restore
	// the FAKE back in RENDER_END. camera stays on fake/wish outside it.
	QAngle m_angRadarSavedVAngle{};
	QAngle m_angRadarSavedEyeAngles{};
	float m_flRadarSavedRotationYaw = 0.f;
	// pure REAL yaw written to v_angle in START. CalcView must use THIS for
	// the delta, never read v_angle back: RemoveVisualEffects (no-recoil)
	// subtracts punch from v_angle between START and CalcView, so a read-back
	// is polluted by punch and leaks recoil into the camera (jerk on shoot /
	// landing). stored pure values keep eye EXACTLY at baseline.
	float m_flRadarSwapRealYaw = 0.f;
	bool m_bRadarVAngleSwapped = false;

	void HandleAnimationEvents( C_CSPlayer *pLocal, CCSGOPlayerAnimState *pState, C_AnimationLayer *layers, /*CUtlVector<uint16_t> uModifiers,*/ CUserCmd *cmd );

	std::array<C_AnimationLayer, 13> m_pPrevAnimOverlays;
	bool m_bHoldingSpace;

	// предсказанный серверный LBY ( сетевой приходит с задержкой RTT ).
	float m_flPredictedLBY = 0.f;
	bool  m_bPredictedLBYValid = false;
	float m_flLBYMismatchSince = -1.f;

	AnimationInfo_t m_uServerAnimations;
	AnimationInfo_t m_uVisualAnimations;
	AnimationInfo_t m_uRenderAnimations;
	AnimationInfo_t m_uBodyAnimations;
	QAngle m_angChokedShotAngle;

	QAngle m_angPreviousAngle;

	void HandleServerAnimation( );
	void HandleAnimations( bool *bSendPacket, CUserCmd *cmd );
};

extern ServerAnimations g_ServerAnimations;