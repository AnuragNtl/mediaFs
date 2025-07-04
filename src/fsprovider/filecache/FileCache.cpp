#include "FileCache.h"

#include <unistd.h>
#include <iostream>
#include <memory>
#include <sys/stat.h>
#include <cstring>

namespace MediaFs {
    InvalidItemException :: InvalidItemException(std::string message) : message(message) { }

    IfstreamFileHandle :: IfstreamFileHandle(std::ifstream &&in) : in(std::move(in)) { }

    int IfstreamFileHandle :: read(char *buf, int start, int size) {
        in.seekg(start);
        in.read(buf, size);
        return in.gcount();
    }

    IfstreamFileHandle :: ~IfstreamFileHandle() {
        in.close();
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
        char *data = new char[size];
        int gcount = fileHandle->read(data, range.first, size);
        buffers[range.first] = std::make_unique<Buffer>();
        buffers[range.first]->data = data;
        buffers[range.first]->size = gcount;
        handleMutex.unlock();
        std::thread rangeRefresher(FileCache::refreshRangesInBackground, this);
        rangeRefresher.detach();
        return {buffers[range.first]->data, buffers[range.first]->size};
    }

    void FileCache :: refreshRangesInBackground(FileCache *fileCache) {
        fileCache->refreshRanges();
    }

    FileCache :: FileCache(std::unique_ptr<FileHandle> &&fileHandle) : fileHandle(std::move(fileHandle)) { }

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
};
