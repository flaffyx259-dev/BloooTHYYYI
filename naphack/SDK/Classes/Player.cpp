#include "player.hpp"
#include "../displacement.hpp"
#include "../pandora.hpp"
#include "weapon.hpp"
#include "../../Features/Rage/TickbaseShift.hpp"

QAngle &C_BasePlayer::m_aimPunchAngle( ) {
	return *( QAngle * )( ( uintptr_t )this + Engine::Displacement.DT_BasePlayer.m_aimPunchAngle );
}

QAngle &C_BasePlayer::m_aimPunchAngleVel( ) {
	return *( QAngle * )( ( uintptr_t )this + Engine::Displacement.DT_BasePlayer.m_aimPunchAngleVel );
}

QAngle &C_BasePlayer::m_viewPunchAngle( ) {
	return *( QAngle * )( ( uintptr_t )this + Engine::Displacement.DT_BasePlayer.m_viewPunchAngle );
}

Vector &C_BasePlayer::m_vecViewOffset( ) {
	return *( Vector * )( ( uintptr_t )this + Engine::Displacement.DT_BasePlayer.m_vecViewOffset );
}
Vector &C_BasePlayer::m_vecVelocity( ) {
	return *( Vector * )( ( uintptr_t )this + Engine::Displacement.DT_BasePlayer.m_vecVelocity );
}

Vector &C_BasePlayer::m_vecNetworkOrigin( ) {
	static auto offset = Memory::FindInDataMap( GetPredDescMap( ), XorStr( "m_vecNetworkOrigin" ) );
	return *( Vector * )( ( uintptr_t )this + offset );
}

Vector &C_BasePlayer::m_vecBaseVelocity( ) {
	return *( Vector * )( ( uintptr_t )this + Engine::Displacement.DT_BasePlayer.m_vecBaseVelocity );
}

Vector &C_BasePlayer::m_vecLadderNormal( ) {
	return *( Vector * )( ( uintptr_t )this + Engine::Displacement.DT_BasePlayer.m_vecLadderNormal );
}

float &C_BasePlayer::m_flFallVelocity( ) {
	return *( float * )( ( uintptr_t )this + Engine::Displacement.DT_BasePlayer.m_flFallVelocity );
}

float &C_BasePlayer::m_flDuckAmount( ) {
	return *( float * )( ( uintptr_t )this + Engine::Displacement.DT_BasePlayer.m_flDuckAmount );
}

float &C_BasePlayer::m_flDuckSpeed( ) {
	return *( float * )( ( uintptr_t )this + Engine::Displacement.DT_BasePlayer.m_flDuckSpeed );
}

char &C_BasePlayer::m_lifeState( ) {
	return *( char * )( ( uintptr_t )this + Engine::Displacement.DT_BasePlayer.m_lifeState );
}

int &C_BasePlayer::m_nTickBase( ) {
	return *( int * )( ( uintptr_t )this + Engine::Displacement.DT_BasePlayer.m_nTickBase );
}

int &C_BasePlayer::m_iHealth( ) {
	return *( int * )( ( uintptr_t )this + Engine::Displacement.DT_BasePlayer.m_iHealth );
}

int C_BasePlayer::m_iMaxHealth( ) {
	return 100;
}

bool &C_BasePlayer::m_bSpotted( ) {
	return *( bool * )( ( uintptr_t )this + Engine::Displacement.DT_BasePlayer.m_bSpotted );
}

int &C_BasePlayer::m_fFlags( ) {
	return *( int * )( ( uintptr_t )this + Engine::Displacement.DT_BasePlayer.m_fFlags );
}

int &C_BasePlayer::m_iDefaultFOV( ) {
	return *( int * )( ( uintptr_t )this + Engine::Displacement.DT_BasePlayer.m_iDefaultFOV );
}

int &C_BasePlayer::m_iObserverMode( ) {
	return *( int * )( ( uintptr_t )this + Engine::Displacement.DT_BasePlayer.m_iObserverMode );
}

CPlayerState &C_BasePlayer::pl( ) {
	return *( CPlayerState * )( ( uintptr_t )this + Engine::Displacement.DT_BasePlayer.pl );
}

