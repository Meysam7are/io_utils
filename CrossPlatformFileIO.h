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

#ifndef MZ_CROSS_PLATFORM_FILE_IO_HEADER
#define MZ_CROSS_PLATFORM_FILE_IO_HEADER
#pragma once

/**
 * @file CrossPlatformFileIO.h
 * @brief Cross-platform file I/O operation abstractions
 *
 * This header provides platform-agnostic file operation functions that internally
 * select the appropriate implementation based on the target platform. It abstracts
 * differences between Windows, macOS, various BSD variants, and other POSIX-compliant
 * systems, presenting a consistent interface for file operations.
 *
 * All functions use native file descriptors (int) and provide error codes that
 * are consistent across platforms.
 *
 * @author Meysam Zare
 * @date October 14, 2025
 */

#include <cstdint>
#include <filesystem>

 // Platform-specific includes
#ifdef _MSC_VER
    // Windows includes
#include <io.h>      // For _open, _close, etc.
#include <fcntl.h>   // For O_RDONLY, O_WRONLY, etc.
#include <share.h>   // For _SH_DENYNO and other share modes
#include <sys/stat.h> // For _S_IREAD, _S_IWRITE
#else
    // POSIX-compliant system includes
#include <unistd.h>  // For close, read, write, lseek, etc.
#include <sys/stat.h> // For stat, fstat
#include <fcntl.h>   // For O_RDONLY, O_WRONLY, etc.
#include <errno.h>   // For error codes

// For platforms that might need specific definitions
#if defined(__APPLE__) || defined(__sun) || defined(__FreeBSD__) || \
        defined(__OpenBSD__) || defined(__NetBSD__) || defined(__linux__)
        // Common POSIX-compliant systems - additional includes can be added here if needed
#endif
#endif

namespace mz {
    namespace io {

        /**
         * @brief Opens a file and returns a file descriptor
         *
         * Creates or opens a file with the specified access mode. This function abstracts
         * the platform-specific differences between Windows (_sopen_s) and POSIX (open).
         *
         * @param fd Reference to an integer where the file descriptor will be stored
         * @param Path File path to open
         * @param OperationMode File access mode flags (e.g., O_RDONLY, O_WRONLY, O_RDWR)
         * @return Error code (0 for success, non-zero for errors)
         */
        inline int OPEN(int& fd, std::filesystem::path const& Path, int OperationMode) noexcept
        {
#ifdef _MSC_VER
            // Windows implementation: _sopen_s with sharing mode and permissions
            // _SH_DENYNO allows other processes to access the file concurrently
            // _S_IREAD | _S_IWRITE gives read and write permissions
            return _sopen_s(&fd, Path.string().c_str(), OperationMode, _SH_DENYNO, _S_IREAD | _S_IWRITE);
#else
            // POSIX implementation
            fd = ::open(Path.string().c_str(), OperationMode, 0666); // 0666 = rw-rw-rw-
            return (fd == -1) ? errno : 0;
#endif
        }

        /**
         * @brief Closes a file descriptor
         *
         * Closes an open file handle, releasing associated system resources.
         *
         * @param fd File descriptor to close
         * @return Error code (0 for success, non-zero for errors)
         */
        inline int CLOSE(int fd) noexcept
        {
#ifdef _MSC_VER
            return _close(fd);
#else
            return ::close(fd);
#endif
        }

        /**
         * @brief Commits pending changes to disk
         *
         * Flushes all buffers associated with the file to disk, ensuring data durability.
         * On Windows, this uses _commit, while POSIX systems use fsync.
         *
         * @param fd File descriptor
         * @return Error code (0 for success, non-zero for errors)
         */
        inline int COMMIT(int fd) noexcept
        {
#ifdef _MSC_VER
            return _commit(fd);
#else
            return ::fsync(fd);
#endif
        }

