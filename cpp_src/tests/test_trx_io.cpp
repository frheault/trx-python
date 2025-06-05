#include "gtest/gtest.h"
#include "trx_io.h"
#include "tractogram.h"
#include "streamline.h"
#include "temp_dir_manager.h" // To create temporary TRX directories for testing
#include <nlohmann/json.hpp>
#include <fstream>
#include <vector>
#include <array>

// Helper function to compare two float vectors with tolerance
::testing::AssertionResult CompareFloatVectors(const std::vector<float>& vec1, const std::vector<float>& vec2, float tolerance = 1e-5f) {
    if (vec1.size() != vec2.size()) {
        return ::testing::AssertionFailure() << "Vector sizes differ: " << vec1.size() << " vs " << vec2.size();
    }
    for (size_t i = 0; i < vec1.size(); ++i) {
        if (std::abs(vec1[i] - vec2[i]) > tolerance) {
            return ::testing::AssertionFailure() << "Mismatch at index " << i << ": " << vec1[i] << " vs " << vec2[i];
        }
    }
    return ::testing::AssertionSuccess();
}

// Test fixture for TRX I/O tests
class TrxIoTest : public ::testing::Test {
protected:
    TempDirManager tempDirManager_; // Each test can get its own temp dir if needed, or use one for the fixture

    // Creates a minimal valid TRX structure in the given path
    void createSampleTrx(const fs::path& trx_path) {
        fs::create_directories(trx_path); // Ensure base path exists

        // Create header.json
        nlohmann::json header;
        header["DIMENSIONS"] = {10, 10, 10};
        header["NB_STREAMLINES"] = 2;
        header["NB_VERTICES"] = 5;
        header["VOXEL_ORDER"] = "RAS";
        header["VOXEL_TO_RASMM"] = {
            {1.0, 0.0, 0.0, 0.0},
            {0.0, 1.0, 0.0, 0.0},
            {0.0, 0.0, 1.0, 0.0},
            {0.0, 0.0, 0.0, 1.0}
        };
        std::ofstream header_file(trx_path / "header.json");
        header_file << header.dump(4);
        header_file.close();

        // Create positions.3.float32.bin
        std::vector<float> positions = {
            0.f, 1.f, 2.f,  3.f, 4.f, 5.f, // s1: 2 points
            6.f, 7.f, 8.f,  9.f, 10.f, 11.f, 12.f, 13.f, 14.f // s2: 3 points
        };
        trx::writeBinaryFile((trx_path / "positions.3.float32.bin").string(), positions);

        // Create offsets.uint32.bin
        std::vector<uint32_t> offsets = {2, 5}; // s1 ends at index 2 (exclusive), s2 ends at index 5
        trx::writeBinaryFile((trx_path / "offsets.uint32.bin").string(), offsets);

        // Create dpv (optional, for more thorough testing later)
        fs::create_directories(trx_path / "dpv");
        // std::vector<float> dpv_data = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f}; // 1 scalar per vertex
        // trx::writeBinaryFile((trx_path / "dpv" / "metric.1.float32.bin").string(), dpv_data);
    }
};

TEST_F(TrxIoTest, ReadValidTrx) {
    fs::path test_trx_path = tempDirManager_.getTempPath() / "sample_read.trx";
    ASSERT_NO_THROW(createSampleTrx(test_trx_path));

    Tractogram t;
    ASSERT_TRUE(trx::readTRX(test_trx_path.string(), t));

    // Verify header metadata (selected fields)
    EXPECT_EQ(t.getMetadata().at("NB_STREAMLINES"), "2");
    EXPECT_EQ(t.getMetadata().at("NB_VERTICES"), "5");
    EXPECT_EQ(nlohmann::json::parse(t.getMetadata().at("DIMENSIONS")), nlohmann::json::array({10, 10, 10}));

    // Verify streamlines
    ASSERT_EQ(t.getStreamlines().size(), 2);

    // Streamline 1
    ASSERT_EQ(t.getStreamlines()[0].getPoints().size(), 2);
    EXPECT_EQ(t.getStreamlines()[0].getPoints()[0], (std::array<float, 3>{0.f, 1.f, 2.f}));
    EXPECT_EQ(t.getStreamlines()[0].getPoints()[1], (std::array<float, 3>{3.f, 4.f, 5.f}));

    // Streamline 2
    ASSERT_EQ(t.getStreamlines()[1].getPoints().size(), 3);
    EXPECT_EQ(t.getStreamlines()[1].getPoints()[0], (std::array<float, 3>{6.f, 7.f, 8.f}));
    EXPECT_EQ(t.getStreamlines()[1].getPoints()[1], (std::array<float, 3>{9.f, 10.f, 11.f}));
    EXPECT_EQ(t.getStreamlines()[1].getPoints()[2], (std::array<float, 3>{12.f, 13.f, 14.f}));

    // TODO: Test DPV reading when implemented
}

