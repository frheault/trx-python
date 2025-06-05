#include <iostream>
#include <string>
#include <vector>
#include <stdexcept> // For std::runtime_error
#include "tractogram.h"
#include "trx_io.h"
#include "conversion.h"
#include "temp_dir_manager.h" // May be needed if conversions use temp dirs

// Basic command line argument parser
struct Args {
    std::string inputFile;
    std::string outputFile;
    std::string referenceFile;
    std::string inFormat;
    std::string outFormat;
};

// Super basic argument parsing
Args parseArgsConverter(int argc, char* argv[]) {
    Args args;
    std::vector<std::string> cli_args;
    for (int i = 1; i < argc; ++i) {
        cli_args.push_back(argv[i]);
    }

    if (cli_args.size() < 2) {
        throw std::runtime_error("Usage: trx_converter <input_file> <output_file> [--reference <ref_path>] [--in_format <fmt>] [--out_format <fmt>]");
    }

    args.inputFile = cli_args[0];
    args.outputFile = cli_args[1];

    for (size_t i = 2; i < cli_args.size(); ++i) {
        if (cli_args[i] == "--reference" && i + 1 < cli_args.size()) {
            args.referenceFile = cli_args[++i];
        } else if (cli_args[i] == "--in_format" && i + 1 < cli_args.size()) {
            args.inFormat = cli_args[++i];
        } else if (cli_args[i] == "--out_format" && i + 1 < cli_args.size()) {
            args.outFormat = cli_args[++i];
        }
    }
    return args;
}

std::string getFileExtension(const std::string& filePath) {
    size_t dotPos = filePath.rfind('.');
    if (dotPos == std::string::npos) return "";
    return filePath.substr(dotPos + 1);
}

std::string detectFormatFromExtension(const std::string& filePath, const std::string& user_specified_format) {
    if (!user_specified_format.empty()) {
        return user_specified_format;
    }
    std::string ext = getFileExtension(filePath);
    if (ext == "trx") return "TRX";
    if (ext == "trk") return "TRK";
    if (ext == "tck") return "TCK";
    // Add other formats if needed
    return ""; // Unknown
}


int main(int argc, char* argv[]) {
    Args args;
    try {
        args = parseArgsConverter(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error parsing arguments: " << e.what() << std::endl;
        return 1;
    }

    std::string in_format = detectFormatFromExtension(args.inputFile, args.inFormat);
    std::string out_format = detectFormatFromExtension(args.outputFile, args.outFormat);

    if (in_format.empty()) {
        std::cerr << "Error: Could not determine input format for " << args.inputFile << std::endl;
        return 1;
    }
    if (out_format.empty()) {
        std::cerr << "Error: Could not determine output format for " << args.outputFile << std::endl;
        return 1;
    }

    std::cout << "Input File: " << args.inputFile << " (Format: " << in_format << ")" << std::endl;
    std::cout << "Output File: " << args.outputFile << " (Format: " << out_format << ")" << std::endl;
    if (!args.referenceFile.empty()) {
        std::cout << "Reference File: " << args.referenceFile << std::endl;
    }

    Tractogram tractogram_data;
    bool load_success = false;

    // 1. Load input tractogram
    std::cout << "Loading input file..." << std::endl;
    if (in_format == "TRX") {
        // Assuming TRX is a folder
        load_success = trx::readTRX(args.inputFile, tractogram_data);
    } else if (in_format == "TCK") {
        tractogram_data = conversion::tckToTrx(args.inputFile);
        load_success = !tractogram_data.getStreamlines().empty() || tractogram_data.getMetadata().count("tck_count"); // Basic check
    } else if (in_format == "TRK") {
        tractogram_data = conversion::trkToTrx(args.inputFile);
        load_success = !tractogram_data.getStreamlines().empty() || tractogram_data.getMetadata().count("trk_version"); // Basic check
    } else {
        std::cerr << "Error: Unsupported input format: " << in_format << std::endl;
        return 1;
    }

    if (!load_success) {
        std::cerr << "Error: Failed to load input file: " << args.inputFile << std::endl;
        return 1;
    }
    std::cout << "Loaded " << tractogram_data.getStreamlines().size() << " streamlines." << std::endl;

    // TODO: Handle reference file if provided. This usually involves aligning
    // the tractogram to the reference image's space. This is a complex step
    // not covered by the current library structure. For now, we'll just note it.
    if (!args.referenceFile.empty()) {
        std::cout << "Note: Reference file processing is not yet implemented in this tool." << std::endl;
    }

    // 2. Save output tractogram
    std::cout << "Saving output file..." << std::endl;
    bool save_success = false;
    if (out_format == "TRX") {
        // Assuming TRX is a folder
        save_success = trx::writeTRX(args.outputFile, tractogram_data);
    } else if (out_format == "TCK") {
        save_success = conversion::trxToTck(tractogram_data, args.outputFile);
    } else if (out_format == "TRK") {
        // save_success = conversion::trxToTrk_file(tractogram_data, args.outputFile); // Needs actual function
        std::cerr << "Error: Saving to TRK format is not yet fully implemented." << std::endl;
    } else {
        std::cerr << "Error: Unsupported output format: " << out_format << std::endl;
        return 1;
    }

    if (!save_success) {
        std::cerr << "Error: Failed to save output file: " << args.outputFile << std::endl;
        return 1;
    }

    std::cout << "Conversion complete." << std::endl;

    return 0;
}
