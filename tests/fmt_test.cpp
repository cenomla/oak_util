#include <oak_util/fmt.h>
#include <oak_util/memory.h>

int main(int, char**) {

	oak::MemoryArena tempArena;
	oak::init_linear_arena(&tempArena, oak::globalAllocator, 1024 * 1024);
	oak::temporaryAllocator = { &tempArena, oak::allocate_from_linear_arena, nullptr };

	auto fmtStr = oak::fmt(oak::temporaryAllocator, "%% %g %g %g %g %g %", 'a', "hello", 49, 0.05, 0.5f);
	oak::print_fmt(fmtStr);
	oak::print_fmt("\n");
	assert(fmtStr == "%% a hello 49 0.050000 0.500000 %");

	fmtStr = oak::fmt(oak::temporaryAllocator, "%e", 200000.0);
	oak::print_fmt(fmtStr);
	oak::print_fmt("\n");
	assert(fmtStr == "2.000000e+05");

	fmtStr = oak::fmt(oak::temporaryAllocator, "%b %o %x", 59, 59, 59);
	oak::print_fmt(fmtStr);
	oak::print_fmt("\n");
	assert(fmtStr == "0b111011 073 0x3b");

	fmtStr = oak::fmt(oak::temporaryAllocator, "%b", static_cast<i64>(0xFFFFFFFFFFFFFFFF));
	oak::print_fmt(fmtStr);
	oak::print_fmt("\n");
	assert(fmtStr == "-0b1");

	fmtStr = oak::fmt(oak::temporaryAllocator, "%b", 0xFFFFFFFFFFFFFFFF);
	oak::print_fmt(fmtStr);
	oak::print_fmt("\n");
	assert(fmtStr == "0b1111111111111111111111111111111111111111111111111111111111111111");


	return 0;
}
