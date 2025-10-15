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

#ifndef MZ_TIME_CONVERSIONS_HEADER_FILE
#define MZ_TIME_CONVERSIONS_HEADER_FILE
#pragma once

#include <string>
#include <string_view>
#include <format>
#include <cstdint>
#include <type_traits>
#include <concepts>
#include <chrono>
#include <optional>
#include <limits>

#include "Encode64.h"

/**
 * @file time_conversions.h
 * @brief Comprehensive time handling utilities built on std::chrono
 *
 * This header provides convenience classes and functions for working with time in C++,
 * including time point conversions, formatting, and arithmetic operations. It includes
 * wrappers around system_clock and steady_clock with improved interfaces.
 *
 * The library features:
 * - Type aliases for common time_point types
 * - Conversion functions between different time units
 * - String formatting utilities for various time formats
 * - Time arithmetic operations
 * - Base64 encoding/decoding of time values
 *
 * @author Meysam Zare
 * @date 2024-10-14
 */
namespace mz {
    namespace time {

        //-----------------------------------------------------------------------------
        // Type aliases for std::chrono types
        //-----------------------------------------------------------------------------
        using namespace std::chrono;
        using std::chrono::system_clock;
        using std::chrono::steady_clock;

        /// @brief System clock time point with native precision
        using SystemTimePoint = time_point<system_clock, system_clock::duration>;
        /// @brief System clock time point with second precision
        using SecondTimePoint = time_point<system_clock, seconds>;
        /// @brief System clock time point with millisecond precision
        using MillisecondTimePoint = time_point<system_clock, milliseconds>;
        /// @brief System clock time point with microsecond precision
        using MicrosecondTimePoint = time_point<system_clock, microseconds>;
        /// @brief System clock time point with nanosecond precision
        using NanosecondTimePoint = time_point<system_clock, nanoseconds>;

        //-----------------------------------------------------------------------------
        // Concepts for template constraints
        //-----------------------------------------------------------------------------

        /**
         * @brief Concept for types that satisfy clock requirements
         */
        template<typename T>
        concept ClockType = requires {
            typename T::rep;    // This is the correct type requirement syntax
            typename T::period;
            typename T::duration;
            typename T::time_point;
            { T::now() } -> std::convertible_to<std::chrono::time_point<T>>;
        };


        /**
         * @brief Concept for types that satisfy duration requirements
         */
        template<typename T>
        concept DurationType = requires {
            typename T::rep;
            typename T::period;
            { std::declval<T>().count() } -> std::convertible_to<typename T::rep>;
                requires std::is_arithmetic_v<typename T::rep>;
                requires std::ratio_less_equal_v<typename T::period, std::ratio<86400>>;  // Reasonable max period (1 day)
        };


        /**
         * @brief Concept for types that satisfy time point requirements
         */
        template<typename T>
        concept TimePointType = requires {
            typename T::clock;
            typename T::duration;
            { std::declval<T>().time_since_epoch() } -> DurationType;
        };

        //-----------------------------------------------------------------------------
        // Time point creation functions
        //-----------------------------------------------------------------------------

        /**
         * @brief Creates a second-precision time point from seconds since epoch
         * @param secondsSinceEpoch Number of seconds since epoch
         * @return Time point with second precision
         */
        [[nodiscard]] inline constexpr SecondTimePoint createSecondTimePoint(int64_t secondsSinceEpoch) noexcept {
            return SecondTimePoint{ seconds{secondsSinceEpoch} };
        }

        /**
         * @brief Creates a millisecond-precision time point from milliseconds since epoch
         * @param millisecondsSinceEpoch Number of milliseconds since epoch
         * @return Time point with millisecond precision
         */
        [[nodiscard]] inline constexpr MillisecondTimePoint createMillisecondTimePoint(int64_t millisecondsSinceEpoch) noexcept {
            return MillisecondTimePoint{ milliseconds{millisecondsSinceEpoch} };
        }

        /**
         * @brief Creates a microsecond-precision time point from microseconds since epoch
         * @param microsecondsSinceEpoch Number of microseconds since epoch
         * @return Time point with microsecond precision
         */
        [[nodiscard]] inline constexpr MicrosecondTimePoint createMicrosecondTimePoint(int64_t microsecondsSinceEpoch) noexcept {
            return MicrosecondTimePoint{ microseconds{microsecondsSinceEpoch} };
        }

        /**
         * @brief Creates a nanosecond-precision time point from nanoseconds since epoch
         * @param nanosecondsSinceEpoch Number of nanoseconds since epoch
         * @return Time point with nanosecond precision
         */
        [[nodiscard]] inline constexpr NanosecondTimePoint createNanosecondTimePoint(int64_t nanosecondsSinceEpoch) noexcept {
            return NanosecondTimePoint{ nanoseconds{nanosecondsSinceEpoch} };
        }

        //-----------------------------------------------------------------------------
        // Time point conversion functions
        //-----------------------------------------------------------------------------

        /**
         * @brief Converts a time point from any clock to system_clock
         * @tparam Clock Source clock type
         * @tparam Duration Source duration type
         * @param timePoint Source time point
         * @return Equivalent time point using system_clock
         */
        template <ClockType Clock, DurationType Duration>
        [[nodiscard]] constexpr SystemTimePoint toSystemTimePoint(time_point<Clock, Duration> timePoint) noexcept {
            return clock_cast<system_clock>(timePoint);
        }

        /**
         * @brief Converts a time point to second precision
         * @tparam Clock Source clock type
         * @tparam Duration Source duration type
         * @param timePoint Source time point
         * @return Time point with second precision
         */
        template <ClockType Clock, DurationType Duration>
        [[nodiscard]] constexpr SecondTimePoint toSecondTimePoint(time_point<Clock, Duration> timePoint) noexcept {
            return time_point_cast<seconds>(toSystemTimePoint(timePoint));
        }

        /**
         * @brief Converts a time point to millisecond precision
         * @tparam Clock Source clock type
         * @tparam Duration Source duration type
         * @param timePoint Source time point
         * @return Time point with millisecond precision
         */
        template <ClockType Clock, DurationType Duration>
        [[nodiscard]] constexpr MillisecondTimePoint toMillisecondTimePoint(time_point<Clock, Duration> timePoint) noexcept {
            return time_point_cast<milliseconds>(toSystemTimePoint(timePoint));
        }

