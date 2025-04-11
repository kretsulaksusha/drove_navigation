#include "path_utils.hpp"

std::string getContentPath(const std::string& filename, const std::string& media_dir) {
    fs::path cwd = fs::current_path();
    std::string relative_dir = (cwd.filename() == "drove_navigation") ? "./" : "../";

    fs::path media_path = relative_dir + media_dir;

    return (media_path / filename).string();
}
