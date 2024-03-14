#include "./Server.h"
#include <list>
#include <fstream>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>
#include <boost/filesystem.hpp>

namespace MediaFs {

    InvalidItemException :: InvalidItemException(std::string message) : message(message) { }

    Server :: Server(int openHandlesSize) : openHandlesSize(openHandlesSize), openHandles(LRUCache<std::string, FileCache*>(openHandlesSize)) {
    }

    const char* Server :: read(std::string path, int &length, int offset) {
        std::cout << "read " << path << " length=" << length << " offset=" << offset << "\n";
        struct stat sb;
        if (stat(path.c_str(), &sb) != 0) {
            char *resp = new char[10];
            strcpy(resp, "not found");
            length = strlen(resp);
            return resp;
        }
        if (!openHandles.has(path)) {
            FileCache* cache = new FileCache(std::unique_ptr<std::ifstream>(new std::ifstream(path, std::ios::in | std::ios::binary)));
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

    FileCache :: FileCache(std::unique_ptr<std::ifstream> &&fileHandle) : fileHandle(std::move(fileHandle)) { }

    void FileCache :: refreshRanges() {
        handleMutex.lock();
        std::list<std::pair<int, std::unique_ptr<Buffer> > > ranges;
        for (auto buffer = buffers.begin(); buffer != buffers.end(); buffer++) {
            std::pair<int, std::unique_ptr<Buffer> > range(buffer->first, std::move(buffer->second));
            ranges.push_back(std::move(range));
        }
        ranges.sort([] (const std::pair<int, std::unique_ptr<Buffer> > &first, const std::pair<int, std::unique_ptr<Buffer> > &second) {
                return first.first < second.first;
                });

        int prevFrom = -1, prevTo = -1;
        std::pair<int, std::unique_ptr<Buffer> > *prevRange = NULL;
        std::list<std::pair<int, std::unique_ptr<Buffer> > > combined;
        for (auto &range : ranges) {
            if (prevRange == NULL) {
                prevRange = &range;
                continue;
            }
            int lb = range.first, ub = range.first + range.second->size;
            int prevFrom = prevRange->first, prevTo = prevRange->first + prevRange->second->size;
            if ((prevFrom >= lb && prevFrom < ub) && (prevTo >= lb && prevTo < ub)) {
                delete prevRange;
                prevRange = NULL;
            } else if ((lb >= prevFrom && lb < prevTo) && (ub >= prevFrom && ub < prevTo)) {
                range.second.release();
            } else if ((lb >= prevFrom && lb < (prevTo + 1)) || (ub >= prevFrom && ub < prevTo)) {
                prevRange = MediaFs::combineRanges(*prevRange, range);
            } else {
                combined.push_back(std::move(*prevRange));
                prevRange = &range;
            }
        }

        if (prevRange != NULL) {
            combined.push_back(std::move(*prevRange));
        }

        buffers.erase(buffers.begin(), buffers.end());

        for (auto &item : combined) {
            buffers[item.first] = std::move(item.second);
        }

        std::cout << "refreshed ranges\n";
        std::cout << *this;

        handleMutex.unlock();
    }

    std::tuple<const char*, int> FileCache :: operator[](std::pair<int, int> range) {
        handleMutex.lock();
        short size = range.second - range.first;
        for (auto &buffer : buffers) {
            if ((range.first >= buffer.first) && (range.second <= (buffer.first + buffer.second->size))) {
                handleMutex.unlock();
                return {buffer.second->data + (range.first - buffer.first), size};
            }
        }
        fileHandle->seekg(range.first);
        char *data = new char[size];
        fileHandle->read(data, size);
        buffers[range.first] = std::make_unique<Buffer>();
        buffers[range.first]->data = data;
        buffers[range.first]->size = fileHandle->gcount();
        handleMutex.unlock();
        std::thread rangeRefresher(FileCache::refreshRangesInBackground, this);
        rangeRefresher.detach();
        return {buffers[range.first]->data, buffers[range.first]->size};
    }

    void FileCache :: refreshRangesInBackground(FileCache *fileCache) {
        fileCache->refreshRanges();
    }

    std::pair<int, std::unique_ptr<Buffer>>* combineRanges(std::pair<int, std::unique_ptr<Buffer> > &firstRange, std::pair<int, std::unique_ptr<Buffer> > &secondRange) {
        Buffer *buffer = new Buffer;
        if (firstRange.first > secondRange.first) {
            auto temp = std::move(firstRange);
            firstRange = std::move(secondRange);
            secondRange = std::move(temp);
        }
        int lb1 = firstRange.first, ub1 = firstRange.first + firstRange.second->size;
        int lb2 = secondRange.first, ub2 = secondRange.first + secondRange.second->size;
        buffer->data = new char[ub2 - lb1];
        memcpy(buffer->data, firstRange.second->data, firstRange.second->size);
        if (lb2 == (ub1 + 1)) {
            memcpy(buffer->data + firstRange.second->size, secondRange.second->data, secondRange.second->size);
        } else {
            memcpy(buffer->data + firstRange.second->size, secondRange.second->data + (ub1 - lb2),ub2 - ub1);
        }
        buffer->size = ub2 - lb1;
        Buffer *firstBuffer = firstRange.second.release(),
               *secondBuffer = secondRange.second.release();
        delete firstBuffer;
        delete secondBuffer;
        return new std::pair<int, std::unique_ptr<Buffer> >(lb1, std::unique_ptr<Buffer>(buffer));
    }

    std::vector<Attr> Server :: readDir(std::string path) const {
        std::vector<Attr> contents;
        boost::filesystem::directory_iterator it;
        boost::filesystem::path fPath(path);
        for (boost::filesystem::directory_iterator it0(path); it0 != it; it0++) {
            contents.push_back(getAttr(it0->path().string()));
        }
        return contents;
    }

    Attr Server :: getAttr(std::string path) const {
        Attr attr;
        attr.name = path;
        attr.size = boost::filesystem::file_size(path);
        attr.supportedType = boost::filesystem::is_directory(path) ? SupportedType::REGULAR_DIR : SupportedType::REGULAR_FILE;
        return attr;
    }

    std::ostream& operator<<(std::ostream &out, const MediaFs::Buffer &buffer) {
        out << "Buffer:\n" << buffer.data << " size : " << buffer.size << "\n";
        return out;
    }

    std::ostream& operator<<(std::ostream &out, const MediaFs::FileCache &cache) {
        for(const auto &buffer: cache.buffers) {
            out << buffer.first << " : " << *buffer.second.get();
        }
        return out;
    }
}

