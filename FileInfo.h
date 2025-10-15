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

#ifndef MZ_FILE_INFO_HEADER_FILE
#define MZ_FILE_INFO_HEADER_FILE

#pragma once

#include <cstdint>
#include <array>
#include <map>
#include <vector>
#include <string>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <optional>
#include <memory>

namespace mz {

    /**
     * @brief Error codes for file operations
     */
    enum class FileError : int8_t {
        None = 0,               ///< No error
        StatusFailed = -1,      ///< Failed to get file status
        CanonicalFailed = -2,   ///< Failed to get canonical path
        TimeFailed = -3,        ///< Failed to get file time
        HardLinkFailed = -4,    ///< Failed to get hard link count
        SizeFailed = -5,        ///< Failed to get file size
        DirectoryEntryFailed = -6 ///< Failed to read directory entry
    };

    /**
     * @brief Unit sizes for file size formatting
     */
    struct FileSizeConstants {
        static constexpr int64_t KB = 1024;
        static constexpr int64_t MB = KB * 1024;
        static constexpr int64_t GB = MB * 1024;
        static constexpr int64_t TB = GB * 1024;
    };

    /**
     * @brief Represents information about a single file or directory.
     *
     * This class provides a comprehensive view of file system entry properties,
     * including size, type, permissions, modification time, and more.
     * It works across platforms through std::filesystem and provides methods
     * for convenient formatting and display of file properties.
     */
    class FileInfo {
    public:
        std::filesystem::path m_path{};                ///< Full path to the file or directory
        std::filesystem::file_type m_type{ std::filesystem::file_type::none }; ///< Type of the file
        std::filesystem::perms m_permissions{ std::filesystem::perms::none };  ///< File permissions
        int64_t m_size{ 0 };                           ///< File size in bytes
        int64_t m_modificationTime{ 0 };               ///< Last write time as seconds since epoch
        int64_t m_hardLinkCount{ 0 };                  ///< Number of hard links

        /**
         * @brief Default constructor
         */
        constexpr FileInfo() noexcept = default;

        /**
         * @brief Constructs FileInfo from a filesystem path
         * @param filePath Path to the file or directory
         *
         * Performs several filesystem operations to gather comprehensive file information.
         * Each operation is protected by error handling.
         */
        explicit FileInfo(const std::filesystem::path& filePath) noexcept;

        /**
         * @brief Constructs FileInfo directly from a directory iterator
         * @param it Directory iterator positioned at the desired entry
         *
         * This constructor is optimized for traversing directories, as it
         * takes file information directly from the iterator.
         */
        explicit FileInfo(const std::filesystem::directory_iterator& it) noexcept;

        /**
         * @brief Constructs FileInfo from a directory iterator with error handling
         * @param it Directory iterator positioned at the desired entry
         * @param ec Error code that will be set if operations fail
         */
        FileInfo(const std::filesystem::directory_iterator& it, std::error_code& ec) noexcept;

        /**
         * @brief Checks if the file or directory exists
         * @return True if exists, false otherwise
         */
        [[nodiscard]] constexpr bool exists() const noexcept {
            return m_type != std::filesystem::file_type::none &&
                m_type != std::filesystem::file_type::not_found;
        }

        /**
         * @brief Checks if the path is a directory
         * @return True if directory, false otherwise
         */
        [[nodiscard]] constexpr bool isDirectory() const noexcept {
            return m_type == std::filesystem::file_type::directory;
        }

        /**
         * @brief Checks if the path is a regular file
         * @return True if regular file, false otherwise
         */
        [[nodiscard]] constexpr bool isRegularFile() const noexcept {
            return m_type == std::filesystem::file_type::regular;
        }

        /**
         * @brief Checks if the path is a symbolic link
         * @return True if symlink, false otherwise
         */
        [[nodiscard]] constexpr bool isSymlink() const noexcept {
            return m_type == std::filesystem::file_type::symlink;
        }

