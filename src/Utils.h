#ifndef UTILS_H

#define UTILS_H

#include <ostream>
#include <string>
#include <vector>

#include "./fsprovider/FSProvider.h"

#define LEN_SEPARATOR ","

namespace MediaFs {

    std::vector<std::string> split(std::string, std::string delim = " ");
    std::ostream& operator<<(std::ostream &out, Attr attr);
};

#endif

