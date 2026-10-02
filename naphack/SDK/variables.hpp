#pragma once
#include "../Utils/extern/FnvHash.hpp"
#include <any>
#include <variant>
#include <map>
#include <d3d9.h>
#include "../Libraries/json.h"
#include "Valve/vector.hpp"
#include "Valve/qangle.hpp"
#include "Classes/CStudioRender.hpp" 
#include <array>
#define USE_XOR

#include "../Utils/extern/XorStr.hpp"

class CUserCmd;

namespace KeyBindType {
	enum {
		HOLD = 0,
		TOGGLE,
		ALWAYS_ON
	};
};

struct hotkey_t {
	hotkey_t( ) { }

	int key = 0, cond = 0;
	bool enabled = false;

	void to_json( nlohmann::json &j ) {
		j = nlohmann::json{
			{ ( XorStr( "key" ) ), key },
			{ ( XorStr( "cond" ) ), cond },
		};
	}

	void from_json( nlohmann::json &j ) {
		j.at( XorStr( "key" ) ).get_to( key );
		j.at( XorStr( "cond" ) ).get_to( cond );
	}
};

extern std::vector<hotkey_t *> g_keybinds;
inline std::vector < std::pair<hotkey_t *, std::string> > g_KeybindNames;

class Color;
class Color_f {
public:
	Color_f( ) = default;
	Color_f( float _r, float _g, float _b, float _a = 1.0f ) :
		r( _r ), g( _g ), b( _b ), a( _a ) {
	}

	Color_f( int _r, int _g, int _b, int _a = 255 ) { SetColor( _r, _g, _b, _a ); }

	Color_f Lerp( const Color_f &c, float t ) const;

	void SetColor( float _r, float _g, float _b, float _a = 1.0f ) {
		r = _r;
		g = _g;
		b = _b;
		a = _a;
	}

	void SetColor( Color clr );

	void SetColor( int _r, int _g, int _b, int _a = 255 ) {
		r = static_cast< float >( _r ) / 255.0f;
		g = static_cast< float >( _g ) / 255.0f;
		b = static_cast< float >( _b ) / 255.0f;
		a = static_cast< float >( _a ) / 255.0f;
	}

	Color_f Alpha( float alpha ) {
		return Color_f( r, g, b, alpha );
	}

	uint32_t Hex( ) const {
		union {
			uint32_t i;
			struct {
				uint8_t bytes[ 4 ];
			};
		} conv;

		conv.bytes[ 0 ] = static_cast< int >( r * 255.0f );
		conv.bytes[ 1 ] = static_cast< int >( g * 255.0f );
		conv.bytes[ 2 ] = static_cast< int >( b * 255.0f );
		conv.bytes[ 3 ] = static_cast< int >( a * 255.0f );

		return conv.i;
	};

	void Color_f::to_json( nlohmann::json &j ) {
		j = nlohmann::json{
			{ XorStr( "r" ), r },
			{ XorStr( "g" ), g },
			{ XorStr( "b" ), b },
			{ XorStr( "a" ), a }
		};
	}

	void Color_f::from_json( nlohmann::json &j ) {
		j.at( XorStr( "r" ) ).get_to( r );
		j.at( XorStr( "g" ) ).get_to( g );
		j.at( XorStr( "b" ) ).get_to( b );
		j.at( XorStr( "a" ) ).get_to( a );
	}

	bool operator==( const Color_f &clr ) const {
		return clr.r == r && clr.g == g && clr.b == b && clr.a == a;
	};

	bool operator!=( const Color_f &clr ) const {
		return clr.r != r || clr.g != g || clr.b != b || clr.a != a;
	};

	Color_f operator*( float v ) const {
		return Color_f( r * v, g * v, b * v, a );
	}

	operator uint32_t( ) const { return Hex( ); };

	operator float *( ) { return &r; };

	float r, g, b, a;

	static Color_f Black;
	static Color_f White;
	static Color_f Gray;

	Color ToRegularColor( );
};

#pragma region Config System

class CBaseGroup {
public: // ghetto fix, for skin changer options setup
	std::map< uint32_t, std::unique_ptr< std::any > > m_options;
	uint32_t m_name;
	nlohmann::json m_json;
	nlohmann::json m_json_default_cfg;
	std::vector< CBaseGroup * > m_children;

	using AllowedTypes = std::variant< int, bool, float, std::string, Color_f >;
	template < typename T >
	using IsTypeAllowed = std::enable_if_t< std::is_constructible_v< AllowedTypes, T >, T * >;

public:
	CBaseGroup( ) = default;
	CBaseGroup( std::string name, CBaseGroup *parent = nullptr ) {
		m_name = hash_32_fnv1a( name.c_str( ) );
		if( parent )
			parent->AddChild( this );
	}

	CBaseGroup( size_t idx, CBaseGroup *parent = nullptr ) {
		m_name = hash_32_fnv1a( std::string( XorStr( "( " ) ).append( std::to_string( idx ) ).append( XorStr( " )" ) ).data( ) );
		if( parent )
			parent->AddChild( this );
	};

protected:
	template < typename T, class... Types >
	auto AddOption( const char *name, Types&& ... args ) -> T * {
		auto pair = m_options.try_emplace( hash_32_fnv1a( name ), std::make_unique< std::any >( std::make_any< T >( std::forward< Types >( args )... ) ) );

		if( typeid( T ).hash_code( ) == typeid( hotkey_t ).hash_code( ) ) {
			auto hotkey = reinterpret_cast< hotkey_t * >( pair.first->second.get( ) );
			if( hotkey ) {
				g_keybinds.push_back( hotkey );
			}
		}

		return reinterpret_cast< T * >( pair.first->second.get( ) );
	};

	auto AddChild( CBaseGroup *group ) -> void { m_children.push_back( group ); };
public:
	template < typename T, class... Types >
	auto Add( const char *name, Types&& ... args ) -> T * {
		auto pair = m_options.try_emplace( hash_32_fnv1a( name ), std::make_unique< std::any >( std::make_any< T >( std::forward< Types >( args )... ) ) );

		if( typeid( T ).hash_code( ) == typeid( hotkey_t ).hash_code( ) ) {
			auto hotkey = reinterpret_cast< hotkey_t * >( pair.first->second.get( ) );
			if( hotkey ) {
				//strcpy( hotkey->name, name );

				g_KeybindNames.emplace_back( hotkey, name );

				g_keybinds.push_back( hotkey );
			}
		}

		return reinterpret_cast< T * >( pair.first->second.get( ) );
	}

	auto GetName( ) const -> uint32_t { return m_name; }

	auto GetJson( ) -> nlohmann::json & { return m_json; }

	auto SetName( const std::string_view &name ) -> void { m_name = hash_32_fnv1a( name.data( ) ); }

