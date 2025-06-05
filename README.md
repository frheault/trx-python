# trx-python

This is a Python implementation of the trx file-format for tractography data.

For details, please visit the documentation web-page at https://tee-ar-ex.github.io/trx-python/.

To install this, you can run:

    pip install trx-python

Or, to install from source:

    git clone https://github.com/tee-ar-ex/trx-python.git
    cd trx-python
    pip install .

### Temporary Directory
The TRX file format uses memmaps to limit RAM usage. When dealing with large files this means several gigabytes could be required on disk (instead of RAM). 

By default, the temporary directory on Linux and MacOS is `/tmp` and on Windows it should be `C:\WINDOWS\Temp`.

If you wish to change the directory add the following variable to your script or to your .bashrc or .bash_profile:
`export TRX_TMPDIR=/WHERE/I/WANT/MY/TMP/DATA` (a)
OR
`export TRX_TMPDIR=use_working_dir` (b)

The provided folder must already exists (a). `use_working_dir` will be the directory where the code is being executed from (b).

The temporary folders should be automatically cleaned. But, if the code crash unexpectedly, make sure the folders are deleted.

---

## C++ Version

This repository also includes a C++ implementation of core TRX data structures and utilities.

### Dependencies

*   **CMake:** Version 3.10 or higher.
*   **C++ Compiler:** A compiler supporting C++17 (for `std::filesystem` and other features).
    *   GCC 7+
    *   Clang 5+
    *   MSVC 2017 (v19.10)+ (ensure the C++17 standard is enabled, e.g., `/std:c++17`)
*   **Google Test:** For running unit tests. Fetched automatically by CMake using `FetchContent`.
*   **nlohmann/json:** For JSON parsing (e.g., `header.json` in TRX). Included as a header-only library directly in `cpp_src/nlohmann/json.hpp`.

### Building the C++ Code

1.  **Clone the repository (if you haven't already):**
    ```bash
    git clone https://github.com/tee-ar-ex/trx-python.git # Or your fork
    cd trx-python/cpp_src
    ```

2.  **Create a build directory and navigate into it:**
    ```bash
    mkdir build
    cd build
    ```

3.  **Run CMake to configure the project:**
    ```bash
    cmake ..
    ```
    *   On Windows, you might need to specify a generator, e.g., `cmake .. -G "Visual Studio 16 2019" -A x64`.

4.  **Build the project:**
    ```bash
    cmake --build . --config Release
    ```
    *   This will build the `trx_lib` static library and the command-line tools (`trx_converter`, `trx_info`) and test executables. Binaries will be located in the `build/tools` and `build/tests` directories respectively (or subdirectories depending on your build system).

### Running Unit Tests

Unit tests are implemented using Google Test.

1.  **Build the project** as described above. This will also build the test executables.
2.  **Run tests using CTest:**
    From the `build` directory:
    ```bash
    ctest
    ```
    Or, to run with more verbose output:
    ```bash
    ctest -V
    ```
3.  **Alternatively, run individual test executables directly:**
    Test binaries are typically found in the `build/tests` directory (or similar, depending on your generator). For example:
    ```bash
    ./tests/core_tests
    ./tests/trx_io_tests
    # etc.
    ```

### Using the C++ Command-Line Tools

The C++ implementation includes the following command-line tools:

*   **`trx_converter`**: Converts tractogram files between TRX, TCK, and TRK formats.
    *   **Usage:**
        ```bash
        ./tools/trx_converter <input_file> <output_file> [--reference <ref_path>] [--in_format <TCK|TRK|TRX>] [--out_format <TCK|TRK|TRX>]
        ```
    *   **Examples:**
        *   Convert a TCK file to TRX format:
            ```bash
            ./tools/trx_converter input.tck output_directory.trx
            ```
        *   Convert a TRX folder to TCK format:
            ```bash
            ./tools/trx_converter input_directory.trx output.tck
            ```
        *   Specify input format if extension is ambiguous:
            ```bash
            ./tools/trx_converter input.dat output.tck --in_format TCK
            ```

*   **`trx_info`**: Prints basic information about a TRX file/folder.
    *   **Usage:**
        ```bash
        ./tools/trx_info <input_trx_folder_path>
        ```
    *   **Example:**
        ```bash
        ./tools/trx_info my_tractogram.trx
        ```
        This will output header information like streamline/vertex counts, dimensions, and (eventually) details about data-per-vertex/streamline and groups.

### TRX File Format (C++ Perspective)

The C++ library aims to support reading and writing the TRX folder-based format as specified in the original documentation.

*   **Current C++ Implementation Status & Limitations:**
    *   **Core Data Structures:** `Streamline` and `Tractogram` classes are implemented for storing tractography data in memory.
    *   **TRX I/O (`trx_io`):**
        *   Reading: Supports `header.json`, `positions` (float32, float64), and `offsets` (uint32, uint64). Basic parsing of `dpv`/`dps`/`groups` directory structures is present, but full data loading and association with streamlines is not yet complete.
        *   Writing: Writes `header.json`, `positions.3.float32.bin`, and `offsets.uint32.bin`. Writing `dpv`/`dps`/`groups` data is not yet implemented.
    *   **Streamline Operations (`streamline_ops`):** Implements streamline hashing (based on first/last 5 points with precision control) and set operations (intersection, union, difference) on hashed streamlines.
    *   **Conversions (`conversion`):**
        *   TCK: Basic reading and writing for Float32 TCK files are implemented. Endianness is assumed to match the system.
        *   TRK: Reading (`trkToTrx`) is a placeholder with basic header parsing. Writing TRK from TRX is not yet implemented.
    *   **Temporary Directory Management (`TempDirManager`):** Provides robust management of temporary directories, configurable via the `TRX_TMPDIR_CPP` environment variable.
    *   **Data Types:** Float16 support for positions is not yet implemented in I/O.
    *   **Memory Mapping:** Placeholders for memory mapping exist in `trx_io.cpp` but are not yet implemented.

### Temporary Directory (C++ Version)
Similar to the Python version, the C++ TRX library components might use temporary storage for certain operations (e.g., if decompressing files or for intermediate steps in conversions, though not heavily used in the current I/O).

The temporary directory can be controlled via the `TRX_TMPDIR_CPP` environment variable:
*   `export TRX_TMPDIR_CPP=/path/to/custom/temp_dir`
*   `export TRX_TMPDIR_CPP=use_working_dir` (uses the directory from which the tool/program is executed)

If `TRX_TMPDIR_CPP` is not set, the system's default temporary directory (e.g., `/tmp` on Linux/macOS) will be used. The `TempDirManager` class handles the creation of unique subdirectories within this base path and their cleanup.
