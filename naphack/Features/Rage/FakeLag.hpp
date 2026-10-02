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
};

extern FakeLag g_FakeLag;