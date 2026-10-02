#pragma once
#include "../../pandora.hpp"

class Movement {
public:
	bool m_bPeeking;
	int m_bStop;
	bool m_bDidStop;
	bool m_bRetrack;
	Vector m_vecAutoPeekPos;
	float m_flOldSpeed = -1.f;
	QAngle m_vecMovementAngles;
	float m_flSidemove;
	float m_flForwardmove;
	bool m_bModifiedMovementBeforePrediction;

	bool m_bDoForceFakewalk;

	bool PressingMovementKeys( CUserCmd* cmd, bool bJump = true ) {
		// sugma
		auto _cmd = Encrypted_t<CUserCmd>( cmd );
		if( !_cmd.IsValid( ) )
			return false;

		return ( _cmd->buttons & IN_MOVELEFT )
			|| ( _cmd->buttons & IN_MOVERIGHT )
			|| ( _cmd->buttons & IN_FORWARD )
			|| ( _cmd->buttons & IN_BACK )
			|| ( bJump && ( _cmd->buttons & IN_JUMP ) );
	}

	void FixMovement( CUserCmd* cmd, QAngle wish_angles );
	void PrePrediction( CUserCmd* cmd, bool* bSendPacket );
	void InPrediction( CUserCmd* cmd );
	void PostPrediction( CUserCmd* cmd );

	void AutoPeek( CUserCmd* cmd );

	void InstantStop( CUserCmd *cmd );
	void AutoStop( CUserCmd *cmd );

	void accelerate( const Vector &wishdir, float wishspeed, float accel, CMoveData *mv, C_CSPlayer *ent );

	void walk_move( CMoveData *mv, C_CSPlayer *ent );

	void friction( CMoveData *mv, C_CSPlayer *player );
	void friction( Vector &vecVelocity, C_CSPlayer *player );

	void check_parameters( CMoveData *mv, C_CSPlayer *player );

	void StopToSpeed( float speed, CMoveData *mv, C_CSPlayer *player );

	void StopToSpeed( float speed, CUserCmd *cmd );

	void FakeDuck( bool* bSendPacket, CUserCmd* cmd, bool bForce = false );
	void MovementControl( CUserCmd *cmd, float velocity, bool yep = true, bool bUseCmdMove = false );
	void DefuseTheBomb( CUserCmd* cmd );
	void MouseDelta( CUserCmd* cmd );
	bool IsPeeking( C_CSPlayer *pEntity, CUserCmd* cmd );
	bool IsPeekingEx( C_CSPlayer *pEntity, CUserCmd *cmd );
	bool PlayerMove( C_CSPlayer *pEntity, Vector &vecOrigin, Vector &vecVelocity, int &fFlags, bool bOnGround );

	bool m_bNearBomb = false;
	// origin of the planted c4 that set m_bNearBomb, used by the anti-aim
	// to aim at the bomb long enough for the server to latch the defuse.
	Vector m_vecNearBombOrigin{ };
};

extern Movement g_Movement;