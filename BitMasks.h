#ifndef MZ_UTILITIES_BITMASK_HEADER_FILE
#define MZ_UTILITIES_BITMASK_HEADER_FILE
#pragma once

#include <intrin.h>
#include <cstdint>
#include <type_traits>
#include <immintrin.h>

/*
*  bitmasks.h
*  Utility functions for generating minimal bitmasks for numerical literals.
*
*  Author: Meysam Zare
*  License: MIT (2020)
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
*
*  The above copyright notice and this permission notice shall be included in all
*  copies or substantial portions of the Software.
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*  SOFTWARE.
*/

/*
    Overview:
    ----------
    These functions compute the smallest bitmask that covers all bits up to the
    most significant bit set in a given unsigned integer. For example, for input
    5 (binary 101), the mask would be 7 (binary 111).

    Functions:
    ----------
    - bit_mask32_safe: Portable, safe version for 32-bit values.
    - bit_mask64_safe: Portable, safe version for 64-bit values.
    - bit_mask32: Optimized version using compiler intrinsics for 32-bit values.
    - bit_mask64: Optimized version using compiler intrinsics for 64-bit values.

    Usage:
    ------
    uint32_t mask = mz::bit_mask32(123);
    uint64_t mask = mz::bit_mask64(123456789ULL);
*/

namespace mz {

    // Returns the smallest 32-bit mask covering all bits up to the highest set bit in UpperBound.
    // Example: UpperBound = 5 (0b101) => returns 7 (0b111)
    inline uint32_t bit_mask32_safe(uint32_t UpperBound) noexcept {
        // Count the number of bits required to represent UpperBound
        uint32_t bit_count = 0;
        uint32_t value = UpperBound;
        while (value) {
            ++bit_count;
            value >>= 1;
        }
        // If UpperBound is 0, returns 0. Otherwise, returns (1 << bit_count) - 1
        return bit_count ? ((1u << bit_count) - 1u) : 0u;
    }

    // Returns the smallest 64-bit mask covering all bits up to the highest set bit in UpperBound.
    // Example: UpperBound = 5 (0b101) => returns 7 (0b111)
    inline uint64_t bit_mask64_safe(uint64_t UpperBound) noexcept {
        // Count the number of bits required to represent UpperBound
        uint64_t bit_count = 0;
        uint64_t value = UpperBound;
        while (value) {
            ++bit_count;
            value >>= 1;
        }
        // If UpperBound is 0, returns 0. Otherwise, returns (1ULL << bit_count) - 1ULL
        return bit_count ? ((1ULL << bit_count) - 1ULL) : 0ULL;
    }

    // Optimized 32-bit mask using compiler intrinsics for leading zero count.
    // Falls back to safe version if intrinsics are unavailable.
    inline uint32_t bit_mask32(uint32_t UpperBound) noexcept {
#if defined(_MSC_VER)
        // __lzcnt returns the number of leading zeros
        // If UpperBound is 0, __lzcnt returns 32, so mask becomes 0
        return UpperBound ? (0xFFFFFFFFu >> __lzcnt(UpperBound)) : 0u;
#elif defined(__GNUC__) || defined(__GNUG__)
        // __builtin_clz returns the number of leading zeros
        // If UpperBound is 0, result is undefined, so check for 0
        return UpperBound ? (0xFFFFFFFFu >> __builtin_clz(UpperBound)) : 0u;
#else
        // Portable fallback
        return bit_mask32_safe(UpperBound);
#endif
    }

    // Optimized 64-bit mask using compiler intrinsics for leading zero count.
    // Falls back to safe version if intrinsics are unavailable.
    inline uint64_t bit_mask64(uint64_t UpperBound) noexcept {
#if defined(_MSC_VER)
        // __lzcnt64 returns the number of leading zeros
        // If UpperBound is 0, __lzcnt64 returns 64, so mask becomes 0
        return UpperBound ? (0xFFFFFFFFFFFFFFFFull >> __lzcnt64(UpperBound)) : 0ull;
#elif defined(__GNUC__) || defined(__GNUG__)
        // __builtin_clzll returns the number of leading zeros
        // If UpperBound is 0, result is undefined, so check for 0
        return UpperBound ? (0xFFFFFFFFFFFFFFFFull >> __builtin_clzll(UpperBound)) : 0ull;
#else
        // Portable fallback
        return bit_mask64_safe(UpperBound);
#endif
    }

} // namespace mz

#endif // MZ_UTILITIES_BITMASK_HEADER_FILE
