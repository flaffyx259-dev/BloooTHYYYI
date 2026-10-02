#pragma once
#include <vector>
#include <map>

#include "../../SDK/variables.hpp"
#include "../../SDK/color.hpp"

template < typename Key, typename Value >
struct Node_t {
	int previous_id;    //0x0000
	int next_id;        //0x0004
	void *_unknown_ptr; //0x0008
	int _unknown;       //0x000C
	Key key;            //0x0010
	Value value;        //0x0014
};

template < typename Key, typename Value >
struct Head_t {
	Node_t< Key, Value > *memory; //0x0000
	int allocation_count;         //0x0004
	int grow_size;                //0x0008
	int start_element;            //0x000C
	int next_available;           //0x0010
	int _unknown;                 //0x0014
	int last_element;             //0x0018
};                              //Size=0x001C

struct paint_kit {
	int id;
	std::string name;
	std::string_view item_name;
	Color defColor[4];
	int default_style;
	float def_length;

	auto operator<( const paint_kit& other ) const -> bool {
		return name < other.name;
	}

	auto operator==( int _id ) const -> bool {
		return id == _id;
	}
};

enum WeaponGroup_t {
	WEAPONGROUP_PISTOL,
	WEAPONGROUP_HEAVYPISTOL,
	WEAPONGROUP_RIFLE,
	WEAPONGROUP_SNIPER,
	WEAPONGROUP_AUTOSNIPER,
	WEAPONGROUP_SUBMACHINE,
	WEAPONGROUP_HEAVY,
	WEAPONGROUP_SHOTGUN,
	WEAPONGROUP_MAX
};

class KitParser {
	enum LoadoutSlot {
		LOADOUT_POSITION_MELEE,
		LOADOUT_POSITION_C4,
		LOADOUT_POSITION_SECONDARY0,
		LOADOUT_POSITION_SECONDARY1,
		LOADOUT_POSITION_SECONDARY2,
		LOADOUT_POSITION_SECONDARY3,
		LOADOUT_POSITION_SECONDARY4,
		LOADOUT_POSITION_SECONDARY5,
		LOADOUT_POSITION_SMG0,
		LOADOUT_POSITION_SMG1,
		LOADOUT_POSITION_SMG2,
		LOADOUT_POSITION_SMG3,
		LOADOUT_POSITION_SMG4,
		LOADOUT_POSITION_SMG5,
		LOADOUT_POSITION_RIFLE0,
		LOADOUT_POSITION_RIFLE1,
		LOADOUT_POSITION_RIFLE2,
		LOADOUT_POSITION_RIFLE3,
		LOADOUT_POSITION_RIFLE4,
		LOADOUT_POSITION_RIFLE5,
		LOADOUT_POSITION_HEAVY0,
		LOADOUT_POSITION_HEAVY1,
		LOADOUT_POSITION_HEAVY2,
		LOADOUT_POSITION_HEAVY3,
		LOADOUT_POSITION_HEAVY4,
		LOADOUT_POSITION_HEAVY5,
		LOADOUT_POSITION_GRENADE0,
		LOADOUT_POSITION_GRENADE1,
		LOADOUT_POSITION_GRENADE2,
		LOADOUT_POSITION_GRENADE3,
		LOADOUT_POSITION_GRENADE4,
		LOADOUT_POSITION_GRENADE5,
		LOADOUT_POSITION_EQUIPMENT0,
		LOADOUT_POSITION_EQUIPMENT1,
		LOADOUT_POSITION_EQUIPMENT2,
		LOADOUT_POSITION_EQUIPMENT3,
		LOADOUT_POSITION_EQUIPMENT4,
		LOADOUT_POSITION_EQUIPMENT5,
		LOADOUT_POSITION_SPACER1,
		LOADOUT_POSITION_SPACER2,
		LOADOUT_POSITION_SPACER3,
		LOADOUT_POSITION_SPACER4,
		LOADOUT_POSITION_SPACER5,
		LOADOUT_POSITION_SPACER6,
		LOADOUT_POSITION_MISC0,
		LOADOUT_POSITION_MISC1,
		LOADOUT_POSITION_MISC2,
		LOADOUT_POSITION_MISC3,
		LOADOUT_POSITION_MISC4,
		LOADOUT_POSITION_MISC5,
		LOADOUT_POSITION_MISC6,
		LOADOUT_POSITION_MISC7,
		LOADOUT_POSITION_MISC8,
		LOADOUT_POSITION_MISC9,
		LOADOUT_POSITION_SPACER7,
		LOADOUT_POSITION_FLAIR0,
	};

	struct item_cstrike {
		bool glove = false;
		bool knife = false;
		int group;
		std::string_view name;
		std::string display_name;
		std::string model_name;
		std::string world_model;
	};

	// Owns its strings. vecWeaponInfo entries used to hold raw c_str() views
	// into item_skins strings living in vecWeapons - every vector realloc
	// (inserts/sorts during init) left them dangling, so GetModelIndex got
	// garbage, returned -1 and the model override was skipped (HUD showed the
	// new knife while hands/world kept the default one). std::map nodes never
	// move, so views into THESE strings stay valid forever.
	struct Item_t {
		Item_t( ) { }

		Item_t( const char* model, const char* world_model, const char* icon = nullptr ) :
			model( model ? model : "" ),
			world_model( world_model ? world_model : "" ),
			icon( icon ? icon : "" ) {
		}

		std::string model;
		std::string world_model;
		std::string icon;
	};

	struct item_skins {
		int id;
		int group;
		std::vector< paint_kit > m_kits;
		std::string_view name;
		std::string display_name;
		std::string model_name;
		std::string world_model;
		bool glove;
		bool knife;

		item_skins( ) {
			paint_kit default_kit;
			default_kit.id = 0;
			default_kit.item_name = XorStr( "#PaintKit_Default" );
			default_kit.name = XorStr( "-" );
			m_kits.push_back( default_kit );
		}

		auto
			operator==( int _id ) const -> bool {
			return id == _id;
		}
	};

public:
	std::vector< paint_kit > all_skins;
	std::vector< paint_kit > vecPaintKits;
	std::vector< paint_kit > vecGlovePaintKits;
	std::vector< item_skins > vecWeapons;
	std::vector< WeaponName_t > vecKnifeNames;
	std::vector< WeaponName_t > vecGloveNames;
	std::map< size_t, Item_t > vecWeaponInfo;
	void Initialize( );
};

extern KitParser g_KitParser;
