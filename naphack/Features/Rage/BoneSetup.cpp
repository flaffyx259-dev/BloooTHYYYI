#include "BoneSetup.hpp"
#include "Animations.hpp"

#include "../../SDK/Displacement.hpp"
#include "../../Hooking/Hooked.hpp"
#include "../../SDK/Classes/weapon.hpp"

#include "../../SDK/Valve/CBaseHandle.hpp"

#include "../../Loader/Exports.h"

inline CHandle<C_BaseEntity> m_hRagdoll( C_CSPlayer *entity ) {
	return *( CHandle<C_BaseEntity>* )( ( int32_t )entity + Engine::Displacement.DT_CSPlayer.m_hRagdoll );
}

BoneSetup g_BoneSetup;

bool BoneSetup::SetupBonesRebuild( C_CSPlayer *entity, matrix3x4_t *pBoneMatrix, int nBoneCount, int boneMask, float time, int flags, QAngle angCustomAngle ) {
	if( *( int * )( uintptr_t( entity ) + Engine::Displacement.DT_BaseViewModel.m_nSequence ) == -1 ) {
		return false;
	}

	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return false;

	if( boneMask == -1 ) {
		boneMask = entity->m_iPrevBoneMask( );
	}

	boneMask = boneMask | 0x80000;

	// If we're setting up LOD N, we have set up all lower LODs also
	// because lower LODs always use subsets of the bones of higher LODs.
	int nLOD = 0;
	int nMask = BONE_USED_BY_VERTEX_LOD0;
	for( ; nLOD < MAX_NUM_LODS; ++nLOD, nMask <<= 1 ) {
		if( boneMask & nMask )
			break;
	}
	for( ; nLOD < MAX_NUM_LODS; ++nLOD, nMask <<= 1 ) {
		boneMask |= nMask;
	}

	auto model_bone_counter = **( unsigned long ** )( Engine::Displacement.C_BaseAnimating.InvalidateBoneCache + 0x000A );

	CBoneAccessor backup_bone_accessor = entity->m_BoneAccessor( );
	CBoneAccessor *bone_accessor = &entity->m_BoneAccessor( );
	if( !bone_accessor )
		return false;

	if( entity->m_iMostRecentModelBoneCounter( ) != model_bone_counter || ( flags & BoneSetupFlags::ForceInvalidateBoneCache ) ) {
		if( FLT_MAX >= entity->m_flLastBoneSetupTime( ) || time < entity->m_flLastBoneSetupTime( ) ) {
			bone_accessor->m_ReadableBones = 0;
			bone_accessor->m_WritableBones = 0;
			entity->m_flLastBoneSetupTime( ) = ( time );
		}

		entity->m_iPrevBoneMask( ) = entity->m_iAccumulatedBoneMask( );
		entity->m_iAccumulatedBoneMask( ) = 0;

		auto hdr = entity->m_pStudioHdr( );
		if( hdr ) { // profiler stuff
			( ( CStudioHdrEx * )hdr )->m_nPerfAnimatedBones = 0;
			( ( CStudioHdrEx * )hdr )->m_nPerfUsedBones = 0;
			( ( CStudioHdrEx * )hdr )->m_nPerfAnimationLayers = 0;
		}
	}

	// Keep track of everything asked for over the entire frame
	// But not those things asked for during bone setup
	auto bu1 = entity->m_iAccumulatedBoneMask( );
	entity->m_iAccumulatedBoneMask( ) |= boneMask;

	// fix enemies out of pvs
	auto bu2 = entity->m_iOcclusionFramecount( );
	auto bu3 = entity->m_iOcclusionFlags( );
	entity->m_iOcclusionFramecount( ) = 0;
	entity->m_iOcclusionFlags( ) = 0;

	// Make sure that we know that we've already calculated some bone stuff this time around.
	auto bu4 = entity->m_iMostRecentModelBoneCounter( );
	entity->m_iMostRecentModelBoneCounter( ) = model_bone_counter;

	bool bReturnCustomMatrix = ( flags & BoneSetupFlags::UseCustomOutput ) && pBoneMatrix;
	CStudioHdr *hdr = entity->m_pStudioHdr( );
	if( !hdr ) {
		return false;
	}

	// Setup our transform based on render angles and origin.
	Vector origin = m_vecCustomOrigin.IsZero( ) ? ( ( flags & BoneSetupFlags::UseInterpolatedOrigin ) ? entity->GetAbsOrigin( ) : entity->m_vecOrigin( ) ) : m_vecCustomOrigin;
	QAngle angles = ( angCustomAngle.x != -1337.f && angCustomAngle.y != -1337.f && angCustomAngle.z != -1337.f ) ? angCustomAngle : entity->GetAbsAngles( );

	alignas( 16 ) matrix3x4_t parentTransform;
	parentTransform.AngleMatrix( angles, origin );

	boneMask |= entity->m_iPrevBoneMask( );

	if( bReturnCustomMatrix ) {
		bone_accessor->m_pBones = pBoneMatrix;
	}

	// Allow access to the bones we're setting up so we don't get asserts in here.
	int oldReadableBones = bone_accessor->GetReadableBones( );
	int oldWritableBones = bone_accessor->GetWritableBones( );
	int newWritableBones = oldReadableBones | boneMask;
	bone_accessor->SetWritableBones( newWritableBones );
	bone_accessor->SetReadableBones( newWritableBones );

	if( !( hdr->_m_pStudioHdr->flags & 0x00000010 ) ) {
		entity->m_iEFlags( ) |= EFL_SETTING_UP_BONES;

		entity->m_pIk( ) = nullptr;
		entity->m_EntClientFlags |= 2; // ENTCLIENTFLAGS_DONTUSEIK

		alignas( 16 ) Vector pos[ 128 ];
		alignas( 16 ) Quaternion q[ 128 ];
		uint8_t computed[ 0x100 ];

		entity->StandardBlendingRules( hdr, pos, q, time, boneMask );

		std::memset( computed, 0, 0x100 );
		entity->BuildTransformations( hdr, pos, q, parentTransform, boneMask, computed );

		entity->m_iEFlags( ) &= ~EFL_SETTING_UP_BONES;

		// entity->ControlMouth( hdr );

		if( !bReturnCustomMatrix /*&& !bSkipAnimFrame*/ ) {
			memcpy( entity->m_vecBonePos( ), &pos[ 0 ], sizeof( Vector ) * hdr->_m_pStudioHdr->numbones );
			memcpy( entity->m_quatBoneRot( ), &q[ 0 ], sizeof( Quaternion ) * hdr->_m_pStudioHdr->numbones );
		}
	}
	else {
		parentTransform = bone_accessor->m_pBones[ 0 ];
	}

	if( /*boneMask & BONE_USED_BY_ATTACHMENT*/ flags & BoneSetupFlags::AttachmentHelper ) {
		using AttachmentHelperFn = void( __thiscall * )( C_BaseEntity *, CStudioHdr * );
		( ( AttachmentHelperFn )Engine::Displacement.Function.m_AttachmentHelper )( entity, hdr );
	}

	// don't override bone cache if we're just generating a standalone matrix
	if( bReturnCustomMatrix ) {
		*bone_accessor = backup_bone_accessor;
	}

	if( pLocal->EntIndex( ) == entity->EntIndex( ) ) {
		entity->m_iAccumulatedBoneMask( ) = bu1;
		entity->m_iOcclusionFramecount( ) = bu2;
		entity->m_iOcclusionFlags( ) = bu3;
		entity->m_iMostRecentModelBoneCounter( ) = bu4;
	}

	return true;
}

