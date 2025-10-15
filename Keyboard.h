/*
MIT License

Copyright (c) 2021 Meysam Zare

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

#pragma once

#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#endif

#ifndef WINVER
#define WINVER 0x0A00
#endif

#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#include <array>
#include <string>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

// Platform-specific includes
#if defined(_WIN32)
#include <conio.h>
#include <Windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#endif

namespace mz {

    /**
     * @brief Cross-platform keyboard input utility for console applications.
     *
     * Keyboard2 provides methods for non-blocking character reading, interactive
     * string input, and numeric input with proper validation and visual feedback.
     * The class supports Windows, Linux, and macOS platforms through conditional
     * compilation and platform-specific implementations.
     */
    class Keyboard {
    private:
        // Track state of number keys (0-9) for edge detection
        std::array<bool, 10> oldKeys{};
        std::array<bool, 10> newKeys{};

        //------------------------------------------------------------------------
        // Platform-specific implementations
        //------------------------------------------------------------------------
#if defined(_WIN32)
    /**
     * @brief Windows implementation of getch.
     * @return The character code read from console input.
     */
        static inline int getch() noexcept {
            return _getch();
        }

        /**
         * @brief Windows implementation of keyboard hit check.
         * @return true if a key has been pressed, false otherwise.
         */
        static inline bool kbhit() noexcept {
            return _kbhit();
        }
#elif defined(__linux__) || defined(__APPLE__)
    /**
     * @brief POSIX implementation of getch.
     * @return The character code read from console input.
     *
     * Sets terminal to raw mode to read a character without echo or buffering,
     * then restores the original terminal settings.
     */
        static int getch() noexcept {
            struct termios oldSettings, newSettings;
            int ch;

            // Save current terminal settings
            tcgetattr(STDIN_FILENO, &oldSettings);

            // Disable canonical mode and echo
            newSettings = oldSettings;
            newSettings.c_lflag &= ~(ICANON | ECHO);
            tcsetattr(STDIN_FILENO, TCSANOW, &newSettings);

            // Read character
            ch = getchar();

            // Restore terminal settings
            tcsetattr(STDIN_FILENO, TCSANOW, &oldSettings);

            return ch;
        }

        /**
         * @brief POSIX implementation of keyboard hit check.
         * @return true if a key has been pressed, false otherwise.
         *
         * Temporarily sets terminal to non-blocking mode to check if
         * input is available, then restores the original settings.
         */
        static bool kbhit() noexcept {
            struct termios oldSettings, newSettings;
            int oldFlags;
            int ch;
            bool result = false;

            // Save current terminal settings
            tcgetattr(STDIN_FILENO, &oldSettings);
            newSettings = oldSettings;
            newSettings.c_lflag &= ~(ICANON | ECHO);
            tcsetattr(STDIN_FILENO, TCSANOW, &newSettings);

            // Save current file descriptor flags
            oldFlags = fcntl(STDIN_FILENO, F_GETFL, 0);

            // Set non-blocking mode
            fcntl(STDIN_FILENO, F_SETFL, oldFlags | O_NONBLOCK);

            // Try to read a character
            ch = getchar();

            // If a character was available, put it back
            if (ch != EOF) {
                ungetc(ch, stdin);
                result = true;
            }

            // Restore settings
            tcsetattr(STDIN_FILENO, TCSANOW, &oldSettings);
            fcntl(STDIN_FILENO, F_SETFL, oldFlags);

            return result;
        }
#else
#error "Unsupported platform - Keyboard2 requires Windows, Linux, or macOS"
#endif

    //------------------------------------------------------------------------
    // Character classification helpers
    //------------------------------------------------------------------------

    /**
     * @brief Check if character is a return/enter key.
     * @param c Character code to check.
     * @return true if return key, false otherwise.
     */
        static constexpr bool isReturn(int c) noexcept {
            return c == '\n' || c == '\r';
        }

        /**
         * @brief Check if character is a digit (0-9).
         * @param c Character code to check.
         * @return true if digit, false otherwise.
         */
        static constexpr bool isDigit(int c) noexcept {
            return c >= '0' && c <= '9';
        }

        /**
         * @brief Check if character is a printable ASCII character.
         * @param c Character code to check.
         * @return true if printable, false otherwise.
         */
        static constexpr bool isPrintable(int c) noexcept {
            return c >= 32 && c <= 126;
        }

        /**
         * @brief Check if character is a special/extended key code.
         * @param c Character code to check.
         * @return true if special key, false otherwise.
         */
        static constexpr bool isSpecial(int c) noexcept {
            return c == 0 || c == 0xE0;
        }

        /**
         * @brief Check if character is escape or control-C.
         * @param c Character code to check.
         * @return true if escape or control-C, false otherwise.
         */
        static constexpr bool isEscape(int c) noexcept {
            return c == 3 || c == 27;
        }

        /**
         * @brief Check if character is backspace.
         * @param c Character code to check.
         * @return true if backspace, false otherwise.
         *
         * Handles both Windows (8) and POSIX (127) backspace codes.
         */
        static constexpr bool isBackspace(int c) noexcept {
            return c == 8 || c == 127;
        }

    public:
        /**
         * @brief Non-blocking character read.
         * @return Character code if a key is pressed, 0 otherwise.
         *
         * Checks if a key has been pressed without blocking, and returns
         * its code if available.
         */
        int getc() noexcept {
            if (kbhit()) {
                return getch();
            }
            return 0;
        }

        /**
         * @brief Detect newly pressed number keys.
         * @return Index (0-9) of first newly pressed key, or 10 if none.
         *
         * On Windows: Detects when a number key is newly pressed (i.e., was not
         * pressed on previous call but is pressed now).
         * On other platforms: Always returns 10 (not supported).
         */
        int getc2() noexcept {
#if defined(_WIN32)
            // Update key states
            for (size_t i = 0; i < oldKeys.size(); ++i) {
                oldKeys[i] = newKeys[i];
            }

            // Only check keys when console is in foreground
            if (GetForegroundWindow() == GetConsoleWindow()) {
                for (int digit = 0; digit < 10; ++digit) {
                    // Check if key is currently pressed (high bit set)
                    newKeys[digit] = (GetAsyncKeyState('0' + digit) & 0x8000) != 0;
                }
            }

            // Clear any buffered input
            FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));

            // Return first key that changed from not-pressed to pressed
            for (int i = 0; i < 10; ++i) {
                if (newKeys[i] && !oldKeys[i]) {
                    return i;
                }
            }
