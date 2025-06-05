#include "gtest/gtest.h"
#include "conversion.h"
#include "tractogram.h"
#include "streamline.h"
#include "temp_dir_manager.h" // For creating a temporary TCK file
#include <fstream>
#include <vector>
#include <array>
#include <limits> // For std::numeric_limits
#include <cmath> // For std::isnan

// Helper to compare streamlines (used for TCK tests)
::testing::AssertionResult CompareStreamlines(const Streamline& s1, const Streamline& s2, float tolerance = 1e-5f) {
    if (s1.getPoints().size() != s2.getPoints().size()) {
        return ::testing::AssertionFailure() << "Streamline point counts differ: "
                                           << s1.getPoints().size() << " vs " << s2.getPoints().size();
    }
    for (size_t i = 0; i < s1.getPoints().size(); ++i) {
        const auto& p1 = s1.getPoints()[i];
        const auto& p2 = s2.getPoints()[i];
        for (int j = 0; j < 3; ++j) {
            if (std::abs(p1[j] - p2[j]) > tolerance) {
                return ::testing::AssertionFailure() << "Point " << i << ", dimension " << j << " differs: "
                                                   << p1[j] << " vs " << p2[j];
            }
        }
    }
    // TODO: Compare scalar data if/when implemented and relevant for conversion
    return ::testing::AssertionSuccess();
}


class ConversionTest : public ::testing::Test {
protected:
    TempDirManager tempDirManager_;

    // Creates a simple TCK file for testing tckToTrx
    void createSampleTckFile(const fs::path& tck_path, const std::vector<Streamline>& streamlines) {
        std::ofstream file(tck_path, std::ios::binary);
        ASSERT_TRUE(file.is_open());

        file << "mrtrix tracks\n";
        file << "datatype: Float32LE\n";
        file << "count: " << streamlines.size() << "\n";
        // Determine data offset carefully
        std::streampos header_end_pos = file.tellp();
        std::string end_marker = "END\n";
        std::string file_offset_line = "file: . " + std::to_string(static_cast<long long>(header_end_pos) + static_cast<long long>(end_marker.length())) + "\n";
        file << file_offset_line;
        file << end_marker;

        const float nan_val = std::numeric_limits<float>::quiet_NaN();
        const std::array<float, 3> streamline_separator = {nan_val, nan_val, nan_val};

        for (const auto& streamline : streamlines) {
            for (const auto& point : streamline.getPoints()) {
                file.write(reinterpret_cast<const char*>(point.data()), sizeof(float) * 3);
            }
            file.write(reinterpret_cast<const char*>(streamline_separator.data()), sizeof(float) * 3);
        }
        file.close();
        ASSERT_TRUE(file.good()); // Check for write errors
    }
};

TEST_F(ConversionTest, TckToTrxAndBack) {
    fs::path original_tck_path = tempDirManager_.getTempPath() / "original.tck";
    fs::path written_tck_path = tempDirManager_.getTempPath() / "written_back.tck";

    // 1. Create an initial Tractogram object
    Tractogram original_trx;
    Streamline s1, s2;
    s1.addPoint({1.0f, 2.0f, 3.0f});
    s1.addPoint({1.1f, 2.1f, 3.1f});
    original_trx.addStreamline(s1);

    s2.addPoint({10.0f, 20.0f, 30.0f});
    s2.addPoint({10.1f, 20.1f, 30.1f});
    s2.addPoint({10.2f, 20.2f, 30.2f});
    original_trx.addStreamline(s2);
    original_trx.addMetadata("source", "test_data");

    // 2. Save it as a TCK file (trxToTck)
    ASSERT_TRUE(conversion::trxToTck(original_trx, original_tck_path.string()));

    // 3. Read that TCK file back into a Tractogram object (tckToTrx)
    Tractogram re_read_trx = conversion::tckToTrx(original_tck_path.string());

    // 4. Compare the re-read Tractogram with the original
    // Metadata might differ slightly due to TCK header fields, focus on streamlines
    ASSERT_EQ(re_read_trx.getStreamlines().size(), original_trx.getStreamlines().size());
    for (size_t i = 0; i < original_trx.getStreamlines().size(); ++i) {
        EXPECT_TRUE(CompareStreamlines(original_trx.getStreamlines()[i], re_read_trx.getStreamlines()[i]));
    }
    // Check some metadata if it's expected to be preserved or added
    EXPECT_TRUE(re_read_trx.getMetadata().count("tck_datatype"));
    EXPECT_EQ(re_read_trx.getMetadata().at("tck_datatype"), "Float32LE");


    // 5. (Optional but good) Write the re-read Tractogram back to another TCK file
    ASSERT_TRUE(conversion::trxToTck(re_read_trx, written_tck_path.string()));

    // 6. Compare the two TCK files (ideally byte-by-byte, or load and compare streamlines again)
    // For simplicity here, we'll load the second TCK and compare with original_trx again.
    Tractogram final_trx = conversion::tckToTrx(written_tck_path.string());
    ASSERT_EQ(final_trx.getStreamlines().size(), original_trx.getStreamlines().size());
    for (size_t i = 0; i < original_trx.getStreamlines().size(); ++i) {
        EXPECT_TRUE(CompareStreamlines(original_trx.getStreamlines()[i], final_trx.getStreamlines()[i]));
    }
}

TEST_F(ConversionTest, TckToTrxEmptyFile) {
    fs::path empty_tck_path = tempDirManager_.getTempPath() / "empty.tck";
    std::ofstream empty_file(empty_tck_path);
    empty_file << "mrtrix tracks\n";
    empty_file << "datatype: Float32LE\n";
    empty_file << "count: 0\n";
    empty_file << "file: . " << (empty_file.tellp() + std::streampos(std::string("END\n").length())) << "\n";
    empty_file << "END\n";
    empty_file.close();

    Tractogram t = conversion::tckToTrx(empty_tck_path.string());
    EXPECT_EQ(t.getStreamlines().size(), 0);
    EXPECT_TRUE(t.getMetadata().count("tck_count"));
    EXPECT_EQ(t.getMetadata().at("tck_count"), "0");
}

// Placeholder for TRK tests - TRK is significantly more complex
TEST_F(ConversionTest, TrkToTrxPlaceholder) {
    // This would require a valid sample.trk file or a function to create one.
    // For now, just acknowledge it's a placeholder.
    // Tractogram t = conversion::trkToTrx("dummy_nonexistent.trk");
    // EXPECT_TRUE(t.getStreamlines().empty()); // Expect empty if file doesn't exist or fails to parse
    std::cout << "Note: Full trkToTrx testing requires a valid sample TRK file and complete parsing logic." << std::endl;
    SUCCEED();
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
