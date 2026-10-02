#pragma once
#include <string>
#include <sstream>

#include "../SDK/sdk.hpp"
#include "../SDK/Valve/vector4d.hpp"

#include <d3d9.h>
#include <d3dx9.h>
#pragma comment(lib, "d3dx9.lib")

using TextureID = void*;
using ColorU32 = uint32_t;
using Rect2D = Vector4D;
using FontHandle = std::size_t;

enum text_flags : int {
	CENTER_X = ( 1 << 0 ),
	CENTER_Y = ( 1 << 1 ),
	ALIGN_RIGHT = ( 1 << 2 ),
	ALIGN_BOTTOM = ( 1 << 3 ),
	DROP_SHADOW = ( 1 << 4 ),
	OUTLINED = ( 1 << 5 ),
};

enum : uint32_t {
	FONT_VERDANA = 0,
	FONT_MENU_BOLD = FONT_VERDANA,
	FONT_MENU = FONT_VERDANA,
	FONT_CSGO_ICONS,
	FONT_VERDANA_30_BOLD,
	FONT_VERDANA_25_REGULAR,
	FONT_VISITOR,
	FONT_PORTER,
	FONT_CSGO_ICONS2,
	FONT_VERDANA_40_BOLD,
};

enum ECorner {
	CORNER_NONE = 0,

	CORNER_TOP_LEFT = 1 << 0,
	CORNER_TOP_RIGHT = 1 << 1,
	CORNER_BOTTOM_LEFT = 1 << 2,
	CORNER_BOTTOM_RIGHT = 1 << 3,

	CORNER_TOP = CORNER_TOP_LEFT | CORNER_TOP_RIGHT,
	CORNER_RIGHT = CORNER_TOP_RIGHT | CORNER_BOTTOM_RIGHT,
	CORNER_BOTTOM = CORNER_BOTTOM_LEFT | CORNER_BOTTOM_RIGHT,
	CORNER_LEFT = CORNER_TOP_LEFT | CORNER_BOTTOM_LEFT,

	CORNER_ALL = CORNER_TOP | CORNER_RIGHT | CORNER_BOTTOM | CORNER_LEFT,
};

enum ETextures : int {
	RAGE,
	PLAYERS,
	WORLD,
	MISC,
	SCRIPT,
	HOME,
	GRADIENT,
	SPREAD_REGULAR,
	SPREAD_RAINBOW,
	MAX,
};

namespace Render {
	namespace Engine {
		struct FontSize_t {
			int m_width;
			int m_height;
		};

		enum StringFlags_t {
			ALIGN_LEFT = 0,
			ALIGN_RIGHT,
			ALIGN_CENTER
		};

		class Font {
		public:
			HFont      m_handle;
			FontSize_t m_size;
			bool m_outline;

		public:
			__forceinline Font( ) : m_handle{ }, m_size{ }, m_outline{ }{};

			Font( const std::string& name, int s, int w, int flags, bool outline = false );
			Font( HFont font );

			void string( int x, int y, Color color, const std::string& text, StringFlags_t flags = ALIGN_LEFT );
			void lua_string( int x, int y, Color color, const std::string& text );
			//void string( int x, int y, Color color, const std::stringstream& text, StringFlags_t flags = ALIGN_LEFT );
			void wstring( int x, int y, Color color, const std::wstring& text, StringFlags_t flags = ALIGN_LEFT );
			FontSize_t size( const std::string& text );
			FontSize_t wsize( const std::wstring& text );
			std::pair<float, float> lua_size( const std::string& text );
		};

		extern Font esp_pixel;
		extern Font esp_bold;
		extern Font esp_bold_wpn;
		extern Font esp_indicator;
		extern Font console;
		extern Font hud;
		extern Font segoe;
		extern Font cs;
		extern Font cs_large;
		extern Font watermark;
		extern Font speclist;

		extern Font menu_regular;
		extern Font menu_small;
		extern Font menu_title;
		extern Font menu_bold;

		extern int m_width;
		extern int m_height;
		extern bool initialized;

		void Initialise( );
		void InitFonts( );
		void Invalidate( );
		bool WorldToScreen( const Vector& world, Vector2D& screen );
		void Line( Vector2D v0, Vector2D v1, Color color );
		void Polygon( int count, Vertex_t* vertices, const Color& col );
		void Polyline( int count, Vertex_t *vertices, const Color &col );
		void FilledTriangle( const Vector2D& pos1, const Vector2D& pos2, const Vector2D& pos3, const Color& col );
		
		// CZAPEK START
		void RoundedRect( const Vector2D& pos, const Vector2D& size, const int radius, const int corners, const Color color );
		void RoundedRect( const Vector2D& pos, const Vector2D& size, const int radius, const Color color );
		void FilledRoundedRect( const Vector2D& pos, const Vector2D& size, const int radius, const int corners, const Color color );
		void FilledRoundedRect( const Vector2D& pos, const Vector2D& size, const int radius, const Color color );
		void GradientRoundedHorizontal( const Vector2D& pos, const Vector2D& size, const int radius, const int corners, const Color color, const Color color1 );
		void GradientRoundedVertical( const Vector2D& pos, const Vector2D& size, const int radius, const int corners, const Color color, const Color color1 );
		void Texture( const Vector2D& pos, const Vector2D& size, const ETextures texture, const Color tint );
		// CZAPEK END

		void SetClip( const Vector2D& pos, const Vector2D& size );
		void ResetClip( );
		void WorldCircle( Vector origin, float radius, Color color, Color colorFill = { } );
		void Line( int x0, int y0, int x1, int y1, Color color );
		void Rect( int x, int y, int w, int h, Color color );
		void RectFilled( int x, int y, int w, int h, Color color );
		void RectFilled( Vector2D pos, Vector2D size, Color color );
		void Rect( Vector2D pos, Vector2D size, Color color );
		void RectOutlined( int x, int y, int w, int h, Color color, Color color2 );
		void CircleFilled( int x, int y, float radius, int segments, Color color );
		void Circle( int x, int y, float radius, int segments, Color color );
		void Gradient( int x, int y, int w, int h, Color color, Color color2, bool horizontal = false );
		void Gradient( Vector2D pos, Vector2D size, Color color, Color color2, bool horizontal );
		void RotateTriangle( std::array<Vector2D, 3> &points, float rotation );
	}

	Vector2D GetScreenSize( );
};

