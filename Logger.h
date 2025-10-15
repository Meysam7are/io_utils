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

#ifndef MZ_LOGGER_HEADER_FILE
#define MZ_LOGGER_HEADER_FILE
#pragma once

#include <string>
#include <string_view>
#include <format>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <vector>
#include <mutex>
#include <optional>

namespace mz {

    /**
     * @brief Severity levels for logging messages
     */
    enum class LogLevel {
        Debug,    ///< Detailed information, typically useful only for diagnosing problems
        Info,     ///< Informational messages that highlight progress of the application
        Warning,  ///< Potentially harmful situations that might cause problems
        Error,    ///< Error events that might still allow the application to continue
        Critical  ///< Very severe error events that will likely lead to application failure
    };

    /**
     * @brief A thread-safe, buffered logging utility with configurable options
     *
     * @tparam ThreadSafe If true, logging operations are protected by a mutex for thread safety
     *                    If false, no synchronization is used (faster but not thread-safe)
     *
     * This class provides a flexible logging system with support for:
     * - Multiple log levels
     * - Timestamped messages
     * - File rotation based on size
     * - Message buffering for performance
     * - Optional thread safety
     * - Custom headers and indentation
     * - Formatted logging with std::format support
     */
    template <bool ThreadSafe = true>
    class Logger {
    private:
        // ---- Core members ----
        std::ofstream m_fileStream{};                 ///< Output file stream
        std::string m_header{};                       ///< Custom header or indentation prefix
        std::vector<std::string> m_buffer;            ///< Message buffer to reduce I/O
        std::optional<std::filesystem::path> m_path;  ///< Current log file path
        LogLevel m_currentLevel{ LogLevel::Info };      ///< Minimum log level to output

        // ---- Configuration options ----
        size_t m_bufferSize{ 1024 };                    ///< Maximum buffer size before flush
        size_t m_maxFileSize{ 10 * 1024 * 1024 };       ///< Maximum log file size (10MB default)
        int m_maxRotationCount{ 5 };                    ///< Maximum number of rotated log files

        // ---- Error tracking ----
        enum class ErrorState {
            None,
            FileOpenError,
            WriteError,
            FormatError,
            RotationError
        };
        ErrorState m_lastError{ ErrorState::None };     ///< Last error that occurred
        std::string m_errorMessage{};                 ///< Error message for the last error

        // ---- Thread synchronization ----
        /**
         * @brief Mutex for thread-safe operations, only used when ThreadSafe is true
         */
        std::mutex m_mutex;

        /**
         * @brief Helper template for conditionally acquiring a lock based on ThreadSafe
         */
        template <typename Func>
        auto withLock(Func&& func) -> decltype(func()) {
            if constexpr (ThreadSafe) {
                std::lock_guard<std::mutex> lock(m_mutex);
                return func();
            }
            else {
                return func();
            }
        }

        /**
         * @brief Convert a LogLevel to a string representation
         * @param level The log level to convert
         * @return String representation of the log level
         */
        static constexpr std::string_view logLevelToString(LogLevel level) noexcept {
            switch (level) {
            case LogLevel::Debug:    return "DEBUG";
            case LogLevel::Info:     return "INFO";
            case LogLevel::Warning:  return "WARNING";
            case LogLevel::Error:    return "ERROR";
            case LogLevel::Critical: return "CRITICAL";
            default:                 return "UNKNOWN";
            }
        }

