#pragma once
#include "../../SDK/sdk.hpp"

class AdaptiveAngle {
public:
	float m_yaw;
	float m_dist;

public:
	// ctor.
	__forceinline AdaptiveAngle( float yaw, float penalty = 0.f ) {
		// set yaw.
		m_yaw = Math::AngleNormalize( yaw );

		// init distance.
		m_dist = 0.f;

		// remove penalty.
		m_dist -= penalty;
	}
};

enum ESide {
	SIDE_LEFT,
	SIDE_RIGHT,
	SIDE_BACK,
	SIDE_FWD,
	SIDE_MAX
};

class AntiAim {
public:
	void Think( CUserCmd *pCmd, bool *bSendPacket, bool *bFinalPacket );
	ESide GetSide( ) { return m_eSide; }

	bool DoEdgeAntiAim( C_CSPlayer *player, QAngle &out );
	C_CSPlayer *GetBestPlayer( bool distance = false );
	float UpdateFreestandPriority( float flLength, int nIndex, bool bDogshit = false );
	bool IsAbleToFlick( C_CSPlayer *pLocal );
private:
	// returns true when anti-aim must be skipped this tick so the +use
	// interaction can start. false means full anti-aim is safe.
	bool HandleUseAction( CUserCmd *pCmd, C_CSPlayer *pLocal );
	void DoFakeYaw( CUserCmd *pCmd, C_CSPlayer *pLocal );
	void DoRealYaw( CUserCmd *pCmd, C_CSPlayer *pLocal );
	void DoPitch( CUserCmd *pCmd, C_CSPlayer *pLocal );
	void AutoDirection( C_CSPlayer *pLocal );
	void PerformBodyFlick( CUserCmd *pCmd, bool *bSendPacket, float flOffset = 0.f );
	void HandleManual( CUserCmd *pCmd, C_CSPlayer *pLocal );
	void DesyncLastMove( CUserCmd *pCmd, bool *bSendPacket );
private:
	float m_flLastRealAngle;

	ESide m_eAutoSide;
	float m_flAutoYaw;
	float m_flAutoDist;
	float m_flAutoTime;
	int  m_nBodyFlicks;
	bool m_bJitterUpdate;

	float m_flRandDistortFactor;
	float m_flRandDistortSpeed;

	ESide m_eBreakerSide = SIDE_MAX;
	ESide m_eSide = SIDE_MAX;
public:
	bool m_bEdging;
	bool m_bHasValidAuto;

	// which of the two actually wrote the yaw this tick. m_bEdging is the OR of
	// both, so it cannot tell them apart for the indicator.
	bool m_bEdgeYawApplied = false;
	bool m_bAutoYawApplied = false;

	float m_flEdgeOrAutoYaw = FLT_MAX;
	bool m_bHidingLBYFlick = false;
	bool m_bDistorting = false;
	bool m_bWaitForBreaker = false;

	float m_flLastPrebrakeAngle;

	bool m_bLastPacket;
	bool m_bHasOverriden;
	float m_flAngleToAdd;
	bool m_bWasHit;

	bool m_bLbyUpdateThisTick;
	QAngle angViewangle;
	float  flLastMovingLby;
	bool m_bAllowFakeWalkFlick;

	bool m_bLandDesynced = false;

	// +use interaction handling.
	// the server resolves +use targets from the real eye angles of the processed
	// usercmd, so the action has to be *started* with restored angles. once the
	// server has latched the interaction it stops re-tracing, and full anti-aim
	// is safe for the remainder of it.
	enum EUseAction { USE_NONE, USE_DEFUSE, USE_HOSTAGE };

	EUseAction m_eUseAction{ USE_NONE };
	bool m_bRestoringUseAngles{ false };   // briefly aiming at the target to start the action.
	bool m_bUseActionLatched{ false };     // server latched it; full AA is safe now.
	float m_flUseStartTime{ 0.f };
	QAngle m_angUseTarget{ };

	// IN_USE state on the previous tick, for rising-edge detection.
	bool m_bPrevInUse{ false };
	// carry state snapshot taken at the +use rising edge. the new interaction
	// is latched once the carry state differs from this (pickup false->true,
	// stop-follow true->false).
	bool m_bCarryingAtPress{ false };

	// realtime of the last tick we actually overrode the real angles for a
	// +use interaction. the standing distortion is suppressed for a moment
	// after that, because the restored angle left LBY somewhere unexpected.
	// only restore ticks feed this: merely holding +use ( carrying a hostage,
	// standing in a doorway ) must not suppress anything.
	float m_flLastUseRestore{ 0.f };

	// true while climbing a ladder; real yaw/pitch must stay intact.
	bool m_bClimbingLadder{ false };
};

extern AntiAim g_AntiAim;