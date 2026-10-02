#include "SkinChanger.hpp"
#include "../../SDK/variables.hpp"
#include "../../SDK/Classes/weapon.hpp"
#include "../../SDK/Classes/player.hpp"
#include "../../SDK/Valve/CBaseHandle.hpp"
#include "KitParser.hpp"
#include "../../SDK/Classes/PropManager.hpp"
#include <algorithm>
#include <memory.h>
#include "../../SDK/Valve/recv_swap.hpp"
#include "../../Utils/extern/FnvHash.hpp"
#include "../../SDK/displacement.hpp"
#include <Windows.h>
#include <cstring>

static auto IsKnife( const int i ) -> bool {
	return ( i >= WEAPON_KNIFE_BAYONET && i < GLOVE_STUDDED_BLOODHOUND ) || i == WEAPON_KNIFE_T || i == WEAPON_KNIFE;
}

static CHandle< C_BaseAttributableItem > hGloveHandle{ };
static uintptr_t hGloveRenderHandle{ };

SkinChanger g_SkinChanger;

int SkinChanger::GetCachedModelIndex( const char *model ) {
	if( !model )
		return -1;

	const auto uHash = hash_32_fnv1a( model );

	const auto it = m_model_index_cache.find( uHash );
	if( it != m_model_index_cache.end( ) )
		return it->second;

	const int nIndex = g_pModelInfo->GetModelIndex( model );

	// a model that is not precached yet resolves to -1; do not cache that, it can
	// become valid later in the round.
	if( nIndex != -1 )
		m_model_index_cache.emplace( uHash, nIndex );

	return nIndex;
}

void SkinChanger::Create( ) {
	RecvProp *prop = nullptr;
	Engine::g_PropManager.GetProp( XorStr( "DT_BaseViewModel" ), XorStr( "m_nSequence" ), &prop );
	m_sequence_hook = std::make_shared<RecvPropHook>( prop, &SequenceProxyFn );
}

void SkinChanger::Destroy( ) {
	// Кэш оригиналов: обязаны восстановить всё, иначе краш на дроп/инспект.
	RestoreOriginals( );
	m_originals.clear( );
	m_icon_overrides.clear( );
	m_model_index_cache.clear( );

	if( m_sequence_hook ) {
		m_sequence_hook->Unhook( );
		m_sequence_hook.reset( );
	}
}

static void FillOriginal( SkinChanger::OriginalItem_t &orig, C_BaseAttributableItem *weapon ) {
	auto &item = weapon->m_Item( );
	orig.definition_index = item.m_iItemDefinitionIndex( );
	orig.fallback_paint_kit = item.m_nFallbackPaintKit( );
	orig.fallback_seed = item.m_nFallbackSeed( );
	orig.fallback_stattrak = item.m_nFallbackStatTrak( );
	orig.fallback_wear = item.m_flFallbackWear( );
	orig.entity_quality = item.m_iEntityQuality( );
	orig.item_id_high = item.m_iItemIDHigh( );
	orig.model_index = weapon->m_nModelIndex( );
	orig.entity = weapon;
	orig.valid = true;
}

bool SkinChanger::CacheOriginalIfNeeded( C_BaseAttributableItem *weapon ) {
	if( !weapon )
		return false;

	const int ent_index = weapon->EntIndex( );
	if( ent_index <= 0 )
		return false;

	auto &item = weapon->m_Item( );
	auto it = m_originals.find( ent_index );

	if( it != m_originals.end( ) && it->second.valid && it->second.entity == weapon ) {
		auto &orig = it->second;

		// в полях лежит то, что записали мы - оригинал уже снят
		if( item.m_iItemDefinitionIndex( ) == orig.written_definition_index &&
			item.m_nFallbackPaintKit( ) == orig.written_paint_kit )
			return false;

		// то же оружие, сервер просто заново прислал свои значения (full update):
		// освежаем оригинал, но это не новое оружие - ForceItemUpdate не нужен
		if( item.m_iItemDefinitionIndex( ) == orig.definition_index ) {
			FillOriginal( orig, weapon );
			return false;
		}
	}

	// новое оружие на этом индексе (покупка, подбор, спавн)
	OriginalItem_t orig;
	FillOriginal( orig, weapon );
	m_originals[ ent_index ] = orig;
	return true;
}

void SkinChanger::RestoreOriginals( ) {
	if( m_originals.empty( ) )
		return;

	for( auto &[ent_index, orig] : m_originals ) {
		if( !orig.valid )
			continue;

		auto *ent = g_pEntityList->GetClientEntity( ent_index );
		if( !ent )
			continue;

		auto *weapon = reinterpret_cast< C_BaseAttributableItem * >( ent );
		if( !weapon )
			continue;

		// индекс мог достаться другой сущности - в чужую память не пишем
		if( reinterpret_cast< void * >( weapon ) != orig.entity )
			continue;

		// возвращаем только то, что меняли сами
		if( weapon->m_Item( ).m_iItemDefinitionIndex( ) != orig.written_definition_index ||
			weapon->m_Item( ).m_nFallbackPaintKit( ) != orig.written_paint_kit )
			continue;

		weapon->m_Item( ).m_iItemDefinitionIndex( ) = orig.definition_index;
		weapon->m_Item( ).m_nFallbackPaintKit( ) = orig.fallback_paint_kit;
		weapon->m_Item( ).m_nFallbackSeed( ) = orig.fallback_seed;
		weapon->m_Item( ).m_nFallbackStatTrak( ) = orig.fallback_stattrak;
		weapon->m_Item( ).m_flFallbackWear( ) = orig.fallback_wear;
		weapon->m_Item( ).m_iEntityQuality( ) = orig.entity_quality;
		weapon->m_Item( ).m_iItemIDHigh( ) = orig.item_id_high;

		if( orig.model_index != 0 && weapon->m_nModelIndex( ) != orig.model_index )
			weapon->SetModelIndex( orig.model_index );
	}

	if( auto pLocal = C_CSPlayer::GetLocalPlayer( ); pLocal && !pLocal->IsDead( ) )
		ForceItemUpdate( pLocal );

	m_originals.clear( );
	m_icon_overrides.clear( );
}

void SkinChanger::RequestFullUpdate( ) {
	// 2018 движок: cl_fullupdate заставляет сервер переслать все DataTable.
	// Дублируем внутренним флагом m_bForceFullUpdate (deltaTick = -1), который
	// уже обрабатывается в FrameStageNotify. Троттлим по времени, чтобы не
	// спамить каждый кадр на пинге 200+.
	static float flLastRequest = -10.f;
	float flNow = 0.f;
	if( g_pGlobalVars.IsValid( ) )
		flNow = g_pGlobalVars->realtime;
	if( flNow - flLastRequest < 1.f )
		return;
	flLastRequest = flNow;

	g_Vars.globals.m_bForceFullUpdate = true;
}

void SkinChanger::OnJoinServer( ) {
	// Вызывается один раз при входе/респавне (см. GameEvent player_spawn /
	// player_connect_full / game_start). Сбрасываем троттлинг скинов и просим
	// полный апдейт, чтобы скины легли на свежие таблицы.
	lastSkinUpdate = 0.f;
	lastGloveUpdate = 0.f;

	// индексы моделей у каждой карты свои
	m_model_index_cache.clear( );
}