        /**
         * @brief Converts a time point to microsecond precision
         * @tparam Clock Source clock type
         * @tparam Duration Source duration type
         * @param timePoint Source time point
         * @return Time point with microsecond precision
         */
        template <ClockType Clock, DurationType Duration>
        [[nodiscard]] constexpr MicrosecondTimePoint toMicrosecondTimePoint(time_point<Clock, Duration> timePoint) noexcept {
            return time_point_cast<microseconds>(toSystemTimePoint(timePoint));
        }

        /**
         * @brief Converts a time point to nanosecond precision
         * @tparam Clock Source clock type
         * @tparam Duration Source duration type
         * @param timePoint Source time point
         * @return Time point with nanosecond precision
         */
        template <ClockType Clock, DurationType Duration>
        [[nodiscard]] constexpr NanosecondTimePoint toNanosecondTimePoint(time_point<Clock, Duration> timePoint) noexcept {
            return time_point_cast<nanoseconds>(toSystemTimePoint(timePoint));
        }

        //-----------------------------------------------------------------------------
        // Epoch time extraction functions
        //-----------------------------------------------------------------------------

        /**
         * @brief Gets the number of seconds since epoch for a time point
         * @tparam Clock Source clock type
         * @tparam Duration Source duration type
         * @param timePoint Source time point
         * @return Seconds since epoch
         */
        template <ClockType Clock, DurationType Duration>
        [[nodiscard]] constexpr int64_t getSecondsSinceEpoch(time_point<Clock, Duration> timePoint) noexcept {
            return time_point_cast<seconds>(toSystemTimePoint(timePoint)).time_since_epoch().count();
        }

        /**
         * @brief Gets the number of milliseconds since epoch for a time point
         * @tparam Clock Source clock type
         * @tparam Duration Source duration type
         * @param timePoint Source time point
         * @return Milliseconds since epoch
         */
        template <ClockType Clock, DurationType Duration>
        [[nodiscard]] constexpr int64_t getMillisecondsSinceEpoch(time_point<Clock, Duration> timePoint) noexcept {
            return time_point_cast<milliseconds>(toSystemTimePoint(timePoint)).time_since_epoch().count();
        }

        /**
         * @brief Gets the number of microseconds since epoch for a time point
         * @tparam Clock Source clock type
         * @tparam Duration Source duration type
         * @param timePoint Source time point
         * @return Microseconds since epoch
         */
        template <ClockType Clock, DurationType Duration>
        [[nodiscard]] constexpr int64_t getMicrosecondsSinceEpoch(time_point<Clock, Duration> timePoint) noexcept {
            return time_point_cast<microseconds>(toSystemTimePoint(timePoint)).time_since_epoch().count();
        }

        /**
         * @brief Gets the number of nanoseconds since epoch for a time point
         * @tparam Clock Source clock type
         * @tparam Duration Source duration type
         * @param timePoint Source time point
         * @return Nanoseconds since epoch
         */
        template <ClockType Clock, DurationType Duration>
        [[nodiscard]] constexpr int64_t getNanosecondsSinceEpoch(time_point<Clock, Duration> timePoint) noexcept {
            return time_point_cast<nanoseconds>(toSystemTimePoint(timePoint)).time_since_epoch().count();
        }

        //-----------------------------------------------------------------------------
        // Time string formatting functions
        //-----------------------------------------------------------------------------

        /**
         * @brief Formats a time point as a date string (YYYY-MM-DD)
         * @tparam Clock Source clock type
         * @tparam Duration Source duration type
         * @param timePoint Source time point
         * @return Formatted date string
         */
        template <ClockType Clock, DurationType Duration>
        [[nodiscard]] std::string formatDate(time_point<Clock, Duration> timePoint) noexcept {
            try {
                return std::format("{:%Y-%m-%d}", toSecondTimePoint(timePoint));
            }
            catch (...) {
                return "Invalid Date";
            }
        }

        /**
         * @brief Formats a time point as a date and time string (YYYY-MM-DD HH:MM:SS)
         * @tparam Clock Source clock type
         * @tparam Duration Source duration type
         * @param timePoint Source time point
         * @return Formatted date and time string
         */
        template <ClockType Clock, DurationType Duration>
        [[nodiscard]] std::string formatDateTime(time_point<Clock, Duration> timePoint) noexcept {
            try {
                return std::format("{:%Y-%m-%d %H:%M:%S}", toSecondTimePoint(timePoint));
            }
            catch (...) {
                return "Invalid DateTime";
            }
        }

        /**
         * @brief Formats a time point as an ISO 8601 string (YYYY-MM-DDTHH:MM:SS.sssZ)
         * @tparam Clock Source clock type
         * @tparam Duration Source duration type
         * @param timePoint Source time point
         * @return Formatted ISO 8601 string
         */
        template <ClockType Clock, DurationType Duration>
        [[nodiscard]] std::string formatISO8601(time_point<Clock, Duration> timePoint) noexcept {
            try {
                auto millisecs = toMillisecondTimePoint(timePoint);
                auto secs = time_point_cast<seconds>(millisecs);
                auto ms = duration_cast<milliseconds>(millisecs.time_since_epoch() - secs.time_since_epoch()).count();
                return std::format("{:%Y-%m-%dT%H:%M:%S}.{:03d}Z", secs, ms);
            }
            catch (...) {
                return "Invalid DateTime";
            }
        }

        /**
         * @brief Formats a time point as a file-friendly string (YYYY_MM_DD__HH_MM_SS)
         * @tparam Clock Source clock type
         * @tparam Duration Source duration type
         * @param timePoint Source time point
         * @return Formatted string suitable for filenames
         */
        template <ClockType Clock, DurationType Duration>
        [[nodiscard]] std::string formatFileTimestamp(time_point<Clock, Duration> timePoint) noexcept {
            try {
                return std::format("{:%Y_%m_%d__%H_%M_%S}", toSecondTimePoint(timePoint));
            }
            catch (...) {
                return "Invalid_DateTime";
            }
        }

        /**
         * @brief Formats a time point with a custom format string
         * @tparam Clock Source clock type
         * @tparam Duration Source duration type
         * @param timePoint Source time point
         * @param formatStr Format string compatible with std::format
         * @return Formatted string
         */
        template <ClockType Clock, DurationType Duration>
        [[nodiscard]] std::string formatCustom(time_point<Clock, Duration> timePoint,
            std::string_view formatStr) noexcept {
            try {
                return std::vformat(formatStr, std::make_format_args(toSecondTimePoint(timePoint)));
            }
            catch (...) {
                return "Format Error";
            }
        }

