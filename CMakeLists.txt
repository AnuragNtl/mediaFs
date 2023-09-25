cmake_minimum_required(VERSION 3.10)

project(MediaFs VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_BUILD_TYPE Debug)

find_package(PkgConfig REQUIRED)

pkg_check_modules(FUSE "fuse")

add_executable(MediaFs src/MediaFs.cpp src/transfer/server/MetadataServer.cpp src/Utils.cpp)

target_link_libraries(MediaFs pthread boost_system)
target_link_libraries(MediaFs ${FUSE_LIBRARIES})
add_definitions(${FUSE_CFLAGS} ${FUSE_CFLAGS_OTHER})

