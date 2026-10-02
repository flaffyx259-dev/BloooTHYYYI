#include "entity.hpp"
#include "../displacement.hpp"
#include "../sdk.hpp"
#include "Player.hpp"
#include "../../Hooking/hooked.hpp"
#include "../../Features/Miscellaneous/SkinChanger.hpp"

void IHandleEntity::SetRefEHandle( const CBaseHandle &handle ) {
	using Fn = void( __thiscall * )( void *, const CBaseHandle & );
	return Memory::VCall<Fn>( this, Index::IHandleEntity::SetRefEHandle )( this, handle );
}

const CBaseHandle &IHandleEntity::GetRefEHandle( ) const {
	using Fn = const CBaseHandle &( __thiscall * )( const IHandleEntity * );
	return Memory::VCall<Fn>( this, Index::IHandleEntity::GetRefEHandle )( this );
}

const uint32_t &IHandleEntity::GetRefEHandleRaw( ) const {
	using Fn = const uint32_t &( __thiscall * )( const IHandleEntity * );
	return Memory::VCall<Fn>( this, Index::IHandleEntity::GetRefEHandle )( this );
}

ICollideable *IClientUnknown::GetCollideable( ) {
	if( !this || g_pClientState->m_nDeltaTick( ) == -1 )
		return nullptr;

	using Fn = ICollideable * ( __thiscall * )( void * );
	return Memory::VCall<Fn>( this, Index::IClientUnknown::GetCollideable )( this );
}

IClientNetworkable *IClientUnknown::GetClientNetworkable( ) {
	if( !this || ( g_pClientState->m_nDeltaTick( ) == -1 && !g_SkinChanger.m_bSkipCheck ) )
		return nullptr;

	using Fn = IClientNetworkable * ( __thiscall * )( void * );
	return Memory::VCall<Fn>( this, Index::IClientUnknown::GetClientNetworkable )( this );
}

IClientRenderable *IClientUnknown::GetClientRenderable( ) {
	if( !this || g_pClientState->m_nDeltaTick( ) == -1 )
		return nullptr;

	using Fn = IClientRenderable * ( __thiscall * )( void * );
	return Memory::VCall<Fn>( this, Index::IClientUnknown::GetClientRenderable )( this );
}

IClientEntity *IClientUnknown::GetIClientEntity( ) {
	if( !this || g_pClientState->m_nDeltaTick( ) == -1 )
		return nullptr;

	using Fn = IClientEntity * ( __thiscall * )( void * );
	return Memory::VCall<Fn>( this, Index::IClientUnknown::GetIClientEntity )( this );
}

C_BaseEntity *IClientUnknown::GetBaseEntity( ) {
	if( !this || ( g_pClientState->m_nDeltaTick( ) == -1 && !g_SkinChanger.m_bSkipCheck ) )
		return nullptr;

	using Fn = C_BaseEntity * ( __thiscall * )( void * );
	return Memory::VCall<Fn>( this, Index::IClientUnknown::GetBaseEntity )( this );
}

Vector &ICollideable::GetCollisionOrigin( ) {
	using Fn = Vector & ( __thiscall * )( void * );
	return /*Memory::VCall<Fn>( this, Index::ICollideable::GetCollisionOrigin )( this )*/Vector( );
}

Vector &ICollideable::OBBMins( ) {
	using Fn = Vector & ( __thiscall * )( void * );
	return Memory::VCall<Fn>( this, Index::ICollideable::OBBMins )( this );
}

Vector &ICollideable::OBBMaxs( ) {
	using Fn = Vector & ( __thiscall * )( void * );
	return Memory::VCall<Fn>( this, Index::ICollideable::OBBMaxs )( this );
}

void ICollideable::CreatePartitionHandle( ) {
	using Fn = void( __thiscall * )( void * );
	//Memory::VCall<Fn>( this, Index::ICollideable::CreatePartitionHandle )( this );
}

SolidType_t ICollideable::GetSolid( ) {
	using Fn = SolidType_t( __thiscall * )( void * );
	return Memory::VCall<Fn>( this, Index::ICollideable::GetSolid )( this );
}

ClientClass *IClientNetworkable::GetClientClass( ) {
	using Fn = ClientClass * ( __thiscall * )( void * );
	return Memory::VCall<Fn>( this, Index::IClientNetworkable::GetClientClass )( this );
}

bool IClientNetworkable::IsDormant( ) {
	if( !this || this == nullptr )
		return true;

	using Fn = bool( __thiscall * )( void * );
	bool Dormant = Memory::VCall<Fn>( this, Index::IClientNetworkable::IsDormant )( this );

	return Dormant;
}

int IClientNetworkable::entindex( ) {
	using Fn = int( __thiscall * )( void * );
	return Memory::VCall<Fn>( this, Index::IClientNetworkable::entindex )( this );
}

void IClientNetworkable::SetDestroyedOnRecreateEntities( void ) {
	using Fn = void( __thiscall * )( void * );
	return Memory::VCall<Fn>( this, 13 )( this );
}

void IClientNetworkable::Release( void ) {
	using Fn = void( __thiscall * )( void * );
	return Memory::VCall<Fn>( this, 1 )( this );
}

void IClientNetworkable::OnPreDataChanged( int updateType ) {
	using Fn = void( __thiscall * )( void *, int );
	return Memory::VCall<Fn>( this, 4 )( this, updateType );
}

void IClientNetworkable::OnDataChanged( int updateType ) {
	if( !this )
		return;

	using Fn = void( __thiscall * )( void *, int );
	return Memory::VCall<Fn>( this, 5 )( this, updateType );
}

void IClientNetworkable::PreDataUpdate( int updateType ) {
	using Fn = void( __thiscall * )( void *, int );
	return Memory::VCall<Fn>( this, 6 )( this, updateType );
}

