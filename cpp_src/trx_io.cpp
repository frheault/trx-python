#include "trx_io.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <array>
#include <map>
#include <limits> // Required for std::numeric_limits
#include <algorithm> // Required for std::equal
#include <stdexcept> // Required for std::runtime_error

// For directory operations (creating, listing files)
#include <sys/stat.h> // For mkdir (POSIX)
#include <dirent.h>   // For opendir, readdir, closedir (POSIX)
#include <cerrno>     // For errno

// nlohmann/json include
#include <nlohmann/json.hpp>

// For memory mapping (placeholders)
// #include <sys/mman.h> // For mmap, munmap (POSIX)
// #include <fcntl.h>    // For open flags (POSIX)
// #include <unistd.h>   // For close, ftruncate (POSIX)

namespace trx
{

// --- Helper Functions ---

// Function to create a directory (platform-dependent)
bool createDirectory(const std::string& path) {
#ifdef _WIN32
    return _mkdir(path.c_str()) == 0 || errno == EEXIST;
#else
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
#endif
}

// Function to list files in a directory (platform-dependent)
std::vector<std::string> listFilesInDirectory(const std::string& directoryPath) {
    std::vector<std::string> files;
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(directoryPath.c_str())) != nullptr) {
        while ((ent = readdir(dir)) != nullptr) {
            std::string filename = ent->d_name;
            if (filename != "." && filename != "..") {
                files.push_back(filename);
            }
        }
        closedir(dir);
    } else {
        // Could not open directory - handle error or return empty list
        std::cerr << "Warning: Could not open directory " << directoryPath << std::endl;
    }
    return files;
}

// Function to join paths (simple version)
std::string joinPath(const std::string& base, const std::string& leaf) {
    if (base.empty()) return leaf;
    if (leaf.empty()) return base;
#ifdef _WIN32
    char last_char = base.back();
    if (last_char != '\\' && last_char != '/') {
        return base + "\\" + leaf;
    }
    return base + leaf;
#else
    char last_char = base.back();
    if (last_char != '/') {
        return base + "/" + leaf;
    }
    return base + leaf;
#endif
}

// Helper to parse filename for data type and dimensions
// e.g., "positions.3.float16.bin" -> {type: "float16", dimensions: 3}
// e.g., "data.1.uint32.bin" -> {type: "uint32", dimensions: 1}
struct FileInfo {
    std::string baseName;
    int dimensions = 0;
    std::string dataType;
    std::string extension;
};

FileInfo parseFileName(const std::string& filename) {
    FileInfo info;
    std::vector<std::string> parts;
    std::string current_part;
    for (char c : filename) {
        if (c == '.') {
            parts.push_back(current_part);
            current_part.clear();
        } else {
            current_part += c;
        }
    }
    parts.push_back(current_part); // last part (extension or part of name)

    if (parts.empty()) return info; // Should not happen with valid names

    info.baseName = parts[0];
    if (parts.size() > 1) {
        info.extension = parts.back();
        if (parts.size() > 2) { // positions.3.float32.bin or data_name.float32.bin
             // Try to parse dimensions if present (e.g. positions.3.float32.bin)
            bool isNumeric = true;
            for(char ch : parts[parts.size()-3]) { // Check third to last part for dimensions
                if(!std::isdigit(ch)) {
                    isNumeric = false;
                    break;
                }
            }
            if(isNumeric && parts.size() >= 4) { // name.dim.type.ext
                info.dimensions = std::stoi(parts[parts.size()-3]);
                info.dataType = parts[parts.size()-2];
            } else { // name.type.ext (no explicit dimension, assume 1 for dpv/dps)
                info.dataType = parts[parts.size()-2];
                info.dimensions = 1; // Default for data per point/streamline if not specified
            }

        } else if (parts.size() > 1 && info.extension == "json") {
             // header.json
        }
        // Else, could be just "filename.bin" (e.g. for offsets) or other formats
    }
    return info;
}


// Template function to read binary data from a file
template <typename T>
bool readBinaryFile(const std::string& filePath, std::vector<T>& data, size_t expected_elements = 0) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Error opening file for reading: " << filePath << std::endl;
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size == 0 && expected_elements == 0) { // Empty file is ok if not expecting elements
        data.clear();
        return true;
    }
    if (size == 0 && expected_elements > 0){
        std::cerr << "Error: File is empty but expected " << expected_elements << " elements: " << filePath << std::endl;
        return false;
    }

    size_t num_elements = static_cast<size_t>(size) / sizeof(T);
    if (expected_elements > 0 && num_elements != expected_elements) {
         std::cerr << "Error: Number of elements in file (" << num_elements
                   << ") does not match expected elements (" << expected_elements
                   << ") for file: " << filePath << std::endl;
        return false;
    }
    if (static_cast<size_t>(size) % sizeof(T) != 0) {
        std::cerr << "Error: File size is not a multiple of element size for file: " << filePath << std::endl;
        return false;
    }


    data.resize(num_elements);
    if (!file.read(reinterpret_cast<char*>(data.data()), size)) {
        std::cerr << "Error reading data from file: " << filePath << std::endl;
        return false;
    }
    // TODO: Add comments/placeholders for memory mapping for reading large files
    return true;
}


