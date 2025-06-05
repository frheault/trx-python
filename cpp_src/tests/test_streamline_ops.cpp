#include "gtest/gtest.h"
#include "streamline_ops.h"
#include "streamline.h"
#include "tractogram.h"

using namespace streamline_ops;

// Helper to create a streamline with a simple sequence of points
Streamline createSimpleStreamline(const std::vector<float>& values) {
    Streamline s;
    for (size_t i = 0; i < values.size(); i += 3) {
        if (i + 2 < values.size()) {
            s.addPoint({values[i], values[i+1], values[i+2]});
        }
    }
    return s;
}

TEST(StreamlineOpsTest, GetStreamlineKeyBasic) {
    Streamline s1 = createSimpleStreamline({1,2,3, 4,5,6, 7,8,9, 10,11,12, 13,14,15}); // 5 points
    Streamline s2 = createSimpleStreamline({1,2,3, 4,5,6, 7,8,9, 10,11,12, 13,14,15});
    Streamline s3 = createSimpleStreamline({0,2,3, 4,5,6, 7,8,9, 10,11,12, 13,14,15});
    Streamline s_short = createSimpleStreamline({1,2,3, 4,5,6}); // 2 points

    // Default precision (should be fixed at 6 decimal places internally for consistency by format_float)
    StreamlineKey key1 = getStreamlineKey(s1);
    StreamlineKey key2 = getStreamlineKey(s2);
    StreamlineKey key3 = getStreamlineKey(s3);
    StreamlineKey key_short = getStreamlineKey(s_short);

    EXPECT_EQ(key1, key2);
    EXPECT_NE(key1, key3);
    EXPECT_NE(key1, key_short);

    // Test with very short streamline (less than HASH_SUBSET_SIZE points)
    Streamline s_tiny = createSimpleStreamline({1,2,3});
    StreamlineKey key_tiny = getStreamlineKey(s_tiny);
    EXPECT_FALSE(key_tiny.empty());
    EXPECT_NE(key_short, key_tiny);


    // Test with 10 points (exactly 2 * HASH_SUBSET_SIZE)
    Streamline s10 = createSimpleStreamline({
        1,2,3, 4,5,6, 7,8,9, 10,11,12, 13,14,15, // 5
        16,17,18, 19,20,21, 22,23,24, 25,26,27, 28,29,30 // 10
    });
    StreamlineKey key10 = getStreamlineKey(s10);
    // Key should be based on all 10 points (5 from start, 5 from end, no middle separator)
    // Check against a key from a slightly different 10-point streamline
    Streamline s10_diff = createSimpleStreamline({
        1,2,3, 4,5,6, 7,8,9, 10,11,12, 13,14,15,
        16,17,18, 19,20,21, 22,23,24, 25,26,27, 28,29,30.1f // Difference in last point
    });
     EXPECT_NE(key10, getStreamlineKey(s10_diff));


    // Test with more than 10 points (e.g., 11 points)
    Streamline s11 = createSimpleStreamline({
        1,2,3, 4,5,6, 7,8,9, 10,11,12, 13,14,15, // P1-5
        16,17,18, // P6 (middle point)
        19,20,21, 22,23,24, 25,26,27, 28,29,30, 31,32,33 // P7-11 (these are last 5)
    });
     StreamlineKey key11 = getStreamlineKey(s11);
     // Key should have a separator like "p1_p2_p3_p4_p5_|p7_p8_p9_p10_p11"
     EXPECT_NE(key10, key11); // Different from exact 10 points
     EXPECT_TRUE(key11.find('|') != std::string::npos);
}

TEST(StreamlineOpsTest, GetStreamlineKeyPrecision) {
    Streamline s1 = createSimpleStreamline({1.1234567f, 2.1234567f, 3.1234567f});
    Streamline s2 = createSimpleStreamline({1.123f, 2.123f, 3.123f});

    StreamlineKey key1_p2 = getStreamlineKey(s1, 2); // Precision 2
    StreamlineKey key2_p2 = getStreamlineKey(s2, 2); // s2 rounded to 2 places is 1.12, 2.12, 3.12
                                                  // s1 rounded to 2 places is 1.12, 2.12, 3.12
                                                  // This test needs careful check of format_float.
                                                  // Default format_float uses setprecision(6) if precision is -1.

    // format_float(1.1234567f, 2) -> "1.12"
    // format_float(1.123f, 2)    -> "1.12"
    EXPECT_EQ(key1_p2, key2_p2); // Should be equal if rounding to 2 decimal places makes them same

    StreamlineKey key1_p3 = getStreamlineKey(s1, 3); // "1.123"
    StreamlineKey key2_p3 = getStreamlineKey(s2, 3); // "1.123"
    EXPECT_EQ(key1_p3, key2_p3);

    StreamlineKey key1_p0 = getStreamlineKey(s1, 0); // "1", "2", "3"
    Streamline s_int = createSimpleStreamline({1.0f, 2.0f, 3.0f});
    StreamlineKey key_s_int_p0 = getStreamlineKey(s_int, 0);
    EXPECT_EQ(key1_p0, key_s_int_p0);


    Streamline s_near1 = createSimpleStreamline({1.0000001f, 2.0f, 3.0f});
    Streamline s_near2 = createSimpleStreamline({1.0000002f, 2.0f, 3.0f});

    EXPECT_EQ(getStreamlineKey(s_near1, 5), getStreamlineKey(s_near2, 5)); // 1.00000 vs 1.00000
    EXPECT_NE(getStreamlineKey(s_near1, 6), getStreamlineKey(s_near2, 6)); // 1.000001 vs 1.000002 (if default precision is high enough)
    // Default precision in getStreamlineKey is -1, which format_float treats as 6.
    EXPECT_NE(getStreamlineKey(s_near1), getStreamlineKey(s_near2));
}


