#pragma once
#include "../../SDK/sdk.hpp"

class FakeLag {
	bool ShouldFakeLag( Encrypted_t<CUserCmd> pCmd );
	int DetermineFakeLagAmount( Encrypted_t<CUserCmd> pCmd );

public:
	void HandleFakeLag( Encrypted_t<bool> bSendPacket, Encrypted_t<CUserCmd> pCmd );

	bool m_bDidFakelagOnPeek;
	int m_iLagLimit;
	int m_iAwaitingChoke;

	int  m_iCycleChoke = 1;       // длина текущего цикла, выбирается один раз на его старте
	int  m_iLastCycleChoke = 1;
	bool m_bWasPeeking = false;
	bool m_bForceMaxNext = false; // после сброса на пике следующий цикл - максимальный
};

extern FakeLag g_FakeLag;