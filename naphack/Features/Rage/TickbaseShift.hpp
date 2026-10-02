#pragma once
#include "../../SDK/sdk.hpp"

enum class ChargeType : int {
	CHARGE_FUCKED,
	CHARGE_DOUBLETAP,
	CHARGE_HIDESHOTS
};

class TickbaseSystem {
public:
	void Charge( );
	void UnCharge( bool forceProcess = false );

	bool CanCharge( );
	bool IncrementProcessTicks( );
	bool IsCharged( );

	void CL_Move( bool bFinalTick, float accumulated_extra_samples );

	bool DoTheTapping( Encrypted_t<bool> bSendPacket, Encrypted_t<CUserCmd> pFrom );

	void RunExploits( Encrypted_t<bool> bSendPacket, Encrypted_t<CUserCmd> pCmd );

	void DoRegularShifting( int iAmount );

	bool CanShift( bool bDoubleTap = false, bool bNoWeap = false );
	int GetCorrection( );

	// charging
	bool m_bPreparedRecharge = false;
	bool m_bPrepareCharge = false;
	bool m_bCharge = false;
	int m_bTeleportUncharge = 0;
	int m_nTicksCharged = 0;
	bool m_bSentFirstAfterCharge = false;
	bool m_bInCharge = false;
	int m_nTicksAfterUncharge = 0;
	ChargeType m_nChargeType = ChargeType::CHARGE_FUCKED;

	bool m_bDisabledFakelag = false;

	bool m_bOverrideIsPaused = false;

	// is this the 2nd dt shot?
	bool m_bTapShot = false;
	bool m_bDoZeroMove = false;
	bool m_bDoAutoStop = false;
	bool m_bDelayAimbot = false;

	bool m_bBreakLC = true;
	bool m_bBreakingLC = false;
	int m_nBreakLCCorrection = -1;
	bool m_bFixedEarlier = false;

	bool m_bShiftMove;
	bool m_bShifting;
	bool m_bDelay;
	bool m_bDelayDont;

	bool m_bFORCESEND = false;
	bool m_bDontCommunicate = false;

	// currently charged ticks
	int m_iProcessTicks;
	int m_iCurrentShiftedTick = 0;
	// ...
	int m_iMaxProcessTicks = 14;
	int m_nLCShiftAmount;
	int m_nAllowLowFpsRecharge = 32;

	// used for WriteUsercmdDeltaToBuffer shifting
	int m_iShiftAmount;
	bool m_bDoubleTapRelated = false;

	bool m_bForceRecharge;

	float m_flLastExploitTime;
	float m_flFramerate;

	float m_flLastLoss;
	bool m_bAllowLossRecharge = true;

	CUserCmd* m_pCurrentCmd;
	bool* m_pCurrentPacket;

	C_WeaponCSBaseGun *m_pWeapon;
	CCSWeaponInfo *m_pWeaponData;

	struct tickbaseFix_t {
		int nCommand;
		int nFixAmount;
		bool bFixPositive;
		bool bFixNegative;
		bool bDone = false;
	};

	bool m_bPerformDefensive = false;
	std::vector< tickbaseFix_t > m_vecTickbaseFix;
};

extern TickbaseSystem g_TickbaseController;