#include "../../pandora.hpp"
#include "../Hooked.hpp"
#include "../../SDK/Displacement.hpp"
#include "../../SDK/Classes/entity.hpp"

void __fastcall Hooked::TraceRay( void* thisptr, void*, const Ray_t& ray, unsigned int fMask, ITraceFilter* pTraceFilter, CGameTrace* pTrace ) {
	oTraceRay( thisptr, ray, fMask, pTraceFilter, pTrace );
}

void __fastcall Hooked::ClipRayCollideable( void* thisptr, void*, const Ray_t& ray, unsigned int fMask, ICollideable* pCollide, CGameTrace* pTrace )
{
	if( !thisptr || !pCollide || !g_Vars.globals.m_bInAwall )
		return oClipRayCollideable( thisptr, ray, fMask, pCollide, pTrace );

	// this used to raise the OBB of EVERY collideable while the autowall flag was up,
	// which put our client volumes above the server's and produced "visible here,
	// occluded there" misses. keep it to the autowall target only, where it covers
	// hitboxes poking out of the collision bounds.
	if( pCollide != g_Vars.globals.m_pAwallTargetCollideable )
		return oClipRayCollideable( thisptr, ray, fMask, pCollide, pTrace );

	auto backup = pCollide->OBBMaxs( ).z;
	pCollide->OBBMaxs( ).z += 5.0f;

	oClipRayCollideable( thisptr, ray, fMask, pCollide, pTrace );

	pCollide->OBBMaxs( ).z = backup;
}