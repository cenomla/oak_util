#include <oak_util/types.h>
#include <oak_util/containers.h>
#include <oak_util/memory.h>
#include <oak_util/algorithm.h>
#include <oak_util/fmt.h>
#include <cstdio>

using namespace oak;


template<typename Soa>
void print_soa(Soa const& soa, i64 const count) {
	oak::print_fmt("--------------------------\n");
	for (i64 i = 0; i < count; ++i) {
		auto [v0, v1, v2] = soa[i];
		oak::print_fmt("[%g, %g, %g]\n", v0 ? "true" : "false", v1, v2);
	}
}

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
		print_fmt("[%g]\n", elem);
	}
}

int test_soa() {

	SOA<bool, i64, f32> soa;
	i64 soaCount = 64;

	soa.init(&temporaryMemory, soaCount);

	for (i64 i = 0; i < soaCount; ++i) {
		auto [v0, v1, v2] = soa[i];
		v0 = true;
		v1 = i;
		v2 = .25 * i;
	}

	print_soa(soa, soaCount);

	return 0;

}

int test_vector() {
	Vector<i32> vector{ &globalAllocator, { 5, 6, 90, 1lu<<40 } };

	assert(vector.count == 4);

	for (i32 i = 0; i != 512; ++i) {
		vector.push(&globalAllocator, i * 40 - 20);
	}

	assert(vector.count == 516);
	assert(vector.capacity == 1024);

	vector.clear();

	assert(vector.count == 0);

	for (i32 i = 0; i != 256; ++i) {
		vector.push(&globalAllocator, i * 40 - 20);
	}

	for (i32 i = 0; i != 256; ++i) {
		vector.insert(&globalAllocator, i * 8 - 49, 0);
	}

	assert(vector.count = 512);
	assert(vector.capacity == 1024);

	vector.destroy(&globalAllocator);

	assert(vector.data == nullptr);

	return 0;
}

int main(int, char**) {

	MemoryArena tempMemory;
	init_linear_arena(&tempMemory, &globalAllocator, 64 * 1024 * 1024);

	temporaryMemory = { &tempMemory, allocate_from_linear_arena, nullptr };

	test_soa();

	HashSet<int, HashFn<int>, CmpFn<int, int>, int, double> set0;
	HashSet<int, HashFn<int>, CmpFn<int, int>, int, double> set1;

	set0.init(&temporaryMemory, 20);
	set1.init(&temporaryMemory, 20);


	for (int i = 0; i < set1.capacity; ++i) {
		set1.insert(i, i + set0.capacity, i + set0.capacity);
	}
	for (int i = 0; i < set0.capacity; ++i) {
		set0.insert(i, i, i);
	}

	set0.remove(set0.find(31));
	print_set(set0);

	set0.remove(set0.find(0));
	print_set(set0);

	set0.remove(set0.find(23));
	print_set(set0);

	test_vector();

	String string = " Hello barn! ";
	assert(find(string, 'o') != -1);

	String string0{ "Test" };
	assert(find(string0, 'e') != -1);

	return 0;
}
