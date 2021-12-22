#ifndef UTILS_HPP
#define UTILS_HPP
#include <iostream>
#include <vector>

bool is_prefix(const std::string &s, const std::string &prefix);
std::vector<std::string> split_string(const std::string &s,
                                      const char delimiter);

#endif