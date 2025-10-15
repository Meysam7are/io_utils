/*
* MIT License
*
* Copyright (c) 2025 Meysam Zare
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#ifndef IO_FILE_HEADER_FILE
#define IO_FILE_HEADER_FILE
#pragma once

/**
 * @file FileIO.h
 * @brief Cross-platform file I/O operations with endian-awareness
 *
 * This file provides a comprehensive set of classes for file operations
 * that work consistently across different platforms. It includes robust
 * error handling, endian-aware I/O operations, and specialized file access
 * modes (read-only, write-only, and read-write).
 *
 * The implementation abstracts platform-specific details to provide a
 * uniform interface for Windows and POSIX-compliant systems.
 *
 * @author Meysam Zare
 * @date October 14, 2025
 */

 // Platform-specific includes
#ifdef _MSC_VER
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

#include <span>
#include <filesystem>

#include "EndianConcepts.h"
#include "coord.h"

namespace mz {
    namespace io {

        /**
         * @struct error_flags
         * @brief Bit flags for tracking various file operation errors
         *
         * This structure uses bit fields to efficiently store error conditions
         * related to file operations. It separates critical errors (bad bits)
         * from operational errors (fail bits).
         *
         * Bad bits (lower byte) indicate critical errors that prevent further operations,
         * while fail bits (upper byte) indicate recoverable operation-specific failures.
         */
        struct error_flags {
            union {
                struct {
                    // Bad bits (critical errors that prevent operation)
                    uint16_t open : 1;      ///< Error occurred during file opening
                    uint16_t reopen : 1;    ///< Attempt to reopen an already open file
                    uint16_t eacces : 1;    ///< Permission denied or read-only violation
                    uint16_t eexists : 1;   ///< File exists (when O_EXCL is specified)
                    uint16_t einval : 1;    ///< Invalid parameter or argument
                    uint16_t emfile : 1;    ///< Too many open files
                    uint16_t enoent : 1;    ///< File or path not found
                    uint16_t invalid : 1;   ///< General invalid operation

                    // Fail bits (operational failures)
                    uint16_t commit : 1;    ///< Error committing changes to disk
                    uint16_t seek : 1;      ///< Error seeking to a position
                    uint16_t tell : 1;      ///< Error getting current position
                    uint16_t read : 1;      ///< Error reading from file
                    uint16_t write : 1;     ///< Error writing to file
                    uint16_t corrupt : 1;   ///< Data corruption detected
                };
                uint16_t value{ 0 };        ///< Combined value of all error flags
            };

            /**
             * @brief Checks if the file handle is in a good state (no errors)
             * @return true if no error flags are set, false otherwise
             */
            constexpr bool good() const noexcept { return !value; }

            /**
             * @brief Checks if any error flag is set
             * @return true if any error flag is set, false otherwise
             */
            constexpr bool fail() const noexcept { return value; }

            /**
             * @brief Checks if any critical (bad) error flag is set
             * @return true if any bad bit is set, false otherwise
             */
            constexpr bool bad() const noexcept { return value & 0xff; }

            /**
             * @brief Sets the open error flag if file handle is invalid
             * @param fh File handle to check
             * @return true if the file handle is invalid or other bad bits are set
             */
            constexpr bool bad(int fh) noexcept { open = (fh == -1); return bad(); }

            /**
             * @brief Resets fail bits while preserving bad bits
             *
             * This allows recovery from operational errors while maintaining
             * awareness of critical errors.
             */
            constexpr void reset() noexcept { value &= 0xff; /* clears fail bits. not bad bits. */ }

            /**
             * @brief Default constructor
             */
            constexpr error_flags() noexcept : value{ 0 } {}

            /**
             * @brief Constructor with initial value
             * @param Value Initial error flags value
             */
            constexpr error_flags(auto Value) noexcept : value{ static_cast<uint16_t>(Value) } {}

