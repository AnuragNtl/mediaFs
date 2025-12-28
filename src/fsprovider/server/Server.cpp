#include "./Server.h"
#include <boost/filesystem/operations.hpp>
#include <fstream>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>
#include <boost/filesystem.hpp>

#include "../../Utils.h"

namespace MediaFs {

    Server :: Server(int openHandlesSize) : openHandlesSize(openHandlesSize), openHandles(LRUCache<std::string, FileCache*>(openHandlesSize)) {
    }

    const char* Server :: read(std::string path, int &length, int offset) {
        path = path[0] == '/' ? path.substr(1) : path;
        struct stat sb;
        if (stat(path.c_str(), &sb) != 0) {
            char *resp = new char[10];
            strcpy(resp, "not found");
            length = strlen(resp);
            return resp;
        }
        if (!openHandles.has(path)) {
            FileCache* cache = new FileCache(std::unique_ptr<FileHandle>(new IfstreamFileHandle(std::move(*(new std::ifstream(path, std::ios::in | std::ios::binary))))));
            openHandles.add(path, cache);
            addedCaches.push_back(cache);
        }
        FileCache &handle = *openHandles[path];
        auto data = handle[std::make_pair(offset, offset + length)];
        length = std::get<1>(data);
        const char *contents = std::get<0>(data);
        return contents;
    }

    Server :: ~Server() {
        for(FileCache *cache : addedCaches) {
            delete cache;
        }
    }

    std::vector<Attr> Server :: readDir(std::string path) {
        if (path == "/") {
            path = ".";
        }
        path = path[0] == '/' ? path.substr(1, path.size() - 1) : path;
        std::vector<Attr> contents;
        boost::filesystem::directory_iterator it;
        boost::filesystem::path fPath(path);
        for (boost::filesystem::directory_iterator it0(path); it0 != it; it0++) {
            std::string path = it0->path().string();
            if (path[0] == '/') {
                path = path.substr(1, path.size() - 1);
            }
            std::cout << "Path " << path << "\n";
            Attr attr = getAttr(path);
            std::cout << "attr name " << attr.name << "\n";
            attr.name = boost::filesystem::basename(path);
            std::string ext = boost::filesystem::extension(path);
            attr.name += ext;
            std::cout << "attr name " << attr.name << "\n";
            contents.push_back(attr);
        }
        return contents;
    }

    Attr Server :: getAttr(std::string path) {
        if (path == "/") {
            path = ".";
        } else {
            path = path[0] == '/' ? path.substr(1, path.size() - 1) : path;
        }
        Attr attr;
        attr.name = path;
        if (!boost::filesystem::exists(path)) {
            throw std::exception();
        }
        if (!boost::filesystem::is_directory(path)) {
            attr.size = boost::filesystem::file_size(path);
        } else {
            attr.size = 4096;
        }
        attr.supportedType = boost::filesystem::is_directory(path) ? SupportedType::REGULAR_DIR : SupportedType::REGULAR_FILE;
        return attr;
    }

    void Server :: updateCaches() {
        for (FileCache *cache : addedCaches) {
            cache->refreshRangesAsync();
        }
    }

}

