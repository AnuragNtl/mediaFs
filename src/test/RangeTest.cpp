#include <cstring>
#include <gtest/gtest.h>
#include <memory>
#include <tuple>
#include <utility>
#include <fstream>
#include "../fsprovider/server/Server.h"

struct RangeParam {
    char *firstBuffer, *secondBuffer;
    int firstPos, secondPos;
    char *expected;
};

class RangeTest : public testing::TestWithParam<RangeParam> {
    public:
        void SetUp();
};

void RangeTest :: SetUp() {

}

TEST_P(RangeTest, Range) {
    RangeParam param = GetParam();
    MediaFs::Buffer *buf1 = new MediaFs::Buffer,
        *buf2 = new MediaFs::Buffer;
    buf1->data = new char[strlen(param.firstBuffer)];
    buf2->data = new char[strlen(param.secondBuffer)];
    memcpy(buf1->data, param.firstBuffer, strlen(param.firstBuffer));
    memcpy(buf2->data, param.secondBuffer, strlen(param.secondBuffer));
    buf1->size = strlen(param.firstBuffer);
    buf2->size = strlen(param.secondBuffer);
    std::unique_ptr<MediaFs::Buffer> ubuf1(buf1);
    std::unique_ptr<MediaFs::Buffer> ubuf2(buf2);
    std::pair<int, std::unique_ptr<MediaFs::Buffer> > pair1(param.firstPos, std::move(ubuf1)),
    pair2(param.secondPos, std::move(ubuf2));
    auto combined = MediaFs::combineRanges(pair1, pair2);
    ASSERT_TRUE(memcmp(param.expected, combined->second->data, strlen(param.expected) * sizeof(char)) == 0);

}

class FileCacheFixture : public testing::Test {
    public:
        MediaFs::FileCache *fileCache;
        void SetUp();
};

void FileCacheFixture :: SetUp() {
    fileCache = new MediaFs::FileCache(std::make_unique<std::ifstream>());
    fileCache->buffers[0] = std::unique_ptr<MediaFs::Buffer>(new MediaFs::Buffer("1234", 4));
    fileCache->buffers[4] = std::unique_ptr<MediaFs::Buffer>(new MediaFs::Buffer("5678", 4));
    fileCache->buffers[8] = std::unique_ptr<MediaFs::Buffer>(new MediaFs::Buffer("5678", 4));
    fileCache->buffers[16] = std::unique_ptr<MediaFs::Buffer>(new MediaFs::Buffer("7890123456", 10));
    fileCache->buffers[21] = std::unique_ptr<MediaFs::Buffer>(new MediaFs::Buffer("3456", 4));
}

INSTANTIATE_TEST_SUITE_P(RangeSuite, RangeTest, testing::Values(
            RangeParam { (char *)"1234", (char *)"3456", 0, 2, (char*)"123456"},
            RangeParam { (char *)"1234", (char *)"4567", 0, 3, (char*)"1234567"},
            RangeParam { (char *)"4567", (char *)"1234", 3, 0, (char*)"1234567"},
            RangeParam { (char *)"1234567890", (char*)"7890", 2, 8, (char *)"1234567890" },
            RangeParam { (char *)"1234567890", (char*)"3456789012", 0, 2, (char *)"123456789012" },
            RangeParam { (char *)"1234", (char*)"123456", 0, 0, (char *)"123456"},
            RangeParam { (char *)"123456", (char*)"3456", 0, 2, (char *)"123456"},
            RangeParam { (char *)"1234", (char*)"0012", 2, 0, (char *)"001234"}
            ));

TEST_F(FileCacheFixture, FileCache) {
    fileCache->refreshRanges();
    ASSERT_EQ(2, fileCache->buffers.size());
    ASSERT_EQ(12, fileCache->buffers[0]->size);
    ASSERT_EQ(10, fileCache->buffers[16]->size);
    ASSERT_EQ(0, memcmp("123456785678", fileCache->buffers[0]->data, 12 * sizeof(char)));
    ASSERT_EQ(0, memcmp("7890123456", fileCache->buffers[16]->data, 10 * sizeof(char)));
}

int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