CBaseHandle &C_BasePlayer::m_hObserverTarget( ) {
	return *( CBaseHandle * )( ( uintptr_t )this + Engine::Displacement.DT_BasePlayer.m_hObserverTarget );
}

CBaseHandle &C_BasePlayer::m_hGroundEntity( ) {
	return *( CBaseHandle * )( uintptr_t( this ) + Engine::Displacement.DT_BasePlayer.m_hGroundEntity );
}

CHandle<C_BaseViewModel> C_BasePlayer::m_hViewModel( ) {
	return *( CHandle<C_BaseViewModel>* )( ( uintptr_t )this + Engine::Displacement.DT_BasePlayer.m_hViewModel );
}

float C_BasePlayer::GetMaxSpeed( ) {
	if( !this )
		return 250.0f;

	auto hWeapon = this->m_hActiveWeapon( );
	if( hWeapon == -1 )
		return 250.0f;

	auto pWeapon = reinterpret_cast< C_WeaponCSBaseGun * >( this->m_hActiveWeapon( ).Get( ) );
	if( !pWeapon )
		return 250.0f;

	auto pWeaponData = pWeapon->GetCSWeaponData( );
	if( !pWeaponData.IsValid( ) )
		return 250.0f;

	if( reinterpret_cast< C_CSPlayer * >( this ) && !reinterpret_cast< C_CSPlayer * >( this )->m_bIsScoped( ) )
		return pWeaponData->m_flMaxSpeed;

	return pWeaponData->m_flMaxSpeed2;
}


float C_BasePlayer::SequenceDuration( CStudioHdr *pStudioHdr, int iSequence ) {
	//55 8B EC 53 57 8B 7D 08 8B D9 85 FF 75 double __userpurge SequenceDuration@<st0>(int a1@<ecx>, float a2@<xmm0>, int *a3, int a4)
	using SequenceDurationFn = float( __thiscall * )( void *, CStudioHdr *, int );
	return Memory::VCall< SequenceDurationFn >( this, 221 )( this, pStudioHdr, iSequence );
}

const Vector &C_BasePlayer::WorldSpaceCenter( ) {
	using WorldSpaceCenterFn = const Vector &( __thiscall * )( void * );
	return Memory::VCall< WorldSpaceCenterFn >( this, 78 )( this );
}

#pragma runtime_checks( "", off )
float C_BasePlayer::GetSequenceMoveDist( CStudioHdr *pStudioHdr, int iSequence ) {
	Vector vecReturn;

	using GetSequenceLinearMotionFn = int( __fastcall * )( CStudioHdr *, int, float *, Vector * );
	( ( GetSequenceLinearMotionFn )Engine::Displacement.Function.m_GetSequenceLinearMotion )( pStudioHdr, iSequence, m_flPoseParameter( ), &vecReturn );
	__asm {
		add esp, 8
	}

	return vecReturn.Length( );
}
#pragma runtime_checks( "", restore )

bool C_BasePlayer::IsDead( ) {
	if( !this )
		return false;

	return ( this->m_lifeState( ) ) || !this->m_iHealth( );
}

void C_BasePlayer::SetCurrentCommand( CUserCmd *cmd ) {
	*( CUserCmd ** )( ( uintptr_t )this + Engine::Displacement.C_BasePlayer.m_pCurrentCommand ) = cmd;
}

Vector C_BasePlayer::GetEyePosition( bool bInterpolated ) {
	Vector vecOrigin = bInterpolated ? GetAbsOrigin( ) : m_vecOrigin( );

	Vector offset = this->m_vecViewOffset( );
	if( offset.z >= 46.1f ) {
		if( offset.z > 64.0f ) {
			offset.z = 64.0f;
		}
	}
	else {
		offset.z = 46.0f;
	}
	vecOrigin += offset;

	return vecOrigin;
}

Vector C_BasePlayer::GetViewHeight( ) {
	Vector offset;
	if( this->m_flDuckAmount( ) == 0.0f ) {
		offset = g_pGameMovement->GetPlayerViewOffset( false );
	}
	else {
		offset = m_vecViewOffset( );
	}
	return offset;
}

