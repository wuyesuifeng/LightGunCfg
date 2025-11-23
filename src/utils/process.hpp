#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <stdio.h>
#include <string>
#include <vector>

#include <string>

namespace ProcessHelper {

    int findLastOfIgnoreCase(std::string str, std::string dst) {
        static int i, j, dstLen;
        static char dstLast;
        dstLen = dst.length() - 1;
        dstLast = toupper(dst.at(dstLen));
        for (i = str.length() - 1; i >= dstLen; i--) {
            if (toupper(str.at(i)) == dstLast) {
                for (j = 1; j < dstLen; j++) {
                    if (toupper(str.at(i - j)) != toupper(dst.at(dstLen - j))) {
                        goto for1;
                    }
                }
                return i + 1;
            }
        for1:
            continue;
        }
        return -1;
    }

    bool ifProcessModules(DWORD dwPID, std::string modulePath) {
        HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
        MODULEENTRY32 me32;

        // Take a snapshot of all modules in the specified process.
        hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPID);
        if (hModuleSnap == INVALID_HANDLE_VALUE) {
            // Log("CreateToolhelp32Snapshot (of modules)");
            CloseHandle(hModuleSnap); // clean the snapshot object
            return false;
        }

        // Set the size of the structure before using it.
        me32.dwSize = sizeof(MODULEENTRY32);

        // Retrieve information about the first module,
        // and exit if unsuccessful
        if (!Module32First(hModuleSnap, &me32)) {
            // Log("Module32First");              // show cause of failure
            CloseHandle(hModuleSnap); // clean the snapshot object
            return false;
        }

        // Now walk the module list of the process,
        // and display information about each module
        do {
            std::string szExePath(me32.szExePath);
            if (findLastOfIgnoreCase(szExePath, modulePath) == szExePath.length()) {
                CloseHandle(hModuleSnap); // clean the snapshot object
                return true;
            }

        } while (Module32Next(hModuleSnap, &me32));

        CloseHandle(hModuleSnap);
        return false;
    }

    bool ifProcess(std::string processName, std::string modulePath) { // 0 not found ; other found; processName "processName.exe"
        HANDLE hProcessSnap;
        PROCESSENTRY32 pe32;
        hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hProcessSnap == INVALID_HANDLE_VALUE) {
            CloseHandle(hProcessSnap); // clean the snapshot object
            return false;
        }
        pe32.dwSize = sizeof(PROCESSENTRY32);
        if (!Process32First(hProcessSnap, &pe32)) {
            CloseHandle(hProcessSnap); // clean the snapshot object
            return false;
        }
        do {
            if (std::string(pe32.szExeFile) == processName &&
                ifProcessModules(pe32.th32ProcessID, modulePath)) {
                CloseHandle(hProcessSnap);
                return true;
            }
        } while (Process32Next(hProcessSnap, &pe32));
        CloseHandle(hProcessSnap);
        return false;
    }
}