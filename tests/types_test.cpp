#include <oak_util/types.h>
#include <oak_util/containers.h>
#include <oak_util/memory.h>
#include <oak_util/algorithm.h>
#include <cstdio>

using namespace oak;

template<typename Set>
void print_set(Set const& set) {
	for (auto [empty, key, value0, value1] : set) {
		if (!empty) {
			fprintf(stdout, "[%lu, %d, %d, %lf]\n", set.slot(set.hash(key)), key, value0, value1);
		}
	}
}

template<typename Vector>
void print_vector(Vector const& vector) {
	for (auto &elem : vector) {
		fprintf(stdout, "[%lu]\n", elem);
	}
}

int main(int, char**) {

	MemoryArena tempMemory;
	init_memory_arena(&tempMemory, &globalAllocator, 64 * 1024 * 1024);

	temporaryMemory.arena = &tempMemory;
	temporaryMemory.allocFn = allocate_from_arena;

	SOA<int, double> soa;
	soa.count = 64;
	soa.data = allocate_soa<int, double>(&temporaryMemory, soa.count);

	for (i64 i = 0; i < soa.count; ++i) {
		auto [v0, v1] = soa[i];
		v0 = i;
		v1 = .5 * i;
	}
	auto [i, d] = soa[0];
	i = -50;
	d = .5 * i;

	auto t = soa[1];
	t = std::make_tuple(-99, .5 * -99);

	for (i64 i = 0; i < soa.count; ++i) {
		auto &[s0, s1] = soa;
		fprintf(stdout, "[%d, %lf]\n", s0[i], s1[i]);
	}


	HashSet<int, HashFn<int>, CmpFn<int, int>, int, double> set;
	set.init(&temporaryMemory, 64);
	set.insert(1, 1, 1.0);
	set.insert(65, 2, 2.0);
	set.insert(129, 3, 3.0);
	set.insert(193, 4, 4.0);
	set.insert(257, 5, 5.0);

	print_set(set);

	set.remove(set.find(129));

	print_set(set);

	Vector<u64> vector{ &temporaryMemory, { 5, 6, 90, 1lu<<40 } };
	print_vector(vector);

	String string = " Hello barn! ";
	assert(find(string, 'o') != -1);

	String string0{ "Test" };
	assert(find(string0, 'e') != -1);

	return 0;
}
