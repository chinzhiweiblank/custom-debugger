#include <iostream>
#include <vector>
#include <sstream>
#include "utils.hpp"

bool is_prefix(const std::string& s, const std::string& prefix) {
    if (s.size() < prefix.size()) {
        return false;
    }
    return std::equal(s.begin(), s.end(), prefix.begin());
}

std::vector<std::string> split_string(const std::string& s, const char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss {s};
    std::string item;

    while (std::getline(ss, item, delimiter)) {
        result.push_back(item);
    }
    return result;
}