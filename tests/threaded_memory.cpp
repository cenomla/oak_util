
#include <cassert>
#include <memory>
#include <thread>

#include <oak_util/fmt.h>
#include <oak_util/memory.h>
#include <oak_util/atomic.h>
#include <oak_util/ptr.h>

void print_atomic_arena(oak::MemoryArena *arena) {
	auto header = static_cast<oak::LinearArenaHeader*>(arena->block);
	oak::print_fmt("Arena: [%x, %g, %g, %g, %g]\n",
			reinterpret_cast<u64>(arena->block),
			arena->size,
			atomic_load(&header->usedMemory, oak::MemoryOrder::RELAXED),
			atomic_load(&header->requestedMemory, oak::MemoryOrder::RELAXED),
			atomic_load(&header->allocationCount, oak::MemoryOrder::RELAXED));
}

void print_stuff(int i) {
	for (int j = 0; j < 10; ++j) {
		oak::print_fmt("Henlo %g\n", i * 10 + j);
	}
}

int main(int , char **) {
	oak::MemoryArena tmp;
	if (oak::init_atomic_linear_arena(&tmp, &oak::globalAllocator, 2 * 1024 * 1024) != oak::Result::SUCCESS) {
		return -1;
	}

	oak::temporaryMemory = { &tmp, oak::allocate_from_atomic_linear_arena, nullptr };

	print_atomic_arena(&tmp);

	oak::Slice<std::thread> threads;
	threads.count = 16;
	threads.data = oak::allocate<std::thread>(&oak::temporaryMemory, threads.count);
	for (int i = 0;i < threads.count; ++i) {
		new (threads.data + i) std::thread{ print_stuff, i };
	}

	print_atomic_arena(&tmp);

	for (auto &thread : threads) {
		thread.join();
	}

	print_atomic_arena(&tmp);

	return 0;
}

