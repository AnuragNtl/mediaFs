cmake_minimum_required(VERSION 3.10)

project(MediaFs VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_BUILD_TYPE Debug)

find_package(PkgConfig REQUIRED)

pkg_check_modules(FUSE "fuse")

add_executable(MediaFs src/MediaFs.cpp src/transfer/server/MetadataServer.cpp src/fsprovider/server/Server.cpp src/Utils.cpp src/fsprovider/client/Client.cpp)

add_subdirectory(googletest)


target_link_libraries(MediaFs pthread boost_system boost_filesystem)
target_link_libraries(MediaFs ${FUSE_LIBRARIES})
add_definitions(${FUSE_CFLAGS} ${FUSE_CFLAGS_OTHER})

enable_testing()

add_executable(RangeTest src/test/RangeTest.cpp src/fsprovider/server/Server.cpp)
add_executable(LRUCacheTest src/test/LRUCacheTest.cpp src/fsprovider/server/Server.cpp)
add_executable(MetadataServerTest src/test/MetadataServerTest.cpp src/transfer/server/MetadataServer.cpp src/Utils.cpp)
add_executable(ClientBufTest src/fsprovider/client/Client.cpp src/test/ClientBufTest.cpp src/transfer/server/MetadataServer.cpp src/Utils.cpp)
target_link_libraries(RangeTest GTest::gtest_main boost_filesystem)
target_link_libraries(LRUCacheTest GTest::gtest_main boost_filesystem)
target_link_libraries(MetadataServerTest GTest::gtest_main)
target_link_libraries(ClientBufTest GTest::gtest_main boost_system boost_filesystem)

include(GoogleTest)
gtest_discover_tests(RangeTest)
gtest_discover_tests(LRUCacheTest)
gtest_discover_tests(MetadataServerTest)
gtest_discover_tests(ClientBufTest)

