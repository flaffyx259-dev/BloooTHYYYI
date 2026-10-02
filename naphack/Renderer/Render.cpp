#define NOMINMAX
#define STB_IMAGE_IMPLEMENTATION

#include "render.hpp"
#include <algorithm>
#include <mutex>

#include <unordered_map>
#include "../pandora.hpp"

#include "../Utils/extern/XorStr.hpp"

#include "Textures/weaponicons.h"
#include "Textures/checkmark.h"
#include "Textures/indicators.h"

#include "../SDK/Displacement.hpp"

#include "../Utils/LogSystem.hpp"

#define STBI_ASSERT(x)

#include <stb_image.h>

#pragma region EngineRender

std::array<uint32_t, static_cast< int >( ETextures::MAX )> textures;
uint32_t &GetTexture( const ETextures texture ) {
	return textures[ static_cast< int >( texture ) ];
}

void MakeTextureFromImage( const ETextures texture, const uint8_t *png_data, size_t size, const int32_t width, const int32_t height ) {
	int image_width, image_height, channels;

	stbi_set_flip_vertically_on_load( false );
	stbi_set_flip_vertically_on_load_thread( false );
	const auto image_data = stbi_load_from_memory( png_data, size, &image_width, &image_height, &channels, 4 );

	if( image_data == nullptr )
		return;

	g_pSurface->UpdateTexture( GetTexture( texture ), image_data, image_width, image_height );
	stbi_image_free( image_data );
}

// ctor.
Render::Engine::Font::Font( const std::string &name, int s, int w, int flags, bool outline ) {
	m_handle = g_pSurface->CreateFont_( );

	const auto actual_lang = g_pFontManager->get_language( );

	m_outline = outline;

	g_pFontManager->get_language( ) = XorStr( "english" );
	g_pSurface->SetFontGlyphSet( m_handle, name.data( ), s, w, 0, 0, flags );
	g_pFontManager->get_language( ) = actual_lang;

	m_size = size( XorStr( "A" ) );
}

// ctor.
Render::Engine::Font::Font( HFont font ) {
	m_outline = false;
	m_handle = font;
	m_size = size( XorStr( "A" ) );
}

void Render::Engine::Font::string( int x, int y, Color color, const std::string &text, StringFlags_t flags /*= Render::DirectX::ALIGN_LEFT */ ) {
	wstring( x, y, color, Math::MultiByteToWide( text ), flags );
}

void Render::Engine::Font::lua_string( int x, int y, Color color, const std::string &text ) {
	wstring( x, y, color, Math::MultiByteToWide( text ), Render::Engine::ALIGN_LEFT );
}

//void Render::Engine::Font::string( int x, int y, Color color, const std::stringstream& text, StringFlags_t flags /*= Render::DirectX::ALIGN_LEFT */ ) {
//	wstring( x, y, color, Math::MultiByteToWide( text.str( ) ), flags );
//}

void Render::Engine::Font::wstring( int x, int y, Color color, const std::wstring &text, StringFlags_t flags /*= Render::DirectX::ALIGN_LEFT */ ) {
	int w, h;

	auto xd = [ & ] ( int xx, int yy, bool b = false, float mult = 0.85f ) {
		g_pSurface->GetTextSize( m_handle, text.c_str( ), w, h );
		g_pSurface->DrawSetTextFont( m_handle );
		g_pSurface->DrawSetTextColor( b ? Color( 0, 0, 0, color.a( ) * mult ) : color );

		if( flags & Render::Engine::ALIGN_RIGHT )
			xx -= w;

		if( flags & Render::Engine::ALIGN_CENTER )
			xx -= w / 2;

		g_pSurface->DrawSetTextPos( xx, yy );
		g_pSurface->DrawPrintText( text.c_str( ), ( int )text.size( ) );
	};

	// trust the process
	if( m_outline ) {
		/*xd( x - 1, y, true, 0.7f );
		xd( x + 1, y, true, 0.89f );
		xd( x, y - 1, true, 0.6f );
		xd( x, y + 1, true, 0.89f );

		xd( x + 1, y + 1, true, 0.6f );*/

		xd( x - 1, y, true, 0.4f );
		xd( x + 1, y, true, 0.79f );
		xd( x, y - 1, true, 0.4f );
		xd( x, y + 1, true, 0.79f );
		xd( x - 1, y - 1, true, 0.22f );
		xd( x - 1, y + 1, true, 0.22f );
		xd( x + 1, y - 1, true, 0.3f );
		xd( x + 1, y + 1, true, 0.6f );
	}

	xd( x, y );
}

