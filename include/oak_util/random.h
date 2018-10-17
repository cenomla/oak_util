#pragma once

#include <cinttypes>

namespace oak {

	struct MemoryArena;

	struct RandomGenerator {
		int i = 0;
		int j = 1;
		int k = 2;
		uint64_t *state = nullptr;

		void init(MemoryArena *arena, int j, int k, uint64_t seed);

		void gen_next();
		int random_int();
		double random_double();
		float random_float();
	};

	int random_range(RandomGenerator *generator, int min, int max);
	float random_range(RandomGenerator *generator, float min, float max);

}

