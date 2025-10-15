/*
MIT License

Copyright (c) 2021-2024 Meysam Zare

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef MZ_RANDOMIZER_HEADER_FILE
#define MZ_RANDOMIZER_HEADER_FILE
#pragma once

#include <cstdint>
#include <random>
#include <vector>
#include <span>
#include <string>
#include <string_view>
#include <limits>
#include <type_traits>
#include <algorithm>
#include <mutex>

/**
 * @file Randomizer.h
 * @brief A high-performance wrapper around the Mersenne Twister random number generator
 *
 * This header provides a flexible and efficient random number generation utility
 * with support for various integer types, ranges, and special constraints.
 * It leverages C++20 features for improved performance and usability.
 *
 * For the conversion of 32-bit pseudo-random numbers to 8, 16, 64 bit values,
 * simple bit operations are used for efficiency. More cryptographically secure
 * operations would be recommended for security-critical applications.
 *
 * @author Meysam Zare
 * @date 2024-10-14
 */

namespace mz {
    /**
     * @class Randomizer
     * @brief A versatile random number generator based on the Mersenne Twister algorithm
     *
     * The Randomizer class provides methods for generating random integers of various
     * sizes (8-bit to 64-bit), with options for signed/unsigned values and specialized
     * constraints like non-zero, positive-only, or negative-only values.
     *
     * It also includes utilities for filling arrays and containers with random values,
     * generating random strings, and producing values within specific ranges.
     */
    class Randomizer {
    public:
        /**
         * @brief Alphanumeric character set used for string generation
         *
         * Includes lowercase letters, digits, and uppercase letters
         */
        static constexpr char const AlphaNumeric[] =
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

        /**
         * @brief Number of characters in the alphanumeric set
         *
         * Size minus 1 to exclude the null terminator
         */
        static constexpr size_t NumAlphaNumeric{ sizeof(AlphaNumeric) - 1 };

        /**
         * @brief Type definition for the Mersenne Twister engine
         *
         * Uses standard MT19937 parameters for high-quality randomness
         */
        using engine_type = std::mt19937;

        // ---- Distribution types for optimized generation ----

        /**
         * @brief Full-range distributions for different integer types
         *
         * Pre-defined distribution objects improve performance by avoiding
         * repeated construction of distribution objects
         */
        using dist_uint8 = std::uniform_int_distribution<uint16_t>;
        using dist_uint16 = std::uniform_int_distribution<uint16_t>;
        using dist_uint32 = std::uniform_int_distribution<uint32_t>;
        using dist_uint64 = std::uniform_int_distribution<uint64_t>;
        using dist_float = std::uniform_real_distribution<float>;
        using dist_double = std::uniform_real_distribution<double>;

    private:
        // ---- Core random generation state ----
        engine_type m_engine;                  ///< Mersenne Twister engine instance
        uint32_t m_seed{ 0 };                    ///< Current seed value
        std::random_device m_randomDevice;     ///< Hardware random device for seeding

        // ---- Pre-initialized distribution objects ----
        dist_uint8 m_dist8{ 0, UINT8_MAX };      ///< 8-bit uniform distribution
        dist_uint16 m_dist16{ 0, UINT16_MAX };   ///< 16-bit uniform distribution
        dist_uint32 m_dist32{ 0, UINT32_MAX };   ///< 32-bit uniform distribution
        dist_float m_distFloat{ 0.0f, 1.0f };    ///< Float distribution [0,1]
        dist_double m_distDouble{ 0.0, 1.0 };    ///< Double distribution [0,1]

        // ---- Thread safety ----
        std::mutex m_mutex;                    ///< Mutex for thread-safe operations

        /**
         * @brief Update the random engine seed
         *
         * @param shift Value to shift the seed by
         * @param reSeed Whether to reseed from hardware random device
         */
        void updateSeed(uint32_t shift, bool reSeed) noexcept {
            if (reSeed && m_randomDevice.entropy() > 0.0) {
                m_seed = m_randomDevice();
            }
            else {
                m_seed += shift;
            }
            m_engine.seed(m_seed);
        }

    public:
        /**
         * @brief Default constructor
         *
         * Initializes the random engine with default state
         */
        Randomizer() noexcept = default;