#endif
            // Default return (no new key or not on Windows)
            return 10;
        }

        /**
         * @brief Read a string with interactive input and editing.
         * @param str Reference to string to fill.
         * @return Number of characters entered.
         *
         * Reads a string character by character, displaying each character as typed.
         * Supports backspace for editing and escape to cancel input.
         * Limits input to printable ASCII characters.
         */
        size_t get(std::string& str) const noexcept {
            // Determine maximum size (use existing capacity or default to 1024)
            const size_t maxSize = str.capacity() ? str.capacity() : 1024;

            // Clear the string to start fresh
            str.clear();

            // Input loop
            while (true) {
                int ch = getch();

                // Handle regular printable characters
                if (isPrintable(ch) && str.size() < maxSize) {
                    // Echo the character
                    char c = static_cast<char>(ch);
                    fwrite(&c, 1, 1, stdout);
                    fflush(stdout);

                    // Add to string
                    str.push_back(c);
                }
                // Handle backspace
                else if (isBackspace(ch) && !str.empty()) {
                    // Remove last character from string
                    str.pop_back();

                    // Erase character from display (backspace, space, backspace)
                    const char eraseSeq[3] = { 8, ' ', 8 };
                    fwrite(eraseSeq, 1, 3, stdout);
                    fflush(stdout);
                }
                // Handle enter/return (finish input)
                else if (isReturn(ch)) {
                    // Output newline
                    const char nl = '\n';
                    fwrite(&nl, 1, 1, stdout);
                    fflush(stdout);
                    break;
                }
                // Handle escape (cancel input)
                else if (isEscape(ch)) {
                    // Erase all characters from display
                    const char eraseSeq[3] = { 8, ' ', 8 };
                    for (size_t i = 0; i < str.size(); ++i) {
                        fwrite(eraseSeq, 1, 3, stdout);
                    }

                    // Output newline and clear string
                    const char nl = '\n';
                    fwrite(&nl, 1, 1, stdout);
                    fflush(stdout);
                    str.clear();
                    break;
                }
                // Handle special keys that need another character read
                else if (isSpecial(ch)) {
                    getch(); // Consume the second byte of the sequence
                }
            }

            return str.size();
        }

        /**
         * @brief Read an unsigned integer with interactive input.
         * @param numDigits Maximum number of digits to read, updated to actual count.
         * @return The numeric value entered.
         *
         * Reads digits from the console, allowing backspace for editing.
         * Returns 0 if no valid digits entered.
         */
        uint64_t uget(int& numDigits) const noexcept {
            // Input validation and limits
            if (numDigits <= 0) {
                numDigits = 0;
                return 0;
            }

            const int maxDigits = 16;
            if (numDigits > maxDigits) {
                numDigits = maxDigits;
            }

            // Digit buffer and position
            std::array<uint8_t, maxDigits> digits{};
            int pos = 0;

            // Input loop
            while (true) {
                int ch = getch();

                // Handle digits
                if (isDigit(ch) && pos < numDigits) {
                    char c = static_cast<char>(ch);
                    digits[pos++] = c - '0';

                    // Echo digit
                    fwrite(&c, 1, 1, stdout);
                    fflush(stdout);
                }
                // Handle backspace
                else if (isBackspace(ch) && pos > 0) {
                    --pos;
                    digits[pos] = 0;

                    // Erase character from display
                    const char eraseSeq[3] = { 8, ' ', 8 };
                    fwrite(eraseSeq, 1, 3, stdout);
                    fflush(stdout);
                }
                // Handle enter/return (finish input)
                else if (isReturn(ch)) {
                    const char nl = '\n';
                    fwrite(&nl, 1, 1, stdout);
                    fflush(stdout);
                    break;
                }
                // Handle escape (cancel input)
                else if (isEscape(ch)) {
                    const char eraseSeq[3] = { 8, ' ', 8 };

                    // Erase all digits from display
                    while (pos > 0) {
                        --pos;
                        digits[pos] = 0;
                        fwrite(eraseSeq, 1, 3, stdout);
                    }

                    const char nl = '\n';
                    fwrite(&nl, 1, 1, stdout);
                    fflush(stdout);
                    break;
                }
                // Handle special keys
                else if (isSpecial(ch)) {
                    getch(); // Consume the second byte
                }
            }

            // Convert digits to numeric value
            uint64_t value = 0;
            for (int i = 0; i < pos; ++i) {
                value = (value * 10) + digits[i];
            }

            // Update actual number of digits entered
            numDigits = pos;
            return value;
        }

        /**
         * @brief Read an unsigned integer value.
         * @param value Reference where the value will be stored.
         * @return true if any digits were entered, false otherwise.
         */
        bool get(unsigned& value) const noexcept {
            int numDigits = 8;  // Maximum 8 digits for unsigned int
            value = static_cast<unsigned>(uget(numDigits));
            return numDigits > 0;
        }

        /**
         * @brief Read a uint64_t value.
         * @param value Reference where the value will be stored.
         * @return true if any digits were entered, false otherwise.
         */
        bool get(uint64_t& value) const noexcept {
            int numDigits = 16;  // Maximum 16 digits for uint64_t
            value = uget(numDigits);
            return numDigits > 0;
        }
    };

} // namespace mz
