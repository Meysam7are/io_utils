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

/**
 * @file MultifileIO.cpp
 * @brief Implementation of the MultiFile class for redundant file operations
 *
 * This file contains the implementation of the MultiFile class, which provides
 * fault-tolerant file I/O through redundant copies. The class maintains a primary
 * file handle and up to 5 redundant copies, ensuring data consistency across all files.
 *
 * @author Meysam Zare
 * @date October 14, 2025
 */

#include <format>
#include <iostream>
#include <cstring> // For memcmp

#include "MultiFileIO.h"
#include "time_conversions.h"

namespace mz {
    namespace io {

        /**
         * @brief Opens the primary file with specified flags
         *
         * This private method is called by the public open(), excl(), and create()
         * methods to initialize the primary file handle. It resets the number of
         * redundant copies and stores the flags for later use when opening copies.
         *
         * @param Path Path to the file to open
         * @param Flags File access flags (e.g., O_RDWR, O_BINARY)
         * @return true if the file was opened successfully, false otherwise
         */
        bool MultiFile::open_(std::filesystem::path const& Path, int Flags) noexcept
        {
            // Reset the number of redundant copies to 0
            NumCopies = 0;

            // Store the flags for use when opening redundant copies
            OpenFlags = Flags;

            // Attempt to open the primary file handle
            return hdl.open_(Path, Flags);
        }

        /**
         * @brief Adds a redundant copy of the file
         *
         * Creates an additional file that mirrors all operations on the primary file.
         * Up to 5 redundant copies can be maintained to provide fault tolerance.
         *
         * @param Path Path to the redundant file
         * @return true if the file was added successfully, false otherwise
         */
        bool MultiFile::add(std::filesystem::path const& Path) noexcept
        {
            // Validate that we haven't exceeded the maximum number of copies
            if (NumCopies < 0 || NumCopies >= 5) {
                return false;
            }

            // Try to open the redundant copy with the same flags as the primary file
            if (!copies[NumCopies].open_(Path, OpenFlags)) {
                return false;
            }

            // Increment the count of redundant copies
            ++NumCopies;
            return true;
        }

        /**
         * @brief Closes all open file handles
         *
         * Closes the primary file handle and all redundant copy handles.
         * This ensures proper resource cleanup and flushes any pending writes.
         */
        void MultiFile::close() noexcept {
            // Close the primary file handle first
            hdl.close();

            // Close all redundant copy handles
            for (int i = 0; i < NumCopies; i++) {
                copies[i].close();
            }
        }

        //
        // STATE RETRIEVAL METHODS
        //

        /**
         * @brief Checks if any file handle has encountered critical errors
         *
         * A MultiFile is considered "bad" if any of its file handles (primary or redundant)
         * is in a bad state. This is a conservative approach that ensures data integrity.
         *
         * @return true if any file handle is in a bad state, false if all are good
         */
        bool MultiFile::bad() const noexcept {
            // Start with the state of the primary file handle
            bool B{ hdl.bad() };

            // Check each redundant copy and combine with OR operation
            // (bad if ANY file is bad)
            for (int i = 0; i < NumCopies; i++) {
                B |= copies[i].bad();
            }
            return B;
        }

        /**
         * @brief Checks if any file operations have failed
         *
         * A MultiFile is considered to have "failed" if any of its file handles
         * has a failure flag set. This includes both critical errors and
         * operation-specific failures.
         *
         * @return true if any file handle has failed, false if all are successful
         */
        bool MultiFile::fail() const noexcept {
            // Start with the state of the primary file handle
            bool B{ hdl.fail() };

            // Check each redundant copy and combine with OR operation
            // (fail if ANY file has failed)
            for (int i = 0; i < NumCopies; i++) {
                B |= copies[i].fail();
            }
            return B;
        }

        /**
         * @brief Checks if all files are in a good state
         *
         * A MultiFile is considered "good" only if ALL of its file handles
         * are in a good state. This ensures that redundancy is maintained.
         *
         * @return true if all file handles are good, false otherwise
         */
        bool MultiFile::good() const noexcept {
            // Start with the state of the primary file handle
            bool B{ hdl.good() };

            // Check each redundant copy and combine with AND operation
            // (good only if ALL files are good)
            for (int i = 0; i < NumCopies; i++) {
                B &= copies[i].good();
            }
            return B;
        }

        /**
         * @brief Checks if all files are open
         *
         * A MultiFile is considered "open" only if ALL of its file handles
         * are open. This ensures that redundancy is maintained.
         *
         * @return true if all file handles are open, false if any is closed
         */
        bool MultiFile::is_open() const noexcept {
            // Start with the state of the primary file handle
            bool B{ hdl.is_open() };

            // Check each redundant copy and combine with AND operation
            // (open only if ALL files are open)
            for (int i = 0; i < NumCopies; i++) {
                B &= copies[i].is_open();
            }
            return B;
        }

