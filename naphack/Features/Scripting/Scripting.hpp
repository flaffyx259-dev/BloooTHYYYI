#pragma once

#include "../../Libraries/LUA/lua/lua.hpp"
#include "../../Libraries/LUA/sol/sol.hpp"
#include "../../Menu/Framework/GUI.h"

#include <mutex>


namespace Scripting {
#if defined(LUA_SCRIPTING)
	extern bool initialized;

	enum itemType_t {
		NONE,
		CHECKBOX,
		DROPDOWN,
		MULTIDROPDOWN,
		COLORPICKER,
		HOTKEY,
		COG,
		SLIDER,
		SLIDER_FLOAT,
		LABEL,
		BUTTON,
		TEXTBOX
	};

	class uiItem {
	public:
		virtual void init( std::string name ) = 0;
		virtual itemType_t get_type( ) {
			return NONE;
		}

		int get( ) {
			return -1;
		}

		void set( int input ) {

		}

		void set_visible( bool input ) {
			visible = input;
		}

	public:
		itemType_t type;
		bool visible = true;
	};

	class checkboxItem : public uiItem {
	public:
		void init( std::string name ) override {
			value = g_Vars.lua.Add<bool>( name.c_str( ), false );
		}

		itemType_t get_type( ) override {
			return CHECKBOX;
		}

		bool get( ) {
			if( !value )
				return false;

			return *value;
		}

		void set( bool input ) {
			if( !value )
				return;

			*value = input;
		}

	public:
		bool* value = nullptr;
	};

	class dropdownItem : public uiItem {
	public:
		void init( std::string name ) override {
			value = g_Vars.lua.Add<int>( name.c_str( ), 0 );
		}

		itemType_t get_type( ) override {
			return DROPDOWN;
		}

		int get( ) {
			if( !value )
				return 0;

			return *value;
		}

		void set( int input ) {
			if( !value )
				return;

			*value = input;
		}

		void update_items( std::vector<std::string> new_items ) {
			items = new_items;
		}

	public:
		int* value = nullptr;
		std::vector<std::string> items;
	};

	class multidropdownItem : public uiItem {
	public:
		void init( std::string name ) override {
			items = new std::vector<MultiItem_t>;
		}

		itemType_t get_type( ) override {
			return MULTIDROPDOWN;
		}

		std::vector<bool> get_all( ) {
			std::vector<bool> ret;

			if( !items )
				return ret;

			for( auto& item : *items ) {
				ret.push_back( *item.value );
			}

			return ret;
		}

		bool get( std::string option ) {
			if( !items || items->empty( ) )
				return false;

			for( auto& item : *items ) {
				if( item.name == option ) {
					return *item.value;
				}
			}

			return false;
		}


		void set( std::string option, bool input ) {
			if( !items || items->empty( ) )
				return;

			for( auto& item : *items ) {
				if( item.name == option ) {
					*item.value = input;
				}
			}
		}

	public:
		std::vector<MultiItem_t>* items;
	};

	class hotkeyItem : public uiItem {
	public:
		void init( std::string name ) override {
			hotkey = g_Vars.lua.Add< hotkey_t >( name.c_str( ) );
		}

		itemType_t get_type( ) override {
			return HOTKEY;
		}

		std::pair<bool, int> get_hotkey( ) {
			if( !hotkey )
				return std::make_pair( false, 0 );

			return std::make_pair( hotkey->enabled, hotkey->cond );
		}

		void set_hotkey( bool input ) {
			if( !hotkey )
				return;

			hotkey->enabled = input;
		}

		void set_hotkey_cond( int input ) {
			if( !hotkey )
				return;

			hotkey->cond = std::clamp( input, 0, 2 );
		}

	public:
		hotkey_t* hotkey;
	};

	class colorpickerItem : public uiItem {
	public:
		void init( std::string name ) override {
			color = g_Vars.lua.Add<Color_f>( name.c_str( ), Color_f::White );
		}

		itemType_t get_type( ) override {
			return COLORPICKER;
		}

		Color get_color( ) {
			if( !color )
				return Color( 255, 255, 255 );

			return color->ToRegularColor( );
		}

		void set_color( Color input ) {
			if( !color )
				return;

			color->SetColor( input );
		}

	public:
		Color_f *color;
	};

	// int slider only for now..
	class sliderItem : public uiItem {
	public:
		void init( std::string name ) override {
			value = g_Vars.lua.Add<int>( name.c_str( ), this->min );
		}