        //-----------------------------------------------------------------------------
        // Wide string formatting functions
        //-----------------------------------------------------------------------------

        /**
         * @brief Formats a time point as a wide date string (YYYY-MM-DD)
         * @tparam Clock Source clock type
         * @tparam Duration Source duration type
         * @param timePoint Source time point
         * @return Formatted wide date string
         */
        template <ClockType Clock, DurationType Duration>
        [[nodiscard]] std::wstring formatDateWide(time_point<Clock, Duration> timePoint) noexcept {
            try {
                return std::format(L"{:%Y-%m-%d}", toSecondTimePoint(timePoint));
            }
            catch (...) {
                return L"Invalid Date";
            }
        }

        /**
         * @brief Formats a time point as a wide date and time string (YYYY-MM-DD HH:MM:SS)
         * @tparam Clock Source clock type
         * @tparam Duration Source duration type
         * @param timePoint Source time point
         * @return Formatted wide date and time string
         */
        template <ClockType Clock, DurationType Duration>
        [[nodiscard]] std::wstring formatDateTimeWide(time_point<Clock, Duration> timePoint) noexcept {
            try {
                return std::format(L"{:%Y-%m-%d %H:%M:%S}", toSecondTimePoint(timePoint));
            }
            catch (...) {
                return L"Invalid DateTime";
            }
        }

        /**
         * @brief Formats a time point as a wide file-friendly string (YYYY_MM_DD__HH_MM_SS)
         * @tparam Clock Source clock type
         * @tparam Duration Source duration type
         * @param timePoint Source time point
         * @return Formatted wide string suitable for filenames
         */
        template <ClockType Clock, DurationType Duration>
        [[nodiscard]] std::wstring formatFileTimestampWide(time_point<Clock, Duration> timePoint) noexcept {
            try {
                return std::format(L"{:%Y_%m_%d__%H_%M_%S}", toSecondTimePoint(timePoint));
            }
            catch (...) {
                return L"Invalid_DateTime";
            }
        }

        //-----------------------------------------------------------------------------
        // Current time functions
        //-----------------------------------------------------------------------------

        /**
         * @brief Gets the current time as hours since epoch
         * @return Current hours since epoch
         */
        [[nodiscard]] inline int64_t getCurrentHours() noexcept {
            return time_point_cast<hours>(system_clock::now()).time_since_epoch().count();
        }

        /**
         * @brief Gets the current time as minutes since epoch
         * @return Current minutes since epoch
         */
        [[nodiscard]] inline int64_t getCurrentMinutes() noexcept {
            return time_point_cast<minutes>(system_clock::now()).time_since_epoch().count();
        }

        /**
         * @brief Gets the current time as seconds since epoch
         * @return Current seconds since epoch
         */
        [[nodiscard]] inline int64_t getCurrentSeconds() noexcept {
            return time_point_cast<seconds>(system_clock::now()).time_since_epoch().count();
        }

        /**
         * @brief Gets the current time as milliseconds since epoch
         * @return Current milliseconds since epoch
         */
        [[nodiscard]] inline int64_t getCurrentMilliseconds() noexcept {
            return time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
        }

        /**
         * @brief Gets the current time as microseconds since epoch
         * @return Current microseconds since epoch
         */
        [[nodiscard]] inline int64_t getCurrentMicroseconds() noexcept {
            return time_point_cast<microseconds>(system_clock::now()).time_since_epoch().count();
        }

        /**
         * @brief Gets the current time as nanoseconds since epoch
         * @return Current nanoseconds since epoch
         */
        [[nodiscard]] inline int64_t getCurrentNanoseconds() noexcept {
            return time_point_cast<nanoseconds>(system_clock::now()).time_since_epoch().count();
        }

        //-----------------------------------------------------------------------------
        // Time point creation from epoch functions
        //-----------------------------------------------------------------------------

        /**
         * @brief Creates a system_clock time point from hours since epoch
         * @param hoursCount Number of hours since epoch
         * @return System clock time point
         */
        [[nodiscard]] inline time_point<system_clock> createTimePointFromHours(int64_t hoursCount) noexcept {
            return time_point<system_clock>{hours{ hoursCount }};
        }

        /**
         * @brief Creates a system_clock time point from minutes since epoch
         * @param minutesCount Number of minutes since epoch
         * @return System clock time point
         */
        [[nodiscard]] inline time_point<system_clock> createTimePointFromMinutes(int64_t minutesCount) noexcept {
            return time_point<system_clock>{minutes{ minutesCount }};
        }

        /**
         * @brief Creates a system_clock time point from seconds since epoch
         * @param secondsCount Number of seconds since epoch
         * @return System clock time point
         */
        [[nodiscard]] inline time_point<system_clock> createTimePointFromSeconds(int64_t secondsCount) noexcept {
            return time_point<system_clock>{seconds{ secondsCount }};
        }

        /**
         * @brief Creates a system_clock time point from milliseconds since epoch
         * @param millisecondsCount Number of milliseconds since epoch
         * @return System clock time point
         */
        [[nodiscard]] inline time_point<system_clock> createTimePointFromMilliseconds(int64_t millisecondsCount) noexcept {
            return time_point<system_clock>{milliseconds{ millisecondsCount }};
        }

        /**
         * @brief Creates a system_clock time point from microseconds since epoch
         * @param microsecondsCount Number of microseconds since epoch
         * @return System clock time point
         */
        [[nodiscard]] inline time_point<system_clock> createTimePointFromMicroseconds(int64_t microsecondsCount) noexcept {
            return time_point<system_clock>{microseconds{ microsecondsCount }};
        }

        //-----------------------------------------------------------------------------
        // Steady clock functions
        //-----------------------------------------------------------------------------

        /**
         * @brief Gets the current steady clock time as hours count
         * @return Current steady clock hours
         */
        [[nodiscard]] inline int64_t getSteadyHours() noexcept {
            return time_point_cast<hours>(steady_clock::now()).time_since_epoch().count();
        }

        /**
         * @brief Gets the current steady clock time as minutes count
         * @return Current steady clock minutes
         */
        [[nodiscard]] inline int64_t getSteadyMinutes() noexcept {
            return time_point_cast<minutes>(steady_clock::now()).time_since_epoch().count();
        }

