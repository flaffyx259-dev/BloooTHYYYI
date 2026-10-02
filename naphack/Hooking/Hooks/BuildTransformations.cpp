#include "../../pandora.hpp"
#include "../Hooked.hpp"
#include "../../SDK/Displacement.hpp"
#include "../../SDK/Classes/Player.hpp"

void __fastcall Hooked::BuildTransformations( C_CSPlayer* pPlayer, uint32_t, CStudioHdr* hdr, Vector* pos, Quaternion* q, const matrix3x4_t& transform, const int32_t mask, BYTE* computed ) {
	g_Vars.globals.szLastHookCalled = XorStr( "420" );

	C_CSPlayer* pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pPlayer || !pLocal || !pPlayer->IsPlayer( ) || pPlayer->EntIndex( ) == pLocal->EntIndex( ) || !g_Vars.rage.enabled )
		return oBuildTransformations( pPlayer, hdr, pos, q, transform, mask, computed );

	// backup jiggle bones.
	//const bool m_isJiggleBonesEnabledBackup = *( bool* )( uintptr_t( pPlayer ) + 0x2930 );

	// overwrite jiggle bones and refuse the game from calling the
	// code responsible for animating our attachments/weapons.
	//*( bool* )( uintptr_t( pPlayer ) + 0x2930 ) = false;

	if( !( pPlayer->m_fEffects( ) & EF_NOINTERP ) )
		pPlayer->m_fEffects( ) |= EF_NOINTERP;

	oBuildTransformations( pPlayer, hdr, pos, q, transform, mask, computed );

	if( !( pPlayer->m_fEffects( ) & EF_NOINTERP ) )
		pPlayer->m_fEffects( ) |= EF_NOINTERP;

	//if( pPlayer->m_fEffects( ) & EF_NOINTERP )
	//	pPlayer->m_fEffects( ) &= ~EF_NOINTERP;

	// restore jiggle bones
	//*( bool* )( uintptr_t( pPlayer ) + 0x2930 ) = m_isJiggleBonesEnabledBackup;
}