        /**
         * @brief Checks if the path is a file (regular or symlink)
         * @return True if file, false otherwise
         */
        [[nodiscard]] constexpr bool isFile() const noexcept {
            return isRegularFile() || isSymlink();
        }

        /**
         * @brief Returns the last write time as a formatted string
         * @return Formatted time string (YYYY-MM-DD HH:MM:SS)
         */
        [[nodiscard]] std::string formatTime() const noexcept;

        /**
         * @brief Returns the last write time as a formatted wide string
         * @return Formatted wide time string (YYYY-MM-DD HH:MM:SS)
         */
        [[nodiscard]] std::wstring formatTimeWide() const noexcept;

        /**
         * @brief Returns the file size as a human-readable string
         * @param exact If true, shows exact bytes; otherwise, uses units
         * @return Human-readable file size string
         */
        [[nodiscard]] std::string formatSize(bool exact = false) const noexcept;

        /**
         * @brief Returns the file size as a human-readable wide string
         * @param exact If true, shows exact bytes; otherwise, uses units
         * @return Human-readable file size wide string
         */
        [[nodiscard]] std::wstring formatSizeWide(bool exact = false) const noexcept;

        /**
         * @brief Returns a string describing the file type
         * @return File type as string
         */
        [[nodiscard]] constexpr std::string_view getTypeAsString() const noexcept;

        /**
         * @brief Returns a wide string describing the file type
         * @return File type as wide string
         */
        [[nodiscard]] constexpr std::wstring_view getTypeAsWideString() const noexcept;

        /**
         * @brief Returns a formatted string with file size and status
         * @return Formatted string
         */
        [[nodiscard]] std::string formatSizeAndStatus() const noexcept;

        /**
         * @brief Returns a formatted wide string with file size and status
         * @return Formatted wide string
         */
        [[nodiscard]] std::wstring formatSizeAndStatusWide() const noexcept;

        /**
         * @brief Returns a formatted string for listing files, with tab size
         * @param tabSize Minimum tab size for formatting
         * @return Formatted list string
         */
        [[nodiscard]] std::string formatList(size_t tabSize = 14) const noexcept;

        /**
         * @brief Returns a formatted wide string for listing files, with tab size
         * @param tabSize Minimum tab size for formatting
         * @return Formatted wide list string
         */
        [[nodiscard]] std::wstring formatListWide(size_t tabSize = 14) const noexcept;

        /**
         * @brief Returns a formatted string for listing file names, with tab size
         * @param tabSize Minimum tab size for formatting
         * @return Formatted list string with file name
         */
        [[nodiscard]] std::string formatListWithName(size_t tabSize = 14) const noexcept;

        /**
         * @brief Returns a formatted wide string for listing file names, with tab size
         * @param tabSize Minimum tab size for formatting
         * @return Formatted wide list string with file name
         */
        [[nodiscard]] std::wstring formatListWithNameWide(size_t tabSize = 14) const noexcept;

        /**
         * @brief Returns an error description if size is negative
         * @return Error string or empty if no error
         */
        [[nodiscard]] std::string getErrorDescription() const noexcept;

        /**
         * @brief Check if the FileInfo has an error
         * @return True if an error occurred during file information gathering
         */
        [[nodiscard]] constexpr bool hasError() const noexcept {
            return m_size < 0;
        }

        /**
         * @brief Get the error code
         * @return FileError enumeration value
         */
        [[nodiscard]] constexpr FileError getError() const noexcept {
            return m_size < 0 ? static_cast<FileError>(m_size) : FileError::None;
        }

    private:
        /**
         * @brief Converts file_time_type to seconds since epoch
         * @param fileDateTime File time
         * @return Seconds since epoch
         */
        static int64_t secondsSinceEpochFromFileTime(std::filesystem::file_time_type fileDateTime) noexcept;

