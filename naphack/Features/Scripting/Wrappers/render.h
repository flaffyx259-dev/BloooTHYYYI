#pragma once

#include "../../../Renderer/Render.hpp"

namespace Wrappers::Renderer {

	void Rect( int x, int y, int w, int h, Color color ) {
		if( !Scripting::Script::in_paint_callback )
			return;

		Render::Engine::Rect( x, y, w, h, color );
	}

	void RectFilled( int x, int y, int w, int h, Color color ) {
		if( !Scripting::Script::in_paint_callback )
			return;

		Render::Engine::RectFilled( x, y, w, h, color );
	}

	void RectRoundedFilled( int x, int y, int w, int h, Color color, int radius = 5 ) {
		if( !Scripting::Script::in_paint_callback )
			return;

		Render::Engine::FilledRoundedRect( { ( float )x, ( float )y }, { ( float )w, ( float )h }, radius, color );
	}

	void RectRounded( int x, int y, int w, int h, Color color, int radius = 5 ) {
		if( !Scripting::Script::in_paint_callback )
			return;

		Render::Engine::RoundedRect( { ( float )x, ( float )y }, { (float)w, ( float )h }, radius, color );
	}

	bool WorldToScreen( const Vector& world, Vector2D& screen ) {
		return Render::Engine::WorldToScreen( world, screen );
	}

	void Polygon( int count, Vertex_t* vertices, Color col ) {
		if( !Scripting::Script::in_paint_callback )
			return;

		Render::Engine::Polygon( count, vertices, col );
	}
	void FilledTriangle( Vector2D pos1, Vector2D pos2, Vector2D pos3, Color col ) {
		if( !Scripting::Script::in_paint_callback )
			return;

		Render::Engine::FilledTriangle( pos1, pos2, pos3, col );
	}

	void WorldCircle( Vector origin, float radius, Color color, Color colorFill ) {
		if( !Scripting::Script::in_paint_callback )
			return;

		Render::Engine::WorldCircle( origin, radius, color, colorFill );
	}

	void Line( int x0, int y0, int x1, int y1, Color color ) {
		if( !Scripting::Script::in_paint_callback )
			return;

		Render::Engine::Line( x0, y0, x1, y1, color );
	}

	void CircleFilled( int x, int y, float radius, int segments, Color color ) {
		if( !Scripting::Script::in_paint_callback )
			return;

		Render::Engine::CircleFilled( x, y, radius, segments, color );
	}

	void Gradient( int x, int y, int w, int h, Color color, Color color2, bool horizontal ) {
		if( !Scripting::Script::in_paint_callback )
			return;

		Render::Engine::Gradient( x, y, w, h, color, color2, horizontal );
	}
	
	void Text( int x, int y, std::string text, Color color ) {
		if( !Scripting::Script::in_paint_callback )
			return;

		Render::Engine::hud.string( x, y, color, text );
	}

	Render::Engine::Font GetFont( std::string fontname, int size, int weight, int flags ) { 
		bool bOutline = false;
		// use epic outline instead :)
		if( flags & FONTFLAG_OUTLINE ) {
			bOutline = true;

			flags &= ~FONTFLAG_OUTLINE;
		}

		return Render::Engine::Font( fontname, size, weight, flags, bOutline );
	}

	std::pair<float, float> GetTextSize( std::string text ) {
		auto xd = Render::Engine::hud.size( text );

		return std::make_pair( xd.m_width, xd.m_height );
	}

	std::pair<float, float> GetScreenSize( ) {
		auto screen = Render::GetScreenSize( );

		return std::make_pair( screen.x, screen.y );
	}
}