		itemType_t get_type( ) override {
			return SLIDER;
		}

		int get( ) {
			if( !value )
				return 0;

			return *value;
		}

		void set( int input ) {
			if( !value )
				return;

			*value = input;
		}

	public:
		int* value = nullptr;
		int max = 1;
		int min = 0;
	};

	class sliderFloatItem : public uiItem {
	public:
		void init( std::string name ) override {
			value = g_Vars.lua.Add<float>( name.c_str( ), this->min );
		}

		itemType_t get_type( ) override {
			return SLIDER_FLOAT;
		}

		float get( ) {
			if( !value )
				return 0;

			return *value;
		}

		void set( float input ) {
			if( !value )
				return;

			*value = input;
		}

	public:
		float* value = nullptr;
		float max = 1;
		float min = 0;
	};

	class labelItem : public uiItem {
	public:
		void init( std::string name ) override {

		}

		itemType_t get_type( ) override {
			return LABEL;
		}
	};

	class buttonItem : public uiItem {
	public:
		void init( std::string name ) override {

		}

		itemType_t get_type( ) override {
			return BUTTON;
		}

		void add_callback( sol::protected_function function ) {
			if( function.valid( ) )
				m_functions.push_back( function );
		}

		std::vector< sol::protected_function > get_functions( ) {
			return m_functions;
		}

	private:
		std::vector< sol::protected_function > m_functions;
	};

	class textBoxItem : public uiItem {
	public:
		void init( std::string name ) override {
			value = g_Vars.lua.Add<std::string>( name.c_str( ) );
		}

		itemType_t get_type( ) override {
			return TEXTBOX;
		}

		std::string get( ) {
			if( !value )
				return XorStr( "error" );

			return *value;
		}

		//void set( const std::string& input ) {
		//	if( !value )
		//		return;

		//	*value = input;
		//}

	public:
		std::string *value;
	};

	struct shot_miss_t {
		int id;
		int target_index;
		int hitchance;
		int hitgroup;
		int damage;
		std::string reason = XorStr( "unknown" );
	};

	struct shot_fire_t {
		int id;
		int target_index;
		int hitchance;
		int hitgroup;
		int damage;
		int bt;
		bool complex_safe;
		bool basic_safe;
	};

	struct luaState_t {
		sol::state state;
		std::vector<std::pair<uint32_t, sol::protected_function>> callbacks;
		std::vector<std::pair<std::string, uiItem*>> ui_items;
		std::string script_name;
	};

	extern std::vector<std::pair<std::string, std::shared_ptr<luaState_t>>> states;

	namespace Script {

		extern void AddCallback( std::string name, sol::protected_function callback, sol::this_state state );
		extern void RemoveCallback( std::string name, sol::protected_function callback, sol::this_state state );

		inline bool loadingscript = false;
		inline bool in_callback = false;
		inline bool in_paint_callback = false;
		extern bool LoadScript( std::string script_name, std::string &script_data, int owner_id = 0, const std::string& scriptId = XorStr( "" ), bool test = false );
		extern void UnloadScript( std::string name );
		extern bool IsScriptLoaded( std::string name );

		template< typename ...Ts>
		inline void DoCallback( uint32_t hash, Ts... args ) {
			if( loadingscript )
				return;

			in_callback = true;

			auto temp = states;

			for( auto& states : temp ) {
				if( !states.first.empty( ) && states.second != nullptr ) {
					for( auto callbacks : states.second->callbacks ) {
						if( callbacks.first != hash )
							continue;

						if( callbacks.second.lua_state( ) == nullptr )
							continue;

						if( callbacks.first == hash_32_fnv1a_const( XorStr( "paint" ) ) )
							in_paint_callback = true;

						sol::safe_function_result result = callbacks.second.call( args... );

						in_paint_callback = false;

						if( result.valid( ) )
							continue;

						sol::error err = result;
						g_EventLog.PushEvent( err.what( ), Color_f( 1.f, 0.f, 0.f ), true, XorStr( "LUA" ) );

						UnloadScript( states.first );
					}
				}
			}

			in_callback = false;
		}
	}


	inline std::vector<std::pair<uint32_t, uiItem*>> gui_items;
	extern void init( );
	extern std::vector<std::string> GetScripts( );
	extern void OpenFolder( );
	extern void ReloadActiveScripts( );
	extern void UnloadActiveScripts( );
#endif
	
}