        /**
         * @brief Rotate log files when the current log file reaches max size
         *
         * Renames existing log files with suffixes .1, .2, etc. up to m_maxRotationCount
         */
        void rotateLogFiles() noexcept {
            if (!m_path) return;

            try {
                // Close current file
                m_fileStream.close();

                // Perform rotation
                auto base = m_path->string();

                // Remove oldest log file if it exists
                auto oldestLog = std::format("{}.{}", base, m_maxRotationCount);
                std::filesystem::remove(oldestLog);

                // Rename existing log files, moving each one up a number
                for (int i = m_maxRotationCount - 1; i > 0; --i) {
                    auto currentLog = std::format("{}.{}", base, i);
                    auto nextLog = std::format("{}.{}", base, i + 1);
                    if (std::filesystem::exists(currentLog)) {
                        std::filesystem::rename(currentLog, nextLog);
                    }
                }

                // Rename current log file to .1
                std::filesystem::rename(base, std::format("{}.1", base));

                // Open a new log file
                m_fileStream.open(base, std::ios::out | std::ios::binary);
                if (!m_fileStream.is_open()) {
                    m_lastError = ErrorState::FileOpenError;
                    m_errorMessage = "Failed to open new log file after rotation";
                }
            }
            catch (const std::exception& ex) {
                m_lastError = ErrorState::RotationError;
                m_errorMessage = std::format("Log rotation failed: {}", ex.what());

                // Attempt to reopen the original file
                if (!m_fileStream.is_open() && m_path) {
                    m_fileStream.open(m_path->string(), std::ios::out | std::ios::binary | std::ios::app);
                }
            }
        }

        /**
         * @brief Check if log rotation is needed and perform rotation if required
         */
        void checkRotation() noexcept {
            if (!m_path || !m_fileStream.is_open()) return;

            try {
                auto size = std::filesystem::file_size(*m_path);
                if (size > m_maxFileSize) {
                    rotateLogFiles();
                }
            }
            catch (...) {
                // Silently ignore file size check errors
            }
        }

        /**
         * @brief Write accumulated messages from buffer to the file
         * @return True if successful, false otherwise
         */
        bool flushBuffer() noexcept {
            if (!m_fileStream.is_open() || m_buffer.empty()) {
                return true; // Nothing to do
            }

            try {
                for (const auto& msg : m_buffer) {
                    m_fileStream << msg;
                }
                m_fileStream.flush();
                m_buffer.clear();
                return true;
            }
            catch (const std::exception& ex) {
                m_lastError = ErrorState::WriteError;
                m_errorMessage = std::format("Failed to flush buffer: {}", ex.what());
                return false;
            }
        }

    public:
        /**
         * @brief Default constructor
         */
        Logger() noexcept = default;

        /**
         * @brief Constructor with indentation
         * @param indent Number of spaces to indent log messages
         */
        explicit Logger(size_t indent) noexcept
            : m_header{ std::format("{:>{}}", "", indent) } {
            m_buffer.reserve(m_bufferSize);
        }

        /**
         * @brief Constructor with custom header
         * @param header Custom header string to prefix log messages
         */
        explicit Logger(std::string header) noexcept
            : m_header{ std::move(header) } {
            m_buffer.reserve(m_bufferSize);
        }

        /**
         * @brief Destructor - ensures all pending messages are written
         */
        ~Logger() noexcept {
            withLock([this]() {
                flushBuffer();
                if (m_fileStream.is_open()) {
                    m_fileStream.close();
                }
                });
        }

        /**
         * @brief Open or create a log file
         * @param path Path to the log file
         * @param mode File open mode (append by default)
         * @return 0 on success, 1 if already open, -1 on error
         */
        int start(const std::filesystem::path& path, int mode = std::ios::app) noexcept {
            return withLock([this, &path, mode]() {
                // Return if already open
                if (m_fileStream.is_open()) {
                    return 1;
                }

                try {
                    // Create directory if it doesn't exist
                    auto directory = path.parent_path();
                    if (!directory.empty() && !std::filesystem::exists(directory)) {
                        std::filesystem::create_directories(directory);
                    }

                    // Open the file
                    m_fileStream.open(path.string(), std::ios::out | std::ios::binary | mode);
                    if (m_fileStream.is_open()) {
                        m_path = path;
                        m_lastError = ErrorState::None;
                        m_errorMessage.clear();
                        return 0;
                    }
                    else {
                        m_lastError = ErrorState::FileOpenError;
                        m_errorMessage = "Failed to open log file";
                        return -1;
                    }
                }
                catch (const std::exception& ex) {
                    m_lastError = ErrorState::FileOpenError;
                    m_errorMessage = std::format("Exception opening log file: {}", ex.what());
                    return -1;
                }
                });
        }