//float C_BasePlayer::GetLayerSequenceCycleRate( C_AnimationLayer* pLayer, int iSequence ) {
//	using GetLayerSequenceCycleRateFn = float( __thiscall* )( void*, C_AnimationLayer*, int );
//	return Memory::VCall< GetLayerSequenceCycleRateFn >( this, 222 )( this, pLayer, iSequence );
//}

C_AnimationLayer &C_BasePlayer::GetAnimLayer( int index ) {
	// ref: CCBaseEntityAnimState::ComputePoseParam_MoveYaw
	// move_x move_y move_yaw
	typedef C_AnimationLayer &( __thiscall *Fn )( void *, int, bool );
	static Fn fn = NULL;

	if( !fn )
		fn = ( Fn )Engine::Displacement.Data.m_uAnimLayer;

	index = Math::Clamp( index, 0, 13 );

	return fn( this, index, true );
}

// L3D451R7, raxer23 (c)
void C_BasePlayer::TryInitiateAnimation( C_AnimationLayer *pLayer, int nSequence ) {
	if( !pLayer || nSequence < 2 )
		return;

	pLayer->m_flPlaybackRate = GetLayerSequenceCycleRate( pLayer, nSequence );
	pLayer->m_nSequence = nSequence;
	pLayer->m_flCycle = pLayer->m_flWeight = 0.f;
};

C_CSPlayer *C_CSPlayer::GetLocalPlayer( ) {
	if( g_Vars.globals.m_bForceFullUpdate )
		return nullptr;

	auto index = g_pEngine->GetLocalPlayer( );

	if( !index )
		return nullptr;

	auto client = g_pEntityList->GetClientEntity( index );

	if( !client )
		return nullptr;

	return ToCSPlayer( client->GetBaseEntity( ) );
}

bool C_CSPlayer::SetupFixedBones( matrix3x4_t *pBoneMatrix, int nBonesCount, int iMask, float flSetupTime ) {
	// backup values
	const int bBackupOcclusionFlags = *( int * )( uintptr_t( this ) + 0xA28 );
	const int bBackupOcclusionFramecount = *( int * )( uintptr_t( this ) + 0xA30 );

	bool bReturn = false;

	int fEffectsBackup = m_fEffects( );

	m_fEffects( ) &= ~EF_NOINTERP;

	g_Vars.globals.m_bInBoneSetup = true;

	// build bones
	bReturn = SetupBones( pBoneMatrix, nBonesCount, iMask, flSetupTime );

	m_fEffects( ) = fEffectsBackup;

	g_Vars.globals.m_bInBoneSetup = false;

	return bReturn;
}

C_CSPlayer *C_CSPlayer::GetPlayerByIndex( int index ) {
	if( g_Vars.globals.m_bForceFullUpdate || g_pClientState->m_nDeltaTick( ) == -1 )
		return nullptr;

	if( !index )
		return nullptr;

	auto client = g_pEntityList->GetClientEntity( index );

	if( !client )
		return nullptr;

	return ToCSPlayer( client->GetBaseEntity( ) );
}

__int64 C_CSPlayer::GetSteamID( ) {
	player_info_t info;
	if( !g_pEngine->GetPlayerInfo( this->EntIndex( ), &info ) )
		return -1;

	if( info.fakeplayer )
		return info.iSteamID;

	return info.steamID64;
}

std::array<int, 5> &C_CSPlayer::m_vecPlayerPatchEconIndices( ) {
	return *( std::array<int, 5>* )( ( uintptr_t )this + Engine::Displacement.DT_CSPlayer.m_vecPlayerPatchEconIndices );
}

CCSGOPlayerAnimState *&C_CSPlayer::m_PlayerAnimState( ) {
	return *( CCSGOPlayerAnimState ** )( ( uintptr_t )this + Engine::Displacement.C_CSPlayer.m_PlayerAnimState );
}

