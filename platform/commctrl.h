// commctrl.h -- POSIX stand-in for the Windows common-controls SDK header.
//
// PART OF THE Win32 PLATFORM SHIM, NOT OF THE MFC INTERFACE.
//
// Unlike winsock2.h/io.h, nothing here can "resolve to the native Linux
// symbol": there is no native counterpart. These are the notification
// structures COMCTL32.DLL posts to a window, and off Windows the controls are
// drawn by the GUI toolkit instead. What survives the port is the DATA -- the
// shape of the notification an MFC message handler receives. eMule casts
// LPARAM to these pointers and reads through them, so a wrong field order
// would compile and then misread at run time.
//
// Which is exactly why this file no longer spells the structures out. They are
// generated from the mingw-w64 headers instead, so the layout comes from a
// maintained SDK rather than from a careful reading of the documentation. An
// earlier hand-written version of this file happened to get NMHDR right; that
// is not a property worth relying on 250 more times.
#pragma once

#ifdef _WIN32
#error "simple_mfc/platform is for non-Windows builds only."
#endif

#include <win32_types.h>
#include <win32_constants.h>
