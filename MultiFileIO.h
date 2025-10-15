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

#ifndef IO_MULTIFILE_HEADER_FILE
#define IO_MULTIFILE_HEADER_FILE
#pragma once

/**
 * @file MultiFileIO.h
 * @brief Implementation of a robust file I/O system that maintains multiple redundant copies
 *
 * The MultiFile class provides fault-tolerant file operations by maintaining a primary file
 * handle and up to 5 redundant copies. All read/write operations are performed across all files,
 * with validation to ensure data consistency. This is particularly useful for critical data
 * that requires protection against corruption or hardware failures.
 *
 * @author Meysam Zare
 * @date October 14, 2025
 */

#include <vector>
#include <filesystem>
#include "FileIO.h"

namespace mz {
    namespace io {

        /**
         * @brief Compares two files byte by byte to check for differences
         *
         * This utility function opens two files and compares their content,
         * reporting the number of differing bytes. It's useful for verifying
         * file integrity or comparing backup files.
         *
         * @param path1 First file path to compare
         * @param path2 Second file path to compare
         * @return Number of differing bytes, or -1 if files cannot be opened or have different sizes
         */
        int compare(std::filesystem::path const& path1, std::filesystem::path const& path2) noexcept;

        /**
         * @class MultiFile
         * @brief Provides redundant file I/O operations with multiple backup copies
         *
         * The MultiFile class manages a primary file handle and up to 5 redundant copies,
         * ensuring that all operations are mirrored across all files. Read operations
         * verify data consistency across all copies, providing protection against
         * silent data corruption.
         *
         * The class offers the same interface as standard file operations, with
         * transparent handling of the redundancy mechanism.
         */
        class MultiFile {
        private:
            /**
             * @brief Primary file handle
             *
             * This is the main file handle used for all operations. All operations
             * are first performed on this handle before being mirrored to redundant copies.
             */
            FileRW hdl;

            /**
             * @brief Array of redundant file handles
             *
             * These handles represent redundant copies of the primary file.
             * All write operations are mirrored to these files, and read operations
             * are verified against these copies to ensure data consistency.
             */
            FileRW copies[5];

            /**
             * @brief Number of active redundant copies
             *
             * Tracks the number of redundant copies currently in use.
             * Must be between 0 and 5, inclusive.
             */
            int NumCopies{ 0 };

            /**
             * @brief Flags used when opening the file
             *
             * Stores the flags used when opening the primary file,
             * so the same flags can be used when opening redundant copies.
             */
            int OpenFlags{ 0 };

            /**
             * @brief Thread-local buffer used for read comparison operations
             *
             * This buffer is used during read operations to temporarily store data
             * from redundant copies for comparison with the primary copy.
             * It's thread-local to ensure thread safety without synchronization.
             */
            inline static thread_local std::vector<uint8_t> buffer{};

        public:
            /**
             * @brief Default constructor
             *
             * Initializes a MultiFile object with no open files.
             */
            MultiFile() noexcept = default;

            /**
             * @brief Destructor
             *
             * Ensures all open file handles are properly closed.
             */
            ~MultiFile() noexcept { close(); }

            /**
             * @brief Opens the primary file with specified flags
             *
             * Opens a file using the specified path and access flags,
             * preparing it for reading and writing operations.
             *
             * @param Path Path to the file to open
             * @param flags Access mode flags (O_RDONLY, O_WRONLY, O_RDWR, etc.)
             * @return true if successful, false if an error occurred
             */
            bool open(std::filesystem::path const& Path, int flags) noexcept {
                return open_(Path, FileRW::filter_open(flags));
            }

            /**
             * @brief Opens the file with exclusive creation flags
             *
             * Opens a file with O_EXCL flag, ensuring the file is created only
             * if it doesn't already exist.
             *
             * @param Path Path to the file to open/create
             * @param flags Additional access mode flags
             * @return true if successful, false if an error occurred
             */
            bool excl(std::filesystem::path const& Path, int flags) noexcept {
                return open_(Path, FileRW::filter_excl(flags));
            }

            /**
             * @brief Creates or opens a file with specified flags
             *
             * Opens a file with O_CREAT flag, creating the file if it doesn't exist.
             *
             * @param Path Path to the file to create/open
             * @param flags Additional access mode flags
             * @return true if successful, false if an error occurred
             */
            bool create(std::filesystem::path const& Path, int flags) noexcept {
                return open_(Path, FileRW::filter_create(flags));
            }

            /**
             * @brief Adds a redundant copy of the file
             *
             * Adds a redundant file that will mirror all operations performed on
             * the primary file. Up to 5 redundant copies can be added.
             *
             * @param Path Path to the redundant file
             * @return true if successful, false if an error occurred or maximum copies exceeded
             */
            bool add(std::filesystem::path const& Path) noexcept;

            /**
             * @brief Closes all file handles
             *
             * Closes the primary file handle and all redundant copy handles.
             */
            void close() noexcept;

            //
            // STATE RETRIEVAL
            //

            /**
             * @brief Checks if any file has encountered critical errors
             *
             * @return true if any file handle is in a bad state, false otherwise
             */
            bool bad() const noexcept;

