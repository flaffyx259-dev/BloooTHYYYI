#include "../Hooked.hpp"
#include "../../SDK/sdk.hpp"
#include "../../SDK/Classes/Player.hpp"

class RenderableInfo_t {
public:
	IClientRenderable *m_pRenderable;
	void *m_pAlphaProperty;
	int m_EnumCount;
	int m_nRenderFrame;
	unsigned short m_FirstShadow;
	unsigned short m_LeafList;
	short m_Area;
	uint16_t m_Flags;   // 0x0016
	uint16_t m_nSplitscreenEnabled; // 0x0018
	Vector m_vecBloatedAbsMins;
	Vector m_vecBloatedAbsMaxs;
	Vector m_vecAbsMins;
	Vector m_vecAbsMaxs;
	int pad;
};

void __fastcall Hooked::CalcRenderableWorldSpaceAABB_Bloated( void *ecx, void *edx, void *info, Vector &absMin, Vector &absMax ) {
	g_Vars.globals.szLastHookCalled = XorStr( "15" );
	//if( !g_Vars.esp.skip_occulusion )
		//return oCalcRenderableWorldSpaceAABB_Bloated( ecx, info, absMin, absMax );

	RenderableInfo_t *pInfo = ( RenderableInfo_t * )info;
	oCalcRenderableWorldSpaceAABB_Bloated( ecx, info, absMin, absMax );

	if( pInfo && pInfo->m_pRenderable ) {
		IClientUnknown *pClientUnknown = pInfo->m_pRenderable->GetIClientUnknown( );
		if( pClientUnknown ) {
			const auto pEntity = pClientUnknown->GetBaseEntity( );

			if( pEntity && pEntity->IsPlayer( ) ) {
				//if( pEntity->EntIndex( ) != g_pEngine->GetLocalPlayer( ) ) {
					pInfo->m_nSplitscreenEnabled = INT_MAX;

					absMin = Vector( -16384.0f, -16384.0f, -16384.0f );
					absMax = Vector( 16384.0f, 16384.0f, 16384.0f );
				//}
			}
		}
	}
}