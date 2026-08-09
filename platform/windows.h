// windows.h -- POSIX stand-in for the Windows SDK's umbrella header.
//
// PART OF THE Win32 PLATFORM SHIM, NOT OF THE MFC INTERFACE. Nothing in this
// directory is MFC or ATL; it reproduces the pieces of the Windows SDK that an
// MFC application reaches past MFC to use directly. It is on the include path
// ONLY on non-Windows builds.
//
// SCOPE: strictly what an actual compilation error has demanded. This is not
// an attempt to reimplement the Windows SDK -- it is the transitive closure of
// what eMule's 244 translation units genuinely reference, and it grows only
// when the compiler asks for something.
//
// The real windows.h is an umbrella over windef.h/winbase.h/winuser.h/wingdi.h
// /winnt.h. Keeping that split here would buy nothing (no non-Windows code
// includes those individually), so the contents are grouped by their SDK home
// in comments instead.
//
// INCLUSION POINT: this file is reached from <afxwin.h>'s non-Windows branch,
// at exactly the spot where the Windows build includes the real <windows.h>.
// That ordering is deliberate and is what lets this header use UINT/WORD/BYTE/
// DWORD/BOOL (afx.h) and POINT/RECT/SIZE (atltypes.h) without redefining them:
// those types have exactly one owner each, and duplicating a struct definition
// would be an outright compile error rather than a merely redundant typedef.
#pragma once

#ifdef _WIN32
#error "simple_mfc/platform is for non-Windows builds only."
#endif

#include <cstddef>
#include <cstdint>
#include <climits>

// ---------------------------------------------------------------------
// winnt.h -- primitive types.
//
// A note on widths, because this is the one place where a careless typedef
// silently corrupts data on the wire: Windows is LLP64 (long is 32 bits),
// Linux is LP64 (long is 64 bits). Every fixed-width Win32 type is therefore
// spelled with a fixed-width C++ type rather than with `long`, so that DWORD
// is 32 bits on both. Only the pointer-sized types (*_PTR) follow the
// pointer.
// ---------------------------------------------------------------------
// LONG/LONGLONG/ULONGLONG/BOOL/UINT/WORD/BYTE/DWORD/HANDLE are NOT here:
// afx.h owns them (see the inclusion-point note above). Re-declaring them
// would have to match afx.h's spelling exactly -- `unsigned long long` and
// `std::uint64_t` are the SAME width but DIFFERENT types on LP64, and that
// mismatch is ill-formed, not harmless.
using CHAR = char;
using SHORT = short;
using WCHAR = wchar_t;
using UCHAR = unsigned char;
using USHORT = unsigned short;
using ULONG = unsigned int;
using DWORDLONG = unsigned long long;
using PVOID = void *;

using PSTR = char *;
using LPSTR = char *;
using PCSTR = const char *;
using LPCSTR = const char *;
using PWSTR = wchar_t *;
using LPWSTR = wchar_t *;
using PCWSTR = const wchar_t *;
using LPCWSTR = const wchar_t *;

using LANGID = unsigned short;
using LCID = std::uint32_t;

// MSVC's rpcndr.h defines this, and eMule uses it pervasively as its raw
// byte type. It must NOT be `std::byte`: eMule does arithmetic on it.
using byte = unsigned char;

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#define MAKEWORD(a, b)  ((WORD)(((BYTE)(a)) | (((WORD)((BYTE)(b))) << 8)))
#define MAKELONG(a, b)  ((LONG)(((WORD)(a)) | (((DWORD)((WORD)(b))) << 16)))
#define LOWORD(l)       ((WORD)(((DWORD_PTR)(l)) & 0xFFFF))
#define HIWORD(l)       ((WORD)((((DWORD_PTR)(l)) >> 16) & 0xFFFF))
#define LOBYTE(w)       ((BYTE)(((DWORD_PTR)(w)) & 0xFF))
#define HIBYTE(w)       ((BYTE)((((DWORD_PTR)(w)) >> 8) & 0xFF))

// Locale identifiers (winnt.h).
#define MAKELANGID(p, s)       ((((WORD)(s)) << 10) | (WORD)(p))
#define PRIMARYLANGID(lgid)    ((WORD)(lgid) & 0x3FF)
#define SUBLANGID(lgid)        ((WORD)(lgid) >> 10)
#define MAKELCID(lgid, srtid)  ((DWORD)((((DWORD)((WORD)(srtid))) << 16) | ((DWORD)((WORD)(lgid)))))

