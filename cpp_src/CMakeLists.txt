cmake_minimum_required(VERSION 3.10)
project(HelloWorldProject VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include header files
# (No specific command needed for header-only libraries until they are part of a target)

# Add executables
add_executable(HelloWorldExecutable main.cpp) # Example: add main.cpp

# Add libraries
# Core data structures (header-only for now, no cpp files to compile yet for Streamline/Tractogram)
# add_library(tractogram_core_lib streamline.cpp tractogram.cpp)

# Ensure C++17 is used for std::filesystem
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# TRX I/O, Streamline Operations, Conversion, and TempDir library
add_library(trx_lib trx_io.cpp streamline_ops.cpp conversion.cpp temp_dir_manager.cpp)
target_include_directories(trx_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} nlohmann)

# Example of linking the TRX I/O library to the HelloWorldExecutable (can be removed if not needed)
target_link_libraries(HelloWorldExecutable PRIVATE trx_lib)

# If tractogram_core_lib had .cpp files and was a separate library:
# target_link_libraries(trx_lib PRIVATE tractogram_core_lib)
# target_link_libraries(HelloWorldExecutable PRIVATE tractogram_core_lib) # Also link core to main if needed directly

# --- Testing Framework (Google Test) ---
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG main # Or a specific release tag like release-1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# --- Command-Line Tools ---
# Tool: trx_converter
add_executable(trx_converter tools/trx_converter.cpp)
target_link_libraries(trx_converter PRIVATE trx_lib)
target_include_directories(trx_converter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}) # For includes like tractogram.h

# Tool: trx_info
add_executable(trx_info tools/trx_info.cpp)
target_link_libraries(trx_info PRIVATE trx_lib)
target_include_directories(trx_info PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}) # For includes like tractogram.h

# --- Unit Tests ---
add_executable(core_tests tests/test_core_data_structures.cpp)
target_link_libraries(core_tests PRIVATE trx_lib GTest::gtest_main)
add_test(NAME CoreDataStructuresTest COMMAND core_tests)

add_executable(trx_io_tests tests/test_trx_io.cpp)
target_link_libraries(trx_io_tests PRIVATE trx_lib GTest::gtest_main nlohmann_json::nlohmann_json) # nlohmann needed for header parsing in test setup
add_test(NAME TrxIoTest COMMAND trx_io_tests)

add_executable(streamline_ops_tests tests/test_streamline_ops.cpp)
target_link_libraries(streamline_ops_tests PRIVATE trx_lib GTest::gtest_main)
add_test(NAME StreamlineOpsTest COMMAND streamline_ops_tests)

add_executable(conversion_tests tests/test_conversion.cpp)
target_link_libraries(conversion_tests PRIVATE trx_lib GTest::gtest_main nlohmann_json::nlohmann_json) # For TempDirManager and potentially TRX I/O helpers if they use json
add_test(NAME ConversionTest COMMAND conversion_tests)
