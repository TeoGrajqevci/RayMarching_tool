#include "rmt/common/fs/DirectoryUtils.h"

#include <cerrno>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>

namespace rmt {

bool ensureDirectoryExists(const std::string& path) {
    if (path.empty()) {
        return false;
    }

    std::string current;
    if (path[0] == '/') {
        current = "/";
    }

    std::stringstream ss(path);
    std::string segment;
    while (std::getline(ss, segment, '/')) {
        if (segment.empty() || segment == ".") {
            continue;
        }

        if (!current.empty() && current[current.size() - 1] != '/') {
            current += "/";
        }
        current += segment;

        if (mkdir(current.c_str(), 0755) != 0) {
            if (errno != EEXIST) {
                return false;
            }
        }
    }
    return true;
}

} // namespace rmt
