#pragma once

namespace GUI::Form {
	void HandleDrag( );
	bool BeginWindow( std::string name );
	void EndWindow( std::string name );

	bool BeginTab( std::string name, std::string text2 );
	bool BeginSubTab( std::string name );

	void AddPopup( std::string name, std::string innerString, bool bHasConfirmButton = false, std::function<void( )> fnOnConfirm = {} );
}
