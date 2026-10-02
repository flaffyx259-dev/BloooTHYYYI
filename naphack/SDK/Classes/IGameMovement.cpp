#include "../sdk.hpp"
#include "../SDK/Classes/player.hpp"

void IGameMovement::ProcessMovement( C_BasePlayer *pPlayer, CMoveData *pMove ) {
	using Fn = void( __thiscall * )( void *, C_BasePlayer *, CMoveData * );
	return Memory::VCall<Fn>( this, Index::IGameMovement::ProcessMovement )( this, pPlayer, pMove );
}

void IGameMovement::Reset( ) {
	using Fn = void( __thiscall * )( void * );
	return Memory::VCall<Fn>( this, Index::IGameMovement::Reset )( this );
}

void IGameMovement::StartTrackPredictionErrors( C_BasePlayer *pPlayer ) {
	using Fn = void( __thiscall * )( void *, C_BasePlayer * );
	return Memory::VCall<Fn>( this, Index::IGameMovement::StartTrackPredictionErrors )( this, pPlayer );
}

void IGameMovement::FinishTrackPredictionErrors( C_BasePlayer *pPlayer ) {
	using Fn = void( __thiscall * )( void *, C_BasePlayer * );
	return Memory::VCall<Fn>( this, Index::IGameMovement::FinishTrackPredictionErrors )( this, pPlayer );
}

Vector IGameMovement::GetPlayerMins( bool bDucked ) {
	return Memory::VCall< const Vector & ( __thiscall * )( void *, bool ) >( this, Index::IGameMovement::GetPlayerMins )( this, bDucked );
}

Vector IGameMovement::GetPlayerMaxs( bool bDucked ) {
	return Memory::VCall< const Vector & ( __thiscall * )( void *, bool ) >( this, Index::IGameMovement::GetPlayerMaxs )( this, bDucked );
}

Vector IGameMovement::GetPlayerViewOffset( bool bDucked ) {
	return Memory::VCall< const Vector & ( __thiscall * )( void *, bool ) >( this, Index::IGameMovement::GetPlayerViewOffset )( this, bDucked );
}

CMoveData IGameMovement::SetupMove( C_CSPlayer *const player, CUserCmd *cmd ) {
	CMoveData data{};

	data.m_bFirstRunOfFunctions = false;
	data.m_bGameCodeMovedPlayer = false;
	data.m_nPlayerHandle = ( int )player->GetRefEHandle( ).Get( );

	data.m_vecVelocity = player->m_vecVelocity( );
	data.m_vecAbsOrigin = player->GetAbsOrigin( );
	data.m_flClientMaxSpeed = player->GetMaxSpeed( );

	data.m_vecAbsViewAngles = data.m_vecAngles = data.m_vecViewAngles = cmd->viewangles;
	data.m_nImpulseCommand = cmd->impulse;
	data.m_nButtons = cmd->buttons;

	data.m_flForwardMove = cmd->forwardmove;
	data.m_flSideMove = cmd->sidemove;
	data.m_flUpMove = cmd->upmove;

	data.m_vecConstraintCenter = Vector( 0,0,0);
	data.m_flConstraintRadius = 0.f;
	data.m_flConstraintSpeedFactor = 0.f;

	//SetupMovementBounds(&data);

	return data;
}