void SkinChanger::OnNetworkUpdate( bool start ) {
	auto &global = g_Vars.m_global_skin_changer;
	m_bSkipCheck = true;

	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !g_pEngine->IsConnected( ) ) {
		m_bSkipCheck = false;
		return;
	}

	if( !m_bFirstTimeGloveHandle ) {
		hGloveHandle.Set( nullptr );
		m_bFirstTimeGloveHandle = true;
	}

	if( !start ) {
		// HIGH-PING FIX: сервер уже перезаписал fallback между START и END.
		// Восстанавливаем наши значения ВТОРОЙ раз до рендера.
		if( global.m_active && pLocal && !pLocal->IsDead( ) )
			PostDataUpdateStart( pLocal );

		if( global.m_update_skins && !global.m_update_gloves ) {
			float flDeltaTime = g_pGlobalVars->realtime - lastSkinUpdate;
			if( flDeltaTime >= 1.f ) {
				ForceItemUpdate( pLocal );
				global.m_update_skins = false;
				lastSkinUpdate = g_pGlobalVars->realtime;
			}
		}

		if( ( !global.m_active || !global.m_glove_changer ) || global.m_update_gloves ) {
			auto pGlove = hGloveHandle.Get( );
			if( pGlove && hGloveHandle.IsValid( ) ) {
				auto pNetworkable = pGlove->GetClientNetworkable( );
				if( pNetworkable ) {
					pNetworkable->SetDestroyedOnRecreateEntities( );
					pNetworkable->Release( );
				}

				hGloveHandle.Set( nullptr );
			}

			const auto glove_config = GetDataFromIndex( global.m_gloves_idx );
			if( ( ( global.m_update_gloves && g_pGlobalVars->realtime - lastGloveUpdate >= 0.5f ) || ( glove_config && !glove_config->m_enabled && glove_config->m_executed ) ) && pLocal && !pLocal->IsDead( ) ) {
				g_Vars.globals.m_bForceFullUpdate = true;

				if( global.m_update_gloves )
					lastGloveUpdate = g_pGlobalVars->realtime;

				global.m_update_gloves = false;
				global.m_update_skins = false;

				if( glove_config )
					glove_config->m_executed = false;
			}
		}

		if( pLocal && global.m_glove_changer ) {
			GloveChanger( pLocal );
		}

		m_bSkipCheck = false;
		return;
	}

	if( !global.m_active ) {
		m_bSkipCheck = false;
		return;
	}

	// Первое применение: до сетевых обновлений.
	PostDataUpdateStart( pLocal );

	m_bSkipCheck = false;
}

void SkinChanger::ApplySkins( C_CSPlayer *local ) {
	if( !local || local->IsDead( ) )
		return;

	if( !g_Vars.m_global_skin_changer.m_active )
		return;

	PostDataUpdateStart( local );
}

void SkinChanger::OnFrameStageNotify( ClientFrameStage_t stage ) {
	// Вне игры сущности уже уничтожены — stale ent_index из прошлого матча
	// нельзя ресторировать на новой карте, просто чистим кэш.
	if( !g_pEngine->IsInGame( ) || !g_pEngine->IsConnected( ) ) {
		if( !m_originals.empty( ) )
			m_originals.clear( );
		if( !m_icon_overrides.empty( ) )
			m_icon_overrides.clear( );
		m_model_index_cache.clear( ); // индексы моделей свои на каждой карте
		return;
	}

	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	if( g_pEngine->IsDrawingLoadingImage( ) )
		return;

	auto &global = g_Vars.m_global_skin_changer;
	if( !global.m_active ) {
		// Скинчейнджер выключен, но оригиналы ещё подменены — вернуть серверные
		// значения, иначе дроп/инспект уронит игру.
		if( !m_originals.empty( ) )
			RestoreOriginals( );
		return;
	}

	switch( stage ) {
		case FRAME_NET_UPDATE_POSTDATAUPDATE_START:
			// Первое применение: ловим данные до того, как сервер их обновит.
			// Итерируем ВСЕ оружия из m_hMyWeapons, чтобы при свиче скины
			// уже были готовы (убирает задержку в 1 кадр).
			ApplySkins( pLocal );
			break;

		default:
			break;
	}
}

void SkinChanger::LockKnifeModels( C_CSPlayer *pLocal ) {
	if( !pLocal || pLocal->IsDead( ) )
		return;

	auto &global = g_Vars.m_global_skin_changer;
	if( !global.m_active || !global.m_knife_changer )
		return;

	const auto itKnife = g_KitParser.vecWeaponInfo.find( global.m_knife_idx );
	if( itKnife == g_KitParser.vecWeaponInfo.end( ) )
		return;

	const auto &knife_info = itKnife->second;
	const int knife_model = GetCachedModelIndex( knife_info.model.c_str( ) );
	if( knife_model == -1 )
		return;

	// 1) Все оружия в инвентаре: у ножей держим override-модель.
	{
		auto weapons = pLocal->m_hMyWeapons( );
		if( weapons ) {
			for( int i = 0; i < 48; ++i ) {
				auto *weapon = reinterpret_cast< C_BaseAttributableItem * >( weapons[ i ].Get( ) );
				if( !weapon )
					continue;

				const int def = weapon->m_Item( ).m_iItemDefinitionIndex( );
				// Только ножи (в т.ч. уже заменённые): обычные пушки не трогаем.
				if( !IsKnife( def ) )
					continue;

				if( weapon->m_nModelIndex( ) != knife_model )
					weapon->SetModelIndex( knife_model );
			}
		}
	}

	// 2) Viewmodel + worldmodel активного оружия: движок сбрасывает их при
	// смене оружия / на высоком пинге после интерполяции.
	const auto pViewModel = reinterpret_cast< C_BaseViewModel * >( pLocal->m_hViewModel( ).Get( ) );
	if( !pViewModel )
		return;

	const auto pVMWeapon = reinterpret_cast< C_BaseAttributableItem * >( pViewModel->m_hWeapon( ).Get( ) );
	if( !pVMWeapon )
		return;

	const int vm_def = pVMWeapon->m_Item( ).m_iItemDefinitionIndex( );
	if( !IsKnife( vm_def ) )
		return;

	if( pViewModel->m_nModelIndex( ) != knife_model )
		pViewModel->SetModelIndex( knife_model );

	if( auto *weapon_data = reinterpret_cast< C_WeaponCSBaseGun * >( pVMWeapon )->GetCSWeaponData( ).Xor( ) ) {
		const int world_model = GetCachedModelIndex( weapon_data->m_szWorldModel );
		if( world_model != -1 ) {
			if( auto *world = pVMWeapon->m_hWeaponWorldModel( ).Get( ) ) {
				if( reinterpret_cast< C_BaseEntity * >( world )->m_nModelIndex( ) != world_model )
					reinterpret_cast< C_BaseEntity * >( world )->SetModelIndex( world_model );
			}
		}
	}
}

bool can_force_item_update( C_BaseCombatWeapon *item ) {
	for( auto &e : item->m_CustomMaterials( ) ) {
		if( e ) {
			const auto &is_valid = *( bool * )( ( uintptr_t )e + 4 + 20 + 4 ); // https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/materialsystem/custom_material.h#L74
			if( !is_valid )
				return false;
		}
	}
	return true;
}

class CCStrike15ItemSystem;
class CCStrike15ItemSchema;