Render::Engine::FontSize_t Render::Engine::Font::size( const std::string &text ) {
	if( text.empty( ) )
		return {};

	return wsize( Math::MultiByteToWide( text ) );
}

Render::Engine::FontSize_t Render::Engine::Font::wsize( const std::wstring &text ) {
	if( text.empty( ) )
		return {};

	FontSize_t res;
	g_pSurface->GetTextSize( m_handle, text.data( ), res.m_width, res.m_height );
	return res;
}

std::pair<float, float> Render::Engine::Font::lua_size( const std::string &text ) {
	auto text_size = size( text );
	return std::make_pair( text_size.m_width, text_size.m_height );
}

namespace Render::Engine {
	Font esp_pixel;
	Font esp_bold;
	Font esp_bold_wpn;
	Font esp_indicator;
	Font console;
	Font hud;
	Font segoe;
	Font cs;
	Font cs_large;
	Font watermark;
	Font speclist;

	Font menu_regular;
	Font menu_small;
	Font menu_title;
	Font menu_bold;

	int m_width;
	int m_height;
	bool initialized;
}

void LoadFontFromResource( char *arr, const size_t size ) {
	DWORD n_fonts;
	AddFontMemResourceEx( arr, size, nullptr, &n_fonts );
}

void Render::Engine::Initialise( ) {
	if( initialized )
		return;

	LoadFontFromResource( weaponicons, sizeof( weaponicons ) );
	LoadFontFromResource( robotomedium, sizeof( robotomedium ) );
	LoadFontFromResource( visitor_font, sizeof( visitor_font ) );
	LoadFontFromResource( smallest_pixel_font, sizeof( smallest_pixel_font ) );

	InitFonts( );

	g_pSurface->GetScreenSize( m_width, m_height );

	for( int texture = 0; texture < ETextures::MAX; ++texture ) {
		GetTexture( ( ETextures )texture ) = g_pSurface->CreateNewTextureID( true );
	}

	MakeTextureFromImage( ETextures::RAGE, rage_icon, sizeof( rage_icon ), 21, 21 );
	MakeTextureFromImage( ETextures::PLAYERS, visuals_icon, sizeof( visuals_icon ), 21, 21 );
	MakeTextureFromImage( ETextures::WORLD, world_icon, sizeof( world_icon ), 21, 21 );
	MakeTextureFromImage( ETextures::MISC, misc_icon, sizeof( misc_icon ), 21, 21 );
	MakeTextureFromImage( ETextures::SCRIPT, script_icon, sizeof( script_icon ), 21, 21 );
	MakeTextureFromImage( ETextures::HOME, home_icon, sizeof( home_icon ), 21, 21 );
	MakeTextureFromImage( ETextures::GRADIENT, blackGradient, sizeof( blackGradient ), 171, 171 );
	MakeTextureFromImage( ETextures::SPREAD_REGULAR, spread_crosshair, sizeof( spread_crosshair ), 1024, 1024 );
	MakeTextureFromImage( ETextures::SPREAD_RAINBOW, rainbow_crosshair, sizeof( rainbow_crosshair ), 1024, 1024 );

	initialized = true;
}

