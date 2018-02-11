#include "string_builder.h"

#include <cstdio>

namespace oak {

	void StringBuilder::push(String str) {
		auto start = string.size;
		string.resize(string.size + str.size);
		memcpy(string.data + start, str.data, str.size);
	}

	void StringBuilder::push(float v) {
		char buffer[128]{ 0 };
		sprintf(buffer, "%f", v);
		push(buffer);
	}

	void StringBuilder::push(int32_t v) {
		char buffer[128]{ 0 };
		sprintf(buffer, "%i", v);
		push(buffer);
	}

	void StringBuilder::push(int64_t v) {
		char buffer[128]{ 0 };
		sprintf(buffer, "%li", v);
		push(buffer);
	}

	void StringBuilder::push(uint32_t v) {
		char buffer[128]{ 0 };
		sprintf(buffer, "%u", v);
		push(buffer);
	}

	void StringBuilder::push(uint64_t v) {
		char buffer[128]{ 0 };
		sprintf(buffer, "%lu", v);
		push(buffer);
	}

}