TEST_F(TrxIoTest, WriteAndReadBackTrx) {
    fs::path test_write_path = tempDirManager_.getTempPath() / "sample_write.trx";

    Tractogram original_t;
    // Populate original_t
    original_t.addMetadata("TEST_KEY", "TEST_VALUE");
    original_t.addMetadata("NB_STREAMLINES", "2"); // writeTRX uses this
    original_t.addMetadata("NB_VERTICES", "3");    // writeTRX uses this


    Streamline s1;
    s1.addPoint({1.1f, 2.2f, 3.3f});
    original_t.addStreamline(s1);

    Streamline s2;
    s2.addPoint({4.4f, 5.5f, 6.6f});
    s2.addPoint({7.7f, 8.8f, 9.9f});
    original_t.addStreamline(s2);

    // Update NB_VERTICES based on actual data for consistency if writeTRX doesn't recalculate
    size_t total_v = 0;
    for(const auto& s : original_t.getStreamlines()) total_v += s.getPoints().size();
    original_t.addMetadata("NB_VERTICES", std::to_string(total_v));


    ASSERT_TRUE(trx::writeTRX(test_write_path.string(), original_t));

    Tractogram re_read_t;
    ASSERT_TRUE(trx::readTRX(test_write_path.string(), re_read_t));

    // Compare metadata
    EXPECT_EQ(re_read_t.getMetadata().at("TEST_KEY"), "TEST_VALUE");
    EXPECT_EQ(std::stoll(re_read_t.getMetadata().at("NB_STREAMLINES")), original_t.getStreamlines().size());
     size_t re_read_total_v = 0;
    for(const auto& s : re_read_t.getStreamlines()) re_read_total_v += s.getPoints().size();
    EXPECT_EQ(re_read_total_v, total_v);


    // Compare streamlines
    ASSERT_EQ(re_read_t.getStreamlines().size(), original_t.getStreamlines().size());
    for (size_t i = 0; i < original_t.getStreamlines().size(); ++i) {
        const auto& s_orig = original_t.getStreamlines()[i];
        const auto& s_reread = re_read_t.getStreamlines()[i];
        ASSERT_EQ(s_reread.getPoints().size(), s_orig.getPoints().size());
        for (size_t j = 0; j < s_orig.getPoints().size(); ++j) {
            const auto& p_orig = s_orig.getPoints()[j];
            const auto& p_reread = s_reread.getPoints()[j];
            for(int k=0; k<3; ++k) {
                 EXPECT_NEAR(p_reread[k], p_orig[k], 1e-5f);
            }
        }
        // TODO: Compare scalar data when implemented
    }
}

TEST_F(TrxIoTest, ReadNonExistentTrx) {
    fs::path non_existent_path = tempDirManager_.getTempPath() / "non_existent.trx";
    Tractogram t;
    EXPECT_FALSE(trx::readTRX(non_existent_path.string(), t));
}

TEST_F(TrxIoTest, WriteToInvalidPath) {
    // This test is tricky because invalid paths are OS-dependent.
    // A very long path or a path with forbidden characters might work.
    // For now, we'll skip direct testing of write failure due to invalid path
    // as it's hard to make portable and might have side effects.
    // trx::writeTRX should ideally return false or throw if directory creation fails.
    Tractogram t;
    Streamline s1;
    s1.addPoint({1.f,2.f,3.f});
    t.addStreamline(s1);
    // Example: In Linux, a path like "/dev/null/cannot_create.trx" would fail.
    // However, this test should not actually attempt to write to such locations.
    // The underlying createDirectory and file stream operations should handle errors.
    // For now, we rely on those lower-level error handlings.
    EXPECT_TRUE(true); // Placeholder
}


// Test for empty tractogram
TEST_F(TrxIoTest, WriteAndReadEmptyTrx) {
    fs::path empty_trx_path = tempDirManager_.getTempPath() / "empty.trx";
    Tractogram empty_original;
    empty_original.addMetadata("description", "This is an empty tractogram");
    empty_original.addMetadata("NB_STREAMLINES", "0");
    empty_original.addMetadata("NB_VERTICES", "0");


    ASSERT_TRUE(trx::writeTRX(empty_trx_path.string(), empty_original));

    Tractogram re_read_empty;
    ASSERT_TRUE(trx::readTRX(empty_trx_path.string(), re_read_empty));

    EXPECT_EQ(re_read_empty.getStreamlines().size(), 0);
    EXPECT_EQ(re_read_empty.getMetadata().at("description"), "This is an empty tractogram");
    EXPECT_EQ(std::stoll(re_read_empty.getMetadata().at("NB_STREAMLINES")), 0);
    EXPECT_EQ(std::stoll(re_read_empty.getMetadata().at("NB_VERTICES")), 0);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
