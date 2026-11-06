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

    const char* storage = (g_pszStorage && g_pszStorage[0] != '\0')
        ? g_pszStorage
        : ((g_pszRootStorage && g_pszRootStorage[0] != '\0') ? g_pszRootStorage : "/storage/emulated/0/Android/data/com.russia.game/files/");

    sprintf(ms_path, "%s%s", storage, path);

    auto file = fopen(ms_path, mode);
    if (!file) {
        char lowerPath[512];
        strncpy(lowerPath, ms_path, sizeof(lowerPath) - 1);
        lowerPath[sizeof(lowerPath) - 1] = '\0';
        for (char* p = lowerPath; *p; ++p) *p = tolower((unsigned char)*p);
        file = fopen(lowerPath, mode);
        if (file) {
            strncpy(ms_path, lowerPath, sizeof(ms_path) - 1);
        }
    }

    if(!file) {
        Log("Fail open file %s", ms_path);
    }
    return file;
}

int32_t CFileMgr::CloseFile(FILE* file) {
    return fclose(file);
}

void CFileMgr::Initialise() {
    CHook::CallFunction<void>("_ZN8CFileMgr10InitialiseEv"); // ������� ����� �����
}
