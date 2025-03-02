#include "../fsprovider/client/Client.h"
#include "gtest/gtest.h"

//void assertBuf(int, int, bool, MediaFs::ClientBuf &);

#define assertBuf(totalLength,_size,contentReady,buf) \
    ASSERT_EQ(totalLength, buf.getTotalPendingLength()); ASSERT_EQ(contentReady, buf.isContentReady()); ASSERT_EQ(_size, buf.size());

TEST(ClientBufTest, ClientSingleBuf) {
    MediaFs::ClientBuf buf;
    char *src = new char[6];
    memcpy(src, "4,abcd", 6 * sizeof(char));
    buf.add(src, 6);
    char *data = new char[6];
    ASSERT_EQ(4, buf.read(data, 6));
    ASSERT_TRUE(memcmp(data, "abcd", 4 * sizeof(char)) == 0);
    delete[] data;
}

const char* getBuf(const char *data) {
    char *buf = new char[strlen(data)];
    strcpy(buf, data);
    return buf;
}

TEST(ClientBufTest, ClientMultiBuf) {
    MediaFs::ClientBuf buf;
    assertBuf(0, 0, 0, buf);
    buf.add(getBuf("0"), 1);
    assertBuf(1, 0, false, buf);
    buf.add(getBuf("0"), 1);
    assertBuf(2, 0, false, buf);
    buf.add(getBuf("1"), 1);
    assertBuf(3, 0, false, buf);
    buf.add(getBuf("0,1234567890"), 12);
    assertBuf(0, 1, true, buf);
    ASSERT_EQ(10, buf.getReadyLength());
    char *data = new char[10];
    buf.read(data, 10);
    ASSERT_TRUE(memcmp("1234567890", data, 10 * sizeof(char)) == 0);
    assertBuf(0, 0, 0, buf);
    ASSERT_EQ(0, buf.getReadyLength());
    delete[] data;
}

TEST(ClientBufTest, ClientMultiBufAdd) {
    MediaFs::ClientBuf buf;
    buf.add(getBuf("10,1234567890"), 13);
    assertBuf(0, 1, true, buf);
    buf.add(getBuf("2,ab"), 4);
    assertBuf(0, 2, true, buf);
    ASSERT_EQ(12, buf.getReadyLength());
    char *data = new char[20];
    buf.read(data, 10);
    ASSERT_TRUE(memcmp("1234567890", data, 10 * sizeof(char)) == 0);
    ASSERT_EQ(2, buf.getReadyLength());
    buf.read(data, 2);
    ASSERT_EQ(0, buf.getReadyLength());
    ASSERT_TRUE(memcmp("ab", data, 2 * sizeof(char)) == 0);
    delete[] data;
}

int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

