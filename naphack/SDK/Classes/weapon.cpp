#include "weapon.hpp"
#include "../displacement.hpp"
#include "../../pandora.hpp"
#include "PropManager.hpp"


float& C_BaseCombatWeapon::m_flNextPrimaryAttack( ) {
	return *( float* )( ( uintptr_t )this + Engine::Displacement.DT_BaseCombatWeapon.m_flNextPrimaryAttack );
}

float& C_BaseCombatWeapon::m_flNextSecondaryAttack( ) {
	return *( float* )( ( uintptr_t )this + Engine::Displacement.DT_BaseCombatWeapon.m_flNextSecondaryAttack );
}

float& C_BaseCombatWeapon::m_flPostponeFireReadyTime( ) {
	return *( float* )( ( uintptr_t )this + Engine::Displacement.DT_WeaponCSBase.m_flPostponeFireReadyTime );
}

int& C_BaseCombatWeapon::m_iBurstShotsRemaining( ) {
	return *( int* )( ( uintptr_t )this + Engine::Displacement.DT_WeaponCSBaseGun.m_iBurstShotsRemaining );
}

float& C_BaseCombatWeapon::m_fNextBurstShot( ) {
	return *( float* )( ( uintptr_t )this + Engine::Displacement.DT_WeaponCSBaseGun.m_fNextBurstShot );
}

CBaseHandle& C_BaseCombatWeapon::m_hOwner( ) {
	return *( CBaseHandle* )( ( uintptr_t )this + Engine::Displacement.DT_BaseCombatWeapon.m_hOwner );
}

int& C_BaseCombatWeapon::m_iClip1( ) {
	return *( int* )( ( uintptr_t )this + Engine::Displacement.DT_BaseCombatWeapon.m_iClip1 );
}

int& C_BaseCombatWeapon::m_iPrimaryReserveAmmoCount( ) {
	return *( int* )( ( uintptr_t )this + Engine::Displacement.DT_BaseCombatWeapon.m_iPrimaryReserveAmmoCount );
}

short& C_BaseCombatWeapon::m_iItemDefinitionIndex( ) {
	return *( short* )( ( uintptr_t )this + Engine::Displacement.DT_BaseCombatWeapon.m_iItemDefinitionIndex );
}

CUtlVector<IRefCounted*>& C_BaseCombatWeapon::m_CustomMaterials( ) {
	return *( CUtlVector< IRefCounted* >* )( ( uintptr_t )this + Engine::Displacement.DT_BaseCombatWeapon.m_CustomMaterials );
}

CUtlVector<IRefCounted*>& C_BaseCombatWeapon::m_CustomMaterials2( ) {
	return *( CUtlVector< IRefCounted* >* )( ( uintptr_t )this + 0x9DC );
}

bool& C_BaseCombatWeapon::m_bCustomMaterialInitialized( ) {
	return *( bool* )( ( uintptr_t )this + Engine::Displacement.DT_BaseCombatWeapon.m_bCustomMaterialInitialized );
}

float& C_WeaponCSBaseGun::m_flRecoilIndex( ) {
	return *( float* )( ( uintptr_t )this + Engine::Displacement.DT_WeaponCSBase.m_flRecoilIndex );
}

float& C_WeaponCSBaseGun::m_fLastShotTime( ) {
	return *( float* )( ( uintptr_t )this + Engine::Displacement.DT_WeaponCSBase.m_fLastShotTime );
}

float& C_WeaponCSBaseGun::m_fAccuracyPenalty( ) {
	return *( float* )( ( uintptr_t )this + Engine::Displacement.DT_WeaponCSBase.m_fAccuracyPenalty );
}

int& C_WeaponCSBaseGun::m_weaponMode( ) {
	return *( int* )( ( uintptr_t )this + Engine::Displacement.DT_WeaponCSBase.m_weaponMode );
}

int& C_WeaponCSBaseGun::m_zoomLevel( ) {
	return *( int* )( ( uintptr_t )this + Engine::Displacement.DT_WeaponCSBaseGun.m_zoomLevel );
}

int& C_WeaponCSBaseGun::m_Activity( ) {
	static unsigned int m_activity = Memory::FindInDataMap( GetPredDescMap( ), XorStr( "m_Activity" ) );
	return *( int* )( ( uintptr_t )this + m_activity );
}

