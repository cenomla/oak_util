#include <oak_util/fmt.h>
#include <oak_util/memory.h>

int main(int, char**) {

	oak::MemoryArena tempArena;
	oak::init_linear_arena(&tempArena, &oak::globalAllocator, 1024 * 1024);
	oak::temporaryMemory = { &tempArena, oak::allocate_from_linear_arena, nullptr };

	auto fmtStr = oak::fmt(&oak::temporaryMemory, "%g %g %g %g %g ", 'a', "hello", 49, 0.05, 0.5f);
	oak::print_fmt(fmtStr);
	assert(fmtStr == "a hello 49 0.05 0.5");

	fmtStr = oak::fmt(&oak::temporaryMemory, "%e", 200000.0);
	oak::print_fmt(fmtStr);


	return 0;
}
