#ifndef FS_PROVIDER_H

#define FS_PROVIDER_H

#include <vector>

#include <string>
#include <cstring>

namespace MediaFs {

    enum SupportedType {
        REGULAR_FILE, REGULAR_DIR
    };

    struct Attr {
        SupportedType supportedType;
        std::string name;
        long size;
    };


    class FSProvider {
        public:
            virtual const char* read(std::string path, int &size, int offset) = 0;
            virtual std::vector<Attr> readDir(std::string path) const = 0;
            virtual Attr getAttr(std::string path) const = 0;;
            virtual ~FSProvider() { }
    };

};

#endif

