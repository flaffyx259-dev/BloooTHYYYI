#pragma once
#include "../../SDK/sdk.hpp"

class LagRecord_t;
struct SimulationContext {
	bool  walking;
	int buttons;
	float Origin;
	Vector m_vecOrigin;
	Vector pMins, pMaxs;
	float someFloat;
	Vector m_vecVelocity;
	int simulationTicks;
	int flags;
	CGameTrace trace;
	float gravity;
	float sv_jump_impulse;
	float stepsize;
	float flMaxSpeed;
	ITraceFilter *filter;
	C_CSPlayer *player;

	void TracePlayerBBox( const Vector &start, const Vector &end, unsigned int fMask, CGameTrace &pm );
	void InitSimulationContext( LagRecord_t *record );
	void ExtrapolatePlayer( float yaw );
	void TryPlayerMove( );
	void RebuildGameMovement( CUserCmd *ucmd );
};