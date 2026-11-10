//
// File manager helpers used by the mobile runtime.
//
#include "CFileMgr.h"
#include "KingmakersAssetPathPolicy.h"
#include "main.h"
#include "util/patch.h"

#include <cerrno>
#include <cstring>
#include <string>
#include <unistd.h>

namespace {

bool IsReadOnlyMode(const char* mode) {
    if (!mode || mode[0] != 'r') return false;
    for (const char* p = mode + 1; *p; ++p) {
        if (*p == '+' || *p == 'w' || *p == 'a') return false;
    }
    return true;
}

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
        if (byte >= 'A' && byte <= 'Z') ch = static_cast<char>(byte - 'A' + 'a');
    }
    return value;
}

// The runtime root is supplied by Java at Samp.initSAMP. Storage fallbacks are
// deliberately not used for client-owned content: accepting an arbitrary
// external root would weaken the path API's trust boundary.
std::string RuntimeRoot() {
    if (g_pszRootStorage && g_pszRootStorage[0] != '\0') return g_pszRootStorage;
    if (g_pszStorage && g_pszStorage[0] != '\0') return g_pszStorage;
    return {};
}

FILE* OpenOwnedReadOnly(const std::string& root, const std::string& relativePath) {
    const int fd = kingmakers::assets::OpenOwnedAssetReadOnly(root, relativePath);
    if (fd < 0) return nullptr;

    FILE* file = fdopen(fd, "rb");
    if (!file) {
        const int savedErrno = errno;
        close(fd);
        errno = savedErrno;
    }
    return file;
}

bool IsOwnedRequest(const std::string& relativePath) {
    return kingmakers::assets::OpenOwnedAssetReadOnly("/", relativePath) >= 0;
}

}  // namespace

void CFileMgr::SetDir(const char* path) {
    ((void (*)(const char*))(g_libGTASA + (VER_x32 ? 0x003F0C54 + 1 : 0x4D293C)))(path);
}

FILE* CFileMgr::OpenFile(const char* path, const char* mode) {
    if (!path || !mode) {
        errno = EINVAL;
        return nullptr;
    }

    const std::string cleanPath = NormalizeSeparators(path);
    const std::string root = RuntimeRoot();
    if (root.empty()) {
        Log("[assets] runtime storage is not initialized; refusing open: %s", path);
        errno = ENOENT;
        return nullptr;
    }

    // Owner content may only be opened read-only, beneath the trusted runtime
    // root, and via the symlink/traversal-resistant openat resolver.
    if (IsOwnedRequest(cleanPath)) {
        if (!IsReadOnlyMode(mode)) {
            Log("[assets] denied write-capable open for owned asset: %s", path);
            errno = EROFS;
            return nullptr;
        }
        FILE* file = OpenOwnedReadOnly(root, cleanPath);
        if (!file) {
            const int savedErrno = errno;
            Log("[assets] rejected or missing owned asset: %s (errno=%d)", path, savedErrno);
            errno = savedErrno;
        }
        return file;
    }

    // Engine/game data is not part of Kingmakers' owned pack. Preserve the
    // existing engine-data lookup behavior for these paths only, while rejecting
    // rooted paths and explicit traversal before constructing a filesystem path.
    if (cleanPath.empty() || cleanPath.front() == '/' || cleanPath.find(':') != std::string::npos) {
        Log("[assets] denied rooted path: %s", path);
        errno = EINVAL;
        return nullptr;
    }
    for (const auto& component : [&]() {
             std::vector<std::string> parts;
             size_t start = 0;
             while (start <= cleanPath.size()) {
                 const size_t end = cleanPath.find('/', start);
                 parts.emplace_back(cleanPath.substr(start, end == std::string::npos ? std::string::npos : end - start));
                 if (end == std::string::npos) break;
                 start = end + 1;
             }
             return parts;
         }()) {
        if (component == ".." || component == "." || component.empty()) {
            Log("[assets] denied traversal or malformed path: %s", path);
            errno = EINVAL;
            return nullptr;
        }
    }

    const std::string lowerPath = LowerAscii(cleanPath);
    const char* candidateDirs[] = {root.c_str()};
    FILE* file = nullptr;
    for (const char* dir : candidateDirs) {
        if (!dir || dir[0] == '\0') continue;
        const size_t dirLength = std::strlen(dir);
        const bool hasSlash = dirLength > 0 && dir[dirLength - 1] == '/';

        std::snprintf(ms_path, sizeof(ms_path), "%s%s%s", dir, hasSlash ? "" : "/", cleanPath.c_str());
        file = std::fopen(ms_path, mode);
        if (file) break;

        if (lowerPath != cleanPath) {
            std::snprintf(ms_path, sizeof(ms_path), "%s%s%s", dir, hasSlash ? "" : "/", lowerPath.c_str());
            file = std::fopen(ms_path, mode);
            if (file) break;
        }
    }

    if (!file) Log("[file] Fail open %s", path);
    return file;
}

int32_t CFileMgr::CloseFile(FILE* file) {
    return file ? std::fclose(file) : 0;
}

void CFileMgr::Initialise() {
    CHook::CallFunction<void>("_ZN8CFileMgr10InitialiseEv");
}
