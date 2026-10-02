#pragma once
#include "../../pandora.hpp"
#include "../../SDK/sdk.hpp"

class Miscellaneous {
public:
	float m_flThirdpersonTransparency;

	void ThirdPerson( );
	void Modulation( );
	void PreserveKillfeed( );
	void Clantag( );
	void RemoveSmoke( );
	void SkyBoxChanger( );
	void ForceCrosshair( );
	void OverrideFOV( CViewSetup* vsView );
	void PerformConvarRelated( );
	void UnlockConVars( );
	void RemoveVisualEffects( );
	bool PrecacheModel( const char *model );

	std::vector<Encrypted_t<CStaticProp>> m_vecStaticProps;

private:
	Color_f m_WallsColor = Color_f( 1.0f, 1.0f, 1.0f, 1.0f );
	Color_f m_SkyColor = Color_f( 1.0f, 1.0f, 1.0f, 1.0f );
	Color_f m_PropsColor = Color_f( 1.0f, 1.0f, 1.0f, 1.0f );
};

extern Miscellaneous g_Misc;