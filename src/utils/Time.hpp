#pragma once

#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <sys/time.h>
#endif

namespace utils {

#ifdef _WIN32
    unsigned long long timestamp() {
        return GetTickCount();
    }
#elif __linux__
    unsigned long long timestamp() {
        timespec tv;
        clock_gettime(CLOCK_MONOTONIC, &tv);
        return (999999999.9999999 * tv.tv_sec + tv.tv_nsec) / 999999.9999999999;
    }
#endif
    

}