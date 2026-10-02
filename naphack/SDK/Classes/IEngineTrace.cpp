#include "IEngineTrace.hpp"
#include "entity.hpp"
#include "Player.hpp"
#include "../displacement.hpp"
#include "../../pandora.hpp"

bool CTraceFilterPlayersOnlySkipOne::ShouldHitEntity( IHandleEntity* pEntityHandle, int ) {
   return pEntityHandle != pEnt && ( ( IClientEntity* ) pEntityHandle )->GetClientClass( )->m_ClassID == CCSPlayer;
}

bool CTraceFilter::ShouldHitEntity( IHandleEntity* pEntityHandle, int ) {
   ClientClass* pEntCC = ( ( IClientEntity* ) pEntityHandle )->GetClientClass( );
   if ( pEntCC && strcmp( ccIgnore, "" ) ) {
	  if ( pEntCC->m_pNetworkName == ccIgnore )
		 return false;
   }

   return !( pEntityHandle == pSkip );
}

bool CGameTrace::DidHitWorld() const {
	if( !hit_entity )
		return false;

	return hit_entity->m_entIndex == 0;
}

bool CGameTrace::DidHitNonWorldEntity() const {
	return hit_entity != nullptr && !DidHitWorld();
}


__forceinline uint32_t IEngineTrace::GetFilterSimpleVtable( ) {
   static const auto filter_simple = *reinterpret_cast< uint32_t* >( Engine::Displacement.Function.m_TraceFilterSimple );
   return filter_simple;
}


bool CTraceFilterSkipTeammates::ShouldHitEntity( IHandleEntity* pEntityHandle, int content ) {
	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return false;

	const auto clientClass = reinterpret_cast< C_BaseEntity* >( pEntityHandle )->GetClientClass( );
	if( !clientClass )
		return false;

	// it should only hit players.....
	return clientClass->m_ClassID == ClassId_t::CCSPlayer && ( !reinterpret_cast< C_CSPlayer* >( pEntityHandle )->IsTeammate( pLocal ) );
}
