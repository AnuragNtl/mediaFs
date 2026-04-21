#include "gtest/gtest.h"

#include "../Utils.h"

TEST(B64, B64Encode) {
    std::string encoded = MediaFs::base64Encode("abcd");
    ASSERT_EQ("YWJjZA==", encoded);
}

TEST(B64, B64Decode) {
    std::string decoded = MediaFs::base64Decode("YWJjZA==");
    ASSERT_EQ("abcd", decoded);
}

int main(int argc, char *argv[]) {

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
