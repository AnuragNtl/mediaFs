#include "../fsprovider/server/Server.h"
#include "gtest/gtest.h"

TEST(LRUTest, LRU) {
    MediaFs::LRUCache<int, int> cache(3);
    cache.add(0, 1);
    cache.add(1, 2);
    cache.add(2, 3);
    ASSERT_TRUE(cache.has(0));
    ASSERT_TRUE(cache.has(2));
    ASSERT_TRUE(cache.has(1));
    ASSERT_FALSE(cache.has(3));
    cache[0];
    cache.add(3, 4);
    ASSERT_FALSE(cache.has(1));
    ASSERT_EQ(1, cache[0]);
    ASSERT_EQ(3, cache[2]);
    ASSERT_EQ(4, cache[3]);
    cache.add(4, 0);
    ASSERT_FALSE(cache.has(0));
    ASSERT_TRUE(cache.has(4));
}

int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