bool BoneSetup::BuildBones( C_CSPlayer *entity, int mask, int flags, matrix3x4_t *pBoneMatrix, QAngle angCustomAngle ) {
	// no need to restore this
	// entity->m_bIsJiggleBonesEnabled( ) = false;

	// Skip occlusion checks in C_CSPlayer::ReevauluateAnimLOD and C_CSPlayer::AccumulateLayers
	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return false;

	auto uBackup0 = *( int * )( uintptr_t( entity ) + 0xA28 );
	auto uBackup1 = *( int * )( uintptr_t( entity ) + 0xA30 );
	auto uBackup2 = *( uint8_t * )( ( uintptr_t )entity + 0x274 );
	auto uBackup3 = *( uint16_t * )( ( uintptr_t )entity + 0x272 );
	auto uBackup4 = *( int * )( uintptr_t( entity ) + 0xA68 );
	auto uBackup5 = *( uintptr_t * )( ( uintptr_t )entity + Engine::Displacement.C_BaseAnimating.m_pFirstBoneSnapshot );
	auto uBackup6 = *( uintptr_t * )( ( uintptr_t )entity + Engine::Displacement.C_BaseAnimating.m_pSecondBoneSnapshot );

	*( int * )( uintptr_t( entity ) + 0xA28 ) = 0;
	*( int * )( uintptr_t( entity ) + 0xA30 ) = 0;

	// Force server calculations in C_BaseAnimatingOverlay::AccumulateInterleavedDispatchedLayers and C_CSPlayer::BuildTransformations
	if( ( *( uint8_t * )( ( uintptr_t )entity + 0x274 ) & 1 ) == 0 )
		*( uint8_t * )( ( uintptr_t )entity + 0x274 ) |= 1;

	if( *( uint16_t * )( ( uintptr_t )entity + 0x272 ) == -1 )
		*( uint16_t * )( ( uintptr_t )entity + 0x272 ) = 0;

	// prevent C_BaseAnimating::ShouldSkipAnimationFrame from being called
	*( int * )( uintptr_t( entity ) + 0xA68 ) = 0;

	if( entity->EntIndex( ) != pLocal->EntIndex( ) ) {
		// fix weird interpolated animations ( C_CSPlayer::AccumulateLayers and C_CSPlayer::BuildTransformations )
		*( uintptr_t * )( ( uintptr_t )entity + Engine::Displacement.C_BaseAnimating.m_pFirstBoneSnapshot ) = 0;
		*( uintptr_t * )( ( uintptr_t )entity + Engine::Displacement.C_BaseAnimating.m_pSecondBoneSnapshot ) = 0;
	}

	// setup bones :-)
	auto bReturn = SetupBonesRebuild( entity, pBoneMatrix != nullptr ? pBoneMatrix : nullptr, -1, mask, entity->m_flSimulationTime( ), flags, angCustomAngle );

	*( int * )( uintptr_t( entity ) + 0xA28 ) = uBackup0;
	*( int * )( uintptr_t( entity ) + 0xA30 ) = uBackup1;
	*( uint8_t * )( ( uintptr_t )entity + 0x274 ) = uBackup2;
	*( uint8_t * )( ( uintptr_t )entity + 0x272 ) = uBackup3;
	*( int * )( uintptr_t( entity ) + 0xA68 ) = uBackup4;

	if( entity->EntIndex( ) != pLocal->EntIndex( ) ) {
		*( uintptr_t * )( ( uintptr_t )entity + Engine::Displacement.C_BaseAnimating.m_pFirstBoneSnapshot ) = uBackup5;
		*( uintptr_t * )( ( uintptr_t )entity + Engine::Displacement.C_BaseAnimating.m_pSecondBoneSnapshot ) = uBackup6;
	}

	return bReturn;
}

