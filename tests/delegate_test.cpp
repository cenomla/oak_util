#include <oak_util/types.h>
#include <oak_util/containers.h>
#include <oak_util/memory.h>
#include <oak_util/delegate.h>
#include <cstdio>

using namespace oak;


int dFn(int *ptr) {
	auto oldValue = *ptr;
	*ptr = 5;
	return oldValue;
}

int main(int, char**) {

	MemoryArena tempMemory;
	init_memory_arena(&tempMemory, &globalAllocator, 64 * 1024 * 1024);

	temporaryMemory.arena = &tempMemory;
	temporaryMemory.allocFn = allocate_from_arena;

	Delegate<int(int *)> delegate;
	delegate.set(&dFn);

	Delegate<int()> ld{ []() -> int { return 4; } };
	/*
	Delegate<int()> ld;
	ld.set([]() -> int { return 4; } );
	*/

	int i = 49;
	fprintf(stdout, "%d\n", delegate(&i));
	fprintf(stdout, "%d\n", i);

	auto delegate1 = std::move(delegate);
	i = 51;
	fprintf(stdout, "%d\n", delegate1(&i));
	fprintf(stdout, "%d\n", i);



	return 0;
}
