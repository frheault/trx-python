#include "streamline_ops.h"
#include <vector>
#include <string>
#include <sstream>
#include <iomanip> // For std::fixed, std::setprecision
#include <cmath>   // For std::round, std::pow
#include <algorithm> // For std::min, std::set_intersection etc. (though not directly used for map ops)
#include <iterator>  // For std::inserter (though not directly used for map ops)

namespace streamline_ops {

// --- Hashing Mechanism ---

// Helper to format a float to a specific precision string
std::string format_float(float val, int precision) {
    std::ostringstream oss;
    if (precision >= 0) {
        oss << std::fixed << std::setprecision(precision) << val;
    } else {
        // Use default stream precision for float (typically 6-7 decimal places)
        // Or convert to string in a way that preserves full precision if possible
        // std::to_string might not be precise enough for all cases.
        // oss << val; // This might not be ideal for hashing if precision varies.
        // Using a high precision fixed format for consistency if no precision is set.
        oss << std::fixed << std::setprecision(6) << val; // Default to 6 for consistency if not specified
    }
    return oss.str();
}


StreamlineKey getStreamlineKey(const Streamline& streamline, int precision) {
    std::ostringstream key_builder;
    const auto& points = streamline.getPoints();
    size_t num_points = points.size();

    if (num_points == 0) {
        return ""; // Or some other representation for an empty streamline
    }

    // First HASH_SUBSET_SIZE points
    for (size_t i = 0; i < std::min(num_points, HASH_SUBSET_SIZE); ++i) {
        for (int dim = 0; dim < 3; ++dim) {
            key_builder << format_float(points[i][dim], precision) << "_";
        }
    }

    // If more than 2*HASH_SUBSET_SIZE points, add a separator for clarity (optional)
    if (num_points > 2 * HASH_SUBSET_SIZE) {
        key_builder << "|";
    }

    // Last HASH_SUBSET_SIZE points (if streamline is long enough)
    if (num_points > HASH_SUBSET_SIZE) {
        size_t start_index_for_last_set = std::max(HASH_SUBSET_SIZE, num_points - HASH_SUBSET_SIZE);
        for (size_t i = start_index_for_last_set; i < num_points; ++i) {
            for (int dim = 0; dim < 3; ++dim) {
                key_builder << format_float(points[i][dim], precision) << "_";
            }
        }
    }
    return key_builder.str();
}

HashedStreamlinesMap hashStreamlines(const std::vector<Streamline>& streamlines, int precision) {
    HashedStreamlinesMap hashed_map;
    for (size_t i = 0; i < streamlines.size(); ++i) {
        StreamlineKey key = getStreamlineKey(streamlines[i], precision);
        // In case of hash collision, this will keep the first encountered index.
        // This matches the Python behavior (dict keeps first value for duplicate keys).
        if (hashed_map.find(key) == hashed_map.end()) {
            hashed_map[key] = i;
        }
    }
    return hashed_map;
}

// --- Streamline Set Operations ---

HashedStreamlinesMap intersection(const HashedStreamlinesMap& map1, const HashedStreamlinesMap& map2) {
    HashedStreamlinesMap result;
    for (const auto& pair1 : map1) {
        if (map2.count(pair1.first)) {
            // Keep original index from map1
            result[pair1.first] = pair1.second;
        }
    }
    return result;
}

HashedStreamlinesMap difference(const HashedStreamlinesMap& map1, const HashedStreamlinesMap& map2) {
    HashedStreamlinesMap result;
    for (const auto& pair1 : map1) {
        if (map2.find(pair1.first) == map2.end()) {
            // Key from map1 is not in map2
            result[pair1.first] = pair1.second;
        }
    }
    return result;
}

HashedStreamlinesMap union_set(const HashedStreamlinesMap& map1, const HashedStreamlinesMap& map2) {
    HashedStreamlinesMap result = map1; // Start with all elements from map1
    for (const auto& pair2 : map2) {
        if (result.find(pair2.first) == result.end()) {
            // If key from map2 is not already in result (from map1), add it.
            // The index here would correspond to map2's original indexing.
            // This might need adjustment if a global indexing is required for the union.
            // For now, storing map2's index.
            result[pair2.first] = pair2.second;
        }
    }
    return result;
}


// --- Perform Streamlines Operation ---

// Main function to operate on Tractogram objects
Tractogram performStreamlinesOperation(
    OperationType op_type,
    const std::vector<const Tractogram*>& tractograms,
    int hash_precision) {

    if (tractograms.empty()) {
        return Tractogram();
    }

    // Hash the first tractogram's streamlines
    HashedStreamlinesMap current_hashes = hashStreamlines(tractograms[0]->getStreamlines(), hash_precision);
    // Keep track of which original tractogram and streamline index corresponds to the keys in current_hashes
    std::map<StreamlineKey, std::pair<size_t, size_t>> key_to_original_source;
    for(const auto& pair : current_hashes){
        key_to_original_source[pair.first] = {0, pair.second};
    }


    // Iteratively apply the operation for subsequent tractograms
    for (size_t i = 1; i < tractograms.size(); ++i) {
        HashedStreamlinesMap next_hashes = hashStreamlines(tractograms[i]->getStreamlines(), hash_precision);
        std::map<StreamlineKey, std::pair<size_t, size_t>> next_key_to_original_source;
         for(const auto& pair : next_hashes){
            next_key_to_original_source[pair.first] = {i, pair.second};
        }

        HashedStreamlinesMap result_hashes;
        switch (op_type) {
            case OperationType::INTERSECTION:
                result_hashes = intersection(current_hashes, next_hashes);
                break;
            case OperationType::DIFFERENCE: // current - next
                result_hashes = difference(current_hashes, next_hashes);
                break;
            case OperationType::UNION:
                result_hashes = union_set(current_hashes, next_hashes);
                break;
        }
        current_hashes = result_hashes;

        // Update key_to_original_source for union:
        // For intersection & difference, keys in current_hashes are already from valid original sources.
        // For union, new keys might have been added from next_hashes.
        if (op_type == OperationType::UNION) {
            for(const auto& pair : current_hashes) {
                if(key_to_original_source.find(pair.first) == key_to_original_source.end()){
                    // This key must have come from next_hashes (map2 in union_set)
                     if(next_key_to_original_source.count(pair.first)){
                        key_to_original_source[pair.first] = next_key_to_original_source[pair.first];
                     }
                }
            }
        } else { // For intersection and difference, filter keys not in the result
            std::map<StreamlineKey, std::pair<size_t, size_t>> filtered_key_to_source;
            for(const auto& pair : current_hashes){
                if(key_to_original_source.count(pair.first)){
                     filtered_key_to_source[pair.first] = key_to_original_source[pair.first];
                }
            }
            key_to_original_source = filtered_key_to_source;
        }
    }

    // Reconstruct the resulting Tractogram
    Tractogram result_tractogram;
    // Copy metadata from the first tractogram (or decide on a merging strategy)
    if (!tractograms.empty()) {
        for (const auto& meta_pair : tractograms[0]->getMetadata()) {
            result_tractogram.addMetadata(meta_pair.first, meta_pair.second);
        }
    }
    result_tractogram.addMetadata("OPERATION_PERFORMED",
        op_type == OperationType::INTERSECTION ? "intersection" :
        op_type == OperationType::DIFFERENCE ? "difference" : "union");


    std::vector<Streamline> result_streamlines;
    for (const auto& pair : current_hashes) {
        const auto& source_info = key_to_original_source[pair.first];
        result_streamlines.push_back(tractograms[source_info.first]->getStreamlines()[source_info.second]);
    }
    // Add streamlines to tractogram
    for(const auto& sl : result_streamlines) {
        result_tractogram.addStreamline(sl);
    }
    // Update NB_STREAMLINES and NB_VERTICES (approximate if only hashes are available)
    result_tractogram.addMetadata("NB_STREAMLINES", std::to_string(result_streamlines.size()));
    size_t total_vertices = 0;
    for(const auto& s : result_streamlines) total_vertices += s.getPoints().size();
    result_tractogram.addMetadata("NB_VERTICES", std::to_string(total_vertices));


    return result_tractogram;
}


// Overload for streamline lists
std::vector<Streamline> performStreamlinesOperation(
    OperationType op_type,
    const std::vector<std::reference_wrapper<const std::vector<Streamline>>>& streamline_lists,
    const std::vector<std::vector<size_t>>& original_indices_lists, // Outer: list_idx, Inner: original indices for that list
    std::vector<std::pair<size_t, size_t>>& out_resulting_streamline_original_indices,
    int hash_precision
) {
    out_resulting_streamline_original_indices.clear();
    if (streamline_lists.empty()) {
        return {};
    }

    HashedStreamlinesMap current_hashes = hashStreamlines(streamline_lists[0].get(), hash_precision);
    // Map to track the true original source (list_idx, index_in_that_list) for each key in current_hashes
    std::map<StreamlineKey, std::pair<size_t, size_t>> key_to_original_source;
    for(const auto& pair : current_hashes){
        key_to_original_source[pair.first] = {0, original_indices_lists[0][pair.second]};
    }

    for (size_t i = 1; i < streamline_lists.size(); ++i) {
        HashedStreamlinesMap next_hashes = hashStreamlines(streamline_lists[i].get(), hash_precision);
        std::map<StreamlineKey, std::pair<size_t, size_t>> next_key_to_original_source;
        for(const auto& pair : next_hashes){
           next_key_to_original_source[pair.first] = {i, original_indices_lists[i][pair.second]};
        }


        HashedStreamlinesMap result_hashes;
        switch (op_type) {
            case OperationType::INTERSECTION:
                result_hashes = intersection(current_hashes, next_hashes);
                break;
            case OperationType::DIFFERENCE:
                result_hashes = difference(current_hashes, next_hashes);
                break;
            case OperationType::UNION:
                result_hashes = union_set(current_hashes, next_hashes);
                break;
        }
        current_hashes = result_hashes;

        // Update key_to_original_source similar to the Tractogram version
        if (op_type == OperationType::UNION) {
             for(const auto& pair : current_hashes) { // pair.second is index in its list, not global
                if(key_to_original_source.find(pair.first) == key_to_original_source.end()){
                     if(next_key_to_original_source.count(pair.first)){
                        key_to_original_source[pair.first] = next_key_to_original_source[pair.first];
                     }
                }
            }
        } else {
            std::map<StreamlineKey, std::pair<size_t, size_t>> filtered_key_to_source;
            for(const auto& pair : current_hashes){
                 if(key_to_original_source.count(pair.first)){ // Key must exist from previous steps
                    filtered_key_to_source[pair.first] = key_to_original_source[pair.first];
                 }
            }
            key_to_original_source = filtered_key_to_source;
        }
    }

    std::vector<Streamline> result_streamlines_vec;
    for (const auto& pair : current_hashes) {
        const auto& source_info = key_to_original_source[pair.first]; // source_info: {list_idx, original_idx_in_that_list}
        // Find the streamline in the correct original list using the original index.
        // This requires that original_indices_lists[source_info.first] contains the index
        // that maps to the correct streamline in streamline_lists[source_info.first].
        // The `pair.second` from HashedStreamlinesMap is the index *within the list passed to hashStreamlines*.
        // We need to find the streamline in `streamline_lists[source_info.first]` whose *original* index is `source_info.second`.

        // This is a bit convoluted. The `pair.second` from `current_hashes` is the index *within its temporary list*.
        // `key_to_original_source[pair.first]` gives `{original_list_idx, original_streamline_idx_in_that_list}`.
        // So we need to find the streamline in `streamline_lists[original_list_idx]`
        // that corresponds to `original_streamline_idx_in_that_list`.
        // The `original_indices_lists` helps map the temporary index back to the true original index.

        // Simpler: current_hashes' value is the index *within the list it was generated from*.
        // key_to_original_source maps the key to {list_idx_of_origin, streamline_idx_in_that_list_of_origin}
        // So, streamline_lists[source_info.first].get()[source_info.second_from_original_hash_map] is the streamline.
        // This means the HashedStreamlinesMap value (pair.second) must be the one used for lookup in its original list.
        // Let's re-trace:
        // 1. hashStreamlines(list, precision) -> map<Key, IdxInList>
        // 2. current_hashes = map for list 0.
        // 3. key_to_original_source: map<Key, {ListIdx=0, OriginalIdxFromList0[IdxInList0]}>
        // When merging, if a key is kept from current_hashes, its source info is correct.
        // If a key is added from next_hashes (in UNION), its source info is {next_list_idx, OriginalIdxFromListNext[IdxInListNext]}

        // The value `pair.second` in `current_hashes` is the *index within the list that the key currently represents*.
        // This list could be the first list, or for UNION, it could be the second list if the key was new.
        // `key_to_original_source` correctly tracks the *absolute original* source.
        const auto& streamline_source_list_idx = source_info.first;
        const auto& original_idx_in_source_list = source_info.second;

        // Find the streamline in streamline_lists[streamline_source_list_idx].get()
        // whose original index (from original_indices_lists) was original_idx_in_source_list.
        // This is tricky because hashStreamlines gives local indices.
        // The HashedStreamlinesMap should store the original index directly if possible.

        // Let's adjust hashStreamlines and the logic to correctly use original_indices_lists.
        // For now, assuming `source_info.second` is the direct index into `streamline_lists[source_info.first].get()`
        // that corresponds to an *unfiltered* version of that list.
        // This part is the most complex if original_indices_lists are not just identity mappings initially.

        // Correct logic: The HashedStreamlinesMap stores the index of the streamline *in the list it was hashed from*.
        // key_to_original_source stores {original_list_index, original_streamline_index_within_that_original_list}
        // So, we need to retrieve the streamline from `streamline_lists[source_info.first].get()`
        // using an index that corresponds to `source_info.second`.
        // This requires `streamline_lists[list_idx]` to be the list from which `original_indices_lists[list_idx]` was derived.

        // The current HashedStreamlinesMap's value (pair.second) IS the index into the *current effective list*
        // that current_hashes represents. key_to_original_source helps us get the actual streamline.
        const std::vector<Streamline>& source_list = streamline_lists[source_info.first].get();
        size_t index_in_source_list = static_cast<size_t>(-1);

        // We need to find which item in `source_list` corresponds to the `original_idx_in_source_list`
        const std::vector<size_t>& current_original_indices = original_indices_lists[source_info.first];
        for(size_t k=0; k < current_original_indices.size(); ++k) {
            if(current_original_indices[k] == original_idx_in_source_list) {
                index_in_source_list = k;
                break;
            }
        }

        if(index_in_source_list != static_cast<size_t>(-1) && index_in_source_list < source_list.size()) {
             result_streamlines_vec.push_back(source_list[index_in_source_list]);
             out_resulting_streamline_original_indices.push_back(source_info); // Store {original_list_idx, original_idx_in_that_list}
        } else {
            // This case should ideally not happen if logic is correct
            std::cerr << "Error: Could not correctly map hashed streamline back to its source." << std::endl;
        }
    }
    return result_streamlines_vec;
}


} // namespace streamline_ops