        /**
         * @brief Constructor with explicit seed
         *
         * @param seed Initial seed value for the random engine
         */
        explicit Randomizer(uint32_t seed) noexcept : m_seed{ seed } {
            m_engine.seed(seed);
        }

        /**
         * @brief Get a random value from the hardware random device
         *
         * @return Random value from hardware device or 0 if not available
         */
        [[nodiscard]] uint32_t getHardwareRandom() noexcept {
            if (m_randomDevice.entropy() > 0.0) {
                return m_randomDevice();
            }
            return 0;
        }

        /**
         * @brief Seed the generator with a hardware-derived value
         *
         * Uses a default seed boost of 137 (a prime number)
         */
        void seed() noexcept {
            m_seed = 0;
            updateSeed(137, true);
        }

        /**
         * @brief Seed the generator with a specific value
         *
         * @param seed Value to seed the generator with
         */
        void seed(uint32_t seed) noexcept {
            m_seed = 0;
            updateSeed(seed, false);
        }

        // ---- Thread-safe variants ----

        /**
         * @brief Thread-safe version of seed()
         */
        void seedThreadSafe() noexcept {
            std::lock_guard<std::mutex> lock(m_mutex);
            seed();
        }

        /**
         * @brief Thread-safe version of seed(uint32_t)
         *
         * @param seed Value to seed the generator with
         */
        void seedThreadSafe(uint32_t seed) noexcept {
            std::lock_guard<std::mutex> lock(m_mutex);
            this->seed(seed);
        }

        // ---- Core random number generation ----

        /**
         * @brief Generate a 32-bit unsigned random integer
         *
         * @param reSeed Whether to reseed the generator
         * @return Random 32-bit unsigned integer
         */
        [[nodiscard]] uint32_t rand32(bool reSeed = false) noexcept {
            uint32_t result = m_dist32(m_engine);
            if (reSeed) {
                updateSeed(result, reSeed);
            }
            return result;
        }

        /**
         * @brief Thread-safe variant of rand32
         *
         * @param reSeed Whether to reseed the generator
         * @return Random 32-bit unsigned integer
         */
        [[nodiscard]] uint32_t rand32ThreadSafe(bool reSeed = false) noexcept {
            std::lock_guard<std::mutex> lock(m_mutex);
            return rand32(reSeed);
        }

        // ---- Unsigned random number generators ----

        /**
         * @brief Generate an 8-bit unsigned random integer
         *
         * Uses a pre-defined distribution for better randomness than bit shifting
         *
         * @param reSeed Whether to reseed the generator
         * @return Random 8-bit unsigned integer
         */
        [[nodiscard]] uint8_t rand8(bool reSeed = false) noexcept {
            return static_cast<uint8_t>(m_dist8(m_engine));
        }

        /**
         * @brief Generate a 16-bit unsigned random integer
         *
         * @param reSeed Whether to reseed the generator
         * @return Random 16-bit unsigned integer
         */
        [[nodiscard]] uint16_t rand16(bool reSeed = false) noexcept {
            return m_dist16(m_engine);
        }

        /**
         * @brief Generate a 64-bit unsigned random integer
         *
         * Combines two 32-bit values for full 64-bit coverage
         *
         * @param reSeed Whether to reseed the generator
         * @return Random 64-bit unsigned integer
         */
        [[nodiscard]] uint64_t rand64(bool reSeed = false) noexcept {
            uint64_t high = static_cast<uint64_t>(rand32(reSeed)) << 32;
            uint64_t low = static_cast<uint64_t>(rand32(reSeed));
            return high | low;
        }

        // ---- Signed random number generators ----

        /**
         * @brief Generate an 8-bit signed random integer
         *
         * @param reSeed Whether to reseed the generator
         * @return Random 8-bit signed integer
         */
        [[nodiscard]] int8_t irand8(bool reSeed = false) noexcept {
            return static_cast<int8_t>(rand8(reSeed));
        }

        /**
         * @brief Generate a 16-bit signed random integer
         *
         * @param reSeed Whether to reseed the generator
         * @return Random 16-bit signed integer
         */
        [[nodiscard]] int16_t irand16(bool reSeed = false) noexcept {
            return static_cast<int16_t>(rand16(reSeed));
        }

