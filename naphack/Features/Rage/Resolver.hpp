#pragma once
#include "Animations.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

enum EResolverStages : int {
	RES_NONE,
	RES_STAND,
	RES_MOVE,
	RES_AIR,
	RES_OVERRIDE,
	RES_MAX
};

enum EBodyState : int {
	BODY_DEFAULT,
	BODY_BREAK
};

enum EConfidence : int {
	CONF_LOW,
	CONF_MED,
	CONF_HIGH,
	CONF_VHIGH
};

struct ResolverData_t {

	static constexpr float kInvalidFloat = std::numeric_limits<float>::quiet_NaN( );

	__forceinline bool IsValidFloat( float fl ) {
		return !std::isnan( fl );
	}

	__forceinline bool IsValidInt( int n ) {
		return n != -1;
	}

	__forceinline void Reset( ) {
		// reset every stage
		for( int i = EResolverStages::RES_STAND; i < EResolverStages::RES_MAX; ++i ) {
			ResetStageSpecific( ( EResolverStages )i );
			m_nMissedShots[ i ] = 0;
		}
	}

	// полный сброс: вызывать только при смене игрока в слоте (другой SteamID / переподключение)
	__forceinline void FullReset( ) {
		Reset( );

		for( int &n : m_nMissedShots )
			n = 0;

		m_vecAngles.clear( );
		m_eBodyState = EBodyState::BODY_DEFAULT;
		m_bHasBeenStatic = false;
		m_bHasUpdatedLBY = false;
		m_flLastWeightAdjustTime = kInvalidFloat;
		m_flLastCycleAdjustTime = kInvalidFloat;
		m_bMissedFreestand = false;
		m_nMissedBody = -1;
		m_ulSteamID = 0;
	}

	template< class T >
	static __forceinline void TrimOld( std::vector< T > &v, size_t max ) {
		if( v.size( ) > max )
			v.erase( v.begin( ), v.begin( ) + ( v.size( ) - max ) );
	}

	// resets jump related data
	__forceinline void ResetStageSpecific( EResolverStages stage ) {
		if( stage <= EResolverStages::RES_NONE ||
			stage >= EResolverStages::RES_MAX )
			return;

		// reset jump related data
		if( stage == EResolverStages::RES_AIR ) {
			m_bPlausibleBody = false;
			return;
		}

		// reset move related data
		if( stage == EResolverStages::RES_MOVE ) {
			m_flLastMovingBody = kInvalidFloat;
			m_flLastMovingTime = kInvalidFloat;
			m_pMoveData = {};
			return;
		}

		// reset stand data
		m_flLastSafeBody = kInvalidFloat;
		m_flNextBodyUpdate = kInvalidFloat;
		m_flFreestandYaw = kInvalidFloat;
		m_flEdgeYaw = kInvalidFloat;
		m_bPredictingBody = false;
		m_bAdjusting = false;
		m_vecDeductedAngles.clear( );
		m_vecPlayerHurtInfo.clear( );
		m_vecBloodAngles.clear( );
		m_eIdealConfidence = EConfidence::CONF_LOW;
		m_eAngleConfidence = EConfidence::CONF_LOW;
		m_flApproxDelta = kInvalidFloat;
		m_nFootSide = 0;
		m_flFootSideTime = kInvalidFloat;
		m_bDistortion = false;
		m_bBodyTimerExpired = false;
		m_bBodyTimerFailed = false;
		m_flBalanceAdjustTime = kInvalidFloat;
		m_flPrevAdjustCycle = kInvalidFloat;
		m_nPrevAdjustSequence = -1;
	
		// DON'T RESET IT HERE
		// m_bMissedFreestand = false;
		// m_nMissedBody = -1;
	}

	// stand specific data
#pragma region STAND_DATA
	struct DeductedAngle_t {
		float m_flAngle;
		float m_flTime;
	};

	struct BloodAngle_t {
		float m_flAngle;
		float m_flTime;
		EConfidence m_eConfidence;
	};

	struct PlayerHurtInfo_t {
		int m_nHitgroup;
		float m_flTime;
	};

	float m_flFreestandYaw = kInvalidFloat;
	float m_flEdgeYaw = kInvalidFloat;
	float m_flNextBodyUpdate = kInvalidFloat;
	bool m_bPredictingBody = false;
	bool m_bMissedFreestand = false;
	bool m_bHasBeenStatic = false;
	bool m_bHasUpdatedLBY = false;
	int m_nMissedBody = -1;
	std::vector<float> m_vecAngles;
	std::vector<DeductedAngle_t> m_vecDeductedAngles;
	std::vector<BloodAngle_t> m_vecBloodAngles;

	// Reset( ) is not called for every entry before first use, so these need
	// initializers of their own.
	EBodyState m_eBodyState = EBodyState::BODY_DEFAULT;
	bool m_bAdjusting = false;

	// sign of the desync, when we can prove it.
	// +1: real angle sits on the positive (left) side of lby, -1: negative (right) side,
	// 0: unknown. this only fixes the SIDE - the magnitude is still bruteforced, which
	// halves the search space.
	int m_nFootSide = 0;
	float m_flFootSideTime = kInvalidFloat;

