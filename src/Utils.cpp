#include <iostream>
#include <ostream>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>

#include "Utils.h"

using transform_width = boost::archive::iterators::transform_width<std::vector<unsigned char>::const_iterator, 6, 8>;
using base64_from_binary = boost::archive::iterators::base64_from_binary<transform_width>;
using binary_from_base64 = boost::archive::iterators::binary_from_base64<transform_width>;

namespace MediaFs {
    std::vector<std::string> split(std::string data, std::string delim) {

        std::vector<std::string> items;
        int i = -1;
        do {
            int index = data.find(delim, ++i);
            if (index != -1) {
                items.push_back(data.substr(i, index - i));
            } else if (i <= data.size()) {
                items.push_back(data.substr(i, data.size() - i));
            }
            i = index;
        } while (i != -1);
        return items;
    }

    std::ostream& operator<<(std::ostream& out, Attr attr) {
        out << "Attr(" << attr.name << "," << attr.size << "," << attr.supportedType << ")\n";
        return out;
    }

    std::string base64Encode(std::string data) {
        std::vector<unsigned char> raw(data.begin(), data.end());
        std::string encoded(base64_from_binary(raw.begin()), base64_from_binary(raw.end()));
        size_t padding = (3 - (data.size() % 3)) % 3;
        encoded.append(padding, '=');
        return encoded;
    }

    std::string base64Decode(std::string data) {
        std::vector<unsigned char> raw(data.begin(), data.end());
        std::string decoded(binary_from_base64(raw.begin()), binary_from_base64(raw.end()));
        return decoded;
    }
};