bool BoneSetup::BuildBonesSimple( C_CSPlayer *player, const int max_bones, const int bone_mask, matrix3x4_t *bone_out ) {
	const auto hdr = player->m_pStudioHdr( );
	if( !hdr )
		return false;

	matrix3x4_t pBackupMatrix[ MAXSTUDIOBONES ];
	std::memcpy( pBackupMatrix, player->m_CachedBoneData( ).Base( ), sizeof( matrix3x4_t ) * player->m_CachedBoneData( ).Count( ) );

	const auto backup_effects = player->m_fEffects( );

	alignas( 16 ) matrix3x4_t parent_transform;
	parent_transform.AngleMatrix( player->GetAbsAngles( ), player->GetAbsOrigin( ) );

	alignas( 16 ) Vector pos[ 128 ];
	alignas( 16 ) Quaternion q[ 128 ];
	uint8_t computed[ 0x100 ];

	uint32_t bone_computed[ 8 ] = { 0 };
	//CServerSetupBones::Studio_BuildMatrices( hdr, parent_transform, player->m_vecBonePos( ), player->m_quatBoneRot( ),
	//										 BONE_USED_BY_ANYTHING,
	//										 player->m_CachedBoneData( ).Base( ), bone_computed );

	std::memcpy( bone_out, player->m_CachedBoneData( ).Base( ), sizeof( matrix3x4_t ) * player->m_CachedBoneData( ).Count( ) );

	std::memcpy( player->m_CachedBoneData( ).Base( ), pBackupMatrix, sizeof( matrix3x4_t ) * player->m_CachedBoneData( ).Count( ) );

	return true;
}

struct bone_setup_t {
	CStudioHdr *hdr{};
	int mask{};
	float *pose_parameter{};
	void *pose_debugger{};