        /**
         * @brief Helper function to format size values with units
         * @param size The size in bytes
         * @param threshold The threshold for this size unit
         * @param divider The value to divide by (e.g., 1024 for KB)
         * @param unitSuffix The suffix to use (e.g., "KB")
         * @param precision Number of decimal places
         * @return Formatted size string
         */
        template<typename CharT>
        static std::basic_string<CharT> formatSizeWithUnit(int64_t size, int64_t threshold,
            int64_t divider, const CharT* unitSuffix,
            int precision = 0) noexcept;
    };

    /**
     * @brief Represents information about a folder and its contents
     *
     * This class provides methods for enumerating and analyzing the contents
     * of a directory, including file counts and types.
     */
    class FolderInfo {
    public:
        std::filesystem::path m_path{};            ///< Path to the folder
        std::vector<FileInfo> m_items;             ///< List of FileInfo objects for each item in the folder
        int m_itemCount{ 0 };                      ///< Total number of items
        int m_fileCount{ 0 };                      ///< Number of files (excluding directories and errors)

        /**
         * @brief Default constructor
         */
        FolderInfo() noexcept = default;

        /**
         * @brief Construct a FolderInfo for the given path
         * @param folderPath Path to initialize with
         */
        explicit FolderInfo(const std::filesystem::path& folderPath) noexcept {
            assignFolder(folderPath);
        }

        /**
         * @brief Populates the FolderInfo with contents of the specified folder
         * @param folderPath Path to the folder
         * @return Reference to this object for method chaining
         */
        FolderInfo& assignFolder(const std::filesystem::path& folderPath) noexcept;

        /**
         * @brief Sort items by name
         * @param ascending True for ascending order, false for descending
         * @return Reference to this object for method chaining
         */
        FolderInfo& sortByName(bool ascending = true) noexcept;

        /**
         * @brief Sort items by size
         * @param ascending True for ascending order, false for descending
         * @return Reference to this object for method chaining
         */
        FolderInfo& sortBySize(bool ascending = true) noexcept;

        /**
         * @brief Sort items by modification time
         * @param ascending True for ascending order, false for descending
         * @return Reference to this object for method chaining
         */
        FolderInfo& sortByTime(bool ascending = true) noexcept;

        /**
         * @brief Filter items to include only files
         * @return Reference to this object for method chaining
         */
        FolderInfo& filterOnlyFiles() noexcept;

        /**
         * @brief Filter items to include only directories
         * @return Reference to this object for method chaining
         */
        FolderInfo& filterOnlyDirectories() noexcept;

        /**
         * @brief Calculate total size of all items in folder
         * @return Total size in bytes
         */
        [[nodiscard]] int64_t calculateTotalSize() const noexcept;
    };

    //------------------------------------------------------------------------------
    // Implementation of inline and template methods
    //------------------------------------------------------------------------------

    inline FileInfo::FileInfo(const std::filesystem::path& filePath) noexcept : m_path{ filePath } {
        std::error_code ec;
        std::filesystem::file_status status;
        std::filesystem::file_time_type time;

        // Try to get symbolic link status
        if (status = std::filesystem::symlink_status(filePath, ec); ec) {
            m_size = static_cast<int64_t>(FileError::StatusFailed);
            return;
        }

        // Store the file type from status
        m_type = status.type();
        m_permissions = status.permissions();

        // Skip some operations if not a regular file or symlink
        if (!isFile() && !isDirectory()) {
            return;
        }

        // Try to get canonical path (resolves symlinks)
        std::filesystem::path canonicalPath;
        if (canonicalPath = std::filesystem::canonical(filePath, ec); ec) {
            if (!isSymlink()) { // It's normal for symlinks to fail canonicalization
                m_size = static_cast<int64_t>(FileError::CanonicalFailed);
                return;
            }
        }
        else {
            // Update path to canonical version
            m_path = canonicalPath;
        }

        // Get last write time
        if (time = std::filesystem::last_write_time(filePath, ec); ec) {
            m_size = static_cast<int64_t>(FileError::TimeFailed);
            return;
        }

        m_modificationTime = secondsSinceEpochFromFileTime(time);

        // Get hard link count
        if (m_hardLinkCount = std::filesystem::hard_link_count(filePath, ec); ec) {
            m_size = static_cast<int64_t>(FileError::HardLinkFailed);
            return;
        }

        // Get file size (only for regular files, not directories)
        if (isRegularFile()) {
            if (m_size = std::filesystem::file_size(filePath, ec); ec) {
                m_size = static_cast<int64_t>(FileError::SizeFailed);
                return;
            }
        }
    }

