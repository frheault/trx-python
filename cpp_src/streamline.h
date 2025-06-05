#ifndef STREAMLINE_H
#define STREAMLINE_H

#include <vector>
#include <array>
#include <string> // For potential future use with named scalar data or properties

/**
 * @brief Represents a single streamline, composed of a sequence of 3D points and associated scalar data.
 */
class Streamline {
public:
    /**
     * @brief Default constructor. Creates an empty streamline.
     */
    Streamline();

    // Member functions
    /**
     * @brief Adds a 3D point to the streamline.
     * @param point An array of 3 floats representing the (x, y, z) coordinates of the point.
     */
    void addPoint(const std::array<float, 3>& point);

    /**
     * @brief Adds a set of scalar data associated with this streamline.
     * This current implementation associates the provided vector of floats as a single scalar dataset
     * for the streamline. If per-point scalar data is needed, this model might need adjustment,
     * or data should be added in a structure that aligns with the number of points.
     * @param data A vector of floats representing scalar values.
     */
    void addScalarData(const std::vector<float>& data);

    /**
     * @brief Gets the list of 3D points forming the streamline.
     * @return A const reference to a vector of 3D points. Each point is an std::array<float, 3>.
     */
    const std::vector<std::array<float, 3>>& getPoints() const;

    /**
     * @brief Gets the list of scalar data sets associated with the streamline.
     * Each element in the outer vector is a "set" of scalars (e.g., representing one metric).
     * @return A const reference to a vector of vector of floats.
     */
    const std::vector<std::vector<float>>& getScalarData() const;

private:
    /// @brief Stores the 3D coordinates of the points forming the streamline.
    std::vector<std::array<float, 3>> points;
    /// @brief Stores sets of scalar data. Each inner vector can represent a different scalar metric
    /// (e.g., FA, MD) associated with the streamline or its points.
    /// The current model is flexible but might require careful interpretation based on how data is added.
    std::vector<std::vector<float>> scalarData;
};

#endif // STREAMLINE_H
