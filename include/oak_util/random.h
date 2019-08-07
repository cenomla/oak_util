#pragma once

#include "types.h"

namespace oak {

	struct DefaultRngParams {};
	constexpr auto default_rng_params = DefaultRngParams{};

	struct LCGenerator {
		u64 state = 0;
		u64 a = 0;
		u64 c = 0;
		u64 m = 0;

		LCGenerator() = default;
		LCGenerator(u64 a, u64 c, u64 m);
		LCGenerator(DefaultRngParams);

		void init(u64 seed);

		void advance_state();
		i32 random_int();
		f64 random_double();
		f32 random_float();
	};

	struct LFGenerator {
		i32 pos = 0;
		i32 l = 0;
		i32 k = 0;
		u64 *state = nullptr;

		LFGenerator() = default;
		LFGenerator(i32 l, i32 k);
		LFGenerator(DefaultRngParams);

		void init(Allocator *allocator, u64 seed);

		void advance_state();
		i32 random_int();
		f64 random_double();
		f32 random_float();
	};

	template<typename T>
	f32 random_range(T *generator, f32 min, f32 max) {
		return min + (generator->random_float() * (max - min));
	}

	template<typename T>
	i32 random_range(T *generator, i32 min, i32 max) {
		return static_cast<i32>(random_range(generator, static_cast<f32>(min), static_cast<f32>(max) + 0.99f));
	}

}

