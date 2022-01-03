#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>
#include <sstream>

bool is_prefix(const std::string &s, const std::string &prefix) {
  if (s.size() < prefix.size()) {
    return false;
  }
  return std::equal(s.begin(), s.end(), prefix.begin());
}

bool is_suffix(const std::string &s, const std::string &suffix) {
  if (s.size() < suffix.size()) {
    return false;
  }
  return std::equal(s.end()-suffix.size(), s.end(), suffix.begin());
}
std::vector<std::string> split_string(const std::string &s,
                                      const char delimiter) {
  std::vector<std::string> result;
  std::stringstream ss{s};
  std::string item;

  while (std::getline(ss, item, delimiter)) {
    result.push_back(item);
  }
  return result;
}

#endif