void IClientNetworkable::PostDataUpdate( int updateType ) {
	if( !this )
		return;

	using Fn = void( __thiscall * )( void *, int );
	return Memory::VCall<Fn>( this, 7 )( this, updateType );
}

const model_t *IClientRenderable::GetModel( ) {
	using Fn = const model_t *( __thiscall * )( void * );
	return Memory::VCall<Fn>( this, Index::IClientRenderable::GetModel )( this );
}

bool IClientRenderable::SetupBones( matrix3x4_t *pBoneToWorld, int nMaxBones, int boneMask, float currentTime ) {
	using Fn = bool( __thiscall * )( void *, matrix3x4_t *, int, int, float );
	return Memory::VCall<Fn>( this, Index::IClientRenderable::SetupBones )( this, pBoneToWorld, nMaxBones, boneMask, currentTime );
}

void IClientRenderable::GetRenderBounds( Vector &mins, Vector &maxs ) {
	using Fn = void( __thiscall * )( void *, Vector &, Vector & );
	return Memory::VCall<Fn>( this, Index::IClientRenderable::RenderBounds )( this, mins, maxs );
}

ClientRenderHandle_t &IClientRenderable::RenderHandle( ) {
	using Fn = ClientRenderHandle_t & ( __thiscall * )( void * );
	return Memory::VCall<Fn>( this, Index::IClientRenderable::RenderHandle )( this );
}

Vector &IClientEntity::OBBMins( ) {
	auto collideable = GetCollideable( );
	if( !collideable )
		return Vector( );

	return collideable->OBBMins( );
}

Vector &IClientEntity::OBBMaxs( ) {
	auto collideable = GetCollideable( );
	if( !collideable )
		return Vector( );

	return collideable->OBBMaxs( );
}

Vector &IClientEntity::GetAbsOrigin( ) {
	using Fn = Vector & ( __thiscall * )( void * );
	return Memory::VCall<Fn>( this, Index::IClientEntity::GetAbsOrigin )( this );
}

QAngle &IClientEntity::GetAbsAngles( ) {
	using Fn = QAngle & ( __thiscall * )( void * );
	return Memory::VCall<Fn>( this, Index::IClientEntity::GetAbsAngles )( this );
}

ClientClass *IClientEntity::GetClientClass( ) {
	auto networkable = GetClientNetworkable( );
	if( !networkable )
		return nullptr;

	return networkable->GetClientClass( );
}

bool IClientEntity::IsDormant( ) {
	if( !this || this == nullptr )
		return true;

	auto networkable = GetClientNetworkable( );
	if( !networkable )
		return true;

	return networkable->IsDormant( );
}

int IClientEntity::EntIndex( ) {
	if( !this || this == nullptr )
		return -1;

	return this->m_entIndex;

	//return networkable->entindex( );
}

const model_t *IClientEntity::GetModel( ) {
	if( !this )
		return nullptr;

	auto renderable = GetClientRenderable( );
	if( !renderable )
		return nullptr;

	return renderable->GetModel( );
}

bool IClientEntity::SetupBones( matrix3x4_t *pBoneToWorld, int nMaxBones, int boneMask, float currentTime ) {
	if( !this )
		return nullptr;

	auto renderable = GetClientRenderable( );
	if( !renderable )
		return nullptr;

	return renderable->SetupBones( pBoneToWorld, nMaxBones, boneMask, currentTime );
}

CIKContext *CIKContext::Allocate( ) {
	typedef void( __thiscall *Construct )( void * );
	auto context = reinterpret_cast< CIKContext * >( g_pMemAlloc->Alloc( 0x1070 ) );
	( ( Construct )Engine::Displacement.CIKContext.m_nConstructor )( context );
	return context;
}

void CIKContext::Construct( ) {
	typedef void( __thiscall *IKConstructEx )( void * );
	auto ik_ctor = ( IKConstructEx )Engine::Displacement.CIKContext.m_nConstructor;
	ik_ctor( this );
}

void CIKContext::Destructor( ) {
	typedef void( __thiscall *IKDestructor )( CIKContext * );
	auto ik_dector = ( IKDestructor )Engine::Displacement.CIKContext.m_nDestructor;
	ik_dector( this );
}

// This somehow got inlined so we need to rebuild it
void CIKContext::ClearTargets( ) {
	static auto constexpr TARGET_SIZE = 85;

	auto i = 0;
	auto count = *reinterpret_cast< int * >( reinterpret_cast< uint32_t >( this ) + static_cast< ptrdiff_t >( 4080 ) );

	if( count > 0 ) {
		auto target = reinterpret_cast< int * >( reinterpret_cast< uint32_t >( this ) + static_cast< ptrdiff_t >( 208 ) );
		do {
			*target = -9999;
			target += TARGET_SIZE;
			++i;
		} while( i < count );
	}
}

void CIKContext::Init( CStudioHdr *hdr, QAngle *angles, Vector *origin, float currentTime, int frames, int boneMask ) {
	typedef void( __thiscall *Init_t )( void *, CStudioHdr *, QAngle *, Vector *, float, int, int );
	auto ik_init = Engine::Displacement.CIKContext.m_nInit;
	( ( Init_t )ik_init )( this, hdr, angles, origin, currentTime, frames, boneMask );
}

void CIKContext::UpdateTargets( Vector *pos, Quaternion *qua, matrix3x4_t *matrix, uint8_t *boneComputed ) {
	typedef void( __thiscall *UpdateTargets_t )( void *, Vector *, Quaternion *, matrix3x4_t *, uint8_t * );
	auto  ik_update_targets = Engine::Displacement.CIKContext.m_nUpdateTargets;
	( ( UpdateTargets_t )ik_update_targets )( this, pos, qua, matrix, boneComputed );
}