        /**
         * @brief Close the log file
         */
        void close() noexcept {
            withLock([this]() {
                flushBuffer();
                if (m_fileStream.is_open()) {
                    m_fileStream.close();
                }
                });
        }

        /**
         * @brief Set the minimum log level
         * @param level The minimum level to log
         */
        void setLevel(LogLevel level) noexcept {
            withLock([this, level]() {
                m_currentLevel = level;
                });
        }

        /**
         * @brief Get the current minimum log level
         * @return Current log level
         */
        LogLevel getLevel() const noexcept {
            return withLock([this]() {
                return m_currentLevel;
                });
        }

        /**
         * @brief Check if a message with the specified level should be logged
         * @param level The level to check
         * @return True if the message should be logged
         */
        bool shouldLog(LogLevel level) const noexcept {
            return withLock([this, level]() {
                return level >= m_currentLevel;
                });
        }

        /**
         * @brief Set the buffer size for batched writes
         * @param size Maximum number of messages to buffer before flushing
         */
        void setBufferSize(size_t size) noexcept {
            withLock([this, size]() {
                m_bufferSize = size;
                m_buffer.reserve(size);
                });
        }

        /**
         * @brief Set the maximum log file size before rotation
         * @param size Maximum file size in bytes
         */
        void setMaxFileSize(size_t size) noexcept {
            withLock([this, size]() {
                m_maxFileSize = size;
                });
        }

        /**
         * @brief Set the maximum number of rotated log files to keep
         * @param count Maximum number of rotated files (excluding the active one)
         */
        void setMaxRotationCount(int count) noexcept {
            withLock([this, count]() {
                m_maxRotationCount = count > 0 ? count : 1;
                });
        }

        /**
         * @brief Check if the logger has encountered an error
         * @return True if an error has occurred
         */
        bool hasError() const noexcept {
            return withLock([this]() {
                return m_lastError != ErrorState::None;
                });
        }

        /**
         * @brief Get the error message for the last error
         * @return Error message or empty string if no error
         */
        std::string getErrorMessage() const noexcept {
            return withLock([this]() {
                return m_errorMessage;
                });
        }

        /**
         * @brief Clear any error state
         */
        void clearError() noexcept {
            withLock([this]() {
                m_lastError = ErrorState::None;
                m_errorMessage.clear();
                });
        }

        /**
         * @brief Manually flush buffered messages to the log file
         * @return True if successful
         */
        bool flush() noexcept {
            return withLock([this]() {
                return flushBuffer();
                });
        }

        /**
         * @brief Add timestamp and header to log message
         * @return Reference to this logger for chaining
         */
        Logger& timestamp() noexcept {
            return withLock([this]() -> Logger& {
                try {
                    auto now = std::chrono::system_clock::now();
                    std::string timeStr = std::format("\n{:%Y-%m-%d %H:%M:%S} ", now);

                    if (m_fileStream.is_open()) {
                        // If buffer would exceed size, flush it first
                        if (m_buffer.size() >= m_bufferSize) {
                            flushBuffer();
                        }
                        m_buffer.push_back(timeStr + m_header);
                    }
                }
                catch (const std::exception& ex) {
                    m_lastError = ErrorState::FormatError;
                    m_errorMessage = std::format("Timestamp formatting failed: {}", ex.what());
                }
                return *this;
                });
        }

        /**
         * @brief Add timestamp, header, and custom label to log message
         * @param label Custom label to append after header
         * @return Reference to this logger for chaining
         */
        Logger& timestamp(std::string_view label) noexcept {
            return withLock([this, label]() -> Logger& {
                try {
                    auto now = std::chrono::system_clock::now();
                    std::string timeStr = std::format("\n{:%Y-%m-%d %H:%M:%S} ", now);

                    if (m_fileStream.is_open()) {
                        // If buffer would exceed size, flush it first
                        if (m_buffer.size() >= m_bufferSize) {
                            flushBuffer();
                        }
                        m_buffer.push_back(timeStr + m_header + ": " + std::string(label));
                    }
                }
                catch (const std::exception& ex) {
                    m_lastError = ErrorState::FormatError;
                    m_errorMessage = std::format("Timestamp formatting failed: {}", ex.what());
                }
                return *this;
                });
        }