        /**
         * @brief Gets the current steady clock time as seconds count
         * @return Current steady clock seconds
         */
        [[nodiscard]] inline int64_t getSteadySeconds() noexcept {
            return time_point_cast<seconds>(steady_clock::now()).time_since_epoch().count();
        }

        /**
         * @brief Gets the current steady clock time as milliseconds count
         * @return Current steady clock milliseconds
         */
        [[nodiscard]] inline int64_t getSteadyMilliseconds() noexcept {
            return time_point_cast<milliseconds>(steady_clock::now()).time_since_epoch().count();
        }

        /**
         * @brief Gets the current steady clock time as microseconds count
         * @return Current steady clock microseconds
         */
        [[nodiscard]] inline int64_t getSteadyMicroseconds() noexcept {
            return time_point_cast<microseconds>(steady_clock::now()).time_since_epoch().count();
        }

        /**
         * @brief Creates a steady_clock time point from hours count
         * @param hoursCount Number of hours
         * @return Steady clock time point
         */
        [[nodiscard]] inline time_point<steady_clock> createSteadyTimePointFromHours(int64_t hoursCount) noexcept {
            return time_point<steady_clock>{hours{ hoursCount }};
        }

        /**
         * @brief Creates a steady_clock time point from minutes count
         * @param minutesCount Number of minutes
         * @return Steady clock time point
         */
        [[nodiscard]] inline time_point<steady_clock> createSteadyTimePointFromMinutes(int64_t minutesCount) noexcept {
            return time_point<steady_clock>{minutes{ minutesCount }};
        }

        /**
         * @brief Creates a steady_clock time point from seconds count
         * @param secondsCount Number of seconds
         * @return Steady clock time point
         */
        [[nodiscard]] inline time_point<steady_clock> createSteadyTimePointFromSeconds(int64_t secondsCount) noexcept {
            return time_point<steady_clock>{seconds{ secondsCount }};
        }

        /**
         * @brief Creates a steady_clock time point from milliseconds count
         * @param millisecondsCount Number of milliseconds
         * @return Steady clock time point
         */
        [[nodiscard]] inline time_point<steady_clock> createSteadyTimePointFromMilliseconds(int64_t millisecondsCount) noexcept {
            return time_point<steady_clock>{milliseconds{ millisecondsCount }};
        }

        /**
         * @brief Creates a steady_clock time point from microseconds count
         * @param microsecondsCount Number of microseconds
         * @return Steady clock time point
         */
        [[nodiscard]] inline time_point<steady_clock> createSteadyTimePointFromMicroseconds(int64_t microsecondsCount) noexcept {
            return time_point<steady_clock>{microseconds{ microsecondsCount }};
        }

        //-----------------------------------------------------------------------------
        // Time parsing functions
        //-----------------------------------------------------------------------------

        /**
         * @brief Parse a date string (YYYY-MM-DD) to a time point
         * @param dateStr Date string in YYYY-MM-DD format
         * @return Optional time point, empty if parsing fails
         */
        [[nodiscard]] inline std::optional<SecondTimePoint> parseDate(std::string_view dateStr) noexcept {
            try {
                std::tm tm = {};
                int year, month, day;

                if (sscanf(dateStr.data(), "%d-%d-%d", &year, &month, &day) == 3) {
                    tm.tm_year = year - 1900;  // Years since 1900
                    tm.tm_mon = month - 1;     // Months since January (0-11)
                    tm.tm_mday = day;          // Day of the month (1-31)
                    tm.tm_hour = 0;
                    tm.tm_min = 0;
                    tm.tm_sec = 0;

                    auto timeT = std::mktime(&tm);
                    if (timeT != -1) {
                        return SecondTimePoint{ seconds{timeT} };
                    }
                }
                return std::nullopt;
            }
            catch (...) {
                return std::nullopt;
            }
        }

        /**
         * @brief Parse a date-time string (YYYY-MM-DD HH:MM:SS) to a time point
         * @param dateTimeStr Date-time string in YYYY-MM-DD HH:MM:SS format
         * @return Optional time point, empty if parsing fails
         */
        [[nodiscard]] inline std::optional<SecondTimePoint> parseDateTime(std::string_view dateTimeStr) noexcept {
            try {
                std::tm tm = {};
                int year, month, day, hour, minute, second;

                if (sscanf(dateTimeStr.data(), "%d-%d-%d %d:%d:%d",
                    &year, &month, &day, &hour, &minute, &second) == 6) {
                    tm.tm_year = year - 1900;  // Years since 1900
                    tm.tm_mon = month - 1;     // Months since January (0-11)
                    tm.tm_mday = day;          // Day of the month (1-31)
                    tm.tm_hour = hour;         // Hours (0-23)
                    tm.tm_min = minute;        // Minutes (0-59)
                    tm.tm_sec = second;        // Seconds (0-60)

                    auto timeT = std::mktime(&tm);
                    if (timeT != -1) {
                        return SecondTimePoint{ seconds{timeT} };
                    }
                }
                return std::nullopt;
            }
            catch (...) {
                return std::nullopt;
            }
        }

        /**
         * @brief Parse an ISO 8601 string (YYYY-MM-DDTHH:MM:SS.sssZ) to a time point
         * @param isoStr ISO 8601 string
         * @return Optional time point, empty if parsing fails
         */
        [[nodiscard]] inline std::optional<MillisecondTimePoint> parseISO8601(std::string_view isoStr) noexcept {
            try {
                std::tm tm = {};
                int year, month, day, hour, minute, second, millisec;

                if (sscanf(isoStr.data(), "%d-%d-%dT%d:%d:%d.%dZ",
                    &year, &month, &day, &hour, &minute, &second, &millisec) >= 6) {
                    tm.tm_year = year - 1900;  // Years since 1900
                    tm.tm_mon = month - 1;     // Months since January (0-11)
                    tm.tm_mday = day;          // Day of the month (1-31)
                    tm.tm_hour = hour;         // Hours (0-23)
                    tm.tm_min = minute;        // Minutes (0-59)
                    tm.tm_sec = second;        // Seconds (0-60)

                    auto timeT = std::mktime(&tm);
                    if (timeT != -1) {
                        auto tp = milliseconds{ timeT };
                        // Add milliseconds if parsed
                        if (isoStr.find('.') != std::string_view::npos) {
                            tp += milliseconds{ millisec };
                        }
                        return MillisecondTimePoint{ tp };
                    }
                }
                return std::nullopt;
            }
            catch (...) {
                return std::nullopt;
            }
        }