float& C_WeaponCSBaseGun::m_flThrowStrength( ) {
	return *( float* )( ( uintptr_t )this + Engine::Displacement.DT_BaseCSGrenade.m_flThrowStrength );
}

float& C_WeaponCSBaseGun::m_fThrowTime( ) {
	return *( float* )( ( uintptr_t )this + Engine::Displacement.DT_BaseCSGrenade.m_fThrowTime );
}

bool& C_WeaponCSBaseGun::m_bPinPulled( ) {
	return *( bool* )( ( uintptr_t )this + Engine::Displacement.DT_BaseCSGrenade.m_bPinPulled );
}

Encrypted_t<CCSWeaponInfo> C_WeaponCSBaseGun::GetCSWeaponData( ) {
	//using Fn = CCSWeaponInfo * ( __thiscall* )( void* );
	//return Memory::VCall<Fn>( this, Index::C_WeaponCSBaseGun::GetCSWeaponData )( this );
	if( !this )
		return nullptr;

	return g_pWeaponSystem->GetWeaponInfo( this->m_iItemDefinitionIndex( ) );
}
//8B 81 ? ? ? ? 85 C0 0F 84 ? ? ? ? C3
//
//10B8F32C
//
//10B8FA44

float C_WeaponCSBaseGun::GetMaxSpeed( ) {
	auto pWeaponData = GetCSWeaponData( );
	if( !pWeaponData.IsValid( ) )
		return 250.0f;

	if( m_zoomLevel( ) == 0 )
		return pWeaponData->m_flMaxSpeed;

	return pWeaponData->m_flMaxSpeed2;
}

float C_WeaponCSBaseGun::GetSpread( ) {
	using Fn = float( __thiscall* )( void* );
	return Memory::VCall<Fn>( this, Index::C_WeaponCSBaseGun::GetSpread )( this );
}

float C_WeaponCSBaseGun::GetInaccuracy( ) {
	using Fn = float( __thiscall* )( void* );
	return Memory::VCall<Fn>( this, Index::C_WeaponCSBaseGun::GetInnacuracy )( this );
}

void C_WeaponCSBaseGun::UpdateAccuracyPenalty( ) {
	using Fn = void( __thiscall* )( void* );
	return Memory::VCall<Fn>( this, Index::C_WeaponCSBaseGun::UpdateAccuracyPenalty )( this );
}

bool C_WeaponCSBaseGun::IsFireTime( ) {
	return ( g_pGlobalVars->curtime >= m_flNextPrimaryAttack( ) );
}

bool C_WeaponCSBaseGun::IsSecondaryFireTime( ) {
	return ( g_pGlobalVars->curtime >= m_flNextSecondaryAttack( ) );
}

bool C_WeaponCSBaseGun::CanScope( ) {
	if( !this )
		return false;

	auto pWeaponData = GetCSWeaponData( );
	if( !pWeaponData.IsValid( ) )
		return false;

	return pWeaponData->m_iWeaponType == WEAPONTYPE_SNIPER_RIFLE || m_iItemDefinitionIndex( ) == WEAPON_SG553 || m_iItemDefinitionIndex( ) == WEAPON_AUG;
}

// hmm ive seen this code somewhere (s/o dex nitro)
Vector C_WeaponCSBaseGun::CalculateSpread( int seed, float inaccuracy, float spread, bool revolver2 ) {
	int        item_def_index;
	float      recoil_index, r1, r2, r3, r4, s1, c1, s2, c2;

	// if we have no bullets, we have no spread.
	auto wep_info = GetCSWeaponData( );
	if( !wep_info.IsValid( ) || !wep_info->m_iBullets )
		return {};

	// get some data for later.
	item_def_index = m_iItemDefinitionIndex( );
	recoil_index = m_flRecoilIndex( );

	// seed randomseed.
	RandomSeed( ( seed & 0xff ) + 1 );

	// generate needed floats.
	r1 = RandomFloat( 0.f, 1.f );
	r2 = RandomFloat( 0.f, M_PI * 2.f );
	r3 = RandomFloat( 0.f, 1.f );
	r4 = RandomFloat( 0.f, M_PI * 2.f );

	// revolver secondary spread.
	if( item_def_index == WEAPON_REVOLVER && revolver2 ) {
		r1 = 1.f - ( r1 * r1 );
		r3 = 1.f - ( r3 * r3 );
	}

	// negev spread.
	else if( item_def_index == WEAPON_NEGEV && recoil_index < 3.f ) {
		for( int i{ 3 }; i > recoil_index; --i ) {
			r1 *= r1;
			r3 *= r3;
		}

		r1 = 1.f - r1;
		r3 = 1.f - r3;
	}

	// get needed sine / cosine values.
	c1 = std::cos( r2 );
	c2 = std::cos( r4 );
	s1 = std::sin( r2 );
	s2 = std::sin( r4 );

	// calculate spread vector.
	return {
		( c1 * ( r1 * inaccuracy ) ) + ( c2 * ( r3 * spread ) ),
		( s1 * ( r1 * inaccuracy ) ) + ( s2 * ( r3 * spread ) ),
		0.f
	};
}


