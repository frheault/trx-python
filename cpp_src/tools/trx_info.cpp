#include <iostream>
#include <string>
#include <vector>
#include "tractogram.h"
#include "trx_io.h" // Assuming readTRX is here
#include <stdexcept> // For std::runtime_error

// Basic command line argument parser
struct ArgsInfo {
    std::string inputFile;
};

ArgsInfo parseArgsInfo(int argc, char* argv[]) {
    ArgsInfo args;
    if (argc != 2) {
        throw std::runtime_error("Usage: trx_info <input_trx_folder_path>");
    }
    args.inputFile = argv[1];
    return args;
}

int main(int argc, char* argv[]) {
    ArgsInfo args;
    try {
        args = parseArgsInfo(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error parsing arguments: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Loading TRX file from: " << args.inputFile << std::endl;

    Tractogram tractogram;
    if (!trx::readTRX(args.inputFile, tractogram)) {
        std::cerr << "Error: Failed to load TRX file." << std::endl;
        return 1;
    }

    std::cout << "\n--- TRX Info ---" << std::endl;

    // Print Header Information (selected fields)
    std::cout << "Header Information:" << std::endl;
    const auto& metadata = tractogram.getMetadata();

    auto print_meta_if_exists = [&](const std::string& key) {
        auto it = metadata.find(key);
        if (it != metadata.end()) {
            std::cout << "  " << it->first << ": " << it->second << std::endl;
        } else {
            std::cout << "  " << key << ": Not found" << std::endl;
        }
    };

    print_meta_if_exists("NB_STREAMLINES");
    print_meta_if_exists("NB_VERTICES");
    print_meta_if_exists("DIMENSIONS");
    print_meta_if_exists("VOXEL_TO_RASMM");
    print_meta_if_exists("VOXEL_ORDER"); // Example of another common header field

    std::cout << "\nStreamline Count: " << tractogram.getStreamlines().size() << std::endl;

    size_t total_points = 0;
    for(const auto& streamline : tractogram.getStreamlines()) {
        total_points += streamline.getPoints().size();
    }
    std::cout << "Total Points/Vertices: " << total_points << std::endl;


    // Placeholder for DPV/DPS/Groups info
    // This requires the Tractogram/Streamline classes to store this information
    // and for trx_io to populate it.

    // Example: Data Per Vertex
    // Assuming Streamline class has a way to get dpv keys or Tractogram stores global dpv info
    std::cout << "\nData Per Vertex (DPV):" << std::endl;
    if (!tractogram.getStreamlines().empty()) {
        const auto& first_streamline_scalars = tractogram.getStreamlines()[0].getScalarData();
        if (!first_streamline_scalars.empty()) {
             std::cout << "  (Showing DPV structure based on first streamline if available)" << std::endl;
            // This is a simplification. TRX allows different streamlines to potentially have different DPV.
            // A full implementation would check all streamlines or use globally stored DPV keys.
            // For now, we assume DPV keys would be stored elsewhere or inferred.
            // Let's imagine we have a function like tractogram.getDpvKeys()
            std::cout << "  Available DPV keys (example, not fully implemented): " << std::endl;
            // for (const auto& key : tractogram.getDpvKeys()) {
            //     std::cout << "    - " << key << " (shape: e.g. " << tractogram.getDpvShape(key) << ")" << std::endl;
            // }
             std::cout << "    (DPV key listing not fully implemented in this tool)" << std::endl;
        } else {
            std::cout << "  No DPV found (or not yet parsed by trx_io)." << std::endl;
        }
    } else {
         std::cout << "  No streamlines to infer DPV from." << std::endl;
    }


    // Example: Data Per Streamline
    std::cout << "\nData Per Streamline (DPS):" << std::endl;
    // Similar to DPV, assuming Tractogram class has a way to get DPS keys.
    // std::cout << "  Available DPS keys (example, not fully implemented): " << std::endl;
    // for (const auto& key : tractogram.getDpsKeys()) {
    //     std::cout << "    - " << key << " (shape: e.g. " << tractogram.getDpsShape(key) << ")" << std::endl;
    // }
    std::cout << "  (DPS key listing not fully implemented in this tool/TRX parsing)" << std::endl;


    // Example: Groups
    std::cout << "\nGroups:" << std::endl;
    // Assuming Tractogram class has a way to get group names.
    // std::cout << "  Available groups (example, not fully implemented): " << std::endl;
    // for (const auto& group_name : tractogram.getGroupNames()) {
    //     std::cout << "    - " << group_name << " (contains X streamlines)" << std::endl;
    // }
    std::cout << "  (Group listing not fully implemented in this tool/TRX parsing)" << std::endl;

    std::cout << "\n--- End of Info ---" << std::endl;

    return 0;
}