        /**
         * @brief Generate a 32-bit signed random integer
         *
         * @param reSeed Whether to reseed the generator
         * @return Random 32-bit signed integer
         */
        [[nodiscard]] int32_t irand32(bool reSeed = false) noexcept {
            return static_cast<int32_t>(rand32(reSeed));
        }

        /**
         * @brief Generate a 64-bit signed random integer
         *
         * @param reSeed Whether to reseed the generator
         * @return Random 64-bit signed integer
         */
        [[nodiscard]] int64_t irand64(bool reSeed = false) noexcept {
            return static_cast<int64_t>(rand64(reSeed));
        }

        // ---- Floating-point random number generators ----

        /**
         * @brief Generate a random float in range [0,1]
         *
         * @param reSeed Whether to reseed the generator
         * @return Random float between 0 and 1 inclusive
         */
        [[nodiscard]] float randf(bool reSeed = false) noexcept {
            float result = m_distFloat(m_engine);
            if (reSeed) {
                updateSeed(static_cast<uint32_t>(result * UINT32_MAX), reSeed);
            }
            return result;
        }

        /**
         * @brief Generate a random double in range [0,1]
         *
         * @param reSeed Whether to reseed the generator
         * @return Random double between 0 and 1 inclusive
         */
        [[nodiscard]] double randd(bool reSeed = false) noexcept {
            double result = m_distDouble(m_engine);
            if (reSeed) {
                updateSeed(static_cast<uint32_t>(result * UINT32_MAX), reSeed);
            }
            return result;
        }

        // ---- Specialized random number generators ----

        /**
         * @brief Generate a non-zero 8-bit signed random integer
         *
         * @param reSeed Whether to reseed the generator
         * @return Random non-zero 8-bit signed integer
         */
        [[nodiscard]] int8_t i8nz(bool reSeed = false) noexcept {
            int8_t x;
            do {
                x = irand8(reSeed);
            } while (!x);
            return x;
        }

        /**
         * @brief Generate a non-zero 16-bit signed random integer
         *
         * @param reSeed Whether to reseed the generator
         * @return Random non-zero 16-bit signed integer
         */
        [[nodiscard]] int16_t i16nz(bool reSeed = false) noexcept {
            int16_t x;
            do {
                x = irand16(reSeed);
            } while (!x);
            return x;
        }

        /**
         * @brief Generate a non-zero 32-bit signed random integer
         *
         * @param reSeed Whether to reseed the generator
         * @return Random non-zero 32-bit signed integer
         */
        [[nodiscard]] int32_t i32nz(bool reSeed = false) noexcept {
            int32_t x;
            do {
                x = irand32(reSeed);
            } while (!x);
            return x;
        }

        /**
         * @brief Generate a non-zero 64-bit signed random integer
         *
         * @param reSeed Whether to reseed the generator
         * @return Random non-zero 64-bit signed integer
         */
        [[nodiscard]] int64_t i64nz(bool reSeed = false) noexcept {
            int64_t x;
            do {
                x = irand64(reSeed);
            } while (!x);
            return x;
        }

        /**
         * @brief Generate a positive 8-bit signed random integer
         *
         * @param reSeed Whether to reseed the generator
         * @return Random positive 8-bit signed integer
         */
        [[nodiscard]] int8_t i8pos(bool reSeed = false) noexcept {
            int8_t x;
            do {
                x = irand8(reSeed);
            } while (x <= 0);
            return x;
        }

        /**
         * @brief Generate a positive 16-bit signed random integer
         *
         * @param reSeed Whether to reseed the generator
         * @return Random positive 16-bit signed integer
         */
        [[nodiscard]] int16_t i16pos(bool reSeed = false) noexcept {
            int16_t x;
            do {
                x = irand16(reSeed);
            } while (x <= 0);
            return x;
        }

        /**
         * @brief Generate a positive 32-bit signed random integer
         *
         * @param reSeed Whether to reseed the generator
         * @return Random positive 32-bit signed integer
         */
        [[nodiscard]] int32_t i32pos(bool reSeed = false) noexcept {
            int32_t x;
            do {
                x = irand32(reSeed);
            } while (x <= 0);
            return x;
        }