        /**
         * @brief Gets the length of a file
         *
         * Retrieves the total size of the file in bytes.
         *
         * @param fd File descriptor
         * @return File size in bytes, or -1 if an error occurred
         */
        inline long long LENGTH(int fd) noexcept
        {
#ifdef _MSC_VER
            return _filelengthi64(fd);
#else
            // Use fstat to get file information, then extract the size
            struct stat buf;
            if (::fstat(fd, &buf) != 0)
                return -1;
            return buf.st_size;
#endif
        }

        /**
         * @brief Checks if the file position is at end-of-file
         *
         * Determines whether the current file position is at or beyond the end of the file.
         *
         * @param fd File descriptor
         * @return 1 if at EOF, 0 if not at EOF, -1 if an error occurred
         */
        inline int END_OF_FILE(int fd) noexcept
        {
#ifdef _MSC_VER
            return _eof(fd);
#else
            // POSIX doesn't have a direct EOF function, so we implement it:
            // 1. Try to read a byte
            // 2. If we get 0 bytes, we're at EOF
            // 3. If the read succeeds, move back one position
            uint8_t dummy;
            ssize_t bytes_read = ::read(fd, &dummy, 1);

            if (bytes_read == 0)
                return 1; // EOF

            if (bytes_read == -1)
                return -1; // Error

            // Move position back after successful read
            if (::lseek(fd, -1, SEEK_CUR) == -1)
                return -1; // Error

            return 0; // Not EOF
#endif
        }

        /**
         * @brief Sets the file position pointer
         *
         * Repositions the file position pointer to a specified location.
         *
         * @param fd File descriptor
         * @param Offset Position offset relative to Origin
         * @param Origin Reference point (SEEK_SET, SEEK_CUR, or SEEK_END)
         * @return New file position, or -1 if an error occurred
         */
        inline long long SEEK64(int fd, long long Offset, int Origin) noexcept
        {
#ifdef _MSC_VER
            return _lseeki64(fd, Offset, Origin);
#else
#if defined(__APPLE__) || defined(__linux__)
            return ::lseek(fd, Offset, Origin);
#else
            // For systems that have lseek64
            return ::lseek64(fd, Offset, Origin);
#endif
#endif
        }

        /**
         * @brief Gets the current file position
         *
         * Retrieves the current position of the file pointer.
         *
         * @param fd File descriptor
         * @return Current file position in bytes from start, or -1 if an error occurred
         */
        inline long long TELL64(int fd) noexcept
        {
#ifdef _MSC_VER
            return _telli64(fd);
#else
            // POSIX doesn't have a direct tell function; use lseek with offset 0 from current position
#if defined(__APPLE__) || defined(__linux__)
            return ::lseek(fd, 0, SEEK_CUR);
#else
            return ::lseek64(fd, 0, SEEK_CUR);
#endif
#endif
        }

        /**
         * @brief Resizes a file
         *
         * Changes the size of a file, either truncating or extending it.
         *
         * @param fd File descriptor
         * @param NewLength New file size in bytes
         * @return Error code (0 for success, non-zero for errors)
         */
        inline int CHANGE_SIZE(int fd, long long NewLength) noexcept
        {
#ifdef _MSC_VER
            return _chsize_s(fd, NewLength);
#else
            return ::ftruncate(fd, NewLength);
#endif
        }

        /**
         * @brief Reads data from a file
         *
         * Reads a specified number of bytes from a file into a buffer.
         *
         * @param fd File descriptor
         * @param _Buf Buffer to store the read data
         * @param Size Number of bytes to read
         * @return Number of bytes read, or -1 if an error occurred
         */
        inline int READ(int fd, void* _Buf, unsigned Size) noexcept
        {
#ifdef _MSC_VER
            return _read(fd, _Buf, Size);
#else
            return static_cast<int>(::read(fd, _Buf, Size));
#endif
        }

