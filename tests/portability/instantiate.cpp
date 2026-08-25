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
#include "eatltypes.h"

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

static_assert(sizeof(POINT) == 8, "a Win32 POINT is two 32-bit LONGs, on LP64 too");
static_assert(sizeof(SIZE) == 8, "a Win32 SIZE is two 32-bit LONGs, on LP64 too");
static_assert(sizeof(RECT) == 16, "a Win32 RECT is four 32-bit LONGs, on LP64 too");
static_assert(sizeof(ECPoint) == sizeof(POINT), "ECPoint adds no storage to POINT");
static_assert(sizeof(ECSize) == sizeof(SIZE), "ECSize adds no storage to SIZE");
static_assert(sizeof(ECRect) == sizeof(RECT), "ECRect adds no storage to RECT");
static_assert(std::is_standard_layout<ECRect>::value,
              "CRect::TopLeft/BottomRight reinterpret the rect as two POINTs");

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
template class ECTempBuffer<TCHAR, 8>;

template class ECSimpleArray<int>;
template class ECSimpleArray<Element*>;

template class ECRBMap<UINT, UINT>;

template class ECStringT<char>;
template class ECStringT<wchar_t>;
template class ECStringT<char16_t>;

template class mfc_detail::ListImpl<void*>;
template class mfc_detail::ListImpl<ECString>;
template class mfc_detail::ArrayImpl<void*>;
template class mfc_detail::ArrayImpl<ECString>;
template class mfc_detail::CStringKeyMapImpl<void*, void*>;
template class mfc_detail::CStringKeyMapImpl<ECString, LPCTSTR>;

namespace {
int Utf16Checks()
{
    using U16 = ECStringT<char16_t>;
    int bad = 0;

    U16 emoji(u"a\U0001F600b");
    if (emoji.GetLength() != 4) ++bad;
    if (emoji.GetAt(1) != 0xD83D) ++bad;
    if (emoji.GetAt(2) != 0xDE00) ++bad;
    if (emoji.Find(u'b') != 3) ++bad;
    if (emoji.ReverseFind(u'b') != 3) ++bad;
    if (emoji.Mid(1, 1).GetLength() != 1) ++bad;
    if (emoji.Mid(1, 1).GetAt(0) != 0xD83D) ++bad;

    U16 wrapped;
    wrapped.Format(u"<%s>", emoji.GetString());
    if (wrapped.GetLength() != 6) ++bad;

    U16 dialect;
    dialect.Format(u"%I64u|%Iu|%hs|%08lx", (unsigned long long)1, (size_t)2, "n", 48879UL);
    if (dialect != U16(u"1|2|n|0000beef")) ++bad;

    U16 padded;
    padded.Format(u"[%-4s][%5d]", u"ab", 42);
    if (padded != U16(u"[ab  ][   42]")) ++bad;

    U16 grown;
    grown.Format(u"%s", std::u16string(600, u'x').c_str());
    if (grown.GetLength() != 600) ++bad;

    U16 upper(u"a\U0001F600b");
    upper.MakeUpper();
    if (upper != U16(u"A\U0001F600B")) ++bad;

    const wchar_t* wide = L"a\U0001F600b";
    const size_t wideLen = std::char_traits<wchar_t>::length(wide);
    const std::u16string asUtf16 = mfc_detail::WideToWide<char16_t>(wide, wideLen);
    if (asUtf16.size() != 4) ++bad;
    if (asUtf16[1] != 0xD83D || asUtf16[2] != 0xDE00) ++bad;

    const std::wstring backAgain =
        mfc_detail::WideToWide<wchar_t>(asUtf16.data(), asUtf16.size());
    if (backAgain.size() != wideLen) ++bad;
    if (backAgain != std::wstring(wide)) ++bad;

    const U16 fromNarrow("abc", 3);
    if (fromNarrow.GetLength() != 3 || fromNarrow.GetAt(2) != u'c') ++bad;

    return bad;
}
}

int main()
{
    ECWinThread thread;
    const int rc = thread.Run();

    const void* trace = &EtraceAppMsg;

    std::printf("instantiation OK: standard %ld (%s), pointer %d bytes, "
                "Run() -> %d, trace category %s\n",
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
                trace != nullptr ? "resolved" : "null");
    const int utf16Bad = Utf16Checks();
    std::printf("UTF-16 string checks: %d failed\n", utf16Bad);

    return (trace != nullptr && utf16Bad == 0) ? 0 : 1;
}