void CIKContext::SolveDependencies( Vector *pos, Quaternion *qua, matrix3x4_t *matrix, uint8_t *boneComputed ) {
	typedef void( __thiscall *SolveDependencies_t )( void *, Vector *, Quaternion *, matrix3x4_t *, uint8_t * );
	auto  ik_solve_dependencies = Engine::Displacement.CIKContext.m_nSolveDependencies;
	( ( SolveDependencies_t )ik_solve_dependencies )( this, pos, qua, matrix, boneComputed );
}

bool C_BaseEntity::IsPlayer( ) {
	if( !this || g_pClientState->m_nDeltaTick( ) == -1 )
		return false;

	using Fn = bool( __thiscall * )( void * );
	return Memory::VCall<Fn>( this, Index::C_BaseEntity::IsPlayer )( this );
}

bool C_BaseEntity::IsWeapon( ) {
	using Fn = bool( __thiscall * )( void * );
	return Memory::VCall<Fn>( this, Index::C_BaseEntity::IsWeapon )( this );
}

void C_BaseEntity::SetAbsVelocity( const Vector &velocity ) {
	static auto m_vecAbsVelocity = SDK::Memory::FindInDataMap( this->GetPredDescMap( ), XorStr( "m_vecAbsVelocity" ) );
	*( Vector * )( ( uintptr_t )this + m_vecAbsVelocity ) = velocity;

	//InvalidatePhysicsRecursive( VELOCITY_CHANGED );
}

Vector &C_BaseEntity::GetAbsVelocity( ) {
	static auto m_vecAbsVelocity = SDK::Memory::FindInDataMap( this->GetPredDescMap( ), XorStr( "m_vecAbsVelocity" ) );
	return *( Vector * )( ( uintptr_t )this + m_vecAbsVelocity );
}

void C_BaseEntity::SetAbsOrigin( const Vector &origin ) {
	reinterpret_cast< void( __thiscall * )( void *, const Vector & ) >( Engine::Displacement.Function.m_uSetAbsOrigin )( this, origin );
}

void C_BaseEntity::InvalidatePhysicsRecursive( int change_flags ) {
	reinterpret_cast< void( __thiscall * )( void *, int ) >( Engine::Displacement.Function.m_uInvalidatePhysics )( this, change_flags );
}

void C_BaseEntity::SetAbsAngles( const QAngle &angles ) {
	reinterpret_cast< void( __thiscall * )( void *, const QAngle & ) >( Engine::Displacement.Function.m_uSetAbsAngles )( this, angles );
}

std::uint8_t &C_BaseEntity::m_MoveType( ) {
	return *( std::uint8_t * )( ( uintptr_t )this + Engine::Displacement.C_BaseEntity.m_MoveType );
}

int &C_BaseEntity::m_iMoveState( ) {
	return *( int * )( ( uintptr_t )this + Engine::Displacement.DT_CSPlayer.m_iMoveState );
}

matrix3x4_t &C_BaseEntity::m_rgflCoordinateFrame( ) {
	return *( matrix3x4_t * )( ( uintptr_t )this + Engine::Displacement.C_BaseEntity.m_rgflCoordinateFrame );
}

Vector &C_BaseEntity::m_vecOldOrigin( ) {
	// https://i.imgur.com/yTpaFxb.png
	return *( Vector * )( ( uintptr_t )this + 0x3AC );
}

int &C_BaseEntity::m_CollisionGroup( ) {
	return *( int * )( ( uintptr_t )this + Engine::Displacement.DT_BaseEntity.m_CollisionGroup );
}

CCollisionProperty *C_BaseEntity::m_Collision( ) {
	return ( CCollisionProperty * )( ( uintptr_t )this + Engine::Displacement.DT_BaseEntity.m_Collision );
}

//CParticleProperty *C_BaseEntity::m_Particles( ) {
//	return ( CParticleProperty * )( ( uintptr_t )this + Engine::Displacement.DT_BaseEntity.m_Particles );
//}

int &C_BaseEntity::m_fEffects( ) {
	return *( int * )( ( uintptr_t )this + Engine::Displacement.DT_BaseEntity.m_fEffects );
}

bool &C_BaseEntity::m_bIsJiggleBonesEnabled( ) {
	return *( bool * )( ( uintptr_t )this + Engine::Displacement.C_BaseAnimating.m_bIsJiggleBonesEnabled );
}

int &C_BaseEntity::m_iEFlags( ) {
	return *( int * )( ( uintptr_t )this + Engine::Displacement.DT_BaseEntity.m_iEFlags );
}

void C_BaseEntity::BuildTransformations( CStudioHdr *hdr, Vector *pos, Quaternion *q, const matrix3x4_t &transform, int mask, uint8_t *computed ) {
	using BuildTransformations_t = void( __thiscall * )( decltype( this ), CStudioHdr *, Vector *, Quaternion *, matrix3x4_t const &, int, uint8_t * );
	return Memory::VCall< BuildTransformations_t >( this, 184 )( this, hdr, pos, q, transform, mask, computed );
}

void C_BaseEntity::StandardBlendingRules( CStudioHdr *hdr, Vector *pos, Quaternion *q, float time, int mask ) {
	using StandardBlendingRules_t = void( __thiscall * )( decltype( this ), CStudioHdr *, Vector *, Quaternion *, float, int );
	return Memory::VCall< StandardBlendingRules_t >( this, 200 )( this, hdr, pos, q, time, mask );
}

CIKContext *&C_BaseEntity::m_pIk( ) {
	return *( CIKContext ** )( ( uintptr_t )this + Engine::Displacement.C_BaseAnimating.m_pIk );
}

