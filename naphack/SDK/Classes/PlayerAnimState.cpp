#include "PlayerAnimState.hpp"
#include "../../Utils/Math.hpp"
#include "../Displacement.hpp"
#include "player.hpp"
#include "../../Hooking/Hooked.hpp"
#include "../../Features/Rage/ServerAnimations.hpp"

void CCSGOPlayerAnimState::Reset( ) {
	using ResetAnimState_t = void( __thiscall * )( CCSGOPlayerAnimState * );
	( ( ResetAnimState_t )Engine::Displacement.Function.m_uResetAnimState )( this );
}

void CCSGOPlayerAnimState::Update( QAngle angles ) {
	__asm
	{
		push 0
		mov ecx, this

		movss xmm1, dword ptr[ angles + 4 ]
		movss xmm2, dword ptr[ angles ]

		call Engine::Displacement.Function.m_uUpdateAnimState
	}
}

void CCSGOPlayerAnimState::UpdateLayer( C_AnimationLayer *layer, int sequence, float rate, float cycle, float weight, int index ) {
	static auto upddd = Memory::Scan( "client.dll", "55 8B EC 51 53 56 57 8B F9 83 7F ? ? 0F 84" );
	static auto update_layer_order_preset = reinterpret_cast< void( __thiscall * )( void *, int, int ) >( upddd );

	if( sequence > 1 ) {
		auto player = ( C_CSPlayer * )this->m_pPlayer;

		if( layer->m_nSequence != sequence )
			player->InvalidatePhysicsRecursive( 16 );

		layer->m_nSequence = sequence;
		layer->m_flPlaybackRate = rate;

		float temp_weight = std::clamp( weight, 0.f, 1.f );
		float temp_cycle = std::clamp( cycle, 0.f, 1.f );

		if( layer->m_flCycle != temp_cycle )
			player->InvalidatePhysicsRecursive( 8 );

		layer->m_flCycle = temp_cycle;

		float prev_weight = layer->m_flWeight;
		if( prev_weight != temp_weight && ( prev_weight == 0.f || temp_weight == 0.f ) )
			player->InvalidatePhysicsRecursive( 16 );

		layer->m_flWeight = temp_weight;

		update_layer_order_preset( this, index, layer->m_nSequence );
	}
}

const char *CCSGOPlayerAnimState::GetWeaponPrefix( ) {
	typedef const char *( __thiscall *fnGetWeaponPrefix )( void * ); 
	return ( ( fnGetWeaponPrefix )Engine::Displacement.Function.m_GetWeaponPrefix ) ( this );
}

void CCSGOPlayerAnimState::ModifyEyePosition( Vector *vecInputEyePos ) {
	if( !this || !this->m_pPlayer )
		return Hooked::oModifyEyePosition( this, vecInputEyePos );

	if( this->m_pPlayer->m_entIndex != g_pEngine->GetLocalPlayer( ) )
		return Hooked::oModifyEyePosition( this, vecInputEyePos );

	const auto m_bSmoothHeightValid = this->m_bSmoothHeightValid;
	const auto m_flCameraSmoothHeight = this->m_flCameraSmoothHeight;

	this->m_bSmoothHeightValid = false;

	Hooked::oModifyEyePosition( this, vecInputEyePos );

	this->m_bSmoothHeightValid = m_bSmoothHeightValid;
	this->m_flCameraSmoothHeight = m_flCameraSmoothHeight;
}

float CCSGOPlayerAnimState::GetMaxFraction( ) {
	float speedFactor = Math::Clamp( m_flSpeedAsPortionOfWalkTopSpeed, 0.0f, 1.0f );
	float groundFraction = ( ( m_flWalkToRunTransition * -0.3f ) - 0.2f ) * speedFactor;
	float maxFraction = groundFraction + 1.0f;

	if( m_flAnimDuckAmount > 0.0f ) {
		float maxVelocity = Math::Clamp( m_flSpeedAsPortionOfCrouchTopSpeed, 0.0f, 1.0f );
		float duckSpeed = m_flAnimDuckAmount * maxVelocity;
		maxFraction += ( duckSpeed * ( 0.5f - maxFraction ) );
	}
	return maxFraction;
}