    inline FileInfo::FileInfo(const std::filesystem::directory_iterator& it) noexcept
        : m_path{ it->path() },
        m_type{ it->symlink_status().type() },
        m_permissions{ it->symlink_status().permissions() } {

        try {
            // Only try to get file size if it's a regular file
            if (isRegularFile()) {
                m_size = static_cast<int64_t>(it->file_size());
            }

            m_modificationTime = secondsSinceEpochFromFileTime(it->last_write_time());
            m_hardLinkCount = static_cast<int64_t>(it->hard_link_count());
        }
        catch (const std::filesystem::filesystem_error&) {
            // Handle any exception from the iterator accessors
            m_size = static_cast<int64_t>(FileError::DirectoryEntryFailed);
        }
    }

    inline FileInfo::FileInfo(const std::filesystem::directory_iterator& it, std::error_code& ec) noexcept
        : m_path{ it->path() },
        m_type{ it->symlink_status().type() },
        m_permissions{ it->symlink_status().permissions() } {

        // For regular files, get the size
        if (isRegularFile()) {
            m_size = ec ? static_cast<int64_t>(FileError::DirectoryEntryFailed) : static_cast<int64_t>(it->file_size());
        }

        // These operations should be safe even with ec
        try {
            m_modificationTime = secondsSinceEpochFromFileTime(it->last_write_time());
            m_hardLinkCount = static_cast<int64_t>(it->hard_link_count());
        }
        catch (const std::filesystem::filesystem_error&) {
            // Set an error if these operations fail
            if (!ec) {
                m_size = static_cast<int64_t>(FileError::DirectoryEntryFailed);
            }
        }
    }

    inline int64_t FileInfo::secondsSinceEpochFromFileTime(std::filesystem::file_time_type fileDateTime) noexcept {
        try {
            auto sysDateTime = std::chrono::clock_cast<std::chrono::system_clock>(fileDateTime);
            auto sysSeconds = std::chrono::time_point_cast<std::chrono::seconds>(sysDateTime);
            return sysSeconds.time_since_epoch().count();
        }
        catch (const std::exception&) {
            // Handle any conversion exceptions
            return 0;
        }
    }

    inline std::string FileInfo::formatTime() const noexcept {
        try {
            auto tp = std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>{
                std::chrono::seconds{m_modificationTime} };
            return std::format("{:%Y-%m-%d %H:%M:%S}", tp);
        }
        catch (const std::exception&) {
            return "Invalid time";
        }
    }

    inline std::wstring FileInfo::formatTimeWide() const noexcept {
        try {
            auto tp = std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>{
                std::chrono::seconds{m_modificationTime} };
            return std::format(L"{:%Y-%m-%d %H:%M:%S}", tp);
        }
        catch (const std::exception&) {
            return L"Invalid time";
        }
    }

    template<typename CharT>
    inline std::basic_string<CharT> FileInfo::formatSizeWithUnit(int64_t size, int64_t threshold,
        int64_t divider, const CharT* unitSuffix,
        int precision) noexcept {
        if (size < threshold) {
            return std::basic_string<CharT>{};
        }

        if (precision == 0) {
            if constexpr (std::is_same_v<CharT, char>) {
                return std::format("{:d} {}", (size / divider), unitSuffix);
            }
            else {
                return std::format(L"{:d} {}", (size / divider), unitSuffix);
            }
        }
        else {
            int64_t whole = size / divider;
            int64_t fraction = ((size - (whole * divider)) * std::pow(10, precision)) / divider;

            if constexpr (std::is_same_v<CharT, char>) {
                return std::format("{:d}.{:d} {}", whole, fraction, unitSuffix);
            }
            else {
                return std::format(L"{:d}.{:d} {}", whole, fraction, unitSuffix);
            }
        }
    }

