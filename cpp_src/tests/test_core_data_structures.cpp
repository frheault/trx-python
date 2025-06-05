#include "gtest/gtest.h"
#include "streamline.h"
#include "tractogram.h"
#include <array>
#include <vector>
#include <string>
#include <map>

// Test fixture for Streamline tests
class StreamlineTest : public ::testing::Test {
protected:
    Streamline streamline;
};

// Test Streamline construction and point addition
TEST_F(StreamlineTest, PointAdditionAndRetrieval) {
    ASSERT_TRUE(streamline.getPoints().empty());
    ASSERT_TRUE(streamline.getScalarData().empty());

    std::array<float, 3> p1 = {1.0f, 2.0f, 3.0f};
    streamline.addPoint(p1);
    ASSERT_EQ(streamline.getPoints().size(), 1);
    ASSERT_EQ(streamline.getPoints()[0], p1);

    std::array<float, 3> p2 = {4.0f, 5.0f, 6.0f};
    streamline.addPoint(p2);
    ASSERT_EQ(streamline.getPoints().size(), 2);
    ASSERT_EQ(streamline.getPoints()[1], p2);
}

// Test Streamline scalar data addition
TEST_F(StreamlineTest, ScalarDataAdditionAndRetrieval) {
    std::vector<float> scalar_data1 = {0.1f, 0.2f};
    // Note: The current Streamline class adds scalar data associated with the *streamline*
    // not per point directly in addScalarData. If it were per-point, we'd add points first.
    // The current Streamline::scalarData is std::vector<std::vector<float>>,
    // implying each call to addScalarData adds a new set of scalars.
    streamline.addScalarData(scalar_data1);
    ASSERT_EQ(streamline.getScalarData().size(), 1);
    ASSERT_EQ(streamline.getScalarData()[0], scalar_data1);

    std::vector<float> scalar_data2 = {0.3f, 0.4f, 0.5f};
    streamline.addScalarData(scalar_data2);
    ASSERT_EQ(streamline.getScalarData().size(), 2);
    ASSERT_EQ(streamline.getScalarData()[1], scalar_data2);
}

// Test fixture for Tractogram tests
class TractogramTest : public ::testing::Test {
protected:
    Tractogram tractogram;
};

// Test Tractogram construction and streamline addition
TEST_F(TractogramTest, StreamlineAdditionAndRetrieval) {
    ASSERT_TRUE(tractogram.getStreamlines().empty());
    ASSERT_TRUE(tractogram.getMetadata().empty());

    Streamline s1;
    s1.addPoint({1.f, 2.f, 3.f});
    tractogram.addStreamline(s1);
    ASSERT_EQ(tractogram.getStreamlines().size(), 1);
    ASSERT_EQ(tractogram.getStreamlines()[0].getPoints().size(), 1);
    ASSERT_EQ(tractogram.getStreamlines()[0].getPoints()[0][0], 1.f);

    Streamline s2;
    s2.addPoint({4.f, 5.f, 6.f});
    s2.addPoint({7.f, 8.f, 9.f});
    tractogram.addStreamline(s2);
    ASSERT_EQ(tractogram.getStreamlines().size(), 2);
    ASSERT_EQ(tractogram.getStreamlines()[1].getPoints().size(), 2);
    ASSERT_EQ(tractogram.getStreamlines()[1].getPoints()[1][2], 9.f);
}

// Test Tractogram metadata
TEST_F(TractogramTest, MetadataAdditionAndRetrieval) {
    tractogram.addMetadata("version", "1.0");
    tractogram.addMetadata("patient_name", "John Doe");

    const auto& metadata = tractogram.getMetadata();
    ASSERT_EQ(metadata.size(), 2);
    ASSERT_EQ(metadata.at("version"), "1.0");
    ASSERT_EQ(metadata.at("patient_name"), "John Doe");

    // Test overwrite/update
    tractogram.addMetadata("version", "2.0");
    ASSERT_EQ(metadata.size(), 2); // Size should remain the same
    ASSERT_EQ(metadata.at("version"), "2.0");
}

// Test Streamline default constructor (called by Tractogram)
TEST_F(StreamlineTest, DefaultConstructor) {
    Streamline s; // Default constructor
    EXPECT_TRUE(s.getPoints().empty());
    EXPECT_TRUE(s.getScalarData().empty());
}

// Test Tractogram default constructor
TEST_F(TractogramTest, DefaultConstructor) {
    Tractogram t; // Default constructor
    EXPECT_TRUE(t.getStreamlines().empty());
    EXPECT_TRUE(t.getMetadata().empty());
}

// Test Streamline copy constructor and assignment (if implicitly generated or custom)
TEST_F(StreamlineTest, CopyOperations) {
    Streamline s1;
    s1.addPoint({1.f, 0.f, 0.f});
    s1.addScalarData({0.5f});

    Streamline s2 = s1; // Copy constructor
    ASSERT_EQ(s2.getPoints().size(), 1);
    ASSERT_EQ(s2.getScalarData().size(), 1);
    ASSERT_EQ(s2.getPoints()[0][0], 1.f);
    ASSERT_EQ(s2.getScalarData()[0][0], 0.5f);

    Streamline s3;
    s3 = s1; // Copy assignment
    ASSERT_EQ(s3.getPoints().size(), 1);
    ASSERT_EQ(s3.getScalarData().size(), 1);
}

// Test Tractogram copy constructor and assignment
TEST_F(TractogramTest, CopyOperations) {
    Tractogram t1;
    Streamline s_temp;
    s_temp.addPoint({1.f,1.f,1.f});
    t1.addStreamline(s_temp);
    t1.addMetadata("key", "value");

    Tractogram t2 = t1; // Copy constructor
    ASSERT_EQ(t2.getStreamlines().size(), 1);
    ASSERT_EQ(t2.getMetadata().size(), 1);
    ASSERT_EQ(t2.getStreamlines()[0].getPoints()[0][1], 1.f);
    ASSERT_EQ(t2.getMetadata().at("key"), "value");

    Tractogram t3;
    t3 = t1; // Copy assignment
    ASSERT_EQ(t3.getStreamlines().size(), 1);
    ASSERT_EQ(t3.getMetadata().size(), 1);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