            /**
             * @brief Checks if any file operations have failed
             *
             * @return true if any file operation has failed, false otherwise
             */
            bool fail() const noexcept;

            /**
             * @brief Checks if all files are in a good state
             *
             * @return true if all file handles are in a good state, false otherwise
             */
            bool good() const noexcept;

            /**
             * @brief Checks if all files are open
             *
             * @return true if all file handles are open, false if any is closed
             */
            bool is_open() const noexcept;

            /**
             * @brief Checks if all files are closed
             *
             * @return true if all file handles are closed, false if any is open
             */
            bool is_closed() const noexcept;

            /**
             * @brief Checks end-of-file status across all files
             *
             * @return 1 if all files are at EOF, 0 if not at EOF, -1 if inconsistent or error
             */
            int eof() const noexcept;

            /**
             * @brief Gets combined error flags from all file handles
             *
             * @return Combined error_flags structure representing all errors across files
             */
            error_flags eflags() const noexcept;

            //
            // BASIC FILE OPERATIONS
            //

            /**
             * @brief Commits changes to all files
             *
             * Flushes all buffers and ensures data is written to disk for all files.
             *
             * @return true if any commit operation failed, false if all succeeded
             */
            bool commit() noexcept;

            /**
             * @brief Gets the length of the files
             *
             * @return File length in bytes, or -1 if inconsistent or error
             */
            int64_t length() const noexcept;

            /**
             * @brief Gets the size of the files (alias for length)
             *
             * @return File size in bytes, or -1 if inconsistent or error
             */
            int64_t size() const noexcept;

            /**
             * @brief Repositions the file pointer in all files
             *
             * @param Pos Position relative to the Dir parameter
             * @param Dir Direction (SEEK_SET, SEEK_CUR, SEEK_END)
             * @return New file position, or -1 if inconsistent or error
             */
            int64_t seek(int64_t Pos, int Dir) const noexcept;

            /**
             * @brief Gets the current file position in all files
             *
             * @return Current file position, or -1 if inconsistent or error
             */
            int64_t tell() const noexcept;

            /**
             * @brief Changes the size of all files
             *
             * @param NewSize New file size in bytes
             * @return true if any resize operation failed, false if all succeeded
             */
            bool chsize(int64_t NewSize) noexcept;

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
            bool bseek_set(int64_t Pos) const noexcept { return seek(Pos, SEEK_SET) < 0; }
            bool bseek_cur(int64_t Pos) const noexcept { return seek(Pos, SEEK_CUR) < 0; }
            bool bseek_end(int64_t Pos) const noexcept { return seek(Pos, SEEK_END) < 0; }

            //
            // WRITE OPERATIONS
            //

            /**
             * @brief Writes data to all files
             *
             * Writes a block of data to the primary file and all redundant copies.
             *
             * @param _Buf Pointer to the data to write
             * @param Size Number of bytes to write
             * @return true if any write operation failed, false if all succeeded
             */
            bool write(void const* _Buf, size_t Size) noexcept;

            /**
             * @brief Writes a single value to all files
             *
             * @tparam T Type of the value (must be trivially copyable)
             * @param t Value to write
             * @return true if any write operation failed, false if all succeeded
             */
            template <mz::endian::TrivialType T>
            bool write(T const& t) noexcept
            {
                void const* Ptr_{ static_cast<void const*>(&t) };
                return write(Ptr_, sizeof(t));
            }

            /**
             * @brief Writes an array of values to all files
             *
             * @tparam T Type of the values (must be trivially copyable)
             * @param P Pointer to the array of values
             * @param N Number of values to write
             * @return true if any write operation failed, false if all succeeded
             */
            template <mz::endian::TrivialType T>
            bool write(T const* P, size_t N) noexcept
            {
                void const* Ptr_{ static_cast<void const*>(P) };
                return write(Ptr_, sizeof(T) * N);
            }

            /**
             * @brief Writes a span of values to all files
             *
             * @tparam T Type of the values (must be trivially copyable)
             * @tparam N Size of the span (deduced from the parameter)
             * @param SP Span of values to write
             * @return true if any write operation failed, false if all succeeded
             */
            template <mz::endian::TrivialType T, size_t N>
            bool write(std::span<T, N> SP) noexcept
            {
                return write(SP.data(), SP.size());
            }

            //
            // ENDIAN-AWARE WRITE OPERATIONS
            // 

            /**
             * @brief Writes a single value with endian conversion
             *
             * Converts the value to the correct endianness before writing.
             *
             * @tparam T Type of the value (must support endian swapping)
             * @param t Value to write
             * @return true if any write operation failed, false if all succeeded
             */
            template <mz::endian::SwapType T>
            bool writeEndian(T t) noexcept
            {
                return write(mz::endian::as_endian(t));
            }