	auto Save( ) -> void {
		m_json.clear( );
		for( auto &[name_hash, opt] : m_options ) {
			std::string name = std::to_string( name_hash );

			// TODO: option class with virtual save function (is it good idea?)
			auto any = *opt.get( );
			auto hash = any.type( ).hash_code( );

			// find out, could iterate AllowedTypes
			if( typeid( int ).hash_code( ) == hash )
				m_json[ name ] = std::any_cast< int >( any );
			else if( typeid( bool ).hash_code( ) == hash )
				m_json[ name ] = std::any_cast< bool >( any );
			else if( typeid( float ).hash_code( ) == hash )
				m_json[ name ] = std::any_cast< float >( any );
			else if( typeid( std::string ).hash_code( ) == hash )
				m_json[ name ] = std::any_cast< std::string >( any );
			else if( typeid( Color_f ).hash_code( ) == hash )
				std::any_cast< Color_f >( any ).to_json( m_json[ name ] );
			else if( typeid( hotkey_t ).hash_code( ) == hash )
				std::any_cast< hotkey_t >( any ).to_json( m_json[ name ] );
		}

		for( auto &child : m_children ) {
			child->Save( );

			auto json = child->GetJson( );
			m_json[ std::to_string( child->GetName( ) ) ] = json;
		}
	}

	auto Load( nlohmann::json &js ) -> void {
		m_json.clear( );
		m_json = js;
		for( auto &[name_hash, opt] : m_options ) {
			std::string name = std::to_string( name_hash );

			// TODO: option class with virtual load function (is it good idea?)
			std::any &any = *opt.get( );
			auto hash = any.type( ).hash_code( );

			if( m_json.count( name ) <= 0 )
				continue;

			try {
				// find out, can iterate AllowedType? 
				if( typeid( int ).hash_code( ) == hash )
					std::any_cast< int & >( any ) = m_json[ name ];
				else if( typeid( bool ).hash_code( ) == hash )
					std::any_cast< bool & >( any ) = m_json[ name ];
				else if( typeid( float ).hash_code( ) == hash )
					std::any_cast< float & >( any ) = m_json[ name ];
				else if( typeid( std::string ).hash_code( ) == hash )
					std::any_cast< std::string & >( any ) = m_json[ name ];
				else if( typeid( Color_f ).hash_code( ) == hash )
					std::any_cast< Color_f & >( any ).from_json( m_json[ name ] );
				else if( typeid( hotkey_t ).hash_code( ) == hash )
					std::any_cast< hotkey_t & >( any ).from_json( m_json[ name ] );
			} catch( std::exception & ) {
				continue;
			}
		}

		for( auto &child : m_children ) {
			child->Load( m_json[ std::to_string( child->GetName( ) ) ] );
		}
	}
};

// TODO: std::map group
template < class ArrayImpl, class = std::enable_if_t< std::is_base_of< CBaseGroup, ArrayImpl >::value > >
class CArrayGroup : public CBaseGroup {
public:
	CArrayGroup( ) { }

	CArrayGroup( const std::string &name, size_t count = 0 ) :
		CBaseGroup( name ) {
		m_children.reserve( count );
		for( auto i = 0u; i < count; ++i ) {
			m_children.emplace_back( new ArrayImpl( i ) );
		}
	}

	auto AddEntry( ) -> size_t {
		size_t idx = m_children.size( );
		m_children.emplace_back( new ArrayImpl( idx ) );
		return idx;
	}

	auto operator[]( ptrdiff_t idx ) -> ArrayImpl * {
		return ( ArrayImpl * )m_children[ idx ];
	}

	auto Size( ) const -> size_t {
		return m_children.size( );
	};

	// update only children
	auto Save( ) -> void {
		m_json.clear( );
		for( auto &child : m_children ) {
			child->Save( );

			auto json = child->GetJson( );
			m_json[ std::to_string( child->GetName( ) ) ] = json;
		}
	}

	auto Load( nlohmann::json &js ) -> void {
		m_json.clear( );
		m_json = js;
		for( auto &child : m_children ) {
			child->Load( m_json[ std::to_string( child->GetName( ) ) ] );
		}
	}
};