bool C_BaseEntity::Teleported( ) {
	// Purpose: Determine whether entity was teleported ( so we can disable interpolation )
	auto this_ptr = reinterpret_cast< uintptr_t >( this );

	// The moveparent received from networking data
	// notice: m_hNetworkMoveParent - is datamap, but m_hOldMoveParent isnt
	auto m_hNetworkMoveParent = *reinterpret_cast< CHandle< C_BaseEntity >* >( this_ptr + 0x148 );
	auto m_hOldMoveParent = *reinterpret_cast< CHandle< C_BaseEntity >* >( this_ptr + 0x314 );

	// Disable interpolation when hierarchy changes
	if( m_hOldMoveParent.Get( ) != m_hNetworkMoveParent.Get( ) )
		return true;

	auto parent_attachment = *( uint8_t * )( this_ptr + 0x2ED );
	auto old_parent_attachment = *( uint8_t * )( this_ptr + 0x2EC );
	if( parent_attachment != old_parent_attachment )
		return true;

	return false;
}

int &C_BaseEntity::m_nSequence( ) {
	static DWORD32 offset = Engine::g_PropManager.GetOffset( XorStr( "DT_BaseAnimating" ), XorStr( "m_nSequence" ) );
	return *( int * )( uintptr_t( this ) + offset );
}

int &C_BaseEntity::m_nAnimationParity( ) {
	static DWORD32 offset = Engine::g_PropManager.GetOffset( XorStr( "DT_BaseViewModel" ), XorStr( "m_nAnimationParity" ) );
	return *( int * )( uintptr_t( this ) + offset );
}

int &C_BaseEntity::m_nNewSequenceParity( ) {
	static DWORD32 offset = Engine::g_PropManager.GetOffset( XorStr( "DT_BaseViewModel" ), XorStr( "m_nNewSequenceParity" ) );
	return *( int * )( uintptr_t( this ) + offset );
}

float &C_BaseEntity::m_flAnimTime( ) {
	static DWORD32 offset = Engine::g_PropManager.GetOffset( XorStr( "DT_BaseEntity" ), XorStr( "m_flAnimTime" ) );
	return *( float * )( uintptr_t( this ) + offset );
}

float &C_BaseEntity::m_flOldAnimTime( ) {
	static DWORD32 offset = Engine::g_PropManager.GetOffset( XorStr( "DT_BaseEntity" ), XorStr( "m_flAnimTime" ) ) + 4;
	return *( float * )( uintptr_t( this ) + offset );
}

float &C_BaseEntity::m_flCycle( ) {
	static DWORD32 offset = Engine::g_PropManager.GetOffset( XorStr( "DT_BaseAnimating" ), XorStr( "m_flCycle" ) );
	return *( float * )( uintptr_t( this ) + offset );
}

float &C_BaseEntity::m_flOldCycle( ) {
	static DWORD32 offset = Engine::g_PropManager.GetOffset( XorStr( "DT_BaseAnimating" ), XorStr( "m_nSequence" ) ) + 0x30;
	return *( float * )( uintptr_t( this ) + offset );
}

float &C_BaseEntity::m_flPlaybackRate( ) {
	static DWORD32 offset = Engine::g_PropManager.GetOffset( XorStr( "DT_BaseAnimating" ), XorStr( "m_flPlaybackRate" ) );
	return *( float * )( uintptr_t( this ) + offset );
}

int &C_BaseEntity::m_iTeamNum( ) {
	return *( int * )( ( uintptr_t )this + Engine::Displacement.DT_BaseEntity.m_iTeamNum );
}

bool C_BaseEntity::IsPlantedC4( ) {
	return GetClientClass( )->m_ClassID == ClassId_t::CPlantedC4;
}

Vector &C_BaseEntity::m_vecOrigin( ) {
	return *( Vector * )( ( uintptr_t )this + Engine::Displacement.DT_BaseEntity.m_vecOrigin );
}

void C_BaseEntity::UpdateVisibilityAllEntities( ) {
	if( Engine::Displacement.C_BasePlayer.UpdateVisibilityAllEntities )
		reinterpret_cast< void( __thiscall * )( void * ) >( Engine::Displacement.C_BasePlayer.UpdateVisibilityAllEntities )( this );
}

float &C_PlantedC4::m_flC4Blow( ) {
	return *( float * )( ( uintptr_t )this + Engine::Displacement.DT_PlantedC4.m_flC4Blow );
}

float &C_PlantedC4::m_flDefuseCountDown( ) {
	return *( float * )( ( uintptr_t )this + Engine::Displacement.DT_PlantedC4.m_flDefuseCountDown );
}

bool &C_PlantedC4::m_bBombDefused( ) {
	return *( bool * )( ( uintptr_t )this + Engine::Displacement.DT_PlantedC4.m_bBombDefused );
}

bool &C_PlantedC4::m_bBeingDefused( ) {
	return *( bool * )( ( uintptr_t )this + 0x0A20 );
}

float &C_BaseEntity::m_flSimulationTime( ) {
	return *( float * )( ( uintptr_t )this + Engine::Displacement.DT_BaseEntity.m_flSimulationTime );
}

float &C_BaseEntity::m_flOldSimulationTime( ) {
	return *( float * )( ( uintptr_t )this + Engine::Displacement.DT_BaseEntity.m_flSimulationTime + 0x4 );
}

void C_BaseEntity::SetPredictionRandomSeed( int seed ) {
	*( int * )( Engine::Displacement.Data.m_uPredictionRandomSeed ) = seed;
}

void C_BaseEntity::SetPredictionPlayer( C_BasePlayer *player ) {
	*( C_BasePlayer ** )( Engine::Displacement.Data.m_uPredictionPlayer ) = player;
}

CBaseHandle &C_BaseEntity::m_hOwnerEntity( ) {
	return *( CBaseHandle * )( ( uintptr_t )this + Engine::Displacement.DT_BaseEntity.m_hOwnerEntity );;
}

CBaseHandle &C_BaseEntity::moveparent( ) {
	return *( CBaseHandle * )( ( uintptr_t )this + Engine::Displacement.DT_BaseEntity.moveparent );;
}

