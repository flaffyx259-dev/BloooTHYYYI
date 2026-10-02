#include "../Hooked.hpp"
#include "../../SDK/Classes/Player.hpp"
#include "../../SDK/Displacement.hpp"

void __fastcall Hooked::SetupMove( void* ecx, void* edx, C_BasePlayer* a3, CUserCmd* a4, IMoveHelper* a5, CMoveData* a6 ) {
	oSetupMove( ecx, a3, a4, a5, a6 );

	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	// doesn't fix the prediction errors when bhopping and hitting ground, but it does
	// fix the weird camera bob that happens. ^i'll also look into that
	if( ( pLocal->m_fFlags( ) & FL_ONGROUND ) != 0 && ( a4->buttons & IN_JUMP ) != 0 ) {
		auto m_vecVelocity = pLocal->m_vecVelocity( );
		m_vecVelocity.x *= 1.2f;
		m_vecVelocity.y *= 1.2f;
		//m_vecVelocity.z *= 1.2f;
		
		// not sure, i don't think so (look @ pseudo code under)
		// m_vecVelocity.z *= 1.2f;
		
		//if( !::m_vecVelocity )
		//{
		//	m_vecVelocity = Utilities::GrabNetvarHash( 0x72E242A1, 0x53630D14 );
		//	::m_vecVelocity = m_vecVelocity;
		//}
		//*( m_vecVelocity + entity ) = *( m_vecVelocity + entity ) * 1.2;
		//if( !m_vecVelocity )
		//{
		//	m_vecVelocity = Utilities::GrabNetvarHash( 0x72E242A1, 0x53630D14 );
		//	::m_vecVelocity = m_vecVelocity;
		//}
		//*( m_vecVelocity + entity + 4 ) = *( m_vecVelocity + entity + 4 ) * 1.2;
		//if( !m_vecVelocity )
		//{
		//	m_vecVelocity = Utilities::GrabNetvarHash( 0x72E242A1, 0x53630D14 );
		//	::m_vecVelocity = m_vecVelocity;
		//}

		*( float* )( ( uintptr_t )a5 + 0x44 ) = m_vecVelocity.x;
		*( float* )( ( uintptr_t )a5 + 0x48 ) = m_vecVelocity.y;
		*( float* )( ( uintptr_t )a5 + 0x4C ) = m_vecVelocity.z;

		*( float* )( ( uintptr_t )a6 + 0x40 ) = m_vecVelocity.x;
		*( float* )( ( uintptr_t )a6 + 0x44 ) = m_vecVelocity.y;
		*( float* )( ( uintptr_t )a6 + 0x48 ) = m_vecVelocity.z;
	}
}