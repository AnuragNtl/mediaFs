#include <cstring>
#include <gtest/gtest.h>
#include <memory>
#include <utility>
#include "../fsprovider/server/Server.h"

class RangeTest : public testing::TestWithParam<std::pair<std::string, std::string> > {
    public:
        void SetUp();
};

void RangeTest :: SetUp() {

}

TEST_P(RangeTest, Range) {
    MediaFs::Buffer *buf1 = new MediaFs::Buffer,
        *buf2 = new MediaFs::Buffer;
    buf1->data = new char[5];
    buf2->data = new char[5];
    strcpy(buf1->data, "1234");
    strcpy(buf2->data, "3456");
    buf1->size = 4;
    buf2->size = 5;
    std::unique_ptr<MediaFs::Buffer> ubuf1(buf1);
    std::unique_ptr<MediaFs::Buffer> ubuf2(buf2);
    std::pair<int, std::unique_ptr<MediaFs::Buffer> > pair1(0, std::move(ubuf1)),
    pair2(2, std::move(ubuf2));
    auto combined = MediaFs::combineRanges(pair1, pair2);
    std::cout << combined->first << "\n";
    std::cout << combined->second->data << "\n";
    std::cout << combined->second->size << "\n";
    ASSERT_TRUE(memcmp("123456", combined->second->data, 7 * sizeof(char)) == 0);

}

INSTANTIATE_TEST_SUITE_P(RangeSuite, RangeTest, testing::Values(std::make_pair(std::string("1234"), std::string("3456")),
            std::make_pair(std::string("1234567890"), std::string("7890"))));

int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