    inline std::string FileInfo::formatSize(bool exact) const noexcept {
        if (m_size < 0) {
            return std::format("ERROR {}", m_size);
        }

        if (exact) {
            return std::format("{} B", m_size);
        }

        if (m_size < FileSizeConstants::KB) {
            return std::format("{:d} B", m_size);
        }

        if (auto result = formatSizeWithUnit<char>(m_size, FileSizeConstants::KB, FileSizeConstants::KB, "KB"); !result.empty()) {
            if (m_size < FileSizeConstants::MB) return result;
        }

        if (auto result = formatSizeWithUnit<char>(m_size, FileSizeConstants::MB, FileSizeConstants::MB, "MB", 1); !result.empty()) {
            if (m_size < FileSizeConstants::GB) return result;
        }

        if (auto result = formatSizeWithUnit<char>(m_size, FileSizeConstants::GB, FileSizeConstants::GB, "GB", 2); !result.empty()) {
            if (m_size < FileSizeConstants::TB) return result;
        }

        return formatSizeWithUnit<char>(m_size, FileSizeConstants::TB, FileSizeConstants::TB, "TB", 2);
    }

    inline std::wstring FileInfo::formatSizeWide(bool exact) const noexcept {
        if (m_size < 0) {
            return std::format(L"ERROR {}", m_size);
        }

        if (exact) {
            return std::format(L"{} B", m_size);
        }

        if (m_size < FileSizeConstants::KB) {
            return std::format(L"{:d} B", m_size);
        }

        if (auto result = formatSizeWithUnit<wchar_t>(m_size, FileSizeConstants::KB, FileSizeConstants::KB, L"KB"); !result.empty()) {
            if (m_size < FileSizeConstants::MB) return result;
        }

        if (auto result = formatSizeWithUnit<wchar_t>(m_size, FileSizeConstants::MB, FileSizeConstants::MB, L"MB", 1); !result.empty()) {
            if (m_size < FileSizeConstants::GB) return result;
        }

        if (auto result = formatSizeWithUnit<wchar_t>(m_size, FileSizeConstants::GB, FileSizeConstants::GB, L"GB", 2); !result.empty()) {
            if (m_size < FileSizeConstants::TB) return result;
        }

        return formatSizeWithUnit<wchar_t>(m_size, FileSizeConstants::TB, FileSizeConstants::TB, L"TB", 2);
    }

    constexpr std::string_view FileInfo::getTypeAsString() const noexcept {
        using enum std::filesystem::file_type;
        switch (m_type) {
        case none:      return "none";
        case not_found: return "not found";
        case regular:   return "regular";
        case directory: return "directory";
        case symlink:   return "symlink";
        case block:     return "block";
        case character: return "character";
        case fifo:      return "fifo";
        case socket:    return "socket";
        case unknown:   return "unknown";
        default:        return "invalid";
        }
    }

    constexpr std::wstring_view FileInfo::getTypeAsWideString() const noexcept {
        using enum std::filesystem::file_type;
        switch (m_type) {
        case none:      return L"none";
        case not_found: return L"not found";
        case regular:   return L"regular";
        case directory: return L"directory";
        case symlink:   return L"symlink";
        case block:     return L"block";
        case character: return L"character";
        case fifo:      return L"fifo";
        case socket:    return L"socket";
        case unknown:   return L"unknown";
        default:        return L"invalid";
        }
    }