        /**
         * @brief Log a message with a specific log level
         * @param level Log level for this message
         * @param msg Message to log
         * @return Reference to this logger for chaining
         */
        Logger& log(LogLevel level, std::string_view msg) noexcept {
            if (!shouldLog(level)) {
                return *this;
            }

            return withLock([this, level, msg]() -> Logger& {
                try {
                    if (m_fileStream.is_open()) {
                        auto levelStr = logLevelToString(level);
                        auto now = std::chrono::system_clock::now();
                        std::string timeStr = std::format("\n{:%Y-%m-%d %H:%M:%S} [{}] ", now, levelStr);

                        // If buffer would exceed size, flush it first
                        if (m_buffer.size() >= m_bufferSize) {
                            flushBuffer();
                            checkRotation();
                        }
                        m_buffer.push_back(timeStr + m_header + ": " + std::string(msg));
                    }
                }
                catch (const std::exception& ex) {
                    m_lastError = ErrorState::FormatError;
                    m_errorMessage = std::format("Log formatting failed: {}", ex.what());
                }
                return *this;
                });
        }

        /**
         * @brief Log a message with formatting
         * @tparam Args Argument types for formatting
         * @param level Log level for this message
         * @param fmt Format string
         * @param args Arguments for format string
         * @return Reference to this logger for chaining
         */
        template<typename... Args>
        Logger& logf(LogLevel level, std::string_view fmt, Args&&... args) noexcept {
            if (!shouldLog(level)) {
                return *this;
            }

            return withLock([this, level, fmt, ... args = std::forward<Args>(args)]() -> Logger& {
                try {
                    std::string formatted = std::format(fmt, std::forward<Args>(args)...);
                    if (m_fileStream.is_open()) {
                        auto levelStr = logLevelToString(level);
                        auto now = std::chrono::system_clock::now();
                        std::string timeStr = std::format("\n{:%Y-%m-%d %H:%M:%S} [{}] ", now, levelStr);

                        // If buffer would exceed size, flush it first
                        if (m_buffer.size() >= m_bufferSize) {
                            flushBuffer();
                            checkRotation();
                        }
                        m_buffer.push_back(timeStr + m_header + ": " + formatted);
                    }
                }
                catch (const std::exception& ex) {
                    m_lastError = ErrorState::FormatError;
                    m_errorMessage = std::format("Log formatting failed: {}", ex.what());
                }
                return *this;
                });
        }

        /**
         * @brief Stream operator for appending text to the log
         * @param msg Message to append
         * @return Reference to this logger for chaining
         */
        Logger& operator<<(std::string_view msg) noexcept {
            return withLock([this, msg]() -> Logger& {
                try {
                    if (m_fileStream.is_open()) {
                        // If buffer would exceed size, flush it first
                        if (m_buffer.size() >= m_bufferSize) {
                            flushBuffer();
                            checkRotation();
                        }
                        m_buffer.push_back(std::string(msg));
                    }
                }
                catch (const std::exception& ex) {
                    m_lastError = ErrorState::WriteError;
                    m_errorMessage = std::format("Write failed: {}", ex.what());
                }
                return *this;
                });
        }

        // Convenience logging methods for different levels

        /**
         * @brief Log a debug message
         * @param msg Message to log
         * @return Reference to this logger for chaining
         */
        Logger& debug(std::string_view msg) noexcept {
            return log(LogLevel::Debug, msg);
        }

        /**
         * @brief Log an info message
         * @param msg Message to log
         * @return Reference to this logger for chaining
         */
        Logger& info(std::string_view msg) noexcept {
            return log(LogLevel::Info, msg);
        }

