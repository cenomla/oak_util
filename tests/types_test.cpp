#include <oak_util/types.h>
#include <oak_util/containers.h>
#include <oak_util/memory.h>
#include <cstdio>

using namespace oak;

int main(int, char**) {

	MemoryArena tempMemory;
	init_memory_arena(&tempMemory, 64 * 1024 * 1024);

	temporaryMemory.state = static_cast<void*>(&tempMemory);
	temporaryMemory.allocFn = detail::arena_alloc_wrapper;

	SOA<int, double> soa;
	soa.data = std::malloc(64 * sizeof(std::tuple<int, double>));
	soa.count = 64;

	for (i64 i = 0; i < soa.count; ++i) {
		auto [v0, v1] = soa[i];
		v0 = i;
		v1 = .5 * i;
	}
	auto [i, d] = soa[0];
	i = -50;
	d = .5 * i;

	for (i64 i = 0; i < soa.count; ++i) {
		auto &[s0, s1] = soa;
		fprintf(stdout, "[%d, %lf]\n", s0[i], s1[i]);
	}




	HashSet<int, String> set;

	return 0;
}
