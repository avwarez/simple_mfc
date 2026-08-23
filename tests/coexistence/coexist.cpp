#if defined(SMFC_COEXIST_MFC_ONLY)
    #include <afx.h>
    #include <afxcoll.h>
    #include <afxtempl.h>
    #include <afxmt.h>
    #include <afxwin.h>
    #include <afxsock.h>
    #include <atltime.h>
#elif defined(SMFC_COEXIST_MFC_FIRST)
    #include <afx.h>
    #include <afxcoll.h>
    #include <afxtempl.h>
    #include <afxmt.h>
    #include <afxwin.h>
    #include <afxsock.h>
    #include <atltime.h>

    #include "eafx.h"
    #include "eafxcoll.h"
    #include "eafxtempl.h"
    #include "eafxmt.h"
    #include "eafxwin.h"
    #include "eafxsock.h"
    #include "eatltime.h"
#elif defined(SMFC_COEXIST_SMFC_FIRST)
    #include "eafx.h"
    #include "eafxcoll.h"
    #include "eafxtempl.h"
    #include "eafxmt.h"
    #include "eafxwin.h"
    #include "eafxsock.h"
    #include "eatltime.h"

    #include <afx.h>
    #include <afxcoll.h>
    #include <afxtempl.h>
    #include <afxmt.h>
    #include <afxwin.h>
    #include <afxsock.h>
    #include <atltime.h>
#else
    #error "Define one of SMFC_COEXIST_MFC_ONLY / _MFC_FIRST / _SMFC_FIRST"
#endif

#if !defined(SMFC_COEXIST_MFC_ONLY)
    #define SMFC_HAVE_SIMPLE_MFC 1
#endif

#include <cstdio>
#include <cstring>
#include <type_traits>

#ifdef FindNextFile
    #define SMFC_MACRO_FIND_NEXT_FILE 1
#else
    #define SMFC_MACRO_FIND_NEXT_FILE 0
#endif
#ifdef GetCurrentTime
    #define SMFC_MACRO_GET_CURRENT_TIME 1
#else
    #define SMFC_MACRO_GET_CURRENT_TIME 0
#endif
#ifdef UNICODE
    #define SMFC_MACRO_UNICODE 1
#else
    #define SMFC_MACRO_UNICODE 0
#endif
#ifdef _WINSOCKAPI_
    #define SMFC_MACRO_WINSOCKAPI 1
#else
    #define SMFC_MACRO_WINSOCKAPI 0
#endif

static unsigned long Win32TickCount()
{
#if SMFC_MACRO_GET_CURRENT_TIME
    return GetCurrentTime();
#else
    return 0;
#endif
}

static int Win32FindNextFileSpelling()
{
#if SMFC_MACRO_FIND_NEXT_FILE
    WIN32_FIND_DATAW data;
    HANDLE search = ::FindFirstFileW(L"C:\\Windows\\*", &data);
    if (search == INVALID_HANDLE_VALUE) return -1;
    int seen = 0;
    while (FindNextFile(search, &data)) ++seen;
    ::FindClose(search);
    return seen > 0 ? 1 : 0;
#else
    return -2;
#endif
}

static int MfcEnumerate()
{
    CFileFind finder;
    BOOL working = finder.FindFile(_T("C:\\Windows\\*"));
    int seen = 0;
    while (working)
    {
        ++seen;
        working = finder.FindNextFile();
    }
    finder.Close();
    return seen;
}

static int MfcCurrentYear()
{
    return CTime::GetCurrentTime().GetYear();
}

#if SMFC_HAVE_SIMPLE_MFC
static int SmfcEnumerate()
{
    ECFileFind finder;
    BOOL working = finder.FindFile(_T("C:\\Windows\\*"));
    int seen = 0;
    while (working)
    {
        ++seen;
        working = finder.FindNextFile();
    }
    finder.Close();
    return seen;
}

static int SmfcCurrentYear()
{
    return ECTime::GetCurrentTime().GetYear();
}
#endif

#ifdef FindNextFile
    #undef FindNextFile
#endif
#ifdef GetCurrentTime
    #undef GetCurrentTime
