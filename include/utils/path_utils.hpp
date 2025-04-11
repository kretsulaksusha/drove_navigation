#ifndef DRONE_NAVIGATION_PATH_UTILS_HPP
#define DRONE_NAVIGATION_PATH_UTILS_HPP

#include <string>
#include <filesystem>

namespace fs = std::filesystem;

std::string getContentPath(const std::string& filename, const std::string& media_dir = "media");

#endif //DRONE_NAVIGATION_PATH_UTILS_HPP
