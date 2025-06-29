#ifndef SERVER_H

#define SERVER_H

#include <iostream>
#include <map>
#include <functional>
#include <memory>
#include <list>
#include <mutex>
#include <tuple>
#include <thread>

#include "../FSProvider.h"
#include "../filecache/FileCache.h"

#define DEFAULT_OPEN_HANDLES_SIZE 10


namespace MediaFs {

    class Server : public FSProvider {
        private:
            LRUCache<std::string, FileCache*> openHandles;
            std::vector<FileCache*> addedCaches;
            int openHandlesSize;
        public:
            Server(int openHandlesSize = DEFAULT_OPEN_HANDLES_SIZE);
            Server(const Server &) = delete;
            const char* read(std::string path, int &, int);
            std::vector<Attr> readDir(std::string path);
            Attr getAttr(std::string path);
            virtual ~Server();

    };
};
#endif

