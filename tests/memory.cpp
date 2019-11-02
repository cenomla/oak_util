#include <cassert>

#include <oak_util/fmt.h>
#include <oak_util/memory.h>
#include <oak_util/ptr.h>

struct Obj {
	int32_t a;
	uint64_t b;
	float c;
};

void print_pool(oak::MemoryArena *pool) {
	// Print pool contents
	auto nodePtr = &static_cast<oak::PoolHeader*>(pool->block)->freeList;
	while ((*nodePtr)) {
		oak::print_fmt("node[%g, %g, %g]\n", (uintptr_t)(*nodePtr), (uintptr_t)(*nodePtr)->next, (*nodePtr)->size);
		nodePtr = &(*nodePtr)->next;
	}
}

int main(int , char **) {
	oak::MemoryArena tmp, poolArena, ringArena;
	if (oak::init_linear_arena(&tmp, &oak::globalAllocator, 2 * 1024 * 1024) != oak::Result::SUCCESS) {
		return -1;
	}

	oak::temporaryMemory = { &tmp, oak::allocate_from_linear_arena, nullptr };

	if (oak::init_memory_pool(&poolArena, &oak::globalAllocator, 1024 * 1024, 8) != oak::Result::SUCCESS) {
		return -1;
	}

	if (oak::init_ring_arena(&ringArena, &oak::globalAllocator, 1024 * 1024) != oak::Result::SUCCESS) {
		return -1;
	}

	{

	oak::Allocator pool{ &poolArena, oak::allocate_from_pool, oak::free_from_pool };

	auto obj1 = oak::allocate<Obj>(&pool, 1);
	*obj1 = Obj{ -49, 40000, 22.2f };

	auto obj2 = oak::allocate<char>(&pool, 1);
	*obj2 = 'a';

	auto obj3 = oak::allocate<Obj>(&pool, 1);
	*obj3 = Obj{ -300, 90, 11.1f };

	assert(obj1->a == -49 && obj1->b == 40000 && obj1->c == 22.2f);
	assert(*obj2 == 'a');
	assert(obj3->a == -300 && obj3->b == 90 && obj3->c == 11.1f);

	print_pool(&poolArena);

	oak::deallocate(&pool, obj1, 1);

	print_pool(&poolArena);

	auto obj4 = oak::allocate<Obj>(&pool, 1);
	assert(obj4 == obj1);

	oak::deallocate(&pool, obj2, 1);
	print_pool(&poolArena);

	oak::deallocate(&pool, obj4, 1);
	print_pool(&poolArena);

	oak::deallocate(&pool, obj3, 1);
	print_pool(&poolArena);

	auto poolHeader = static_cast<oak::PoolHeader*>(poolArena.block);
	auto node = &poolHeader->freeList;
	while (*node) {
		if ((*node)->next) {
			assert(oak::add_ptr((*node), (*node)->size) == (*node)->next);
		}
		node = &(*node)->next;
	}
	}

	{
		auto header = static_cast<oak::RingArenaHeader*>(ringArena.block);
		auto obj0 = oak::allocate_from_ring_arena(&ringArena, 53, 8);
		auto obj1 = oak::allocate_from_ring_arena(&ringArena, 1024, 8);
		auto obj2 = oak::allocate_from_ring_arena(&ringArena, 1024 * 1024, 8);
		auto obj3 = oak::allocate_from_ring_arena(&ringArena, 4, 8);
		auto obj4 = oak::allocate_from_ring_arena(&ringArena, 8, 8);

		assert(!obj2);

		assert(obj0);
		assert(obj1);
		assert(obj3);
		assert(obj4);

		oak::deallocate_from_ring_arena(&ringArena, obj0, 53);
		oak::deallocate_from_ring_arena(&ringArena, obj1, 1024);
		oak::deallocate_from_ring_arena(&ringArena, obj3, 4);
		oak::deallocate_from_ring_arena(&ringArena, obj4, 8);

		assert(header->requestedMemory == 0);
		assert(header->usedMemory == sizeof(oak::RingArenaHeader));
		assert(header->allocationCount == 0);


		for (i32 i = 0; i < 200; ++i) {
			oak::allocate_from_ring_arena(&ringArena, 97, 8);
		}

		oak::print_fmt(
				"ring arena efficiency: %g\n",
				static_cast<double>(header->requestedMemory) / static_cast<double>(header->usedMemory));

		oak::clear_ring_arena(&ringArena);

		for (i32 i = 0; i < 10; ++i) {
			auto p = oak::allocate_from_ring_arena(&ringArena, 300 * 1024, 8);
			oak::deallocate_from_ring_arena(&ringArena, p, 300 * 1024);
		}

		assert(header->requestedMemory == 0);
		assert(header->usedMemory == sizeof(oak::RingArenaHeader));
		assert(header->allocationCount == 0);

	}
	return 0;
}
