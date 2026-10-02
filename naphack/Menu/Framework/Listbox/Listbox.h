#pragma once

namespace Scripting {
	struct cloud_script_token_t;
	struct cloud_script_t;
}

namespace GUI::Controls {
	bool Listbox(std::string id, std::vector<std::string> elements, int* option, bool bSearchBar, int iSizeInElements);
	bool Luabox( const std::string &id, std::vector<std::string> elements, int *option, bool bSearchBar, int iSizeInElements );
}