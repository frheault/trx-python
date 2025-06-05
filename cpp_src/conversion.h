#ifndef CONVERSION_H
#define CONVERSION_H

#include "tractogram.h"
#include <string>
#include <vector>

/**
 * @brief Namespace for converting tractogram data between TRX format and other common formats like TRK and TCK.
 */
namespace conversion {

// --- TRK Format Conversion ---
// TRK file format specification can be found at: http://trackvis.org/docs/?subsect=fileformat
// Key aspects:
// - Binary header (1000 bytes).
// - Streamlines are sequences of (x,y,z) float coordinates.
// - Can store scalars per point and properties per streamline.
// - Endianness: Little-endian.

/**
 * @brief (Conceptual) Converts a TRX Tractogram to a TRK file.
 * This function signature is conceptual as TRK writing involves creating a file with a specific header
 * and binary data structure, not just returning a Tractogram object.
 * A more practical signature would be:
 * `bool writeTrk(const std::string& trk_filepath, const Tractogram& trx_tractogram, const TrkWriterOptions& options);`
 *
 * @param trx_tractogram The input Tractogram in TRX format.
 * @return A Tractogram object (placeholder, actual implementation would write to file).
 * @note This is a placeholder. Full TRK writing requires detailed header construction and binary formatting.
 */
// Tractogram trxToTrk(const Tractogram& trx_tractogram); // Placeholder, see above

/**
 * @brief Converts a TRK file to a TRX Tractogram object.
 * Reads the TRK file header and streamline data, then populates a Tractogram object.
 * @param trk_filepath Path to the .trk file.
 * @return A Tractogram object representing the data from the TRK file.
 *         If reading fails (e.g., file not found, invalid format), an empty or partially
 *         populated Tractogram might be returned, and errors will be logged to std::cerr.
 * @note Current implementation is a placeholder and needs to be fully developed
 *       to correctly parse the TRK binary header and streamline data, including handling
 *       endianness, scalars, and properties if present.
 */
Tractogram trkToTrx(const std::string& trk_filepath);


// --- TCK Format Conversion ---
// TCK file format (MRtrix3) specification can be found at: https://mrtrix.readthedocs.io/en/latest/reference/file_formats/tck.html
// Key aspects:
// - ASCII header with key-value pairs (e.g., "datatype: Float32LE", "count: NNN", "file: . OFFSET").
// - Header ends with "END" on a new line.
// - Streamline data is typically binary, following the header at a specified offset.
// - Points are (x,y,z) float coordinates.
// - Streamlines are separated by a specific sequence of NaNs or Infs.

/**
 * @brief Converts a TRX Tractogram to a TCK file.
 * Writes the TRX data into the TCK format, including an ASCII header and binary streamline data.
 * @param trx_tractogram The TRX Tractogram object to convert.
 * @param tck_filepath Path where the .tck file will be saved.
 * @return True if the conversion and writing were successful, false otherwise.
 * @note Assumes output data type is Float32 Little Endian.
 */
bool trxToTck(const Tractogram& trx_tractogram, const std::string& tck_filepath);

/**
 * @brief Converts a TCK file to a TRX Tractogram object.
 * Reads the TCK file, parsing its ASCII header and binary streamline data,
 * then populates a Tractogram object.
 * @param tck_filepath Path to the .tck file.
 * @return A Tractogram object representing the data from the TCK file.
 *         If reading fails, an empty or partially populated Tractogram might be returned,
 *         and errors will be logged to std::cerr.
 * @note Current implementation supports Float32 (LE/BE, though endianness conversion is not yet handled)
 *       and expects data to be at an offset specified by the "file: . OFFSET" header field.
 */
Tractogram tckToTrx(const std::string& tck_filepath);

} // namespace conversion

#endif // CONVERSION_H