            /**
             * @brief Sets appropriate error flags based on system errno
             * @param err System error code
             *
             * Maps standard system error codes to the appropriate error flags,
             * facilitating consistent error handling across platforms.
             */
            constexpr void fill_open_errors(errno_t err) noexcept {
                value = 0;
                eacces = err & EACCES;      // path is a directory, or open-for-writing a read-only file
                eexists = err & EEXIST;     // _O_CREAT and _O_EXCL, but filename already exists.
                einval = err & EINVAL;      // Invalid oflag, shflag, pmode, or pfh or filename was a null pointer. in chage size if size < 0
                emfile = err & EMFILE;      // No more file descriptors available.
                enoent = err & ENOENT;      // File or path not found
                //eacces = err & EBADF;     // in change size when the file is read only
                //eacces = err & ENOSPC;    // in change size when no space left on disk
            }
        };

        // Forward declarations
        class mfile;
        class MultiFile;

        /**
         * @class BasicFile
         * @brief Base class providing core file operations
         *
         * This templated class implements core file operations with compile-time
         * specification of read/write capabilities. The template parameter N
         * determines if the file supports reading (N & 1), writing (N & 2), or both.
         *
         * @tparam N Access mode flags: 1=read, 2=write, 3=read+write
         */
        template <size_t N>
        class BasicFile {
            // Friend declarations for classes that need access to protected/private members
            friend class MultiFile;
            friend class mfile;

            // Compile-time constants determining file access capabilities
            static constexpr bool READER{ bool(N & 1) }; ///< File supports read operations
            static constexpr bool WRITER{ bool(N & 2) }; ///< File supports write operations

        public:
            /**
             * @brief Gets the current error flags
             * @return Structure containing all error flags
             */
            constexpr error_flags eflags() const noexcept { return e_flags; }

            /**
             * @brief Checks if any critical error has occurred
             * @return true if any bad bit is set or file handle is invalid
             */
            constexpr bool bad() const noexcept { return e_flags.bad(f_handle); }

            /**
             * @brief Checks if any error has occurred
             * @return true if any error flag is set, false otherwise
             */
            constexpr bool fail() const noexcept { return e_flags.fail(); }

            /**
             * @brief Checks if the file handle is in a good state
             * @return true if no error flags are set, false otherwise
             */
            constexpr bool good() const noexcept { return e_flags.good(); }

            /**
             * @brief Gets the native file descriptor
             * @return Platform-specific file handle
             */
            constexpr int fd() const noexcept { return f_handle; }

            /**
             * @brief Checks if the file is open
             * @return true if the file handle is valid, false otherwise
             */
            constexpr bool is_open() const noexcept { return f_handle != -1; }

            //
            // BASIC FILE OPERATIONS
            //

            /**
             * @brief Gets the length of the file
             * @return File length in bytes, or -1 if an error occurred
             */
            int64_t length() const noexcept {
                int64_t res = os_size();
                if (res == -1) {
                    e_flags.invalid = 1;
                }
                return res;
            }

            /**
             * @brief Gets the size of the file (alias for length)
             * @return File size in bytes, or -1 if an error occurred
             */
            int64_t size() const noexcept { return length(); }

            /**
             * @brief Checks if the file position is at end-of-file
             * @return 1 if at EOF, 0 if not at EOF, -1 if an error occurred
             */
            int eof() const noexcept {
                int res{ os_eof() };
                if (res == -1) {
                    e_flags.invalid = 1;
                }
                return res;
            }

            /**
             * @brief Commits changes to disk
             *
             * Flushes buffers and ensures data is written to disk.
             * Available only for files opened with write access.
             *
             * @return true if an error occurred, false on success
             */
            bool commit() noexcept requires (WRITER)
            {
                if (is_open() && good()) {
                    int res = os_commit();
                    if (res) {
                        e_flags.invalid = 1;
                        return true;
                    }
                    return false;
                }
                return true;
            }

            /**
             * @brief Changes the size of the file
             *
             * Extends or truncates the file to the specified size.
             * Available only for files opened with write access.
             *
             * @param NewSize New file size in bytes
             * @return true if an error occurred, false on success
             */
            bool chsize(int64_t NewSize) noexcept requires (WRITER)
            {
                auto err = os_chsize(NewSize);
                if (err) {
                    e_flags.invalid = 1;
                    e_flags.eacces |= err & (EBADF | ENOSPC);
                    e_flags.einval |= err & EINVAL;
                    return true;
                }
                return false;
            }