	void InitPose( Vector pos[ ], Quaternion q[ ], CStudioHdr *hdr ) {
		__asm
		{
			mov eax, this
			mov esi, q
			mov edx, pos
			push dword ptr[ hdr + 4 ]
			mov ecx, [ eax ]
			push esi
			call Engine::Displacement.CBoneSetup.InitPose
			add esp, 8
		}
	}

	void AccumulatePose( Vector pos[ ], Quaternion q[ ], int sequence, float cycle, float flWeight, float flTime, CIKContext *pIKContext ) {
		using AccumulatePoseFn = void( __thiscall * )( bone_setup_t *, Vector *a2, Quaternion *a3, int a4, float a5, float a6, float a7, CIKContext *a8 );
		auto Accumulate_Pose = ( AccumulatePoseFn )Engine::Displacement.CBoneSetup.AccumulatePose;
		return Accumulate_Pose( this, pos, q, sequence, cycle, flWeight, flTime, pIKContext );
	}

	void CalcAutoplaySequences( Vector pos[ ], Quaternion q[ ], float flRealTime, CIKContext *pIKContext ) {
		__asm
		{
			movss   xmm3, flRealTime
			mov eax, pIKContext
			mov ecx, this
			push eax
			push q
			push pos
			call Engine::Displacement.CBoneSetup.CalcAutoplaySequences
		}
	}

	void CalcBoneAdj( Vector pos[ ], Quaternion q[ ], float *controllers, int boneMask ) {
		__asm
		{
			mov     eax, controllers
			mov     ecx, this
			mov     edx, pos; a2
			push    dword ptr[ ecx + 4 ]; a5
			mov     ecx, [ ecx ]; a1
			push    eax; a4
			push    q; a3
			call    Engine::Displacement.CBoneSetup.CalcBoneAdj
			add     esp, 0xC
		}
	}
};

struct mstudioposeparamdesc_t {
	int sznameindex;
	inline char *const pszName( void ) const { return ( ( char * )this ) + sznameindex; }
	int flags;   // ?? ( volvo, really? )
	float start; // starting value
	float end;   // ending value
	float loop;  // looping range, 0 for no looping, 360 for rotations, etc.
};

mstudioposeparamdesc_t *pPoseParameter( CStudioHdr *hdr, int index ) {
	using poseParametorFN = mstudioposeparamdesc_t * ( __thiscall * )( CStudioHdr *, int );
	poseParametorFN p_pose_parameter = ( poseParametorFN )Engine::Displacement.Function.m_pPoseParameter;
	return p_pose_parameter( hdr, index );
}

__forceinline float get_pose_parameter_value( CStudioHdr *hdr, int index, float value ) {
	if( index < 0 || index > 24 )
		return 0.f;


	auto pose_parameter = pPoseParameter( hdr, index );
	if( !pose_parameter )
		return 0.f;

	if( pose_parameter->loop ) {
		float wrap = ( pose_parameter->start + pose_parameter->end ) / 2.f + pose_parameter->loop / 2.f;
		float shift = pose_parameter->loop - wrap;

		value = value - pose_parameter->loop * std::floorf( ( value + shift ) / pose_parameter->loop );
	}

	return ( value - pose_parameter->start ) / ( pose_parameter->end - pose_parameter->start );
}

void update_cache( uintptr_t bonemerge ) {
	static auto BoneMergeUpdateCache = ( void( __thiscall * )( uintptr_t ) )Engine::Displacement.CBoneMergeCache.m_nUpdateCache;
	BoneMergeUpdateCache( bonemerge );
}

void merge_matching_poses( std::uintptr_t &bone_merge, float *poses, float *target_poses ) {
	update_cache( bone_merge );

	if( *( std::uintptr_t * )( bone_merge + 0x10 ) && *( std::uintptr_t * )( bone_merge + 0x8C ) ) {
		int *index = ( int * )( bone_merge + 0x20 );
		for( int i = 0; i < 24; ++i ) {
			if( *index != -1 ) {
				C_CSPlayer *target = *( C_CSPlayer ** )( bone_merge + 0x4 );
				CStudioHdr *hdr = target->m_pStudioHdr( );
				float pose_param_value = 0.f;

				if( hdr && *( studiohdr_t ** )hdr && i >= 0 ) {
					float pose = target_poses[ i ];
					auto pose_param = pPoseParameter( hdr, i );

					pose_param_value = pose * ( pose_param->end - pose_param->start ) + pose_param->start;
				}

				C_CSPlayer *second_target = *( C_CSPlayer ** )( bone_merge );
				CStudioHdr *second_hdr = second_target->m_pStudioHdr( );

				poses[ *index ] = get_pose_parameter_value( second_hdr, *index, pose_param_value );
			}

			++index;
		}
	}
}