    inline std::string FileInfo::formatSizeAndStatus() const noexcept {
        using enum std::filesystem::file_type;

        std::string sizeStr = formatSize();

        switch (m_type) {
        case regular:   return std::format("{} {:>10}", (m_hardLinkCount > 1 ? 'h' : ' '), sizeStr, m_path.string());
        case symlink:   return std::format("{}s{:>10}", (m_hardLinkCount > 1 ? 'h' : ' '), sizeStr, m_path.string());
        case directory: return std::format("  directory ", m_path.string());
        case none:      return std::format("  none      ", m_path.string());
        case not_found: return std::format("  not found ", m_path.string());
        case block:     return std::format("  block     ", m_path.string());
        case character: return std::format("  character ", m_path.string());
        case fifo:      return std::format("  fifo      ", m_path.string());
        case socket:    return std::format("  socket    ", m_path.string());
        case unknown:   return std::format("  unknown   ", m_path.string());
        default:        return std::format("  invalid   ", m_path.string());
        }
    }

    inline std::wstring FileInfo::formatSizeAndStatusWide() const noexcept {
        using enum std::filesystem::file_type;

        std::wstring sizeStr = formatSizeWide();

        switch (m_type) {
        case regular:   return std::format(L"{} {:>10}", (m_hardLinkCount > 1 ? 'h' : ' '), sizeStr, m_path.wstring());
        case symlink:   return std::format(L"{}s{:>10}", (m_hardLinkCount > 1 ? 'h' : ' '), sizeStr, m_path.wstring());
        case directory: return std::format(L"  directory ", m_path.wstring());
        case none:      return std::format(L"  none      ", m_path.wstring());
        case not_found: return std::format(L"  not found ", m_path.wstring());
        case block:     return std::format(L"  block     ", m_path.wstring());
        case character: return std::format(L"  character ", m_path.wstring());
        case fifo:      return std::format(L"  fifo      ", m_path.wstring());
        case socket:    return std::format(L"  socket    ", m_path.wstring());
        case unknown:   return std::format(L"  unknown   ", m_path.wstring());
        default:        return std::format(L"  invalid   ", m_path.wstring());
        }
    }

    inline std::string FileInfo::formatList(size_t tabSize) const noexcept {
        tabSize = tabSize > 14 ? tabSize : 14;
        std::string statusStr = formatSizeAndStatus();
        return std::format("{:<{}}{}", statusStr, tabSize, m_path.string());
    }

    inline std::wstring FileInfo::formatListWide(size_t tabSize) const noexcept {
        tabSize = tabSize > 14 ? tabSize : 14;
        std::wstring statusStr = formatSizeAndStatusWide();
        return std::format(L"{:<{}}{}", statusStr, tabSize, m_path.wstring());
    }

    inline std::string FileInfo::formatListWithName(size_t tabSize) const noexcept {
        tabSize = tabSize > 14 ? tabSize : 14;
        std::string statusStr = formatSizeAndStatus();
        return std::format("{:<{}}{}", statusStr, tabSize, m_path.filename().string());
    }

    inline std::wstring FileInfo::formatListWithNameWide(size_t tabSize) const noexcept {
        tabSize = tabSize > 14 ? tabSize : 14;
        std::wstring statusStr = formatSizeAndStatusWide();
        return std::format(L"{:<{}}{}", statusStr, tabSize, m_path.filename().wstring());
    }

    inline std::string FileInfo::getErrorDescription() const noexcept {
        if (m_size >= 0) {
            return {};
        }

        switch (static_cast<FileError>(m_size)) {
        case FileError::StatusFailed:       return "Failed to get file status";
        case FileError::CanonicalFailed:    return "Failed to get canonical path";
        case FileError::TimeFailed:         return "Failed to get file time";
        case FileError::HardLinkFailed:     return "Failed to get hard link count";
        case FileError::SizeFailed:         return "Failed to get file size";
        case FileError::DirectoryEntryFailed: return "Failed to read directory entry";
        default:                            return "Unknown error";
        }
    }