QAngle &C_CSPlayer::m_angEyeAngles( ) {
	return *( QAngle * )( ( uintptr_t )this + Engine::Displacement.DT_CSPlayer.m_angEyeAngles );
}

int &C_CSPlayer::m_nSurvivalTeam( ) {
	return *( int * )( ( uintptr_t )this + Engine::Displacement.DT_CSPlayer.m_nSurvivalTeam );
}

int &C_CSPlayer::m_ArmorValue( ) {
	return *( int * )( ( uintptr_t )this + Engine::Displacement.DT_CSPlayer.m_ArmorValue );
}

int &C_CSPlayer::m_iAccount( ) {
	return *( int * )( ( uintptr_t )this + Engine::Displacement.DT_CSPlayer.m_iAccount );
}

int &C_CSPlayer::m_iFOV( ) {
	return *( int * )( ( uintptr_t )this + Engine::Displacement.DT_CSPlayer.m_iFOV );
}

float C_CSPlayer::GetFOV( ) {
	return Memory::VCall< float( __thiscall * )( decltype( this ) ) >( this, 321 )( this );
}

int &C_CSPlayer::m_iShotsFired( ) {
	return *( int * )( ( uintptr_t )this + Engine::Displacement.DT_CSPlayer.m_iShotsFired );
}

int &C_CSPlayer::m_iAddonBits( ) {
	static auto m_iAddonBits = Engine::g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_iAddonBits" ) );
	return *( int * )( uintptr_t( this ) + m_iAddonBits );
}

float &C_CSPlayer::m_flFlashDuration( ) {
	return *( float * )( ( uintptr_t )this + Engine::Displacement.DT_CSPlayer.m_flFlashDuration );
}

float &C_CSPlayer::m_flSecondFlashAlpha( ) {
	return *( float * )( uintptr_t( this ) + Engine::Displacement.DT_CSPlayer.m_flFlashDuration - 0xC );
}

float &C_CSPlayer::m_flVelocityModifier( ) {
	return *( float * )( ( uintptr_t )this + Engine::Displacement.DT_CSPlayer.m_flVelocityModifier );
}

float &C_CSPlayer::m_flLowerBodyYawTarget( ) {
	return *( float * )( ( uintptr_t )this + Engine::Displacement.DT_CSPlayer.m_flLowerBodyYawTarget );
}

float &C_CSPlayer::m_flSpawnTime( ) {
	static auto m_iAddonBits = Engine::g_PropManager.GetOffset( XorStr( "DT_CSPlayer" ), XorStr( "m_iAddonBits" ) );
	return *( float * )( uintptr_t( this ) + m_iAddonBits - 4 );
}

float &C_CSPlayer::m_flHealthShotBoostExpirationTime( ) {
	return *( float * )( ( uintptr_t )this + Engine::Displacement.DT_CSPlayer.m_flHealthShotBoostExpirationTime );
}

float &C_CSPlayer::m_surfaceFriction( ) {
	// ye ik...
	static int offset = SDK::Memory::FindInDataMap( this->GetPredDescMap( ), XorStr( "m_surfaceFriction" ) );
	return *( float * )( ( uintptr_t )this + offset );
}

bool &C_CSPlayer::m_bHasHelmet( ) {
	return *( bool * )( ( uintptr_t )this + Engine::Displacement.DT_CSPlayer.m_bHasHelmet );
}

bool &C_CSPlayer::m_bHasHeavyArmor( ) {
	return *( bool * )( ( uintptr_t )this + Engine::Displacement.DT_CSPlayer.m_bHasHeavyArmor );
}

bool &C_CSPlayer::m_bIsScoped( ) {
	return *( bool * )( ( uintptr_t )this + Engine::Displacement.DT_CSPlayer.m_bScoped );
}

bool &C_CSPlayer::m_bIsWalking( ) {
	return *( bool * )( ( uintptr_t )this + Engine::Displacement.DT_CSPlayer.m_bIsWalking );
}

float &C_CSPlayer::m_flThirdpersonRecoil( ) {
	return *( float * )( ( uintptr_t )this + Engine::Displacement.DT_CSPlayer.m_flThirdpersonRecoil );
}