void SkinChanger::PostDataUpdateStart( C_CSPlayer *pLocal ) {
	if( !pLocal )
		return;

	if( pLocal->IsDead( ) )
		return;

	const auto local_index = pLocal->EntIndex( );

	player_info_t player_info;
	if( !g_pEngine->GetPlayerInfo( local_index, &player_info ) )
		return;

	auto &global = g_Vars.m_global_skin_changer;

	// Handle weapon configs
	// Итерация по ВСЕМ оружиям из m_hMyWeapons (не только активному), чтобы при
	// смене оружия скины уже были готовы и не было задержки в 1 кадр. Критично
	// на высоком пинге, где свич + сетевой апдейт приходят в одном пакете.
	{
		auto weapons = pLocal->m_hMyWeapons( );
		if( !weapons )
			return;
		for( int i = 0; i < 48; ++i ) {
			auto weapon = ( C_BaseAttributableItem * )weapons[ i ].Get( );
			if( !weapon )
				continue;

			CacheOriginalIfNeeded( weapon );

			auto &definition_index = weapon->m_Item( ).m_iItemDefinitionIndex( );

			auto idx = IsKnife( definition_index ) ? global.m_knife_idx : definition_index;

			const auto active_conf = GetDataFromIndex( idx );

			// GetDataFromIndex( ) returns null when nothing matches, and m_knife_idx
			// defaults to 0 which has no entry - so this wrote through a null pointer
			// before the check below ever ran.
			if( active_conf && IsKnife( definition_index ) )
				active_conf->m_enabled = g_Vars.m_global_skin_changer.m_knife_changer;

			if( active_conf ) {
				ApplyConfigOnAttributableItem( weapon, active_conf, player_info.xuid_low );
			}
			else {
				EraseOverrideIfExistsByIndex( definition_index );
			}
		}
	}

	const auto pViewModel = ( C_BaseViewModel * )pLocal->m_hViewModel( ).Get( );
	if( !pViewModel )
		return;

	const auto pViewModelWeapon = ( C_BaseAttributableItem * )pViewModel->m_hWeapon( ).Get( );
	if( !pViewModelWeapon )
		return;

	auto idx = pViewModelWeapon->m_Item( ).m_iItemDefinitionIndex( );
	const auto itWeaponInfo = g_KitParser.vecWeaponInfo.find( idx );
	if( itWeaponInfo != g_KitParser.vecWeaponInfo.end( ) ) {
		const auto &override_info = itWeaponInfo->second;
		const auto override_model_index = GetCachedModelIndex( override_info.model.c_str( ) );

		const auto weapon = reinterpret_cast< C_WeaponCSBaseGun * >( pViewModelWeapon );
		if( weapon ) {
			// old-build behavior: SetModelIndex, but only when it actually
			// differs (else the viewmodel animation resets every update) and
			// never with -1 (unprecached model would blank the viewmodel).
			if( override_model_index != -1 && pViewModel->m_nModelIndex( ) != override_model_index ) {
				pViewModel->SetModelIndex( override_model_index );

				// after a model swap the rate sometimes sticks at 0 -> the
				// sequence is set but frozen. kick it back to normal speed.
				if( pViewModel->m_flPlaybackRate( ) == 0.0f )
					pViewModel->m_flPlaybackRate( ) = 1.0f;
			}

			auto weapondata = weapon->GetCSWeaponData( );
			if( weapondata.IsValid( ) ) {
				const auto override_world_model_index = GetCachedModelIndex( weapondata->m_szWorldModel );
				const auto world_model = pViewModelWeapon->m_hWeaponWorldModel( ).Get( );
				if( world_model && override_world_model_index != -1 && world_model->m_nModelIndex( ) != override_world_model_index ) {
					world_model->SetModelIndex( override_world_model_index );
				}
			}
		}
	}
}

void SkinChanger::DestroyGlove( ) {
	if( g_pEngine->IsConnected( ) || g_pEngine->IsInGame( ) )
		return;

	auto pGlove = hGloveHandle.Get( );
	if( pGlove && hGloveHandle.IsValid( ) ) {
		auto pNetworkable = pGlove->GetClientNetworkable( );
		if( pNetworkable ) {
			pNetworkable->SetDestroyedOnRecreateEntities( );
			pNetworkable->Release( );
		}

		hGloveHandle.Set( nullptr );
	}
}

void SkinChanger::EraseOverrideIfExistsByIndex( const int definition_index ) {
	if( g_KitParser.vecWeaponInfo.count( definition_index ) <= 0 )
		return;

	// We have info about the item not needed to be overridden
	const auto &original_item = g_KitParser.vecWeaponInfo.at( definition_index );
	auto &icon_override_map = m_icon_overrides;

	if( original_item.icon.empty( ) )
		return;

	const auto override_entry = icon_override_map.find( std::string_view( original_item.icon ) );

	// We are overriding its icon when not needed
	if( override_entry != end( icon_override_map ) )
		icon_override_map.erase( override_entry ); // Remove the leftover override
}

#include "../../SDK/Valve/utlmap.hpp"

struct WeaponPaintableMaterial_t {
	char m_szName[ 128 ];
	char m_szOriginalMaterialName[ 128 ];
	char m_szFolderName[ 128 ];
	int m_nViewModelSize;						// texture size
	int m_nWorldModelSize;						// texture size
	float m_flWeaponLength;
	float m_flUVScale;
	bool m_bBaseTextureOverride;
	bool m_bMirrorPattern;
};

