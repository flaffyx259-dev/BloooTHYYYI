#pragma once
#include <stack>
#include <optional>

#include "../../Utils/InputSys.hpp"
#include "../../SDK/sdk.hpp"
#include "../Helpers/InputHelper.h"

/* Prototypes */
struct MultiItem_t {
	std::string name;
	bool *value;
};

/* Context structs */
struct DropdownInfo_t {
	size_t HashedID = 0;

	std::map<size_t, float> uScroll;
	std::map<size_t, float> flAnimation;
	bool bOpen = true;

	std::vector<std::string> Elements;
	int *Option = nullptr;

	int MaxItems = 10;

	bool DraggingScroll = false;

	float Size = 0.f;
	Vector2D Pos = { 0, 0 };
};

struct MultiDropdownInfo_t {
	size_t HashedID = 0;

	std::map<size_t, float> uScroll;
	std::map<size_t, float> flAnimation;
	bool bOpen = true;

	std::vector<MultiItem_t> Elements = {};

	int MaxItems = 10;
	bool DraggingScroll = false;

	float Size = 0.f;
	Vector2D Pos = { 0, 0 };
};

struct ColorPickerInfo_t {
	size_t HashedID = 0;

	Vector2D vecBasePos = { 0, 0 };
	Vector2D vecPos = { 0, 0 };

	Color_f *fColor = nullptr;

	bool bRenderAlpha = true;

	bool bPickingAlpha = false;
	bool bPickingColor = false;
	bool bPickingHue = false;

	bool bFirstColorInit = true;

	bool bRightClicked = false;
	bool bPastingColor = false;
	bool bCopyingColor = false;

	bool bOpen = true;
	std::unordered_map<size_t, float> flAnimation;
};

struct ConfigInfo_t {
	size_t HashedID = 0;

	Vector2D vecPos = { 0, 0 };

	bool bCopyingFiggy = false;
	std::function<void( )> fnCopy;

	bool bPastingFiggy = false;
	std::function<void( )> fnPaste;

	std::string szText = {};
	CVariables::RAGE *pConfig = nullptr;

	bool bOpen = true;
	std::unordered_map<size_t, float> flAnimation;
};

struct HotkeyInfo_t {
	size_t HashedID = 0;

	Vector2D vecPos = { 0, 0 };

	hotkey_t *pHotkey = nullptr;

	std::map<size_t, float> flAnimation;
	bool bLeftClicked = true;
};

struct SliderInfo_t {
	std::map<size_t, float> ValueAnimation;
	std::map<size_t, float> ValueTimer;

	std::map<size_t, float> PreviewAnimation;
	std::map<size_t, float> PreviousAmount;

	std::map<size_t, float> LastChangeTime;

	std::map<size_t, std::pair<float, bool>> ShouldChangeValue;
};

struct PopupInfo_t {
	size_t HashedID = 0;

	Vector2D vecCenterPosition;

	size_t ActiveButton = 0;
	bool bOpen = false;
	float flAnimation = 0.f;

	std::string szTitle;
	std::string szInnerString;
	bool bHasConfirmButton;

	std::function<void( )> fnFunction;
};
#define MENU_SIZE_X 600
#define MENU_SIZE_Y 520

// fwd decl
struct MenuContext_t;
struct Windows_t {
	//static int m_nIndex;
	MenuContext_t *m_pContext;
	std::function<void( )> m_fnRender;
};

inline std::vector<Windows_t> m_vecWindows;

/* "Object" structs */
struct MenuContext_t {
	// determine which window should be drawn on top
	float flLastActivity;
	bool bHoveringWindow;
	bool bActiveWindow = true;
	int iLastPressed;

	//static int nIndex;

	Vector2D pos = { 100, 100 };
	Vector2D size = { MENU_SIZE_X, MENU_SIZE_Y };

	MenuContext_t( Vector2D _pos, Vector2D _size ) {
		size = _size;
		pos = _pos;

		/*Windows_t _this;
		_this.m_nIndex = nIndex;
		_this.m_pContext = this;

		m_vecWindows.push_back( _this );*/

		//nIndex++;
	}

	Vector2D NextGroupPos;
	std::stack< Vector2D > CursorPosStack;

	Vector2D NextGroupAdj;

	std::vector< std::pair<std::string, std::string> > Tabs;
	int ActiveTab = 0;

	std::map<int, std::vector< std::string >> SubTabs;
	std::map<int, int> ActiveSubTab;

	DropdownInfo_t DropdownInfo;
	MultiDropdownInfo_t MultiDropdownInfo;
	ColorPickerInfo_t ColorPickerInfo;
	HotkeyInfo_t HotkeyInfo;
	ConfigInfo_t ConfigInfo;
	SliderInfo_t SliderInfo;
	PopupInfo_t PopupInfo;

