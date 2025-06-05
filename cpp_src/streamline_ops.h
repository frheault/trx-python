#ifndef STREAMLINE_OPS_H
#define STREAMLINE_OPS_H

#include "streamline.h"
#include "tractogram.h"
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <array>   // For std::array
#include <cmath>   // For std::round, std::pow
#include <iomanip> // For std::fixed, std::setprecision
#include <sstream> // For std::ostringstream
#include <functional> // For std::reference_wrapper

/**
 * @brief Namespace for streamline operations such as hashing and set logic (intersection, union, difference).
 */
namespace streamline_ops {

/// @brief Default precision for hashing floating point coordinates. -1 indicates no explicit rounding (uses default float to string conversion).
const int DEFAULT_HASH_PRECISION = -1;

/// @brief Number of points from the start and end of a streamline to use for generating its hash key.
const size_t HASH_SUBSET_SIZE = 5;

/// @brief Type alias for a streamline key, typically a string generated from a subset of its points.
using StreamlineKey = std::string;

/// @brief Type alias for a map storing streamline keys and their original indices within a list or tractogram.
using HashedStreamlinesMap = std::unordered_map<StreamlineKey, size_t>;

/**
 * @brief Generates a hashable key for a single streamline.
 * The key is typically created from a subset of the streamline's points (first/last N points).
 * Point coordinates can be rounded to a specified precision.
 * @param streamline The Streamline object to hash.
 * @param precision The number of decimal places to round coordinates to.
 *                  DEFAULT_HASH_PRECISION (-1) means no explicit rounding beyond default float-to-string.
 * @return A StreamlineKey (string) representing the hashable key.
 */
StreamlineKey getStreamlineKey(const Streamline& streamline, int precision = DEFAULT_HASH_PRECISION);

/**
 * @brief Hashes a list (vector) of streamlines.
 * @param streamlines A vector of Streamline objects.
 * @param precision The precision to use for generating keys (passed to getStreamlineKey).
 * @return A HashedStreamlinesMap where keys are the generated streamline keys and
 *         values are the original indices of the streamlines in the input vector.
 *         If duplicate keys are generated (identical streamlines based on hashing criteria),
 *         the index of the first encountered streamline is stored.
 */
HashedStreamlinesMap hashStreamlines(const std::vector<Streamline>& streamlines, int precision = DEFAULT_HASH_PRECISION);

/**
 * @brief Computes the intersection of two sets of hashed streamlines.
 * The resulting map contains only keys present in both input maps.
 * Original indices are taken from map1.
 * @param map1 The first HashedStreamlinesMap.
 * @param map2 The second HashedStreamlinesMap.
 * @return A new HashedStreamlinesMap representing the intersection.
 */
HashedStreamlinesMap intersection(const HashedStreamlinesMap& map1, const HashedStreamlinesMap& map2);

/**
 * @brief Computes the difference of two sets of hashed streamlines (map1 - map2).
 * The resulting map contains keys present in map1 but not in map2.
 * Original indices are taken from map1.
 * @param map1 The HashedStreamlinesMap to subtract from.
 * @param map2 The HashedStreamlinesMap whose keys will be removed from map1.
 * @return A new HashedStreamlinesMap representing the difference.
 */
HashedStreamlinesMap difference(const HashedStreamlinesMap& map1, const HashedStreamlinesMap& map2);

/**
 * @brief Computes the union of two sets of hashed streamlines.
 * The resulting map contains all keys from map1 and map2.
 * If a key is present in both, the original index from map1 is preserved.
 * For keys unique to map2, their original index from map2 is used.
 * @param map1 The first HashedStreamlinesMap.
 * @param map2 The second HashedStreamlinesMap.
 * @return A new HashedStreamlinesMap representing the union.
 */
HashedStreamlinesMap union_set(const HashedStreamlinesMap& map1, const HashedStreamlinesMap& map2);

/**
 * @brief Enum to specify the type of set operation to perform on streamlines.
 */
enum class OperationType {
    INTERSECTION, ///< Perform intersection of streamline sets.
    DIFFERENCE,   ///< Perform difference of streamline sets (first - subsequent).
    UNION         ///< Perform union of streamline sets.
};

/**
 * @brief Performs a specified set operation iteratively on streamlines from a list of tractograms.
 * This function hashes the streamlines from each tractogram and applies the operation.
 * The resulting Tractogram contains unique streamlines based on the operation's outcome.
 * @param op_type The type of set operation (INTERSECTION, DIFFERENCE, UNION).
 * @param tractograms A vector of const pointers to Tractogram objects. Pointers are used
 *                    to avoid copying potentially large tractogram data.
 * @param hash_precision The precision used for hashing streamlines.
 * @return A new Tractogram object containing the resulting streamlines. Metadata from the
 *         first tractogram is typically copied, and an "OPERATION_PERFORMED" metadata field is added.
 *         The original source of each resulting streamline (which tractogram and its index there) is tracked internally.
 */
Tractogram performStreamlinesOperation(
    OperationType op_type,
    const std::vector<const Tractogram*>& tractograms,
    int hash_precision = DEFAULT_HASH_PRECISION
);

/**
 * @brief Performs a specified set operation iteratively on lists of streamlines.
 * This overload is useful when working directly with streamline vectors instead of full Tractogram objects.
 * It allows more granular tracking of the origin of resulting streamlines.
 * @param op_type The type of set operation (INTERSECTION, DIFFERENCE, UNION).
 * @param streamline_lists A vector of const references (via std::reference_wrapper) to vectors of Streamline objects.
 * @param original_indices_lists A vector of vectors, where each inner vector contains the original
 *                               indices corresponding to the streamlines in the respective list in `streamline_lists`.
 *                               This is used to map resulting streamlines back to their absolute original source.
 * @param out_resulting_streamline_original_indices A vector that will be populated with pairs, where each pair
 *                                                  indicates the original list index and the original streamline index
 *                                                  (from `original_indices_lists`) for each streamline in the returned vector.
 * @param hash_precision The precision used for hashing streamlines.
 * @return A vector of Streamline objects that are the result of the set operation.
 */
std::vector<Streamline> performStreamlinesOperation(
    OperationType op_type,
    const std::vector<std::reference_wrapper<const std::vector<Streamline>>>& streamline_lists,
    const std::vector<std::vector<size_t>>& original_indices_lists,
    std::vector<std::pair<size_t, size_t>>& out_resulting_streamline_original_indices,
    int hash_precision = DEFAULT_HASH_PRECISION
);


} // namespace streamline_ops

#endif // STREAMLINE_OPS_H