            /**
             * @brief Repositions the file position pointer
             *
             * @param Pos Position relative to the Dir parameter
             * @param Dir Direction (SEEK_SET, SEEK_CUR, SEEK_END)
             * @return New file position, or -1 if an error occurred
             */
            int64_t seek(int64_t Pos, int Dir) const noexcept
            {
                int64_t res = os_seek(Pos, Dir);
                if (res == -1) {
                    e_flags.seek = 1;
                    e_flags.invalid = 1;
                }
                return res;
            }

            /**
             * @brief Gets the current file position
             * @return Current position in bytes from start, or -1 if an error occurred
             */
            int64_t tell() const noexcept
            {
                int64_t res = os_tell();
                if (res == -1) {
                    e_flags.invalid = 1;
                    e_flags.tell = 1;
                }
                return res;
            }

            /**
             * @brief Convenience methods for seeking to specific positions
             *
             * These methods provide easier access to the seek operation with
             * predefined direction parameters.
             */
            int64_t seek_set(int64_t Pos) const noexcept { return seek(Pos, SEEK_SET); }
            int64_t seek_cur(int64_t Pos) const noexcept { return seek(Pos, SEEK_CUR); }
            int64_t seek_end(int64_t Pos) const noexcept { return seek(Pos, SEEK_END); }

            /**
             * @brief Boolean variants of seek operations
             *
             * These methods return true if the seek operation failed, making
             * them suitable for conditional statements.
             */
            int64_t bseek_set(int64_t Pos) const noexcept { return seek(Pos, SEEK_SET) == -1; }
            int64_t bseek_cur(int64_t Pos) const noexcept { return seek(Pos, SEEK_CUR) == -1; }
            int64_t bseek_end(int64_t Pos) const noexcept { return seek(Pos, SEEK_END) == -1; }

            /**
             * @brief Reads data from the file
             *
             * Available only for files opened with read access.
             *
             * @param _Buf Buffer to store the read data
             * @param Size Number of bytes to read
             * @return true if an error occurred or less than Size bytes were read, false on success
             */
            bool read(void* _Buf, size_t Size) const noexcept requires (READER)
            {
                auto res = os_read(_Buf, unsigned(Size));
                if (res == Size) {
                    return false;
                }
                else {
                    e_flags.read = 1;
                    if (res == -1) {
                        e_flags.invalid = 1;
                    }
                    return true;
                }
            }

            /**
             * @brief Writes data to the file
             *
             * Available only for files opened with write access.
             *
             * @param _Buf Data to write
             * @param Size Number of bytes to write
             * @return true if an error occurred or less than Size bytes were written, false on success
             */
            bool write(void const* _Buf, size_t Size) noexcept requires (WRITER) {
                auto res = os_write(_Buf, unsigned(Size));
                if (res == Size) {
                    return false;
                }
                else {
                    e_flags.write = 1;
                    if (res == -1) {
                        e_flags.invalid = 1;
                    }
                    return true;
                }
            }

            //
            // READ OPERATIONS
            //

            /**
             * @brief Reads a single value from the file
             *
             * @tparam T Type of the value to read (must be trivially copyable and non-const)
             * @param t Reference where the value will be stored
             * @return true if an error occurred, false on success
             */
            template <mz::endian::TrivialTypeNonConst T>
            bool read(T& t) const noexcept requires (READER)
            {
                return read((void*)&t, sizeof(T));
            }

            /**
             * @brief Reads a single value with endian conversion
             *
             * Reads a value and converts it from file endianness to native endianness.
             *
             * @tparam T Type of the value (must support endian swapping and be non-const)
             * @param t Reference where the value will be stored
             * @return true if an error occurred, false on success
             */
            template <mz::endian::SwapTypeNonConst T>
            bool readEndian(T& t) const noexcept requires (READER)
            {
                bool Err{ read(t) };
                t = mz::endian::as_endian(t);
                return Err;
            }

