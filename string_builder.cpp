#include "string_builder.h"

namespace oak {

	void StringBuilder::push(String str) {
		auto start = string.size;
		string.resize(string.size + str.size);
		memcpy(string.data + start, str.data, str.size);
	}

}
