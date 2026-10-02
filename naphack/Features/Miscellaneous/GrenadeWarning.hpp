#pragma once
#include "../../pandora.hpp"
#include "../../SDK/Classes/Player.hpp"

#include "../../SDK/Displacement.hpp"

class GrenadeWarning {
public:
	__forceinline static void TraceHull( const Vector& src, const Vector& dst, const Vector& mins, const Vector& maxs, int mask, IHandleEntity* entity, int collision_group, CGameTrace* trace ) {
		std::uintptr_t filter[ 4 ] = {
			*reinterpret_cast< std::uintptr_t* >( Engine::Displacement.Function.m_TraceFilterSimple ),
			reinterpret_cast< std::uintptr_t >( entity ),
			collision_group,
			0
		};

		auto ray = Ray_t( );

		ray.Init( src, dst, mins, maxs );

		g_pEngineTrace->TraceRay( ray, mask, reinterpret_cast< CTraceFilter* >( &filter ), trace );
	}

	static inline float CSGOArmor( float flDamage, int ArmorValue ) {
		float flArmorRatio = 0.5f;
		float flArmorBonus = 0.5f;
		if( ArmorValue > 0 ) {
			float flNew = flDamage * flArmorRatio;
			float flArmor = ( flDamage - flNew ) * flArmorBonus;

			if( flArmor > static_cast< float >( ArmorValue ) ) {
				flArmor = static_cast< float >( ArmorValue ) * ( 1.f / flArmorBonus );
				flNew = flDamage - flArmor;
			}

			flDamage = flNew;
		}
		return flDamage;
	}

	__forceinline static void TraceLine( const Vector& src, const Vector& dst, int mask, IHandleEntity* entity, int collision_group, CGameTrace* trace ) {
		std::uintptr_t filter[ 4 ] = {
			*reinterpret_cast< std::uintptr_t* >( Engine::Displacement.Function.m_TraceFilterSimple ),
			reinterpret_cast< std::uintptr_t >( entity ),
			collision_group,
			0
		};

		auto ray = Ray_t( );

		ray.Init( src, dst );

		g_pEngineTrace->TraceRay( ray, mask, reinterpret_cast< CTraceFilter* >( &filter ), trace );
	}

	template <class T>
	__forceinline T Lerp( float flPercent, T const& A, T const& B )
	{
		return A + ( B - A ) * flPercent;
	}

	struct GrenadeData_t {
		__forceinline GrenadeData_t( ) = default;

		__forceinline GrenadeData_t( C_CSPlayer* owner, int index, const Vector& origin, const Vector& velocity, float throw_time, int offset, C_BaseEntity* entity ) : GrenadeData_t( ) {
			m_pOwner = owner;
			m_iWeapIndex = index;

			// note: store an ehandle instead of a raw pointer. we intentionally keep entries around
			// after the projectile goes dormant / gets freed, so a raw pointer would dangle.
			if( entity )
				m_hNadeEntity = entity->GetRefEHandle( );

			Predict( origin, velocity, throw_time, offset );
		}

		// resolves the projectile every frame; returns nullptr once the entity is gone.
		__forceinline C_BaseEntity* GetNadeEntity( ) const {
			if( !m_hNadeEntity.IsValid( ) )
				return nullptr;

			return reinterpret_cast< C_BaseEntity* >( m_hNadeEntity.Get( ) );
		}

		bool PhysicsSimulate( );

		void PhysicsTraceEntity( const Vector& src, const Vector& dst, std::uint32_t mask, CGameTrace& trace );

		void PhysicsPushEntity( const Vector& push, CGameTrace& trace );

		void PerformFlyCollision( CGameTrace& trace );

		void Think( );

		__forceinline void Detonate( const bool bBounced ) {
			m_bDetonated = true;

			UpdatePath( bBounced );
		}

		__forceinline void UpdatePath( const bool bBounced ) {
			m_iLastUpdateTick = m_iTick;

			m_Path.emplace_back( m_vecOrigin, bBounced );
		}

		void Predict( const Vector& origin, const Vector& velocity, float throw_time, int offset );

		// result of the "how dangerous is this nade to me" evaluation
		struct Danger_t {
			bool m_bSafe{ true };
			float m_flSeverity{ 0.f };  // 0..1, used for colour + indicator distance
			int m_iDamage{ 0 };
		};

		// true when the landing spot is inside any smoke, which kills molotov spread
		static bool IsInsideSmoke( const Vector& point );

		Danger_t EvaluateDanger( C_CSPlayer* pLocal, const Vector& vecDetonation, bool bFireReached ) const;

		void DrawIndicator( C_CSPlayer* pLocal, const Vector& vecDetonation, const Danger_t& danger, float flPercent );

		bool Draw( );

		bool m_bDetonated{ }, m_bLocalPredicted{ false };
		C_CSPlayer* m_pOwner{ };
		Vector m_vecOrigin{ }, m_vecVelocity{ };
		IClientEntity* m_pLastHitEntity{ };
		CBaseHandle m_hNadeEntity{ };
		int m_iCollisionGroup{ };
		float m_flDetonationTime{ }, m_flExpireTime{ };
		int m_iWeapIndex{ }, m_iTick{ }, m_iNextThinkTick{ }, m_iLastUpdateTick{ }, m_nBounces{ };
		std::vector< std::pair< Vector, bool > > m_Path{ };

		// off-screen indicator state, smoothed per grenade instead of per process
		// (the old code used function-level statics, so multiple nades fought over one value).
		Vector2D m_vecIndicator{ };
		bool m_bIndicatorValid{ false };

		// inputs the current path was predicted from, so we can skip re-simulating
		// an unchanged projectile every single frame
		float m_flPredictedSimTime{ -1.f };
		int m_iPredictedOffset{ -1 };
	} m_data{ };

	std::unordered_map< unsigned long, GrenadeData_t > m_List{ };
public:
	std::unordered_map< unsigned long, GrenadeData_t >& GetList( ) {
		return m_List;
	}

	std::vector<std::tuple<int, float, unsigned long>> m_vecEventNades;

	void Run( C_BaseEntity* entity );

	// drops detonated / removed grenades and stale grenade_thrown events
	void Prune( );

	void PredictLocal( );
	void DrawLocal( );
};

extern GrenadeWarning g_GrenadeWarning;