float CCSGOPlayerAnimState::GetDesyncDelta( bool useMinYaw ) {
	float frac = GetMaxFraction( );

	return useMinYaw ? m_flAimYawMin * frac : m_flAimYawMax * frac;
}

struct mstudioposeparamdesc_t1 {
	int sznameindex;
	inline char *const pszName( void ) const { return ( ( char * )this ) + sznameindex; }
	int flags;   // ?? ( volvo, really? )
	float start; // starting value
	float end;   // ending value
	float loop;  // looping range, 0 for no looping, 360 for rotations, etc.
};
mstudioposeparamdesc_t1 *pPoseParameter( CStudioHdr *hdr, int index ) {
	using poseParametorFN = mstudioposeparamdesc_t1 * ( __thiscall * )( CStudioHdr *, int );
	poseParametorFN p_pose_parameter = ( poseParametorFN )Engine::Displacement.Function.m_pPoseParameter;
	return p_pose_parameter( hdr, index );
}

void animstate_pose_param_cache_t::SetValue( C_CSPlayer *player, float flValue ) {
	auto hdr = player->m_pStudioHdr( );
	if( hdr ) {
		auto pose_param = pPoseParameter( hdr, index );
		if( !pose_param )
			return;

		auto PoseParam = *pose_param;

		if( PoseParam.loop ) {
			float wrap = ( PoseParam.start + PoseParam.end ) / 2.0f + PoseParam.loop / 2.0f;
			float shift = PoseParam.loop - wrap;

			flValue = flValue - PoseParam.loop * std::floorf( ( flValue + shift ) / PoseParam.loop );
		}

		auto ctlValue = ( flValue - PoseParam.start ) / ( PoseParam.end - PoseParam.start );
		player->m_flPoseParameter( )[ index ] = ctlValue;
	}
}

