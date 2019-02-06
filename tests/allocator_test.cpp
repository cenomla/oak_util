#include <oak_util/allocator.h>
#include <oak_util/ptr.h>

using oak::experimental;

int main(int, char **) {

	Allocator allocator;
	allocator.allocFn = [](u64 size, u64 alignment) {
		static i64 offset = 0;
		static u8 buffer[1024 * 1024];

		offset = align_int(offset, alignment);
		auto ptr = static_cast<void*>(&buffer[offset]);
		offset += size;

		return ptr;
	};
	allocator.freeFn = []() {};

	int *i = nullptr;

	mem_alloc(&i, 1);
	assert(i);

	mem_make(&i, 1, 10);
	assert(i);
	assert(*i == 10);

	return 0;
}
