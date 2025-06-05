#ifndef TRACTOGRAM_H
#define TRACTOGRAM_H

#include "streamline.h"
#include <vector>
#include <map>
#include <string>

/**
 * @brief Represents a collection of streamlines and associated metadata.
 * This class serves as a container for an entire tractography dataset.
 */
class Tractogram {
public:
    /**
     * @brief Default constructor. Creates an empty tractogram.
     */
    Tractogram();

    // Member functions
    /**
     * @brief Adds a streamline to the tractogram.
     * @param streamline The Streamline object to add.
     */
    void addStreamline(const Streamline& streamline);

    /**
     * @brief Adds or updates a metadata entry.
     * Metadata is stored as key-value pairs. If the key already exists, its value is updated.
     * @param key The metadata key (string).
     * @param value The metadata value (string).
     */
    void addMetadata(const std::string& key, const std::string& value);

    /**
     * @brief Gets all streamlines stored in the tractogram.
     * @return A const reference to a vector of Streamline objects.
     */
    const std::vector<Streamline>& getStreamlines() const;

    /**
     * @brief Gets the metadata associated with the tractogram.
     * @return A const reference to a map of string key-value pairs.
     */
    const std::map<std::string, std::string>& getMetadata() const;

private:
    /// @brief Container for all the Streamline objects in this tractogram.
    std::vector<Streamline> streamlines;
    /// @brief Container for metadata, stored as key-value string pairs.
    /// This can include information from file headers or user-defined attributes.
    std::map<std::string, std::string> metadata;
};

#endif // TRACTOGRAM_H
