// timeapi.h -- POSIX stand-in for the Windows multimedia timer header.
//
// PART OF THE Win32 PLATFORM SHIM, NOT OF THE MFC INTERFACE.
#pragma once

#ifdef _WIN32
#error "simple_mfc/platform is for non-Windows builds only."
#endif

#include <win32_kernel.h>

// timeGetTime and GetTickCount return the same thing off Windows.
//
// On Windows they differ in resolution, not in meaning: GetTickCount is tied
// to the scheduler tick (~15.6 ms by default) while timeGetTime follows the
// multimedia timer, which is why eMule calls this one for its transfer-rate
// sampling. clock_gettime(CLOCK_MONOTONIC) is already better than either, so
// the distinction disappears here rather than being simulated.
inline unsigned int timeGetTime() { return GetTickCount(); }

// timeBeginPeriod/timeEndPeriod ask Windows to raise the global timer
// resolution for the whole system, at a real cost in power. Linux timers are
// already high-resolution and there is nothing to request, so these are
// genuine no-ops -- not stubs standing in for something unimplemented.
inline unsigned int timeBeginPeriod(unsigned int) { return 0; }   // TIMERR_NOERROR
inline unsigned int timeEndPeriod(unsigned int) { return 0; }