void Render::Engine::InitFonts( ) {
	esp_pixel = Font( XorStr( "Smallest Pixel-7" ), 11, FW_NORMAL, FONTFLAG_NONE, true );
	esp_bold = Font( XorStr( "Visitor TT2 BRK" ), 9, FW_NORMAL, FONTFLAG_NONE, true );
	esp_bold_wpn = Font( XorStr( "Visitor TT2 BRK" ), 9, FW_NORMAL, FONTFLAG_NONE, true );
	segoe = Font( XorStr( "Segoe UI" ), 13, FW_NORMAL, FONTFLAG_ANTIALIAS | FONTFLAG_DROPSHADOW );
	console = Font( XorStr( "Lucida Console" ), 10, FW_DONTCARE, FONTFLAG_DROPSHADOW );
	hud = Font( XorStr( "Verdana" ), 12, FW_NORMAL, FONTFLAG_ANTIALIAS | FONTFLAG_DROPSHADOW );
	cs = Font( XorStr( "WeaponIcons" ), 14, FW_NORMAL, FONTFLAG_ANTIALIAS | FONTFLAG_DROPSHADOW );
	cs_large = Font( XorStr( "WeaponIcons" ), 20, FW_NORMAL, FONTFLAG_ANTIALIAS | FONTFLAG_DROPSHADOW );
	esp_indicator = Font( XorStr( "Arial" ), 21, 800, FONTFLAG_ANTIALIAS );
	watermark = Font( XorStr( "Tahoma" ), 12, 400, FONTFLAG_DROPSHADOW );

	speclist = Font( XorStr( "Courier New" ), 14, FW_NORMAL, FONTFLAG_OUTLINE );
	menu_regular = Font( XorStr( "Verdana" ), 13, FW_NORMAL, FONTFLAG_NONE );
	menu_bold = Font( XorStr( "Visitor TT2 BRK" ), 9, FW_NORMAL, FONTFLAG_NONE, true );
	menu_small = Font( XorStr( "Verdana" ), 13, FW_NORMAL, FONTFLAG_NONE );
	menu_title = Font( XorStr( "Calibri" ), 15, FW_BOLD, FONTFLAG_ANTIALIAS );
}

void Render::Engine::Invalidate( ) {
	initialized = false;
}

bool Render::Engine::WorldToScreen( const Vector &world, Vector2D &screen ) {
	float w;
	static ptrdiff_t ptrViewMatrix;
	if( !ptrViewMatrix ) {
		ptrViewMatrix = static_cast< ptrdiff_t >( ::Engine::Displacement.Data.m_uViewMatrix );
		ptrViewMatrix += 0x3;
		ptrViewMatrix = *reinterpret_cast< uintptr_t * >( ptrViewMatrix );
		ptrViewMatrix += 176;
	}

	const VMatrix &matrix = *( VMatrix * )ptrViewMatrix;

	// check if it's in view first.
	// note - dex; w is below 0 when world position is around -90 / +90 from the player's camera on the y axis.
	w = matrix[ 3 ][ 0 ] * world.x + matrix[ 3 ][ 1 ] * world.y + matrix[ 3 ][ 2 ] * world.z + matrix[ 3 ][ 3 ];
	if( w < 0.001f )
		return false;

	// calculate x and y.
	screen.x = matrix[ 0 ][ 0 ] * world.x + matrix[ 0 ][ 1 ] * world.y + matrix[ 0 ][ 2 ] * world.z + matrix[ 0 ][ 3 ];
	screen.y = matrix[ 1 ][ 0 ] * world.x + matrix[ 1 ][ 1 ] * world.y + matrix[ 1 ][ 2 ] * world.z + matrix[ 1 ][ 3 ];

	screen /= w;

	// calculate screen position.
	screen.x = ( m_width / 2 ) + ( screen.x * m_width ) / 2;
	screen.y = ( m_height / 2 ) - ( screen.y * m_height ) / 2;

	return true;
}

void Render::Engine::Line( Vector2D v0, Vector2D v1, Color color ) {
	g_pSurface->DrawSetColor( color );
	g_pSurface->DrawLine( v0.x, v0.y, v1.x, v1.y );
}

void Render::Engine::Polygon( int count, Vertex_t *vertices, const Color &col ) {
	static int texture_id;

	if( !g_pSurface->IsTextureIDValid( texture_id ) )
		texture_id = g_pSurface->CreateNewTextureID( );

	g_pSurface->DrawSetColor( col.RGBA[ 0 ], col.RGBA[ 1 ], col.RGBA[ 2 ], col.RGBA[ 3 ] );
	g_pSurface->DrawSetTexture( texture_id );
	g_pSurface->DrawTexturedPolygon( count, vertices );
}