void ModifyPaintkitColor( C_BaseAttributableItem *pAttribute, CVariables::skin_changer_data *pConfig ) {
	int nKit = pConfig->m_filter_paint_kits ? pConfig->m_paint_kit : pConfig->m_paint_kit_no_filter;

	if( !pAttribute )
		return;

	// nothing to do unless a (re)color is pending. the item schema paint kit
	// lookup below used to run for every weapon on every network update even
	// though its result was only ever used inside this branch.
	if( !pConfig->m_set_color || !pConfig->m_change_paint_kit )
		return;

	auto &item = pAttribute->m_Item( );
	auto &definition_index = item.m_iItemDefinitionIndex( );

	static auto sig_address = Memory::Scan( XorStr( "client.dll" ), XorStr( "E8 ? ? ? ? FF 76 0C 8D 48 04 E8" ) );

	// Skip the opcode, read rel32 address
	static auto item_system_offset = *reinterpret_cast< std::int32_t * >( sig_address + 1 );

	// Add the offset to the end of the instruction
	static auto item_system_fn = reinterpret_cast< CCStrike15ItemSystem * ( * )( ) >( sig_address + 5 + item_system_offset );

	// Skip VTable, first member variable of ItemSystem is ItemSchema
	static auto item_schema = reinterpret_cast< CCStrike15ItemSchema * >( std::uintptr_t( item_system_fn( ) ) + sizeof( void * ) );

	// Skip the instructions between, skip the opcode, read rel32 address
	const auto get_paint_kit_definition_offset = *reinterpret_cast< std::int32_t * >( sig_address + 11 + 1 );

	// Add the offset to the end of the instruction
	const auto get_paint_kit_definition_fn = reinterpret_cast< CPaintKit * ( __thiscall * )( CCStrike15ItemSchema *, int ) >( sig_address + 11 + 5 + get_paint_kit_definition_offset );

	auto paintKit = get_paint_kit_definition_fn( item_schema, item.m_nFallbackPaintKit( ) );
	if( paintKit ) {
		// reset to default paint kit color (not work xd (sometime))
		if( pConfig->m_reset_color ) {
			pConfig->color_1.r = g_KitParser.vecPaintKits[ nKit ].defColor[ 0 ].r( ) / 255.f;
			pConfig->color_1.g = g_KitParser.vecPaintKits[ nKit ].defColor[ 0 ].g( ) / 255.f;
			pConfig->color_1.b = g_KitParser.vecPaintKits[ nKit ].defColor[ 0 ].b( ) / 255.f;
			pConfig->color_1.a = g_KitParser.vecPaintKits[ nKit ].defColor[ 0 ].a( ) / 255.f;

			pConfig->color_2.r = g_KitParser.vecPaintKits[ nKit ].defColor[ 1 ].r( ) / 255.f;
			pConfig->color_2.g = g_KitParser.vecPaintKits[ nKit ].defColor[ 1 ].g( ) / 255.f;
			pConfig->color_2.b = g_KitParser.vecPaintKits[ nKit ].defColor[ 1 ].b( ) / 255.f;
			pConfig->color_2.a = g_KitParser.vecPaintKits[ nKit ].defColor[ 1 ].a( ) / 255.f;

			pConfig->color_3.r = g_KitParser.vecPaintKits[ nKit ].defColor[ 2 ].r( ) / 255.f;
			pConfig->color_3.g = g_KitParser.vecPaintKits[ nKit ].defColor[ 2 ].g( ) / 255.f;
			pConfig->color_3.b = g_KitParser.vecPaintKits[ nKit ].defColor[ 2 ].b( ) / 255.f;
			pConfig->color_3.a = g_KitParser.vecPaintKits[ nKit ].defColor[ 2 ].a( ) / 255.f;

			pConfig->color_4.r = g_KitParser.vecPaintKits[ nKit ].defColor[ 3 ].r( ) / 255.f;
			pConfig->color_4.g = g_KitParser.vecPaintKits[ nKit ].defColor[ 3 ].g( ) / 255.f;
			pConfig->color_4.b = g_KitParser.vecPaintKits[ nKit ].defColor[ 3 ].b( ) / 255.f;
			pConfig->color_4.a = g_KitParser.vecPaintKits[ nKit ].defColor[ 3 ].a( ) / 255.f;

			// don't want to reset it again
			pConfig->m_reset_color = false;
		}

		// reset to default 
		if( !pConfig->m_custom_color && !pConfig->m_set_color ) {
			for( int n = 0; n <= 3; ++n ) {
				// no clue how this fails
				paintKit->rgbaColor[ n ] = g_KitParser.vecPaintKits[ nKit ].defColor[ n ];
			}
		}
		// modify our colors
		else {
			paintKit->rgbaColor[ 0 ] = ( pConfig->color_1.ToRegularColor( ) );
			paintKit->rgbaColor[ 1 ] = ( pConfig->color_2.ToRegularColor( ) );
			paintKit->rgbaColor[ 2 ] = ( pConfig->color_3.ToRegularColor( ) );
			paintKit->rgbaColor[ 3 ] = ( pConfig->color_4.ToRegularColor( ) );
		}

		// change paint kit phong
		if( pConfig->m_change_phong ) {
			paintKit->uchPhongExponent = ( unsigned char )std::clamp( pConfig->m_phong_exponent, 0.0f, 255.0f );
			paintKit->uchPhongAlbedoBoost = ( unsigned char )std::clamp( pConfig->m_phong_albedo_boost, 0.0f, 255.0f );
			paintKit->uchPhongIntensity = ( unsigned char )std::clamp( pConfig->m_phong_intensity, 0.0f, 255.0f );
		}

		static auto m_ItemOffset = Engine::g_PropManager.GetOffset( XorStr( "DT_BaseCombatWeapon" ), XorStr( "m_Item" ) );
		auto pItem = reinterpret_cast< void * >( uintptr_t( &item ) + m_ItemOffset );

		static auto CreateCustomWeaponMaterialsFn = reinterpret_cast< void( __thiscall * )( void *item, int nWeaponId, bool bIgnorePicMip, int diffuseTextureSize ) >(
			Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 83 E4 ? 81 EC ? ? ? ? 53 56 57 8B F9 89 7C 24 ? E8" ) ) );

		static auto GetStaticDataFn = reinterpret_cast< void *( __thiscall * )( void *item ) >(
			Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 51 56 57 8B F1 E8 ? ? ? ? 0F B7 8E" ) ) );

		// basically, where skins get updated/initialised it "compares" against already existing skins
		// to see if they already exist, if so then it uses the cached one - and if it's a new entry
		// or there's a difference, then it will add it to the skins list. 
		// https://github.com/perilouswithadollarsign/cstrike15_src/blob/f82112a2388b841d72cb62ca48ab1846dfcc11c8/game/shared/econ/econ_item_view.cpp#L2376-L2385
		// here is what it compares with, so the code below forces the game to think its a new entry
		// by modifying some bullshit number (m_flWeaponLength) by a little amount, and then the game
		// uses our new skin with this bullshit changed number

		auto pStaticData = GetStaticDataFn( reinterpret_cast< void * >( uintptr_t( pAttribute ) + Engine::Displacement.DT_BaseAttributableItem.m_Item ) );
		if( pStaticData ) {
			auto pPaintData = reinterpret_cast< CUtlVector< WeaponPaintableMaterial_t >* >( ( uintptr_t )( pStaticData )+0x19C );
			if( pPaintData ) {
				int nNumMaterialsToPaint = paintKit->bOnlyFirstMaterial ? 1 : pPaintData->Count( );

				// modify this paintkits bullshit number for every entry
				for( int nCustomMaterialIndex = 0; nCustomMaterialIndex < nNumMaterialsToPaint; nCustomMaterialIndex++ ) {
					( *pPaintData )[ nCustomMaterialIndex ].m_flWeaponLength += 0.00001f;
				}
			}
		}

		// call the function responsible for creating/updating paint kits
		//	CreateCustomWeaponMaterialsFn( reinterpret_cast< void * >( uintptr_t( pAttribute ) + Engine::Displacement.DT_BaseAttributableItem.m_Item ), definition_index, false, 9 );

		g_Vars.m_global_skin_changer.m_update_skins = true;

		// we set the color for this paintkit, dont do it again
		pConfig->m_set_color = false;
	}
}

void SkinChanger::ApplyConfigOnAttributableItem( C_BaseAttributableItem *pAttribute, CVariables::skin_changer_data *pConfig, const unsigned xuid_low ) {
	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	if( !pAttribute )
		return;

	if( !pConfig ) {
		EraseOverrideIfExistsByIndex( pAttribute->m_Item( ).m_iItemDefinitionIndex( ) );
		return;
	}

	if( !pConfig->m_enabled ) {
		auto &item = pAttribute->m_Item( );
		auto itOrig = m_originals.find( pAttribute->EntIndex( ) );

		// возвращаем оригинал, только если в полях лежит то, что записали МЫ.
		// запись из кэша больше не удаляем - иначе оружие тут же кэшируется как "новое" и цикл повторяется
		if( itOrig != m_originals.end( ) && itOrig->second.valid && itOrig->second.entity == pAttribute &&
			item.m_iItemDefinitionIndex( ) == itOrig->second.written_definition_index &&
			item.m_nFallbackPaintKit( ) == itOrig->second.written_paint_kit ) {
			auto &orig = itOrig->second;

			item.m_iItemDefinitionIndex( ) = orig.definition_index;
			item.m_nFallbackPaintKit( ) = orig.fallback_paint_kit;
			item.m_nFallbackSeed( ) = orig.fallback_seed;
			item.m_nFallbackStatTrak( ) = orig.fallback_stattrak;
			item.m_flFallbackWear( ) = orig.fallback_wear;
			item.m_iEntityQuality( ) = orig.entity_quality;
			item.m_iItemIDHigh( ) = orig.item_id_high;

			if( orig.model_index != 0 && pAttribute->m_nModelIndex( ) != orig.model_index )
				pAttribute->SetModelIndex( orig.model_index );

			orig.written_definition_index = 0;
			orig.written_paint_kit = INT_MIN;

			// материалы перестроим ОДИН раз
			g_Vars.m_global_skin_changer.m_update_skins = true;
		}

		EraseOverrideIfExistsByIndex( item.m_iItemDefinitionIndex( ) );
		return;
	}

	// Первый раз видим это оружие — запоминаем серверный оригинал ДО перезаписи.
	// На high-ping сервер шлёт DataTable чаще и перезатирает fallback, поэтому
	// оригинал нужен только один раз, а наши значения накатываем каждый кадр.
	CacheOriginalIfNeeded( pAttribute );

	int nKit = pConfig->m_filter_paint_kits ? pConfig->m_paint_kit : pConfig->m_paint_kit_no_filter;

	const int replaceKit = pConfig->m_filter_paint_kits ? nKit : g_KitParser.vecPaintKits[ nKit ].id;

	auto &item = pAttribute->m_Item( );
	auto &global = g_Vars.m_global_skin_changer;

	auto &definition_index = item.m_iItemDefinitionIndex( );

	// Force fallback values to be used.
	item.m_iItemIDHigh( ) = -1;

	// Set the owner of the weapon to our lower XUID. (fixes StatTrak)
	item.m_iAccountID( ) = xuid_low;

	item.m_nFallbackPaintKit( ) = replaceKit;

	//if( !g_Vars.globals.dotest )
	item.m_nFallbackSeed( ) = int( pConfig->m_seed );

	item.m_iEntityQuality( ) = 0;

	if( pConfig->m_stat_trak ) {
		item.m_nFallbackStatTrak( ) = 1337;
		item.m_iEntityQuality( ) = 9;
	}
	else {
		item.m_nFallbackStatTrak( ) = -1;
		item.m_iEntityQuality( ) = 0;
	}

	ModifyPaintkitColor( pAttribute, pConfig );

	item.m_flFallbackWear( ) = pConfig->m_wear > 99.f ? 0.00001f : ( pConfig->m_wear != 0.f ) ? ( 100.f - pConfig->m_wear ) / 100.f : pConfig->m_wear;

	auto &icon_override_map = m_icon_overrides;

	bool knife = IsKnife( definition_index );

	int definition_override = 0;

	if( knife ) {
		if( global.m_knife_changer ) {
			definition_override = global.m_knife_idx;
			item.m_iEntityQuality( ) = 3;
		}
		else {
			// definition_override = ( pLocal->m_iTeamNum( ) == TEAM_CT ) ? WEAPON_KNIFE : WEAPON_KNIFE_T;
		}
	}
	else if( ( pConfig->m_definition_index >= GLOVE_STUDDED_BLOODHOUND && pConfig->m_definition_index <= GLOVE_HYDRA ) || pConfig->m_definition_index == 4725 /*BROKEN FANG GLOVES*/ )
		definition_override = global.m_gloves_idx;

	if( definition_override && ( definition_override != definition_index ) ) {
		// We have info about what we gonna override it to
		if( g_KitParser.vecWeaponInfo.count( definition_override ) > 0 ) {
			const auto replacement_item = &g_KitParser.vecWeaponInfo.at( definition_override );

			const auto old_definition_index = definition_index;

			item.m_iItemDefinitionIndex( ) = definition_override;

			// Set the weapon model index -- required for paint kits to work on replacement items after the 29/11/2016 update.
			// old-build behavior: SetModelIndex, guarded so it only fires on an
			// actual change and never with -1 (unprecached model blanks the gun).
			const auto idx = GetCachedModelIndex( replacement_item->model.c_str( ) );
			if( idx != -1 && pAttribute->m_nModelIndex( ) != idx )
				pAttribute->SetModelIndex( idx );

			global.m_update_skins = true; // один ForceItemUpdate на смену ножа

			// We didn't override 0, but some actual weapon, that we have data for
			if( old_definition_index ) {
				if( g_KitParser.vecWeaponInfo.count( old_definition_index ) > 0 ) {
					const auto original_item = &g_KitParser.vecWeaponInfo.at( old_definition_index );
				if( !original_item->icon.empty( ) && !replacement_item->icon.empty( ) ) {
					// hashing a string_view is strlen + fnv over the whole model
					// path; only pay for it when the mapping actually changes.
					// views point into Item_t-owned strings (map nodes are
					// stable), so the stored mapping never dangles.
					const auto itIcon = icon_override_map.find( std::string_view( original_item->icon ) );
					if( itIcon == icon_override_map.end( ) || itIcon->second != std::string_view( replacement_item->icon ) )
						icon_override_map[ std::string_view( original_item->icon ) ] = std::string_view( replacement_item->icon );
				}
				}
			}
		}
	}
	else {
		EraseOverrideIfExistsByIndex( definition_index );
	}

	// запоминаем, что записали - так кэш и отключение отличают наши значения от серверных
	if( auto itOrig = m_originals.find( pAttribute->EntIndex( ) ); itOrig != m_originals.end( ) ) {
		itOrig->second.written_definition_index = item.m_iItemDefinitionIndex( );
		itOrig->second.written_paint_kit = item.m_nFallbackPaintKit( );
	}

	pConfig->m_executed = false;
}

// C_CSPlayer::m_pViewmodelArmConfig: если NULL, UpdateAllViewmodelAddons
// сносит старые руки и собирает их заново с текущей перчаткой.
// ОТКЛЮЧЕНО: грубый скан 0x3000..0xA000 читает за пределами объекта игрока,
// а strstr() по непроверенному указателю падает на невыровненной памяти.
// Это и был краш. Оставлен no-op, чтобы не трогать точку вызова.
static void ResetViewmodelArms( C_CSPlayer *pLocal ) {
	( void )pLocal;
	return;
}

void SkinChanger::GloveChanger( C_CSPlayer *pLocal ) {
	if( !pLocal )
		return;

	const auto iLocalIndex = pLocal->EntIndex( );

	player_info_t player_info;
	if( !g_pEngine->GetPlayerInfo( iLocalIndex, &player_info ) )
		return;

	// m_nBody выбирает вариант рук под перчатки. Сервер и респавн сбрасывают его
	// в 0, а раньше он ставился один раз при создании — после спавна руки ломались
	// (дефолтные рукава + перчатки, «лишние руки»). Поэтому форсим каждый кадр.
	static int nBodyOffset = 0;
	if( !nBodyOffset ) {
		if( const auto pMap = pLocal->GetPredDescMap( ) )
			nBodyOffset = ( int )Memory::FindInDataMap( pMap, XorStr( "m_nBody" ) );
	}
	auto fnSetBody = [ & ] ( int v ) {
		if( nBodyOffset )
			*( int * )( ( uintptr_t )pLocal + nBodyOffset ) = v;
	};

	auto &global = g_Vars.m_global_skin_changer;
	const auto glove_config = GetDataFromIndex( global.m_gloves_idx );

	const auto hWearables = pLocal->m_hMyWearables( );
	if( !hWearables )
		return;

	// Чейнджер выключен: вернуть обычные руки, иначе ломанные руки навсегда.
	static bool s_bBodySet = false;
	if( !global.m_active || !global.m_glove_changer || !glove_config || global.m_gloves_idx == 0 ) {
		if( s_bBodySet ) { fnSetBody( 0 ); s_bBodySet = false; }
		return;
	}
	s_bBodySet = true;

	// Наша ли это перчатка: сущность должна существовать, быть CEconWearable и
	// нести наш definition_index.
	auto fnIsOurGlove = [ & ] ( C_BaseAttributableItem *pEnt ) {
		if( !pEnt )
			return false;
		if( auto *cc = pEnt->GetClientClass( ) ) {
			if( cc->m_ClassID != CEconWearable )
				return false;
		}
		else {
			return false;
		}
		return pEnt->m_Item( ).m_iItemDefinitionIndex( ) == ( int )glove_config->m_definition_index;
	};

	if( hWearables ) {
		auto pWearable = ( C_BaseAttributableItem * )hWearables[ 0 ].Get( );
		// Слот может указывать на чужую/протухшую сущность (индекс переиспользован
		// после спавна или смены карты): такую перчаткой не считаем.
		if( pWearable && !fnIsOurGlove( pWearable ) )
			pWearable = nullptr;

		if( !pWearable ) {
			// Возвращаем свою прошлую перчатку БЕЗ создания дубликата.
			// Раньше каждый спавн плодил новую wearable, а старые оставались висеть
			// на игроке — отсюда «лишние руки», которые не пропадают (за T видно сразу).
			if( auto pTheGlove = hGloveHandle.Get( ); pTheGlove && hGloveHandle.IsValid( ) && fnIsOurGlove( pTheGlove ) ) {
				hWearables[ 0 ] = hGloveHandle;
				pWearable = pTheGlove;
			}
			else if( pTheGlove ) {
				// Протухший хэндл — прибить, чтобы не цеплял мусор.
				if( auto *net = pTheGlove->GetClientNetworkable( ) ) {
					net->SetDestroyedOnRecreateEntities( );
					net->Release( );
				}
				hGloveHandle.Set( nullptr );
			}
		}

		if( !pWearable ) {
			// reference (skins.cpp): fresh index past the highest entity plus
			// a random serial. do NOT steal the slot of a live entity (the old
			// code reused the first CPlasma slot, stomping a real effect).
			const int iEntry = g_pEntityList->GetHighestEntityIndex( ) + 1;

			for( ClientClass *pClass = g_pClient->GetAllClasses( ); pClass; pClass = pClass->m_pNext ) {
				if( pClass->m_ClassID != CEconWearable )
					continue;

				// reference uses a random serial in [0xA00, 0xFFF]; a fixed
				// 4095 can collide with a real entity handle.
				const int iSerial = RandomInt( 0xA00, 0xFFF );

				reinterpret_cast< CreateClientClassFn >( pClass->m_pCreateFn )( iEntry, iSerial );
				hWearables[ 0 ] = iEntry | iSerial << 16;

				break;
			}

			pWearable = ( C_BaseAttributableItem * )g_pEntityList->GetClientEntity( iEntry );
			if( pWearable ) {
				static auto fnEquip
					= reinterpret_cast< int( __thiscall * )( void *, void * ) >(
						Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 83 EC 10 53 8B 5D 08 57 8B F9" ) )
						);

				static auto fnInitializeAttributes
					= reinterpret_cast< int( __thiscall * )( void * ) >(
						Memory::Scan( XorStr( "client.dll" ), XorStr( "55 8B EC 83 E4 F8 83 EC 0C 53 56 8B F1 8B 86" ) )
						);

				//glove_config->m_executed = true;

				auto config = glove_config;

				// note; this was an unguarded std::map::at( ). a glove definition with no
				// vecWeaponInfo entry threw std::out_of_range from inside a hooked engine
				// function with no handler on the stack, which terminates the process.
				const auto itReplacement = g_KitParser.vecWeaponInfo.find( ( int )glove_config->m_definition_index );
				if( itReplacement == g_KitParser.vecWeaponInfo.end( ) )
					return;

				const auto replacement_item = &itReplacement->second;

				pWearable->m_Item( ).m_iItemIDHigh( ) = -1;
				pWearable->m_Item( ).m_iItemDefinitionIndex( ) = ( int )glove_config->m_definition_index;
				pWearable->m_Item( ).m_nFallbackPaintKit( ) = config->m_filter_paint_kits ? config->m_paint_kit : g_KitParser.vecPaintKits[ config->m_paint_kit_no_filter ].id;
				pWearable->m_Item( ).m_iEntityQuality( ) = 4;
				pWearable->m_Item( ).m_iAccountID( ) = player_info.xuid_low;;
				pWearable->m_Item( ).m_bInitialized( ) = true;
				pWearable->m_Item( ).m_nFallbackSeed( ) = glove_config->m_seed;

				// old-build behavior: the wearable takes the world model, like the
				// stock glove changer this was ported from. never write -1
				// (unprecached model would blank the arms).
				const int glove_model_index = GetCachedModelIndex( replacement_item->world_model.c_str( ) );
				if( glove_model_index == -1 )
					return;

				pWearable->SetModelIndex( glove_model_index );

				fnEquip( pWearable, pLocal );

				fnSetBody( 1 );

				fnInitializeAttributes( pWearable );

				//printf( "pre: %i\n", pWearable->GetClientRenderable( )->RenderHandle( ) );
				g_pClientLeafSystem->_CreateRenderableHandle( pWearable );

				if( pWearable->GetClientRenderable( ) )
					hGloveRenderHandle = pWearable->GetClientRenderable( )->RenderHandle( );

				auto networkable = pWearable->GetClientNetworkable( );
				if( networkable ) {
					networkable->PreDataUpdate( 0 );
				}

				// Запомнить созданное, иначе следующий спавн сделает дубликат.
				hGloveHandle.Set( pWearable );
				ResetViewmodelArms( pLocal );
			}
		}
		else {
			// Перчатка на месте: защитить fallback от сетевой перезаписи (high-ping),
			// как у оружия, + держать модель и m_nBody каждый кадр.
			pWearable->m_Item( ).m_iItemIDHigh( ) = -1;
			pWearable->m_Item( ).m_iAccountID( ) = player_info.xuid_low;
			pWearable->m_Item( ).m_nFallbackPaintKit( ) = glove_config->m_filter_paint_kits ? glove_config->m_paint_kit : g_KitParser.vecPaintKits[ glove_config->m_paint_kit_no_filter ].id;
			pWearable->m_Item( ).m_nFallbackSeed( ) = glove_config->m_seed;
			pWearable->m_Item( ).m_iEntityQuality( ) = 4;

			if( const auto itRep = g_KitParser.vecWeaponInfo.find( ( int )glove_config->m_definition_index ); itRep != g_KitParser.vecWeaponInfo.end( ) ) {
				const int gi = GetCachedModelIndex( itRep->second.world_model.c_str( ) );
				if( gi != -1 && pWearable->m_nModelIndex( ) != gi )
					pWearable->SetModelIndex( gi );
			}

			fnSetBody( 1 );

			if( auto r = pWearable->GetClientRenderable( ); r && r->RenderHandle( ) == 0xffff ) {
				g_pClientLeafSystem->_CreateRenderableHandle( pWearable );
				hGloveRenderHandle = r->RenderHandle( );
			}

			hGloveHandle.Set( pWearable );
		}
	}
}

void SkinChanger::SequenceProxyFn( const CRecvProxyData *proxy_data_const, void *entity, void *output ) {
	if( g_SkinChanger.m_sequence_hook ) {
		static auto original_fn = g_SkinChanger.m_sequence_hook->GetOriginalFunction( );

		// Remove the constness from the proxy data allowing us to make changes.
		const auto proxy_data = const_cast< CRecvProxyData * >( proxy_data_const );

		const auto view_model = static_cast< C_BaseViewModel * >( entity );

		g_SkinChanger.DoSequenceRemapping( proxy_data, view_model );

		// Call the original function with our edited data.
		original_fn( proxy_data_const, entity, output );
	}
}

void SkinChanger::DoSequenceRemapping( CRecvProxyData *data, C_BaseViewModel *entity ) {
	const auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal || pLocal->IsDead( ) )
		return;

	const auto pOwner = entity->m_hOwner( ).Get( );
	if( pOwner != pLocal )
		return;

	const auto pViewModelWeapon = entity->m_hWeapon( ).Get( );
	if( !pViewModelWeapon )
		return;

	auto idx = pViewModelWeapon->m_Item( ).m_iItemDefinitionIndex( );
	const auto itWeaponInfo = g_KitParser.vecWeaponInfo.find( idx );
	if( itWeaponInfo == g_KitParser.vecWeaponInfo.end( ) )
		return;

	if( !g_Vars.m_global_skin_changer.m_knife_changer )
		return;

	const auto weapon_info = &itWeaponInfo->second;

	if( weapon_info ) {
		const auto &override_model = weapon_info->model;

		auto &sequence = data->m_Value.m_Int;

		// note; hash_32_fnv1a_const( ) is a constexpr *recursive* function. called on a
		// runtime pointer like this one it degrades into one call per character of the
		// model path, every time the netvar is decoded. the iterative version is the
		// same hash, computed in a loop.
		sequence = GetNewAnimation( hash_32_fnv1a( override_model.c_str( ) ), sequence, entity );
	}
}

int SkinChanger::GetNewAnimation( const uint32_t model, const int sequence, C_BaseViewModel *viewModel ) {

	// This only fixes if the original knife was a default knife.
	// The best would be having a function that converts original knife's sequence
	// into some generic enum, then another function that generates a sequence
	// from the sequences of the new knife. I won't write that.
	enum ESequence {
		SEQUENCE_DEFAULT_DRAW = 0,
		SEQUENCE_DEFAULT_IDLE1 = 1,
		SEQUENCE_DEFAULT_IDLE2 = 2,
		SEQUENCE_DEFAULT_LIGHT_MISS1 = 3,
		SEQUENCE_DEFAULT_LIGHT_MISS2 = 4,
		SEQUENCE_DEFAULT_HEAVY_MISS1 = 9,
		SEQUENCE_DEFAULT_HEAVY_HIT1 = 10,
		SEQUENCE_DEFAULT_HEAVY_BACKSTAB = 11,
		SEQUENCE_DEFAULT_LOOKAT01 = 12,

		SEQUENCE_BUTTERFLY_DRAW = 0,
		SEQUENCE_BUTTERFLY_DRAW2 = 1,
		SEQUENCE_BUTTERFLY_LOOKAT01 = 13,
		SEQUENCE_BUTTERFLY_LOOKAT03 = 15,

		SEQUENCE_FALCHION_IDLE1 = 1,
		SEQUENCE_FALCHION_HEAVY_MISS1 = 8,
		SEQUENCE_FALCHION_HEAVY_MISS1_NOFLIP = 9,
		SEQUENCE_FALCHION_LOOKAT01 = 12,
		SEQUENCE_FALCHION_LOOKAT02 = 13,

		SEQUENCE_DAGGERS_IDLE1 = 1,
		SEQUENCE_DAGGERS_LIGHT_MISS1 = 2,
		SEQUENCE_DAGGERS_LIGHT_MISS5 = 6,
		SEQUENCE_DAGGERS_HEAVY_MISS2 = 11,
		SEQUENCE_DAGGERS_HEAVY_MISS1 = 12,

		SEQUENCE_BOWIE_IDLE1 = 1,
	};

	// pick a variant deterministically from ( model, sequence ).
	//
	// this used to be `rand( ) % ( high - low + 1 ) + low`. that is called from a recv
	// proxy, so whenever the proxy ran twice for the same logical server value - which
	// happens on every full update, and this cheat forces those - the viewmodel got a
	// *different* sequence back. the engine then saw a sequence change and restarted
	// the animation with m_flCycle = 0. that is the knife animation stutter.
	//
	// same value in, same value out: the animation is stable across re-decodes, and
	// different draws still vary because `sequence` differs.
	auto random_sequence = [ & ] ( const int low, const int high ) -> int {
		if( high <= low )
			return low;

		std::uint32_t uSeed = model ^ ( std::uint32_t( sequence ) * 2654435761u );

		// finalizer so adjacent sequence numbers do not map to adjacent variants.
		uSeed ^= uSeed >> 16;
		uSeed *= 2246822507u;
		uSeed ^= uSeed >> 13;

		return low + int( uSeed % std::uint32_t( high - low + 1 ) );
	};

	// Hashes for best performance.
	switch( model ) {
		case hash_32_fnv1a_const( ( "models/weapons/v_knife_butterfly.mdl" ) ):
		{
			switch( sequence ) {
				case SEQUENCE_DEFAULT_DRAW:
					return random_sequence( SEQUENCE_BUTTERFLY_DRAW, SEQUENCE_BUTTERFLY_DRAW2 );
				case SEQUENCE_DEFAULT_LOOKAT01:
					return random_sequence( SEQUENCE_BUTTERFLY_LOOKAT01, SEQUENCE_BUTTERFLY_LOOKAT03 );
				default:
					return sequence + 1;
			}
		}
		case hash_32_fnv1a_const( ( "models/weapons/v_knife_falchion_advanced.mdl" ) ):
		{
			switch( sequence ) {
				case SEQUENCE_DEFAULT_IDLE2:
					return SEQUENCE_FALCHION_IDLE1;
				case SEQUENCE_DEFAULT_HEAVY_MISS1:
					return random_sequence( SEQUENCE_FALCHION_HEAVY_MISS1, SEQUENCE_FALCHION_HEAVY_MISS1_NOFLIP );
				case SEQUENCE_DEFAULT_LOOKAT01:
					return random_sequence( SEQUENCE_FALCHION_LOOKAT01, SEQUENCE_FALCHION_LOOKAT02 );
				case SEQUENCE_DEFAULT_DRAW:
				case SEQUENCE_DEFAULT_IDLE1:
					return sequence;
				default:
					return sequence - 1;
			}
		}
		case hash_32_fnv1a_const( ( "models/weapons/v_knife_push.mdl" ) ):
		{
			switch( sequence ) {
				case SEQUENCE_DEFAULT_IDLE2:
					return SEQUENCE_DAGGERS_IDLE1;
				case SEQUENCE_DEFAULT_LIGHT_MISS1:
				case SEQUENCE_DEFAULT_LIGHT_MISS2:
					return random_sequence( SEQUENCE_DAGGERS_LIGHT_MISS1, SEQUENCE_DAGGERS_LIGHT_MISS5 );
				case SEQUENCE_DEFAULT_HEAVY_MISS1:
					return random_sequence( SEQUENCE_DAGGERS_HEAVY_MISS2, SEQUENCE_DAGGERS_HEAVY_MISS1 );
				case SEQUENCE_DEFAULT_HEAVY_HIT1:
				case SEQUENCE_DEFAULT_HEAVY_BACKSTAB:
				case SEQUENCE_DEFAULT_LOOKAT01:
					return sequence + 3;
				case SEQUENCE_DEFAULT_DRAW:
				case SEQUENCE_DEFAULT_IDLE1:
					return sequence;
				default:
					return sequence + 2;
			}
		}
		case hash_32_fnv1a_const( ( "models/weapons/v_knife_survival_bowie.mdl" ) ):
		{
			switch( sequence ) {
				case SEQUENCE_DEFAULT_DRAW:
				case SEQUENCE_DEFAULT_IDLE1:
					return sequence;
				case SEQUENCE_DEFAULT_IDLE2:
					return SEQUENCE_BOWIE_IDLE1;
				default:
					return sequence - 1;
			}
		}
		case hash_32_fnv1a_const( ( "models/weapons/v_knife_ursus.mdl" ) ):
		case hash_32_fnv1a_const( ( "models/weapons/v_knife_skeleton.mdl" ) ):
		case hash_32_fnv1a_const( ( "models/weapons/v_knife_outdoor.mdl" ) ):
		case hash_32_fnv1a_const( ( "models/weapons/v_knife_canis.mdl" ) ):
		case hash_32_fnv1a_const( ( "models/weapons/v_knife_cord.mdl" ) ):
		case hash_32_fnv1a_const( ( "models/weapons/v_knife_navaja.mdl" ) ):
		case hash_32_fnv1a_const( ( "models/weapons/v_knife_classic.mdl" ) ):
		case hash_32_fnv1a_const( ( "models/weapons/v_knife_css.mdl" ) ):
		case hash_32_fnv1a_const( ( "models/weapons/v_knife_kukri.mdl" ) ):
		{
			switch( sequence ) {
				case SEQUENCE_DEFAULT_DRAW:
					return random_sequence( SEQUENCE_BUTTERFLY_DRAW, SEQUENCE_BUTTERFLY_DRAW2 );
				case SEQUENCE_DEFAULT_LOOKAT01:
					return random_sequence( SEQUENCE_BUTTERFLY_LOOKAT01, 14 );
				default:
					return sequence + 1;
			}
		}
		case hash_32_fnv1a_const( ( "models/weapons/v_knife_stiletto.mdl" ) ):
		{
			switch( sequence ) {
				case SEQUENCE_DEFAULT_LOOKAT01:
					return random_sequence( 12, 13 );
				default:
					return sequence;
			}
		}
		case hash_32_fnv1a_const( ( "models/weapons/v_knife_widowmaker.mdl" ) ):
		{
			switch( sequence ) {
				case SEQUENCE_DEFAULT_LOOKAT01:
					return random_sequence( 14, 15 );
				default:
					return sequence;
			}
		}

		default:
			return sequence;
	}

	return sequence;
}

CVariables::skin_changer_data *SkinChanger::GetDataFromIndex( int idx ) {
	// entries are created once at startup, one per weapon/glove definition, so
	// the definition index -> config mapping never changes afterwards. this was
	// a linear scan over ~50 entries with a pointer chase per element, called
	// ~50 times per network update.
	static std::unordered_map< int, CVariables::skin_changer_data * > map;
	if( map.empty( ) ) {
		auto &skin_data = g_Vars.m_skin_changer;
		for( size_t i = 0u; i < skin_data.Size( ); ++i ) {
			auto skin = skin_data[ i ];
			map[ skin->m_definition_index ] = skin;
		}
	}

	const auto it = map.find( idx );
	return it != map.end( ) ? it->second : nullptr;
}

void SkinChanger::ForceItemUpdate( C_CSPlayer *pLocal ) {
	if( !pLocal || pLocal->IsDead( ) )
		return;

	auto ForceUpdate = [ ] ( C_BaseCombatWeapon *pItem ) {
		C_EconItemView *view = &pItem->m_Item( );

		if( !view )
			return;

		if( !pItem->GetClientNetworkable( ) )
			return;

		auto clearRefCountedVector = [ ] ( CUtlVector< IRefCounted * > &vec ) {
			for( int i = 0; i < vec.m_Size; ++i ) {
				if( &vec.m_Memory ) {
					if( vec.m_Memory.m_pMemory ) {
						auto &element = vec.m_Memory.m_pMemory[ i ];
						if( element ) {
							element->unreference( );
							element = nullptr;
						}
					}
				}
			}
			vec.m_Size = 0;
		};

		// только отпускаем указатели, внутрь объекта НЕ пишем: один материал может
		// использоваться сразу несколькими оружиями
		auto dropCustomMaterials = [ ] ( CUtlVector< IRefCounted * > &vec ) {
			if( vec.m_Memory.m_pMemory ) {
				for( int i = 0; i < vec.m_Size; ++i )
					vec.m_Memory.m_pMemory[ i ] = nullptr;
			}
			vec.m_Size = 0;
		};

		pItem->m_bCustomMaterialInitialized( ) = false;
		dropCustomMaterials( pItem->m_CustomMaterials( ) );
		dropCustomMaterials( view->m_CustomMaterials( ) );
		clearRefCountedVector( view->m_VisualsDataProcessors( ) );

		const auto pNetworkable = pItem->GetClientNetworkable( );
		if( pNetworkable ) {
			pNetworkable->PostDataUpdate( 0 );
			pNetworkable->OnDataChanged( 0 );
		}
	};
	auto &global = g_Vars.m_global_skin_changer;
	auto weapons = pLocal->m_hMyWeapons( );
	if( !weapons )
		return;
	for( size_t i = 0; i < 48; ++i ) {
		auto weaponHandle = weapons[ i ];
		// HIGH-PING: в инвентаре бывают дырки (nullptr между валидными
		// хэндлами после дропа/смерти). break пропускал бы хвост — нужен
		// continue, чтобы все 48 слотов всегда обновлялись.
		if( !weaponHandle.IsValid( ) )
			continue;

		auto pWeapon = ( C_WeaponCSBaseGun * )weaponHandle.Get( );
		if( !pWeapon )
			continue;

		//if( can_force_item_update( ( C_BaseCombatWeapon * )pWeapon ) )
		ForceUpdate( pWeapon );
	}

	UpdateHud( );
}

void SkinChanger::UpdateHud( ) {

}