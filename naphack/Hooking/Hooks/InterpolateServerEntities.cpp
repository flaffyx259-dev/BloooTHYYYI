#include "../Hooked.hpp"
#include "../../SDK/Displacement.hpp"
#include "../../SDK/Classes/Player.hpp"
#include "../../Features/Rage/Animations.hpp"
#include "../../Features/Rage/ServerAnimations.hpp"
#include "../../Features/Rage/BoneSetup.hpp"
#include "../../Features/Rage/TickbaseShift.hpp"

#define bonesnapshot_get( _name ) _name
void __cdecl Hooked::InterpolateServerEntities( ) {
	g_Vars.globals.szLastHookCalled = XorStr( "11" );

	C_CSPlayer *pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal || !g_pEngine->IsInGame( ) )
		return oInterpolateServerEntities( );

	g_TickbaseController.m_bOverrideIsPaused = g_TickbaseController.m_bInCharge;

	oInterpolateServerEntities( );

	g_TickbaseController.m_bOverrideIsPaused = false;

	if( pLocal && !pLocal->IsDead( ) && pLocal->m_CachedBoneData( ).Count( ) > 0 ) {
		for( int i = 0; i < MAXSTUDIOBONES; ++i ) for( int n = 0; n <= 2; ++n )
			g_ServerAnimations.m_uVisualAnimations.m_pMatrix[ i ].m[ n ][ 3 ] += pLocal->GetAbsOrigin( )[ n ];

		std::memcpy( pLocal->m_CachedBoneData( ).Base( ), g_ServerAnimations.m_uVisualAnimations.m_pMatrix, pLocal->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );
		pLocal->ForceBoneCache( );

		for( int i = 0; i < MAXSTUDIOBONES; ++i ) for( int n = 0; n <= 2; ++n )
			g_ServerAnimations.m_uVisualAnimations.m_pMatrix[ i ].m[ n ][ 3 ] -= pLocal->GetAbsOrigin( )[ n ];
	}


	using AttachmentHelper_t = void( __thiscall * )( C_CSPlayer *, CStudioHdr * );
	static AttachmentHelper_t AttachmentHelperFn = ( AttachmentHelper_t )Engine::Displacement.Function.m_AttachmentHelper;

	for( int i = 1; i <= g_pGlobalVars->maxClients; ++i ) {
		const auto pEntity = reinterpret_cast< C_CSPlayer * >( g_pEntityList->GetClientEntity( i ) );
		if( !pEntity )
			continue;

		if( pEntity->EntIndex( ) == g_pEngine->GetLocalPlayer( ) )
			continue;

		if( pEntity->IsDead( ) )
			continue;

		if( pEntity->IsDormant( ) )
			continue;

		//if( pEntity->IsTeammate( C_CSPlayer::GetLocalPlayer( ) ) )
		//	continue;

		const auto pAnimData = g_Animations.GetAnimationEntry( i );
		if( !pAnimData || pAnimData->m_deqRecords.empty( ) )
			continue;

		LagRecord_t pRecord = pAnimData->m_deqRecords.front( );

		matrix3x4_t pMatrix[ MAXSTUDIOBONES ];
		std::memcpy( pMatrix, pRecord.m_pVisualMatrix, MAXSTUDIOBONES * sizeof( matrix3x4_t ) );
		if( !pMatrix || !pEntity->m_CachedBoneData( ).Count( ) )
			continue;

		const Vector &vecRenderOrigin = pEntity->GetAbsOrigin( );

		const int nBoneCount = /*bUseCount ? pEntity->m_CachedBoneData( ).Count( ) :*/ MAXSTUDIOBONES;
		for( auto i = 0; i < nBoneCount; i++ )
			pMatrix[ i ].MatrixSetColumn( pMatrix[ i ].MatrixGetColumn( 3 ) + vecRenderOrigin, 3 );

		std::memcpy( pEntity->m_CachedBoneData( ).Base( ), pMatrix, pEntity->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );
		std::memcpy( pEntity->m_BoneAccessor( ).m_pBones, pMatrix, pEntity->m_CachedBoneData( ).Count( ) * sizeof( matrix3x4_t ) );

		pEntity->m_iMostRecentModelBoneCounter( ) = *( int * )Engine::Displacement.Data.m_uModelBoneCounter;
		pEntity->m_BoneAccessor( ).m_ReadableBones = pEntity->m_BoneAccessor( ).m_WritableBones = 0xFFFFFFFF;
		pEntity->m_flLastBoneSetupTime( ) = FLT_MAX;

		if( const auto pStudioHdr = pEntity->m_pStudioHdr( ); pStudioHdr ) {
			using AttachmentHelperFn = void( __thiscall * )( C_BaseEntity *, CStudioHdr * );
			( ( AttachmentHelperFn )Engine::Displacement.Function.m_AttachmentHelper )( pEntity, pStudioHdr );
		}
	}
}