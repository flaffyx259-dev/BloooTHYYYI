#pragma once
#include "../../SDK/sdk.hpp"
#include "../Rage/Animations.hpp"
#include <climits>

class SkinChanger {
public:
	void Create( );
	void Destroy( );
	void OnNetworkUpdate( bool start = true );
	void PostDataUpdateStart( C_CSPlayer *local );

	// High-ping stability: двойная защита от сети (2018 source).
	// START = до сетевых PostDataUpdate, END = восстановление после перезаписи
	// сервером, RENDER_START = жёсткая блокировка модели ножа каждый кадр.
	void OnFrameStageNotify( ClientFrameStage_t stage );
	void ApplySkins( C_CSPlayer *local );
	void LockKnifeModels( C_CSPlayer *local );

	// cl_fullupdate один раз при входе/респавне, чтобы сервер переслал все таблицы.
	void RequestFullUpdate( );
	void OnJoinServer( );

	// Кэш оригиналов: восстановить всё при выгрузке, иначе краш на дроп/инспект.
	struct OriginalItem_t {
		int definition_index = 0;
		int fallback_paint_kit = 0;
		int fallback_seed = 0;
		int fallback_stattrak = -1;
		float fallback_wear = 0.f;
		int entity_quality = 0;
		int item_id_high = 0;
		int model_index = 0;
		bool valid = false;
		// что записали мы: по этому отличаем свои значения от свежих серверных
		int written_definition_index = 0;
		int written_paint_kit = INT_MIN;
		// для какой сущности снят оригинал (индексы переиспользуются после дропа/спавна)
		void *entity = nullptr;
	};
	bool CacheOriginalIfNeeded( C_BaseAttributableItem *weapon );
	void RestoreOriginals( );

	void DestroyGlove( );
	bool m_bSkipCheck;
	std::unordered_map< std::string_view, std::string_view > m_icon_overrides;
	std::unordered_map< int, OriginalItem_t > m_originals;
private:
	// g_pModelInfo->GetModelIndex( ) resolves a string against the model dictionary
	// on every call. the paths never change at runtime, so memoise them instead of
	// doing that per network update.
	int GetCachedModelIndex( const char *model );
	std::unordered_map< std::uint32_t, int > m_model_index_cache;

	void EraseOverrideIfExistsByIndex( const int definition_index );
	void ApplyConfigOnAttributableItem( C_BaseAttributableItem *item, CVariables::skin_changer_data *config, const unsigned xuid_low );
	void GloveChanger( C_CSPlayer *local );
	static void SequenceProxyFn( const CRecvProxyData *proxy_data_const, void *entity, void *output );
	void DoSequenceRemapping( CRecvProxyData *data, C_BaseViewModel *entity );
	int GetNewAnimation( const uint32_t model, const int sequence, C_BaseViewModel *viewModel );
	CVariables::skin_changer_data *GetDataFromIndex( int idx );
	void ForceItemUpdate( C_CSPlayer *local );
	void UpdateHud( );

	RecvPropHook::Shared m_sequence_hook = nullptr;

	float lastSkinUpdate = 0.0f;
	float lastGloveUpdate = 0.0f;
	bool m_bFirstTimeGloveHandle = false;
};

extern SkinChanger g_SkinChanger;