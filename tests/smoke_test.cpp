// Minimal smoke test: verifies that the library compiles, links and works
// for the pieces that are actually implemented (afx.h, afxcoll.h,
// afxtempl.h, afxmt.h, atltime.h, atltypes.h, atlenc.h, atlconv.h,
// atlalloc.h, afximpl.h, afxinet.h). Not an exhaustive test suite.
#include "afx.h"
#include "afxcoll.h"
#include "afxtempl.h"
#include "afxmt.h"
#include "atltime.h"
#include "atltypes.h"
#include "atlenc.h"
#include "atlconv.h"
#include "atlalloc.h"
#include "afximpl.h"
#include "afxinet.h"

// "Declaration-only" headers (no .cpp): here we only check that they are
// includable and that the types/hierarchies compile, without actually
// using them.
#include "afxwin.h"
#include "afxext.h"
#include "afxdlgs.h"
#include "afxdd_.h"
#include "afxcmn.h"
#include "afxole.h"
#include "afxdhtml.h"
#include "afxdtctl.h"
#include "afxsock.h"

#include <cstdio>
#include <cstring>
#include <sstream>

#ifdef _MSC_VER
#include <crtdbg.h>
#endif

// CHECK, not assert(): <cassert>'s assert is compiled out entirely by
// NDEBUG, which CMake defines in Release/RelWithDebInfo. With assert the
// Release run of this executable checked *nothing* and reported success
// unconditionally, so only the Debug job was ever a real test. CHECK is
// active in every configuration, reports every failure instead of dying
// on the first, and makes the process exit non-zero so CTest sees it.
static int g_failures = 0;
#define CHECK(expr)                                                        \
    do                                                                     \
    {                                                                      \
        if (!(expr))                                                       \
        {                                                                  \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__,   \
                         #expr);                                           \
            ++g_failures;                                                  \
        }                                                                  \
    } while (0)

