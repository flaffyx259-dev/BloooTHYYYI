#pragma once

#include "../../SDK/sdk.hpp"
#include <optional>

enum EModelTypes {
	TYPE_DEFAULT,
	TYPE_MATERIAL,
	TYPE_FLAT,
	TYPE_GLOW,
};

struct AimbotMatrix_t {
	C_CSPlayer *m_pEntity;
	int m_nEntIndex;

	matrix3x4_t m_pMatrix[ MAXSTUDIOBONES ] = { };
	float m_flTime;
	float m_flAlpha;
	
	std::optional<std::function<void( float flAlphaModifier )>> fnOverrideFunc;
};

class Models {
	std::vector<AimbotMatrix_t> m_vecAimbotMatrix;

	bool m_bInitialised;
	bool m_bForceOverride;

	matrix3x4_t *m_pCustomMatrix;

	IMaterial *m_pMaterial;
	IMaterial *m_pShiny;
	IMaterial *m_pFlat;
	IMaterial *m_pGlow;

	bool InitialiseModelTypes( );
	void ApplyMaterial( CVariables::CHAMS *pCham, bool bIgnoreZ );
	bool IsRenderingChams( CVariables::CHAMS *pCham );
	bool ValidateEntity( C_CSPlayer *pEntity );

	// functions made for rendering models with custom matrices
	void HandleHistoryMatrix( C_CSPlayer *pEntity );
	void HandleBodyUpdateMatrix( C_CSPlayer *pEntity );
	void HandleDormantMatrix( C_CSPlayer *pEntity );
	void HandleRebuiltDrawModel( C_CSPlayer *pEntity, matrix3x4_t *pMatrix );

	C_BaseEntity *GetMoveParentEntity( C_BaseEntity *pEntity );
	bool AttachmentBelongsToPlayer( C_BaseEntity *pEntity, C_CSPlayer *pOwnerEntity );
public:
	bool m_bAllowedBoneSetup = false;

	std::function<void( )> fnDrawModel;
	std::function<void( float flAlpha )> fnSetAlpha;
	std::function<void( )> fnInvalidateMaterial;
	std::array<float, 65 > m_flLeAlpha;

	void HandleModels( void *ecx, IMatRenderContext *pMatRenderContext, DrawModelState_t &uDrawModelState, ModelRenderInfo_t &uRenderInfo, matrix3x4_t *pMatrix );

	void HandleDormantMatrices( );
	void HandleAimbotMatrices( );
	void AddAimbotMatrix( C_CSPlayer *pEntity, matrix3x4_t *pMatrix );
};

extern Models g_Models;