CBaseHandle &C_BaseEntity::m_hCombatWeaponParent( ) {
	return *( CBaseHandle * )( ( uintptr_t )this + Engine::Displacement.DT_BaseWeaponWorldModel.m_hCombatWeaponParent );;
}

void C_BaseAnimating::UpdateClientSideAnimation( ) {
	g_Vars.globals.m_bUpdatingAnimations = true;
	using Fn = void( __thiscall * )( void * );
	Memory::VCall<Fn>( this, Index::C_BaseAnimating::UpdateClientSideAnimation )( this );
	g_Vars.globals.m_bUpdatingAnimations = false;
}

void C_BaseAnimating::InvalidateBoneCache( ) {
	*( uint32_t * )( &m_flLastBoneSetupTime( ) ) = 0xFF7FFFFF;
	m_iMostRecentModelBoneCounter( ) = 0;
	m_BoneAccessor( ).m_ReadableBones = m_BoneAccessor( ).m_WritableBones = 0;
}

void C_BaseAnimating::ForceBoneCache( ) {
	m_iMostRecentModelBoneCounter( ) = *( int * )Engine::Displacement.Data.m_uModelBoneCounter;
	m_BoneAccessor( ).m_ReadableBones = m_BoneAccessor( ).m_WritableBones = 0xFFFFFFFF;
	m_flLastBoneSetupTime( ) = FLT_MAX;
}

void C_BaseAnimating::LockStudioHdr( ) {
	auto _LockStudioHdr = ( void( __thiscall * )( void * ) )Engine::Displacement.Function.m_LockStudioHdr;
	_LockStudioHdr( this );
}

void C_BaseEntity::ControlMouth( CStudioHdr *pStudioHdr ) {
	typedef void( __thiscall *ControlMouthFn )( void *, CStudioHdr * );
	Memory::VCall< ControlMouthFn >( this, 192 )( this, pStudioHdr );
}

bool C_BaseAnimating::ComputeHitboxSurroundingBox( Vector &mins, Vector &maxs, const matrix3x4_t *boneTransform ) {
	auto model = GetModel( );
	if( !model )
		return false;

	auto hdr = g_pModelInfo->GetStudiomodel( model );
	if( !hdr )
		return false;

	mstudiohitboxset_t *set = hdr->pHitboxSet( m_nHitboxSet( ) );
	if( !set || !set->numhitboxes )
		return false;

	const matrix3x4_t *bones = boneTransform ? boneTransform : this->m_BoneAccessor( ).m_pBones;
	mins.Init( FLT_MAX, FLT_MAX, FLT_MAX );
	maxs.Init( -FLT_MAX, -FLT_MAX, -FLT_MAX );
	Vector abs_min, abs_max;

	for( int i = 0; i < set->numhitboxes; i++ ) {
		mstudiobbox_t *pbox = set->pHitbox( i );

		bones[ pbox->bone ].TransformAABB( pbox->bbmin, pbox->bbmax, abs_min, abs_max );

		mins = abs_min.Min( mins );
		maxs = abs_max.Max( maxs );
	}

	return true;
}

int C_BaseAnimating::GetSequenceActivity( int sequence ) {
	auto model = this->GetModel( );
	if( !model )
		return -1;

	auto hdr = g_pModelInfo->GetStudiomodel( model );
	if( !hdr )
		return -1;

	// sig for studiohdr_t version: 53 56 8B F1 8B DA 85 F6 74 55
	// sig for C_BaseAnimating version: 55 8B EC 83 7D 08 FF 56 8B F1 74 3D
	// c_csplayer vfunc 242, follow calls to find the function.
	return reinterpret_cast< int( __fastcall * )( void *, studiohdr_t *, int ) >( Engine::Displacement.Function.m_uGetSequenceActivity )( this, hdr, sequence );
}

int C_BaseAnimating::LookupSequence( const char *label ) {
	typedef int( __thiscall *fnLookupSequence )( void *, const char * );
	return ( ( fnLookupSequence )Engine::Displacement.Function.m_uLookupSequence ) ( this, label );
}

void C_BaseAnimating::UpdateIKLocks( float time ) {
	typedef void( __thiscall *oCalculateIKLocks )( void *, float );
	Memory::VCall< oCalculateIKLocks >( this, 192 )( this, time );
}

void C_BaseAnimating::CalculateIKLocks( float time ) {
	typedef void( __thiscall *oCalculateIKLocks )( void *, float );
	Memory::VCall< oCalculateIKLocks >( this, 193 )( this, time );
}

int &C_BaseAnimating::m_nHitboxSet( ) {
	return *( int * )( ( uintptr_t )this + Engine::Displacement.DT_BaseAnimating.m_nHitboxSet );
}

bool &C_BaseAnimating::m_bIsLookingAtWeapon( ) {
	return *( bool * )( ( uintptr_t )this + Engine::Displacement.DT_BaseAnimating.m_bIsLookingAtWeapon );
}

int &C_BaseAnimating::m_iMostRecentModelBoneCounter( ) {
	return *( int * )( ( uintptr_t )this + Engine::Displacement.C_BaseAnimating.m_iMostRecentModelBoneCounter );
}

int &C_BaseAnimating::m_iPrevBoneMask( ) {
	return *( int * )( ( uintptr_t )this + Engine::Displacement.C_BaseAnimating.m_iPrevBoneMask );
}

int &C_BaseAnimating::m_iAccumulatedBoneMask( ) {
	return *( int * )( ( uintptr_t )this + Engine::Displacement.C_BaseAnimating.m_iAccumulatedBoneMask );
}

int &C_BaseAnimating::m_iOcclusionFramecount( ) {
	return *( int * )( ( uintptr_t )this + Engine::Displacement.C_BaseAnimating.m_iOcclusionFramecount );
}

int &C_BaseAnimating::m_iOcclusionFlags( ) {
	return *( int * )( ( uintptr_t )this + Engine::Displacement.C_BaseAnimating.m_iOcclusionFlags );
}