void c_bone_builder::store( C_CSPlayer *player, matrix3x4_t *matrix, int mask ) {
	auto state = player->m_PlayerAnimState( );

	animating = player;
	origin = player == C_CSPlayer::GetLocalPlayer( ) ? player->GetAbsOrigin( ) : player->m_vecOrigin( );
	layers = player->m_AnimOverlay( ).Base( );
	this->hdr = player->m_pStudioHdr( );
	layer_count = 13;
	angles = player->GetAbsAngles( );
	this->matrix = matrix;
	this->mask = mask;

	time = player->m_flSimulationTime( );

	attachments = false;
	ik_ctx = false;
	dispatch = true;

	// player->store_poses( poses.data( ) );
	std::memcpy( poses.data( ), player->m_flPoseParameter( ), sizeof( float ) * 24 );

	eye_angles = player->m_angEyeAngles( );

	auto weapon = player->m_hActiveWeapon( ).IsValid( ) ? ( C_BaseAttributableItem * )( player->m_hActiveWeapon( ).Get( ) ) : nullptr;

	C_CSPlayer *world_weapon = nullptr;
	if( weapon )
		world_weapon = weapon->m_hWeaponWorldModel( ).IsValid( ) ? ( C_CSPlayer * )( weapon->m_hWeaponWorldModel( ).Get( ) ) : nullptr;

	if( world_weapon )
		std::memcpy( poses_world.data( ), world_weapon->m_flPoseParameter( ), sizeof( float ) * 24 );
	else
		std::memcpy( poses_world.data( ), player->m_flPoseParameter( ), sizeof( float ) * 24 );

	filled = true;
}

void c_bone_builder::store( LagRecord_t *pRecord, int mask, ESides eSide ) {
	if( !pRecord )
		return;

	auto pEntity = pRecord->m_pEntity;
	if( !pEntity )
		return;

	const float m_flSimulationTime = pEntity->m_flSimulationTime( );

	Vector m_vecAbsOrigin;
	Vector m_vecOrigin;

	QAngle m_angAbsAngles;
	QAngle m_angEyeAngles;

	std::array<C_AnimationLayer, 13> m_pServerAnimOverlays;
	std::array<float, 20> m_pPoseParameters;

	// backup shit
	m_vecOrigin = pEntity->m_vecOrigin( );
	m_vecAbsOrigin = pEntity->GetAbsOrigin( );

	m_angEyeAngles = pEntity->m_angEyeAngles( );
	m_angAbsAngles = pEntity->GetAbsAngles( );

	std::memcpy( m_pServerAnimOverlays.data( ), pEntity->m_AnimOverlay( ).Base( ), 13 * sizeof( C_AnimationLayer ) );
	std::memcpy( m_pPoseParameters.data( ), pEntity->m_flPoseParameter( ), sizeof( float ) * 20 );

	// build bones with shit from this record
	pEntity->SetAbsOrigin( pRecord->m_vecOrigin );
	pEntity->m_vecOrigin( ) = pRecord->m_vecOrigin;

	pEntity->m_angEyeAngles( ) = pRecord->m_angEyeAngles;
	pEntity->SetAbsAngles( pRecord->m_sAnims[ eSide ].m_angAbsAngles );

	std::memcpy( pEntity->m_AnimOverlay( ).Base( ), pRecord->m_sAnims[ eSide ].m_pServerAnimOverlays.data( ), 13 * sizeof( C_AnimationLayer ) );
	std::memcpy( pEntity->m_flPoseParameter( ), pRecord->m_sAnims[ eSide ].m_pPoseParameters.data( ), sizeof( float ) * 20 );

	pEntity->m_flSimulationTime( ) = pRecord->m_flAnimationTime;

	// prepare bonestup with our record data
	store( pEntity, pRecord->m_sAnims[ eSide ].m_pMatrix, mask );

	// restore back to original data
	pEntity->SetAbsOrigin( m_vecAbsOrigin );
	pEntity->m_vecOrigin( ) = m_vecOrigin;

	pEntity->m_angEyeAngles( ) = m_angEyeAngles;
	pEntity->SetAbsAngles( m_angAbsAngles );

	std::memcpy( pEntity->m_AnimOverlay( ).Base( ), m_pServerAnimOverlays.data( ), 13 * sizeof( C_AnimationLayer ) );
	std::memcpy( pEntity->m_flPoseParameter( ), m_pPoseParameters.data( ), sizeof( float ) * 20 );

	pEntity->m_flSimulationTime( ) = m_flSimulationTime;
}

