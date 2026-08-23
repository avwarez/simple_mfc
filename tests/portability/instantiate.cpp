// Portability probe: does every declaration in this branch actually
// COMPILE, and LINK, on this platform?
//
// The conformance suite (../conformance) answers a different question --
// does a method behave like real MFC's -- and it answers it only for the
// methods it calls, on the one platform where real MFC exists. Two gaps
// follow from that, and this file closes both:
//
//   1. A member of a class TEMPLATE is not compiled until something
//      instantiates it. CArray/CList/CMap/CTypedPtr*/CTempBuffer/
//      CSimpleArray/CRBMap/CStringT are templates, so a body that does not
//      compile at all sits there undetected until the first caller shows
//      up -- in eMule, months later. The explicit instantiations below
//      force every member of every one of them to be compiled here.
//
//   2. A member declared and never defined is a link error, not a compile
//      error, and only at the first odr-use. That is exactly how
//      LoadString, AllocSysString and AfxDynamicDownCast sat broken in
//      this branch: declared, never defined, and nothing referenced them.
//      Taking the address of the members no conformance case reaches
//      makes the linker demand them here.
//
// It also pins, per platform, the two facts the rest of the library is
// built on: the language standard, and the widths of the Win32-shaped
// integer types whose whole point is to be the same width everywhere.
#include "eafx.h"
#include "eafxcoll.h"
#include "eafximpl.h"
#include "eafxinet.h"
#include "eafxmt.h"
#include "eafxsock.h"
#include "eafxtempl.h"
#include "eafxwin.h"
#include "eatlalloc.h"
#include "eatlbase.h"
#include "eatlcoll.h"
#include "eatlconv.h"
#include "eatlenc.h"
#include "eatlsimpcoll.h"
#include "eatltime.h"
#include "esockimpl.h"

#include <cstdio>

// ---------------------------------------------------------------------
// 1. The language standard.
//
// MSVC reports the real value in _MSVC_LANG; its __cplusplus stays at
// 199711 unless /Zc:__cplusplus is passed, so asking __cplusplus alone
// would silently pass on the one compiler most likely to be configured
// with an older standard.
// ---------------------------------------------------------------------
#if defined(_MSVC_LANG)
static_assert(_MSVC_LANG >= 202302L, "simple_mfc is built as C++23 (MSVC: /std:c++latest)");
#else
static_assert(__cplusplus >= 202302L, "simple_mfc is built as C++23 (-std=c++23)");
#endif

// ---------------------------------------------------------------------
// 2. The Win32-shaped integer widths.
//
// These are not internal details: they are the widths of eMule's on-disk
// and on-wire formats (known.met, the eDonkey packet headers), which other
// clients read. A DWORD that is 8 bytes wide on some target writes a file
// nothing else can parse -- and nothing would report an error. See the
// LP64/LLP64 note in eafx.h.
// ---------------------------------------------------------------------
static_assert(sizeof(BYTE) == 1, "BYTE is one byte");
static_assert(sizeof(WORD) == 2, "WORD is 16 bits");
static_assert(sizeof(DWORD) == 4, "DWORD is 32 bits on every target, LP64 included");
static_assert(sizeof(LONG) == 4, "LONG is 32 bits on every target, LP64 included");
static_assert(sizeof(LONGLONG) == 8, "LONGLONG is 64 bits");
static_assert(sizeof(ULONGLONG) == 8, "ULONGLONG is 64 bits");
static_assert(sizeof(BOOL) == 4, "BOOL is Win32's 32-bit int, not bool");
static_assert(sizeof(__time64_t) == 8, "CTime is 64-bit time, on 32-bit targets too");
static_assert(sizeof(INT_PTR) == sizeof(void*), "INT_PTR is pointer-sized, as its name says");
static_assert(sizeof(EPOSITION) == sizeof(void*), "POSITION is a pointer-shaped cookie");

// ---------------------------------------------------------------------
// 3. Every class template, instantiated in full.
//
// An explicit instantiation DEFINITION compiles every member of the class
// for that argument list, whether or not anything calls it. The argument
// lists below are the shapes eMule actually uses: a value type, a string
// type, and a pointer type for each container.
// ---------------------------------------------------------------------
namespace {
struct Element : public ECObject { int n = 0; };
} // namespace

template class ECArray<int>;
template class ECArray<ECString, LPCTSTR>;
template class ECArray<Element*>;

template class ECList<int>;
template class ECList<ECString, LPCTSTR>;
template class ECList<Element*>;

template class ECMap<UINT, UINT, UINT, UINT>;
template class ECMap<LPCTSTR, LPCTSTR, ECString, LPCTSTR>;
template class ECMap<UINT, UINT, Element*, Element*>;

template class ECTypedPtrList<ECPtrList, Element*>;
template class ECTypedPtrList<ECObList, Element*>;
template class ECTypedPtrArray<ECPtrArray, Element*>;

template class ECTempBuffer<BYTE>;
template class ECTempBuffer<wchar_t, 8>;

template class ECSimpleArray<int>;
template class ECSimpleArray<Element*>;

template class ECRBMap<UINT, UINT>;

// Both character types: CStringA is not a curiosity, it is what eMule's
// narrow-string paths (Base64, the HTTP layer) are written against.
template class ECStringT<char>;
template class ECStringT<wchar_t>;

// The internal templates the concrete afxcoll.h collections derive from or
// hold: their members are instantiated only as their public wrappers call
// them, so a body no wrapper reaches is unchecked without this.
template class mfc_detail::ListImpl<void*>;
template class mfc_detail::ListImpl<ECString>;
template class mfc_detail::ArrayImpl<void*>;
template class mfc_detail::ArrayImpl<ECString>;
template class mfc_detail::CStringKeyMapImpl<void*, void*>;
template class mfc_detail::CStringKeyMapImpl<ECString, LPCTSTR>;

// ---------------------------------------------------------------------
// 4. The two members no conformance case can reach.
//
// CWinThread::Run is MFC's message pump, so a probe that called the real
// one would never return; AfxGetModuleThreadState is declared in MFC's
// private afxstat_.h, so the real-MFC probe has nothing to compare
// against. Neither has a conformance case -- which is exactly why they are
// called here, where the only question asked is whether they link and
// return. (This branch's Run is not a pump: with no message queue to
// drain it returns immediately, so calling it is safe.)
// ---------------------------------------------------------------------
int main()
{
    ECWinThread thread;             // constructed only: nothing is started
    const int rc = thread.Run();

    EAFX_MODULE_THREAD_STATE* state = EAfxGetModuleThreadState();

    std::printf("instantiation OK: standard %ld, pointer %d bytes, "
                "Run() -> %d, module thread state %s\n",
                static_cast<long>(
#if defined(_MSVC_LANG)
                    _MSVC_LANG
#else
                    __cplusplus
#endif
                ),
                static_cast<int>(sizeof(void*)), rc,
                state != nullptr ? "resolved" : "null");
    return state != nullptr ? 0 : 1;
}