bool &C_BaseAnimating::m_bClientSideAnimation( ) {
	return *( bool * )( ( uintptr_t )this + Engine::Displacement.DT_BaseAnimating.m_bClientSideAnimation );
}

bool &C_BaseAnimating::m_bClientSideRagdoll( ) {
	return *( bool * )( ( uintptr_t )this + Engine::Displacement.DT_BaseAnimating.m_bClientSideRagdoll );
}

bool &C_BaseAnimating::m_bShouldDraw( ) {
	return *( bool * )( ( uintptr_t )this + Engine::Displacement.C_BaseAnimating.m_bShouldDraw );
}

float &C_BaseAnimating::m_flLastBoneSetupTime( ) {
	return *( float * )( ( uintptr_t )this + Engine::Displacement.C_BaseAnimating.m_flLastBoneSetupTime );
}

float *C_BaseAnimating::m_flPoseParameter( ) {
	return ( float * )( ( uintptr_t )this + Engine::Displacement.DT_BaseAnimating.m_flPoseParameter );
}

CBoneAccessor &C_BaseAnimating::m_BoneAccessor( ) {
	return *( CBoneAccessor * )( ( uintptr_t )this + Engine::Displacement.C_BaseAnimating.m_BoneAccessor );
}

CUtlVector<matrix3x4_t> &C_BaseAnimating::m_CachedBoneData( ) {
	return *( CUtlVector<matrix3x4_t>* )( ( uintptr_t )this + Engine::Displacement.C_BaseAnimating.m_CachedBoneData );
}

CUtlVector<C_AnimationLayer> &C_BaseAnimating::m_AnimOverlay( ) {
	return *( CUtlVector<C_AnimationLayer>* )( ( uintptr_t )this + Engine::Displacement.C_BaseAnimating.m_AnimOverlay );;
}

Vector *C_BaseAnimating::m_vecBonePos( ) {
	return ( Vector * )( ( uintptr_t )this + Engine::Displacement.C_BaseAnimating.m_nCachedBonesPosition );
}

Quaternion *C_BaseAnimating::m_quatBoneRot( ) {
	return ( Quaternion * )( ( uintptr_t )this + Engine::Displacement.C_BaseAnimating.m_nCachedBonesRotation );;
}

CStudioHdr *&C_BaseAnimating::m_pStudioHdr( ) {
	return *( CStudioHdr ** )( ( uintptr_t )this + ( Engine::Displacement.C_BaseAnimating.m_pStudioHdr ) );
}

CBaseHandle &C_BaseCombatCharacter::m_hActiveWeapon( ) {
	return *( CBaseHandle * )( ( uintptr_t )this + Engine::Displacement.DT_BaseCombatCharacter.m_hActiveWeapon );
}

float &C_BaseCombatCharacter::m_flNextAttack( ) {
	return *( float * )( ( uintptr_t )this + Engine::Displacement.DT_BaseCombatCharacter.m_flNextAttack );
}

CBaseHandle *C_BaseCombatCharacter::m_hMyWeapons( ) {
	return ( CBaseHandle * )( ( uintptr_t )this + Engine::Displacement.DT_BaseCombatCharacter.m_hMyWeapons );
}

CBaseHandle *C_BaseCombatCharacter::m_hMyWearables( ) {
	return ( CBaseHandle * )( ( uintptr_t )this + Engine::Displacement.DT_BaseCombatCharacter.m_hMyWearables );
}

float C_Inferno::m_flSpawnTime( ) {
	return *( float * )( ( uintptr_t )this + 0x20 );
}

int *C_Inferno::m_fireXDelta( ) {
	static auto offset = Engine::g_PropManager.GetOffset( XorStr( "DT_Inferno" ), XorStr( "m_fireXDelta" ) );

	return ( int * )( ( uintptr_t )this + offset );
}

int *C_Inferno::m_fireYDelta( ) {
	static auto offset = Engine::g_PropManager.GetOffset( XorStr( "DT_Inferno" ), XorStr( "m_fireYDelta" ) );

	return ( int * )( ( uintptr_t )this + offset );
}

int *C_Inferno::m_fireZDelta( ) {
	static auto offset = Engine::g_PropManager.GetOffset( XorStr( "DT_Inferno" ), XorStr( "m_fireZDelta" ) );

	return ( int * )( ( uintptr_t )this + offset );
}

int C_Inferno::m_fireCount( ) {
	static auto offset = Engine::g_PropManager.GetOffset( XorStr( "DT_Inferno" ), XorStr( "m_fireCount" ) );

	return *( int * )( ( uintptr_t )this + offset );
}

int C_SmokeGrenadeProjectile::m_nSmokeEffectTickBegin( ) {
	return *( int * )( ( uintptr_t )this + Engine::Displacement.DT_SmokeGrenadeProjectile.m_nSmokeEffectTickBegin );
}

bool C_SmokeGrenadeProjectile::m_bDidSmokeEffect( ) {
	return *( bool * )( ( uintptr_t )this + Engine::Displacement.DT_SmokeGrenadeProjectile.m_bDidSmokeEffect );
}

void CCollisionProperty::SetCollisionBounds( const Vector &mins, const Vector &maxs ) {
	using Fn = void( __thiscall * )( CCollisionProperty *, const Vector &, const Vector & );
	static auto mem = ( Fn )Engine::Displacement.Function.m_SetCollisionBounds;
	mem( this, mins, maxs );
}