// On Windows a failing assert/abort inside the debug CRT reports through
// _CrtDbgReport, whose default destination for _CRT_ASSERT and _CRT_ERROR
// is _CRTDBG_MODE_WNDW: a MODAL MESSAGE BOX. On a headless CI runner
// nobody ever clicks it, so the process never exits and the job hangs
// until the runner's own limit kills it -- which is exactly what happened
// here (two consecutive "build (Debug)" jobs cancelled after ~5 hours,
// while "build (Release)", where NDEBUG had removed the assertions, passed
// in under a minute). Route every CRT report to stderr instead, so a
// failure is a fast, readable failure rather than a hang.
static void SilenceWindowsCrtDialogs()
{
#ifdef _MSC_VER
    for (int report : {_CRT_WARN, _CRT_ERROR, _CRT_ASSERT})
    {
        _CrtSetReportMode(report, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(report, _CRTDBG_FILE_STDERR);
    }
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
}

int main()
{
    SilenceWindowsCrtDialogs();

    CString s = L"  simple_mfc  ";
    s.Trim();
    CHECK(s == CString(L"simple_mfc"));

    CObList list;
    CObject a, b;
    list.AddTail(&a);
    list.AddTail(&b);
    CHECK(list.GetCount() == 2);

    CArray<int> arr;
    arr.Add(42);
    CHECK(arr[0] == 42);

    // CMap<CString, LPCTSTR, ...> (the CMapStringToPtr/CMapStringToString
    // idiom): guards against the identity-hash-on-a-throwaway-CString bug
    // (HashKey<LPCTSTR> content specialization in afxtempl.h) -- without
    // it, GetCount() still looks right (every insert lands *somewhere*),
    // but Lookup/RemoveKey on a key inserted moments earlier silently
    // reports "not found".
    CMap<CString, LPCTSTR, int, int> smap;
    smap.SetAt(L"one", 1);
    smap.SetAt(L"two", 2);
    int mapVal = 0;
    CHECK(smap.Lookup(L"two", mapVal) != FALSE);
    CHECK(mapVal == 2);
    CHECK(smap.RemoveKey(L"one") != FALSE);
    CHECK(smap.GetCount() == 1);

    CCriticalSection cs;
    {
        CSingleLock lk(&cs, TRUE);
        CHECK(lk.IsLocked());
    }

    CTime t = CTime::GetCurrentTime();
    (void)t;

    // CObject::Dump / CDumpContext (found via the qualified-call blind-spot
    // rescan: CObject::Dump(dc) super-calls in eMule/srchybrid).
    std::wostringstream oss;
    CDumpContext dcTest(oss);
    CObject dumpObj;
    dumpObj.Dump(dcTest);
    CHECK(oss.str() == L"CObject");

    // CFileException::ThrowOsError (static factory, found the same way).
    try
    {
        CFileException::ThrowOsError(2 /*ERROR_FILE_NOT_FOUND*/, L"missing.txt");
        CHECK(false && "ThrowOsError must throw");
    }
    catch (CFileException* e)
    {
        CHECK(e->m_cause == CFileException::fileNotFound);
        CHECK(e->m_strFileName == CString(L"missing.txt"));
        e->Delete();
    }

    // CPoint / CSize / CRect (atltypes.h) -- pure coordinate arithmetic,
    // no GDI/HWND involved.
    CPoint p1(10, 20), p2(3, 4);
    CPoint pSum = p1 + p2;
    CHECK(pSum.x == 13 && pSum.y == 24);
    p1.Offset(1, 1);
    CHECK(p1.x == 11 && p1.y == 21);

    CSize sz(5, 7);
    CHECK((p2 + sz).x == 8 && (p2 + sz).y == 11);

    CRect rc(0, 0, 100, 50);
    CHECK(rc.Width() == 100 && rc.Height() == 50);
    CHECK(rc.PtInRect(CPoint(50, 25)) != FALSE);
    CHECK(rc.PtInRect(CPoint(100, 50)) == FALSE); // right/bottom exclusive
    rc.OffsetRect(10, 10);
    CHECK(rc.left == 10 && rc.top == 10 && rc.right == 110 && rc.bottom == 60);
    rc.InflateRect(5, 5);
    CHECK(rc.left == 5 && rc.right == 115);
    CHECK(rc.TopLeft().x == rc.left && rc.TopLeft().y == rc.top);
    CHECK(rc.BottomRight().x == rc.right && rc.BottomRight().y == rc.bottom);

    CRect rcA(0, 0, 10, 10), rcB(5, 5, 15, 15);
    CRect rcI = rcA & rcB;
    CHECK(rcI.left == 5 && rcI.top == 5 && rcI.right == 10 && rcI.bottom == 10);
    CRect rcU = rcA | rcB;
    CHECK(rcU.left == 0 && rcU.top == 0 && rcU.right == 15 && rcU.bottom == 15);

    // CArchive (afx.h), built on the already-implemented CMemFile: a
    // store/load round trip through the primitive-type operators, which
    // is the exact pattern eMule's own part-file metadata code uses.
    {
        CMemFile mf;
        CArchive arStore(&mf, CArchive::store);
        int nTotal = 42;
        UINT nRemaining = 7;
        DWORD nFragments = 3;
        arStore << nTotal << nRemaining << nFragments;
        arStore.Close();

        mf.SeekToBegin();
        CArchive arLoad(&mf, CArchive::load);
        int gotTotal = 0;
        UINT gotRemaining = 0;
        DWORD gotFragments = 0;
        arLoad >> gotTotal >> gotRemaining >> gotFragments;
        CHECK(gotTotal == 42 && gotRemaining == 7 && gotFragments == 3);
        arLoad.Close();
    }

    // Base64Encode / Base64EncodeGetRequiredLength (atlenc.h).
    {
        const char* src = "Hello, MFC!"; // -> "SGVsbG8sIE1GQyE="
        int nSrcLen = static_cast<int>(std::strlen(src));
        int nNeeded = Base64EncodeGetRequiredLength(nSrcLen, ATL_BASE64_FLAG_NOCRLF);
        std::vector<char> dst(static_cast<size_t>(nNeeded) + 1, 0);
        int nOutLen = nNeeded;
        BOOL ok = Base64Encode(reinterpret_cast<const BYTE*>(src), nSrcLen, dst.data(), &nOutLen, ATL_BASE64_FLAG_NOCRLF);
        CHECK(ok != FALSE);
        dst[static_cast<size_t>(nOutLen)] = 0;
        CHECK(std::strcmp(dst.data(), "SGVsbG8sIE1GQyE=") == 0);
    }

    // AtlUnicodeToUTF8 (atlconv.h): the two-pass "measure then fill" form
    // eMule's StringConversion.cpp uses.
    {
        // \u00e9, not a literal e-acute: this file is UTF-8 without a BOM, and
        // MSVC (absent /utf-8, which CMakeLists.txt now passes) decodes
        // such a source in the machine's ANSI code page instead. On the
        // CI runner (CP1252) the two UTF-8 bytes C3 A9 were therefore read
        // as the TWO characters U+00C3 U+00A9, making this literal 5 chars
        // long rather than 4 -- so nSrcChars=5 stopped covering the
        // terminator, the conversion produced an unterminated buffer, and
        // the check below failed. A universal-character-name means the
        // same thing under every source encoding.
        const wchar_t* wsrc = L"caf\u00e9"; // "caf\xc3\xa9" in UTF-8
        int nSrcChars = 5; // 4 chars + terminator, matching the -1 convention below
        int nNeeded = AtlUnicodeToUTF8(wsrc, nSrcChars, nullptr, 0);
        std::vector<char> dst(static_cast<size_t>(nNeeded), 0);
        int nOutLen = AtlUnicodeToUTF8(wsrc, nSrcChars, dst.data(), nNeeded);
        CHECK(nOutLen == nNeeded);
        CHECK(std::strcmp(dst.data(), "caf\xc3\xa9") == 0);
    }

    // CTempBuffer<T> (atlalloc.h): both the fixed (stack) and heap paths.
    {
        // Not named "small": that's a legacy MIDL typedef for char,
        // declared in <rpcndr.h> (pulled in transitively by <oleauto.h>
        // on _WIN32), and collides as a spurious redeclaration.
        CTempBuffer<int, 64> smallBuf; // 64 bytes fixed => fits 16 ints
        smallBuf.Allocate(4);
        for (int i = 0; i < 4; ++i) smallBuf[i] = i * i;
        CHECK(smallBuf[3] == 9);

        CTempBuffer<int, 16> big; // 16 bytes fixed => only 4 ints; ask for more
        big.Allocate(100);
        for (int i = 0; i < 100; ++i) big[i] = i;
        CHECK(big[99] == 99);

        // Growing across the fixed->heap boundary keeps what was already
        // written. Pinned down here rather than in the conformance suite
        // on purpose: ATL documents no contract either way for
        // Reallocate, so this is a promise simple_mfc makes about itself,
        // not a behavior real ATL can be held to.
        CTempBuffer<int, 16> grow; // 4 ints on the stack
        grow.Allocate(4);
        for (int i = 0; i < 4; ++i) grow[i] = 100 + i;
        grow.Reallocate(64); // forces the move to the heap
        for (int i = 0; i < 4; ++i) CHECK(grow[i] == 100 + i);
    }

    // AfxParseURL (afxinet.h).
    {
        DWORD svc = 0;
        CString server, object;
        INTERNET_PORT port = 0;
        BOOL ok = AfxParseURL(L"https://example.com:8443/path/to/file", svc, server, object, port);
        CHECK(ok != FALSE);
        CHECK(svc == AFX_INET_SERVICE_HTTPS);
        CHECK(server == CString(L"example.com"));
        CHECK(object == CString(L"/path/to/file"));
        CHECK(port == 8443);
    }

    // CMemFile::Detach/Attach (afx.h).
    {
        CMemFile mf;
        const char payload[] = "detach-me";
        mf.Write(payload, sizeof(payload) - 1);
        BYTE* pRaw = mf.Detach();
        CHECK(mf.GetLength() == 0);

        CMemFile mf2;
        mf2.Attach(pRaw, sizeof(payload) - 1);
        CHECK(mf2.GetLength() == sizeof(payload) - 1);
        mf2.SeekToBegin();
        char buf[16] = {};
        mf2.Read(buf, sizeof(payload) - 1);
        CHECK(std::strcmp(buf, "detach-me") == 0);
    }

    // AfxGetModuleThreadState (afximpl.h): non-null, stable within a thread.
    {
        AFX_MODULE_THREAD_STATE* st1 = AfxGetModuleThreadState();
        AFX_MODULE_THREAD_STATE* st2 = AfxGetModuleThreadState();
        CHECK(st1 != nullptr && st1 == st2);
    }

    CWnd* w = nullptr; (void)w;
    CDC* dc = nullptr; (void)dc;
    CPropertySheet* sheet = nullptr; (void)sheet;
    CTreeCtrl* tree = nullptr; (void)tree;
    CImageList* iml = nullptr; (void)iml;
    COleDropTarget* dropTarget = nullptr; (void)dropTarget;
    CDHtmlDialog* dhtml = nullptr; (void)dhtml;
    CDateTimeCtrl* dtp = nullptr; (void)dtp;
    CAsyncSocket* sock = nullptr; (void)sock;
    // CPalette went from an incomplete forward declaration to a real
    // class definition during the FRONTEND/GDI blind-spot pass (see
    // ../../mfc_scan_srchybrid.md addendum) — checked here like the
    // other declaration-only GUI/GDI classes above.
    CPalette* pal = nullptr; (void)pal;

    if (g_failures != 0)
    {
        std::printf("simple_mfc smoke test: %d FAILED check(s)\n", g_failures);
        return 1;
    }
    std::printf("simple_mfc smoke test: ALL OK\n");
    return 0;
}
