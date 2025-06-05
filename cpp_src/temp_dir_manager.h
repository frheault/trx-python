#ifndef TEMP_DIR_MANAGER_H
#define TEMP_DIR_MANAGER_H

#include <string>
#include <filesystem> // C++17 filesystem library
#include <chrono>     // For timestamp
#include <random>     // For UUID-like random numbers
#include <stdexcept>  // For runtime_error

// Forward declare namespace for filesystem to avoid error with some compilers.
// namespace std { namespace filesystem {} } // This line might be needed for some older C++17 compilers/setups.
// For modern C++17 compilers, simply using 'fs' alias after include should be fine.
namespace fs = std::filesystem;

/**
 * @brief Manages the creation and cleanup of temporary directories.
 * This class helps in creating a unique temporary subdirectory for operations
 * that need temporary storage, and ensures it's cleaned up afterwards.
 */
class TempDirManager {
public:
    /**
     * @brief Constructs a TempDirManager.
     * Determines the base temporary directory based on the `TRX_TMPDIR_CPP` environment variable:
     * - If `TRX_TMPDIR_CPP="use_working_dir"`, the current working directory is used as the base.
     * - If `TRX_TMPDIR_CPP="/path/to/a/directory"`, the specified path is used as the base.
     *   The path must exist and be a directory.
     * - If the environment variable is not set or is empty, the system's default temporary
     *   directory (e.g., `/tmp` or result of `GetTempPath`) is used.
     *
     * A unique subdirectory (e.g., `trx_cpp_<timestamp>_<random>`) will be managed within this base directory.
     *
     * @param unique_subdir_name Optional name for the unique subdirectory. If empty, a name
     *                           will be automatically generated to ensure uniqueness.
     * @param auto_cleanup If true (default), the unique subdirectory and its contents will be
     *                     removed when the TempDirManager object is destructed.
     * @throws std::runtime_error if the path specified by `TRX_TMPDIR_CPP` (if set and not "use_working_dir")
     *                            does not exist, is not a directory, or if there are permission issues.
     */
    explicit TempDirManager(const std::string& unique_subdir_name = "", bool auto_cleanup = true);

    /**
     * @brief Destructor.
     * If auto-cleanup is enabled and a temporary path was created, this destructor
     * will attempt to remove the unique temporary subdirectory and all its contents.
     * Errors during cleanup are typically logged to `std::cerr` but do not throw exceptions.
     */
    ~TempDirManager();

    // Disable copy constructor and copy assignment to prevent issues with resource management.
    TempDirManager(const TempDirManager&) = delete;
    TempDirManager& operator=(const TempDirManager&) = delete;

    // Enable move constructor and move assignment for efficient transfer of ownership.
    TempDirManager(TempDirManager&& other) noexcept;
    TempDirManager& operator=(TempDirManager&& other) noexcept;

    /**
     * @brief Gets the full path to the unique temporary subdirectory.
     * If the subdirectory does not already exist, this function will attempt to create it.
     * @return A `std::filesystem::path` object representing the path to the unique temporary subdirectory.
     * @throws std::runtime_error if the subdirectory cannot be created (e.g., due to permissions or
     *                            if a file with the same name already exists at that path).
     */
    fs::path getTempPath();

    /**
     * @brief Returns the base directory that was chosen for temporary storage.
     * This could be the system's temporary directory, the current working directory,
     * or a path specified by the `TRX_TMPDIR_CPP` environment variable.
     * @return A `std::filesystem::path` to the base temporary directory.
     */
    fs::path getBaseTempDir() const;

    /**
     * @brief Returns the name of the unique subdirectory that is or will be created
     * within the base temporary directory.
     * @return A `std::filesystem::path` object containing just the name of the unique subdirectory.
     */
    fs::path getUniqueSubdirName() const;


    /**
     * @brief Manually triggers the cleanup of the unique temporary subdirectory and its contents.
     * This is useful if `auto_cleanup` was set to false during construction or disabled later.
     * If the path was not created or has already been cleaned up, this function does nothing.
     * Errors during cleanup are logged to `std::cerr`.
     */
    void cleanup();

    /**
     * @brief Disables automatic cleanup of the temporary subdirectory in the destructor.
     * This can be useful for debugging purposes, to inspect the contents of the temporary directory
     * after the program or relevant scope has finished.
     */
    void disableAutoCleanup();

    /**
     * @brief Enables automatic cleanup of the temporary subdirectory in the destructor.
     * This is the default behavior.
     */
    void enableAutoCleanup();

private:
    /// @brief The base directory where the unique temporary subdirectory will be created.
    fs::path base_temp_dir_;
    /// @brief The full path to the unique temporary subdirectory.
    fs::path unique_subdir_path_;
    /// @brief The string name of the unique subdirectory.
    std::string unique_subdir_name_str_;
    /// @brief Flag indicating whether to automatically clean up the directory on destruction.
    bool auto_cleanup_;
    /// @brief Flag to track if getTempPath() has been called and successfully created/verified the directory.
    bool path_created_ = false;

    /**
     * @brief Initializes the `base_temp_dir_` based on environment variables or system defaults.
     * @throws std::runtime_error if an environment variable specifies an invalid path.
     */
    void initializeBaseTempDir();

    /**
     * @brief Generates a unique name for the temporary subdirectory.
     * Typically uses a combination of "trx_cpp_", a timestamp, and random numbers.
     * @return A string representing the unique subdirectory name.
     */
    std::string generateUniqueSubdirName();
};

#endif // TEMP_DIR_MANAGER_H