        /**
         * @brief Checks if all files are closed
         *
         * Note: There appears to be a logical error in the original implementation.
         * This method is checking if any file is open, not if all files are closed.
         * The correct implementation should check if all files are closed.
         *
         * @return true if all file handles are closed, false if any is open
         */
        bool MultiFile::is_closed() const noexcept {
            // Start with the state of the primary file handle
            // Note: This logic checks if primary is OPEN, not CLOSED
            bool B{ hdl.is_open() };

            // Check each redundant copy and combine with OR operation
            // (Not closed if ANY file is open)
            for (int i = 0; i < NumCopies; i++) {
                B |= copies[i].is_open();
            }
            return !B;  // Return the negation for "is_closed" semantics
        }

        /**
         * @brief Checks end-of-file status across all files
         *
         * Returns EOF status, but only if all files report the same status.
         * If any file reports a different EOF status, returns -1 to indicate
         * an inconsistent state.
         *
         * @return 1 if all files are at EOF, 0 if none are at EOF, -1 if inconsistent or error
         */
        int MultiFile::eof() const noexcept {
            // Get EOF status from primary file
            int R{ hdl.eof() };

            // Compare with each redundant copy
            for (int i = 0; i < NumCopies; i++) {
                // If any copy has a different EOF status, return -1 (error/inconsistent)
                R = copies[i].eof() != R ? -1 : R;
            }
            return R;
        }

        /**
         * @brief Gets combined error flags from all file handles
         *
         * Combines the error flags from all file handles using bitwise OR operations.
         * This provides a comprehensive view of all errors encountered across all files.
         *
         * @return Combined error_flags structure representing all errors
         */
        error_flags MultiFile::eflags() const noexcept {
            // Start with error flags from the primary file
            error_flags fl{ hdl.eflags() };

            // Combine with error flags from each redundant copy
            for (int i = 0; i < NumCopies; i++) {
                fl.value |= copies[i].e_flags.value;
            }
            return fl;
        }

        //
        // BASIC FILE OPERATIONS
        //

        /**
         * @brief Commits changes to all files
         *
         * Flushes all buffers and ensures data is written to disk for all files.
         * This operation is crucial for ensuring data durability across all copies.
         *
         * @return true if any commit operation failed, false if all succeeded
         */
        bool MultiFile::commit() noexcept
        {
            // First commit the primary file
            bool B{ hdl.commit() };

            // Then commit each redundant copy
            for (int i = 0; i < NumCopies; i++) {
                // Use AND to accumulate success status (success only if ALL commits succeed)
                B &= copies[i].commit();
            }
            return B;
        }

        /**
         * @brief Gets the length of the files
         *
         * Returns the file length, but only if all files report the same length.
         * If any file reports a different length, returns -1 to indicate
         * an inconsistent state.
         *
         * @return File length in bytes, or -1 if inconsistent or error
         */
        int64_t MultiFile::length() const noexcept
        {
            // Get length from primary file
            int64_t L{ hdl.length() };

            // Compare with each redundant copy
            for (int i = 0; i < NumCopies; i++) {
                // If any copy has a different length, return -1 (error/inconsistent)
                L = copies[i].length() != L ? -1 : L;
            }
            return L;
        }

        /**
         * @brief Gets the size of the files (alias for length)
         *
         * Simple alias for the length() method for API consistency.
         *
         * @return File size in bytes, or -1 if inconsistent or error
         */
        int64_t MultiFile::size() const noexcept {
            return length();
        }

        /**
         * @brief Repositions the file pointer in all files
         *
         * Moves the file pointer to the specified position in all files.
         * If any file reports a different position after the operation,
         * returns -1 to indicate an inconsistent state.
         *
         * @param Pos Position relative to the Dir parameter
         * @param Dir Direction (SEEK_SET, SEEK_CUR, SEEK_END)
         * @return New file position, or -1 if inconsistent or error
         */
        int64_t MultiFile::seek(int64_t Pos, int Dir) const noexcept
        {
            // Seek in primary file first
            int64_t P{ hdl.seek(Pos, Dir) };

            // Then seek in each redundant copy
            for (int i = 0; i < NumCopies; i++) {
                // If any copy reports a different position, return -1 (error/inconsistent)
                P = P != copies[i].seek(Pos, Dir) ? -1 : P;
            }
            return P;
        }

        /**
         * @brief Gets the current file position in all files
         *
         * Returns the current file position, but only if all files report the same position.
         * If any file reports a different position, returns -1 to indicate
         * an inconsistent state.
         *
         * @return Current file position, or -1 if inconsistent or error
         */
        int64_t MultiFile::tell() const noexcept
        {
            // Get position from primary file
            int64_t P{ hdl.tell() };

            // Compare with each redundant copy
            for (int i = 0; i < NumCopies; i++) {
                // If any copy reports a different position, return -1 (error/inconsistent)
                P = P != copies[i].tell() ? -1 : P;
            }
            return P;
        }