// Template function to write binary data to a file
template <typename T>
bool writeBinaryFile(const std::string& filePath, const std::vector<T>& data) {
    std::ofstream file(filePath, std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file for writing: " << filePath << std::endl;
        return false;
    }
    if (!data.empty()) {
        file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size() * sizeof(T)));
        if (!file) {
             std::cerr << "Error writing data to file: " << filePath << std::endl;
            return false;
        }
    }
    // TODO: Add comments/placeholders for memory mapping for writing large files
    return true;
}


// --- TRX Reading Logic ---
bool readTRX(const std::string& trxFolderPath, Tractogram& tractogram) {
    tractogram = Tractogram(); // Clear any existing data

    // 1. Read header.json
    std::string headerPath = joinPath(trxFolderPath, "header.json");
    std::ifstream headerFile(headerPath);
    if (!headerFile.is_open()) {
        std::cerr << "Error: Could not open header.json at " << headerPath << std::endl;
        return false;
    }
    nlohmann::json header_json;
    try {
        headerFile >> header_json;
    } catch (nlohmann::json::parse_error& e) {
        std::cerr << "Error parsing header.json: " << e.what() << std::endl;
        return false;
    }
    headerFile.close();

    // Populate Tractogram metadata from header
    if (header_json.contains("VOXEL_TO_RASMM") && header_json["VOXEL_TO_RASMM"].is_array()) {
        // Assuming VOXEL_TO_RASMM is stored as a string in metadata for simplicity, or parse into a matrix type
        tractogram.addMetadata("VOXEL_TO_RASMM", header_json["VOXEL_TO_RASMM"].dump());
    }
    if (header_json.contains("DIMENSIONS") && header_json["DIMENSIONS"].is_array()) {
        tractogram.addMetadata("DIMENSIONS", header_json["DIMENSIONS"].dump());
    }
    long long nb_streamlines_header = 0;
    if (header_json.contains("NB_STREAMLINES") && header_json["NB_STREAMLINES"].is_number()) {
        nb_streamlines_header = header_json["NB_STREAMLINES"].get<long long>();
        tractogram.addMetadata("NB_STREAMLINES", std::to_string(nb_streamlines_header));
    } else {
        std::cerr << "Warning: NB_STREAMLINES not found or not a number in header.json" << std::endl;
    }
    long long nb_vertices_header = 0;
     if (header_json.contains("NB_VERTICES") && header_json["NB_VERTICES"].is_number()) {
        nb_vertices_header = header_json["NB_VERTICES"].get<long long>();
        tractogram.addMetadata("NB_VERTICES", std::to_string(nb_vertices_header));
    } else {
        std::cerr << "Warning: NB_VERTICES not found or not a number in header.json" << std::endl;
    }
    // Store all other header fields as metadata
    for (auto& [key, value] : header_json.items()) {
        if (value.is_string()) {
            tractogram.addMetadata(key, value.get<std::string>());
        } else {
            tractogram.addMetadata(key, value.dump());
        }
    }


    // 2. Read offsets (determine data type from filename if possible, or assume default)
    std::vector<uint32_t> offsets_uint32;
    std::vector<uint64_t> offsets_uint64;
    bool offsets_is_64bit = false;

    // Try to find offsets file (e.g., offsets.uint32.bin or offsets.uint64.bin)
    std::string offsetsPath_u32 = joinPath(trxFolderPath, "offsets.uint32.bin");
    std::string offsetsPath_u64 = joinPath(trxFolderPath, "offsets.uint64.bin");

    std::ifstream offsetsFileTest_u32(offsetsPath_u32);
    std::ifstream offsetsFileTest_u64(offsetsPath_u64);

    if (offsetsFileTest_u32.good()) {
        if (!readBinaryFile(offsetsPath_u32, offsets_uint32, static_cast<size_t>(nb_streamlines_header))) return false;
    } else if (offsetsFileTest_u64.good()) {
        if (!readBinaryFile(offsetsPath_u64, offsets_uint64, static_cast<size_t>(nb_streamlines_header))) return false;
        offsets_is_64bit = true;
    } else {
        std::cerr << "Error: Could not find offsets.uint32.bin or offsets.uint64.bin in " << trxFolderPath << std::endl;
        return false;
    }
    offsetsFileTest_u32.close();
    offsetsFileTest_u64.close();

    size_t num_streamlines = offsets_is_64bit ? offsets_uint64.size() : offsets_uint32.size();
    if (num_streamlines != static_cast<size_t>(nb_streamlines_header)) {
         std::cerr << "Warning: Number of streamlines in offsets file (" << num_streamlines
                   << ") does not match header NB_STREAMLINES (" << nb_streamlines_header << ")." << std::endl;
         // Potentially adjust num_streamlines or return error based on strictness
    }


    // 3. Read positions (determine data type and dimensions from filename)
    std::string positionsFilename;
    FileInfo positionsFileInfo;
    std::vector<std::string> root_files = listFilesInDirectory(trxFolderPath);
    for(const auto& rf : root_files){
        if(rf.rfind("positions.", 0) == 0){
            positionsFilename = rf;
            break;
        }
    }
    if(positionsFilename.empty()){
        std::cerr << "Error: Could not find positions file (e.g., positions.3.float32.bin) in " << trxFolderPath << std::endl;
        return false;
    }
    positionsFileInfo = parseFileName(positionsFilename);
    if (positionsFileInfo.dimensions == 0 || positionsFileInfo.dataType.empty()) {
        std::cerr << "Error: Could not parse positions filename: " << positionsFilename << std::endl;
        return false;
    }
    if (positionsFileInfo.dimensions != 3) {
         std::cerr << "Warning: Positions file dimensions are not 3: " << positionsFilename << std::endl;
    }


    std::vector<float> positions_float32;
    std::vector<double> positions_float64;
    // Add float16 support if a library or manual conversion is available
    // std::vector<some_float16_type> positions_float16;

    std::string fullPositionsPath = joinPath(trxFolderPath, positionsFilename);
    size_t total_vertices_times_dim = static_cast<size_t>(nb_vertices_header) * static_cast<size_t>(positionsFileInfo.dimensions);

    if (positionsFileInfo.dataType == "float32") {
        if (!readBinaryFile(fullPositionsPath, positions_float32, total_vertices_times_dim)) return false;
    } else if (positionsFileInfo.dataType == "float64") {
        if (!readBinaryFile(fullPositionsPath, positions_float64, total_vertices_times_dim)) return false;
    } else if (positionsFileInfo.dataType == "float16") {
        std::cerr << "Error: float16 positions not yet supported without a conversion library." << std::endl;
        return false;
    } else {
        std::cerr << "Error: Unsupported data type for positions: " << positionsFileInfo.dataType << std::endl;
        return false;
    }

    // 4. Reconstruct streamlines
    for (size_t i = 0; i < num_streamlines; ++i) {
        Streamline s;
        size_t start_offset = (i == 0) ? 0 : (offsets_is_64bit ? offsets_uint64[i-1] : offsets_uint32[i-1]);
        size_t end_offset = offsets_is_64bit ? offsets_uint64[i] : offsets_uint32[i];
        size_t num_points_in_streamline = end_offset - start_offset;

        for (size_t pt_idx = 0; pt_idx < num_points_in_streamline; ++pt_idx) {
            std::array<float, 3> point;
            size_t base_vertex_idx = start_offset + pt_idx;
            for (int dim = 0; dim < positionsFileInfo.dimensions; ++dim) {
                size_t current_pos_idx = base_vertex_idx * positionsFileInfo.dimensions + dim;
                if (dim < 3) { // only take first 3 dimensions for point
                    if (positionsFileInfo.dataType == "float32") {
                        point[dim] = positions_float32[current_pos_idx];
                    } else if (positionsFileInfo.dataType == "float64") {
                        point[dim] = static_cast<float>(positions_float64[current_pos_idx]);
                    }
                    // else if (positionsFileInfo.dataType == "float16") { ... }
                }
            }
            s.addPoint(point);
        }
        tractogram.addStreamline(s);
    }

    // 5. Read dpv (data per vertex)
    std::string dpvPath = joinPath(trxFolderPath, "dpv");
    std::vector<std::string> dpvFiles = listFilesInDirectory(dpvPath);
    for (const auto& dpvFile : dpvFiles) {
        FileInfo dpvFileInfo = parseFileName(dpvFile);
        if (dpvFileInfo.dataType.empty()) {
            std::cerr << "Warning: Could not parse dpv filename: " << dpvFile << std::endl;
            continue;
        }
        std::string fullDpvPath = joinPath(dpvPath, dpvFile);
        // Read data based on dpvFileInfo.dataType and dpvFileInfo.dimensions
        // And associate with Streamline::scalarData or a new Tractogram field.
        // This part requires careful handling of different data types and mapping to streamlines.
        // For now, we'll skip the full implementation of dpv/dps/groups.
        std::cout << "Skipping reading of dpv file (not fully implemented): " << fullDpvPath << std::endl;
    }

    // 6. Read dps (data per streamline) - Similar to dpv
    std::string dpsPath = joinPath(trxFolderPath, "dps");
    std::vector<std::string> dpsFiles = listFilesInDirectory(dpsPath);
     for (const auto& dpsFile : dpsFiles) {
        // ... parsing and reading logic ...
        std::cout << "Skipping reading of dps file (not fully implemented): " << joinPath(dpsPath, dpsFile) << std::endl;
    }

    // 7. Read groups - Read files in 'groups' directory, then data from 'dpg'
    std::string groupsPath = joinPath(trxFolderPath, "groups");
    std::vector<std::string> groupFiles = listFilesInDirectory(groupsPath);
    for (const auto& groupFile : groupFiles) {
        // ... parsing and reading logic for group indices ...
        std::cout << "Skipping reading of group file (not fully implemented): " << joinPath(groupsPath, groupFile) << std::endl;
        // Then read corresponding dpg data
    }


    std::cout << "TRX file reading partially implemented. Header, positions, and offsets are processed." << std::endl;
    return true;
}

