
#include <cassert>
#include <thread>

#include <oak_util/fmt.h>
#include <oak_util/memory.h>
#include <oak_util/ptr.h>

void print_arena(oak::MemoryArena *arena) {
	oak::print_fmt("Arena: [%, %, %]\n", reinterpret_cast<u64>(arena->block), arena->size, static_cast<oak::MemoryArenaHeader*>(arena->block)->usedMemory.load(std::memory_order_release));
}

void print_stuff(int i) {
	oak::print_fmt("Henlo %\n", i);
}

int main(int , char **) {
	oak::MemoryArena tmp;
	if (oak::init_memory_arena(&tmp, &oak::globalAllocator, 2 * 1024 * 1024) != oak::Result::SUCCESS) {
		return -1;
	}

	oak::temporaryMemory = { &tmp, oak::allocate_from_arena, nullptr };

	print_arena(&tmp);

	oak::Slice<std::thread> threads;
	threads.count = 16;
	threads.data = oak::allocate<std::thread>(&oak::temporaryMemory, threads.count);
	for (int i = 0;i < threads.count; ++i) {
		new (threads.data + i) std::thread{ print_stuff, i };
	}

	print_arena(&tmp);

	for (auto &thread : threads) {
		thread.join();
	}

	print_arena(&tmp);

	return 0;
}
