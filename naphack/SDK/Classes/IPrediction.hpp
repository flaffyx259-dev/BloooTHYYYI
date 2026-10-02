#pragma once

class IPrediction
{
public:
	void Update( int startframe, bool validframe, int incoming_acknowledged, int outgoing_command );
	void SetupMove( C_BasePlayer* player, CUserCmd* ucmd, void* pHelper, void * move );
	void FinishMove( C_BasePlayer* player, CUserCmd* ucmd, void * move );
	int CheckMovingGround( C_BasePlayer* player, double unk );
	bool InPrediction( );
	void SetLocalViewAngles( QAngle &ang );
	CGlobalVarsBase* GetUnpredictedGlobals( );

	void ForceRepredict( bool bHadPredictionErrors = false ) {
		m_PreviousStartFrame = -1;
		m_nCommandsPredicted = 0;

		if( bHadPredictionErrors )
			m_bPreviousAckHadErrors = 1;
	}

	char pad00[ 8 ];                            // 0x0
	bool m_bInPrediction;                     // 0x8
	bool m_bOldPredictionValue;              // 0x9
	bool m_bEnginePaused;                     // 0xA
	int m_PreviousStartFrame;               // 0xC
	int m_nIncomingPacketNumber;             // 0x10
	float m_flLastServerWorldTimeStamp;     // 0x14
	bool m_bIsFirstTimePredicted;           // 0x18
	int m_nCommandsPredicted;                 // 0x1C
	int m_nServerAckCommands;       // 0x20
	bool m_bPreviousAckHadErrors;           // 0x24
};