void Render::Engine::Polyline( int count, Vertex_t *vertices, const Color &col ) {
	static int texture_id;

	if( !g_pSurface->IsTextureIDValid( texture_id ) )
		texture_id = g_pSurface->CreateNewTextureID( );

	g_pSurface->DrawSetColor( col.RGBA[ 0 ], col.RGBA[ 1 ], col.RGBA[ 2 ], col.RGBA[ 3 ] );
	g_pSurface->DrawSetTexture( texture_id );
	g_pSurface->DrawTexturedPolyLine( vertices, count );
}

void Render::Engine::FilledTriangle( const Vector2D &pos1, const Vector2D &pos2, const Vector2D &pos3, const Color &col ) {
	static Vertex_t triangle_vert[ 3 ];

	triangle_vert[ 0 ].Init( pos1 );
	triangle_vert[ 1 ].Init( pos2 );
	triangle_vert[ 2 ].Init( pos3 );

	Polygon( 3, triangle_vert, col );
}

void Render::Engine::RoundedRect( const Vector2D &pos, const Vector2D &size, const int radius, const int corners, const Color color ) {
	const auto radius_f = static_cast< float >( radius );
	const auto round_top_left = ( corners & CORNER_TOP_LEFT ) != 0;
	const auto round_top_right = ( corners & CORNER_TOP_RIGHT ) != 0;
	const auto round_bottom_left = ( corners & CORNER_BOTTOM_LEFT ) != 0;
	const auto round_bottom_right = ( corners & CORNER_BOTTOM_RIGHT ) != 0;

	std::vector<Vertex_t> vertices;

	vertices.reserve( ( round_top_left ? 6 : 1 ) + ( round_top_right ? 6 : 1 ) + ( round_bottom_left ? 6 : 1 ) + ( round_bottom_right ? 6 : 1 ) );

	for( auto i = 0; i < 4; i++ ) {
		const auto round_corner = i == 0 && round_top_right || i == 1 && round_bottom_right || i == 2 && round_bottom_left || i == 3 && round_top_left;

		if( !round_corner ) {
			vertices.emplace_back( Vector2D( static_cast< float >( i < 2 ? pos.x + size.x : pos.x ), static_cast< float >( i % 3 ? pos.y + size.y : pos.y ) ), 0.0f );

			continue;
		}

		const auto vert_x = static_cast< float >( pos.x + ( i < 2 ? size.x - radius : radius ) );
		const auto vert_y = static_cast< float >( pos.y + ( i % 3 ? size.y - radius : radius ) );

		for( auto j = 0; j < 6; j++ ) {
			const auto angle = DEG2RAD( 90.0f * i + 15.0f * j );

			vertices.emplace_back( Vector2D( vert_x + radius_f * std::sin( angle ), vert_y - radius_f * std::cos( angle ) ), 0.0f );
		}
	}

	static int texture;
	if( !g_pSurface->IsTextureIDValid( texture ) )
		texture = g_pSurface->CreateNewTextureID( true );

	//m_pSurface->DrawSetTextureRGBA( texture, Color::White( ).RGBA, 1, 1 );
	g_pSurface->DrawSetTexture( texture );

	g_pSurface->DrawSetColor( color );
	g_pSurface->DrawTexturedPolyLine( vertices.data( ), vertices.size( ) );
}

void Render::Engine::RoundedRect( const Vector2D &pos, const Vector2D &size, const int radius, const Color color ) {
	RoundedRect( pos, size - 1, radius, CORNER_ALL, color );
}

