#include "temp_dir_manager.h"
#include <cstdlib> // For std::getenv
#include <iostream> // For std::cerr, std::cout
#include <sstream>  // For std::ostringstream
#include <iomanip>  // For std::put_time
#include <random>   // For std::random_device, std::mt19937, std::uniform_int_distribution
#include <algorithm> // For std::remove_all (in cleanup)

namespace fs = std::filesystem;

// Helper to generate a somewhat unique name
std::string TempDirManager::generateUniqueSubdirName() {
    // Get current time for timestamp
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm;
#ifdef _WIN32
    localtime_s(&now_tm, &now_c);
#else
    localtime_r(&now_c, &now_tm);
#endif

    std::ostringstream oss_ts;
    oss_ts << std::put_time(&now_tm, "%Y%m%d_%H%M%S");

    // Add some random numbers for more uniqueness (pseudo-UUID like)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 0xFFFF);

    std::ostringstream oss_rand;
    oss_rand << std::hex << std::setfill('0') << std::setw(4) << distrib(gen)
             << std::setw(4) << distrib(gen);

    return "trx_cpp_" + oss_ts.str() + "_" + oss_rand.str();
}

void TempDirManager::initializeBaseTempDir() {
    const char* env_var = std::getenv("TRX_TMPDIR_CPP");
    if (env_var != nullptr) {
        std::string env_val = env_var;
        if (env_val == "use_working_dir") {
            base_temp_dir_ = fs::current_path();
        } else {
            base_temp_dir_ = env_val;
            if (!fs::exists(base_temp_dir_)) {
                throw std::runtime_error("TRX_TMPDIR_CPP path does not exist: " + base_temp_dir_.string());
            }
            if (!fs::is_directory(base_temp_dir_)) {
                throw std::runtime_error("TRX_TMPDIR_CPP path is not a directory: " + base_temp_dir_.string());
            }
        }
    } else {
        base_temp_dir_ = fs::temp_directory_path();
    }
}

TempDirManager::TempDirManager(const std::string& unique_subdir_name, bool auto_cleanup)
    : auto_cleanup_(auto_cleanup) {
    initializeBaseTempDir();
    if (unique_subdir_name.empty()) {
        unique_subdir_name_str_ = generateUniqueSubdirName();
    } else {
        unique_subdir_name_str_ = unique_subdir_name;
    }
    unique_subdir_path_ = base_temp_dir_ / unique_subdir_name_str_;
}

TempDirManager::~TempDirManager() {
    if (auto_cleanup_ && path_created_) {
        cleanup();
    }
}

TempDirManager::TempDirManager(TempDirManager&& other) noexcept
    : base_temp_dir_(std::move(other.base_temp_dir_)),
      unique_subdir_path_(std::move(other.unique_subdir_path_)),
      unique_subdir_name_str_(std::move(other.unique_subdir_name_str_)),
      auto_cleanup_(other.auto_cleanup_),
      path_created_(other.path_created_) {
    // Ensure the moved-from object doesn't attempt cleanup
    other.path_created_ = false;
    other.auto_cleanup_ = false;
}

TempDirManager& TempDirManager::operator=(TempDirManager&& other) noexcept {
    if (this != &other) {
        // Cleanup own resources if they were created and auto_cleanup is on
        if (auto_cleanup_ && path_created_) {
            cleanup();
        }

        base_temp_dir_ = std::move(other.base_temp_dir_);
        unique_subdir_path_ = std::move(other.unique_subdir_path_);
        unique_subdir_name_str_ = std::move(other.unique_subdir_name_str_);
        auto_cleanup_ = other.auto_cleanup_;
        path_created_ = other.path_created_;

        // Ensure the moved-from object doesn't attempt cleanup
        other.path_created_ = false;
        other.auto_cleanup_ = false;
    }
    return *this;
}


fs::path TempDirManager::getTempPath() {
    if (!path_created_) {
        try {
            if (fs::exists(unique_subdir_path_)) {
                // If it exists and is not a directory, this is an issue.
                if (!fs::is_directory(unique_subdir_path_)) {
                     throw std::runtime_error("Temporary path exists but is not a directory: " + unique_subdir_path_.string());
                }
                // If it exists and is a directory, we can use it.
                // Optionally, one might want to clear it or ensure it's empty.
                // For now, we'll just use it as is if it already exists.
                std::cout << "Warning: Temporary subdirectory already exists: " << unique_subdir_path_.string() << std::endl;
            } else {
                if (!fs::create_directories(unique_subdir_path_)) {
                    // The create_directories function returns false if it fails and the path does not exist post-call.
                    // It might throw an exception for other reasons (e.g. permissions), which we'd catch below.
                    if(!fs::exists(unique_subdir_path_)) { // Check again, as create_directories might fail but dir exists due to race
                         throw std::runtime_error("Failed to create temporary subdirectory: " + unique_subdir_path_.string());
                    }
                }
            }
            path_created_ = true;
        } catch (const fs::filesystem_error& e) {
            throw std::runtime_error("Filesystem error while creating temporary subdirectory '" +
                                     unique_subdir_path_.string() + "': " + e.what() +
                                     " (Code: " + std::to_string(e.code().value()) + ")");
        } catch (const std::exception& e) { // Catch other potential exceptions
             throw std::runtime_error("Error creating temporary subdirectory '" +
                                     unique_subdir_path_.string() + "': " + e.what());
        }
    }
    return unique_subdir_path_;
}

fs::path TempDirManager::getBaseTempDir() const {
    return base_temp_dir_;
}

fs::path TempDirManager::getUniqueSubdirName() const {
    return unique_subdir_name_str_;
}

void TempDirManager::cleanup() {
    if (path_created_ && fs::exists(unique_subdir_path_)) {
        try {
            std::uintmax_t n = fs::remove_all(unique_subdir_path_);
            std::cout << "Cleaned up temporary directory: " << unique_subdir_path_.string()
                      << ", removed " << n << " files/directories." << std::endl;
            path_created_ = false; // Mark as cleaned up
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error cleaning up temporary directory '" << unique_subdir_path_.string()
                      << "': " << e.what() << " (Code: " << std::to_string(e.code().value()) << ")" << std::endl;
            // Depending on policy, might re-throw or just log.
        } catch (const std::exception& e) {
             std::cerr << "Error cleaning up temporary directory '" << unique_subdir_path_.string()
                      << "': " << e.what() << std::endl;
        }
    }
}

void TempDirManager::disableAutoCleanup() {
    auto_cleanup_ = false;
}

void TempDirManager::enableAutoCleanup() {
    auto_cleanup_ = true;
}