        /**
         * @brief Generate a positive 64-bit signed random integer
         *
         * @param reSeed Whether to reseed the generator
         * @return Random positive 64-bit signed integer
         */
        [[nodiscard]] int64_t i64pos(bool reSeed = false) noexcept {
            int64_t x;
            do {
                x = irand64(reSeed);
            } while (x <= 0);
            return x;
        }

        /**
         * @brief Generate a negative 8-bit signed random integer
         *
         * @param reSeed Whether to reseed the generator
         * @return Random negative 8-bit signed integer
         */
        [[nodiscard]] int8_t i8neg(bool reSeed = false) noexcept {
            int8_t x;
            do {
                x = irand8(reSeed);
            } while (x >= 0);
            return x;
        }

        /**
         * @brief Generate a negative 16-bit signed random integer
         *
         * @param reSeed Whether to reseed the generator
         * @return Random negative 16-bit signed integer
         */
        [[nodiscard]] int16_t i16neg(bool reSeed = false) noexcept {
            int16_t x;
            do {
                x = irand16(reSeed);
            } while (x >= 0);
            return x;
        }

        /**
         * @brief Generate a negative 32-bit signed random integer
         *
         * @param reSeed Whether to reseed the generator
         * @return Random negative 32-bit signed integer
         */
        [[nodiscard]] int32_t i32neg(bool reSeed = false) noexcept {
            int32_t x;
            do {
                x = irand32(reSeed);
            } while (x >= 0);
            return x;
        }

        /**
         * @brief Generate a negative 64-bit signed random integer
         *
         * @param reSeed Whether to reseed the generator
         * @return Random negative 64-bit signed integer
         */
        [[nodiscard]] int64_t i64neg(bool reSeed = false) noexcept {
            int64_t x;
            do {
                x = irand64(reSeed);
            } while (x >= 0);
            return x;
        }

        // ---- Function call operators ----

        /**
         * @brief Function call operator for generating a 32-bit random integer
         *
         * Allows using the Randomizer object as a function
         *
         * @param reSeed Whether to reseed the generator
         * @return Random 32-bit unsigned integer
         */
        [[nodiscard]] uint32_t operator()(bool reSeed = false) noexcept {
            return rand32(reSeed);
        }

        /**
         * @brief Function call operator for filling a variable with a random value
         *
         * @tparam T Integral type to fill with a random value
         * @param t Reference to the variable to fill
         * @param reSeed Whether to reseed the generator
         */
        template <std::integral T>
        void operator()(T& t, bool reSeed = false) noexcept {
            t = static_cast<T>(rand32(reSeed));
        }

        // ---- Range-based random number generation ----

        /**
         * @brief Generate a random integer within a specified range [min,max]
         *
         * @tparam T Integral type of the range bounds and result
         * @param min Lower bound (inclusive)
         * @param max Upper bound (inclusive)
         * @param reSeed Whether to reseed the generator
         * @return Random value within the specified range
         */
        template <std::integral T>
        [[nodiscard]] T range(T min, T max, bool reSeed = false) noexcept {
            if (min >= max) return min;

            // Create a distribution for this range
            std::uniform_int_distribution<T> dist(min, max);

            // Get a result and possibly reseed
            T result = dist(m_engine);
            if (reSeed) {
                updateSeed(static_cast<uint32_t>(result), reSeed);
            }

            return result;
        }

        /**
         * @brief Generate a floating-point random value within a specified range [min,max]
         *
         * @tparam T Floating-point type of the range bounds and result
         * @param min Lower bound (inclusive)
         * @param max Upper bound (inclusive)
         * @param reSeed Whether to reseed the generator
         * @return Random value within the specified range
         */
        template <std::floating_point T>
        [[nodiscard]] T range(T min, T max, bool reSeed = false) noexcept {
            if (min >= max) return min;

            // Create a distribution for this range
            std::uniform_real_distribution<T> dist(min, max);

            // Get a result and possibly reseed
            T result = dist(m_engine);
            if (reSeed) {
                updateSeed(static_cast<uint32_t>(result * 1000000), reSeed);
            }

            return result;
        }

        /**
         * @brief Generate a 32-bit unsigned integer within a specified range [min,max]
         *
         * @param min Lower bound (inclusive)
         * @param max Upper bound (inclusive)
         * @param reSeed Whether to reseed the generator
         * @return Random value within the specified range
         */
        [[nodiscard]] uint32_t range32(uint32_t min, uint32_t max, bool reSeed = false) noexcept {
            return range<uint32_t>(min, max, reSeed);
        }