    inline FolderInfo& FolderInfo::assignFolder(const std::filesystem::path& folderPath) noexcept {
        m_path = folderPath;
        m_itemCount = 0;
        m_fileCount = 0;
        m_items.clear();

        // Estimate item count for vector reservation
        try {
            auto dirSize = std::distance(std::filesystem::directory_iterator(folderPath),
                std::filesystem::directory_iterator());
            m_items.reserve(static_cast<size_t>(dirSize));
        }
        catch (...) {
            // Skip reservation on error
        }

        // Iterate through directory
        std::error_code ec;
        for (auto it = std::filesystem::directory_iterator{ folderPath, ec };
            it != std::filesystem::directory_iterator(); ++it) {
            ++m_itemCount;

            if (ec) {
                ec = {}; // Reset error code and continue
                continue;
            }

            // Create FileInfo from iterator
            FileInfo fileInfo(it, ec);

            if (!ec && (fileInfo.m_size > 0 || fileInfo.isDirectory())) {
                if (fileInfo.isRegularFile()) {
                    ++m_fileCount;
                }
                m_items.emplace_back(std::move(fileInfo));
            }
        }

        return *this;
    }

    inline FolderInfo& FolderInfo::sortByName(bool ascending) noexcept {
        try {
            if (ascending) {
                std::sort(m_items.begin(), m_items.end(), [](const FileInfo& a, const FileInfo& b) {
                    return a.m_path.filename() < b.m_path.filename();
                    });
            }
            else {
                std::sort(m_items.begin(), m_items.end(), [](const FileInfo& a, const FileInfo& b) {
                    return a.m_path.filename() > b.m_path.filename();
                    });
            }
        }
        catch (...) {
            // Ignore sort errors
        }
        return *this;
    }

    inline FolderInfo& FolderInfo::sortBySize(bool ascending) noexcept {
        try {
            if (ascending) {
                std::sort(m_items.begin(), m_items.end(), [](const FileInfo& a, const FileInfo& b) {
                    return a.m_size < b.m_size;
                    });
            }
            else {
                std::sort(m_items.begin(), m_items.end(), [](const FileInfo& a, const FileInfo& b) {
                    return a.m_size > b.m_size;
                    });
            }
        }
        catch (...) {
            // Ignore sort errors
        }
        return *this;
    }

    inline FolderInfo& FolderInfo::sortByTime(bool ascending) noexcept {
        try {
            if (ascending) {
                std::sort(m_items.begin(), m_items.end(), [](const FileInfo& a, const FileInfo& b) {
                    return a.m_modificationTime < b.m_modificationTime;
                    });
            }
            else {
                std::sort(m_items.begin(), m_items.end(), [](const FileInfo& a, const FileInfo& b) {
                    return a.m_modificationTime > b.m_modificationTime;
                    });
            }
        }
        catch (...) {
            // Ignore sort errors
        }
        return *this;
    }

    inline FolderInfo& FolderInfo::filterOnlyFiles() noexcept {
        try {
            auto newEnd = std::remove_if(m_items.begin(), m_items.end(), [](const FileInfo& info) {
                return !info.isFile();
                });
            m_items.erase(newEnd, m_items.end());
        }
        catch (...) {
            // Ignore filter errors
        }
        return *this;
    }

    inline FolderInfo& FolderInfo::filterOnlyDirectories() noexcept {
        try {
            auto newEnd = std::remove_if(m_items.begin(), m_items.end(), [](const FileInfo& info) {
                return !info.isDirectory();
                });
            m_items.erase(newEnd, m_items.end());
        }
        catch (...) {
            // Ignore filter errors
        }
        return *this;
    }

    inline int64_t FolderInfo::calculateTotalSize() const noexcept {
        int64_t total = 0;
        for (const auto& item : m_items) {
            if (item.m_size > 0) {
                total += item.m_size;
            }
        }
        return total;
    }

} // namespace mz

#endif // MZ_FILE_INFO_HEADER_FILE
