#include "oak_util/random.h"

#include <cassert>
#include <oak_util/memory.h>
#include <oak_util/bit.h>

// Used to convert an i32 to f64
#define TWO_M52 2.2204460492503131e-16
// The parameters for the lcg that seeds the lfg
#define LFG_SEED_A 25214903917
#define LFG_SEED_C 11
#define LFG_SEED_M 45

namespace oak {

	LCGenerator::LCGenerator(u64 a_, u64 c_, u64 m_)
		: a{ a_ }, c{ c_ }, m{ (u64{ 1 } << m_) - 1 } {}

	LCGenerator::LCGenerator(DefaultRngParams) : LCGenerator(25214903917, 11, 45) {}

	void LCGenerator::init(u64 const seed) {
		state = seed;
	}

	void LCGenerator::advance_state() {
		state = (state * a + c) & m;
	}

	i32 LCGenerator::random_int() {
		advance_state();
		// Return the 15th - 46th bits
		return static_cast<i32>((state >> 15) & 0x7FFFFFFF);
	}

	f64 LCGenerator::random_double() {
		advance_state();
		return static_cast<f64>(state << (52 - blog2(m + 1))) * TWO_M52;
	}

	f32 LCGenerator::random_float() {
		return static_cast<f32>(random_double());
	}

	LFGenerator::LFGenerator(i32 l_, i32 k_) : l{ l_ }, k{ k_ } {}

	LFGenerator::LFGenerator(DefaultRngParams) : l{ 31 }, k{ 63 } {}

	void LFGenerator::init(Allocator *allocator, u64 seed) {
		state = make<u64>(allocator, k);
		LCGenerator rng{ default_rng_params };
		rng.init(seed);
		for (i32 i = 0; i < k; ++i) {
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

	i32 LFGenerator::random_int() {
		advance_state();
		return static_cast<i32>((state[pos] >> 15) & 0x7FFFFFFF);
	}

	f64 LFGenerator::random_double() {
		advance_state();
		return (state[pos] >> 12) * TWO_M52;
	}

	f32 LFGenerator::random_float() {
		return static_cast<f32>(random_double());
	}

}

