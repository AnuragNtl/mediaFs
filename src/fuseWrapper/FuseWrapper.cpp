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

    filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);
    for(const auto &item : contents) {
        std::cout << "readdir item" << item.name << "\n";
        struct stat stBuf;
        setAttr(item, &stBuf);
        filler(buffer, item.name.c_str(), NULL, 0);
    }

    return 0;
}

int read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {

    int sz = size;
    const char *data = MediaFs::client->read(path, sz, offset);
    memcpy(buffer, data, sz);
    MediaFs::CacheableFSProvider *cacheableProvider = dynamic_cast<MediaFs::CacheableFSProvider*>(MediaFs::client);
    if (cacheableProvider != NULL) {
        cacheableProvider->updateCaches();
    }
    return sz;
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
    return -ENOSYS;
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