            /**
             * @brief Writes an array of values with endian conversion
             *
             * Converts each value to the correct endianness before writing.
             *
             * @tparam T Type of the values (must support endian swapping)
             * @param Ptr_ Pointer to the array of values
             * @param N Number of values to write
             * @return true if any write operation failed, false if all succeeded
             */
            template <mz::endian::SwapType T>
            bool writeEndian(T const* Ptr_, size_t N) noexcept
            {
                if constexpr (mz::endian::endian_mismatch)
                {
                    // Resize buffer to hold the converted values
                    buffer.resize(sizeof(T) * N);
                    memcpy(buffer.data(), Ptr_, sizeof(T) * N);

                    // Convert each value to the correct endianness
                    T* P = reinterpret_cast<T*>(buffer.data());
                    for (size_t i = 0; i < N; i++) {
                        P[i] = mz::endian::as_endian(P[i]);
                    }
                    return write(P, N);
                }
                else {
                    // No conversion needed if endianness already matches
                    return write(Ptr_, N);
                }
            }

            /**
             * @brief Writes a span of values with endian conversion
             *
             * @tparam T Type of the values (must support endian swapping)
             * @tparam N Size of the span (deduced from the parameter)
             * @param SP Span of values to write
             * @return true if any write operation failed, false if all succeeded
             */
            template <mz::endian::SwapType T, size_t N>
            bool writeEndian(std::span<T, N> SP) noexcept
            {
                return writeEndian(SP.data(), SP.size());
            }

            //
            // READ OPERATIONS
            //

            /**
             * @brief Reads data from files with verification
             *
             * Reads a block of data from the primary file and verifies it against
             * all redundant copies to ensure data consistency.
             *
             * @param Ptr_ Pointer to the buffer where data will be stored
             * @param Size Number of bytes to read
             * @return true if any read operation failed or inconsistent data, false if successful
             */
            bool read(void* Ptr_, size_t Size) const noexcept;

            /**
             * @brief Reads a single value with verification
             *
             * @tparam T Type of the value (must be trivially copyable and non-const)
             * @param t Reference where the value will be stored
             * @return true if any read operation failed or inconsistent data, false if successful
             */
            template <mz::endian::TrivialTypeNonConst T>
            bool read(T& t) const noexcept
            {
                return read((void*)&t, sizeof(T));
            }

            /**
             * @brief Reads an array of values with verification
             *
             * @tparam T Type of the values (must be trivially copyable and non-const)
             * @param P Pointer to the array where values will be stored
             * @param N Number of values to read
             * @return true if any read operation failed or inconsistent data, false if successful
             */
            template <mz::endian::TrivialTypeNonConst T>
            bool read(T* P, size_t N) const noexcept
            {
                return read((void*)P, sizeof(T) * N);
            }

            /**
             * @brief Reads values into a span with verification
             *
             * @tparam T Type of the values (must be trivially copyable and non-const)
             * @tparam CNT Size of the span (deduced from the parameter)
             * @param SP Span where values will be stored
             * @return true if any read operation failed or inconsistent data, false if successful
             */
            template <mz::endian::TrivialTypeNonConst T, size_t CNT>
            bool read(std::span<T, CNT> SP) const noexcept
            {
                return read(SP.data(), SP.size());
            }

            //
            // ENDIAN-AWARE READ OPERATIONS
            //

            /**
             * @brief Reads a single value with endian conversion
             *
             * Reads a value and converts it from file endianness to native endianness.
             *
             * @tparam T Type of the value (must support endian swapping and be non-const)
             * @param t Reference where the value will be stored
             * @return true if any read operation failed or inconsistent data, false if successful
             */
            template <mz::endian::SwapTypeNonConst T>
            bool readEndian(T& t) const noexcept
            {
                bool Error{ read(t) };
                t = mz::endian::as_endian(t);
                return Error;
            }

            /**
             * @brief Reads an array of values with endian conversion
             *
             * Reads values and converts them from file endianness to native endianness.
             *
             * @tparam T Type of the values (must support endian swapping and be non-const)
             * @param P Pointer to the array where values will be stored
             * @param N Number of values to read
             * @return true if any read operation failed or inconsistent data, false if successful
             */
            template <mz::endian::SwapTypeNonConst T>
            bool readEndian(T* P, size_t N) const noexcept
            {
                bool Error = read(P, N);
                if (!Error) {
                    for (size_t i = 0; i < N; i++) {
                        P[i] = mz::endian::as_endian(P[i]);
                    }
                }
                return Error;
            }

            /**
             * @brief Reads values into a span with endian conversion
             *
             * @tparam T Type of the values (must support endian swapping and be non-const)
             * @tparam CNT Size of the span (deduced from the parameter)
             * @param SP Span where values will be stored
             * @return true if any read operation failed or inconsistent data, false if successful
             */
            template <mz::endian::TrivialTypeNonConst T, size_t CNT>
            bool readEndian(std::span<T, CNT> SP) const noexcept
            {
                return readEndian(SP.data(), SP.size());
            }

        private:
            /**
             * @brief Internal file open implementation
             *
             * Opens the primary file with the specified flags and resets the redundant copy counter.
             *
             * @param Path Path to the file to open
             * @param Flags Access mode flags
             * @return true if successful, false if an error occurred
             */
            bool open_(std::filesystem::path const& Path, int Flags) noexcept;
        };
    }
}

#endif // IO_MULTIFILE_HEADER_FILE