	// the balance-adjust layer is playing in both records while an lby update was due:
	// the player is deliberately masking the turn and we cannot tell the side. don't
	// pretend to know it - say so, and let the aimbot require a safer point.
	bool 	m_bDistortion = false;
	float m_flLastWeightAdjustTime = kInvalidFloat;
	float m_flLastCycleAdjustTime = kInvalidFloat;
	float m_flLastSafeBody = kInvalidFloat;
	EConfidence m_eIdealConfidence = EConfidence::CONF_LOW;
	EConfidence m_eAngleConfidence = EConfidence::CONF_LOW;

	// модель серверного таймера LBY ( csgo_playeranimstate.cpp, SetUpVelocity )
	bool m_bBodyTimerExpired = false; // таймер истёк, флика не было -> реал в пределах 35° от ног
	bool m_bBodyTimerFailed = false; // промахнулись по такому LBY -> не верим до реального флика
	float m_flBalanceAdjustTime = kInvalidFloat;
	float m_flPrevAdjustCycle = kInvalidFloat;
	int m_nPrevAdjustSequence = -1;

	float m_flApproxDelta = kInvalidFloat;

	// blood shit
	std::vector<PlayerHurtInfo_t> m_vecPlayerHurtInfo;

#pragma endregion

	// move specific data
#pragma region MOVE_DATA
	LagRecord_t m_pMoveData;

	float m_flLastMovingBody = kInvalidFloat;
	float m_flLastMovingTime = kInvalidFloat;
#pragma endregion

	// jump specific data
#pragma region JUMP_DATA
	bool m_bPlausibleBody = false;
#pragma endregion

public:
	// count missed shots separately for each resolver stage
	int m_nMissedShots[ EResolverStages::RES_MAX ] = {};
	__forceinline int GetMissedShots( EResolverStages stage ) {
		if( stage < 0 || stage >= EResolverStages::RES_MAX )
			return 0;

		return m_nMissedShots[ stage ];
	}

	// excluded: this is the current stage we're in,
	// means we will NOT reset missed shots for this stage
	//
	// NOTE: this used to zero every other stage's counter on each entry into a stage.
	// players alternate between stand and move constantly, so a single step wiped the
	// bruteforce progress of the stage we were about to come back to and the search
	// never converged. counters are per-stage already, so they can simply persist;
	// they are cleared when we land a hit, or when the player changes.
	__forceinline void UpdateMissedShots( EResolverStages excluded ) {
		( void )excluded;
	}

	__forceinline void IncrementMissedShots( EResolverStages stage ) {
		if( stage <= EResolverStages::RES_NONE || stage >= EResolverStages::RES_MAX )
			return;

		++m_nMissedShots[ stage ];
	}

	// we hit them on this stage, so the current hypothesis is working - drop the
	// accumulated bruteforce progress for it.
	__forceinline void ResetMissedShots( EResolverStages stage ) {
		if( stage <= EResolverStages::RES_NONE || stage >= EResolverStages::RES_MAX )
			return;

		m_nMissedShots[ stage ] = 0;
	}

	uint64_t m_ulSteamID = 0;
};

struct ImpactInfo_t {
	Vector m_vecStart;
	Vector m_vecEnd;

	// -1 means "we don't know which hitgroup we aimed at" - the deduction resolver
	// must not filter on it then.
	int m_iExpectedHitgroup = -1;
	bool m_bDealtDamage = false;
};

class C_TEEffectDispatch;
class Resolver {
	EResolverStages UpdateResolverStage( LagRecord_t *pRecord );

	void OnPlayerResolve( LagRecord_t *pRecord, LagRecord_t *pPreviousRecord );

	void OnPlayerStand( LagRecord_t *pRecord, LagRecord_t *pPreviousRecord );
	void OnPlayerStandTrial( LagRecord_t *pRecord, LagRecord_t *pPreviousRecord );
	void OnPlayerMove( LagRecord_t *pRecord, LagRecord_t *pPreviousRecord );
	void OnPlayerJump( LagRecord_t *pRecord, LagRecord_t *pPreviousRecord );
	void OnPlayerOverride( LagRecord_t *pRecord, LagRecord_t *pPreviousRecord );

	float GetRelativePitch( LagRecord_t *pRecord, float flAngle );
	float GetRelativeYaw( LagRecord_t *pRecord, float flAngle );
	float GetFreestandYaw( LagRecord_t *pRecord, const std::vector< float > &vecCandidates = { } );
	void GetApproximateBodyState( LagRecord_t *pRecord, LagRecord_t *pPrevious );
	void GetApproximateBodyDelta( LagRecord_t *pRecord, LagRecord_t *pPrevious );

public:
	// scratch entry handed out for out-of-range indices. callers write into the
	// returned reference, so returning a local here was a dangling reference.
	// this one is real storage: garbage in it is harmless, UB was not.
	ResolverData_t m_InvalidResolverData;

	__forceinline ResolverData_t &GetResolverData( int n ) {
		if( n <= 0 || n >= 65 ) {
			m_InvalidResolverData.FullReset( );

			return m_InvalidResolverData;
		}

		return m_arrResolverData.at( n );
	}

	void OnBulletImpact( LagRecord_t *pRecord, ImpactInfo_t *info );
	void OnSpawnBlood( C_TEEffectDispatch *pBlood );

	void CorrectShotRecord( LagRecord_t *record );

	LagRecord_t FindIdealRecord( Animations::AnimationEntry_t *data, float flTargetDamage = 1337.f );

	void ResolvePlayers( LagRecord_t *pRecord, LagRecord_t *pPreviousRecord );
	std::array< ResolverData_t, 65 > m_arrResolverData;
};

extern Resolver g_Resolver;