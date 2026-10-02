#include "../hooked.hpp"
#include "../../Utils/extern/FnvHash.hpp"
#include "../../Features/Rage/EnginePrediction.hpp"
#include "../../Features/Visuals/Visuals.hpp"

void __fastcall Hooked::EmitSound( IEngineSound *ecx, uint32_t, IRecipientFilter &filter, int iEntIndex, int iChannel,
						  const char *pSoundEntry, unsigned int nSoundEntryHash, const char *pSample,
						  float flVolume, int nSeed, float flAttenuation, int iFlags, int iPitch,
						  const Vector *pOrigin, const Vector *pDirection, void *pUtlVecOrigins,
						  bool bUpdatePositions, float soundtime, int speakerentity, int unk ) {
	g_Vars.globals.szLastHookCalled = XorStr( "7" );

	if( strstr( pSample, XorStr( "weapon" ) ) && ( strstr( pSample, XorStr( "draw" ) ) || strstr( pSample, XorStr( "deploy" ) ) ) ) {
		static uint32_t uPreviousSample = 0;
		const uint32_t uSample = hash_32_fnv1a( pSample );

		// don't play the same sound twice; happens when fakelagging
		// and pulling out a weapon (i.e. ur choke cycle resets, the
		// moment it does, the sound will re-play aswell as the viewmodel anim etc)
		if( uPreviousSample == uSample ) {
			flVolume = 0.f;
		}

		uPreviousSample = uSample;
	}


end:
	oEmitSound( ecx, filter, iEntIndex, iChannel, pSoundEntry, nSoundEntryHash, pSample, flVolume, nSeed, flAttenuation, iFlags, iPitch, pOrigin, pDirection, pUtlVecOrigins, bUpdatePositions, soundtime, speakerentity, unk );
}