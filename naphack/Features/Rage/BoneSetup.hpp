#pragma once
#include "../../SDK/sdk.hpp"
#include "../../SDK/Classes/Player.hpp"

enum BoneSetupFlags {
	None = 0,
	UseInterpolatedOrigin = ( 1 << 0 ),
	UseCustomOutput = ( 1 << 1 ),
	ForceInvalidateBoneCache = ( 1 << 2 ),
	AttachmentHelper = ( 1 << 3 ),
};

class BoneSetup {
public:
	Vector m_vecCustomOrigin = { };
	bool SetupBonesRebuild( C_CSPlayer* entity, matrix3x4_t* pBoneMatrix, int nBoneCount, int boneMask, float time, int flags, QAngle angCustomAngle = QAngle{ -1337.f,  -1337.f,  -1337.f } );
	bool BuildBones( C_CSPlayer *entity, int boneMask, int flags, matrix3x4_t *pBoneMatrix = nullptr, QAngle angCustomAngle = QAngle{ -1337.f,  -1337.f,  -1337.f } );

	bool BuildBonesSimple( C_CSPlayer *entity, const int max_bones, const int bone_mask, matrix3x4_t *bone_out );

	matrix3x4_t *m_boneMatrix = nullptr; 

	Vector m_vecOrigin{ };
	QAngle m_angAngles{ };

	int m_boneMask = 0;

	float m_flPoseParameters[ 24 ];
	float m_flWorldPoses[ 24 ];
	C_AnimationLayer *m_animLayers = nullptr;
	CCSGOPlayerAnimState m_animState;

	float m_flCurtime = 0.0f;

	C_CSPlayer *m_animating = nullptr;
};

extern BoneSetup g_BoneSetup;
struct LagRecord_t;
enum ESides;

class c_bone_builder {
private:
	void get_skeleton( Vector *position, Quaternion *q );
	void studio_build_matrices( CStudioHdr *hdr, const matrix3x4_t &world_transform, Vector *pos, Quaternion *q, int mask, matrix3x4_t *out, uint32_t *bone_computed );

public:
	bool filled{};

	bool ik_ctx{};
	bool attachments{};
	bool dispatch{};

	int mask{};
	int layer_count{};

	float time{};

	matrix3x4_t *matrix{};
	CStudioHdr *hdr{};
	C_AnimationLayer *layers{};
	C_CSPlayer *animating{};

	Vector origin{};
	QAngle angles{};
	QAngle eye_angles{};

	std::array<float, 24> poses{};
	std::array<float, 24> poses_world{};

	void store( C_CSPlayer *player, matrix3x4_t *matrix, int mask );
	void store( LagRecord_t* pRecord, int mask, ESides eSide = ( ESides )0 );
	void setup( );

	__forceinline void reset( ) {
		filled = false;
		ik_ctx = false;
		attachments = false;

		mask = 0;
		layer_count = 0;
		layer_count = 0;

		time = 0.f;

		matrix = nullptr;
		hdr = nullptr;
		layers = nullptr;
		animating = nullptr;

		origin.Init( );
		angles.Init( );
		eye_angles.Init( );

		poses = {};
		poses_world = {};
	}
};