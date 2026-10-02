#include "../gui.h"
#include "../../../pandora.hpp"
#include "../../../Features/Scripting/Scripting.hpp"

bool GUI::Controls::Label( std::string name ) {
	Vector2D CursorPos = PopCursorPos( );
	Vector2D DrawPos = ctx->pos + CursorPos;

	const float flBackupMultiplier = g_pSurface->DrawGetAlphaMultiplier( );

	if( !ctx->enabled )
		g_pSurface->DrawSetAlphaMultiplier( flBackupMultiplier * 0.3f );

	std::transform( name.begin( ), name.end( ), name.begin( ), ::tolower );

	Render::Engine::menu_regular.string( DrawPos.x, DrawPos.y - 1, Color( 150, 150, 150 ), GUI::SplitStr( name, '#' )[ 0 ].data( ) );

	if( !ctx->enabled )
		g_pSurface->DrawSetAlphaMultiplier( flBackupMultiplier );

	GUI::PushCursorPos( CursorPos + Vector2D( 0, Render::Engine::menu_regular.size( GUI::SplitStr( name, '#' )[ 0 ].data( ) ).m_height + GUI::ObjectPadding( ) ) );

	ctx->szLastElementName = GUI::SplitStr( name, '#' )[ 0 ];

	return ( name.size( ) > 0 );
}
