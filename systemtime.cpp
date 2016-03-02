// ©2013-2014 Cameron Desrochers

#include "systemtime.h"
#include <climits>

#if defined(_MSC_VER) && _MSC_VER < 1700
#include <intrin.h>
#define CompilerMemBar() _ReadWriteBarrier()
#else
#include <atomic>
#define CompilerMemBar() std::atomic_signal_fence(std::memory_order_seq_cst)
#endif

#if defined(ST_WINDOWS)

#include <windows.h>

namespace moodycamel
{
void sleep(int milliseconds)
{
	::Sleep(milliseconds);
}
SystemTime getSystemTime()
{
	LARGE_INTEGER t;
	CompilerMemBar();
	if (!QueryPerformanceCounter(&t)) {
		return static_cast<SystemTime>(-1);
	}
	CompilerMemBar();
	
	return static_cast<SystemTime>(t.QuadPart);
}

double getTimeDelta(SystemTime start)
{
	LARGE_INTEGER t;
	CompilerMemBar();
	if (start == static_cast<SystemTime>(-1) || !QueryPerformanceCounter(&t)) {
		return -1;
	}
	CompilerMemBar();

	auto now = static_cast<SystemTime>(t.QuadPart);

	LARGE_INTEGER f;
	if (!QueryPerformanceFrequency(&f)) {
		return -1;
	}

	return static_cast<double>(static_cast<__int64>(now - start)) / f.QuadPart * 1000;
}

}  // end namespace moodycamel

#elif defined(ST_APPLE)

#include <mach/mach.h>
#include <mach/mach_time.h>
#include <unistd.h>
#include <time.h>

namespace moodycamel
{

void sleep(int milliseconds)
{
	::usleep(milliseconds * 1000);
}

SystemTime getSystemTime()
{
	CompilerMemBar();
	std::uint64_t result = mach_absolute_time();
	CompilerMemBar();
	
	return result;
}

double getTimeDelta(SystemTime start)
{
	CompilerMemBar();
	std::uint64_t end = mach_absolute_time();
	CompilerMemBar();

	mach_timebase_info_data_t tb = { 0 };
	mach_timebase_info(&tb);
	double toNano = static_cast<double>(tb.numer) / tb.denom;
	
	return static_cast<double>(end - start) * toNano * 0.000001;
}

}  // end namespace moodycamel

#elif defined(ST_NIX)
#include <unistd.h>
namespace moodycamel
{
void sleep(int milliseconds)
{
	::usleep(milliseconds * 1000);
}
SystemTime getSystemTime()
{
    uint32_t ignored;
    return __rdtscp(&ignored);
}
double getTimeDelta(SystemTime start)
{
    return static_cast<double>(getSystemTime() - start);
}

}  // end namespace moodycamel

#endif
