#include "../hooked.hpp"
#include "../../SDK/Classes/Player.hpp"
#include "../../Features/Visuals/Models.hpp"
//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
struct matrix3x4_t;

void __fastcall Hooked::DrawModelExecute( void *ecx, void *EDX, IMatRenderContext *MatRenderContext, DrawModelState_t &DrawModelState, ModelRenderInfo_t &RenderInfo, matrix3x4_t *pCustomBoneToWorld ) {
	g_Vars.globals.szLastHookCalled = XorStr( "6" );
	if( !MatRenderContext || !pCustomBoneToWorld || !ecx || g_pEngine->IsDrawingLoadingImage( ) )
		return oDrawModelExecute( ecx, MatRenderContext, DrawModelState, RenderInfo, pCustomBoneToWorld );

	C_CSPlayer *local = C_CSPlayer::GetLocalPlayer( );
	if( !local )
		return oDrawModelExecute( ecx, MatRenderContext, DrawModelState, RenderInfo, pCustomBoneToWorld );

	static IMaterial *xblur_mat = g_pMaterialSystem->FindMaterial( XorStr( "dev/blurfilterx_nohdr" ), TEXTURE_GROUP_OTHER, true );
	static IMaterial *yblur_mat = g_pMaterialSystem->FindMaterial( XorStr( "dev/blurfiltery_nohdr" ), TEXTURE_GROUP_OTHER, true );
	static IMaterial *scope = g_pMaterialSystem->FindMaterial( XorStr( "dev/scope_bluroverlay" ), TEXTURE_GROUP_OTHER, true );
	static IMaterial *lens_dirt = g_pMaterialSystem->FindMaterial( XorStr( "models/weapons/shared/scope/scope_lens_dirt" ), TEXTURE_GROUP_OTHER, true );
	static IMaterial *scope_arc = g_pMaterialSystem->FindMaterial( XorStr( "models/weapons/shared/scope/scope_arc" ), TEXTURE_GROUP_OTHER, true );
	if( !xblur_mat->GetMaterialVarFlag( MATERIAL_VAR_NO_DRAW ) ) {
		xblur_mat->SetMaterialVarFlagSane( MATERIAL_VAR_NO_DRAW, true );
		yblur_mat->SetMaterialVarFlagSane( MATERIAL_VAR_NO_DRAW, true );
	}

	scope->SetMaterialVarFlagSane( MATERIAL_VAR_NO_DRAW, true );
	lens_dirt->SetMaterialVarFlagSane( MATERIAL_VAR_NO_DRAW, true );
	scope_arc->SetMaterialVarFlagSane( MATERIAL_VAR_NO_DRAW, true );

	g_Models.HandleModels( ecx, MatRenderContext, DrawModelState, RenderInfo, pCustomBoneToWorld );
	// g_Chams.OnDrawModelExecute( ECX, MatRenderContext, DrawModelState, RenderInfo, pCustomBoneToWorld );
}

void __fastcall Hooked::SetColorModulation( void *ecx, void *edx, float const *pColor ) {
	oSetColorModulation( ecx, pColor );
}