void Render::Engine::FilledRoundedRect( const Vector2D &pos, const Vector2D &size, const int radius, const int corners, const Color color ) {
	const auto radius_f = static_cast< float >( radius );
	const auto round_top_left = ( corners & CORNER_TOP_LEFT ) != 0;
	const auto round_top_right = ( corners & CORNER_TOP_RIGHT ) != 0;
	const auto round_bottom_left = ( corners & CORNER_BOTTOM_LEFT ) != 0;
	const auto round_bottom_right = ( corners & CORNER_BOTTOM_RIGHT ) != 0;

	std::vector<Vertex_t> vertices;

	vertices.reserve( ( round_top_left ? 6 : 1 ) + ( round_top_right ? 6 : 1 ) + ( round_bottom_left ? 6 : 1 ) + ( round_bottom_right ? 6 : 1 ) );

	for( auto i = 0; i < 4; i++ ) {
		const auto round_corner = i == 0 && round_top_right || i == 1 && round_bottom_right || i == 2 && round_bottom_left || i == 3 && round_top_left;

		if( !round_corner ) {
			vertices.emplace_back( Vector2D( static_cast< float >( i < 2 ? pos.x + size.x : pos.x ), static_cast< float >( i % 3 ? pos.y + size.y : pos.y ) ), 0.0f );

			continue;
		}

		const auto vert_x = static_cast< float >( pos.x + ( i < 2 ? size.x - radius : radius ) );
		const auto vert_y = static_cast< float >( pos.y + ( i % 3 ? size.y - radius : radius ) );

		for( auto j = 0; j < 6; j++ ) {
			const auto angle = DEG2RAD( 90.0f * i + 15.0f * j );

			vertices.emplace_back( Vector2D( vert_x + radius_f * std::sin( angle ), vert_y - radius_f * std::cos( angle ) ), 0.0f );
		}
	}

	static int texture;
	if( !g_pSurface->IsTextureIDValid( texture ) )
		texture = g_pSurface->CreateNewTextureID( true );

	//m_pSurface->DrawSetTextureRGBA( texture, Color::White( ).RGBA, 1, 1 );
	g_pSurface->DrawSetTexture( texture );

	g_pSurface->DrawSetColor( color );
	g_pSurface->DrawTexturedPolygon( vertices.size( ), vertices.data( ) );
}

void Render::Engine::FilledRoundedRect( const Vector2D &pos, const Vector2D &size, const int radius, const Color color ) {
	FilledRoundedRect( pos, size - 1, radius, CORNER_ALL, color );
}

void Render::Engine::GradientRoundedHorizontal( const Vector2D &pos, const Vector2D &size, const int radius, const int corners, const Color color, const Color color1 ) {
	const auto round_top_left = ( corners & CORNER_TOP_LEFT ) != 0;
	const auto round_top_right = ( corners & CORNER_TOP_RIGHT ) != 0;
	const auto round_bottom_left = ( corners & CORNER_BOTTOM_LEFT ) != 0;
	const auto round_bottom_right = ( corners & CORNER_BOTTOM_RIGHT ) != 0;

	const int x = pos.x;
	const int y = pos.y;
	const int w = size.x;
	const int h = size.y;

	round_top_left
		? FilledRoundedRect( Vector2D( x, y ), Vector2D( radius, radius ), radius, CORNER_TOP_LEFT, color )
		: RectFilled( x, y, radius, radius, color );
	round_top_right
		? FilledRoundedRect( Vector2D( x + w - radius, y ), Vector2D( radius, radius ), radius, CORNER_TOP_RIGHT, color1 )
		: RectFilled( x + w - radius, y, radius, radius, color1 );
	round_bottom_left
		? FilledRoundedRect( Vector2D( x, y + h - radius ), Vector2D( radius, radius ), radius, CORNER_BOTTOM_LEFT, color )
		: RectFilled( x, y + h - radius, radius, radius, color );
	round_bottom_right
		? FilledRoundedRect( Vector2D( x + w - radius, y + h - radius ), Vector2D( radius, radius ), radius, CORNER_BOTTOM_RIGHT, color1 )
		: RectFilled( x + w - radius, y + h - radius, radius, radius, color1 );

	RectFilled( x, y + radius, radius, h - radius * 2, color );
	RectFilled( x + w - radius, y + radius, radius, h - radius * 2, color1 );

	Render::Engine::Gradient( x + radius, y, w - radius * 2, h, color, color1, true );
}

