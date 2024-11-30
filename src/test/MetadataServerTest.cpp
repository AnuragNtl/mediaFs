#include "../transfer/server/MetadataServer.h"
#include "gtest/gtest.h"

TEST(MetadataServerTest, MetadataServer) {
    auto generated = MediaFs::MediaPacketParser::generate('r', {"param1", "param2"});
    ASSERT_EQ("r param1 param2\n", generated);
}

int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


