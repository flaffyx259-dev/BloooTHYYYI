#pragma once
#include "Animations.hpp"
#include <vector>
#include <deque>
#include <cfloat>

#include "Ragebot.hpp"
struct ShotInformation_t {
	//AimPoint_t m_pBestPoint;
	//AimTarget_t m_pTarget;

	int m_iTargetIndex;
	int m_iEnemyHealth;
	int m_iPredictedDamage;
	int m_iMinimalDamage;
	// never assigned by the ragebot right now (see Ragebot.cpp), so they must default
	// to values the consumers can recognise as "unknown" instead of stack garbage.
	int m_iHitchance = -1;
	int m_iTraceHitgroup = -1;
	int m_iHistoryTicks;
	int m_iShotID;
	// monotonic, not wrapped at 100 - use this for matching events to shots.
	int m_iUniqueShotID = -1;
	float m_flTime;

	LagRecord_t m_pRecord;

	std::string m_szFiredLog;

	bool m_bLogged = false;

	bool m_bTapShot = false;
	bool m_bMatched = false;
	bool m_bHadPredError = false;

	std::string m_szName = "";
	std::string m_szFlags;

	mstudiobbox_t* m_pHitbox = nullptr;

	Vector m_vecStart;
	Vector m_vecEnd;
};

struct ShotEvents_t {
	struct player_hurt_t {
		int m_iDamage;
		int m_iHealth;
		int m_iHitgroup;
		int m_iTargetIndex;
	};

	std::vector<Vector> m_vecImpacts;
	std::vector<player_hurt_t> m_vecDamage;

	ShotInformation_t m_ShotData;

	// perpendicular distance from a point to this shot's firing ray. events carry no
	// shot id, so this is what we use to attribute them to the right shot instead of
	// blindly taking the newest one.
	float DistanceToRay( const Vector &point ) const {
		Vector vecDirection = m_ShotData.m_vecEnd - m_ShotData.m_vecStart;

		const float flLength = vecDirection.Length( );
		if( flLength <= 0.f )
			return m_ShotData.m_vecStart.Distance( point );

		vecDirection /= flLength;

		// don't clamp the far end - penetration impacts land past the aim point.
		const float flAlong = std::max( 0.f, ( point - m_ShotData.m_vecStart ).Dot( vecDirection ) );

		return ( m_ShotData.m_vecStart + vecDirection * flAlong ).Distance( point );
	}

	// impacts arrive in the order the server sends them, and on penetration they
	// run from the closest to the furthest surface. for "did the bullet stop short"
	// we need the CLOSEST one to the eye position, never back( ).
	const Vector *GetNearestImpact( ) const {
		const Vector *pNearest = nullptr;
		float flBestDistance = FLT_MAX;

		for( const auto &impact : m_vecImpacts ) {
			const float flDistance = m_ShotData.m_vecStart.DistanceSquared( impact );
			if( flDistance < flBestDistance ) {
				flBestDistance = flDistance;
				pNearest = &impact;
			}
		}

		return pNearest;
	}
};

class ShotHandling {
public:
	std::string HitgroupToString( int hitgroup );
	void ProcessShots( );
	void RegisterShot( ShotInformation_t& shot );
	void GameEvent( IGameEvent* pEvent );

	bool TraceShot( ShotEvents_t* shot );

	// events carry no shot id, so pick the pending shot the event best belongs to
	// instead of always taking m_vecFilteredShots.back( ), which misattributes
	// everything while two shots are in flight (double tap, fast spray).
	ShotEvents_t* MatchShotByImpact( const Vector& vecImpact );
	ShotEvents_t* MatchShotByTarget( int nTargetIndex );

	// single place that logs a miss, so every classification path reports the same way.
	void ReportMiss( const ShotEvents_t& shot, const std::string& szReason );

	std::deque<ShotEvents_t> m_vecFilteredShots;
	std::deque<ShotEvents_t> m_vecShots;
};

extern ShotHandling g_ShotHandling;