void Render::Engine::GradientRoundedVertical( const Vector2D &pos, const Vector2D &size, const int radius, const int corners, const Color color, const Color color1 ) {
	const auto round_top_left = ( corners & CORNER_TOP_LEFT ) != 0;
	const auto round_top_right = ( corners & CORNER_TOP_RIGHT ) != 0;
	const auto round_bottom_left = ( corners & CORNER_BOTTOM_LEFT ) != 0;
	const auto round_bottom_right = ( corners & CORNER_BOTTOM_RIGHT ) != 0;

	const int w = size.x;
	const int h = size.y;

	round_top_left
		? FilledRoundedRect( Vector2D( pos.x, pos.y ), Vector2D( radius, radius ), radius, CORNER_TOP_LEFT, color )
		: RectFilled( pos.x, pos.y, radius, radius, color );
	round_top_right
		? FilledRoundedRect( Vector2D( pos.x + w - radius, pos.y ), Vector2D( radius, radius ), radius, CORNER_TOP_RIGHT, color )
		: RectFilled( pos.x + w - radius, pos.y, radius, radius, color );
	round_bottom_left
		? FilledRoundedRect( Vector2D( pos.x, pos.y + h - radius ), Vector2D( radius, radius ), radius, CORNER_BOTTOM_LEFT, color1 )
		: RectFilled( pos.x, pos.y + h - radius, radius, radius, color1 );
	round_bottom_right
		? FilledRoundedRect( Vector2D( pos.x + w - radius, pos.y + h - radius ), Vector2D( radius, radius ), radius, CORNER_BOTTOM_RIGHT, color1 )
		: RectFilled( pos.x + w - radius, pos.y + h - radius, radius, radius, color1 );

	RectFilled( pos.x + radius, pos.y, w - radius * 2, radius, color );
	RectFilled( pos.x + radius, pos.y + h - radius, w - radius * 2, radius, color1 );

	Render::Engine::Gradient( pos.x, pos.y + radius, w, h - radius * 2, color, color1 );
}

void Render::Engine::SetClip( const Vector2D &pos, const Vector2D &size ) {
	g_pSurface->bClippingEnabled = true;
	g_pSurface->SetClipRect( pos.x, pos.y, pos.x + size.x, pos.y + size.y );
}

void Render::Engine::ResetClip( ) {
	SetClip( { 0, 0 }, Vector2D( Render::Engine::m_width, Render::Engine::m_height ) );

	g_pSurface->bClippingEnabled = false;
}

void Render::Engine::WorldCircle( Vector origin, float radius, Color color, Color colorFill ) {
	float step = static_cast< float >( M_PI ) * 2.0f / 128;
	for( float a = 0; a < ( M_PI * 2.0f ); a += step ) {
		Vector start( radius * cosf( a ) + origin.x, radius * sinf( a ) + origin.y, origin.z );
		Vector end( radius * cosf( a + step ) + origin.x, radius * sinf( a + step ) + origin.y, origin.z );

		Vector2D out, out1, pos3d;

		if( Render::Engine::WorldToScreen( end, out1 ) && Render::Engine::WorldToScreen( start, out ) ) {
			if( colorFill.a( ) && Render::Engine::WorldToScreen( origin, pos3d ) ) {
				FilledTriangle( out, out1, pos3d, colorFill );
			}

			Line( out, out1, color );
		}
	}
}

void Render::Engine::Texture( const Vector2D &pos, const Vector2D &size, const ETextures texture, const Color tint ) {
	g_pSurface->DrawSetColor( tint );
	g_pSurface->DrawSetTexture( GetTexture( texture ) );
	g_pSurface->DrawTexturedRect( pos.x, pos.y, pos.x + size.x, pos.y + size.y );
}

void Render::Engine::Line( int x0, int y0, int x1, int y1, Color color ) {
	g_pSurface->DrawSetColor( color );
	g_pSurface->DrawLine( x0, y0, x1, y1 );
}