bool &C_CSPlayer::m_bWaitForNoAttack( ) {
	return *( bool * )( ( uintptr_t )this + Engine::Displacement.DT_CSPlayer.m_bWaitForNoAttack );
}

bool &C_CSPlayer::m_bIsPlayerGhost( ) {
	return *( bool * )( ( uintptr_t )this + Engine::Displacement.DT_CSPlayer.m_bIsPlayerGhost );
}

int &C_CSPlayer::m_iMatchStats_Kills( ) {
	return *( int * )( ( uintptr_t )this + Engine::Displacement.DT_CSPlayer.m_iMatchStats_Kills );
}

int &C_CSPlayer::m_iMatchStats_Deaths( ) {
	return *( int * )( ( uintptr_t )this + Engine::Displacement.DT_CSPlayer.m_iMatchStats_Deaths );
}

int &C_CSPlayer::m_iMatchStats_HeadShotKills( ) {
	return *( int * )( ( uintptr_t )this + Engine::Displacement.DT_CSPlayer.m_iMatchStats_HeadShotKills );
}

bool &C_CSPlayer::m_bGunGameImmunity( ) {
	return *( bool * )( ( uintptr_t )this + Engine::Displacement.DT_CSPlayer.m_bGunGameImmunity );
}

bool &C_CSPlayer::m_bIsDefusing( ) {
	return *( bool * )( ( uintptr_t )this + Engine::Displacement.DT_CSPlayer.m_bIsDefusing );
}

CBaseHandle &C_CSPlayer::m_hCarriedHostage( ) {
	return *( CBaseHandle * )( ( uintptr_t )this + Engine::Displacement.DT_CSPlayer.m_hCarriedHostage );
}

bool &C_CSPlayer::m_bHasDefuser( ) {
	return *( bool * )( ( uintptr_t )this + Engine::Displacement.DT_CSPlayer.m_bHasDefuser );
}

bool &C_CSPlayer::m_bInNoDefuseArea( ) {
	return *( bool * )( ( uintptr_t )this + Engine::Displacement.DT_CSPlayer.m_bInNoDefuseArea );
}

bool &C_CSPlayer::m_bDuckOverride( ) {
	return *( bool * )( uintptr_t( this ) + Engine::Displacement.DT_CSPlayer.m_bDuckOverride );
}

