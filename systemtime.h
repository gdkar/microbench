// ©2013-2014 Cameron Desrochers

#pragma once

#if defined(_WIN32)
#define ST_WINDOWS
#elif defined(__APPLE__) && defined(__MACH__)
#define ST_APPLE
#elif defined(__linux__) || defined(__FreeBSD__) || defined(BSD)
#define ST_NIX
#else
#error "Unknown platform"
#endif

#if defined(ST_WINDOWS)
namespace moodycamel { typedef unsigned long long SystemTime; }
#elif defined(ST_APPLE)
#include <cstdint>
namespace moodycamel { typedef std::uint64_t SystemTime; }
#elif defined(ST_NIX)
#include <cstdint>
#include <time.h>
#include <x86intrin.h>
namespace moodycamel { typedef std::uint64_t SystemTime; }
//namespace moodycamel { typedef timespec SystemTime; }
#endif

namespace moodycamel
{
    void sleep(int milliseconds);
    SystemTime getSystemTime();
    double getTimeDelta(SystemTime start);
}
