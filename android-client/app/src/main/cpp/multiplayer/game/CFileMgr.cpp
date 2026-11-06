//
// // Created on by NoobCooder 14.04.2023.
//

#include "CFileMgr.h"
#include "main.h"
#include "util/patch.h"

void CFileMgr::SetDir(const char *path) {
    ( ( void(*)(const char *path) )(g_libGTASA + (VER_x32 ? 0x003F0C54 + 1 : 0x4D293C)) )(path);
}

FILE* CFileMgr::OpenFile(const char *path, const char *mode) {
    if (!path) return nullptr;

    char cleanPath[512];
    strncpy(cleanPath, path, sizeof(cleanPath) - 1);
    cleanPath[sizeof(cleanPath) - 1] = '\0';
    for (char* p = cleanPath; *p; ++p) {
        if (*p == '\\') *p = '/';
    }

    char lowerPath[512];
    strncpy(lowerPath, cleanPath, sizeof(lowerPath) - 1);
    lowerPath[sizeof(lowerPath) - 1] = '\0';
    for (char* p = lowerPath; *p; ++p) {
        *p = tolower((unsigned char)*p);
    }

    const char* candidateDirs[] = {
        g_pszStorage,
        g_pszRootStorage,
        "/storage/emulated/0/Android/data/com.russia.game/files/",
        "/storage/emulated/0/Android/data/com.kingmakers.rp/files/",
        "/data/data/com.russia.game/files/",
        "/data/data/com.kingmakers.rp/files/",
        "/data/user/0/com.russia.game/files/",
        "/data/user/0/com.kingmakers.rp/files/"
    };
    const size_t numCandidateDirs = sizeof(candidateDirs) / sizeof(candidateDirs[0]);

    FILE* file = nullptr;
    for (size_t i = 0; i < numCandidateDirs; ++i) {
        const char* dir = candidateDirs[i];
        if (!dir || dir[0] == '\0') continue;
        bool hasSlash = (dir[strlen(dir) - 1] == '/');

        snprintf(ms_path, sizeof(ms_path), "%s%s%s", dir, hasSlash ? "" : "/", cleanPath);
        file = fopen(ms_path, mode);
        if (file) break;

        snprintf(ms_path, sizeof(ms_path), "%s%s%s", dir, hasSlash ? "" : "/", lowerPath);
        file = fopen(ms_path, mode);
        if (file) break;
    }

    if (!file) {
        Log("Fail open file %s", path);
    }
    return file;
}

int32_t CFileMgr::CloseFile(FILE* file) {
    return fclose(file);
}

void CFileMgr::Initialise() {
    CHook::CallFunction<void>("_ZN8CFileMgr10InitialiseEv"); // ������� ����� �����
}