bool C_WeaponCSBaseGun::IsInThrow( ) {
	if( !m_bPinPulled( ) ) {
		float throwTime = m_fThrowTime( );

		if( throwTime > 0 )
			return true;
	}
	return false;
}

wchar_t* C_WeaponCSBaseGun::GetItemName( bool bRenderNametag ) {
	static auto m_ItemOffset = Engine::g_PropManager.GetOffset( XorStr( "DT_BaseCombatWeapon" ), XorStr( "m_Item" ) );

	using GetItemName_t = wchar_t* ( __thiscall* )( void*, bool );
	static GetItemName_t oGetItemName = reinterpret_cast< GetItemName_t >( Engine::Displacement.Function.m_GetItemName );
	
	g_Vars.globals.m_bDontCallingTheSkinNameMaybe = true;
	auto result = oGetItemName( reinterpret_cast< void* >( uintptr_t( this ) + m_ItemOffset ), !bRenderNametag );
	g_Vars.globals.m_bDontCallingTheSkinNameMaybe = false;

	return result;
}

CUtlVector<IRefCounted*>& C_EconItemView::m_CustomMaterials( ) {
	static auto _m_CustomMaterials = Engine::Displacement.DT_BaseAttributableItem.m_Item + 0x14;
	return *( CUtlVector< IRefCounted* >* )( ( uintptr_t )this + _m_CustomMaterials );
}

CUtlVector<IRefCounted *> &C_EconItemView::m_CustomMaterials2( ) {
	return *( CUtlVector< IRefCounted * >* )( ( uintptr_t )this + Engine::Displacement.DT_BaseCombatWeapon.m_CustomMaterials );
}

CUtlVector<IRefCounted*>& C_EconItemView::m_VisualsDataProcessors( ) {
	static auto _m_VisualsDataProcessors = Engine::Displacement.DT_BaseAttributableItem.m_Item + 0x220;
	return *( CUtlVector< IRefCounted* >* )( ( uintptr_t )this + _m_VisualsDataProcessors );
}

int32_t& C_EconItemView::m_bInitialized( ) {
	return *( int32_t* )( ( int32_t )this + Engine::Displacement.DT_BaseAttributableItem.m_bInitialized );
}

int32_t& C_EconItemView::m_iEntityLevel( ) {
	return *( int32_t* )( ( int32_t )this + Engine::Displacement.DT_BaseAttributableItem.m_iEntityLevel );
}

int32_t& C_EconItemView::m_iAccountID( ) {
	return *( int32_t* )( ( int32_t )this + Engine::Displacement.DT_BaseAttributableItem.m_iAccountID );
}

int32_t& C_EconItemView::m_iItemIDLow( ) {
	return *( int32_t* )( ( int32_t )this + Engine::Displacement.DT_BaseAttributableItem.m_iItemIDLow );
}

int32_t& C_EconItemView::m_iItemIDHigh( ) {
	return *( int32_t* )( ( int32_t )this + Engine::Displacement.DT_BaseAttributableItem.m_iItemIDHigh );
}

int32_t& C_EconItemView::m_iEntityQuality( ) {
	return *( int32_t* )( ( int32_t )this + Engine::Displacement.DT_BaseAttributableItem.m_iEntityQuality );
}

uint32_t& C_EconItemView::m_nFallbackPaintKit( ) {
	return *( uint32_t* )( ( int32_t )this + Engine::Displacement.DT_BaseAttributableItem.m_nFallbackPaintKit );
}