void c_bone_builder::setup( ) {
	alignas( 16 ) Vector position[ 128 ] = { };
	alignas( 16 ) Quaternion q[ 128 ] = { };

	auto ik_context = ( CIKContext * )animating->m_pIk( );

	if( !ik_ctx )
		ik_context = nullptr;

	hdr = animating->m_pStudioHdr( );

	if( !hdr )
		return;

	uint32_t bone_computed[ 8 ]{};
	std::memset( bone_computed, 0, 8 * sizeof( uint32_t ) );

	bool sequences_available = !*( int * )( *( uintptr_t * )hdr + 0x150 ) || *( int * )( ( uintptr_t )hdr + 0x4 );

	if( ik_context ) {
		ik_context->Init( hdr, &angles, &origin, time, TIME_TO_TICKS( time ), mask );

		if( sequences_available )
			get_skeleton( position, q );

		animating->UpdateIKLocks( time );
		ik_context->UpdateTargets( position, q, matrix, ( uint8_t * )bone_computed );
		animating->CalculateIKLocks( time );
		ik_context->SolveDependencies( position, q, matrix, ( uint8_t * )bone_computed );
	}
	else if( sequences_available )
		get_skeleton( position, q );

	matrix3x4_t transform{};
	transform.AngleMatrix( angles, origin );

	studio_build_matrices( hdr, transform, position, q, mask, matrix, bone_computed );

	if( mask & BONE_USED_BY_ATTACHMENT && attachments && animating->m_pStudioHdr( ) ) {
		using AttachmentHelperFn = void( __thiscall * )( C_BaseEntity *, CStudioHdr * );
		( ( AttachmentHelperFn )Engine::Displacement.Function.m_AttachmentHelper )( animating, animating->m_pStudioHdr( ) );
	}

	animating->m_flLastBoneSetupTime( ) = time;

	animating->m_BoneAccessor( ).m_ReadableBones |= mask;
	animating->m_BoneAccessor( ).m_WritableBones |= mask;
	animating->m_iMostRecentModelBoneCounter( ) = *( int * )Engine::Displacement.Data.m_uModelBoneCounter;

	const auto mdl = animating->GetModel( );
	if( !mdl )
		return;

	auto hdr = g_pModelInfo->GetStudiomodel( mdl );
	if( !hdr )
		return;

	const auto hitbox_set = hdr->pHitboxSet( animating->m_nHitboxSet( ) );
	if( !hitbox_set )
		return;

	for( int i{}; i < hitbox_set->numhitboxes; ++i ) {
		const auto hitbox = hitbox_set->pHitbox( i );
		if( !hitbox
			|| hitbox->m_flRadius >= 0.f )
			continue;

		matrix3x4_t rot_mat{};
		rot_mat.AngleMatrix( hitbox->m_angAngles );
		rot_mat.ConcatTransforms( matrix[ hitbox->bone ] );
	}
}

__forceinline bool can_be_animated( C_CSPlayer *player ) {
	// m_bUseNewAnimstate
	if( !( *( bool * )( ( uintptr_t )player + 0x39E1 ) ) || !player->m_PlayerAnimState( ) )
		return false;

	auto weapon = player->m_hActiveWeapon( ).IsValid( ) ? ( C_BaseAttributableItem * )( player->m_hActiveWeapon( ).Get( ) ) : nullptr;
	if( !weapon )
		return false;

	auto weaponWorldModel = weapon->m_hWeaponWorldModel( ).IsValid( ) ? ( C_CSPlayer * )weapon->m_hWeaponWorldModel( ).Get( ) : nullptr;
	if( !weaponWorldModel || *( short * )( ( std::uintptr_t )weaponWorldModel + 0x26E ) == -1 )
		return player == C_CSPlayer::GetLocalPlayer( );

	return true;
}

