#include <cstring>
#include <unistd.h>
#include <iostream>
#include <functional>

#include "FuseWrapper.h"


int readDir(const char *path, void *buffer, fuse_fill_dir_t filler,
        off_t offset,
        struct fuse_file_info *fi) {
    std::cout << "Path readDir " << path << "\n";
    filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);
    filler(buffer, "s2", NULL, 0);
    return 0;
}

int read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {

    std::cout << "Path read " << path << "\n";
    strcpy(buffer, "abcdef");
    return 10;
}

int open(const char *path, struct fuse_file_info *fi) {
    return 0;
}

int getAttr(const char *path, struct stat *st) {

    std::cout << "Path getAttr " << path << "\n";
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
        st->st_size = 20;
    }
    return 0;
}


int ioctlHandler(const char *, int, void *, struct fuse_file_info *info, unsigned int, void*) {
    std::cout << "ioctl \n";
    return 0;
}

namespace MediaFs {

    struct fuse_operations* getRegistered() {
        struct fuse_operations *o = new struct fuse_operations;
        o->readdir = readDir;
        o->ioctl = ioctlHandler;
        o->read = read;
        o->getattr = getAttr;
        return o;
    }
};