#define LANG_NEUTRAL     0x00
#define LANG_ENGLISH     0x09
#define SUBLANG_NEUTRAL  0x00
#define SUBLANG_DEFAULT  0x01
#define SUBLANG_SYS_DEFAULT 0x02
#define SORT_DEFAULT     0x0

#define LOCALE_USER_DEFAULT   MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), SORT_DEFAULT)
#define LOCALE_SYSTEM_DEFAULT MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT), SORT_DEFAULT)

// ---------------------------------------------------------------------
// windef.h -- handles.
//
// Every one of these is an opaque kernel/GDI object on Windows. Off Windows
// there is no such object; they exist so that code that stores, compares and
// null-checks a handle still compiles. A measurement of eMule's own usage
// found that 55% of handle uses are exactly that -- guards and comparisons,
// never a Win32 call -- which is why `void*` is enough here.
// ---------------------------------------------------------------------
using HMODULE = void *;
using HKEY = void *;
using HFILE = int;
using HTHEME = void *;
using HHOOK = void *;
using HACCEL = void *;
using HKL = void *;
using HDROP = void *;
using HRSRC = void *;
using HMONITOR = void *;
using HDWP = void *;
using HENHMETAFILE = void *;
using HMETAFILE = void *;
// HRESULT is not here either: atlcomcli.h owns it (single owner per symbol).

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)(std::intptr_t)-1)
#endif

// ---------------------------------------------------------------------
// wingdi.h
// ---------------------------------------------------------------------
struct RGBQUAD
{
    BYTE rgbBlue;
    BYTE rgbGreen;
    BYTE rgbRed;
    BYTE rgbReserved;
};

struct tagBITMAPINFOHEADER
{
    DWORD biSize;
    LONG  biWidth;
    LONG  biHeight;
    WORD  biPlanes;
    WORD  biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG  biXPelsPerMeter;
    LONG  biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
};
using BITMAPINFOHEADER = tagBITMAPINFOHEADER;
using LPBITMAPINFOHEADER = BITMAPINFOHEADER *;
using PBITMAPINFOHEADER = BITMAPINFOHEADER *;

struct tagBITMAPINFO
{
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD          bmiColors[1];
};
using BITMAPINFO = tagBITMAPINFO;
using LPBITMAPINFO = BITMAPINFO *;
using PBITMAPINFO = BITMAPINFO *;

#define RGB(r, g, b) ((COLORREF)(((BYTE)(r)) | (((WORD)((BYTE)(g))) << 8) | (((DWORD)((BYTE)(b))) << 16)))
#define GetRValue(c) ((BYTE)((c) & 0xFF))
#define GetGValue(c) ((BYTE)((((WORD)(c)) >> 8) & 0xFF))
#define GetBValue(c) ((BYTE)(((c) >> 16) & 0xFF))

#define BI_RGB       0
#define DIB_RGB_COLORS 0
#define DIB_PAL_COLORS 1

// ---------------------------------------------------------------------
// winuser.h -- system colours, resource loading flags, help, WM_COPYDATA.
// ---------------------------------------------------------------------
#define COLOR_SCROLLBAR         0
#define COLOR_BACKGROUND        1
#define COLOR_ACTIVECAPTION     2
#define COLOR_INACTIVECAPTION   3
#define COLOR_MENU              4
#define COLOR_WINDOW            5
#define COLOR_WINDOWFRAME       6
#define COLOR_MENUTEXT          7
#define COLOR_WINDOWTEXT        8
#define COLOR_CAPTIONTEXT       9
#define COLOR_ACTIVEBORDER      10
#define COLOR_INACTIVEBORDER    11
#define COLOR_APPWORKSPACE      12
#define COLOR_HIGHLIGHT         13
#define COLOR_HIGHLIGHTTEXT     14
#define COLOR_BTNFACE           15
#define COLOR_BTNSHADOW         16
#define COLOR_GRAYTEXT          17
#define COLOR_BTNTEXT           18
#define COLOR_INACTIVECAPTIONTEXT 19
#define COLOR_BTNHIGHLIGHT      20
#define COLOR_3DDKSHADOW        21
#define COLOR_3DLIGHT           22
#define COLOR_INFOTEXT          23
#define COLOR_INFOBK            24
#define COLOR_HOTLIGHT          26
#define COLOR_GRADIENTACTIVECAPTION   27
#define COLOR_GRADIENTINACTIVECAPTION 28
#define COLOR_MENUHILIGHT       29
#define COLOR_MENUBAR           30