        //-----------------------------------------------------------------------------
        // Calendar utility functions
        //-----------------------------------------------------------------------------

        /**
         * @brief Gets the day of week from a time point (0 = Sunday, 6 = Saturday)
         * @param timePoint Time point to get day of week from
         * @return Day of week (0-6)
         */
        template <ClockType Clock, DurationType Duration>
        [[nodiscard]] inline int getDayOfWeek(time_point<Clock, Duration> timePoint) noexcept {
            try {
                std::time_t tt = system_clock::to_time_t(toSystemTimePoint(timePoint));
                std::tm tm;

#ifdef _MSC_VER
                localtime_s(&tm, &tt);
#else
                tm = *std::localtime(&tt);
#endif

                return tm.tm_wday;
            }
            catch (...) {
                return -1;
            }
        }

        /**
         * @brief Gets the day of month from a time point (1-31)
         * @param timePoint Time point to get day of month from
         * @return Day of month (1-31)
         */
        template <ClockType Clock, DurationType Duration>
        [[nodiscard]] inline int getDayOfMonth(time_point<Clock, Duration> timePoint) noexcept {
            try {
                std::time_t tt = system_clock::to_time_t(toSystemTimePoint(timePoint));
                std::tm tm;

#ifdef _MSC_VER
                localtime_s(&tm, &tt);
#else
                tm = *std::localtime(&tt);
#endif

                return tm.tm_mday;
            }
            catch (...) {
                return -1;
            }
        }

        /**
         * @brief Gets the month from a time point (0-11, 0 = January)
         * @param timePoint Time point to get month from
         * @return Month (0-11)
         */
        template <ClockType Clock, DurationType Duration>
        [[nodiscard]] inline int getMonth(time_point<Clock, Duration> timePoint) noexcept {
            try {
                std::time_t tt = system_clock::to_time_t(toSystemTimePoint(timePoint));
                std::tm tm;

#ifdef _MSC_VER
                localtime_s(&tm, &tt);
#else
                tm = *std::localtime(&tt);
#endif

                return tm.tm_mon;
            }
            catch (...) {
                return -1;
            }
        }

        /**
         * @brief Gets the year from a time point
         * @param timePoint Time point to get year from
         * @return Year (e.g., 2024)
         */
        template <ClockType Clock, DurationType Duration>
        [[nodiscard]] inline int getYear(time_point<Clock, Duration> timePoint) noexcept {
            try {
                std::time_t tt = system_clock::to_time_t(toSystemTimePoint(timePoint));
                std::tm tm;

#ifdef _MSC_VER
                localtime_s(&tm, &tt);
#else
                tm = *std::localtime(&tt);
#endif

                return tm.tm_year + 1900;
            }
            catch (...) {
                return -1;
            }
        }

        //-----------------------------------------------------------------------------
        // SystemTime class
        //-----------------------------------------------------------------------------

        /**
         * @brief A wrapper class for system_clock time points with a specific duration
         * @tparam Duration Duration type to use for this time point
         */
        template <DurationType Duration>
        class SystemTime {
        public:
            /// Minimum possible time value
            static constexpr int64_t MIN_VALUE{ std::numeric_limits<int64_t>::min() };
            /// Maximum possible time value
            static constexpr int64_t MAX_VALUE{ std::numeric_limits<int64_t>::max() };

            /// Duration type for this time point
            using duration = Duration;
            /// Clock type for this time point
            using clock = std::chrono::system_clock;
            /// Time point type for this time point
            using time_point = std::chrono::time_point<clock, duration>;

            /**
             * @brief Converts a time point to an epoch count for this duration
             * @tparam Clock Source clock type
             * @tparam SourceDuration Source duration type
             * @param timePoint Source time point
             * @return Epoch count in this duration
             */
            template <ClockType Clock, DurationType SourceDuration>
            static constexpr int64_t getCount(std::chrono::time_point<Clock, SourceDuration> const& timePoint) noexcept {
                return std::chrono::time_point_cast<duration>(
                    std::chrono::clock_cast<clock, Clock, SourceDuration>(timePoint)
                ).time_since_epoch().count();
            }

            /// Default constructor
            constexpr SystemTime() noexcept = default;

            /**
             * @brief Constructs from an epoch count
             * @param epochCount Number of duration units since epoch
             */
            explicit constexpr SystemTime(int64_t epochCount) noexcept : m_epochCount{ epochCount } {}

            /**
             * @brief Assignment from an epoch count
             * @param epochCount Number of duration units since epoch
             * @return Reference to this object
             */
            constexpr SystemTime& operator=(int64_t epochCount) noexcept {
                m_epochCount = epochCount;
                return *this;
            }

            /**
             * @brief Constructs from a time point
             * @tparam Clock Source clock type
             * @tparam SourceDuration Source duration type
             * @param timePoint Source time point
             */
            template <ClockType Clock, DurationType SourceDuration>
            explicit constexpr SystemTime(std::chrono::time_point<Clock, SourceDuration> const& timePoint) noexcept
                : m_epochCount{ getCount(timePoint) } {
            }

            /**
             * @brief Constructs from another SystemTime with a different duration
             * @tparam OtherDuration Source duration type
             * @param other Source SystemTime
             */
            template <DurationType OtherDuration>
            explicit constexpr SystemTime(SystemTime<OtherDuration> const& other) noexcept
                : m_epochCount{ getCount(other.toTimePoint()) } {
            }

            /**
             * @brief Assignment from a time point
             * @tparam Clock Source clock type
             * @tparam SourceDuration Source duration type
             * @param timePoint Source time point
             * @return Reference to this object
             */
            template <ClockType Clock, DurationType SourceDuration>
            constexpr SystemTime& operator=(std::chrono::time_point<Clock, SourceDuration> const& timePoint) noexcept {
                m_epochCount = getCount(timePoint);
                return *this;
            }

            /**
             * @brief Gets the current system time
             * @return SystemTime object for current time
             */
            static constexpr SystemTime now() noexcept {
                return SystemTime{ std::chrono::time_point_cast<duration>(clock::now()) };
            }