            /**
             * @brief Reads an array of values from the file
             *
             * @tparam T Type of the values (must be trivially copyable and non-const)
             * @param P Pointer to the array where values will be stored
             * @param N Number of values to read
             * @return true if an error occurred, false on success
             */
            template <mz::endian::TrivialTypeNonConst T>
            bool read(T* P, size_t N) const noexcept requires (READER)
            {
                return read((void*)P, sizeof(T) * N);
            }

            /**
             * @brief Reads an array of values with endian conversion
             *
             * @tparam T Type of the values (must support endian swapping and be non-const)
             * @param P Pointer to the array where values will be stored
             * @param N Number of values to read
             * @return true if an error occurred, false on success
             */
            template <mz::endian::SwapTypeNonConst T>
            bool readEndian(T* P, size_t N) const noexcept requires (READER)
            {
                if constexpr (mz::endian::endian_mismatch)
                {
                    // Read and convert each value individually when endianness differs
                    for (size_t i = 0; i < N && good(); i++) { readEndian(P[i]); }
                    return fail();
                }
                else {
                    // No conversion needed if endianness matches
                    return read(P, N);
                }
            }

            /**
             * @brief Reads values into a span
             *
             * @tparam T Type of the values (must be trivially copyable and non-const)
             * @tparam N Size of the span (deduced from the parameter)
             * @param SP Span where values will be stored
             * @return true if an error occurred, false on success
             */
            template <mz::endian::TrivialTypeNonConst T, size_t N>
            bool read(std::span<T, N> SP) const noexcept requires (READER)
            {
                return read(SP.data(), SP.size());
            }

            /**
             * @brief Reads values into a span with endian conversion
             *
             * @tparam T Type of the values (must support endian swapping and be non-const)
             * @tparam N Size of the span (deduced from the parameter)
             * @param SP Span where values will be stored
             * @return true if an error occurred, false on success
             */
            template <mz::endian::SwapTypeNonConst T, size_t N>
            bool readEndian(std::span<T, N> SP) const noexcept requires (READER)
            {
                return readEndian(SP.data(), SP.size());
            }

            /**
             * @brief Reads a string from the file
             *
             * Reads a length-prefixed string. The format is:
             * - 32-bit length prefix
             * - String data
             * - 32-bit length suffix (for validation)
             *
             * @param S String where the data will be stored
             * @return true if an error occurred or validation failed, false on success
             */
            bool read(std::string& S) const noexcept requires (READER)
            {
                uint32_t Len{ 0 };
                if (read(Len)) return true;
                S.resize(Len);
                if (read(S.data(), Len) || read(Len)) { return true; }
                if (Len != S.size()) { e_flags.corrupt = 1; return true; }
                return false;
            }

            /**
             * @brief Reads a string from the file with endian conversion
             *
             * Similar to read(std::string&) but performs endian conversion
             * on the length prefix and suffix.
             *
             * @param S String where the data will be stored
             * @return true if an error occurred or validation failed, false on success
             */
            bool readEndian(std::string& S) const noexcept requires (READER)
            {
                uint32_t Len{ 0 };
                if (readEndian(Len)) return true;
                S.resize(Len);
                if (readEndian(S.data(), Len) || readEndian(Len)) { return true; }
                if (Len != S.size()) { e_flags.corrupt = 1; return true; }
                return false;
            }

            /**
             * @brief Reads a wide string from the file
             *
             * Reads a length-prefixed wide string. The format is:
             * - 32-bit length prefix
             * - Wide string data
             * - 32-bit length suffix (for validation)
             *
             * @param W Wide string where the data will be stored
             * @return true if an error occurred or validation failed, false on success
             */
            bool read(std::wstring& W) const noexcept requires (READER)
            {
                uint32_t Len{ 0 };
                if (read(Len)) return true;
                W.resize(Len);
                if (read(W.data(), Len) || read(Len)) { return true; }
                if (Len != W.size()) { e_flags.corrupt = 1; return true; }
                return false;
            }

