#pragma once

#include <xmmintrin.h>

#include "types.h"

namespace oak {

	using vf128 = __m128;
	using vd128 = __m128d;
	using vi128 = __m128i;

	OAK_FINLINE vi128 simd_splat_i8x16(i8 v) {
		return _mm_set1_epi8(v);
	}

	OAK_FINLINE vi128 simd_loadu_i128(void const* mem) {
		return _mm_loadu_si128(static_cast<__m128i const*>(mem));
	}

	OAK_FINLINE vi128 simd_cmpeq_i8x16(vi128 lhs, vi128 rhs) {
		return _mm_cmpeq_epi8(lhs, rhs);
	}

	OAK_FINLINE vi128 simd_cmplt_i8x16(vi128 lhs, vi128 rhs) {
		return _mm_cmplt_epi8(lhs, rhs);
	}

	OAK_FINLINE vi128 simd_cmpgt_i8x16(vi128 lhs, vi128 rhs) {
		return _mm_cmpgt_epi8(lhs, rhs);
	}

	OAK_FINLINE vi128 simd_and_i128(vi128 lhs, vi128 rhs) {
		return _mm_and_si128(lhs, rhs);
	}

	OAK_FINLINE vi128 simd_andnot_i128(vi128 lhs, vi128 rhs) {
		return _mm_andnot_si128(lhs, rhs);
	}

	OAK_FINLINE vi128 simd_or_i128(vi128 lhs, vi128 rhs) {
		return _mm_or_si128(lhs, rhs);
	}

	OAK_FINLINE u32 simd_movemask_i8x16(vi128 v) {
		return _mm_movemask_epi8(v);
	}

}
