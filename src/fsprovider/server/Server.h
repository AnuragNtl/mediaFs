#ifndef SERVER_H

#define SERVER_H

#include "../FSProvider.h"

namespace MediaFs {
    class Server : public FSProvider {
        public:
            int read(std::string path, char *buffer, int size, int offset) const;
            std::vector<Attr> readDir(std::string path) const;
            Attr getAttr(std::string path) const;

    };
};

#endif