            /**
             * @brief Reads a wide string from the file with endian conversion
             *
             * Similar to read(std::wstring&) but performs endian conversion
             * on the length prefix, wide characters, and length suffix.
             *
             * @param W Wide string where the data will be stored
             * @return true if an error occurred or validation failed, false on success
             */
            bool readEndian(std::wstring& W) const noexcept requires (READER)
            {
                uint32_t Len{ 0 };
                if (readEndian(Len)) return true;
                W.resize(Len);
                if (readEndian(W.data(), Len) || readEndian(Len)) { return true; }
                if (Len != W.size()) { e_flags.corrupt = 1; return true; }
                return false;
            }

            //
            // WRITE OPERATIONS
            //

            /**
             * @brief Writes a single value to the file
             *
             * @tparam T Type of the value (must be trivially copyable)
             * @param t Value to write
             * @return true if an error occurred, false on success
             */
            template <mz::endian::TrivialType T>
            bool write(T t) noexcept requires (WRITER)
            {
                return write((void const*)&t, sizeof(t));
            }

            /**
             * @brief Writes a single value with endian conversion
             *
             * Converts the value from native endianness to file endianness before writing.
             *
             * @tparam T Type of the value (must support endian swapping)
             * @param t Value to write
             * @return true if an error occurred, false on success
             */
            template <mz::endian::SwapType T>
            bool writeEndian(T t) noexcept requires (WRITER)
            {
                return write(mz::endian::as_endian(t));
            }

            /**
             * @brief Writes an array of values to the file
             *
             * @tparam T Type of the values (must be trivially copyable)
             * @param P Pointer to the array of values
             * @param N Number of values to write
             * @return true if an error occurred, false on success
             */
            template <mz::endian::TrivialType T>
            bool write(T const* P, size_t N) noexcept requires (WRITER)
            {
                return write((void const*)P, sizeof(T) * N);
            }

            /**
             * @brief Writes an array of values with endian conversion
             *
             * @tparam T Type of the values (must support endian swapping)
             * @param P Pointer to the array of values
             * @param N Number of values to write
             * @return true if an error occurred, false on success
             */
            template <mz::endian::SwapType T>
            bool writeEndian(T const* P, size_t N) noexcept requires (WRITER)
            {
                if constexpr (mz::endian::endian_mismatch)
                {
                    // Write and convert each value individually when endianness differs
                    for (size_t i = 0; i < N && good(); i++) { writeEndian(P[i]); }
                    return fail();
                }
                else {
                    // No conversion needed if endianness matches
                    return write(P, N);
                }
            }

            /**
             * @brief Writes a span of values to the file
             *
             * @tparam T Type of the values (must be trivially copyable)
             * @tparam N Size of the span (deduced from the parameter)
             * @param SP Span of values to write
             * @return true if an error occurred, false on success
             */
            template <mz::endian::TrivialType T, size_t N>
            bool write(std::span<T, N> SP) noexcept requires (WRITER)
            {
                return write(SP.data(), SP.size());
            }

            /**
             * @brief Writes a span of values with endian conversion
             *
             * @tparam T Type of the values (must support endian swapping)
             * @tparam N Size of the span (deduced from the parameter)
             * @param SP Span of values to write
             * @return true if an error occurred, false on success
             */
            template <mz::endian::SwapType T, size_t N>
            bool writeEndian(std::span<T, N> SP) noexcept requires (WRITER)
            {
                return writeEndian(SP.data(), SP.size());
            }

            /**
             * @brief Writes a string to the file
             *
             * Writes a length-prefixed string. The format is:
             * - 32-bit length prefix
             * - String data
             * - 32-bit length suffix (for validation)
             *
             * @param S String to write
             * @return true if an error occurred, false on success
             */
            bool write(std::string const& S) noexcept requires (WRITER)
            {
                uint32_t Len = uint32_t(S.size());
                return write(Len) || write(S.data(), S.size()) || write(Len);
            }