        /**
         * @brief Log a warning message
         * @param msg Message to log
         * @return Reference to this logger for chaining
         */
        Logger& warning(std::string_view msg) noexcept {
            return log(LogLevel::Warning, msg);
        }

        /**
         * @brief Log an error message
         * @param msg Message to log
         * @return Reference to this logger for chaining
         */
        Logger& error(std::string_view msg) noexcept {
            return log(LogLevel::Error, msg);
        }

        /**
         * @brief Log a critical message
         * @param msg Message to log
         * @return Reference to this logger for chaining
         */
        Logger& critical(std::string_view msg) noexcept {
            return log(LogLevel::Critical, msg);
        }

        // Formatted convenience methods

        /**
         * @brief Log a formatted debug message
         * @tparam Args Argument types for formatting
         * @param fmt Format string
         * @param args Arguments for format string
         * @return Reference to this logger for chaining
         */
        template<typename... Args>
        Logger& debugf(std::string_view fmt, Args&&... args) noexcept {
            return logf(LogLevel::Debug, fmt, std::forward<Args>(args)...);
        }

        /**
         * @brief Log a formatted info message
         * @tparam Args Argument types for formatting
         * @param fmt Format string
         * @param args Arguments for format string
         * @return Reference to this logger for chaining
         */
        template<typename... Args>
        Logger& infof(std::string_view fmt, Args&&... args) noexcept {
            return logf(LogLevel::Info, fmt, std::forward<Args>(args)...);
        }

        /**
         * @brief Log a formatted warning message
         * @tparam Args Argument types for formatting
         * @param fmt Format string
         * @param args Arguments for format string
         * @return Reference to this logger for chaining
         */
        template<typename... Args>
        Logger& warningf(std::string_view fmt, Args&&... args) noexcept {
            return logf(LogLevel::Warning, fmt, std::forward<Args>(args)...);
        }

        /**
         * @brief Log a formatted error message
         * @tparam Args Argument types for formatting
         * @param fmt Format string
         * @param args Arguments for format string
         * @return Reference to this logger for chaining
         */
        template<typename... Args>
        Logger& errorf(std::string_view fmt, Args&&... args) noexcept {
            return logf(LogLevel::Error, fmt, std::forward<Args>(args)...);
        }

        /**
         * @brief Log a formatted critical message
         * @tparam Args Argument types for formatting
         * @param fmt Format string
         * @param args Arguments for format string
         * @return Reference to this logger for chaining
         */
        template<typename... Args>
        Logger& criticalf(std::string_view fmt, Args&&... args) noexcept {
            return logf(LogLevel::Critical, fmt, std::forward<Args>(args)...);
        }

        // For backward compatibility with the original API

        /**
         * @brief Add timestamp (backward compatibility method)
         * @return Reference to this logger for chaining
         */
        Logger& ts() noexcept {
            return timestamp();
        }

        /**
         * @brief Add timestamp with label (backward compatibility method)
         * @param header Label to append
         * @return Reference to this logger for chaining
         */
        Logger& ts(const std::string& header) noexcept {
            return timestamp(header);
        }

        /**
         * @brief Start the logger (backward compatibility method)
         * @param path Log file path
         * @param mode File open mode
         * @return Status code (0=success, 1=already open, -1=error)
         */
        int Start(const std::filesystem::path& path, int mode = std::ios::app) noexcept {
            return start(path, mode);
        }
    };

    // Pre-defined loggers for common use cases
    inline static Logger<true> ErrLog{}; ///< Thread-safe error logger
    inline static Logger<true> WarLog{}; ///< Thread-safe warning logger
    inline static Logger<true> EvtLog{}; ///< Thread-safe event logger

    // Non-thread-safe versions for high-performance single-threaded scenarios
    inline static Logger<false> FastErrLog{}; ///< Non-thread-safe error logger
    inline static Logger<false> FastWarLog{}; ///< Non-thread-safe warning logger
    inline static Logger<false> FastEvtLog{}; ///< Non-thread-safe event logger

} // namespace mz

#endif // MZ_LOGGER_HEADER_FILE
