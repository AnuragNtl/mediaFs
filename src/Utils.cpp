#include <iostream>

#include "Utils.h"

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
};