            /**
             * @brief Gets a time point at a specific delay from now
             * @param delay Delay in current duration units
             * @return SystemTime object delayed from current time
             */
            static constexpr SystemTime fromNow(int64_t delay) noexcept {
                return SystemTime{ now().m_epochCount + delay };
            }

            /**
             * @brief Resets the time to minimum value
             */
            constexpr void clear() noexcept { m_epochCount = MIN_VALUE; }

            /**
             * @brief Checks if the time has a valid value
             * @return True if the time is valid
             */
            [[nodiscard]] constexpr bool isValid() const noexcept { return m_epochCount != MIN_VALUE; }

            /**
             * @brief Gets the time point representation
             * @return Time point object
             */
            [[nodiscard]] constexpr time_point toTimePoint() const noexcept {
                return time_point{ duration{ m_epochCount } };
            }

            /**
             * @brief Converts to a C-style time_t
             * @return time_t value
             */
            [[nodiscard]] constexpr time_t toTimeT() const noexcept {
                return std::chrono::system_clock::to_time_t(toTimePoint());
            }

            /**
             * @brief Encodes the time as a base64 string
             * @return Base64 encoded string
             */
            [[nodiscard]] std::string toBase64() const noexcept {
                return mz::Encoder64::ToString(m_epochCount);
            }

            /**
             * @brief Formats the time as a date-time string
             * @return Formatted date-time string
             */
            [[nodiscard]] std::string toString() const noexcept {
                try {
                    return std::format("{:%Y-%m-%d %H:%M:%S}", toTimePoint());
                }
                catch (...) {
                    return "Invalid DateTime";
                }
            }

            /**
             * @brief Formats the time as a wide date-time string
             * @return Formatted wide date-time string
             */
            [[nodiscard]] std::wstring toWideString() const noexcept {
                try {
                    return std::format(L"{:%Y-%m-%d %H:%M:%S}", toTimePoint());
                }
                catch (...) {
                    return L"Invalid DateTime";
                }
            }

            /**
             * @brief Formats the time as a file-friendly string
             * @return Formatted string suitable for filenames
             */
            [[nodiscard]] std::string toFileString() const noexcept {
                try {
                    return std::format("{:%Y_%m_%d__%H_%M_%S}", toTimePoint());
                }
                catch (...) {
                    return "Invalid_DateTime";
                }
            }

            /**
             * @brief Formats the time as a file-friendly wide string
             * @return Formatted wide string suitable for filenames
             */
            [[nodiscard]] std::wstring toFileWideString() const noexcept {
                try {
                    return std::format(L"{:%Y_%m_%d__%H_%M_%S}", toTimePoint());
                }
                catch (...) {
                    return L"Invalid_DateTime";
                }
            }

            /// Epoch count stored in the specified duration
            int64_t m_epochCount{ MIN_VALUE };

            /**
             * @brief Adds a duration to this time
             * @tparam OtherDuration Duration type to add
             * @param other Duration to add
             * @return Reference to this object
             */
            template <DurationType OtherDuration>
            constexpr SystemTime& operator+=(OtherDuration other) noexcept {
                auto tp = toTimePoint() + other;
                m_epochCount = getCount(tp);
                return *this;
            }

            /**
             * @brief Subtracts a duration from this time
             * @tparam OtherDuration Duration type to subtract
             * @param other Duration to subtract
             * @return Reference to this object
             */
            template <DurationType OtherDuration>
            constexpr SystemTime& operator-=(OtherDuration other) noexcept {
                auto tp = toTimePoint() - other;
                m_epochCount = getCount(tp);
                return *this;
            }

            /**
             * @brief Adds days to this time
             * @param days Number of days to add
             * @return Reference to this object
             */
            constexpr SystemTime& addDays(int days) noexcept {
                return (*this) += std::chrono::hours{ days * 24 };
            }

            /**
             * @brief Adds hours to this time
             * @param hours Number of hours to add
             * @return Reference to this object
             */
            constexpr SystemTime& addHours(int hours) noexcept {
                return (*this) += std::chrono::hours{ hours };
            }

            /**
             * @brief Adds minutes to this time
             * @param minutes Number of minutes to add
             * @return Reference to this object
             */
            constexpr SystemTime& addMinutes(int minutes) noexcept {
                return (*this) += std::chrono::minutes{ minutes };
            }

            /**
             * @brief Adds seconds to this time
             * @param seconds Number of seconds to add
             * @return Reference to this object
             */
            constexpr SystemTime& addSeconds(int seconds) noexcept {
                return (*this) += std::chrono::seconds{ seconds };
            }

            /**
             * @brief Less-than comparison
             * @param left Left operand
             * @param right Right operand
             * @return True if left is earlier than right
             */
            friend constexpr bool operator<(SystemTime left, SystemTime right) noexcept {
                return left.m_epochCount < right.m_epochCount;
            }

            /**
             * @brief Less-than-or-equal comparison
             * @param left Left operand
             * @param right Right operand
             * @return True if left is not later than right
             */
            friend constexpr bool operator<=(SystemTime left, SystemTime right) noexcept {
                return left.m_epochCount <= right.m_epochCount;
            }

            /**
             * @brief Greater-than comparison
             * @param left Left operand
             * @param right Right operand
             * @return True if left is later than right
             */
            friend constexpr bool operator>(SystemTime left, SystemTime right) noexcept {
                return left.m_epochCount > right.m_epochCount;
            }

            /**
             * @brief Greater-than-or-equal comparison
             * @param left Left operand
             * @param right Right operand
             * @return True if left is not earlier than right
             */
            friend constexpr bool operator>=(SystemTime left, SystemTime right) noexcept {
                return left.m_epochCount >= right.m_epochCount;
            }

            /**
             * @brief Equality comparison
             * @param left Left operand
             * @param right Right operand
             * @return True if times are equal
             */
            friend constexpr bool operator==(SystemTime left, SystemTime right) noexcept {
                return left.m_epochCount == right.m_epochCount;
            }

            /**
             * @brief Inequality comparison
             * @param left Left operand
             * @param right Right operand
             * @return True if times are not equal
             */
            friend constexpr bool operator!=(SystemTime left, SystemTime right) noexcept {
                return left.m_epochCount != right.m_epochCount;
            }

            /**
             * @brief Duration between two times
             * @param left Left operand
             * @param right Right operand
             * @return Duration from right to left
             */
            friend constexpr duration operator-(SystemTime left, SystemTime right) noexcept {
                return duration{ left.m_epochCount - right.m_epochCount };
            }