uintptr_t &GetBoneMerge( C_CSPlayer *player ) {
	return *( uintptr_t * )( ( uintptr_t )player + Engine::Displacement.C_BaseAnimating.m_pBoneMerge );
}

void c_bone_builder::get_skeleton( Vector *position, Quaternion *q ) {
	alignas( 16 ) Vector new_position[ 128 ]{};
	alignas( 16 ) Quaternion new_q[ 128 ]{};

	auto ik_context = ( CIKContext * )animating->m_pIk( );

	if( !ik_ctx )
		ik_context = nullptr;

	alignas( 16 ) char buffer[ 32 ];
	alignas( 16 ) bone_setup_t *bone_setup = ( bone_setup_t * )&buffer;

	bone_setup->hdr = hdr;
	bone_setup->mask = mask;
	bone_setup->pose_parameter = poses.data( );
	bone_setup->pose_debugger = nullptr;

	bone_setup->InitPose( position, q, hdr );
	bone_setup->AccumulatePose( position, q, animating->m_nSequence( ), animating->m_flCycle( ), 1.f, time, ik_context );

	int layer[ 13 ] = { };

	for( int i = 0; i < layer_count; ++i ) {
		C_AnimationLayer &final_layer = layers[ i ];

		if( final_layer.m_flWeight > 0.f && final_layer.m_nOrder != 13 && final_layer.m_nOrder >= 0 && final_layer.m_nOrder < layer_count )
			layer[ final_layer.m_nOrder ] = i;
	}

	char tmp_buffer[ 4208 ]{};
	auto world_ik = ( CIKContext * )tmp_buffer;

	auto weapon = animating->m_hActiveWeapon( ).IsValid( ) ? ( C_BaseAttributableItem * )animating->m_hActiveWeapon( ).Get( ) : nullptr;
	C_CSPlayer *world_weapon = nullptr;
	if( weapon )
		world_weapon = weapon->m_hWeaponWorldModel( ).IsValid( ) ? ( C_CSPlayer * )weapon->m_hWeaponWorldModel( ).Get( ) : nullptr;

	auto wrong_weapon = [&] ( ) {
		if( can_be_animated( animating ) && world_weapon ) {
			uintptr_t bone_merge = GetBoneMerge( world_weapon );
			if( bone_merge ) {
				merge_matching_poses( bone_merge, poses_world.data( ), poses.data( ) );

				auto world_hdr = world_weapon->m_pStudioHdr( );

				world_ik->Construct( );
				world_ik->Init( world_hdr, &angles, &origin, time, 0, BONE_USED_BY_BONE_MERGE );

				alignas( 16 ) char buffer2[ 32 ]{};
				alignas( 16 ) bone_setup_t *world_setup = ( bone_setup_t * )&buffer2;

				world_setup->hdr = world_hdr;
				world_setup->mask = BONE_USED_BY_BONE_MERGE;
				world_setup->pose_parameter = poses_world.data( );
				world_setup->pose_debugger = nullptr;

				world_setup->InitPose( new_position, new_q, world_hdr );

				for( int i = 0; i < layer_count; ++i ) {
					C_AnimationLayer *layer = &layers[ i ];

					if( layer && layer->m_nSequence > 1 && layer->m_flWeight > 0.f ) {
						if( dispatch && animating == C_CSPlayer::GetLocalPlayer( ) ) {
							using UpdateDispatchLayer = void( __thiscall * )( void *, C_AnimationLayer *, CStudioHdr *, int );
							Memory::VCall< UpdateDispatchLayer >( animating, 241 )( animating, layer, world_hdr, layer->m_nSequence );
						}

						if( !dispatch || layer->m_nDispatchSequence_2 <= 0 || layer->m_nDispatchSequence_2 >= ( *( studiohdr_t ** )world_hdr )->numlocalseq )
							bone_setup->AccumulatePose( position, q, layer->m_nSequence, layer->m_flCycle, layer->m_flWeight, time, ik_context );
						else if( dispatch ) {
							typedef void( __thiscall *oIKAddDependencies )( CIKContext *, float, int, int, const float[ ], float );
							static auto add_dependencies = Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 81 EC BC ? ? ? 53 56 57" ) );

							auto copy_to_follow = ( void( __thiscall * )( uintptr_t, Vector *, Quaternion *, int, Vector *, Quaternion * ) )Engine::Displacement.CBoneMergeCache.m_CopyToFollow;
							auto copy_from_follow = ( void( __thiscall * )( uintptr_t, Vector *, Quaternion *, int, Vector *, Quaternion * ) ) Engine::Displacement.CBoneMergeCache.m_CopyFromFollow;


							/*static auto copy_from_follow = offsets::copy_from_follow.cast<void( __thiscall * )( std::uintptr_t, vec3_t *, quaternion_t *, int, vec3_t *, quaternion_t * )>( );
							static auto add_dependencies = offsets::add_dependencies.cast<void( __thiscall * )( c_ik_context *, float, int, int, const float[ ], float )>( );
							static auto copy_to_follow = offsets::copy_to_follow.cast<void( __thiscall * )( std::uintptr_t, vec3_t *, quaternion_t *, int, vec3_t *, quaternion_t * )>( );*/

							copy_from_follow( bone_merge, position, q, BONE_USED_BY_BONE_MERGE, new_position, new_q );
							if( ik_context ) {
								( oIKAddDependencies( add_dependencies ) (
									ik_context, *( float * )( ( std::uintptr_t )animating + 0xA14 ), layer->m_nSequence, layer->m_flCycle, poses.data( ), layer->m_flWeight ) );
							}

							world_setup->AccumulatePose( new_position, new_q, layer->m_nDispatchSequence_2, layer->m_flCycle, layer->m_flWeight, time, world_ik );
							copy_to_follow( bone_merge, new_position, new_q, BONE_USED_BY_BONE_MERGE, position, q );
						}
					}
				}

				world_ik->Destructor( );
				return false;
			}
		}
		return true;
	};

	if( wrong_weapon( ) ) {
		for( int i = 0; i < this->layer_count; ++i ) {
			int layer_count = layer[ i ];

			if( layer_count >= 0 && layer_count < this->layer_count ) {
				C_AnimationLayer *final_layer = &layers[ i ];
				bone_setup->AccumulatePose( position, q, final_layer->m_nSequence, final_layer->m_flCycle, final_layer->m_flWeight, time, ik_context );
			}
		}
	}

	if( ik_context ) {
		world_ik->Construct( );
		world_ik->Init( hdr, &angles, &origin, time, 0, mask );
		bone_setup->CalcAutoplaySequences( position, q, time, world_ik );
		world_ik->Destructor( );
	}
	else
		bone_setup->CalcAutoplaySequences( position, q, time, nullptr );

	bone_setup->CalcBoneAdj( position, q, ( float * )( ( std::uintptr_t )animating + 0xA54 ), mask );
}