            /**
             * @brief Writes a string with endian conversion
             *
             * Similar to write(std::string const&) but performs endian conversion
             * on the length prefix and suffix.
             *
             * @param S String to write
             * @return true if an error occurred, false on success
             */
            bool writeEndian(std::string const& S) noexcept requires (WRITER)
            {
                uint32_t Len = uint32_t(S.size());
                return writeEndian(Len) || writeEndian(S.data(), S.size()) || writeEndian(Len);
            }

            /**
             * @brief Writes a wide string to the file
             *
             * Writes a length-prefixed wide string. The format is:
             * - 32-bit length prefix
             * - Wide string data
             * - 32-bit length suffix (for validation)
             *
             * @param W Wide string to write
             * @return true if an error occurred, false on success
             */
            bool write(std::wstring const& W) noexcept requires (WRITER)
            {
                uint32_t Len = uint32_t(W.size());
                return write(Len) || write(W.data(), W.size()) || write(Len);
            }

            /**
             * @brief Writes a wide string with endian conversion
             *
             * Similar to write(std::wstring const&) but performs endian conversion
             * on the length prefix, wide characters, and length suffix.
             *
             * @param W Wide string to write
             * @return true if an error occurred, false on success
             */
            bool writeEndian(std::wstring const& W) noexcept requires (WRITER)
            {
                uint32_t Len = uint32_t(W.size());
                return writeEndian(Len) || writeEndian(W.data(), W.size()) || writeEndian(Len);
            }

            /**
             * @brief Closes the file handle
             *
             * For files opened with write access, attempts to commit changes
             * before closing.
             */
            void close() noexcept requires (WRITER) {
                if (is_open()) {
                    if (good()) {
                        os_commit();
                    }
                    os_close();
                    f_handle = -1;
                }
            }

            /**
             * @brief Closes the file handle (read-only version)
             */
            void close() noexcept requires (!WRITER) {
                if (is_open()) {
                    os_close();
                    f_handle = -1;
                }
            }

        protected:
            /**
             * @brief Default constructor
             *
             * Initializes the file handle to an invalid state and sets the
             * open error flag.
             */
            BasicFile() noexcept : f_handle{ -1 }, e_flags{ 1 } {}

            /**
             * @brief Destructor
             *
             * Ensures the file handle is properly closed.
             */
            ~BasicFile() noexcept { close(); }

            /**
             * @brief Opens a file with the specified path and flags
             *
             * @param Path Path to the file
             * @param Flags File access mode flags
             * @return 0 on success, non-zero on error
             */
            int open(std::filesystem::path const& Path, int Flags) noexcept
            {
                e_flags.value = 0;
                if (f_handle != -1) {
                    e_flags.reopen = 1;
                    close();
                }
                else {
                    auto err = os_open(Path.string().c_str(), Flags);

                    e_flags.fill_open_errors(err);
                    if (f_handle == -1) {
                        e_flags.open = 1;
                    }
                    else if (err) {
                        e_flags.open = 1;
                        _close(f_handle);
                        f_handle = -1;
                    }
                }
                return !good();
            }

        private:
            int f_handle{ -1 };         ///< Platform-specific file handle
            mutable error_flags e_flags; ///< Error status flags

            //
            // PLATFORM-SPECIFIC IMPLEMENTATIONS
            //

#ifdef _MSC_VER
    // Windows implementations
            int os_eof() const noexcept { return _eof(f_handle); }
            long long os_size() const noexcept { return _filelengthi64(f_handle); }
            long long os_tell() const noexcept { return _telli64(f_handle); }
            long long os_seek(long long Offset, int Origin) const noexcept { return _lseeki64(f_handle, Offset, Origin); }

            int os_chsize(long long Size) noexcept { return _chsize_s(f_handle, Size); }
            int os_commit() noexcept { return _commit(f_handle); }
            int os_close() noexcept { return _close(f_handle); }
            int os_open(std::filesystem::path const& Path, int OperationFlags) noexcept {
                return _sopen_s(&f_handle, Path.string().c_str(), OperationFlags, _SH_DENYNO, _S_IREAD | _S_IWRITE);
            }

