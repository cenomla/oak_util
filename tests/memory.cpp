#include <cassert>
#include "../fmt.h"
#include "../memory.h"

struct Obj {
	int32_t a;
	uint64_t b;
	float c;
};

void print_pool(oak::MemoryArena *pool) {
	//print pool contents
	auto nodePtr = &static_cast<oak::PoolHeader*>(pool->block)->freeList;
	while ((*nodePtr)) {
		oak::print_fmt("node[%, %, %]\n", (uintptr_t)(*nodePtr), (uintptr_t)(*nodePtr)->next, (*nodePtr)->size);
		nodePtr = &(*nodePtr)->next;
	}
}

int main(int argc, char **argv) {
	auto tmp = oak::create_memory_arena(2'000'000);
	oak::temporaryMemory = &tmp;
	auto arena = oak::create_memory_arena(2'000'000);
	auto pool = oak::create_pool(&arena, 1'000'000);

	auto obj1 = static_cast<Obj*>(oak::allocate_from_pool(&pool, sizeof(Obj), 1));
	*obj1 = Obj{ -49, 40000, 22.2f };
	auto obj2 = static_cast<char*>(oak::allocate_from_pool(&pool, sizeof(char), 1));
	*obj2 = 'a';
	auto obj3 = static_cast<Obj*>(oak::allocate_from_pool(&pool, sizeof(Obj), 1));
	*obj3 = Obj{ -300, 90, 11.1f };

	assert(obj1->a == -49 && obj1->b == 40000 && obj1->c == 22.2f);
	assert(*obj2 == 'a');
	assert(obj3->a == -300 && obj3->b == 90 && obj3->c == 11.1f);

	print_pool(&pool);

	oak::free_from_pool(&pool, obj1, sizeof(Obj), 1);

	print_pool(&pool);

	auto obj4 = static_cast<Obj*>(oak::allocate_from_pool(&pool, sizeof(Obj), 1));
	assert(obj4 == obj1);

	oak::free_from_pool(&pool, obj2, sizeof(char), 1);
	print_pool(&pool);
	oak::free_from_pool(&pool, obj4, sizeof(Obj), 1);
	print_pool(&pool);
	oak::free_from_pool(&pool, obj3, sizeof(Obj), 1);
	print_pool(&pool);

	auto poolHeader = static_cast<oak::PoolHeader*>(pool.block);
	auto node = &poolHeader->freeList;
	while (*node) {
		if ((*node)->next) {
			assert(oak::add_ptr((*node), (*node)->size) == (*node)->next);
		}
		node = &(*node)->next;
	}

	return 0;
}