#endif

#define SMFC_DETECT_MEMBER(tag, member)                                      \
    template <class T, class = void>                                         \
    struct Has##tag : std::false_type {};                                    \
    template <class T>                                                       \
    struct Has##tag<T, std::void_t<decltype(&T::member)>> : std::true_type {}

namespace
{
SMFC_DETECT_MEMBER(FindNextFile, FindNextFile);
SMFC_DETECT_MEMBER(FindNextFileW, FindNextFileW);
SMFC_DETECT_MEMBER(GetCurrentTime, GetCurrentTime);
SMFC_DETECT_MEMBER(GetTickCount, GetTickCount);

void Line(const char* name, const char* value)
{
    std::printf("%s\t%s\n", name, value);
    std::fflush(stdout);
}

void LineInt(const char* name, long long value)
{
    std::printf("%s\t%lld\n", name, value);
    std::fflush(stdout);
}

void LineBool(const char* name, bool value)
{
    Line(name, value ? "true" : "false");
}

int g_records = 0;

struct Counted
{
    Counted(const char* n, const char* v)  { Line(n, v);     ++g_records; }
    Counted(const char* n, long long v)    { LineInt(n, v);  ++g_records; }
    Counted(const char* n, bool v)         { LineBool(n, v); ++g_records; }
};

#define REC(name, value) Counted(name, value)

CStringW ScratchDir()
{
    wchar_t temp[MAX_PATH] = {0};
    ::GetTempPathW(MAX_PATH, temp);
    CStringW dir(temp);
    dir += L"smfc_coexist";
    ::CreateDirectoryW(dir, nullptr);
    return dir;
}
}

static void ProbeRealMfc()
{
    REC("mfc.macro.FindNextFile.live",   SMFC_MACRO_FIND_NEXT_FILE != 0);
    REC("mfc.macro.GetCurrentTime.live", SMFC_MACRO_GET_CURRENT_TIME != 0);
    REC("mfc.macro.UNICODE.live",        SMFC_MACRO_UNICODE != 0);
    REC("mfc.macro._WINSOCKAPI_.live",   SMFC_MACRO_WINSOCKAPI != 0);

    REC("mfc.win32.GetCurrentTime.nonzero", Win32TickCount() != 0);
    REC("mfc.win32.FindNextFile.enumerates", (long long)Win32FindNextFileSpelling());

    REC("mfc.CFileFind.member.FindNextFile",  HasFindNextFile<CFileFind>::value);
    REC("mfc.CFileFind.member.FindNextFileW", HasFindNextFileW<CFileFind>::value);
    REC("mfc.CTime.member.GetCurrentTime",    HasGetCurrentTime<CTime>::value);
    REC("mfc.CTime.member.GetTickCount",      HasGetTickCount<CTime>::value);

    REC("mfc.CFileFind.enumerates", MfcEnumerate() > 0);
    REC("mfc.CTime.GetCurrentTime.plausible_year", MfcCurrentYear() >= 2020);

    REC("mfc.sizeof.CString", (long long)sizeof(CString));
    REC("mfc.sizeof.CObject", (long long)sizeof(CObject));
    REC("mfc.sizeof.CTime",   (long long)sizeof(CTime));
    REC("mfc.sizeof.CFile",   (long long)sizeof(CFile));

    CString s(_T("Coexistence"));
    s.MakeUpper();
    s += _T("/42");
    CStringA narrow(s);
    REC("mfc.CString.value", (const char*)narrow);
    REC("mfc.CString.GetLength", (long long)s.GetLength());

    CString formatted;
    formatted.Format(_T("%d|%s|%08lx"), -7, _T("x"), 48879UL);
    CStringA formattedNarrow(formatted);
    REC("mfc.CString.Format", (const char*)formattedNarrow);

    CTime when(2026, 8, 23, 14, 30, 0);
    CStringA stamp(when.Format(_T("%Y-%m-%dT%H:%M:%S")));
    REC("mfc.CTime.Format", (const char*)stamp);
    REC("mfc.CTimeSpan.GetTotalSeconds",
        (long long)(when - CTime(2026, 8, 23, 14, 0, 0)).GetTotalSeconds());

    CString path = ScratchDir() + _T("\\mfc.bin");
    CFile file;
    const BYTE payload[] = {1, 2, 3, 4, 5, 6, 7, 8};
    REC("mfc.CFile.Open", file.Open(path, CFile::modeCreate | CFile::modeReadWrite) != FALSE);
    file.Write(payload, sizeof payload);
    file.SeekToBegin();
    BYTE readback[sizeof payload] = {0};
    REC("mfc.CFile.Read", (long long)file.Read(readback, sizeof readback));
    REC("mfc.CFile.GetLength", (long long)file.GetLength());
    REC("mfc.CFile.roundtrip", memcmp(payload, readback, sizeof payload) == 0);
    file.Close();

    int caught = 0;
    try
    {
        AfxThrowFileException(CFileException::fileNotFound, -1, _T("nowhere"));
    }
    catch (CFileException* e)
    {
        caught = e->m_cause;
        e->Delete();
    }
    REC("mfc.CFileException.caught_cause", (long long)caught);
    REC("mfc.CFileException.ClassName",
        RUNTIME_CLASS(CFileException)->m_lpszClassName);

    CObList list;
    CFileException* held = new CFileException(CFileException::diskFull);
    list.AddTail(held);
    REC("mfc.CObList.GetCount", (long long)list.GetCount());
    REC("mfc.CObList.IsKindOf",
        list.GetHead()->IsKindOf(RUNTIME_CLASS(CException)) != FALSE);
    held->Delete();

    CMapStringToPtr map;
    map.SetAt(_T("k"), (void*)0x1234);
    void* found = nullptr;
    REC("mfc.CMapStringToPtr.Lookup", map.Lookup(_T("k"), found) != FALSE);
    REC("mfc.CMapStringToPtr.value", (long long)(ULONG_PTR)found);

    ::DeleteFileW(path);
}

