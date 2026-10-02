#include "../Hooked.hpp"
#include "../../SDK/Displacement.hpp"
#include "../../SDK/sdk.hpp"
#include "../../SDK/Classes/Player.hpp"
#include "../../SDK/Valve/CBaseHandle.hpp"
#include "../../Features/Rage/EnginePrediction.hpp"
#include "../../Features/Rage/Animations.hpp"

void Hooked::m_flVelocityModifier( const CRecvProxyData *pData, void *pStruct, void *pOut ) {
	m_flVelocityModifierSwap->GetOriginalFunction( )( pData, pStruct, pOut );

	if( pStruct ) {
		auto ent = ( C_BasePlayer * )( ( C_BasePlayer * )pStruct )->GetBaseEntity( );
		if( !ent )
			return;

		if( ent->EntIndex( ) == g_pEngine->GetLocalPlayer( ) ) {
			if( pData->m_Value.m_Float != g_Prediction.velocity_modifier ) {
				g_Prediction.velocity_modifier = pData->m_Value.m_Float;
			}
		}
	}
}