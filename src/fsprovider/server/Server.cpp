#include "./Server.h"
#include <fstream>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>
#include <boost/filesystem.hpp>

namespace MediaFs {

    Server :: Server(int openHandlesSize) : openHandlesSize(openHandlesSize), openHandles(LRUCache<std::string, FileCache*>(openHandlesSize)) {
    }

    const char* Server :: read(std::string path, int &length, int offset) {
        path = path[0] == '/' ? path.substr(1) : path;
        std::cout << "read " << path << " length=" << length << " offset=" << offset << "\n";
        struct stat sb;
        if (stat(path.c_str(), &sb) != 0) {
            char *resp = new char[10];
            strcpy(resp, "not found");
            length = strlen(resp);
            return resp;
        }
        if (!openHandles.has(path)) {
            FileCache* cache = new FileCache(std::unique_ptr<FileHandle>(new IfstreamFileHandle(std::ifstream(path, std::ios::in | std::ios::binary))));
            openHandles.add(path, cache);
            addedCaches.push_back(cache);
        }
        FileCache &handle = *openHandles[path];
        auto data = handle[std::make_pair(offset, offset + length)];
        length = std::get<1>(data);
        return std::get<0>(data);
    }

    Server :: ~Server() {
        std::cout << "~Server\n";
        for(FileCache *cache : addedCaches) {
            delete cache;
        }
    }

    std::vector<Attr> Server :: readDir(std::string path) {
        path = path[0] == '/' ? path.substr(1) : path;
        std::vector<Attr> contents;
        boost::filesystem::directory_iterator it;
        boost::filesystem::path fPath(path);
        for (boost::filesystem::directory_iterator it0(path); it0 != it; it0++) {
            contents.push_back(getAttr(it0->path().string()));
        }
        return contents;
    }

    Attr Server :: getAttr(std::string path) {
        path = path[0] == '/' ? path.substr(1) : path;
        Attr attr;
        attr.name = path;
        attr.size = boost::filesystem::file_size(path);
        attr.supportedType = boost::filesystem::is_directory(path) ? SupportedType::REGULAR_DIR : SupportedType::REGULAR_FILE;
        return attr;
    }

}