#define group_begin( group_name )                                  \
                                                                   \
  class group_name : public CBaseGroup {                           \
                                                                   \
  public:                                                          \
    group_name( CBaseGroup* parent = nullptr ) :                   \
        CBaseGroup( #group_name, parent ){};                       \
    group_name( const char* name, CBaseGroup* parent = nullptr ) : \
        CBaseGroup( name, parent ){};                              \
                                                                   \
    group_name( size_t idx, CBaseGroup* parent = nullptr ) :       \
        CBaseGroup( idx, parent ) {}

#define group_end() }

#define group_end_child( group_name, var_name ) \
  }                                             \
  ;                                             \
                                                \
  group_name var_name = group_name( XorStr(#group_name), this )

#define add_child_group( group_name, var_name ) group_name var_name = group_name( XorStr(#group_name), this )
#define add_child_group_ex( group_name, var_name ) group_name var_name = group_name( XorStr(#var_name), this )
#define add_child_group_name( group_name, var_name, name ) group_name var_name = group_name( XorStr(name), this )

#define config_option( type, name, ... ) type& name = *this->AddOption< type >( XorStr(#name), __VA_ARGS__ );
#define config_option_separate( type, name, parent, ... ) type& name = *parent.AddOption< type >( XorStr(#name), __VA_ARGS__ );

#define config_keybind( name ) config_option( hotkey_t, name )
#pragma endregion

class ConVar;

struct SpreadRandom_t {
	float flRand1;
	float flRandPi1;
	float flRand2;
	float flRandPi2;
};

struct KeybindWindow_t {
	hotkey_t *pHotkey;
	std::string szName = "";
};

struct Sounds_t {
	int m_sound_idx;
	Vector m_sound_origin;
	float m_sound_time;

	bool m_ack = false;
};

class CVariables : public CBaseGroup {
public:
	typedef struct _GLOBAL {
		int m_iSelectedConfig = 0;

		Color_f testtt1 = Color_f( 1.f, 1.f, 1.f, 1.f );
		Color_f testtt2 = Color_f( 1.f, 1.f, 1.f, 1.f );
		Color_f testtt3 = Color_f( 1.f, 1.f, 1.f, 1.f );
		Color_f testtt4 = Color_f( 1.f, 1.f, 1.f, 1.f );
		bool dotest;

		bool m_bGameOver = false;
		bool m_bUpdateAnims[ 65 ];

		std::vector<float > vecBrutePreSort;
		std::vector<float > vecBrutePostSort;

		std::vector<std::string> vecLayerDebug;

		bool oppa = false;
		bool boneSertup = FALSE;
		bool menuOpen = false;
		bool m_bDisableInput = false;
		bool hackUnload = false;
		bool bSendMessage = false;
		bool m_bRunningExploit;
		bool m_bLoadDefaultConfig = false;

		bool m_bInCreateMove = false;
		bool m_bInPostScreenEffects = false;

		bool m_bConsoleOpen = false;
		bool m_bChatOpen = false;
		bool m_bBuyMenuOpen = false;
		bool m_bIsGameFocused = false;

		bool m_bPlantingC4[ 65 ];

		int m_iCurrentRageGroup = 0;

		bool m_bForceFullUpdate = false;
		int m_iTicksSinceLastUpdate = 0;

		float m_flInaccuracy;

		bool m_bForceFakewalk = false;
		bool m_bFakeWalking = false;
		bool m_bInAwall = false;
		// collideable of the autowall target, if any. the OBB inflation in
		// Hooked::ClipRayCollideable applies to this one only, so we don't raise every
		// other collideable in the frame above its server-side volume.
		void *m_pAwallTargetCollideable = nullptr;
		bool m_bUpdatingAnimations;
		bool m_bDidQuickStop;
		bool m_bDoingQuickStop;
		bool m_bShotAutopeek = false;
		bool m_bShotWhileChoking = false;
		bool m_bShotWhileHiding = false;
		bool m_bSentPreviousTick = false;
		bool m_bThirdpersonAnimation = true;
		bool m_bFadeOutChams = true;
		bool m_bDidRagebot = false;
		bool m_bHoldingKnife = false;
		bool m_bLockAutoPeek = true;
		int m_bShotWhileHidingTicks = 0;
		int m_iTick;

		bool m_bAllowDPIScale = false;
		bool m_bAllowReadPackets = false;

		bool m_bHealthBarStringLeft = false;
		bool m_bUseCollisionBoundingBoxes = false;
		bool m_bChangedDefaultNameFont = false;
		bool m_bChangedDefaultWpnFont = false;
		bool m_bFlagsUppercase = true;
		bool m_bWeaponUppercase = true;
		int m_nFlagsOffset = 2;

		float test = 512;

		int m_nNumThreads;

		int m_iWeaponIndex = -1;
		int m_iWeaponIndexSkins = -1;

		bool m_bInsideFireRange = false;

		float m_flPenetrationDamage = 0.f;
		float m_flLastShotRealtime = 0.f;
		float m_flLastVelocityModifier;
		float m_flLastDuckAmount = 0.f;
		Vector m_vecLastShotEyePosition;

		int m_eLastStage;

		std::vector<std::string> m_vecPaintKits;
		std::vector<std::string> m_vecGloveKits;

		std::vector<KeybindWindow_t> m_vecKeybinds;

		std::vector<std::pair<float, std::string>> m_vecDevMessages;

		struct cheat_header_t {
			std::array<float, 128u> the_array;
			uint64_t sub_expiration;
			uint32_t access_token;
		};

		cheat_header_t user_info;

		HINSTANCE hModule;

		bool m_bDontCallingTheSkinNameMaybe;

		std::string szLastHookCalled = XorStr( "null" );
		std::vector<std::string> m_vecHitsounds;

		std::vector<Sounds_t> m_vecSounds;

		bool m_bInBoneSetup;

		Vector m_vecCameraPosition;
		QAngle m_angCameraAngles;

	} GLOBAL, *PGLOBAL;

	group_begin( MENU );
	config_option( int, m_count, 0 );
	config_option( int, m_selected, 0 );
	config_option( bool, expand_side, true );
	config_option( bool, lock_layout, false );
	config_option( bool, unsafe_scripts, false );
	config_option( bool, revolver_or_deagle, false );

	config_option( int, dpi_menu, 0 );
	config_option( int, dpi_esp, 0 );

	config_option( Color_f, ascent, Color_f( 76 / 255.f, 185 / 255.f, 254 / 255.f ) );
	config_keybind( key );

	config_option( bool, ignore_load_popup, false );
	config_option( bool, ignore_save_popup, false );
	config_option( bool, ignore_delete_popup, false );
	config_option( bool, ignore_reset_popup, false );

	config_option( bool, ignore_popup, true );
	config_option( bool, cursor, true );
	config_option( bool, watermark, true );
	config_option( bool, whitelist, true );
	config_keybind( whitelist_disable_key );
	config_option( float, size_x, 725 );
	config_option( float, size_y, 450 );

	config_option( float, keybind_pos_x, 50 );
	config_option( float, keybind_pos_y, 50 );
	group_end( );

#pragma region Skin Changer
	group_begin( skin_changer_data );
	config_option( bool, m_enabled, false );
	config_option( bool, m_filter_paint_kits, true );
	config_option( bool, m_custom, false );
	config_option( int, m_paint_kit, 0 );
	config_option( int, m_paint_kit_no_filter, 0 );
	config_option( int, m_seed, 0 );
	config_option( bool, m_stat_trak, false );
	config_option( float, m_wear, 100.f );
	config_option( float, m_phong_exponent, 0.f );
	config_option( float, m_phong_albedo_boost, 0.f );
	config_option( float, m_phong_intensity, 0.f );
	config_option( std::string, m_custom_name, "" );
	config_option( bool, m_change_paint_kit, false );
	config_option( bool, m_change_phong, false );

	config_option( bool, m_custom_color, false );
	config_option( bool, m_set_color, false );
	config_option( bool, m_reset_color, false );
	config_option( Color_f, color_1, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, color_2, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, color_3, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, color_4, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	// run-time data only
	//friend class Interfaces::feature::SkinChanger;
	//friend class OptionStorage;
	int m_paint_kit_index = 0;
	uint16_t m_definition_index = 0;
	bool m_executed = false;

	group_end( );

	group_begin( skin_changer_global_data );
	config_option( bool, m_active, false );

	config_option( bool, m_knife_changer, false );
	config_option( int, m_knife_idx, 0 );
	int m_knife_vector_idx = 0;

	config_option( bool, m_glove_changer, false );
	config_option( int, m_gloves_idx, 0 );
	int m_gloves_vector_idx = 0;

	bool m_update_gloves = false;
	bool m_update_skins = false;

	group_end( );
#pragma endregion

#pragma region Lua Scripts
	group_begin( lua_scripts_data );

	config_option( bool, m_load, false );
	config_option( std::string, m_script_name, XorStr( "" ) );

	config_option( std::string, m_script_id, XorStr( "" ) );
	config_option( bool, m_is_cloud, false );
	config_option( int, m_owner_id, 0 );

	group_end( );
#pragma endregion

#pragma region Rage general group
	group_begin( RAGE_GENERAL );
	config_option( bool, enabled, false );
	config_keybind( key );

	config_option( bool, dormant_aimbot, false );
	config_keybind( dormant_aimbot_key );

	config_option( bool, team_check, false );

	config_option( bool, knife_bot, false );
	config_option( int, knife_bot_type, 0 );
	config_option( bool, silent_aim, false );
	config_option( bool, visual_silent_aim, false );
	config_option( bool, auto_fire, false );
	config_option( bool, exploit, false );
	config_option( bool, double_tap_teleport, false );
	config_option( float, double_tap_teleport_distance, 10.f );
	config_option( bool, double_tap_recharge_knife, false );

	config_option( bool, double_tap_recharge_threat, false );
	config_option( bool, double_tap_recharge_shot_delay, false );
	config_option( bool, double_tap_recharge_custom_delay, false );
	config_option( int, double_tap_recharge_delay, 1 );

	config_option( bool, double_tap_adaptive, false );
	config_option( bool, double_tap_duck, false );
	config_option( bool, double_tap_peek, false );
	config_option( int, exploit_reserve, 1 );
	config_option( int, lag_peek_factor, 10 );
	config_option( int, double_tap_type, 0 );
	config_option( bool, force_body_aim, false );
	config_keybind( force_body_aim_key );

	config_option( bool, resolver, false );
	config_option( bool, resolver_trial, false );
	config_option( bool, deduct_bruteforce, false );
	config_option( bool, resolver_override, false );
	config_keybind( resolver_override_key );

	config_option( bool, wait_for_lby_flick, false );
	config_keybind( wait_for_lby_flick_key );

	config_option( bool, fake_lag, false );

	// conditions
	config_option( bool, fake_lag_standing, false );
	config_option( bool, fake_lag_moving, false );
	config_option( bool, fake_lag_air, false );
	config_option( bool, fake_lag_unduck, false );
	config_option( bool, fake_lag_peeking, false )
	config_option( bool, fake_lag_firing, false );

	config_option( int, fake_lag_amount, 7 );
	config_option( int, fake_lag_type, 0 );

	config_option( bool, anti_aim_active, false );
	config_option( bool, anti_aim_dormant_check, false );
	config_option( bool, anti_aim_at_players, false );

	config_option( int, anti_aim_pitch, 0 );
	config_option( int, anti_aim_yaw, 0 );
	config_option( bool, anti_aim_yaw_add_jitter, false );
	config_option( int, anti_aim_yaw_jitter, 0 );
	config_option( int, anti_aim_yaw_running, 0 );
	config_option( int, anti_aim_yaw_running_jitter, 0 );
	config_option( int, anti_aim_yaw_spin_speed, 1 );
	config_option( int, anti_aim_yaw_spin_direction, 180 );
	config_option( int, anti_aim_base_yaw_additive, 90 );
	config_option( float, anti_aim_experimental_range, 140.f );
	//config_option( int, anti_aim_yaw_jumping, 0 );
	//config_option( int, anti_aim_yaw_jumping_jitter, 0 );

	config_option( int, anti_aim_fake_yaw, 0 );
	config_option( int, anti_aim_fake_yaw_relative, 0 );
	config_option( int, anti_aim_fake_yaw_jitter, 0 );

	config_option( bool, anti_aim_twist, false );
	config_option( bool, anti_aim_edge, false );
	config_keybind( anti_aim_edge_key );
	config_option( bool, anti_aim_freestand, false );
	config_keybind( anti_aim_freestand_key );

	config_option( bool, anti_aim_random_on_hit, false );
	config_option( bool, linear_lel, false );

	config_option( int, anti_aim_fake_flick, 0 );
	config_keybind( anti_aim_fake_flick_key );

	config_option( bool, anti_aim_fake_body, false );
	config_option( int, anti_aim_fake_body_side, 0 );
	config_option( bool, anti_aim_fake_body_crooked, false );
	config_option( bool, anti_aim_fake_body_twist, false );
	config_option( bool, anti_aim_fake_body_hide, false );
	config_option( bool, anti_aim_fake_body_balance, true );
	config_option( int, anti_aim_fake_body_amount, 115 );
	config_option( int, aaa, 14 );
	config_option( float, aaaa, 22 );

	config_option( bool, anti_aim_desync_land_first, false );
	config_option( bool, anti_aim_desync_land_force, false );
	config_keybind( anti_aim_desync_land_key );
	config_option( int, anti_aim_desync_land_delta, 135 );

	config_option( bool, anti_aim_distortion_stand, false );
	config_option( bool, anti_aim_distortion_air, false );
	config_option( bool, anti_aim_distortion_move, false );
	config_option( bool, anti_aim_distortion_crouch, false );
	config_option( int, anti_aim_distortion_side, 0 );
	config_option( bool, anti_aim_distortion_disable_on_manual, false )
	config_option( bool, anti_aim_distortion_hide_flick, false );
	config_option( bool, anti_aim_distortion_hide_moving, false );
	config_keybind( anti_aim_distortion_key );

	config_option( bool, anti_aim_manual, false );

	config_option( bool, anti_aim_manual_arrows, false );
	config_option( int, anti_aim_manual_arrows_size, 22 );
	config_option( int, anti_aim_manual_arrows_spacing, 60 );
	config_option( Color_f, anti_aim_manual_arrows_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, anti_aim_manual_ignore_distortion, false );
	config_option( bool, anti_aim_indi, false );
	config_option( bool, anti_aim_fps, false );
	config_option( Color_f, anti_aim_indi_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );


	config_keybind( anti_aim_lock_angle_key );
	config_keybind( anti_aim_manual_left_key );
	config_keybind( anti_aim_manual_right_key );
	config_keybind( anti_aim_manual_back_key );
	config_keybind( anti_aim_manual_forward_key );

	config_option( bool, disable_anti_aim_manual_air, false );

	config_keybind( min_damage_override_key );
	config_keybind( force_safepoint_key );

	config_keybind( double_tap_bind );
	config_option( bool, double_tap_lc_break, false );
	config_option( float, double_tap_hitchance, 0.f );
	config_keybind( hide_shots_bind );

	config_keybind( force_recharge );

	config_option( bool, resolver_low_delta_shots_enable, false );
	config_option( int, resolver_low_delta_shots, 0 );

	config_option( int, target_limit, 4 );

	config_option( bool, low_performance_bt, false );

	config_option( bool, visualize_aimpoints, false );
	config_option( Color_f, visualize_aimpoints_clr, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	// debug stuff here.
#if defined(DEV) || defined(BETA_MODE) || defined(DEBUG_MODE)
	config_option( bool, visualize_safepoints, false );
	config_option( bool, visualize_basic_safepoints, false );

	config_option( bool, visualize_resolved_angles, false );
	config_option( bool, visualize_extrap_stomach, false );
	config_option( bool, visualize_old_logs, false );
	config_option( bool, breaking_lc, false );
#endif

	group_end( );

	group_begin( RAGE );
	config_option( bool, override_default_config, false );

	config_option( bool, hitbox_head, false );
	config_option( bool, hitbox_neck, false );
	config_option( bool, hitbox_upper_chest, false );
	config_option( bool, hitbox_chest, false );
	config_option( bool, hitbox_lower_chest, false );
	config_option( bool, hitbox_stomach, false );
	config_option( bool, hitbox_pelvis, false );
	config_option( bool, hitbox_arms, false );
	config_option( bool, hitbox_legs, false );
	config_option( bool, hitbox_feet, false );

	config_option( bool, multipoint_head, false );
	config_option( bool, multipoint_upper_chest, false );
	config_option( bool, multipoint_chest, false );
	config_option( bool, multipoint_lower_chest, false );
	config_option( bool, multipoint_arms, false );
	config_option( bool, multipoint_stomach, false );
	config_option( bool, multipoint_legs, false );
	config_option( bool, multipoint_feet, false );

	config_option( int, multipoint_style, 0 );

	config_option( int, fake_lag_correction, 0 );
	config_option( int, accuracy_boost, 0 );

	config_option( int, hitchance, 45 );
	config_option( float, hitchance_dt, 0.f );
	config_option( bool, ensure_accuracy_strict, true );
	config_option( float, damage_accuracy, 0.f );
	config_option( bool, fake_duck_accurate, true );

	config_option( float, pointscale, 0.f );
	config_option( float, headscale, 0.f );
	config_option( bool, static_pointscale, false );

	config_option( int, minimal_damage, 0 );
	config_option( int, minimal_damage_override, 5 );
	config_option( bool, disable_wall_penetration, false );
	config_option( int, wall_penetration_damage, 20 );
	config_option( bool, lower_air_minimal_damage, false );

	config_option( bool, scale_damage_on_hp, false );

	config_option( bool, auto_scope, false );
	config_option( bool, auto_scope_hitchance, false );
	config_option( bool, auto_scope_rifles, false );

	config_option( int, auto_stop, 0 );
	config_option( bool, auto_stop_between_shots, false );
	config_option( bool, lock_fakewalk_on_movement, false );

	config_option( bool, prefer_body_always, false );
	config_option( bool, prefer_body_lethal, false );
	config_option( bool, prefer_body_lethal_x2, false );
	config_option( bool, prefer_body_fake, false );
	config_option( bool, prefer_body_air, false );
	config_option( bool, prefer_body_exploit, false );

	config_option( bool, force_body_always, false );
	config_option( bool, force_body_health, false );
	config_option( int, force_body_health_min, 50 );
	config_option( bool, force_body_fake, false );
	config_option( bool, force_body_air, false );
	config_option( bool, force_body_peek, false );

	config_option( bool, on_peek_chest, false );
	config_option( bool, on_peek_pelvis, true );
	config_option( bool, on_peek_stomach, false );
		
		group_end( );
#pragma endregion

#pragma region Antiaim group

	group_begin( ANTIAIM );


	group_end( );
#pragma endregion

#pragma region Fakelag group
	group_begin( FAKELAG );

	group_end( );
#pragma endregion

#pragma region Chams group
	group_begin( CHAMS );

	config_option( bool, enabled, false );

	config_option( int, material, 1 );

	config_option( bool, visible, false );
	config_option( Color_f, visible_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, invisible, false );
	config_option( Color_f, invisible_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( float, reflectivity, 0.f );
	config_option( Color_f, reflectivity_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( float, rim, 0.f );
	config_option( float, shine, 0.f );

	config_option( bool, glow_overlay, false );
	config_option( bool, glow_overlay_through_walls, false );
	config_option( float, glow_overlay_opacity, 50.f );
	config_option( Color_f, glow_overlay_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	group_end( );
#pragma endregion

#pragma region ESP group
	// TODO: separate visuals from esp
	group_begin( ESP );


	// chams
	config_option( bool, chams_enabled, false );

	config_option( bool, chams_local, false );
	config_option( bool, chams_desync, false );
	config_option( bool, chams_hands, false );
	config_option( bool, chams_weapon, false );
	config_option( bool, chams_attachments_local, false );
	config_option( bool, chams_attachments_enemy, false );
	config_option( bool, chams_enemy, false );
	config_option( bool, chams_teammate, false );
	config_option( bool, chams_history, false );
	config_option( bool, chams_shot, false );

	config_option( int, chams_local_mat, 0 );
	config_option( int, chams_desync_mat, 0 );
	config_option( int, hands_chams_mat, 0 );
	config_option( int, weapon_chams_mat, 0 );
	config_option( int, attachments_chams_local_mat, 0 );
	config_option( int, attachments_chams_enemy_mat, 0 );
	config_option( int, enemy_chams_mat, 0 );
	config_option( int, team_chams_mat, 0 );
	config_option( int, enemy_chams_death_mat, 0 );
	config_option( int, team_chams_death_mat, 0 );
	config_option( int, chams_history_mat, 0 );
	config_option( int, chams_hitmatrix_mat, 0 );

	config_option( bool, enemy_chams_xqz, false );
	config_option( bool, enemy_chams_vis, false );
	config_option( bool, team_chams_xqz, false );
	config_option( bool, team_chams_vis, false );
	config_option( bool, enemy_death_chams_history_xqz, false );
	config_option( bool, enemy_death_chams_history_vis, false );

	config_option( bool, chams_local_outline, false );
	config_option( bool, chams_local_outline_wireframe, false );

	config_option( bool, chams_ghost_outline, false );
	config_option( bool, chams_ghost_outline_wireframe, false );

	config_option( bool, chams_enemy_outline, false );
	config_option( bool, chams_enemy_outline_visible, false );
	config_option( bool, chams_enemy_outline_wireframe, false );

	config_option( bool, chams_hands_outline, false );
	config_option( bool, chams_hands_outline_wireframe, false );
	config_option( bool, chams_weapon_outline, false );
	config_option( bool, chams_weapon_outline_wireframe, false );
	config_option( bool, chams_attachments_outline_enemy, false );
	config_option( bool, chams_attachments_outline_wireframe_enemy, false );
	config_option( bool, chams_attachments_outline_local, false );
	config_option( bool, chams_attachments_outline_wireframe_local, false );
	config_option( bool, chams_teammate_outline, false );
	config_option( bool, chams_history_outline, false );
	config_option( bool, chams_history_outline_wireframe, false );
	config_option( bool, chams_hitmatrix_outline, false );
	config_option( bool, chams_hitmatrix_outline_wireframe, false );

	config_option( float, chams_local_outline_value, 0.f );
	config_option( float, chams_ghost_outline_value, 0.f );
	config_option( float, chams_hands_outline_value, 0.f );
	config_option( float, chams_weapon_outline_value, 0.f );
	config_option( float, chams_attachments_outline_enemy_value, 0.f );
	config_option( float, chams_attachments_outline_local_value, 0.f );
	config_option( float, chams_enemy_outline_value, 0.f );
	config_option( float, chams_teammate_outline_value, 0.f );
	config_option( float, chams_history_outline_value, 0.f );
	config_option( float, chams_lag_outline_value, 0.f );
	config_option( float, chams_hitmatrix_outline_value, 0.f );

	config_option( Color_f, chams_local_shine_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, chams_ghost_shine_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, chams_hands_shine_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, chams_weapon_shine_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, chams_enemy_shine_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, chams_teammate_shine_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, chams_history_shine_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, chams_lag_shine_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, chams_hitmatrix_shine_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( Color_f, chams_local_outline_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, chams_ghost_outline_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, chams_hands_outline_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, chams_weapon_outline_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, chams_attachments_enemy_outline_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, chams_attachments_local_outline_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, chams_enemy_outline_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, chams_teammate_outline_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, chams_history_outline_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, chams_lag_outline_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, chams_hitmatrix_outline_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( Color_f, chams_local_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, chams_desync_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, hands_chams_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, weapon_chams_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, attachments_chams_local_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, attachments_chams_enemy_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, enemy_chams_color_vis, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, team_chams_color_vis, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, enemy_chams_color_xqz, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, team_chams_color_xqz, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, chams_lag_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, chams_history_color1, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, chams_history_color2, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, chams_shot_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( Color_f, chams_local_pearlescence_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( float, chams_local_pearlescence, 0.f );
	config_option( float, chams_local_shine, 0.f );

	config_option( Color_f, chams_desync_pearlescence_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( float, chams_desync_pearlescence, 0.f );
	config_option( float, chams_desync_shine, 0.f );

	config_option( Color_f, chams_local_attachments_pearlescence_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( float, chams_local_attachments_pearlescence, 0.f );
	config_option( float, chams_local_attachments_shine, 0.f );

	config_option( Color_f, chams_enemy_pearlescence_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( float, chams_enemy_pearlescence, 0.f );
	config_option( float, chams_enemy_shine, 0.f );

	config_option( Color_f, chams_history_pearlescence_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( float, chams_history_pearlescence, 0.f );
	config_option( float, chams_history_shine, 0.f );

	config_option( Color_f, chams_weapon_pearlescence_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( float, chams_weapon_pearlescence, 0.f );
	config_option( float, chams_weapon_shine, 0.f );

	config_option( Color_f, chams_hands_pearlescence_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( float, chams_hands_pearlescence, 0.f );
	config_option( float, chams_hands_shine, 0.f );

	config_option( Color_f, chams_hitmatrix_pearlescence_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( float, chams_hitmatrix_pearlescence, 0.f );
	config_option( float, chams_hitmatrix_shine, 0.f );

	config_option( bool, chams_hitmatrix, false );
	config_option( float, chams_hitmatrix_duration, 3.f );
	config_option( Color_f, chams_hitmatrix_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( Color_f, local_skeleton_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, local_skeleton_outline_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( bool, local_skeleton, false );
	config_option( bool, local_skeleton_head, false );

	config_option( bool, remove_scope, false );
	config_option( int, remove_scope_type, 1 );
	config_option( bool, remove_scope_zoom, false );
	config_option( bool, remove_scope_blur, false );
	config_option( bool, remove_recoil_shake, false );
	config_option( bool, remove_recoil_punch, false );
	config_option( bool, remove_flash, false );
	config_option( bool, remove_smoke, false );
	config_option( bool, remove_sleeves, false );
	config_option( bool, remove_hands, false );
	config_option( bool, remove_bob, false );
	config_option( bool, remove_fog, false );
	config_option( bool, remove_zoom_senstivity_decrease, false );

	config_option( Color_f, scope_gradient_color1, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, scope_gradient_color2, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( int, scope_gradient_padding, 25 );
	config_option( int, scope_gradient_width, 100 );
	config_option( int, scope_gradient_height, 100 );

	config_option( bool, keybind_window, false );

	config_option( bool, draw_c4_2d, false );
	config_option( bool, draw_c4_3d, false );
	config_option( Color_f, draw_c4_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( int, target_capsules, 0 );
	config_option( Color_f, target_capsules_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, autowall_crosshair, false );
	config_option( Color_f, autowall_crosshair_no_penetration, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, autowall_crosshair_penetration, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, skip_occulusion, false );

	config_option( bool, nades, false );
	config_option( Color_f, nades_text_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, dropped_weapons, false );
	config_option( bool, dropped_weapons_glow, false );
	config_option( bool, dropped_weapons_ammo, false );
	config_option( Color_f, dropped_weapons_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, dropped_weapons_ammo_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, dropped_weapons_glow_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );


	config_option( int, sky_changer, 0 );
	config_option( std::string, sky_changer_name, "" );

	config_option( bool, extended_esp, false );

	config_option( bool, fov_crosshair, false );
	config_option( Color_f, fov_crosshair_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, force_sniper_crosshair, false );

	config_option( bool, vizualize_hitmarker, false );
	config_option( Color_f, hitmarker_screen_headshot_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, hitmarker_screen_regular_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, visualise_damage_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, hitsound, false );
	config_option( int, hitsound_type, 0 );
	config_option( int, hitsound_custom, 0 );
	config_option( float, hitsound_volume, 100.f );

	config_option( bool, visualize_hitmarker_world, false );
	config_option( bool, visualize_damage, false );

	config_option( bool, zeus_distance, false );
	config_option( Color_f, zeus_distance_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, blur_in_scoped, false );
	config_option( bool, blur_on_grenades, false );
	config_option( float, blur_in_scoped_value, 50.f );

	config_option( Color_f, backtrack_ideal_record_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( bool, backtrack_draw_last, true );
	config_option( bool, backtrack_draw_ideal, false );
	config_option( bool, backtrack_draw_lerped, false );
	config_option( bool, backtrack_draw_all, false );
	config_option( bool, backtrack_manage_alpha, false );
	config_option( bool, backtrack_current_threat, false );

	config_option( bool, lerp_visible, false );
	config_option( bool, lerp_invisible, false );
	config_option( bool, lerp_lerped, false );
	config_option( Color_f, second_lerped_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, aspect_ratio, false );
	config_option( float, aspect_ratio_value, 1.5f );

	config_option( int, spread_crosshair, 0 );
	config_option( float, spread_crosshair_opacity, 100.f );
	config_option( Color_f, spread_crosshair_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );


	config_option( bool, remove_blur_effect, false );
	config_option( bool, remove_post_proccesing, false );

	config_option( bool, server_impacts, false );
	config_option( bool, client_impacts, false );
	config_option( bool, target_impacts, false );
	config_option( Color_f, server_impacts_color, Color_f( 0.0f, 0.0f, 1.f, 0.5f ) );
	config_option( Color_f, client_impacts_color, Color_f( 1.f, 0.f, 0.f, 0.5f ) );

	config_option( Color_f, target_impacts_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( float, impacts_size, 1.5f );

	config_option( bool, world_modulation, false );
	config_option( bool, prop_modulation, false );
	config_option( bool, sky_modulation, false );
	config_option( bool, fullbright_modulation, false );

	config_option( Color_f, world_modulation_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, prop_modulation_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, sky_modulation_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( int, weather, 0 );
	config_option( float, weather_alpha, 40.f );

	config_option( bool, indicator_mindmg, false );
	config_option( bool, indicator_antiaim_lby, false );
	config_option( Color_f, indicator_antiaim_side_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( bool, indicator_antiaim_side, false );
	config_option( bool, indicator_crosshair, false );
	config_option( bool, indicator_pingspike, false );
	config_option( bool, indicator_lagcomp, false );

	config_option( bool, crosshair_indicator_mindmg, false );
	config_option( bool, crosshair_indicator_lby, false );
	config_option( bool, crosshair_indicator_override, false );
	config_option( bool, crosshair_indicator_body, false );
	config_option( bool, crosshair_indicator_ping, false );

	config_option( bool, fog_effect, false );
	config_option( bool, fog_blind, false );
	config_option( float, fog_density, 0.f );
	config_option( float, fog_hdr_scale, 0.f );
	config_option( int, fog_distance, 1000 );
	config_option( Color_f, fog_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, fog_color_secondary, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, weather_controller, false );
	config_option( int, weather_type, 0 );

	config_option( bool, sunset_mode, false );

	config_option( bool, ambient_ligtning, false );
	config_option( Color_f, ambient_ligtning_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, fog_modulation, false );
	config_option( Color_f, fog_modulation_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( float, fog_start_s, 0.f );
	config_option( float, fog_end_s, 7500.f );

	config_option( bool, grenade_prediction, false );
	config_option( Color_f, grenade_prediction_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, grenade_proximity_warning, false );
	config_option( Color_f, grenade_proximity_warning_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, bullet_tracer_local, false );
	config_option( Color_f, bullet_tracer_local_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, bullet_tracer_local_glow_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( bool, bullet_tracer_enemy, false );
	config_option( Color_f, bullet_tracer_enemy_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, bullet_tracer_enemy_glow_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( int, bullet_tracer_life, 3 );

	config_option( bool, grenades, false );
	config_option( Color_f, grenades_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, grenades_glow, false );
	config_option( Color_f, grenades_glow_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, grenades_radius, false );

	config_option( bool, grenades_radius_fire, false );
	config_option( Color_f, grenades_radius_fire_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, grenades_radius_smoke, false );
	config_option( Color_f, grenades_radius_smoke_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, third_person, false );
	config_option( bool, first_person_on_nade, false );
	config_option( bool, first_person_dead, false );
	config_keybind( third_person_bind );

	config_option( bool, shared_esp, false );
	config_option( bool, shared_esp_teammates, false );

	config_option( bool, ingame_radar, false );
	config_option( int, spectators, 0 );

	group_end( );
#pragma endregion

	group_begin( PLAYER_VISUALS );

	config_option( bool, box, false );
	config_option( Color_f, box_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, health, false );
	config_option( bool, health_color_override, true );
	config_option( Color_f, health_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, name, false );
	config_option( Color_f, name_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, skeleton, false );
	config_option( Color_f, skeleton_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );
	config_option( Color_f, skeleton_outline_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, history_skeleton, false );
	config_option( bool, history_skeleton_all, false );
	config_option( bool, history_skeleton_all_manage_alpha, false );
	config_option( Color_f, history_skeleton_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, ammo, false );
	config_option( Color_f, ammo_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, lby_timer, false );
	config_option( Color_f, lby_timer_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, ping, false );
	config_option( Color_f, ping_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, weapon, false );
	config_option( bool, weapon_ignore_skin, false );
	config_option( Color_f, weapon_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, weapon_icon, false );
	config_option( Color_f, weapon_icon_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, view_arrows, false );
	config_option( float, view_arrows_distance, 50.f );
	config_option( int, view_arrows_size, 16 );
	config_option( Color_f, view_arrows_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, glow, false );
	config_option( Color_f, glow_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, disable_material, false );

	config_option( bool, outline, false );
	config_option( Color_f, outline_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, flag_money, false );
	config_option( bool, flag_ping, false );
	config_option( bool, flag_armor, false );
	config_option( bool, flag_scope, false );
	config_option( bool, flag_utility, false );
	config_option( bool, flag_flashed, false );
	config_option( bool, flag_reloading, false );
	config_option( bool, flag_distance, false );
	config_option( bool, flag_exploit, false );

	config_option( bool, flag_resolver, false );
	config_option( bool, flag_resolver_mode, false );
	config_option( bool, flag_resolver_body, false );
	config_option( bool, flag_debug, false );

	config_option( bool, dormant_chams, false );

	config_option( bool, dormant, false );
	config_option( bool, dormant_color_custom, false );
	config_option( bool, dormant_xtnd, false );
	config_option( Color_f, dormant_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	group_end( );

#pragma region LUA group
	group_begin( LUA );

	config_option( bool, test, false );

	group_end( );

#pragma endregion

#pragma region MISC group
	group_begin( MISC );

	config_option( bool, active, false );
	config_option( bool, bunnyhop, false );
	config_option( bool, autostrafer, false );
	config_option( bool, autostrafer_wasd, false );
	config_option( float, autostrafer_smooth, 70.f );
	config_option( bool, show_skinchanger, false );
	config_option( bool, move_exploit, false );
	config_keybind( move_exploit_key );

	config_option( bool, fastduck, false );
	config_option( bool, edge_jump, false );
	config_keybind( edge_jump_key );
	config_option( bool, quickstop, false );
	config_option( bool, accurate_walk, false );
	config_option( bool, slide_walk, false );

	config_option( bool, mask_changer, false );
	config_option( int, mask_changer_selection, 0 );

	config_option( bool, slow_walk, false );
	config_option( int, slow_walk_type, 0 );
	config_keybind( slow_walk_bind );
	config_option( float, slow_walk_speed, 100.f );
	config_option( bool, slow_walk_custom, false );

	config_option( float, viewmodel_fov, 68.f );
	config_option( bool, viewmodel_change, false );
	config_option( float, viewmodel_x, 2.f );
	config_option( float, viewmodel_y, 2.f );
	config_option( float, viewmodel_z, -2.f );

	config_option( bool, clantag_changer, false );

	config_option( bool, preserve_killfeed, false );
	config_option( float, preserve_killfeed_time, 1.0f );

	config_option( bool, event_bomb, false );
	config_option( bool, event_harm, false );
	config_option( bool, event_dmg, false );
	config_option( bool, event_buy, false );
	config_option( bool, event_resolver, false );
	config_option( bool, event_misc, false );
	config_option( bool, event_console, false );
	config_option( bool, event_fire, false );

	config_option( bool, cl_lagcomp_bypass, false );
	config_option( bool, unlocked_hidden_cvars, false );

	// rotate the in-game radar (and the local blip arrow) to the real
	// server-side yaw, in first and third person.
	config_option( bool, radar_real_yaw, false );

	config_option( bool, autobuy_enabled, false );
	config_option( int, autobuy_first_weapon, 0 );
	config_option( int, autobuy_second_weapon, 0 );
	config_option( bool, autobuy_flashbang, false );
	config_option( bool, autobuy_smokegreanade, false );
	config_option( bool, autobuy_molotovgrenade, false );
	config_option( bool, autobuy_hegrenade, false );
	config_option( bool, autobuy_decoy, false );
	config_option( bool, autobuy_armor, false );
	config_option( bool, autobuy_zeus, false );
	config_option( bool, autobuy_defusekit, false );

	config_option( int, leg_movement, 0 );

	config_option( bool, zeus_bot, false );
	config_option( float, zeus_bot_hitchance, 80.f );

	config_option( bool, anti_untrusted, true );

	config_option( bool, autopeek, false );
	config_option( int, autopeek_retrack, 0 );

	config_keybind( autopeek_bind );
	config_option( Color_f, autopeek_color, Color_f( 1.f, 1.f, 1.f, 1.f ) );

	config_option( bool, money_revealer, false );
	config_option( bool, auto_accept, false );

	config_option( bool, unlock_inventory, false );
	config_option( bool, auto_release_grenade, false );

	config_option( float, override_fov, 90.f );
	config_option( float, override_fov_scope, 100.f );
	config_option( bool, show_viewmodel_in_scope );

	config_option( bool, filter_console, true );
	config_option( bool, ignore_radio, true );
	config_option( bool, extended_backtrack, false );
	config_option( bool, ping_spike, false );
	config_keybind( ping_spike_key );
	config_option( int, ping_spike_amount, 200 );

	config_option( bool, time_shift, false );
	config_keybind( time_shift_key );
	config_option( int, time_shift_amount, 200 );

	config_keybind( force_ignore_whitelist );

	config_option( bool, disable_server_messages, false );

#if defined(BETA_MODE) || defined(DEV)
	config_option( bool, undercover_log, false );
	config_option( bool, undercover_fire_log, false );
	config_option( bool, undercover_watermark, false );
	config_option( bool, debug_dead_time, false );
	config_option( bool, debug_show_models, false );
#if defined(DEV)
	config_option( int, what_developer_is_this, 0 );
#endif

#endif

	group_end( );

#pragma endregion

	add_child_group_ex( MENU, menu );
	add_child_group_ex( RAGE_GENERAL, rage );

	RAGE *rage_option( );
	RAGE *current_rage_option;

	add_child_group_ex( RAGE, rage_default );
	add_child_group_ex( RAGE, rage_pistols );
	add_child_group_ex( RAGE, rage_revolver );
	add_child_group_ex( RAGE, rage_deagle );
	add_child_group_ex( RAGE, rage_awp );
	add_child_group_ex( RAGE, rage_scout );
	add_child_group_ex( RAGE, rage_autosnipers );
	add_child_group_ex( RAGE, rage_zeus );

	add_child_group_ex( ANTIAIM, antiaim );
	add_child_group_ex( ESP, esp );

	add_child_group_ex( CHAMS, chams_enemy );
	add_child_group_ex( CHAMS, chams_teammates );
	add_child_group_ex( CHAMS, chams_local );
	add_child_group_ex( CHAMS, chams_body_update_enemy );
	add_child_group_ex( CHAMS, chams_body_update_local );
	add_child_group_ex( CHAMS, chams_backtrack );
	add_child_group_ex( CHAMS, chams_aimbot );
	add_child_group_ex( CHAMS, chams_ragdolls );

	add_child_group_ex( CHAMS, chams_weapons );
	add_child_group_ex( CHAMS, chams_arms );
	add_child_group_ex( CHAMS, chams_attachments_local );
	add_child_group_ex( CHAMS, chams_attachments_enemy );

	add_child_group_ex( PLAYER_VISUALS, visuals_enemy );
	add_child_group_ex( PLAYER_VISUALS, visuals_local );
	add_child_group_ex( MISC, misc );
	add_child_group_ex( skin_changer_global_data, m_global_skin_changer );

	CArrayGroup<skin_changer_data> m_skin_changer;
	CArrayGroup<lua_scripts_data> m_loaded_luas;

	add_child_group_ex( LUA, lua );

	GLOBAL globals;

	// convars
	ConVar *sv_accelerate;
	ConVar *sv_airaccelerate;
	ConVar *sv_gravity;
	ConVar *sv_jump_impulse;
	ConVar *sv_penetration_type;
	ConVar *sv_showimpacts_time;

	ConVar *m_yaw;
	ConVar *m_pitch;
	ConVar *sensitivity;

	ConVar *cl_sidespeed;
	ConVar *cl_forwardspeed;
	ConVar *cl_upspeed;
	ConVar *cl_extrapolate;
	ConVar *cl_lagcompensation;

	ConVar *sv_noclipspeed;

	ConVar *weapon_recoil_scale;
	ConVar *view_recoil_tracking;

	ConVar *r_jiggle_bones;

	ConVar *mp_friendlyfire;

	ConVar *sv_maxunlag;
	ConVar *sv_minupdaterate;
	ConVar *sv_maxupdaterate;

	ConVar *sv_client_min_interp_ratio;
	ConVar *sv_client_max_interp_ratio;
	ConVar *sv_showlagcompensation_duration;

	ConVar *cl_interp_ratio;
	ConVar *cl_interp;
	ConVar *cl_updaterate;
	ConVar *cl_pred_optimize;

	ConVar *game_type;
	ConVar *game_mode;

	ConVar *ff_damage_bullet_penetration;
	ConVar *ff_damage_reduction_bullets;

	ConVar *mp_c4timer;

	ConVar *mp_teammates_are_enemies;
	ConVar *mp_damage_scale_ct_head;
	ConVar *mp_damage_scale_t_head;
	ConVar *mp_damage_scale_ct_body;
	ConVar *mp_damage_scale_t_body;
	ConVar *mp_anyone_can_pickup_c4;
	ConVar *mp_maxrounds;

	ConVar *viewmodel_fov;
	ConVar *viewmodel_offset_x;
	ConVar *viewmodel_offset_y;
	ConVar *viewmodel_offset_z;

	ConVar *mat_ambient_light_r;
	ConVar *mat_ambient_light_g;
	ConVar *mat_ambient_light_b;

	ConVar *sv_show_impacts;

	ConVar *molotov_throw_detonate_time;
	ConVar *weapon_molotov_maxdetonateslope;
	ConVar *net_client_steamdatagram_enable_override;
	ConVar *mm_dedicated_search_maxping;

	ConVar *cl_foot_contact_shadows;
	ConVar *cl_csm_shadows;
	ConVar *cl_csm_max_shadow_dist;
	ConVar *cl_csm_rot_override;
	ConVar *cl_csm_rot_x;
	ConVar *cl_csm_rot_y;
	ConVar *cl_csm_rot_z;

	ConVar *developer;
	ConVar *con_enable;
	ConVar *con_filter_enable;
	ConVar *con_filter_text;
	ConVar *con_filter_text_out;
	ConVar *contimes;
	ConVar *crosshair;

	ConVar *r_drawmodelstatsoverlay;

	ConVar *sv_clockcorrection_msecs;
	ConVar *sv_max_usercmd_future_ticks;
	ConVar *sv_maxusrcmdprocessticks;
	ConVar *engine_no_focus_sleep;
	ConVar *r_3dsky;

	ConVar *host_limitlocal;

	ConVar *r_RainRadius;
	ConVar *r_rainalpha;
	ConVar *cl_crosshair_sniper_width;
	ConVar *sv_friction;
	ConVar *sv_stopspeed;
	ConVar *sv_maxspeed;
	ConVar *name;
	ConVar *sleep_when_meeting_framerate;

	ConVar *zoom_sensitivity_ratio_mouse;

	ConVar *fog_override;
	ConVar *fog_start;
	ConVar *fog_end;
	ConVar *fog_startskybox;
	ConVar *fog_endskybox;
	ConVar *fog_color;
	ConVar *fog_colorskybox;
	ConVar *fog_maxdensity;
	ConVar *maxdensityskybox;

	void Create( );
};

struct WeaponName_t {
	WeaponName_t( int32_t definition_index, const char *name ) :
		definition_index( definition_index ),
		name( name ? name : "" ) {
	}

	int32_t definition_index = 0;
	// Owns the string: KitParser used to store a c_str() view into
	// item_skins::display_name (vecWeapons reallocates during init),
	// leaving menu dropdowns with dangling names.
	std::string name;
};

extern CVariables g_Vars;
