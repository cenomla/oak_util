#include "oak_util/random.h"

#include <cassert>
#include <oak_util/memory.h>
#include <oak_util/bit.h>

// Used to convert an int to double
#define TWO_M52 2.2204460492503131e-16
// The parameters for the lcg that seeds the lfg
#define LFG_SEED_A 25214903917
#define LFG_SEED_C 11
#define LFG_SEED_M 45

namespace oak {

	void LCGenerator::init(uint64_t a_, uint64_t c_, uint64_t m_, uint64_t seed) {
		a = a_;
		c = c_;
		m = (uint64_t{ 1 } << m_) - 1;
		state = seed;
	}

	void LCGenerator::advance_state() {
		state = (state * a + c) & m;
	}

	int LCGenerator::random_int() {
		advance_state();
		// Return the 15th - 46th bits
		return static_cast<int>((state >> 15) & 0x7FFFFFFF);
	}

	double LCGenerator::random_double() {
		advance_state();
		return static_cast<double>(state << (52 - blog2(m + 1))) * TWO_M52;
	}

	float LCGenerator::random_float() {
		return static_cast<float>(random_double());
	}

	void LFGenerator::init(MemoryArena *arena, int l_, int k_, uint64_t seed) {
		l = l_;
		k = k_;
		state = make_structs<uint64_t>(arena, k);
		LCGenerator rng;
		rng.init(LFG_SEED_A, LFG_SEED_C, LFG_SEED_M, seed);
		for (int i = 0; i < k; ++i) {
			state[i] = rng.state;
			rng.advance_state();
		}
	}

	void LFGenerator::advance_state() {
		assert(state);
		++pos;
		if (pos >= k) { pos -= k; }
		auto lpos = pos - l;
		if (lpos < 0) { lpos += k; }
		state[pos] = state[pos] + state[lpos];
	}

	int LFGenerator::random_int() {
		advance_state();
		return static_cast<int>((state[pos] >> 15) & 0x7FFFFFFF);
	}

	double LFGenerator::random_double() {
		advance_state();
		return (state[pos] >> 12) * TWO_M52;
	}

	float LFGenerator::random_float() {
		return static_cast<float>(random_double());
	}

}

