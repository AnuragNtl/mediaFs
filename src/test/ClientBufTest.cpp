#include "../fsprovider/client/Client.h"
#include "gtest/gtest.h"

void assertBuf(int, int, bool, MediaFs::ClientBuf &);

TEST(ClientBufTest, ClientSingleBuf) {
    MediaFs::ClientBuf buf;
    buf.add("4,abcd", 6);
    char *data = new char[6];
    ASSERT_EQ(4, buf.read(data, 6));
    ASSERT_TRUE(memcmp(data, "abcd", 4 * sizeof(char)) == 0);
}

TEST(ClientBufTest, ClientMultiBuf) {
    MediaFs::ClientBuf buf;
    assertBuf(0, 0, 0, buf);
    buf.add("0", 1);
    assertBuf(0, 0, false, buf);
    buf.add("0", 1);
    assertBuf(0, 0, false, buf);
    buf.add("1", 1);
    assertBuf(0, 0, false, buf);
    buf.add("0,1234567890", 12);
    assertBuf(10, 1, true, buf);
    char *data = new char[10];
    buf.read(data, 10);
    ASSERT_TRUE(memcmp("1234567890", data, 10 * sizeof(char)) == 0);
}

int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

void assertBuf(int totalLength, int size, bool contentReady, MediaFs::ClientBuf &buf) {
    ASSERT_EQ(totalLength, buf.getTotalLength());
    ASSERT_EQ(contentReady, buf.isContentReady());
    ASSERT_EQ(size, buf.size());
}