void C_CSPlayer::RunPostThink( ) {
	static auto PostThinkPhysics = reinterpret_cast< bool( __thiscall * )( void * ) >( Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 83 E4 ? 81 EC ? ? ? ? 53 8B D9 56 57 83 BB" ) ) );
	static auto SimulatePlayerSimulatedEntities = reinterpret_cast< void( __thiscall * )( void * ) >( Memory::Scan( XorStr( "client.dll" ), XorStr( "56 8B F1 57 8B BE ? ? ? ? 83 EF ? 78 ? 90" ) ) );

	g_pMDLCache->BeginLock( );

	if( !this->IsDead( ) ) {
		// UpdateBounds
		Memory::VCall<void( __thiscall * )( void * )>( this, 329 )( this );

		if( this->m_fFlags( ) & FL_ONGROUND )
			this->m_flFallVelocity( ) = 0.f;

		if( *( int * )( uintptr_t( this ) + Engine::Displacement.DT_BaseAnimating.m_nSequence ) == -1 ) {
			// SetSequence
			Memory::VCall<void( __thiscall * )( void *, int )>( this, 213 )( this, 0 );
		}

		// unnamed func
		Memory::VCall<void( __thiscall * )( void * )>( this, 214 )( this );
		PostThinkPhysics( this );
	}

	SimulatePlayerSimulatedEntities( this );
	g_pMDLCache->EndLock( );
}

int C_CSPlayer::SetNextThink( int tick ) {
	typedef int( __thiscall *fnSetNextThink ) ( C_CSPlayer *, int tick );
	auto ret = ( ( fnSetNextThink )Engine::Displacement.Function.m_uNextThink ) ( this, tick );
	return ret;
}

void C_CSPlayer::Think( ) {
	const auto next_think = ( int * )( ( std::uintptr_t )this + 0xF8 );

	static auto think = reinterpret_cast< void( __thiscall * )( void *, int ) >( Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 56 57 8B F9 8B B7 ? ? ? ? 8B C6" ) ) );

	if( *next_think > 0 && *next_think <= this->m_nTickBase( ) ) {
		*next_think = -1;

		think( this, 0 );
		Memory::VCall<void( __thiscall * )( void * )>( this, 137 )( this );
	}
}

void C_CSPlayer::PreThink( ) {
	static auto physics_run_think = reinterpret_cast< bool( __thiscall * )( void *, int ) >( Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 83 EC ? 53 56 57 8B F9 8B 87 ? ? ? ? C1 E8" ) ) );

	if( physics_run_think( this, 0 ) )
		Memory::VCall<void( __thiscall * )( void * )>( this, 307 )( this );
}

void C_CSPlayer::PostThink( ) {
	using Fn = void( __thiscall * )( void * );
	Memory::VCall<Fn>( this, /*316*/ 317 )( this );
}

bool C_CSPlayer::is( std::string networkname ) {
	if( !this )
		return false;

	auto clientClass = this->GetClientClass( );
	if( !clientClass )
		return false;

	return Engine::g_PropManager.GetClientID( networkname ) == clientClass->m_ClassID;
}

bool C_CSPlayer::IsTeammate( C_CSPlayer *player, bool nigger ) {
	if( !player || !this )
		return false;

	if( g_Vars.mp_teammates_are_enemies->GetBool( ) ) {
		return this->EntIndex( ) == g_pEngine->GetLocalPlayer( );
	}

	C_CSPlayer *ent = player;

	if( nigger )
		if( player->EntIndex( ) == g_pEngine->GetLocalPlayer( ) ) {
			if( player->IsDead( ) ) {
				if( player->m_hObserverTarget( ).IsValid( ) ) {
					const auto spec = ( C_CSPlayer * )player->m_hObserverTarget( ).Get( );
					if( spec ) {
						if( player->m_iObserverMode( ) == 4 || player->m_iObserverMode( ) == 5 )
							ent = spec;
					}
				}
			}
		}

	if( g_Vars.game_type->GetInt( ) == 6 ) {
		if( m_nSurvivalTeam( ) >= 0 && m_nSurvivalTeam( ) == ent->m_nSurvivalTeam( ) )
			return true;

		return false;
	}

	return this->m_iTeamNum( ) == ent->m_iTeamNum( );
}

surfacedata_t *C_CSPlayer::m_SurfaceData( ) {
	return *( surfacedata_t ** )( uintptr_t( this ) + 0x35A0 );
}

float C_CSPlayer::m_flStamina( ) {
	return *( float* )( uintptr_t( this ) + Engine::Displacement.DT_CSPlayer.m_flStamina );
}

bool C_CSPlayer::CanShoot( bool bSkipRevolver, bool bSkipNextAttack ) {
	bool local = EntIndex( ) == g_pEngine->GetLocalPlayer( );

	auto weapon = ( C_WeaponCSBaseGun * )( this->m_hActiveWeapon( ).Get( ) );
	if( !weapon )
		return false;

	auto weapon_data = weapon->GetCSWeaponData( );
	if( !weapon_data.IsValid( ) )
		return false;

	if( this->m_fFlags( ) & 0x40 )
		return false;

	if( g_pGameRules.IsValid( ) ) {
		if( g_pGameRules->m_bFreezePeriod( ) )
			return false;
	}

	if( this->m_bWaitForNoAttack( ) )
		return false;

	if( *( int * )( uintptr_t( this ) + Engine::Displacement.DT_CSPlayer.m_iPlayerState ) )
		return false;

	if( this->m_bIsDefusing( ) )
		return false;

	if( weapon->m_iItemDefinitionIndex( ) == WEAPON_HEALTHSHOT )
		return false;

	//if( this->IsReloading( ) )
	//	return false;

	if( weapon->m_iItemDefinitionIndex( ) == WEAPON_ZEUS ) {
		if( weapon->m_iClip1( ) == 0 )
			return false;
	}

	if( weapon_data->m_iWeaponType >= WEAPONTYPE_PISTOL && weapon_data->m_iWeaponType <= WEAPONTYPE_MACHINEGUN && weapon->m_iClip1( ) < 1 )
		return false;

	int nTicksToAccount = 0;
	if( g_Vars.rage.exploit ) {
		if( g_Vars.rage.hide_shots_bind.enabled && !g_Vars.rage.double_tap_bind.enabled && g_TickbaseController.CanShift( ) ) {

			//TODO: REVIEW THIS WHEN PRED FIX IS PERFECT LMAO
			// reason for this being 8 not 7
			// accounting for 1 extra tick to prevent pred error...
			nTicksToAccount = -8;
		}
		else {
			if( g_Vars.rage.double_tap_bind.enabled && g_TickbaseController.m_bBreakingLC && g_TickbaseController.m_bBreakLC && g_TickbaseController.CanShift( false, true ) ) {

				// in createmove we already account for tickbase
				// to fix aimbot etc
				// though revolver fixes up itself in runcommand with tickbase :D !
				// so to fix other issues (like aa disabling) we undo the fix here.
				if( ( weapon->m_iItemDefinitionIndex( ) == WEAPON_REVOLVER ) && !bSkipRevolver ) {
					nTicksToAccount = -g_TickbaseController.m_nLCShiftAmount;
				}
			}
		}
	}

	float curtime = TICKS_TO_TIME( this->m_nTickBase( ) ) + TICKS_TO_TIME( nTicksToAccount );

	if( !bSkipNextAttack ) {
		if( curtime < m_flNextAttack( ) )
			return false;
	}

	if( ( weapon->m_iItemDefinitionIndex( ) == WEAPON_GLOCK || weapon->m_iItemDefinitionIndex( ) == WEAPON_FAMAS ) && weapon->m_iBurstShotsRemaining( ) > 0 ) {
		if( curtime >= weapon->m_fNextBurstShot( ) )
			return true;
	}

	if( curtime < weapon->m_flNextPrimaryAttack( ) )
		return false;

	if( weapon->m_iItemDefinitionIndex( ) != WEAPON_REVOLVER )
		return true;

	if( bSkipRevolver )
		return true;

	if( *( int * )( uintptr_t( weapon ) + Engine::Displacement.DT_BaseAnimating.m_nSequence ) != 5 )
		return false;

	float yep = weapon->m_flPostponeFireReadyTime( );

	return curtime >= yep;
}

bool C_CSPlayer::IsFiring( CUserCmd *pCmd ) {
	if( !pCmd )
		return false;

	auto weapon = ( C_WeaponCSBaseGun * )( this->m_hActiveWeapon( ).Get( ) );
	if( !weapon )
		return false;

	auto weapon_data = weapon->GetCSWeaponData( );
	if( !weapon_data.IsValid( ) )
		return false;

	if( weapon_data->m_iWeaponType == WEAPONTYPE_KNIFE ) {
		return ( pCmd->buttons & ( IN_ATTACK | IN_ATTACK2 ) ) && CanShoot( );
	}
	else {
		return ( ( pCmd->buttons & IN_ATTACK ) || ( ( weapon->m_iItemDefinitionIndex( ) == WEAPON_GLOCK || weapon->m_iItemDefinitionIndex( ) == WEAPON_FAMAS ) && weapon->m_iBurstShotsRemaining( ) ) ) && CanShoot( );
	}
}

bool C_CSPlayer::IsReloading( ) {
	if( !this )
		return false;

	auto animLayer = this->m_AnimOverlay( ).Element( 1 );
	if( !animLayer.m_pOwner )
		return false;

	return GetSequenceActivity( animLayer.m_nSequence ) == 967 && animLayer.m_flWeight != 0.f;
}

Vector C_CSPlayer::GetEyePosition( bool bModifyEyePos, bool bInterpolated ) {
	Vector eyePosition;
	eyePosition.Init( );

	const float oldz = this->m_vecViewOffset( ).z;
	if( this->m_vecViewOffset( ).z > 64.0f ) {
		this->m_vecViewOffset( ).z = 64.0f;
	}
	else if( this->m_vecViewOffset( ).z <= 46.05f ) {
		this->m_vecViewOffset( ).z = 46.0f;
	}

	if( bModifyEyePos ) {
		Memory::VCall<void( __thiscall * )( void *, Vector * )>( this, 277 )( this, &eyePosition );
	}
	else {
		eyePosition = this->C_BasePlayer::GetEyePosition( bInterpolated );
	}

	this->m_vecViewOffset( ).z = oldz;

	return eyePosition;
}

Vector C_CSPlayer::GetHitboxPosition( int hitbox ) {
	if( !this || !this->m_CachedBoneData( ).Base( ) )
		return { };

	const auto count = this->m_CachedBoneData( ).Count( );
	if( !count || count < hitbox )
		return { };

	mstudiohitboxset_t *pHitboxSet = nullptr;
	if( auto pStudioHdr = this->m_pStudioHdr( ); pStudioHdr ) {
		if( pStudioHdr->_m_pStudioHdr )
			pHitboxSet = pStudioHdr->_m_pStudioHdr->pHitboxSet( this->m_nHitboxSet( ) );
	}

	if( !pHitboxSet )
		return { };

	const auto phitbox = pHitboxSet->pHitbox( hitbox );
	if( !phitbox || phitbox->bone > count )
		return { };

	matrix3x4_t bonetoworld = this->m_CachedBoneData( ).Base( )[ phitbox->bone ];

	return Vector( bonetoworld[ 0 ][ 3 ], bonetoworld[ 1 ][ 3 ], bonetoworld[ 2 ][ 3 ] );;
}

QAngle C_CSPlayer::DecayAimPunchAngle( QAngle &vPunchAngleVelocity ) {
	QAngle vPunchAngleExpDecay;
	float flExpDecay, flLinDecay, flLinDecayUsable;

	float flPunchAngleExpDecayLength;

	static auto weapon_recoil_decay2_exp = g_pCVar->FindVar( XorStr( "weapon_recoil_decay2_exp" ) );
	static auto weapon_recoil_decay2_lin = g_pCVar->FindVar( XorStr( "weapon_recoil_decay2_lin" ) );
	static auto weapon_recoil_vel_decay = g_pCVar->FindVar( XorStr( "weapon_recoil_vel_decay" ) );

	QAngle &angPunchAngle = m_aimPunchAngle( );

	flExpDecay = weapon_recoil_decay2_exp->GetFloat( );
	flLinDecay = weapon_recoil_decay2_lin->GetFloat( );

	vPunchAngleExpDecay = angPunchAngle;
	flPunchAngleExpDecayLength = Vector( vPunchAngleExpDecay.x, vPunchAngleExpDecay.y, vPunchAngleExpDecay.z ).Length( );

	flLinDecayUsable = flLinDecay * g_pGlobalVars->interval_per_tick;

	// inlined HybridDecay
	if( ( flPunchAngleExpDecayLength > 0.0 ) && ( flPunchAngleExpDecayLength > flLinDecayUsable ) ) {
		float flMultiplier;

		flMultiplier = 1.0f - ( flLinDecayUsable / flPunchAngleExpDecayLength );
		vPunchAngleExpDecay *= flMultiplier;
	}
	else {
		vPunchAngleExpDecay = { 0, 0, 0 };
	}

	angPunchAngle = vPunchAngleExpDecay + ( vPunchAngleExpDecay * g_pGlobalVars->interval_per_tick * 0.5f );
	vPunchAngleVelocity *= expf( g_pGlobalVars->interval_per_tick * -weapon_recoil_vel_decay->GetFloat( ) );
	angPunchAngle += ( vPunchAngleExpDecay * g_pGlobalVars->interval_per_tick * 0.5f );

	return angPunchAngle;
}

#define LODWORD(x)  (*((int*)&(x)))  // low dword

void C_CSPlayer::ClampBonesInSurroundingBox( matrix3x4_t *pMatrix, int nBoneMask, bool bUseGame ) {

}