// LoadImage flags.
#define LR_DEFAULTCOLOR     0x0000
#define LR_MONOCHROME       0x0001
#define LR_COLOR            0x0002
#define LR_COPYRETURNORG    0x0004
#define LR_COPYDELETEORG    0x0008
#define LR_LOADFROMFILE     0x0010
#define LR_LOADTRANSPARENT  0x0020
#define LR_DEFAULTSIZE      0x0040
#define LR_VGACOLOR         0x0080
#define LR_LOADMAP3DCOLORS  0x1000
#define LR_CREATEDIBSECTION 0x2000
#define LR_SHARED           0x8000

#define IMAGE_BITMAP  0
#define IMAGE_ICON    1
#define IMAGE_CURSOR  2

// WinHelp commands.
#define HELP_CONTEXT        0x0001
#define HELP_QUIT           0x0002
#define HELP_INDEX          0x0003
#define HELP_CONTENTS       0x0003
#define HELP_HELPONHELP     0x0004
#define HELP_SETINDEX       0x0005
#define HELP_CONTEXTPOPUP   0x0008
#define HELP_KEY            0x0101
#define HELP_COMMAND        0x0102
#define HELP_FINDER         0x000B

struct tagCOPYDATASTRUCT
{
    ULONG_PTR dwData;
    DWORD     cbData;
    PVOID     lpData;
};
using COPYDATASTRUCT = tagCOPYDATASTRUCT;
using PCOPYDATASTRUCT = COPYDATASTRUCT *;

struct tagWINDOWPLACEMENT
{
    UINT  length;
    UINT  flags;
    UINT  showCmd;
    POINT ptMinPosition;
    POINT ptMaxPosition;
    RECT  rcNormalPosition;
};

#define WPF_SETMINPOSITION       0x0001
#define WPF_RESTORETOMAXIMIZED   0x0002
#define WPF_ASYNCWINDOWPLACEMENT 0x0004

// ---------------------------------------------------------------------------
// Generated from the mingw-w64 SDK headers.
//
// These two come LAST because they build on the primitives declared above and
// on afx.h's UINT/WORD/BYTE/DWORD and atltypes.h's POINT/RECT. Everything in
// them was produced by clang preprocessing a maintained SDK, not transcribed:
// constants and struct layouts are exactly the class of thing where a
// hand-typed value compiles fine and then misbehaves silently at run time.
//
// The hand-written parts of this file stay hand-written on purpose -- they are
// the native Linux mappings, which no SDK header can supply.
// ---------------------------------------------------------------------------
#include <win32_types.h>
#include <win32_constants.h>

// The behavioural half, which no SDK header can supply: kernel32 entry points
// re-aimed at their native Linux counterparts.
#include <win32_kernel.h>

// min/max.
//
// windef.h really does define these as macros (NOMINMAX suppresses them, and
// eMule does not define it), but reproducing them as macros here is not an
// option: the first sweep with them in place produced 417 errors inside
// libstdc++, because <functional> and <deque> call std::max and the macro
// rewrites the qualified name into nonsense. An #ifndef guard does not help --
// the macro is defined before those headers are read.
//
// Function templates instead. They satisfy every call site eMule has, they do
// not touch std::min/std::max, and common_type keeps the mixed-type calls
// (int against DWORD) that the macro handled by being untyped.
#include <type_traits>

template <typename A, typename B>
constexpr typename std::common_type<A, B>::type min(A a, B b)
{
    return (b < a) ? static_cast<typename std::common_type<A, B>::type>(b)
                   : static_cast<typename std::common_type<A, B>::type>(a);
}

template <typename A, typename B>
constexpr typename std::common_type<A, B>::type max(A a, B b)
{
    return (a < b) ? static_cast<typename std::common_type<A, B>::type>(b)
                   : static_cast<typename std::common_type<A, B>::type>(a);
}

// DEFINE_GUID in its DECLARING form -- the default. platform/initguid.h
// #undefs this and redefines it to allocate storage, which is the SDK's
// mechanism for having exactly one translation unit own the constants.
#ifndef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    extern "C" const GUID name
#endif