            /**
             * @brief Addition with a duration
             * @tparam OtherDuration Duration type to add
             * @param left Left operand (time)
             * @param right Right operand (duration)
             * @return New time after adding duration
             */
            template <DurationType OtherDuration>
            friend constexpr SystemTime operator+(SystemTime left, OtherDuration right) noexcept {
                return SystemTime{ left.toTimePoint() + right };
            }

            /**
             * @brief Subtraction of a duration
             * @tparam OtherDuration Duration type to subtract
             * @param left Left operand (time)
             * @param right Right operand (duration)
             * @return New time after subtracting duration
             */
            template <DurationType OtherDuration>
            friend constexpr SystemTime operator-(SystemTime left, OtherDuration right) noexcept {
                return SystemTime{ left.toTimePoint() - right };
            }
        };

        /// SystemTime with seconds precision
        using SecondTime = SystemTime<std::chrono::seconds>;
        /// SystemTime with milliseconds precision
        using MillisecondTime = SystemTime<std::chrono::milliseconds>;
        /// SystemTime with microseconds precision
        using MicrosecondTime = SystemTime<std::chrono::microseconds>;
        /// SystemTime with nanoseconds precision
        using NanosecondTime = SystemTime<std::chrono::nanoseconds>;

        //-----------------------------------------------------------------------------
        // SteadyTime class
        //-----------------------------------------------------------------------------

        /**
         * @brief A wrapper class for steady_clock time points with a specific duration
         * @tparam Duration Duration type to use for this time point
         *
         * SteadyTime is useful for measuring precise intervals but cannot be converted
         * to calendar time directly. Use SystemTime for calendar times.
         */
        template <DurationType Duration>
        class SteadyTime {
        public:
            /// Minimum possible time value
            static constexpr int64_t MIN_VALUE{ std::numeric_limits<int64_t>::min() };
            /// Maximum possible time value
            static constexpr int64_t MAX_VALUE{ std::numeric_limits<int64_t>::max() };

            /// Duration type for this time point
            using duration = Duration;
            /// Clock type for this time point
            using clock = std::chrono::steady_clock;
            /// Time point type for this time point
            using time_point = std::chrono::time_point<clock, duration>;
            /// System clock type for conversions
            using system_clock = std::chrono::system_clock;
            /// System time point type for conversions
            using system_time_point = std::chrono::time_point<system_clock, duration>;

            /**
             * @brief Converts a time point to an epoch count for this duration
             * @tparam Clock Source clock type
             * @tparam SourceDuration Source duration type
             * @param timePoint Source time point
             * @return Epoch count in this duration
             */
            template <ClockType Clock, DurationType SourceDuration>
            static constexpr int64_t getCount(std::chrono::time_point<Clock, SourceDuration> const& timePoint) noexcept {
                return std::chrono::time_point_cast<duration>(
                    std::chrono::clock_cast<clock, Clock, SourceDuration>(timePoint)
                ).time_since_epoch().count();
            }

            /// Default constructor
            constexpr SteadyTime() noexcept = default;

            /**
             * @brief Constructs from an epoch count
             * @param epochCount Number of duration units since epoch
             */
            explicit constexpr SteadyTime(int64_t epochCount) noexcept : m_epochCount{ epochCount } {}

            /**
             * @brief Assignment from an epoch count
             * @param epochCount Number of duration units since epoch
             * @return Reference to this object
             */
            constexpr SteadyTime& operator=(int64_t epochCount) noexcept {
                m_epochCount = epochCount;
                return *this;
            }

            /**
             * @brief Constructs from a time point
             * @tparam Clock Source clock type
             * @tparam SourceDuration Source duration type
             * @param timePoint Source time point
             */
            template <ClockType Clock, DurationType SourceDuration>
            explicit constexpr SteadyTime(std::chrono::time_point<Clock, SourceDuration> const& timePoint) noexcept
                : m_epochCount{ getCount(timePoint) } {
            }

            /**
             * @brief Assignment from a time point
             * @tparam Clock Source clock type
             * @tparam SourceDuration Source duration type
             * @param timePoint Source time point
             * @return Reference to this object
             */
            template <ClockType Clock, DurationType SourceDuration>
            constexpr SteadyTime& operator=(std::chrono::time_point<Clock, SourceDuration> const& timePoint) noexcept {
                m_epochCount = getCount(timePoint);
                return *this;
            }

            /**
             * @brief Gets the current steady time
             * @return SteadyTime object for current time
             */
            static constexpr SteadyTime now() noexcept {
                return SteadyTime{ std::chrono::time_point_cast<duration>(clock::now()) };
            }

            /**
             * @brief Resets the time to minimum value
             */
            constexpr void clear() noexcept { m_epochCount = MIN_VALUE; }

            /**
             * @brief Checks if the time has a valid value
             * @return True if the time is valid
             */
            [[nodiscard]] constexpr bool isValid() const noexcept { return m_epochCount != MIN_VALUE; }

            /**
             * @brief Gets the time point representation
             * @return Steady clock time point
             */
            [[nodiscard]] constexpr time_point toTimePoint() const noexcept {
                return time_point{ duration{ m_epochCount } };
            }

            /**
             * @brief Converts to a system clock time point
             * @return System clock time point
             * @note This approximation may not be accurate as steady_clock and
             *       system_clock have different epoch times and progress rates
             */
            [[nodiscard]] constexpr system_time_point toSystemTimePoint() const noexcept {
                return system_time_point{ duration{ m_epochCount } };
            }

            /**
             * @brief Converts to a C-style time_t
             * @return time_t value
             * @note This approximation may not be accurate
             */
            [[nodiscard]] constexpr time_t toTimeT() const noexcept {
                return std::chrono::system_clock::to_time_t(toSystemTimePoint());
            }

            /**
             * @brief Encodes the time as a base64 string
             * @return Base64 encoded string
             */
            [[nodiscard]] std::string toBase64() const noexcept {
                return mz::Encoder64::ToString(m_epochCount);
            }

            /**
             * @brief Formats the time as a date-time string
             * @return Formatted date-time string
             * @note This is an approximation as steady_clock does not represent wall clock time
             */
            [[nodiscard]] std::string toString() const noexcept {
                try {
                    return std::format("{:%Y-%m-%d %H:%M:%S}", toSystemTimePoint());
                }
                catch (...) {
                    return "Invalid DateTime";
                }
            }