        /**
         * @brief Changes the size of all files
         *
         * Resizes all files to the specified size. This operation is used to
         * extend or truncate files.
         *
         * @param NewSize New file size in bytes
         * @return true if any resize operation failed, false if all succeeded
         */
        bool MultiFile::chsize(int64_t NewSize) noexcept
        {
            // First resize the primary file
            bool Error{ hdl.chsize(NewSize) };

            // Then resize each redundant copy
            for (int i = 0; i < NumCopies; i++) {
                // Use OR to accumulate error status (error if ANY resize fails)
                Error |= copies[i].chsize(NewSize);
            }
            return Error;
        }

        /**
         * @brief Writes data to all files
         *
         * Writes a block of data to the primary file and all redundant copies.
         * This ensures all files maintain identical content.
         *
         * @param _Buf Pointer to the data to write
         * @param Size Number of bytes to write
         * @return true if any write operation failed, false if all succeeded
         */
        bool MultiFile::write(void const* _Buf, size_t Size) noexcept
        {
            // First write to the primary file
            bool Error = hdl.write(_Buf, Size);

            // Then write to each redundant copy
            for (int i = 0; i < NumCopies; i++) {
                // Use OR to accumulate error status (error if ANY write fails)
                Error = copies[i].write(_Buf, Size) || Error;
            }
            return Error;
        }

        /**
         * @brief Reads data from files with verification
         *
         * Reads a block of data from the primary file and verifies it against
         * all redundant copies to ensure data consistency.
         *
         * @param Ptr_ Pointer to the buffer where data will be stored
         * @param Size Number of bytes to read
         * @return true if any read operation failed or data is inconsistent, false if successful
         */
        bool MultiFile::read(void* Ptr_, size_t Size) const noexcept
        {
            // Resize the verification buffer to accommodate the requested data
            buffer.resize(Size);
            void* Buf_ = static_cast<void*>(buffer.data());

            // Read from the primary file into the user's buffer
            bool Error = hdl.read(Ptr_, Size) || hdl.fail();

            // Read from each redundant copy into our verification buffer and compare
            for (int i = 0; i < NumCopies; i++) {
                // Accumulate errors: read operation failed OR file is in failure state
                Error = copies[i].read(Buf_, Size) || copies[i].fail() || Error;

                // Compare the data from the primary file with the redundant copy
                // If they differ, set the error flag
                Error = memcmp(Ptr_, Buf_, Size) != 0 || Error;
            }
            return Error;
        }

        /**
         * @brief Compares two files byte by byte to check for differences
         *
         * This utility function opens two files and compares their content,
         * reporting the number of differing bytes.
         *
         * @param P1 Path to the first file
         * @param P2 Path to the second file
         * @return Number of differing bytes, or -1 if files cannot be opened or have different sizes
         */
        int compare(std::filesystem::path const& P1, std::filesystem::path const& P2) noexcept
        {
            // Open both files in read-only, binary mode
            mz::io::FileRO F1, F2;

            // Create buffers for reading file chunks
            std::vector<uint8_t> V1(4096);  // 4KB buffer for first file
            std::vector<uint8_t> V2(4096);  // 4KB buffer for second file

            // Try to open first file
            if (!F1.open(P1, O_BINARY)) {
                return -1;  // Error: couldn't open first file
            }

            // Try to open second file
            if (!F2.open(P2, O_BINARY)) {
                return -1;  // Error: couldn't open second file
            }

            // Get file lengths
            int64_t L1 = F1.length();
            int64_t L2 = F2.length();

            // Files must be the same size to be comparable
            if (L1 < L2) {
                return -1;  // Error: first file is smaller than second
            }
            if (L1 > L2) {
                return -1;  // Error: first file is larger than second
            }

            // Counter for number of differing bytes
            int64_t cnt{ 0 };

            // Read and compare files in chunks
            while (L1)
            {
                // Determine chunk size (up to 4KB)
                int64_t L = L1 > 4096 ? 4096 : L1;

                // Read chunk from first file
                if (F1.read(V1.data(), L)) {
                    return -1;  // Error: couldn't read from first file
                }

                // Read chunk from second file
                if (F2.read(V2.data(), L)) {
                    return -1;  // Error: couldn't read from second file
                }

                // Compare bytes in the chunks
                for (int64_t i = 0; i < L; i++) {
                    if (V1[i] != V2[i]) {
                        // Found a difference, increment counter
                        ++cnt;

                        // Calculate absolute position in the file
                        int64_t j = L2 - L1 + i;

                        // Output detailed information about the difference
                        std::cout << std::format("V1[{}]:{}  V2[{}]:{}  cnt:{}  \n",
                            j, V1[i], j, V2[i], cnt);
                    }
                }

                // Reduce remaining bytes counter
                L1 -= L;
            }

            // Return the total number of differences found
            return int(cnt);
        }
    }
}
