#ifndef TRX_IO_H
#define TRX_IO_H

#include "tractogram.h"
#include "streamline.h" // It's good practice to include what you use directly
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp> // For JSON parsing

// Forward declaration if needed, though for TRX structure, direct includes are probably better.

/**
 * @brief Namespace for TRX (Tractogram File Format) input/output operations.
 */
namespace trx
{

// Structs to represent the TRX file structure (header part)
// These can be expanded as needed based on the full TRX specification.
// Currently, these are not directly returned but used internally or for conceptual clarity.
/**
 * @struct HeaderData
 * @brief Represents essential data fields typically found in a TRX header.json.
 * @note This struct is for conceptual organization; actual header data is often
 *       directly populated into the Tractogram's metadata or used during parsing.
 */
struct HeaderData {
    /// @brief Transformation matrix from voxel coordinates to RAS (Right-Anterior-Superior) world coordinates.
    std::vector<std::vector<float>> voxel_to_rasmm;
    /// @brief Dimensions of the reference volume (e.g., anatomical image).
    std::vector<int> dimensions;
    // Other fields from header.json like "NB_STREAMLINES", "NB_VERTICES"
    // will be stored directly in the Tractogram object or used during parsing.
    // Additional metadata can be stored in Tractogram::metadata
};

// Main functions for TRX I/O

/**
 * @brief Reads a TRX tractogram from a specified folder path.
 * The TRX format is expected to be a directory containing a 'header.json' file,
 * 'positions.*.bin', 'offsets.*.bin', and optional data per point/streamline/group.
 * @param trxFolderPath Path to the root folder of the TRX dataset.
 * @param tractogram A Tractogram object to be populated with the loaded data.
 * @return True if reading was successful, false otherwise.
 * @note Comments on potential TempDirManager integration for compressed files:
 *       // TODO: For compressed TRX (.trx.gz), instantiate TempDirManager here to handle decompression.
 */
bool readTRX(const std::string& trxFolderPath, Tractogram& tractogram);

/**
 * @brief Writes a Tractogram object to a specified folder path in TRX format.
 * This will create the necessary directory structure (header.json, binary files for
 * positions, offsets, etc.) within the given folder.
 * @param trxFolderPath Path to the folder where the TRX dataset will be written.
 *                      This folder will be created if it doesn't exist.
 * @param tractogram The Tractogram object to write.
 * @return True if writing was successful, false otherwise.
 * @note Comments on potential TempDirManager integration for incremental writing:
 *       // TODO: Consider using TempDirManager if TRX components are written incrementally.
 */
bool writeTRX(const std::string& trxFolderPath, const Tractogram& tractogram);

} // namespace trx

#endif // TRX_IO_H
