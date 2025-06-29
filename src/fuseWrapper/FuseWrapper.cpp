#include <cstring>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <functional>

#include "FuseWrapper.h"

int readDir(const char *path, void *buffer, fuse_fill_dir_t filler,
        off_t offset,
        struct fuse_file_info *fi) {
    int size;
    std::vector<MediaFs::Attr> contents = MediaFs::client->readDir(std::string(path));

    for(const auto &item : contents) {
        struct stat stBuf;
        setAttr(item, &stBuf);
        filler(buffer, item.name.c_str(), &stBuf, 0);
    }

    std::cout << "Path readDir " << path << "\n";
    /*filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);
    filler(buffer, "s2.mp4", NULL, 0);*/
    return 0;
}

int read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {

    int sz = size;
    MediaFs::client->read(path, sz, offset);
    return sz;
    ////////
    std::cout << "Path read " << path << "\n";
    if (offset > 788609574) return -1;
    std::ifstream in("/home/administrator/My Darling Clementine (1946)/My.Darling.Clementine.1946.720p.BluRay.x264.YIFY.mp4", std::ios::binary | std::ios::in);
    //std::ifstream in("/home/administrator/package.json", std::ios::binary | std::ios::in);
    in.seekg(offset, std::ios::beg);
    in.read(buffer, size);
    std::cout << "read " << size << " bytes with offset " << offset << "\n";
    int count = in.gcount();
    /*if (in.eof()) {
        count = -1;
    }*/
    std::cout << "size read " << count << "\n";
    in.close();
    return count;
    //strcpy(buffer, "abcdef");
    //return 10;
}

int open(const char *path, struct fuse_file_info *fi) {
    return 0;
}

void setAttr(MediaFs::Attr attr, struct stat *st) {
    st->st_uid = getuid();
    st->st_gid = getgid();
    st->st_atime = time(NULL);
    st->st_mtime = time(NULL);
    st->st_nlink = 1;
    st->st_size = attr.size;
    if (attr.supportedType == MediaFs::SupportedType::REGULAR_DIR) {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
    } else {
        st->st_mode = S_IFREG | 0644;
    }
}

int getAttr(const char *path, struct stat *st) {

    std::cout << "Path getAttr " << path << "\n";
    try {
        setAttr(MediaFs::client->getAttr(path), st);
    } catch (std::exception &e) {
        return -1;
    }
    return 0;

    st->st_uid = geteuid();
    st->st_gid = getgid();
    st->st_atime = time(NULL);
    st->st_mtime = time(NULL);
    st->st_nlink = 1;
    if (std::string(path) == "/") {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
    } else {
        st->st_mode = S_IFREG | 0644;
        st->st_size = 788609574;
    }
    return 0;
}


int ioctlHandler(const char *, int, void *, struct fuse_file_info *info, unsigned int, void*) {
    std::cout << "ioctl \n";
    return 0;
}

namespace MediaFs {

    Client *client = NULL;

    struct fuse_operations* getRegistered() {
        MediaFs::client = new MediaFs::Client();
        struct fuse_operations *o = new struct fuse_operations;
        o->readdir = readDir;
        o->ioctl = ioctlHandler;
        o->read = read;
        o->getattr = getAttr;
        return o;
    }
};


