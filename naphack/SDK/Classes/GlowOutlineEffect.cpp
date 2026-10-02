#include "../sdk.hpp"
#include "../../pandora.hpp"

int CGlowObjectManager::AddGlowBox( Vector vecStart, Vector vecEnd, float flSize, Color colColor, float flLifetime ) {
	const auto vec3tempOrientation = ( vecEnd - vecStart );
	Vector angGrTrajAngles;
	Math::VectorAngles( vec3tempOrientation, angGrTrajAngles );

	int nIndex = m_GlowBoxDefinitions.AddToTail( );
	m_GlowBoxDefinitions[ nIndex ].m_vPosition = vecStart;
	m_GlowBoxDefinitions[ nIndex ].m_angOrientation = { angGrTrajAngles.x, angGrTrajAngles.y, angGrTrajAngles.z };
	m_GlowBoxDefinitions[ nIndex ].m_vMins = Vector( 0, -flSize, -flSize );
	m_GlowBoxDefinitions[ nIndex ].m_vMaxs = Vector( vec3tempOrientation.Length( ), flSize, flSize );
	m_GlowBoxDefinitions[ nIndex ].m_colColor = colColor;
	m_GlowBoxDefinitions[ nIndex ].m_flBirthTimeIndex = g_pGlobalVars->curtime;
	m_GlowBoxDefinitions[ nIndex ].m_flTerminationTimeIndex = g_pGlobalVars->curtime + flLifetime;
	return nIndex;
}