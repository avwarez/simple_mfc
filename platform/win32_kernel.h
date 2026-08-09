// win32_kernel.h -- the kernel32 entry points that DO have a native Linux
// counterpart, mapped onto it.
//
// PART OF THE Win32 PLATFORM SHIM, NOT OF THE MFC INTERFACE.
//
// This is the half of platform/ that generation cannot produce. win32_types.h
// and win32_constants.h are copied from a maintained SDK because declarations
// are the same everywhere; these are behaviour, and the behaviour has to be
// re-aimed at Linux. Each one below resolves to a real system facility, per
// the rule for this directory -- nothing here returns a plausible-looking
// constant to keep the compiler quiet.
//
// What is deliberately NOT here: the GDI and USER32 functions (BitBlt,
// SelectObject, GetSysColor, DrawIconEx, ...). Those have no kernel to map to;
// they are drawing, and drawing belongs to the GUI driver. Putting fake
// versions here would be the exact dishonesty this directory is meant to
// avoid.
#pragma once

#ifdef _WIN32
#error "simple_mfc/platform is for non-Windows builds only."
#endif

#include <cerrno>
#include <ctime>
#include <unistd.h>

// ---------------------------------------------------------------------
// Time.
//
// CLOCK_MONOTONIC, not CLOCK_REALTIME: GetTickCount is a monotonic
// milliseconds-since-boot counter, and eMule uses it for timeouts and rate
// calculations. Reading wall-clock time instead would make every timer in the
// program jump whenever NTP stepped the clock -- and jump backwards, which
// eMule's `now - then > timeout` comparisons are not written to survive.
//
// The 32-bit return wraps after 49.7 days exactly as the Windows one does.
// That is not a limitation to fix here: eMule compares tick values with
// subtraction, which stays correct across the wrap, and widening the type
// would change the overflow behaviour those comparisons rely on.
inline unsigned int GetTickCount()
{
    struct timespec ts;
    ::clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<unsigned int>(static_cast<unsigned long long>(ts.tv_sec) * 1000ULL
                                     + static_cast<unsigned long long>(ts.tv_nsec) / 1000000ULL);
}

inline unsigned long long GetTickCount64()
{
    struct timespec ts;
    ::clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<unsigned long long>(ts.tv_sec) * 1000ULL
           + static_cast<unsigned long long>(ts.tv_nsec) / 1000000ULL;
}

// Sleep(0) on Windows yields the rest of the timeslice rather than sleeping,
// and eMule uses it that way in its upload throttler's spin. nanosleep(0)
// would return immediately without yielding, so route that case to sched_yield.
inline void Sleep(unsigned int dwMilliseconds)
{
    if (dwMilliseconds == 0) {
        ::sched_yield();
        return;
    }
    struct timespec req;
    req.tv_sec = static_cast<time_t>(dwMilliseconds / 1000u);
    req.tv_nsec = static_cast<long>((dwMilliseconds % 1000u) * 1000000ul);
    while (::nanosleep(&req, &req) == -1 && errno == EINTR) {
        // restart with the remainder nanosleep wrote back
    }
}

// ---------------------------------------------------------------------
// Last-error.
//
// errno IS the native equivalent, and mapping onto it rather than onto a
// private variable is what makes the WSAGetLastError mapping in winsock2.h
// consistent with this one: a failed socket call sets errno, and both
// spellings then report the same thing.
inline unsigned int GetLastError()
{
    return static_cast<unsigned int>(errno);
}

inline void SetLastError(unsigned int dwErrCode)
{
    errno = static_cast<int>(dwErrCode);
}

// ---------------------------------------------------------------------
// Process and system identity.
inline unsigned int GetCurrentProcessId() { return static_cast<unsigned int>(::getpid()); }
inline unsigned int GetCurrentThreadId()  { return static_cast<unsigned int>(::gettid()); }
