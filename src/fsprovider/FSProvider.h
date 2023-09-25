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
            virtual int read(std::string path, char *buffer, int size, int offset) const  {
                strcpy(buffer, "1234abcd\n");
                return 5;
            }
            virtual std::vector<Attr> readDir(std::string path) const  {
                Attr attr;
                attr.name = "abcd";
                attr.size = 10;
                attr.supportedType = SupportedType::REGULAR_DIR;
                return {attr};
            }
            virtual Attr getAttr(std::string path) const  {
                Attr attr;
                attr.name = "abcd";
                attr.size = 10;
                attr.supportedType = SupportedType::REGULAR_DIR;
                return attr;
            };
    };

};

#endif