__forceinline static matrix3x4_t multiply_matrix( matrix3x4_t in1, matrix3x4_t in2 ) {
	matrix3x4_t result{};
	result[ 0 ][ 0 ] = in1[ 0 ][ 0 ] * in2[ 0 ][ 0 ] + in1[ 0 ][ 1 ] * in2[ 1 ][ 0 ] +
		in1[ 0 ][ 2 ] * in2[ 2 ][ 0 ];
	result[ 0 ][ 1 ] = in1[ 0 ][ 0 ] * in2[ 0 ][ 1 ] + in1[ 0 ][ 1 ] * in2[ 1 ][ 1 ] +
		in1[ 0 ][ 2 ] * in2[ 2 ][ 1 ];
	result[ 0 ][ 2 ] = in1[ 0 ][ 0 ] * in2[ 0 ][ 2 ] + in1[ 0 ][ 1 ] * in2[ 1 ][ 2 ] +
		in1[ 0 ][ 2 ] * in2[ 2 ][ 2 ];
	result[ 0 ][ 3 ] = in1[ 0 ][ 0 ] * in2[ 0 ][ 3 ] + in1[ 0 ][ 1 ] * in2[ 1 ][ 3 ] +
		in1[ 0 ][ 2 ] * in2[ 2 ][ 3 ] + in1[ 0 ][ 3 ];
	result[ 1 ][ 0 ] = in1[ 1 ][ 0 ] * in2[ 0 ][ 0 ] + in1[ 1 ][ 1 ] * in2[ 1 ][ 0 ] +
		in1[ 1 ][ 2 ] * in2[ 2 ][ 0 ];
	result[ 1 ][ 1 ] = in1[ 1 ][ 0 ] * in2[ 0 ][ 1 ] + in1[ 1 ][ 1 ] * in2[ 1 ][ 1 ] +
		in1[ 1 ][ 2 ] * in2[ 2 ][ 1 ];
	result[ 1 ][ 2 ] = in1[ 1 ][ 0 ] * in2[ 0 ][ 2 ] + in1[ 1 ][ 1 ] * in2[ 1 ][ 2 ] +
		in1[ 1 ][ 2 ] * in2[ 2 ][ 2 ];
	result[ 1 ][ 3 ] = in1[ 1 ][ 0 ] * in2[ 0 ][ 3 ] + in1[ 1 ][ 1 ] * in2[ 1 ][ 3 ] +
		in1[ 1 ][ 2 ] * in2[ 2 ][ 3 ] + in1[ 1 ][ 3 ];
	result[ 2 ][ 0 ] = in1[ 2 ][ 0 ] * in2[ 0 ][ 0 ] + in1[ 2 ][ 1 ] * in2[ 1 ][ 0 ] +
		in1[ 2 ][ 2 ] * in2[ 2 ][ 0 ];
	result[ 2 ][ 1 ] = in1[ 2 ][ 0 ] * in2[ 0 ][ 1 ] + in1[ 2 ][ 1 ] * in2[ 1 ][ 1 ] +
		in1[ 2 ][ 2 ] * in2[ 2 ][ 1 ];
	result[ 2 ][ 2 ] = in1[ 2 ][ 0 ] * in2[ 0 ][ 2 ] + in1[ 2 ][ 1 ] * in2[ 1 ][ 2 ] +
		in1[ 2 ][ 2 ] * in2[ 2 ][ 2 ];
	result[ 2 ][ 3 ] = in1[ 2 ][ 0 ] * in2[ 0 ][ 3 ] + in1[ 2 ][ 1 ] * in2[ 1 ][ 3 ] +
		in1[ 2 ][ 2 ] * in2[ 2 ][ 3 ] + in1[ 2 ][ 3 ];
	return result;
}

__forceinline static matrix3x4_t angle_matrix( const QAngle angles ) {
	matrix3x4_t result{};

	m128 angle, sin, cos;
	angle.f[ 0 ] = DEG2RAD( angles.x );
	angle.f[ 1 ] = DEG2RAD( angles.y );
	angle.f[ 2 ] = DEG2RAD( angles.z );
	sincos_ps( angle.v, &sin.v, &cos.v );

	result[ 0 ][ 0 ] = cos.f[ 0 ] * cos.f[ 1 ];
	result[ 1 ][ 0 ] = cos.f[ 0 ] * sin.f[ 1 ];
	result[ 2 ][ 0 ] = -sin.f[ 0 ];

	const auto crcy = cos.f[ 2 ] * cos.f[ 1 ];
	const auto crsy = cos.f[ 2 ] * sin.f[ 1 ];
	const auto srcy = sin.f[ 2 ] * cos.f[ 1 ];
	const auto srsy = sin.f[ 2 ] * sin.f[ 1 ];

	result[ 0 ][ 1 ] = sin.f[ 0 ] * srcy - crsy;
	result[ 1 ][ 1 ] = sin.f[ 0 ] * srsy + crcy;
	result[ 2 ][ 1 ] = sin.f[ 2 ] * cos.f[ 0 ];

	result[ 0 ][ 2 ] = sin.f[ 0 ] * crcy + srsy;
	result[ 1 ][ 2 ] = sin.f[ 0 ] * crsy - srcy;
	result[ 2 ][ 2 ] = cos.f[ 2 ] * cos.f[ 0 ];

	return result;
}

__forceinline static matrix3x4_t angle_matrix( const QAngle angle, const Vector pos ) {
	auto result = angle_matrix( angle );
	result[ 0 ][ 3 ] = pos.x;
	result[ 1 ][ 3 ] = pos.y;
	result[ 2 ][ 3 ] = pos.z;
	return result;
}