            int os_read(void* _Buf, unsigned Size) const noexcept { return _read(f_handle, _Buf, Size); }
            int os_write(void const* _Buf, unsigned Size) const noexcept { return _write(f_handle, _Buf, Size); }

#else
    // POSIX implementations
            long long os_tell() const noexcept { return lseek64(f_handle, 0, SEEK_CUR); }
            long long os_seek(long long Offset, int Origin) const noexcept { return lseek64(f_handle, Offset, Origin); }

            int os_chsize(long long Size) noexcept { return ftruncate(f_handle, Size); }
            int os_commit() noexcept { return fsync(f_handle); }
            int os_close() noexcept { return close(f_handle); }
            int os_open(std::filesystem::path const& Path, int OperationFlags) noexcept {
                return open(&f_handle, Path.string().c_str(), OperationFlags);
            }

            int os_read(void* _Buf, unsigned Size) const noexcept { return read(f_handle, _Buf, Size); }
            int os_write(void const* _Buf, unsigned Size) const noexcept { return write(f_handle, _Buf, Size); }

            long long os_size() const noexcept {
                struct stat buf;
                if (fstat(fd, &buf)) return -1;
                return buf.st_size;
            }

            int os_eof() const noexcept {
                uint8_t dummy;
                ssize_t bytes_read = read(fd, &dummy, 1);
                if (bytes_read == 0) { return 1; }
                if (bytes_read == -1) { return -1; }
                if (lseek64(fd, -1, SEEK_CUR) == -1) { return -1; }
                return 0;
            }
#endif
        };

        /**
         * @class FileRO
         * @brief Read-only file handle
         *
         * Specialization of BasicFile that only allows read operations.
         */
        class FileRO : public BasicFile<1> {
        public:
            /**
             * @brief Default constructor
             */
            FileRO() noexcept = default;

            /**
             * @brief Constructor that opens a file
             *
             * @param Path Path to the file to open
             * @param flags Access mode flags
             */
            FileRO(std::filesystem::path const& Path, int flags) { BasicFile::open(Path, filter_open(flags)); }

            /**
             * @brief Opens a file for reading
             *
             * @param Path Path to the file to open
             * @param flags Access mode flags
             * @return true if successful, false if an error occurred
             */
            bool open(std::filesystem::path const& Path, int flags) noexcept {
                return BasicFile::open(Path, filter_open(flags)) == 0;
            }

        private:
            /**
             * @brief Filters flags to ensure read-only access
             *
             * Adds O_RDONLY and removes any conflicting flags.
             *
             * @param flags Original access mode flags
             * @return Modified flags that ensure read-only access
             */
            static constexpr int filter_open(int flags) noexcept {
                return (flags | O_RDONLY) & ~(O_RDWR | _O_TRUNC | O_WRONLY | _O_APPEND | O_CREAT | O_EXCL);
            }
        };

        /**
         * @class FileWO
         * @brief Write-only file handle
         *
         * Specialization of BasicFile that only allows write operations.
         */
        class FileWO : public BasicFile<2> {
        public:
            /**
             * @brief Default constructor
             */
            FileWO() noexcept = default;

            /**
             * @brief Opens an existing file for writing
             *
             * @param Path Path to the file to open
             * @param flags Access mode flags
             * @return true if successful, false if an error occurred
             */
            bool open(std::filesystem::path const& Path, int flags) noexcept {
                return BasicFile::open(Path, filter_open(flags)) == 0;
            }

            /**
             * @brief Opens a new file with exclusive creation
             *
             * Fails if the file already exists.
             *
             * @param Path Path to the file to create
             * @param flags Access mode flags
             * @return true if successful, false if an error occurred
             */
            bool excl(std::filesystem::path const& Path, int flags) noexcept {
                return BasicFile::open(Path, filter_excl(flags)) == 0;
            }

