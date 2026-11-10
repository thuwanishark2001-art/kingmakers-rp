#include "KingmakersAssetPathPolicy.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <string>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

namespace kingmakers::assets {
namespace {

bool IsOwnedNamespace(const std::string& path) {
    const auto slash = path.find('/');
    if (slash == std::string::npos || slash == 0) return false;

    std::string first = path.substr(0, slash);
    std::transform(first.begin(), first.end(), first.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    if (first == "samp" || first == "gui" || first == "fonts" || first == "kingmakers") {
        return true;
    }
    if (first != "audio") return false;

    const auto secondSlash = path.find('/', slash + 1);
    if (secondSlash == std::string::npos) return false;
    std::string second = path.substr(slash + 1, secondSlash - slash - 1);
    std::transform(second.begin(), second.end(), second.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return second == "streams";
}

bool NormalizeRelativePath(const std::string& input, std::string* normalized) {
    if (!normalized || input.empty() || input.front() == '/' || input.front() == '\\') return false;
    if (input.find('\0') != std::string::npos || input.find(':') != std::string::npos) return false;

    std::vector<std::string> components;
    std::string component;
    for (char ch : input) {
        if (ch == '/' || ch == '\\') {
            if (component.empty() || component == "." || component == "..") return false;
            components.push_back(component);
            component.clear();
        } else {
            component.push_back(ch);
        }
    }
    if (component.empty() || component == "." || component == "..") return false;
    components.push_back(component);
    if (components.size() < 2) return false;

    normalized->clear();
    for (const auto& part : components) {
        if (part.find('\0') != std::string::npos) return false;
        if (!normalized->empty()) normalized->push_back('/');
        normalized->append(part);
    }
    return IsOwnedNamespace(*normalized);
}

}  // namespace

int OpenOwnedAssetReadOnly(const std::string& runtimeRoot, const std::string& virtualPath) {
    std::string relativePath;
    if (runtimeRoot.empty() || !NormalizeRelativePath(virtualPath, &relativePath)) {
        errno = EINVAL;
        return -1;
    }

    int directory = open(runtimeRoot.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (directory < 0) return -1;

    size_t start = 0;
    while (true) {
        const size_t slash = relativePath.find('/', start);
        const bool finalComponent = slash == std::string::npos;
        const std::string component = relativePath.substr(
            start, finalComponent ? std::string::npos : slash - start);
        const int flags = O_RDONLY | O_CLOEXEC | O_NOFOLLOW |
                          (finalComponent ? 0 : O_DIRECTORY);
        const int next = openat(directory, component.c_str(), flags);
        const int savedErrno = errno;
        close(directory);
        if (next < 0) {
            errno = savedErrno;
            return -1;
        }
        if (finalComponent) return next;
        directory = next;
        start = slash + 1;
    }
}

}  // namespace kingmakers::assets
