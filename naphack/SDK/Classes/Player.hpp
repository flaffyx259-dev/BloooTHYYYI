#pragma once

#include "entity.hpp"

#include "PlayerAnimState.hpp"
#include "../Valve/CBaseHandle.hpp"

class C_BaseViewModel;

class CPlayerState {
public:
	virtual ~CPlayerState( ) { }
	bool deadflag;
	QAngle		v_angle; // Viewing angle (player only)

	// The client .dll only cares about deadflag
	// the game and engine .dlls need to worry about the rest of this data
	// Player's network name
	string_t	netname;

	// 0:nothing, 1:force view angles, 2:add avelocity
	int			fixangle;

	// delta angle for fixangle == FIXANGLE_RELATIVE
	QAngle		anglechange;

	// flag to single the HLTV/Replay fake client, not transmitted
	bool		hltv;
	bool		replay;
	int			frags;
	int			deaths;
};

#define VF(y, x, z, ...) x y##(__VA_ARGS__)\
    { \
        _asm mov eax, dword ptr[ecx]\
        _asm mov edx, z\
        _asm imul edx, 4\
        _asm mov eax, dword ptr[eax + edx]\
        _asm pop ebp\
        _asm pop ebp\
        _asm jmp eax\
    }

class C_BasePlayer : public C_BaseCombatCharacter {
public:
	QAngle& m_aimPunchAngle( );
	QAngle& m_aimPunchAngleVel( );
	QAngle& m_viewPunchAngle( );
	Vector& m_vecViewOffset( );
	Vector& m_vecVelocity( );
	Vector& m_vecNetworkOrigin( );
	Vector& m_vecBaseVelocity( );
	Vector& m_vecLadderNormal( );
	float& m_flFallVelocity( );
	float& m_flDuckAmount( );
	float& m_flDuckSpeed( );
	char& m_lifeState( );
	int& m_nTickBase( );
	int& m_iHealth( );
	int& m_fFlags( );
	int& m_iDefaultFOV( );
	int& m_iObserverMode( );
	int m_iMaxHealth( );

	bool &m_bSpotted( );

	CPlayerState& pl( );
	CBaseHandle& m_hObserverTarget( );
	CBaseHandle& m_hGroundEntity( );
	CHandle<C_BaseViewModel> m_hViewModel( );
	float GetMaxSpeed( );
	float SequenceDuration( CStudioHdr* pStudioHdr, int iSequence );
	const Vector& WorldSpaceCenter( );
	float GetSequenceMoveDist( CStudioHdr* pStudioHdr, int iSequence );
public:

	bool IsDead( );
	void SetCurrentCommand( CUserCmd* cmd );

	Vector GetEyePosition( bool bInterpolated = true );
	Vector GetViewHeight( );
	C_AnimationLayer& GetAnimLayer( int index );

	VF( GetLayerSequenceCycleRate, float, 217, C_AnimationLayer*, int );
	//float GetLayerSequenceCycleRate( C_AnimationLayer* pLayer, int iSequence );
	void TryInitiateAnimation( C_AnimationLayer* pLayer, int nSequence );
};

class C_CSPlayer : public C_BasePlayer {
public:
	static C_CSPlayer* GetLocalPlayer( );
	bool SetupFixedBones( matrix3x4_t *pBoneMatrix, int nBonesCount, int iMask, float flSetupTime );
	static C_CSPlayer* GetPlayerByIndex( int index );
	__int64 GetSteamID( );

	bool IsTeammate( C_CSPlayer* player, bool nigger = true );
	surfacedata_t *m_SurfaceData( );
	float m_flStamina( );
	bool CanShoot( bool bSkipRevolver = false, bool bSkipNextAttack = false );
	bool IsFiring( CUserCmd* pCmd );
	bool IsReloading( );

	Vector GetEyePosition( bool bModifyEyePos = true, bool bInterpolated = true );
	Vector GetHitboxPosition( int hitbox );

	// thank you stacker
	QAngle DecayAimPunchAngle( QAngle& vPunchAngleVelocity );

	void ClampBonesInSurroundingBox( matrix3x4_t *pMatrix, int nMask, bool bUseGame = false );

public:
	CCSGOPlayerAnimState*& m_PlayerAnimState( );
	QAngle& m_angEyeAngles( );
	int& m_nSurvivalTeam( );
	int& m_ArmorValue( );
	int& m_iAccount( );
	int& m_iFOV( );
	float GetFOV( );
	int& m_iShotsFired( );
	int &m_iAddonBits( );
	float& m_flFlashDuration( );
	float& m_flSecondFlashAlpha( );
	float& m_flVelocityModifier( );
	float& m_flLowerBodyYawTarget( );
	float& m_flSpawnTime( );
	float& m_flHealthShotBoostExpirationTime( );
	float& m_surfaceFriction( );
	bool& m_bHasHelmet( );
	bool& m_bHasHeavyArmor( );
	bool& m_bIsScoped( );
	bool &m_bIsWalking( );
	float &m_flThirdpersonRecoil( );
	bool& m_bWaitForNoAttack( );
	bool& m_bIsPlayerGhost( );
	std::array<int, 5>& m_vecPlayerPatchEconIndices( );
	int& m_iMatchStats_Kills( );
	int& m_iMatchStats_Deaths( );
	int& m_iMatchStats_HeadShotKills( );
	bool& m_bGunGameImmunity( );
	bool& m_bIsDefusing( );
	CBaseHandle& m_hCarriedHostage( );
	bool& m_bHasDefuser( );
	bool& m_bInNoDefuseArea( );
	bool& m_bDuckOverride( );
	void RunPostThink( );
	int SetNextThink( int tick );
	void Think( );
	void PreThink( );
	void PostThink( );

	bool	is( std::string networkname );
};

FORCEINLINE C_CSPlayer* ToCSPlayer( C_BaseEntity* pEnt ) {
	if( !pEnt || !pEnt->m_entIndex )
		return nullptr;

	return reinterpret_cast< C_CSPlayer* >( pEnt );
}
