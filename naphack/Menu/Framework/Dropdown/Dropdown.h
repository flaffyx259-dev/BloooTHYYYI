#pragma once

namespace GUI::Controls {
	bool Dropdown( std::string name, std::vector< std::string > values, int* var_name, int max_items = 10, std::pair<std::string, std::string> tt = { } );
}