            /**
             * @brief Formats the time as a wide date-time string
             * @return Formatted wide date-time string
             * @note This is an approximation as steady_clock does not represent wall clock time
             */
            [[nodiscard]] std::wstring toWideString() const noexcept {
                try {
                    return std::format(L"{:%Y-%m-%d %H:%M:%S}", toSystemTimePoint());
                }
                catch (...) {
                    return L"Invalid DateTime";
                }
            }

            /**
             * @brief Formats the time as a file-friendly string
             * @return Formatted string suitable for filenames
             * @note This is an approximation as steady_clock does not represent wall clock time
             */
            [[nodiscard]] std::string toFileString() const noexcept {
                try {
                    return std::format("{:%Y_%m_%d__%H_%M_%S}", toSystemTimePoint());
                }
                catch (...) {
                    return "Invalid_DateTime";
                }
            }

            /**
             * @brief Formats the time as a file-friendly wide string
             * @return Formatted wide string suitable for filenames
             * @note This is an approximation as steady_clock does not represent wall clock time
             */
            [[nodiscard]] std::wstring toFileWideString() const noexcept {
                try {
                    return std::format(L"{:%Y_%m_%d__%H_%M_%S}", toSystemTimePoint());
                }
                catch (...) {
                    return L"Invalid_DateTime";
                }
            }

            /// Epoch count stored in the specified duration
            int64_t m_epochCount{ 0 };

            /**
             * @brief Adds a duration to this time
             * @tparam OtherDuration Duration type to add
             * @param other Duration to add
             * @return Reference to this object
             */
            template <DurationType OtherDuration>
            constexpr SteadyTime& operator+=(OtherDuration other) noexcept {
                m_epochCount = SteadyTime{ toTimePoint() + other }.m_epochCount;
                return *this;
            }

            /**
             * @brief Subtracts a duration from this time
             * @tparam OtherDuration Duration type to subtract
             * @param other Duration to subtract
             * @return Reference to this object
             */
            template <DurationType OtherDuration>
            constexpr SteadyTime& operator-=(OtherDuration other) noexcept {
                m_epochCount = SteadyTime{ toTimePoint() - other }.m_epochCount;
                return *this;
            }

            /**
             * @brief Adds hours to this time
             * @param hours Number of hours to add
             * @return Reference to this object
             */
            constexpr SteadyTime& addHours(int hours) noexcept {
                return (*this) += std::chrono::hours{ hours };
            }

            /**
             * @brief Adds minutes to this time
             * @param minutes Number of minutes to add
             * @return Reference to this object
             */
            constexpr SteadyTime& addMinutes(int minutes) noexcept {
                return (*this) += std::chrono::minutes{ minutes };
            }

            /**
             * @brief Adds seconds to this time
             * @param seconds Number of seconds to add
             * @return Reference to this object
             */
            constexpr SteadyTime& addSeconds(int seconds) noexcept {
                return (*this) += std::chrono::seconds{ seconds };
            }

            /**
             * @brief Less-than comparison
             * @param left Left operand
             * @param right Right operand
             * @return True if left is earlier than right
             */
            friend constexpr bool operator<(SteadyTime left, SteadyTime right) noexcept {
                return left.m_epochCount < right.m_epochCount;
            }

            /**
             * @brief Less-than-or-equal comparison
             * @param left Left operand
             * @param right Right operand
             * @return True if left is not later than right
             */
            friend constexpr bool operator<=(SteadyTime left, SteadyTime right) noexcept {
                return left.m_epochCount <= right.m_epochCount;
            }

            /**
             * @brief Greater-than comparison
             * @param left Left operand
             * @param right Right operand
             * @return True if left is later than right
             */
            friend constexpr bool operator>(SteadyTime left, SteadyTime right) noexcept {
                return left.m_epochCount > right.m_epochCount;
            }

            /**
             * @brief Greater-than-or-equal comparison
             * @param left Left operand
             * @param right Right operand
             * @return True if left is not earlier than right
             */
            friend constexpr bool operator>=(SteadyTime left, SteadyTime right) noexcept {
                return left.m_epochCount >= right.m_epochCount;
            }

            /**
             * @brief Equality comparison
             * @param left Left operand
             * @param right Right operand
             * @return True if times are equal
             */
            friend constexpr bool operator==(SteadyTime left, SteadyTime right) noexcept {
                return left.m_epochCount == right.m_epochCount;
            }

            /**
             * @brief Inequality comparison
             * @param left Left operand
             * @param right Right operand
             * @return True if times are not equal
             */
            friend constexpr bool operator!=(SteadyTime left, SteadyTime right) noexcept {
                return left.m_epochCount != right.m_epochCount;
            }

            /**
             * @brief Duration between two times
             * @param left Left operand
             * @param right Right operand
             * @return Duration from right to left
             */
            friend constexpr duration operator-(SteadyTime left, SteadyTime right) noexcept {
                return duration{ left.m_epochCount - right.m_epochCount };
            }

            /**
             * @brief Addition with a duration
             * @tparam OtherDuration Duration type to add
             * @param left Left operand (time)
             * @param right Right operand (duration)
             * @return New time after adding duration
             */
            template <DurationType OtherDuration>
            friend constexpr SteadyTime operator+(SteadyTime left, OtherDuration right) noexcept {
                return SteadyTime{ left.toTimePoint() + right };
            }

            /**
             * @brief Subtraction of a duration
             * @tparam OtherDuration Duration type to subtract
             * @param left Left operand (time)
             * @param right Right operand (duration)
             * @return New time after subtracting duration
             */
            template <DurationType OtherDuration>
            friend constexpr SteadyTime operator-(SteadyTime left, OtherDuration right) noexcept {
                return SteadyTime{ left.toTimePoint() - right };
            }
        };

        /// SteadyTime with seconds precision
        using SteadySecondTime = SteadyTime<std::chrono::seconds>;
        /// SteadyTime with milliseconds precision
        using SteadyMillisecondTime = SteadyTime<std::chrono::milliseconds>;
        /// SteadyTime with microseconds precision
        using SteadyMicrosecondTime = SteadyTime<std::chrono::microseconds>;
        /// SteadyTime with nanoseconds precision
        using SteadyNanosecondTime = SteadyTime<std::chrono::nanoseconds>;

    } // namespace time
} // namespace mz

#endif // MZ_TIME_CONVERSIONS_HEADER_FILE