        // ---- Container randomization ----

        /**
         * @brief Fill a span with random values
         *
         * Optimized for different element sizes to maximize performance
         *
         * @tparam T Element type (must be integral)
         * @tparam N Size of the span (deduced)
         * @param span Span to fill with random values
         * @param reSeed Whether to reseed the generator
         */
        template <std::integral T, size_t N>
        void randomize(std::span<T, N> span, bool reSeed = false) noexcept {
            if constexpr (sizeof(T) == 1) {
                // Optimize for 8-bit values by extracting 4 values from each 32-bit random number
                uint32_t buffer{ 0 };
                int remainingBytes = 0;

                for (auto& x : span) {
                    if (remainingBytes == 0) {
                        buffer = rand32(reSeed);
                        remainingBytes = 4;
                    }

                    x = static_cast<T>(buffer & 0xFF);
                    buffer >>= 8;
                    remainingBytes--;
                }
            }
            else if constexpr (sizeof(T) == 2) {
                // Optimize for 16-bit values by extracting 2 values from each 32-bit random number
                uint32_t buffer{ 0 };
                int remainingShorts = 0;

                for (auto& x : span) {
                    if (remainingShorts == 0) {
                        buffer = rand32(reSeed);
                        remainingShorts = 2;
                    }

                    x = static_cast<T>(buffer & 0xFFFF);
                    buffer >>= 16;
                    remainingShorts--;
                }
            }
            else if constexpr (sizeof(T) == 8) {
                // Use rand64 directly for 64-bit values
                for (auto& x : span) {
                    x = static_cast<T>(rand64(reSeed));
                }
            }
            else {
                // Default case for 32-bit and other sizes
                for (auto& x : span) {
                    x = static_cast<T>(rand32(reSeed));
                }
            }
        }

        /**
         * @brief Fill a C-style array with random values
         *
         * @tparam T Element type (must be integral)
         * @tparam N Size of the array
         * @param array Array to fill with random values
         * @param reSeed Whether to reseed the generator
         */
        template<std::integral T, size_t N>
        void randomize(T(&array)[N], bool reSeed = false) noexcept {
            randomize(std::span(array), reSeed);
        }

        /**
         * @brief Fill a vector with random values
         *
         * @tparam T Element type (must be integral)
         * @param vec Vector to fill with random values
         * @param reSeed Whether to reseed the generator
         */
        template<std::integral T>
        void randomize(std::vector<T>& vec, bool reSeed = false) noexcept {
            randomize(std::span(vec), reSeed);
        }

        /**
         * @brief Shuffle the elements in a span
         *
         * Uses Fisher-Yates algorithm for efficient shuffling
         *
         * @tparam T Element type
         * @tparam N Size of the span (deduced)
         * @param span Span to shuffle
         * @param reSeed Whether to reseed the generator
         */
        template <typename T, size_t N>
        void shuffle(std::span<T, N> span, bool reSeed = false) noexcept {
            if (span.empty()) return;

            for (size_t i = span.size() - 1; i > 0; --i) {
                size_t j = range32(0, static_cast<uint32_t>(i), reSeed);
                std::swap(span[i], span[j]);
            }
        }

        /**
         * @brief Shuffle a C-style array
         *
         * @tparam T Element type
         * @tparam N Size of the array
         * @param array Array to shuffle
         * @param reSeed Whether to reseed the generator
         */
        template<typename T, size_t N>
        void shuffle(T(&array)[N], bool reSeed = false) noexcept {
            shuffle(std::span(array), reSeed);
        }

        /**
         * @brief Shuffle a vector
         *
         * @tparam T Element type
         * @param vec Vector to shuffle
         * @param reSeed Whether to reseed the generator
         */
        template<typename T>
        void shuffle(std::vector<T>& vec, bool reSeed = false) noexcept {
            shuffle(std::span(vec), reSeed);
        }

        // ---- String generation ----