void Render::Engine::Rect( int x, int y, int w, int h, Color color ) {
	g_pSurface->DrawSetColor( color );
	g_pSurface->DrawOutlinedRect( x, y, x + w, y + h );
}

void Render::Engine::RectFilled( int x, int y, int w, int h, Color color ) {
	g_pSurface->DrawSetColor( color );
	g_pSurface->DrawFilledRect( x, y, x + w, y + h );
}

void Render::Engine::RectFilled( Vector2D pos, Vector2D size, Color color ) {
	Render::Engine::RectFilled( pos.x, pos.y, size.x, size.y, color );
}

void Render::Engine::Rect( Vector2D pos, Vector2D size, Color color ) {
	Render::Engine::Rect( pos.x, pos.y, size.x, size.y, color );
}

void Render::Engine::RectOutlined( int x, int y, int w, int h, Color color, Color color2 ) {
	Rect( x, y, w, h, color );
	Rect( x - 1, y - 1, w + 2, h + 2, color2 );
	Rect( x + 1, y + 1, w - 2, h - 2, color2 );
}

// thanks nitro
void Render::Engine::CircleFilled( int x, int y, float radius, int segments, Color color ) {
	static int texture;
	if( !g_pSurface->IsTextureIDValid( texture ) )
		texture = g_pSurface->CreateNewTextureID( true );

	//m_pSurface->DrawSetTextureRGBA( texture, Color::White( ).RGBA, 1, 1 );
	g_pSurface->DrawSetColor( color );
	g_pSurface->DrawSetTexture( texture );

	std::vector< Vertex_t > vertices{ };

	float step = ( M_PI * 2.f ) / segments;
	for( float i{ 0.f }; i < ( M_PI * 2.f ); i += step )
		vertices.emplace_back( Vector2D{ x + ( radius * std::cos( i ) ), y + ( radius * std::sin( i ) ) } );

	g_pSurface->DrawTexturedPolygon( vertices.size( ), vertices.data( ) );

	g_pSurface->DrawSetColor( color );
	g_pSurface->DrawOutlinedCircle( x, y, radius, segments );
}

void Render::Engine::Circle( int x, int y, float radius, int segments, Color color ) {
	g_pSurface->DrawSetColor( color );
	g_pSurface->DrawOutlinedCircle( x, y, radius, segments );
}

void Render::Engine::Gradient( int x, int y, int w, int h, Color color, Color color2, bool horizontal ) {
	g_pSurface->DrawSetColor( color );
	g_pSurface->DrawFilledRectFade( x, y, x + w, y + h, color.a( ), 0, horizontal );

	g_pSurface->DrawSetColor( color2 );
	g_pSurface->DrawFilledRectFade( x, y, x + w, y + h, 0, color2.a( ), horizontal );
}

void Render::Engine::Gradient( Vector2D pos, Vector2D size, Color color, Color color2, bool horizontal ) {
	g_pSurface->DrawSetColor( color );
	g_pSurface->DrawFilledRectFade( pos.x, pos.y, pos.x + size.x, pos.y + size.y, color.a( ), 0, horizontal );

	g_pSurface->DrawSetColor( color2 );
	g_pSurface->DrawFilledRectFade( pos.x, pos.y, pos.x + size.x, pos.y + size.y, 0, color2.a( ), horizontal );
}

#pragma endregion

Vector2D Render::GetScreenSize( ) {
	return Vector2D( Render::Engine::m_width, Render::Engine::m_height );
}

void Render::Engine::RotateTriangle( std::array< Vector2D, 3 > &points, float rotation ) {
	const auto vecPointsCenter = ( points.at( 0 ) + points.at( 1 ) + points.at( 2 ) ) / 3;
	for( auto &point : points ) {
		point -= vecPointsCenter;

		const auto temp_x = point.x;
		const auto temp_y = point.y;

		const auto theta = DEG2RAD( rotation );
		const auto c = cos( theta );
		const auto s = sin( theta );

		point.x = temp_x * c - temp_y * s;
		point.y = temp_x * s + temp_y * c;

		point += vecPointsCenter;
	}
};