float get_pose_parameter_valuee( CStudioHdr *hdr, int index, float value ) {
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


float animstate_pose_param_cache_t::GetValue( C_CSPlayer *player ) {
	auto idx = this->index;

	auto hdr = player->m_pStudioHdr( );
	if( !hdr )
		return 0.f;

	auto val = player->m_flPoseParameter( )[ idx ];
	return get_pose_parameter_valuee( hdr, idx, val );
}

void CCSGOPlayerAnimState::IncrementLayerCycle( C_AnimationLayer *Layer, bool bIsLoop ) {
	float_t flNewCycle = ( Layer->m_flPlaybackRate * this->m_flLastUpdateIncrement ) + Layer->m_flCycle;
	if( !bIsLoop && flNewCycle >= 1.0f )
		flNewCycle = 0.999f;

	flNewCycle -= ( int32_t )( flNewCycle );
	if( flNewCycle < 0.0f )
		flNewCycle += 1.0f;

	if( flNewCycle > 1.0f )
		flNewCycle -= 1.0f;

	Layer->m_flCycle = flNewCycle;
}

void CCSGOPlayerAnimState::IncrementLayerWeight( C_AnimationLayer *layer ) {
	if( std::fabsf( layer->m_flWeightDeltaRate ) <= 0.f )
		return;

	float current_weight = layer->m_flWeight;
	current_weight += m_flLastUpdateIncrement * layer->m_flWeightDeltaRate;
	current_weight = std::clamp<float>( current_weight, 0, 1 );

	SetLayerWeight( layer, current_weight );
}

bool CCSGOPlayerAnimState::IsLayerSequenceFinished( C_AnimationLayer *Layer, float flTime ) {
	return ( Layer->m_flPlaybackRate * flTime ) + Layer->m_flCycle >= 1.0f;
}

void CCSGOPlayerAnimState::SetLayerCycle( C_AnimationLayer *pAnimationLayer, float flCycle ) {
	if( pAnimationLayer )
		pAnimationLayer->m_flCycle = flCycle;
}

void CCSGOPlayerAnimState::SetLayerRate( C_AnimationLayer *pAnimationLayer, float flRate ) {
	if( pAnimationLayer )
		pAnimationLayer->m_flPlaybackRate = flRate;
}

void CCSGOPlayerAnimState::SetLayerWeight( C_AnimationLayer *pAnimationLayer, float flWeight ) {
	if( pAnimationLayer )
		pAnimationLayer->m_flWeight = flWeight;
}

void CCSGOPlayerAnimState::SetLayerWeightRate( C_AnimationLayer *pAnimationLayer, float flPrevious ) {
	if( pAnimationLayer )
		pAnimationLayer->m_flWeightDeltaRate = ( pAnimationLayer->m_flWeight - flPrevious ) / m_flLastUpdateIncrement;
}

void CCSGOPlayerAnimState::SetLayerSequence( C_AnimationLayer *pAnimationLayer, int iActivity ) {
	int32_t iSequence = this->SelectSequenceFromActivityModifier( iActivity );
	if( iSequence < 2 )
		return;

	pAnimationLayer->m_flCycle = 0.0f;
	pAnimationLayer->m_flWeight = 0.0f;
	pAnimationLayer->m_nSequence = iSequence;
	pAnimationLayer->m_flPlaybackRate = m_pPlayer->GetLayerSequenceCycleRate( pAnimationLayer, iSequence );
}

float CCSGOPlayerAnimState::GetLayerIdealWeightFromSeqCycle( C_AnimationLayer *pAnimationLayer ) {
	auto player = reinterpret_cast< C_CSPlayer * >( this->m_pPlayer );

	auto hdr = player->m_pStudioHdr( );
	if( !hdr )
		return 0;

	auto &sequence_desc = hdr->pSeqdesc( pAnimationLayer->m_nSequence );

	float cycle = pAnimationLayer->m_flCycle;
	if( cycle >= 0.999f )
		cycle = 1.f;

	float ease_in = sequence_desc.fadeintime;
	float ease_out = sequence_desc.fadeouttime;

	float ideal_weight = 0.f;
	if( ease_in > 0 && cycle < ease_in )
		ideal_weight = Math::SmoothStepBounds( 0, ease_in, cycle );
	else if( ease_out < 1 && cycle > ease_out )
		ideal_weight = Math::SmoothStepBounds( 1.0f, ease_out, cycle );

	if( ideal_weight < 0.0015f )
		return 0.f;

	return ( std::clamp( ideal_weight, 0.f, 1.f ) );
}

void CCSGOPlayerAnimState::IncrementLayerCycleWeightRateGeneric( C_AnimationLayer *pAnimationLayer ) {
	float flWeightPrevious = pAnimationLayer->m_flWeight;
	IncrementLayerCycle( pAnimationLayer, false );
	SetLayerWeight( pAnimationLayer, GetLayerIdealWeightFromSeqCycle( pAnimationLayer ) );
	SetLayerWeightRate( pAnimationLayer, flWeightPrevious );
}

int CCSGOPlayerAnimState::SelectSequenceFromActivityModifier( int iActivity ) {
	bool bIsPlayerDucked = m_flAnimDuckAmount > 0.55f;
	bool bIsPlayerRunning = m_flSpeedAsPortionOfWalkTopSpeed > 0.25f;

	int32_t iLayerSequence = -1;
	switch( iActivity ) {
		case ACT_CSGO_JUMP:
		{
			iLayerSequence = 15 + static_cast < int32_t >( bIsPlayerRunning );
			if( bIsPlayerDucked )
				iLayerSequence = 17 + static_cast < int32_t >( bIsPlayerRunning );
		}
		break;

		case ACT_CSGO_ALIVE_LOOP:
		{
			iLayerSequence = 8;
			if( m_pWeaponLast != m_pWeapon )
				iLayerSequence = 9;
		}
		break;

		case ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING:
		{
			iLayerSequence = 6;
		}
		break;

		case ACT_CSGO_FALL:
		{
			iLayerSequence = 14;
		}
		break;

		case ACT_CSGO_IDLE_TURN_BALANCEADJUST:
		{
			iLayerSequence = 4;
		}
		break;

		case ACT_CSGO_LAND_LIGHT:
		{
			iLayerSequence = 20;
			if( bIsPlayerRunning )
				iLayerSequence = 22;

			if( bIsPlayerDucked ) {
				iLayerSequence = 21;
				if( bIsPlayerRunning )
					iLayerSequence = 19;
			}
		}
		break;

		case ACT_CSGO_LAND_HEAVY:
		{
			iLayerSequence = 23;
			if( bIsPlayerDucked )
				iLayerSequence = 24;
		}
		break;

		case ACT_CSGO_CLIMB_LADDER:
		{
			iLayerSequence = 13;
		}
		break;
		default: break;
	}

	return iLayerSequence;
}