TEST(StreamlineOpsTest, HashStreamlines) {
    std::vector<Streamline> streamlines;
    streamlines.push_back(createSimpleStreamline({1,2,3, 4,5,6})); // idx 0
    streamlines.push_back(createSimpleStreamline({7,8,9, 10,11,12})); // idx 1
    streamlines.push_back(createSimpleStreamline({1,2,3, 4,5,6})); // Duplicate of idx 0

    HashedStreamlinesMap hashed_map = hashStreamlines(streamlines);

    ASSERT_EQ(hashed_map.size(), 2); // Only 2 unique streamlines

    StreamlineKey key0 = getStreamlineKey(streamlines[0]);
    StreamlineKey key1 = getStreamlineKey(streamlines[1]);

    EXPECT_TRUE(hashed_map.count(key0));
    EXPECT_EQ(hashed_map[key0], 0); // Should store index of first occurrence

    EXPECT_TRUE(hashed_map.count(key1));
    EXPECT_EQ(hashed_map[key1], 1);
}

TEST(StreamlineOpsTest, SetOperations) {
    Streamline s1 = createSimpleStreamline({1,2,3});
    Streamline s2 = createSimpleStreamline({4,5,6});
    Streamline s3 = createSimpleStreamline({7,8,9});
    Streamline s4 = createSimpleStreamline({1,2,3}); // Same as s1

    HashedStreamlinesMap map1, map2;
    map1[getStreamlineKey(s1)] = 0; // Key for s1, original index 0 in its list
    map1[getStreamlineKey(s2)] = 1; // Key for s2, original index 1 in its list

    map2[getStreamlineKey(s4)] = 0; // Key for s4 (same as s1), original index 0 in its list
    map2[getStreamlineKey(s3)] = 1; // Key for s3, original index 1 in its list

    // Intersection
    HashedStreamlinesMap res_intersect = intersection(map1, map2);
    ASSERT_EQ(res_intersect.size(), 1);
    EXPECT_TRUE(res_intersect.count(getStreamlineKey(s1)));
    EXPECT_EQ(res_intersect[getStreamlineKey(s1)], 0); // Index from map1

    // Difference (map1 - map2)
    HashedStreamlinesMap res_diff = difference(map1, map2);
    ASSERT_EQ(res_diff.size(), 1);
    EXPECT_TRUE(res_diff.count(getStreamlineKey(s2)));
    EXPECT_EQ(res_diff[getStreamlineKey(s2)], 1); // Index from map1

    // Union
    HashedStreamlinesMap res_union = union_set(map1, map2);
    ASSERT_EQ(res_union.size(), 3);
    EXPECT_TRUE(res_union.count(getStreamlineKey(s1))); // from map1
    EXPECT_EQ(res_union[getStreamlineKey(s1)], 0);      // index from map1
    EXPECT_TRUE(res_union.count(getStreamlineKey(s2))); // from map1
    EXPECT_EQ(res_union[getStreamlineKey(s2)], 1);      // index from map1
    EXPECT_TRUE(res_union.count(getStreamlineKey(s3))); // from map2
    EXPECT_EQ(res_union[getStreamlineKey(s3)], 1);      // index from map2
}

// Basic test for performStreamlinesOperation with Tractogram objects
TEST(StreamlineOpsTest, PerformTractogramOperationUnion) {
    Tractogram t1, t2;
    Streamline s1 = createSimpleStreamline({1,2,3}); // Key K1
    Streamline s2 = createSimpleStreamline({4,5,6}); // Key K2
    Streamline s3 = createSimpleStreamline({7,8,9}); // Key K3

    t1.addStreamline(s1); // t1 has K1 (idx 0)
    t1.addStreamline(s2); // t1 has K2 (idx 1)
    t1.addMetadata("NB_STREAMLINES", "2");
    t1.addMetadata("NB_VERTICES", std::to_string(s1.getPoints().size() + s2.getPoints().size()));


    t2.addStreamline(s2); // t2 has K2 (idx 0) - duplicate of s2 in t1
    t2.addStreamline(s3); // t2 has K3 (idx 1)
    t2.addMetadata("NB_STREAMLINES", "2");
    t2.addMetadata("NB_VERTICES", std::to_string(s2.getPoints().size() + s3.getPoints().size()));


    std::vector<const Tractogram*> tractograms_to_op = {&t1, &t2};
    Tractogram result_tractogram = performStreamlinesOperation(OperationType::UNION, tractograms_to_op);

    ASSERT_EQ(result_tractogram.getStreamlines().size(), 3);
    // Check if K1, K2, K3 are present. Order might vary due to unordered_map in hashing.
    // We need to hash the resulting streamlines to check.
    HashedStreamlinesMap result_hashes = hashStreamlines(result_tractogram.getStreamlines());
    EXPECT_TRUE(result_hashes.count(getStreamlineKey(s1)));
    EXPECT_TRUE(result_hashes.count(getStreamlineKey(s2)));
    EXPECT_TRUE(result_hashes.count(getStreamlineKey(s3)));

    EXPECT_EQ(std::stoll(result_tractogram.getMetadata().at("NB_STREAMLINES")), 3);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
