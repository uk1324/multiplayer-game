#pragma once
#include <utils/int.hpp>

// C++ 20 <bit> header doesn't work.
constexpr auto isPowerOf2(usize x) -> bool {
	// A power of 2 is in binary 10000... so subtracting 1 from it results in 01111... which when ended togther produce zero.
	return !(x == 0) && !(x & (x - 1));
}