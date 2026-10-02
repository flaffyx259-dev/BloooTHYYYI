#include "../sdk.hpp"
#include "../../pandora.hpp"
#include "../Displacement.hpp"

void IPrediction::Update( int startframe, bool validframe, int incoming_acknowledged, int outgoing_command ) {
	using Fn = void( __thiscall* )( void*, int, bool, int, int );
	return Memory::VCall<Fn>( this, Index::IPrediction::Update )( this, startframe, validframe, incoming_acknowledged, outgoing_command );
}

void IPrediction::SetupMove( C_BasePlayer* player, CUserCmd* ucmd, void * pHelper, void * move ) {
	using Fn = void( __thiscall* )( void*, C_BasePlayer*, CUserCmd*, void *, void * );
	return Memory::VCall<Fn>( this, Index::IPrediction::SetupMove )( this, player, ucmd, pHelper, move );
}

void IPrediction::FinishMove( C_BasePlayer* player, CUserCmd* ucmd, void * move ) {
	using Fn = void( __thiscall* )( void*, C_BasePlayer*, CUserCmd*, void * );
	return Memory::VCall<Fn>( this, Index::IPrediction::FinishMove )( this, player, ucmd, move );
}

int IPrediction::CheckMovingGround( C_BasePlayer* player, double unk ) {
	using Fn = int( __thiscall* )( void*, C_BasePlayer*, double );
	return Memory::VCall<Fn>( this, Index::IPrediction::CheckMovingGround )( this, player, unk );
}

bool IPrediction::InPrediction( ) {
	using Fn = bool( __thiscall* )( void* );
	return Memory::VCall<Fn>( this, Index::IPrediction::InPrediction )( this );
}

void IPrediction::SetLocalViewAngles( QAngle &ang ) {
	using Fn = void( __thiscall* )( void*, QAngle& );
	Memory::VCall<Fn>( this, 13 )( this, ang );
}

CGlobalVarsBase* IPrediction::GetUnpredictedGlobals( ) {
	if( InPrediction( ) )
		return reinterpret_cast< CGlobalVarsBase* >( uint32_t( this ) + 0x4c );

	return g_pGlobalVars.Xor( );
}
