#ifndef FS_PROVIDER_H

#define FS_PROVIDER_H

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
            virtual int read(std::string path, char *buffer, int size, int offset) const  {};
            virtual std::vector<Attr> readDir(std::string path) const  {};
            virtual Attr getAttr(std::string path) const  {};
    };

};

#endif

