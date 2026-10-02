#pragma once
#include "../../SDK/sdk.hpp"
#include "Animations.hpp"
#include "Autowall.hpp"

class LagRecord_t;

enum EAutoStopType : int{
	STOP_NONE,
	STOP_SLIDE,
	STOP_FAKE
};

class Aimbot {
private:
	struct Aimbot_t {
		C_CSPlayer *m_pLocal;
		CUserCmd *m_pCmd;
		bool *m_pSendPacket;
		Vector m_vecEyePosition;

		C_WeaponCSBaseGun *m_pWeapon;
		CCSWeaponInfo *m_pWeaponInfo;

		CVariables::RAGE *m_pSettings;

		bool m_bDidStop;
		bool m_bInPredicted;
		bool m_bFillPredictedTargets;
		bool m_bPredictEnemyPlayer;
		int m_iOverrideMinDamage;
		int m_nForcedChokeTicks;
		bool m_bFullyStuck;
		int m_nStuckTicks;

		std::vector< int > m_bPredictedLethal;
		std::vector< int > m_bPredictedBody;
		std::array< int, 65 > m_bPredictedHighestDamage;

		int m_iLastTarget;

		int m_iShotID;
		bool m_bIsZeus;

		float m_flIntendedAccuracy;

		bool m_bInitialisedRandomSpread = false;
		SpreadRandom_t m_uRandomSpread[ 256 ];

		int m_nLastWeaponType;
		int m_nLastWeaponIndex;
		float m_flCockTime;

		bool m_bDidSetupMatrix = false;
		matrix3x4_t m_matSpaceRotation;

		bool m_bAllocatedTheShit = false;

		int m_iCurrentTargetIndex = 0;
	};

	EAutoStopType ChooseStop( );

public:
	std::array< Animations::AnimationEntry_t, 64 > m_players;
	std::vector< Animations::AnimationEntry_t * >   m_targets;
	std::array<std::pair<bool, std::string>, 65> m_arrNapUsers;


	// target selection stuff.
	float m_best_dist;
	float m_best_fov;
	float m_best_damage;
	int   m_best_hp;
	float m_best_lag;
	float m_best_height;

	// found target stuff.
	C_CSPlayer *m_target;
	Vector      m_angle;
	Vector     m_aim;
	float      m_damage;
	int		   m_health;
	mstudiobbox_t *m_bbox;
	LagRecord_t m_record;

	// fake latency stuff.
	bool       m_fake_latency;

	// 0 = none
	// 1 = full stop
	// 2 = slow motion
	// 3 = fake walk
	EAutoStopType m_stop;

	bool m_bSendNextCommand;

	float m_flLimitTargets;
	float m_flLimitTime;
	float m_flFrameRateMultiplier;

public:
	__forceinline void reset( ) {
		// reset aimbot data.
		init( );

		// reset all players data.
		for( auto &p : m_players )
			p.reset( );
	}

	__forceinline bool IsValidTarget( C_CSPlayer *player ) {
		if( !player )
			return false;

		if( !player->IsPlayer( ) )
			return false;

		if( player->IsDead( ) )
			return false;

		auto pLocalPlayer = C_CSPlayer::GetLocalPlayer( );
		if( !pLocalPlayer )
			return false;

		if( player->m_iTeamNum( ) == pLocalPlayer->m_iTeamNum( ) )
			return false;

		return true;
	}

	std::vector<std::pair<C_CSPlayer *, int>> m_vecLastSkippedPlayers{ };
public:
	Aimbot_t m_AimbotInfo;

	// aimbot.
	void init( );
	Vector SetupEyePosition( QAngle ang );
	CVariables::RAGE *GetRageSettings( );
	int GetMinimalDamage( C_CSPlayer *pEntity );
	void CockRevolver( );
	std::deque<std::pair<C_CSPlayer *, int>> FindTargets( );
	void think( bool *bSendPacket, CUserCmd *pCmd );
	void find( );
	bool can_hit_hitbox( const Vector start, const Vector end );
	bool CheckHitchance( C_CSPlayer *player, const QAngle &angle );
	bool SelectTarget( LagRecord_t *record, const Vector &aim, float damage );
	float GetPointSafety( LagRecord_t *pRecord, const Vector &vecStart, const Vector &vecPoint, int nHitbox );
	void apply( );
	bool HandleLagcomp( Animations::AnimationEntry_t *data );
	void NoSpread( );
};

extern Aimbot g_Ragebot;