        /**
         * @brief Fill a span of characters with random alphanumeric values
         *
         * @param span Span to fill with random characters
         * @param reSeed Whether to reseed the generator
         */
        void alphanumeric(std::span<char> span, bool reSeed = false) noexcept {
            uint32_t buffer = rand32(reSeed);
            unsigned int bits = 32; // Track available bits

            for (auto& ch : span) {
                // We need ~6 bits per character (64 possible values)
                // Regenerate when we have fewer than 6 bits left
                if (bits < 6) {
                    buffer = rand32(reSeed);
                    bits = 32;
                }

                // Use lowest 6 bits for character index
                ch = AlphaNumeric[buffer & 0x3F]; // 0x3F = 63 (6 bits)
                buffer >>= 6;
                bits -= 6;
            }
        }

        /**
         * @brief Fill a span of wide characters with random alphanumeric values
         *
         * @param span Span to fill with random characters
         * @param reSeed Whether to reseed the generator
         */
        void alphanumeric(std::span<wchar_t> span, bool reSeed = false) noexcept {
            uint32_t buffer = rand32(reSeed);
            unsigned int bits = 32; // Track available bits

            for (auto& ch : span) {
                // We need ~6 bits per character (64 possible values)
                // Regenerate when we have fewer than 6 bits left
                if (bits < 6) {
                    buffer = rand32(reSeed);
                    bits = 32;
                }

                // Use lowest 6 bits for character index
                ch = static_cast<wchar_t>(AlphaNumeric[buffer & 0x3F]); // 0x3F = 63 (6 bits)
                buffer >>= 6;
                bits -= 6;
            }
        }

        /**
         * @brief Fill a character array with random alphanumeric values
         *
         * @tparam N Size of the array
         * @param array Array to fill with random characters
         * @param reSeed Whether to reseed the generator
         */
        template<size_t N>
        void alphanumeric(char(&array)[N], bool reSeed = false) noexcept {
            alphanumeric(std::span(array, N - 1), reSeed); // Exclude null terminator
            array[N - 1] = '\0'; // Ensure null termination
        }

        /**
         * @brief Fill a wide character array with random alphanumeric values
         *
         * @tparam N Size of the array
         * @param array Array to fill with random characters
         * @param reSeed Whether to reseed the generator
         */
        template<size_t N>
        void alphanumeric(wchar_t(&array)[N], bool reSeed = false) noexcept {
            alphanumeric(std::span(array, N - 1), reSeed); // Exclude null terminator
            array[N - 1] = L'\0'; // Ensure null termination
        }

        /**
         * @brief Generate a string of random alphanumeric characters
         *
         * @param length Length of the string to generate
         * @param reSeed Whether to reseed the generator
         * @return String of random alphanumeric characters
         */
        [[nodiscard]] std::string string(size_t length, bool reSeed = false) noexcept {
            std::string result(length, '\0');
            if (length > 0) {
                alphanumeric(std::span(result), reSeed);
            }
            return result;
        }

        /**
         * @brief Generate a wide string of random alphanumeric characters
         *
         * @param length Length of the string to generate
         * @param reSeed Whether to reseed the generator
         * @return Wide string of random alphanumeric characters
         */
        [[nodiscard]] std::wstring wstring(size_t length, bool reSeed = false) noexcept {
            std::wstring result(length, L'\0');
            if (length > 0) {
                alphanumeric(std::span(result), reSeed);
            }
            return result;
        }

        /**
         * @brief Fill an existing string with random alphanumeric characters
         *
         * @param str String to fill with random characters
         * @param reSeed Whether to reseed the generator
         */
        void alphanumeric(std::string& str, bool reSeed = false) noexcept {
            alphanumeric(std::span(str), reSeed);
        }

        /**
         * @brief Fill an existing wide string with random alphanumeric characters
         *
         * @param str Wide string to fill with random characters
         * @param reSeed Whether to reseed the generator
         */
        void alphanumeric(std::wstring& str, bool reSeed = false) noexcept {
            alphanumeric(std::span(str), reSeed);
        }

        /**
         * @brief Thread-safe version of string generation
         *
         * @param length Length of the string to generate
         * @param reSeed Whether to reseed the generator
         * @return String of random alphanumeric characters
         */
        [[nodiscard]] std::string stringThreadSafe(size_t length, bool reSeed = false) noexcept {
            std::lock_guard<std::mutex> lock(m_mutex);
            return string(length, reSeed);
        }
    };
}

#endif // MZ_RANDOMIZER_HEADER_FILE
