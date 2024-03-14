#ifndef CLIENT_H
#define CLIENT_H

#include <vector>
#include "../FSProvider.h"

namespace MediaFs {
    class Client  : public FSProvider {
        private:
            void sendRequest(std::vector<std::string>);
        public:
            const char* read(std::string path, int &size, int offset);
            std::vector<Attr> read(std::string path) const;
            Attr getAttr(std::string path) const;
    };
};

#endif

