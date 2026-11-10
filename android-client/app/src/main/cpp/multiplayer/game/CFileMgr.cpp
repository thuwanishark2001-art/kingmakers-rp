// File manager helpers used by the mobile runtime.
#include "CFileMgr.h"
#include "KingmakersAssetPathPolicy.h"
#include "main.h"
#include "util/patch.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include <unistd.h>

namespace {

constexpr size_t kMaxVirtualPath = 512;

std::string NormalizeSeparators(const char* input) {
    std::string result(input ? input : "");
    for (char& ch : result) {
        if (ch == '\\') ch = '/';
    }
    return result;
}

std::string LowerAscii(std::string value) {
    for (char& ch : value) {
        const unsigned char byte = static_cast<unsigned char>(ch);
        if (byte >= 'A' && byte <= 'Z') {
            ch = static_cast<char>(byte - 'A' + 'a');
        }
    }
    return value;
}

// Pure string classification. Never touch the filesystem here: path ownership
// must not be inferred by opening a candidate (which can leak descriptors and
// confuses a missing owned asset with a non-owned path).
bool IsOwnedRequest(const std::string& path) {
    const std::string lower = LowerAscii(path);
    return lower.rfind("samp/", 0) == 0 ||
           lower.rfind("gui/", 0) == 0 ||
           lower.rfind("fonts/", 0) == 0 ||
           lower.rfind("kingmakers/", 0) == 0 ||
           lower.rfind("audio/streams/", 0) == 0;
}

bool HasUnsafePathSyntax(const std::string& path) {
    if (path.empty() || path.front() == '/' || path.find(':') != std::string::npos) {
        return true;
    }

    std::vector<std::string> components;
    size_t start = 0;
    while (start <= path.size()) {
        const size_t end = path.find('/', start);
        const std::string component = path.substr(
            start, end == std::string::npos ? std::string::npos : end - start);
        if (component.empty() || component == "." || component == "..") {
            return true;
        }
        components.push_back(component);
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return components.empty();
}

bool IsReadOnlyMode(const char* mode) {
    if (mode == nullptr || mode[0] != 'r') return false;
    for (const char* p = mode + 1; *p != '\0'; ++p) {
        if (*p == '+' || *p == 'w' || *p == 'a') return false;
    }
    return true;
}

// Samp.initSAMP supplies this app-private runtime root. Do not replace it with
// a process-wide or public-storage root for Kingmakers-owned content.
std::string RuntimeRoot() {
    if (g_pszRootStorage != nullptr && g_pszRootStorage[0] != '\0') {
        return g_pszRootStorage;
    }
    return {};
}

FILE* OpenOwnedReadOnly(const std::string& root, const std::string& relativePath) {
    const int fd = kingmakers::assets::OpenOwnedAssetReadOnly(root, relativePath);
    if (fd < 0) return nullptr;

    FILE* file = fdopen(fd, "rb");
    if (file == nullptr) {
        const int savedErrno = errno;
        close(fd);
        errno = savedErrno;
    }
    return file;
}

}  // namespace

void CFileMgr::SetDir(const char* path) {
    ((void (*)(const char*))(g_libGTASA + (VER_x32 ? 0x003F0C54 + 1 : 0x4D293C)))(path);
}

FILE* CFileMgr::OpenFile(const char* path, const char* mode) {
    if (path == nullptr || mode == nullptr) {
        errno = EINVAL;
        return nullptr;
    }

    // Bound the input before copying it into std::string or path buffers.
    const size_t pathLength = strnlen(path, kMaxVirtualPath);
    if (pathLength == 0 || pathLength >= kMaxVirtualPath) {
        Log("[assets] denied empty or overlong path");
        errno = ENAMETOOLONG;
        return nullptr;
    }

    const std::string cleanPath = NormalizeSeparators(path);
    if (HasUnsafePathSyntax(cleanPath)) {
        Log("[assets] denied unsafe path: %s", path);
        errno = EINVAL;
        return nullptr;
    }

    const std::string root = RuntimeRoot();
    if (root.empty()) {
        Log("[assets] app-private runtime root is unavailable: %s", path);
        errno = ENOENT;
        return nullptr;
    }

    if (IsOwnedRequest(cleanPath)) {
        if (!IsReadOnlyMode(mode)) {
            Log("[assets] denied write-capable open for owned asset: %s", path);
            errno = EROFS;
            return nullptr;
        }

        // The resolver validates the owned namespace and opens each component
        // beneath the real app-private root using openat(O_NOFOLLOW).
        FILE* file = OpenOwnedReadOnly(root, cleanPath);
        if (file == nullptr) {
            const int savedErrno = errno;
            Log("[assets] owned asset missing or rejected: %s (errno=%d)", path, savedErrno);
            errno = savedErrno;
        }
        return file;
    }

    // Base-game data is imported into the same app-private runtime directory,
    // but is intentionally not accepted by the Kingmakers-owned asset policy.
    // Never fall back to arbitrary external roots.
    const std::string lowerPath = LowerAscii(cleanPath);
    const bool rootHasSlash = root.back() == '/';
    std::snprintf(ms_path, sizeof(ms_path), "%s%s%s",
                  root.c_str(), rootHasSlash ? "" : "/", cleanPath.c_str());
    FILE* file = std::fopen(ms_path, mode);

    if (file == nullptr && lowerPath != cleanPath) {
        std::snprintf(ms_path, sizeof(ms_path), "%s%s%s",
                      root.c_str(), rootHasSlash ? "" : "/", lowerPath.c_str());
        file = std::fopen(ms_path, mode);
    }

    if (file == nullptr) {
        Log("[file] fail open: %s", path);
    }
    return file;
}

int32_t CFileMgr::CloseFile(FILE* file) {
    return file != nullptr ? std::fclose(file) : 0;
}

void CFileMgr::Initialise() {
    CHook::CallFunction<void>("_ZN8CFileMgr10InitialiseEv");
}
