#ifndef MZ_UTILITIES_ENCODER64_HEADER_FILE
#define MZ_UTILITIES_ENCODER64_HEADER_FILE
#pragma once

#include <string>
#include <concepts>

/*
*  encode64.h
*  A simple 64-character encoder implementation.
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
    ---------
    Encoder64 provides a static interface for encoding integral values into a
    string using a 64-character alphabet (similar to Base64, but not for binary data).
    Each 6 bits of the input value are mapped to a character in the alphabet.

    Usage:
    ------
    std::string encoded = mz::Encoder64::to_string(123456u);
    std::string encoded2 = mz::Encoder64{}(123456u);

    Features:
    ---------
    - Supports all std::integral types (uint8_t, uint16_t, uint32_t, uint64_t, etc.).
    - Compile-time constexpr encoding for constant expressions.
    - Operator() for convenient functor-style usage.
*/

namespace mz {

    class Encoder64 {
        // 64-character alphabet for encoding (A-Z, a-z, 0-9, +, /)
        static constexpr char alphabet[64]{
            'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
            'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
            'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
            'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
        };

        // Maps the lower 6 bits of x to a character in the alphabet.
        static constexpr char encode_char(auto value) noexcept {
            return alphabet[value & 0x3F];
        }

    public:
        /*
            Converts an integral value to a string using the 64-character alphabet.
            Each 6 bits of the input are mapped to a character.
            The output length depends on the size of the input type:
            - 8 bits: 2 chars
            - 16 bits: 3 chars
            - 32 bits: 6 chars
            - 64 bits: 11 chars
        */
        template <std::integral T>
        static constexpr std::string to_string(T value) noexcept {
            if constexpr (sizeof(T) == 1) {
                // 8 bits: 2 characters
                return std::string{ encode_char(value >> 6), encode_char(value) };
            }
            else if constexpr (sizeof(T) == 2) {
                // 16 bits: 3 characters
                return std::string{ encode_char(value >> 12), encode_char(value >> 6), encode_char(value) };
            }
            else if constexpr (sizeof(T) == 4) {
                // 32 bits: 6 characters
                return std::string{
                    encode_char(value >> 30), encode_char(value >> 24),
                    encode_char(value >> 18), encode_char(value >> 12),
                    encode_char(value >> 6), encode_char(value)
                };
            }
            else {
                // 64 bits: 11 characters
                return std::string{
                    encode_char(value >> 60), encode_char(value >> 54),
                    encode_char(value >> 48), encode_char(value >> 42),
                    encode_char(value >> 36), encode_char(value >> 30),
                    encode_char(value >> 24), encode_char(value >> 18),
                    encode_char(value >> 12), encode_char(value >> 6),
                    encode_char(value)
                };
            }
        }

        /*
            Functor-style operator for encoding integral values.
            Example: Encoder64{}(12345u);
        */
        template <std::integral T>
        constexpr std::string operator()(T value) const noexcept {
            return to_string(value);
        }
    };

} // namespace mz

#endif // MZ_UTILITIES_ENCODER64_HEADER_FILE