void c_bone_builder::studio_build_matrices( CStudioHdr *hdr, const matrix3x4_t &world_transform, Vector *pos, Quaternion *q, int mask, matrix3x4_t *out, uint32_t *bone_computed ) {
	int i = 0;
	int chain_length = 0;
	int bone = -1;
	auto studio_hdr = *( studiohdr_t ** )hdr;

	if( bone < -1 || bone >= studio_hdr->numbones )
		bone = 0;

	CUtlVector<int> *bone_parent = ( CUtlVector<int>* )( ( std::uintptr_t )hdr + 0x44 );
	CUtlVector<int> *bone_flags = ( CUtlVector<int>* )( ( std::uintptr_t )hdr + 0x30 );

	int chain[ 128 ]{};
	if( bone <= -1 ) {
		chain_length = studio_hdr->numbones;

		for( i = 0; i < studio_hdr->numbones; ++i )
			chain[ chain_length - i - 1 ] = i;
	}
	else {
		i = bone;

		do {
			chain[ chain_length++ ] = i;
			i = bone_parent->Element( i );

		} while( i != -1 );
	}

	matrix3x4_t bone_matrix{};
	for( int j = chain_length - 1; j >= 0; --j ) {
		i = chain[ j ];

		if( ( 1 << ( i & 0x1F ) ) & bone_computed[ i >> 5 ] )
			continue;

		int flag = bone_flags->Element( i );
		int parent = bone_parent->Element( i );

		if( ( flag & mask ) && q ) {
			bone_matrix.QuaternionMatrix( q[ i ], pos[ i ] );

			if( parent == -1 )
				out[ i ] = world_transform.ConcatTransforms( bone_matrix );
			else
				out[ i ] = out[ parent ].ConcatTransforms( bone_matrix );
		}
	}
}