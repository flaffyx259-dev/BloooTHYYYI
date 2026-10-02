#include "../Hooked.hpp"
#include "../../SDK/Classes/Player.hpp"
#include "../../SDK/Displacement.hpp"
#include "../../Features/Visuals/EventLogger.hpp"

bool __fastcall Hooked::PostNetworkDataReceived( void *ecx, void *, int commands_acknowledged ) {
	return oPostNetworkDataReceived( ecx, commands_acknowledged );
}