// --- TRX Writing Logic ---
bool writeTRX(const std::string& trxFolderPath, const Tractogram& tractogram) {
    // 1. Create base directory and subdirectories
    if (!createDirectory(trxFolderPath)) return false;
    if (!createDirectory(joinPath(trxFolderPath, "dpv"))) return false;
    if (!createDirectory(joinPath(trxFolderPath, "dps"))) return false;
    if (!createDirectory(joinPath(trxFolderPath, "groups"))) return false;
    if (!createDirectory(joinPath(trxFolderPath, "dpg"))) return false;

    // 2. Write header.json
    nlohmann::json header_json;
    for (const auto& meta_pair : tractogram.getMetadata()) {
        // Attempt to parse metadata values that might be JSON strings themselves
        try {
            header_json[meta_pair.first] = nlohmann::json::parse(meta_pair.second);
        } catch (nlohmann::json::parse_error&) {
            header_json[meta_pair.first] = meta_pair.second; // Store as plain string if not parsable JSON
        }
    }
     // Ensure NB_STREAMLINES and NB_VERTICES are numbers if they were stored as strings from metadata
    if (header_json.contains("NB_STREAMLINES") && header_json["NB_STREAMLINES"].is_string()) {
        header_json["NB_STREAMLINES"] = std::stoll(header_json["NB_STREAMLINES"].get<std::string>());
    } else if (!header_json.contains("NB_STREAMLINES")) {
         header_json["NB_STREAMLINES"] = tractogram.getStreamlines().size();
    }

    size_t total_vertices = 0;
    for(const auto& s : tractogram.getStreamlines()){
        total_vertices += s.getPoints().size();
    }
    if (header_json.contains("NB_VERTICES") && header_json["NB_VERTICES"].is_string()) {
        header_json["NB_VERTICES"] = std::stoll(header_json["NB_VERTICES"].get<std::string>());
    } else if (!header_json.contains("NB_VERTICES")) {
        header_json["NB_VERTICES"] = total_vertices;
    }


    std::string headerPath = joinPath(trxFolderPath, "header.json");
    std::ofstream headerFile(headerPath);
    if (!headerFile.is_open()) {
        std::cerr << "Error: Could not open header.json for writing at " << headerPath << std::endl;
        return false;
    }
    headerFile << header_json.dump(4); // Pretty print with 4 spaces
    headerFile.close();


    // 3. Prepare and write positions and offsets
    std::vector<float> positions_data; // Assuming float32 for now
    std::vector<uint32_t> offsets_data; // Assuming uint32 for now

    size_t current_offset = 0;
    for (const auto& streamline : tractogram.getStreamlines()) {
        for (const auto& point : streamline.getPoints()) {
            positions_data.push_back(point[0]);
            positions_data.push_back(point[1]);
            positions_data.push_back(point[2]);
        }
        current_offset += streamline.getPoints().size();
        offsets_data.push_back(static_cast<uint32_t>(current_offset));
    }

    if (!writeBinaryFile(joinPath(trxFolderPath, "positions.3.float32.bin"), positions_data)) return false;
    if (!writeBinaryFile(joinPath(trxFolderPath, "offsets.uint32.bin"), offsets_data)) return false;

    // 4. Write dpv, dps, groups, dpg (Placeholder - needs actual data from Tractogram)
    // Example for one dpv file:
    // std::vector<float> example_dpv_data; // Populate this
    // if (!writeBinaryFile(joinPath(trxFolderPath, "dpv/scalar_name.1.float32.bin"), example_dpv_data)) return false;

    std::cout << "TRX file writing partially implemented. Header, positions, and offsets are written." << std::endl;
    return true;
}

} // namespace trx
