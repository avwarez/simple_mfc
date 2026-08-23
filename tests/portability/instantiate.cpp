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
#include <ctime>

#if defined(_MSVC_LANG)
static_assert(_MSVC_LANG > 202002L, "simple_mfc is built as C++23 (MSVC: /std:c++latest)");
#else
static_assert(__cplusplus > 202002L, "simple_mfc is built as C++23 (-std=c++23 / -std=c++2b)");
#endif

static_assert(sizeof(BYTE) == 1, "BYTE is one byte");
static_assert(sizeof(WORD) == 2, "WORD is 16 bits");
static_assert(sizeof(DWORD) == 4, "DWORD is 32 bits on every target, LP64 included");
static_assert(sizeof(LONG) == 4, "LONG is 32 bits on every target, LP64 included");
static_assert(sizeof(LONGLONG) == 8, "LONGLONG is 64 bits");
static_assert(sizeof(ULONGLONG) == 8, "ULONGLONG is 64 bits");
static_assert(sizeof(BOOL) == 4, "BOOL is Win32's 32-bit int, not bool");
static_assert(sizeof(__time64_t) == 8, "CTime is 64-bit time, on 32-bit targets too");
static_assert(sizeof(std::time_t) >= 8, "CTime converts through time_t: a 32-bit one stops in 2038");
static_assert(sizeof(INT_PTR) == sizeof(void*), "INT_PTR is pointer-sized, as its name says");
static_assert(sizeof(EPOSITION) == sizeof(void*), "POSITION is a pointer-shaped cookie");

namespace {
struct Element : public ECObject { int n = 0; };
}

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

template class ECStringT<char>;
template class ECStringT<wchar_t>;

template class mfc_detail::ListImpl<void*>;
template class mfc_detail::ListImpl<ECString>;
template class mfc_detail::ArrayImpl<void*>;
template class mfc_detail::ArrayImpl<ECString>;
template class mfc_detail::CStringKeyMapImpl<void*, void*>;
template class mfc_detail::CStringKeyMapImpl<ECString, LPCTSTR>;

int main()
{
    ECWinThread thread;
    const int rc = thread.Run();

    EAFX_MODULE_THREAD_STATE* state = EAfxGetModuleThreadState();

    std::printf("instantiation OK: standard %ld (%s), pointer %d bytes, "
                "Run() -> %d, module thread state %s\n",
                static_cast<long>(
#if defined(_MSVC_LANG)
                    _MSVC_LANG
#else
                    __cplusplus
#endif
                ),
#if defined(_MSVC_LANG)
                _MSVC_LANG >= 202302L ? "C++23" : "C++2b",
#else
                __cplusplus >= 202302L ? "C++23" : "C++2b",
#endif
                static_cast<int>(sizeof(void*)), rc,
                state != nullptr ? "resolved" : "null");
    return state != nullptr ? 0 : 1;
}
