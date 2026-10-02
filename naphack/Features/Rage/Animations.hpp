#pragma once

#include "../../SDK/sdk.hpp"
#include "../../SDK/Displacement.hpp"

#include "../../SDK/Classes/Player.hpp"
#include "../../SDK/Classes/weapon.hpp"
#include "../../SDK/Valve/CBaseHandle.hpp"

#include "../../Utils/Threading/shared_mutex.h"
#include "../../Utils/Threading/mutex.h"

#include "BoneSetup.hpp"

#include <map>
#include <deque>

enum EResolverStages;
enum EConfidence;
class c_bone_builder;

struct SideInfo_t {
	c_bone_builder m_pBoneBuilder;
	alignas( 16 ) matrix3x4_t m_pMatrix[ 128 ];

	std::array<C_AnimationLayer, 13> m_pServerAnimOverlays;
	std::array<float, 20> m_pPoseParameters;

	QAngle m_angAbsAngles;

	// custom side data
	bool m_bUpdated = false;
	bool m_bSimulated = false;
	float m_flFootYaw = 0.f;

	CCSGOPlayerAnimState m_pAnimState;
};

struct HitboxPosition_t {
	int nHitboxIndex;
	Vector vecPosition;
};

enum ESides {
	SIDE_SERVER,
	SIDE_OPPOSITEL,
	SIDE_OPPOSITER,
	SIDE_MIDDLE ,
	SIDE_INVALID
};

struct LagRecord_t {
	C_CSPlayer *m_pEntity;
	int m_nEntIndex;

	// netvars
	float m_flSimulationTime;
	float m_flLowerBodyYawTarget;
	float m_flDuckAmount;
	float m_fLastShotTime;
	float m_flDurationInAir;

	bool m_bGunGameImmunity;

	int m_iChosenMatrix = 0;

	int m_fFlags;

	Vector m_vecMins;
	Vector m_vecMaxs;

	Vector m_vecOrigin;
	Vector m_vecAbsOrigin;

	Vector m_vecVelocity;
	Vector m_vecAbsVelocity;

	QAngle m_angEyeAngles;

	C_WeaponCSBaseGun *m_pWeapon;

	SideInfo_t m_sAnims[ ESides::SIDE_INVALID ];
	
	alignas( 16 ) matrix3x4_t m_pVisualMatrix[ 128 ];
	alignas( 16 ) matrix3x4_t m_pBackupMatrix[ 128 ];

	// record specific
	int m_fPredFlags = 0;
	float m_flPredSimulationTime = 0.f;
	Vector m_vecPredOrigin;
	Vector m_vecPredVelocity;
	int m_nIdeality = 0;
	float m_flBodyEyeDelta = 0.f;

	int m_nPredictedTicks = 0;

	int m_nServerTick = 0;

	Vector m_vecOriginDelta;

	// сырая средняя скорость по пачке ( сдвиг origin / время ) - для экстраполяции.
	// m_vecVelocity потом подгоняется под момент анимации, прогнозу нужна эта
	Vector m_vecRawVelocity;

	// records are default-constructed by m_deqRecords.emplace_front( ) and SetupRecord
	// does not assign all of these, so they need initializers or they start as garbage.
	bool m_bBrokeTeleportDst = false;
	bool m_bExtrapolated = false;
	bool m_bInvalid = false;
	bool m_bShiftingTickbase = false;
	bool m_bDelaying = false;

	int m_nChokedTicks = 0;
	int m_nServerDeltaTicks = 0;

	bool m_bSimTick = false;
	bool m_bFixingPitch = false;
	bool m_bShooting = false;

	float m_flCreationTime;

	float m_flChokedTime;
	float m_flAnimationTime;
	float m_flInterpolateTime;

	float m_flLastNonShotPitch;

	// these enums are only forward-declared here, so value-initialize them.
	// both have their "none/low" state at 0.
	EResolverStages m_eResolverStage{};
	EConfidence m_eConfidence{};
	int m_iResolverType = 0;
	std::string m_szResolver = "invalid";

	bool m_bIsResolved = false;
	bool m_bLBYFlicked = false;

	bool m_bPotentialDesync = false;

	// which hypothesis the resolver actually used for this record. the miss feedback
	// needs to know this, and it cannot be derived from m_iResolverType: the numbers
	// differ between OnPlayerStand and OnPlayerStandTrial (trial reports 78 for every
	// post-miss angle), so matching on 12/15 never fired under resolver_trial.
	bool m_bUsedFreestand = false;
	bool m_bUsedSameLBY = false;

	// подсказка footyaw от резолвера ( RAX пишет m_abs_yaw до апдейта )
	bool m_bHasFootYaw = false;
	float m_flFootYawHint = 0.f;

	Vector m_vecEyePosition;
	Vector m_vecLowerChestPosition;
	Vector m_vecChestPosition;
	Vector m_vecPelvisPosition;
	Vector m_vecStomachPosition;

	// aimbot specific
	mstudiobbox_t *m_bbox;

	// record func stuff
	bool m_bIsBackup;

	void SetupRecord( C_CSPlayer *pEntity, bool bBackup = false );
	void ApplyRecord( C_CSPlayer *pEntity );
	// прогнозные записи ( m_bExtrapolated ) проходят безусловно: см. комментарий-контракт
	// в теле функции. все остальные проверяются по модели серверного lag compensation.
	bool IsRecordValid( );

	__forceinline void Predict( ) {
		m_vecPredOrigin = m_vecOrigin;
		m_vecPredVelocity = m_vecVelocity;
		m_flPredSimulationTime = m_flSimulationTime;
		m_fPredFlags = m_fFlags;
	}
};

enum HitscanMode : int {
	NORMAL = 0,
	LETHAL = 1,
	LETHAL2 = 3,
	PREFER = 4
};

