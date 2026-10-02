#pragma once
#include "sdk.hpp"

typedef unsigned short ClientRenderHandle_t;

class CClientLeafSubSystemData;
using RenderableModelType_t = int;
class SetupRenderInfo_t;
class ClientLeafShadowHandle_t;
class ClientShadowHandle_t;

class IClientLeafSystem {
public:
	// Adds and removes renderables from the leaf lists
	// CreateRenderableHandle stores the handle inside pRenderable.
	virtual void CreateRenderableHandle( IClientRenderable *pRenderable, bool bRenderWithViewModels, RenderableTranslucencyType_t nType, RenderableModelType_t nModelType, uint32 nSplitscreenEnabled = 0xFFFFFFFF ) = 0; // = RENDERABLE_MODEL_UNKNOWN_TYPE ) = 0;
	virtual void RemoveRenderable( ClientRenderHandle_t handle ) = 0;
	virtual void AddRenderableToLeaves( ClientRenderHandle_t renderable, int nLeafCount, unsigned short *pLeaves ) = 0;
	virtual void SetTranslucencyType( ClientRenderHandle_t handle, RenderableTranslucencyType_t nType ) = 0;
	virtual void RenderInFastReflections( ClientRenderHandle_t handle, bool bRenderInFastReflections ) = 0;
	virtual void DisableShadowDepthRendering( ClientRenderHandle_t handle, bool bDisable ) = 0;
	virtual void DisableCSMRendering( ClientRenderHandle_t handle, bool bDisable ) = 0;

	void _CreateRenderableHandle( void *obj ) {
		typedef void( __thiscall *tOriginal )( void *, int, int, char, signed int, char );
		Memory::VCall<tOriginal>( this, 0 )( this, reinterpret_cast< uintptr_t >( obj ) + 0x4, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF );
	}
};