#if SMFC_HAVE_SIMPLE_MFC
static void ProbeSimpleMfc()
{
    REC("smfc.CFileFind.member.FindNextFile",  HasFindNextFile<ECFileFind>::value);
    REC("smfc.CFileFind.member.FindNextFileW", HasFindNextFileW<ECFileFind>::value);
    REC("smfc.CTime.member.GetCurrentTime",    HasGetCurrentTime<ECTime>::value);
    REC("smfc.CTime.member.GetTickCount",      HasGetTickCount<ECTime>::value);

    REC("smfc.CFileFind.enumerates", SmfcEnumerate() > 0);
    REC("smfc.CTime.GetCurrentTime.plausible_year", SmfcCurrentYear() >= 2020);

    ECString s(_T("Coexistence"));
    s.MakeUpper();
    s += _T("/42");
    REC("smfc.CString.GetLength", (long long)s.GetLength());

    ECString formatted;
    formatted.Format(_T("%d|%s|%08lx"), -7, _T("x"), 48879UL);
    REC("smfc.CString.Format.GetLength", (long long)formatted.GetLength());

    ECTime when(2026, 8, 23, 14, 30, 0);
    REC("smfc.CTime.Format.GetLength",
        (long long)when.Format(_T("%Y-%m-%dT%H:%M:%S")).GetLength());

    ECString dir((LPCTSTR)ScratchDir());
    ECString path = dir + _T("\\smfc.bin");
    ECFile file;
    const BYTE payload[] = {1, 2, 3, 4, 5, 6, 7, 8};
    REC("smfc.CFile.Open",
        file.Open(path, ECFile::modeCreate | ECFile::modeReadWrite) != FALSE);
    file.Write(payload, sizeof payload);
    file.SeekToBegin();
    BYTE readback[sizeof payload] = {0};
    REC("smfc.CFile.Read", (long long)file.Read(readback, sizeof readback));
    REC("smfc.CFile.roundtrip", memcmp(payload, readback, sizeof payload) == 0);
    file.Close();

    int caught = 0;
    try
    {
        EAfxThrowFileException(ECFileException::fileNotFound, -1, _T("nowhere"));
    }
    catch (ECFileException* e)
    {
        caught = e->m_cause;
        delete e;
    }
    REC("smfc.CFileException.caught_cause", (long long)caught);

    bool mfcCatchSawOurs = false;
    try
    {
        EAfxThrowFileException(ECFileException::diskFull);
    }
    catch (CFileException* e)
    {
        mfcCatchSawOurs = true;
        e->Delete();
    }
    catch (ECFileException* e)
    {
        delete e;
    }
    REC("smfc.CFileException.caught_by_mfc_handler", mfcCatchSawOurs);

    bool ourCatchSawMfc = false;
    try
    {
        AfxThrowFileException(CFileException::diskFull);
    }
    catch (ECFileException* e)
    {
        ourCatchSawMfc = true;
        delete e;
    }
    catch (CFileException* e)
    {
        e->Delete();
    }
    REC("smfc.mfc_exception.caught_by_our_handler", ourCatchSawMfc);

    CString fromOurs((LPCTSTR)s);
    ECString fromTheirs((LPCTSTR)fromOurs);
    REC("smfc.interop.CString_roundtrip", fromTheirs == s);

    {
        ECString path = dir + _T("\\interop_mfc_to_smfc.bin");
        CFile out;
        out.Open((LPCTSTR)path, CFile::modeCreate | CFile::modeWrite);
        CArchive store(&out, CArchive::store);
        store << (BYTE)0xA5 << (WORD)0x1234 << (DWORD)0xDEADBEEFUL
              << (int)-7 << (double)1.5 << (ULONGLONG)0x0102030405060708ULL;
        store.Close();
        out.Close();

        ECFile in;
        in.Open(path, ECFile::modeRead);
        ECArchive load(&in, ECArchive::load);
        BYTE by = 0; WORD w = 0; DWORD dw = 0; int i = 0;
        double d = 0; ULONGLONG q = 0;
        load >> by >> w >> dw >> i >> d >> q;
        load.Close();
        in.Close();
        REC("smfc.interop.CArchive.mfc_to_smfc",
            by == 0xA5 && w == 0x1234 && dw == 0xDEADBEEFUL && i == -7
                && d == 1.5 && q == 0x0102030405060708ULL);
        ::DeleteFileW((LPCTSTR)path);
    }
    {
        ECString path = dir + _T("\\interop_smfc_to_mfc.bin");
        ECFile out;
        out.Open(path, ECFile::modeCreate | ECFile::modeWrite);
        ECArchive store(&out, ECArchive::store);
        store << (BYTE)0xA5 << (WORD)0x1234 << (DWORD)0xDEADBEEFUL
              << (int)-7 << (double)1.5 << (ULONGLONG)0x0102030405060708ULL;
        store.Close();
        out.Close();

        CFile in;
        in.Open((LPCTSTR)path, CFile::modeRead);
        CArchive load(&in, CArchive::load);
        BYTE by = 0; WORD w = 0; DWORD dw = 0; int i = 0;
        double d = 0; ULONGLONG q = 0;
        load >> by >> w >> dw >> i >> d >> q;
        load.Close();
        in.Close();
        REC("smfc.interop.CArchive.smfc_to_mfc",
            by == 0xA5 && w == 0x1234 && dw == 0xDEADBEEFUL && i == -7
                && d == 1.5 && q == 0x0102030405060708ULL);
        ::DeleteFileW((LPCTSTR)path);
    }

    ECObList list;
    ECFileException* held = new ECFileException(ECFileException::diskFull);
    list.AddTail(held);
    REC("smfc.CObList.GetCount", (long long)list.GetCount());
    REC("smfc.CObList.IsKindOf",
        list.GetHead()->IsKindOf(ERUNTIME_CLASS(ECException)) != FALSE);
    delete held;

    ::DeleteFileW((LPCTSTR)path);
}
#endif

int main()
{
    ProbeRealMfc();
#if SMFC_HAVE_SIMPLE_MFC
    ProbeSimpleMfc();
#endif
    std::printf("#END\t%d\n", g_records);
    std::fflush(stdout);
    return 0;
}