struct HitscanData_t {
	float  m_damage;
	Vector m_pos;

	__forceinline HitscanData_t( ) : m_damage{ 0.f }, m_pos{}{}
};

struct HitscanBox_t {
	int         m_index;
	HitscanMode m_mode;

	__forceinline bool operator==( const HitscanBox_t &c ) const {
		return m_index == c.m_index && m_mode == c.m_mode;
	}
};

class Animations {
public:
	struct AnimationEntry_t {
	private:
		bool m_bEnteredDormancy;
		float m_flSpawnTime;

		float m_flOldSimulationTime = 0.f;

		void SimulateSideAnimation( LagRecord_t *pRecord, LagRecord_t *pPrevious, ESides eSide, float flTargetAbs );
		void UpdateAnimations( LagRecord_t *pRecord );
	public:

		float m_flPreviousLayer11Cycle = -1.f;
		float m_flPreviousLayer11CyclePostDataUpdate = -1.f;
		float m_flOldSimulationTimeAlt = 0.f;

		int m_nServerTick = 0;

		__forceinline void ClearRecords( ) {
			// прогноз строится от истории, поэтому сбрасываем его всегда, даже когда
			// история уже пуста: иначе ESP-флаг и кеш переживают смерть/дормант игрока
			m_bPredictionValid = false;
			m_nPredictedFromTick = -1;

			if( m_deqRecords.empty( ) )
				return;

			m_deqRecords.clear( );

			m_flOldSimulationTime = 0.f;
			m_flPreviousLayer11Cycle = -1.f;
		}

		void UpdatePlayer( LagRecord_t *pRecord );

		C_CSPlayer *m_pEntity;
		std::deque<LagRecord_t> m_deqRecords;

		LagRecord_t m_backupRecord;

		void UpdateEntry( C_CSPlayer *pEntity );

		// ---- extrapolation ( lag compensation fallback ) ----
		// only used when the target has no usable real record, never as the
		// primary path: extrapolating a player we can actually backtrack to is
		// strictly worse than using the record the server already has.
		//
		// the pair mirrors RAX: the second entry is the state one choke cycle
		// behind the first, which the animation update needs as its "previous".
		LagRecord_t m_PredictedRecord;
		LagRecord_t m_PrevPredictedRecord;

		// server tick the prediction was built from, so we only redo the work
		// once per net update instead of once per call.
		int m_nPredictedFromTick = -1;
		bool m_bPredictionValid = false;

		// сколько раз экстраполяция реально отдала запись. нужно именно для проверки
		// достижимости: код фоллбэка легко компилируется и остаётся мёртвым, и отличить
		// "работает" от "выглядит правильно" можно только по ненулевому счётчику.
		int m_nPredictionHits = 0;

		// returns nullptr when extrapolation is impossible or pointless.
		LagRecord_t *ExtrapolateRecords( );

		// сервер стоит на нашей новейшей записи к моменту обработки команды
		bool IsNewestServerHead( ) const;

	private:
		void UpdatePredictedAnimations( LagRecord_t *pPredicted, LagRecord_t *pPrevious, int nChokedTicks );
	public:

		// aimbot specific
		void SetupHitboxes( LagRecord_t *record, bool history );
		bool SetupHitboxPoints( LagRecord_t *record, matrix3x4_t *bones, int index, std::vector< std::pair<mstudiobbox_t *, std::pair<Vector, bool>> > &points );
		bool GetBestAimPosition( Vector &aim, float &damage, LagRecord_t *record );

		using hitboxcan_t = stdpp::unique_vector< HitscanBox_t >;
		hitboxcan_t m_hitboxes;

		bool m_bDelay;

		__forceinline void reset( ) {
			m_pEntity = nullptr;
			m_flSpawnTime = 0.f;

			m_deqRecords.clear( );
			m_hitboxes.clear( );
		}
	};

private:
	std::array< AnimationEntry_t, 65 > m_uAnimationEntry;
public:
	void OnFrameStageNotify( );

	LagRecord_t GetLatestRecord( int nEntIndex, bool bValidCheck = true );
	LagRecord_t GetOldestRecord( int nEntIndex, bool bValidCheck = true );

	bool GetVisualMatrix( C_CSPlayer *pEntity, matrix3x4_t *pMatrix, bool bBodyUpdate );

	void UpdatePlayerSimple( C_CSPlayer* pEntity );

	__forceinline AnimationEntry_t *GetAnimationEntry( int nEntIndex ) {
		return &m_uAnimationEntry.at( nEntIndex );
	}

	__forceinline bool BreakingTeleportDistance( int nEntIndex ) {
		auto pAnimEntry = GetAnimationEntry( nEntIndex );
		if( !pAnimEntry )
			return false;

		if( pAnimEntry->m_deqRecords.empty( ) )
			return false;

		for( auto &record : pAnimEntry->m_deqRecords ) {
			if( !record.IsRecordValid( ) )
				continue;

			// return out of function, anyway
			// this invalidates the whole track
			if( record.m_bBrokeTeleportDst ) {
				return true;
			}
		}

		return false;
	}

	float m_flOutgoingLatency;
	float m_flIncomingLatency;
	float m_flLatencyJitter = 0.f;
	float m_fLerpTime;
	int m_nArrivalTick;

	// recompute the tick our current command will be processed on.
	// call this again from CreateMove once the choke decision for this command is known,
	// the frame-stage value is one full choke cycle stale by then.
	// bSendingNow: команда уходит в ЭТОМ пакете ( выстрел ), ожидание чока не добавляем
	void UpdateArrivalTick( bool bSendingNow = false );

private:
	void UpdateLerpTime( );
	void UpdateLatency( );
};

extern Animations g_Animations;