uint32_t& C_EconItemView::m_nFallbackSeed( ) {
	return *( uint32_t* )( ( int32_t )this + Engine::Displacement.DT_BaseAttributableItem.m_nFallbackSeed );
}

uint32_t& C_EconItemView::m_nFallbackStatTrak( ) {
	return *( uint32_t* )( ( int32_t )this + Engine::Displacement.DT_BaseAttributableItem.m_nFallbackStatTrak );
}

float& C_EconItemView::m_flFallbackWear( ) {
	return *( float* )( ( int32_t )this + Engine::Displacement.DT_BaseAttributableItem.m_flFallbackWear );
}

str_32& C_EconItemView::m_szCustomName( ) {
	return *( str_32* )( ( int32_t )this + Engine::Displacement.DT_BaseAttributableItem.m_szCustomName );
}

int16_t& C_EconItemView::m_iItemDefinitionIndex( ) {
	return *( int16_t* )( ( int32_t )this + Engine::Displacement.DT_BaseAttributableItem.m_iItemDefinitionIndex );
}

uint64_t& C_BaseAttributableItem::m_OriginalOwnerXuid( ) {
	return *( uint64_t* )( ( int32_t )this + Engine::Displacement.DT_BaseAttributableItem.m_OriginalOwnerXuidLow );
}

int32_t& C_BaseAttributableItem::m_OriginalOwnerXuidLow( ) {
	return *( int32_t* )( ( int32_t )this + Engine::Displacement.DT_BaseAttributableItem.m_OriginalOwnerXuidLow );
}

int32_t& C_BaseAttributableItem::m_OriginalOwnerXuidHigh( ) {
	return *( int32_t* )( ( int32_t )this + Engine::Displacement.DT_BaseAttributableItem.m_OriginalOwnerXuidHigh );
}

int32_t& C_BaseAttributableItem::m_nFallbackPaintKit( ) {
	return *( int32_t* )( ( int32_t )this + Engine::Displacement.DT_BaseAttributableItem.m_nFallbackPaintKit );
}

int32_t& C_BaseAttributableItem::m_nFallbackSeed( ) {
	return *( int32_t* )( ( int32_t )this + Engine::Displacement.DT_BaseAttributableItem.m_nFallbackSeed );
}

int32_t& C_BaseAttributableItem::m_nFallbackStatTrak( ) {
	return *( int32_t* )( ( int32_t )this + Engine::Displacement.DT_BaseAttributableItem.m_nFallbackStatTrak );
}

int32_t& C_BaseAttributableItem::m_flFallbackWear( ) {
	return *( int32_t* )( ( int32_t )this + Engine::Displacement.DT_BaseAttributableItem.m_flFallbackWear );
}

str_32& C_BaseAttributableItem::m_szCustomName( ) {
	return *( str_32* )( ( int32_t )this + Engine::Displacement.DT_BaseAttributableItem.m_szCustomName );
}

CHandle<C_BaseEntity> C_BaseAttributableItem::m_hWeaponWorldModel( ) {
	return *( CHandle<C_BaseEntity>* )( ( int32_t )this + Engine::Displacement.DT_BaseCombatWeapon.m_hWeaponWorldModel );
}

CHandle<C_BaseEntity> C_BaseViewModel::m_hOwner( ) {
	return *( CHandle<C_BaseEntity>* )( ( int32_t )this + Engine::Displacement.DT_BaseViewModel.m_hOwner );
}

CHandle<C_BaseCombatWeapon> C_BaseViewModel::m_hWeapon( ) {
	return *( CHandle<C_BaseCombatWeapon>* )( ( int32_t )this + Engine::Displacement.DT_BaseViewModel.m_hWeapon );
}

void C_BaseViewModel::SendViewModelMatchingSequence( int sequence ) {
	using Fn = void( __thiscall* )( void*, int );
	return  Memory::VCall<Fn>( this, 246 )( this, sequence );
}

int& C_BaseEntity::m_nModelIndex( ) {
	return *( int* )( ( int32_t )this + Engine::Displacement.DT_BaseEntity.m_nModelIndex );
}

int& C_BaseEntity::m_nPrecipType( ) {
	return *( int* )( ( uintptr_t )this + Engine::Displacement.DT_Precipitation.m_nPrecipType );
}
