# IO Utils Library
MIT License | C++20 | Cross-Platform
## Overview
IO Utils is a comprehensive C++ library that provides robust, cross-platform utilities for input/output operations, file handling, logging, randomization, and data encoding. It offers a set of high-performance, type-safe abstractions that simplify common I/O tasks while maintaining exceptional performance and reliability.
## Key Features
•	Cross-platform file operations - Consistent API across Windows, macOS, and Linux
•	High-performance logging - Thread-safe, configurable logging with multiple output targets
•	Secure randomization - Cryptographically secure random number generation
•	Multi-file I/O - Efficient handling of operations across multiple files
•	Base64 encoding/decoding - Fast implementation with both standard and URL-safe variants
•	Time conversion utilities - Comprehensive date and time handling with timezone support
•	Keyboard input handling - Cross-platform keyboard input with support for special keys
•	Bit manipulation - Efficient bit-level operations with easy-to-use interfaces
•	Type-safe serialization - Consistent binary data serialization and deserialization
## Requirements
•	C++20 compatible compiler (GCC 10+, Clang 10+, MSVC 19.29+)
•	CMake 3.16+ (for building)
•	No external dependencies
## Installation
### Using CMake
```
git clone https://github.com/meysam-zm/io_utils.git
cd io_utils
mkdir build && cd build
cmake ..
cmake --build .
cmake --install .
```
### As a Subproject
Add the repository as a git submodule:
```
git submodule add https://github.com/meysam-zm/io_utils.git extern/io_utils
```
Then in your CMakeLists.txt:
```
add_subdirectory(extern/io_utils)
target_link_libraries(your_target_name PRIVATE mz::io_utils)
```
## Components
### FileIO
Cross-platform file operations with consistent error handling:
```
#include <mz/io/FileIO.h>

// Read a file into memory
auto result = mz::io::readFile("config.json");
if (result) {
    std::vector<uint8_t>& data = result.value();
    // Process data...
}

// Write data to a file
std::string content = "Hello, World!";
bool success = mz::io::writeFile("output.txt", content);
```
### MultiFileIO
Handle operations across multiple files efficiently:
```
#include <mz/io/MultiFileIO.h>

mz::io::MultiFileReader reader;
reader.openFiles({"data1.bin", "data2.bin", "data3.bin"});

// Read sequentially through all files
std::vector<uint8_t> buffer(1024);
while (reader.read(buffer)) {
    // Process buffer contents...
}
```
### Logger
Thread-safe, configurable logging system:
```
#include <mz/io/Logger.h>

// Configure logger
mz::io::Logger::Config config;
config.consoleOutput = true;
config.fileOutput = true;
config.filename = "application.log";
config.level = mz::io::LogLevel::Info;
mz::io::Logger::initialize(config);

// Log messages at different levels
Logger::trace("Detailed tracing information");
Logger::debug("Debugging information");
Logger::info("General information");
Logger::warning("Warning message");
Logger::error("Error message");
Logger::critical("Critical error message");
```
### Randomizer
Cryptographically secure random number generation:
```
#include <mz/io/Randomizer.h>

// Generate random integers
int randomInt = mz::io::Randomizer::getInt(1, 100);

// Generate random bytes
std::vector<uint8_t> randomBytes(32);
mz::io::Randomizer::getBytes(randomBytes);

// Generate random floating-point values
double randomDouble = mz::io::Randomizer::getDouble(0.0, 1.0);
```
### Encode64
Base64 encoding and decoding:
```
#include <mz/io/Encode64.h>

// Standard Base64 encoding
std::string encoded = mz::io::Encoder64::toString(binaryData);

// URL-safe Base64 encoding
std::string urlSafe = mz::io::Encoder64::toUrlSafeString(binaryData);

// Decoding
std::vector<uint8_t> decoded = mz::io::Decoder64::fromString(encoded);
```
### TimeConversions
Comprehensive time handling utilities:
```
#include <mz/io/TimeConversions.h>

// Get current time
auto now = mz::time::getCurrentSeconds();

// Format time
std::string formatted = mz::time::formatDateTime(now);

// Parse time
auto timePoint = mz::time::parseDateTime("2025-10-15 14:30:00");
```
### FileInfo
Detailed file information with cross-platform support:
```
#include <mz/io/FileInfo.h>

mz::FileInfo info("document.txt");
if (info.exists()) {
    std::cout << "Size: " << info.formatSize() << std::endl;
    std::cout << "Modified: " << info.formatTime() << std::endl;
    std::cout << "Type: " << info.getTypeAsString() << std::endl;
}

mz::FolderInfo folder("documents");
folder.sortByName().filterOnlyFiles();
for (const auto& item : folder.m_items) {
    std::cout << item.formatListWithName() << std::endl;
}
```
### BitMasks
Efficient bit-level operations:
```
#include <mz/io/BitMasks.h>

// Create bit masks
auto mask = mz::BitMask<uint32_t>(0b11110000);

// Check bit states
bool isSet = mask.test(7);

// Modify bits
mask.set(1);
mask.clear(2);
mask.toggle(3);
```
## Performance Considerations
•	Zero-copy operations where possible
•	Optimized buffer management for minimal allocations
•	Thread-safe implementations without excessive locking
•	Cache-friendly data structures and algorithms
•	Platform-specific optimizations where beneficial
•	Compile-time optimizations for modern C++ compilers
Error Handling
IO Utils uses a consistent error handling approach:
•	Return values are wrapped in std::optional or std::expected for operations that might fail
•	Detailed error codes and messages for precise error identification
•	No exceptions by default (configurable with MZ_IO_USE_EXCEPTIONS)
•	Thread-safe logging of errors for diagnostics
## Platform Support
•	Windows (10 and later)
•	macOS (10.15 and later)
•	Linux (modern distributions with glibc 2.17+)
•	BSD variants
•	Any POSIX-compliant system
## License
This library is distributed under the MIT License. See the LICENSE file for details.
## Contributing
Contributions are welcome! Please feel free to submit a Pull Request.
1.	Fork the repository
2.	Create your feature branch (git checkout -b feature/amazing-feature)
3.	Commit your changes (git commit -m 'Add some amazing feature')
4.	Push to the branch (git push origin feature/amazing-feature)
5.	Open a Pull Request
## Credits
Developed by Meysam Zare (c) 2021-2025