	Vector2D ParentPos;
	Vector2D ParentSize;

	std::string parent;
	size_t FocusedID = 0;

	std::string CurrentTab;
	std::string CurrentSubTab;
	std::string CurrentGroup;
	std::string CurrentWeaponGroup;

	bool dragging = false;
	bool resizing = false;
	bool setup = true;
	bool typing = false;
	bool hovered_listbox = false;
	bool dragging_scrollbar = false;
	bool set_cursor = false;
	HCURSOR override_cursor = 0;
	bool enabled = true;

	bool allow_tooltip = false;
	std::pair<std::string, std::string> tooltip = { };

	float animation = 0.f;

	std::string szLastElementName;
};

namespace GUI {

	// current ctx being user
	inline MenuContext_t *ctx = new MenuContext_t( Vector2D( 100, 100 ), Vector2D( MENU_SIZE_X, MENU_SIZE_Y ) );

	inline MenuContext_t *pMenu = new MenuContext_t( Vector2D( 100, 100 ), Vector2D( MENU_SIZE_X, MENU_SIZE_Y ) );
	inline MenuContext_t *pSkinchanger = new MenuContext_t( Vector2D( 100, 100 ), Vector2D( 530, 530 ) );

	// tell UI which context menu to use
	inline void SetContext( MenuContext_t *context ) {
		ctx = context;
	}

	// modify this if you'd like to change the spacing between each object
	inline int ObjectPadding( ) {
		constexpr int spacing{ 4 };

		return spacing + 1;
	};

	void SetNextTooltip( std::string tooltip, std::string highlighted );
	bool ContextMenuOpen( );
	Vector2D PopCursorPos( );
	Vector2D GetLastCursorPos( );
	void PushCursorPos( const Vector2D &cursor_pos );
	void CopyReference( bool bForceCantReference = false );

	void AdjustGroupPos( const Vector2D &cursor_pos );

	size_t Hash( const std::string &name );

	float AnimationInterval( float interval = 0.035f );

	template < typename T = float >
	inline float Approach( float a, float b, float multiplier ) {
		multiplier = std::clamp( multiplier, 0.f, 1.f );

		float ret = ( a + static_cast< T >( multiplier * ( b - a ) ) );

		// some weird floating-point precision stuff causing anims
		// to take a long(er) time to fully finish, speed her up :)
		if( fabs( ret - b ) < .0075f )
			ret = b;

		//if( b > 0.f ) {
		//	if( ret > b )
		//		ret = b;
		//} else {
		//	if( ret < b )
		//		ret = b;
		//}

		return std::max( ret, 0.f );
	}

	void DrawArrow( Vector2D vecPos, Color clr, bool bFaceDown = true );
	float MapNumber( float input, float input_min, float input_max, float output_min, float output_max );
	std::vector<std::string> SplitStr( const std::string &str, char separator );
}

__forceinline void MemeCopy( const std::string &actualFiggy ) {
	if( OpenClipboard( nullptr ) ) {
		EmptyClipboard( );
		HGLOBAL clipboard_buffer = GlobalAlloc( GMEM_DDESHARE, actualFiggy.size( ) + 1 );
		char *buffer = ( char * )GlobalLock( clipboard_buffer );
		if( buffer ) {
			strcpy( buffer, actualFiggy.c_str( ) );
		}

		GlobalUnlock( clipboard_buffer );
		SetClipboardData( CF_TEXT, clipboard_buffer );
		CloseClipboard( );
	}
}

__forceinline std::string MemePaste( ) {
	// bruh this better be the right format...
	if( IsClipboardFormatAvailable( CF_TEXT ) ) {
		if( OpenClipboard( nullptr ) ) {
			std::string input( ( char * )GetClipboardData( CF_TEXT ) );
			CloseClipboard( );
			return input;
		}
	}
}

__forceinline bool IsPandoraFiggy( const char *decoded_string ) {
	return !( decoded_string[ 0 ] != '(' ||
			  decoded_string[ 1 ] != 'p' ||
			  decoded_string[ 2 ] != 'd' ||
			  decoded_string[ 3 ] != 'r' ||
			  decoded_string[ 4 ] != ')' ||
			  decoded_string[ 5 ] != ' ' ||
			  decoded_string[ 6 ] != '-' ||
			  decoded_string[ 7 ] != ' ' );
};

#include "form/form.h"
#include "group/group.h"
#include "checkbox/checkbox.h"
#include "button/button.h"
#include "label/label.h"
#include "slider/slider.h"
#include "dropdown/dropdown.h"
#include "hotkey/hotkey.h"
#include "colorpicker/colorpicker.h"
#include "multidropdown/multidropdown.h"
#include "Listbox/Listbox.h"
#include "Textbox/Textbox.h"