            /**
             * @brief Creates a new file or opens an existing one
             *
             * @param Path Path to the file to create/open
             * @param flags Access mode flags
             * @return true if successful, false if an error occurred
             */
            bool create(std::filesystem::path const& Path, int flags) noexcept {
                return BasicFile::open(Path, filter_create(flags)) == 0;
            }

        private:
            /**
             * @brief Filters flags to ensure write-only access
             *
             * @param flags Original access mode flags
             * @return Modified flags that ensure write-only access
             */
            static constexpr int filter_open(int flags) noexcept {
                return (flags | O_WRONLY) & ~(O_RDWR | O_CREAT | O_EXCL);
            }

            /**
             * @brief Filters flags for exclusive creation with write-only access
             *
             * @param flags Original access mode flags
             * @return Modified flags for exclusive creation with write-only access
             */
            static constexpr int filter_excl(int flags) noexcept {
                return (flags | O_WRONLY | O_CREAT | O_EXCL) & ~O_RDWR;
            }

            /**
             * @brief Filters flags for creation or opening with write-only access
             *
             * @param flags Original access mode flags
             * @return Modified flags for creation with write-only access
             */
            static constexpr int filter_create(int flags) noexcept {
                return (flags | O_WRONLY | O_CREAT) & ~(O_RDWR | O_EXCL);
            }
        };

        /**
         * @class FileRW
         * @brief Read-write file handle
         *
         * Specialization of BasicFile that allows both read and write operations.
         */
        class FileRW : public BasicFile<3> {
            friend class mfile;
            friend class MultiFile;

            /**
             * @brief Internal method to open a file with specified flags
             *
             * Used by friend classes to directly open files with specific flags.
             *
             * @param Path Path to the file to open
             * @param flags Access mode flags
             * @return true if successful, false if an error occurred
             */
            bool open_(std::filesystem::path const& Path, int flags) noexcept {
                return BasicFile::open(Path, flags) == 0;
            }

        public:
            /**
             * @brief Opens an existing file for reading and writing
             *
             * @param Path Path to the file to open
             * @param flags Access mode flags
             * @return true if successful, false if an error occurred
             */
            bool open(std::filesystem::path const& Path, int flags) noexcept {
                return BasicFile::open(Path, filter_open(flags)) == 0;
            }

            /**
             * @brief Opens a new file with exclusive creation
             *
             * Fails if the file already exists.
             *
             * @param Path Path to the file to create
             * @param flags Access mode flags
             * @return true if successful, false if an error occurred
             */
            bool excl(std::filesystem::path const& Path, int flags) noexcept {
                return BasicFile::open(Path, filter_excl(flags)) == 0;
            }

            /**
             * @brief Creates a new file or opens an existing one
             *
             * @param Path Path to the file to create/open
             * @param flags Access mode flags
             * @return true if successful, false if an error occurred
             */
            bool create(std::filesystem::path const& Path, int flags) noexcept {
                return BasicFile::open(Path, filter_create(flags)) == 0;
            }

            /**
             * @brief Default constructor
             */
            FileRW() noexcept = default;

        private:
            /**
             * @brief Filters flags to ensure read-write access
             *
             * @param flags Original access mode flags
             * @return Modified flags that ensure read-write access
             */
            static constexpr int filter_open(int flags) noexcept {
                return (flags | O_RDWR) & ~(_O_RDONLY | O_WRONLY & O_CREAT & O_EXCL);
            }

            /**
             * @brief Filters flags for exclusive creation with read-write access
             *
             * @param flags Original access mode flags
             * @return Modified flags for exclusive creation with read-write access
             */
            static constexpr int filter_excl(int flags) noexcept {
                return (flags | O_RDWR | O_CREAT | O_EXCL) & ~(_O_RDONLY | O_WRONLY);
            }

            /**
             * @brief Filters flags for creation or opening with read-write access
             *
             * @param flags Original access mode flags
             * @return Modified flags for creation with read-write access
             */
            static constexpr int filter_create(int flags) noexcept {
                return (flags | O_RDWR | O_CREAT) & ~(_O_RDONLY | O_WRONLY & O_EXCL);
            }
        };
    }
}

#endif // IO_FILE_HEADER_FILE
