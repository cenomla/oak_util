#include <oak_util/memory.h>
#include <oak_util/random.h>
#include <oak_util/fmt.h>

int main(int, char**) {

	oak::MemoryArena tempArena;
	oak::init_linear_arena(&tempArena, &oak::globalAllocator, 1024 * 1024);
	oak::temporaryMemory = { &tempArena, oak::allocate_from_linear_arena, nullptr };

	oak::PCGenerator rng{ oak::default_rng_params };

	f32 sum = 0.f;
	for (i32 i = 0; i < 10000; ++i) {
		auto rn = rng.random_float();
		assert(rn <= 1.0);
		sum += rn;
	}
	oak::print_fmt("Random sum: %g\n", sum);

	return 0;
}