__forceinline static QAngle matrix_angles( matrix3x4_t &matrix ) {
	float forward[ 3 ];
	float left[ 3 ];
	float up[ 3 ];

	forward[ 0 ] = matrix[ 0 ][ 0 ];
	forward[ 1 ] = matrix[ 1 ][ 0 ];
	forward[ 2 ] = matrix[ 2 ][ 0 ];

	left[ 0 ] = matrix[ 0 ][ 1 ];
	left[ 1 ] = matrix[ 1 ][ 1 ];
	left[ 2 ] = matrix[ 2 ][ 1 ];

	up[ 2 ] = matrix[ 2 ][ 2 ];

	m128 a1;
	a1.f[ 0 ] = forward[ 0 ] * forward[ 0 ] + forward[ 1 ] * forward[ 1 ];
	auto calc = sqrt_ps( a1.v );
	const auto xy_dist = reinterpret_cast< const m128 * >( &calc )->f[ 0 ];

	if( xy_dist > 0.001f )
		return QAngle( RAD2DEG( atan2f( -forward[ 2 ], xy_dist ) ),
					   RAD2DEG( atan2f( forward[ 1 ], forward[ 0 ] ) ),
					   RAD2DEG( atan2f( left[ 2 ], up[ 2 ] ) ) );

	return QAngle( RAD2DEG( atan2f( -forward[ 2 ], xy_dist ) ),
				   RAD2DEG( atan2f( -left[ 0 ], left[ 1 ] ) ),
				   0 );
}


void C_BaseEntity::DrawHitboxMatrix( matrix3x4_t *bones, mstudiobbox_t *pHitbox, float flDuration, Color clr ) {
	if( !bones )
		return;

	if( !this->GetModel( ) )
		return;

	auto pHdr = g_pModelInfo->GetStudiomodel( this->GetModel( ) );
	if( !pHdr )
		return;

	auto pHitboxSet = pHdr->pHitboxSet( ( ( C_CSPlayer * )this )->m_nHitboxSet( ) );
	if( !pHitboxSet )
		return;

	bool bCustomColor = ( clr.r( ) + clr.g( ) + clr.b( ) ) != 6;

	auto RenderCapsule = [&] ( mstudiobbox_t *hitbox ) {
		if( !hitbox )
			return;

		if( hitbox->m_flRadius <= 0.f ) {
			const auto position = Math::MatrixGetOrigin( bones[ hitbox->bone ] );
			const auto roation = angle_matrix( hitbox->m_angAngles );
			auto transform = multiply_matrix( bones[ hitbox->bone ], roation );
			const auto angles = matrix_angles( transform );

			return g_pDebugOverlay->AddBoxOverlay( position, hitbox->bbmin, hitbox->bbmax, angles,
												   bCustomColor ? clr.r( ) : g_Vars.esp.target_capsules_color.r * 255.f,
												   bCustomColor ? clr.g( ) : g_Vars.esp.target_capsules_color.g * 255.f,
												   bCustomColor ? clr.b( ) : g_Vars.esp.target_capsules_color.b * 255.f,
												   bCustomColor ? clr.a( ) : ( g_Vars.esp.target_capsules_color.a * 255.f ) / 2.f,
												   flDuration == 0.f ? g_Vars.esp.chams_hitmatrix_duration : flDuration );
		}

		auto min = hitbox->bbmin.Transform( bones[ hitbox->bone ] );
		auto max = hitbox->bbmax.Transform( bones[ hitbox->bone ] );

		g_pDebugOverlay->AddCapsuleOverlay( min, max, hitbox->m_flRadius,
											bCustomColor ? clr.r( ) : g_Vars.esp.target_capsules_color.r * 255.f,
											bCustomColor ? clr.g( ) : g_Vars.esp.target_capsules_color.g * 255.f,
											bCustomColor ? clr.b( ) : g_Vars.esp.target_capsules_color.b * 255.f,
											bCustomColor ? clr.a( ) : g_Vars.esp.target_capsules_color.a * 255.f,
											flDuration == 0.f ? g_Vars.esp.chams_hitmatrix_duration : flDuration, true );
	};

	if( pHitbox ) {
		if( pHitbox->group != Hitgroup_Stomach && pHitbox->group != Hitgroup_Chest ) {
			RenderCapsule( pHitbox );
			return;
		}
	}

	for( int i = 0; i < pHitboxSet->numhitboxes; ++i ) {
		auto hitbox = pHitboxSet->pHitbox( i );
		if( !hitbox )
			continue;

		if( pHitbox ) {
			if( pHitbox->group == Hitgroup_Stomach || pHitbox->group == Hitgroup_Chest ) {
				if( hitbox->group != Hitgroup_Stomach && hitbox->group != Hitgroup_Chest ) {
					continue;
				}
			}
		}

		RenderCapsule( hitbox );
	}
}

//void CNewParticleEffect::SetControlPoint( int nWhichPoint, const Vector &v ) {
//	using SetControlPoint_t = void( __thiscall * )( void *, int, const Vector & );
//	static auto SetControlPointFn = reinterpret_cast< SetControlPoint_t >( Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 53 8B 5D 0C 56" ) ) );
//
//	SetControlPointFn( this, nWhichPoint, v );
//}
//
//void CNewParticleEffect::SetControlPointEntity( int nWhichPoint, C_BaseEntity *pEntity ) {
//	using SetControlPointEntity_t = void( __thiscall * )( void *, int, C_BaseEntity * );
//	static auto SetControlPointEntityFn = reinterpret_cast< SetControlPointEntity_t >( Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 53 56 8B F1 57" ) ) );
//
//	SetControlPointEntityFn( this, nWhichPoint, pEntity );
//}
//
//CNewParticleEffect *CParticleProperty::Create( const char *pszParticleName, int iAttachType, int iAttachmentPoint /*= -1*/, Vector vecOriginOffset /*= vec3_origin*/, matrix3x4_t *vecOffsetMatrix /*= NULL*/ ) {
//	using Create_t = CNewParticleEffect * ( __thiscall * )( void *thisptr, const char *Src, int a3, int a4, __int64 a5, int a6, int a7, int a8 );
//	static auto CreateFn = reinterpret_cast< Create_t >( Memory::CallableFromRelative( Memory::Scan( XorStr( "client.dll" ), XorStr( "E8 ? ? ? ? 8B C8 8D 9F ? ? ? ?" ) ) ) );
//
//	return CreateFn( this, pszParticleName, iAttachType, iAttachmentPoint, { }, 0, { }, { } );
//}