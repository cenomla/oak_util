#pragma once

#include <cinttypes>

namespace oak {

	struct MemoryArena;

	struct LCGenerator {
		uint64_t state = 0;
		uint64_t a = 0;
		uint64_t c = 0;
		uint64_t m = 0;

		void init(uint64_t a, uint64_t c, uint64_t m, uint64_t seed);

		void advance_state();
		int random_int();
		double random_double();
		float random_float();
	};

	struct LFGenerator {
		int pos = 0;
		int l = 0;
		int k = 0;
		uint64_t *state = nullptr;

		void init(MemoryArena *arena, int l, int k, uint64_t seed);

		void advance_state();
		int random_int();
		double random_double();
		float random_float();
	};

	template<typename T>
	int random_range(T *generator, int min, int max) {
		return min + (generator->random_int() % (max - min));
	}

	template<typename T>
	float random_range(T *generator, float min, float max) {
		return min + (generator->random_float() * (max - min));
	}

}

