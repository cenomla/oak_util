#include "oak_util/random.h"

#include <cassert>
#include "oak_util/memory.h"

// Used to convert an int to double
#define TWO_M52 2.2204460492503131e-16

namespace oak {

	void RandomGenerator::init(MemoryArena *arena, int j_, int k_, uint64_t seed) {
		j = j_;
		k = k_;
		state = make_structs<uint64_t>(arena, k);
		state[0] = seed & uint64_t{ 0xFFFFFFFF00000000 };
		++i;
	}

	void RandomGenerator::gen_next() {
		assert(state);
		auto idx0 = i - j;
		auto idx1 = i - k;
		if (idx0 < 0) { idx0 = k - idx0; }
		if (idx1 < 0) { idx1 = k - idx1; }
		state[i++] = state[idx0] * state[idx1];
		if (i >= k) {
			i -= k;
		}
	}

	int RandomGenerator::random_int() {
		gen_next();
		return static_cast<int>(state[i - 1] >> 33);
	}

	double RandomGenerator::random_double() {
		gen_next();
		return (state[i - 1] >> 12) * TWO_M52;
	}

	float RandomGenerator::random_float() {
		return static_cast<float>(random_double());
	}

	int random_range(RandomGenerator *generator, int min, int max) {
		return min + (generator->random_int() * (max - min));
	}

	float random_range(RandomGenerator *generator, float min, float max) {
		return min + (generator->random_float() * (max - min));
	}

}