        /**
         * @brief Writes data to a file
         *
         * Writes a specified number of bytes from a buffer to a file.
         *
         * @param fd File descriptor
         * @param _Buf Buffer containing data to write
         * @param Size Number of bytes to write
         * @return Number of bytes written, or -1 if an error occurred
         */
        inline int WRITE(int fd, void const* _Buf, unsigned Size) noexcept
        {
#ifdef _MSC_VER
            return _write(fd, _Buf, Size);
#else
            return static_cast<int>(::write(fd, _Buf, Size));
#endif
        }

        /**
         * @brief Creates platform-independent file access modes
         *
         * This namespace contains constants that define common file access modes
         * that will be mapped to the appropriate platform-specific values.
         */
        namespace FileMode {
            // Common file access modes
#ifdef _MSC_VER
            constexpr int Read = _O_RDONLY;
            constexpr int Write = _O_WRONLY;
            constexpr int ReadWrite = _O_RDWR;
            constexpr int Create = _O_CREAT;
            constexpr int Truncate = _O_TRUNC;
            constexpr int Append = _O_APPEND;
            constexpr int Binary = _O_BINARY;
            constexpr int Text = _O_TEXT;
            constexpr int Exclusive = _O_EXCL;
#else
            constexpr int Read = O_RDONLY;
            constexpr int Write = O_WRONLY;
            constexpr int ReadWrite = O_RDWR;
            constexpr int Create = O_CREAT;
            constexpr int Truncate = O_TRUNC;
            constexpr int Append = O_APPEND;
            constexpr int Binary = 0;  // POSIX doesn't distinguish binary mode
            constexpr int Text = 0;  // POSIX doesn't distinguish text mode
            constexpr int Exclusive = O_EXCL;
#endif
        }

        /**
         * @brief Helper function to check if a file exists
         *
         * Uses the C++17 filesystem library to check for file existence
         * in a platform-independent way.
         *
         * @param path Path to the file
         * @return true if file exists, false otherwise
         */
        inline bool FILE_EXISTS(const std::filesystem::path& path) noexcept
        {
            std::error_code ec;
            return std::filesystem::exists(path, ec) && !std::filesystem::is_directory(path, ec);
        }

        /**
         * @brief Helper function to get file size
         *
         * Uses the C++17 filesystem library to get file size
         * in a platform-independent way.
         *
         * @param path Path to the file
         * @return Size of the file in bytes, or -1 if an error occurred
         */
        inline long long FILE_SIZE(const std::filesystem::path& path) noexcept
        {
            std::error_code ec;
            auto size = std::filesystem::file_size(path, ec);
            if (ec) {
                return -1;
            }
            return static_cast<long long>(size);
        }

        /**
         * @brief Helper function to delete a file
         *
         * Uses the C++17 filesystem library to delete a file
         * in a platform-independent way.
         *
         * @param path Path to the file
         * @return true if file was deleted, false otherwise
         */
        inline bool FILE_REMOVE(const std::filesystem::path& path) noexcept
        {
            std::error_code ec;
            return std::filesystem::remove(path, ec);
        }

        /**
         * @brief Helper function to rename a file
         *
         * Uses the C++17 filesystem library to rename a file
         * in a platform-independent way.
         *
         * @param oldPath Current path of the file
         * @param newPath New path for the file
         * @return true if file was renamed, false otherwise
         */
        inline bool FILE_RENAME(const std::filesystem::path& oldPath, const std::filesystem::path& newPath) noexcept
        {
            std::error_code ec;
            std::filesystem::rename(oldPath, newPath, ec);
            return !ec;
        }

        /**
         * @brief Helper function to create a directory
         *
         * Uses the C++17 filesystem library to create a directory
         * in a platform-independent way.
         *
         * @param path Path to the directory
         * @return true if directory was created, false otherwise
         */
        inline bool DIR_CREATE(const std::filesystem::path& path) noexcept
        {
            std::error_code ec;
            return std::filesystem::create_directory(path, ec);
        }

    } // namespace io
} // namespace mz

#endif // MZ_CROSS_PLATFORM_FILE_IO_HEADER
