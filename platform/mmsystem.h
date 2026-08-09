// mmsystem.h -- POSIX stand-in for the Windows multimedia header.
//
// PART OF THE Win32 PLATFORM SHIM, NOT OF THE MFC INTERFACE.
#pragma once

#ifdef _WIN32
#error "simple_mfc/platform is for non-Windows builds only."
#endif

#include <timeapi.h>
#include <win32_constants.h>

// PlaySound has no native counterpart: playing audio on Linux means picking a
// sound server (PulseAudio, PipeWire, ALSA), which is a dependency decision
// and not a header mapping. It is DECLARED here and left to the GUI driver to
// implement -- the toolkit already has to be linked, and it knows how to make
// a sound. A version that silently returned TRUE without playing anything
// would compile, pass every test we have, and be wrong in the one way nobody
// would notice until the app ran.
BOOL PlaySoundW(LPCWSTR pszSound, HMODULE hmod, DWORD fdwSound);
BOOL PlaySoundA(LPCSTR pszSound, HMODULE hmod, DWORD fdwSound);

#ifdef _UNICODE
#define PlaySound PlaySoundW
#else
#define PlaySound PlaySoundA
#endif
