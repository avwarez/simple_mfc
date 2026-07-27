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
#include "atlsimpcoll.h"
#include "atlcoll.h"
#include "afximpl.h"
#include "afxinet.h"

// afxsock.h (CAsyncSocket) is a real, linked implementation and gets a
// functional loopback test below, not just an includability check.
#include "afxsock.h"

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

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <thread>

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

// ---------------------------------------------------------------------
// CAsyncSocket functional tests. All over 127.0.0.1 loopback, which does
// not drop or reorder, so the blocking receives below cannot hang the way
// a real network could. Each returns true on success.
// ---------------------------------------------------------------------
static bool TestSyncTcp()
{
    CAsyncSocket listener;
    if (!listener.Create(0, SOCK_STREAM, 0, L"127.0.0.1"))
        return false;
    if (!listener.Listen(1))
        return false;
    CString addr;
    UINT port = 0;
    if (!listener.GetSockName(addr, port) || port == 0)
        return false;

    CAsyncSocket client;
    if (!client.Create(0, SOCK_STREAM, 0, L"127.0.0.1"))
        return false;
    if (!client.Connect(L"127.0.0.1", port)) // blocking connect over loopback
        return false;

    CAsyncSocket accepted;
    if (!listener.Accept(accepted))
        return false;

    const char msg[] = "ping";
    if (client.Send(msg, 4) != 4)
        return false;
    char buf[8] = {0};
    if (accepted.Receive(buf, 4) != 4)
        return false;
    return std::memcmp(buf, "ping", 4) == 0;
}

static bool TestSyncUdp()
{
    CAsyncSocket receiver;
    if (!receiver.Create(0, SOCK_DGRAM, 0, L"127.0.0.1"))
        return false;
    CString addr;
    UINT port = 0;
    if (!receiver.GetSockName(addr, port) || port == 0)
        return false;

    CAsyncSocket sender;
    if (!sender.Create(0, SOCK_DGRAM, 0, L"127.0.0.1"))
        return false;
    const char msg[] = "hello";
    if (sender.SendTo(msg, 5, port, L"127.0.0.1") != 5)
        return false;

    char buf[16] = {0};
    CString from;
    UINT fromPort = 0;
    const int n = receiver.ReceiveFrom(buf, sizeof(buf), from, fromPort);
    return n == 5 && std::memcmp(buf, "hello", 5) == 0;
}

// Exercises the async reactor: the derived socket's OnReceive must fire on
// the reactor thread after a datagram arrives, without any message pump.
namespace {
struct AsyncReceiver : public CAsyncSocket
{
    std::atomic<bool> got{false};
    char buf[16] = {0};
    int n = 0;
    void OnReceive(int /*nErrorCode*/) override
    {
        n = Receive(buf, sizeof(buf));
        if (n > 0)
            got = true;
    }
};
} // namespace

static bool TestAsyncUdp()
{
    AsyncReceiver receiver;
    if (!receiver.Create(0, SOCK_DGRAM, FD_READ)) // async (non-blocking) socket
        return false;
    CString addr;
    UINT port = 0;
    if (!receiver.GetSockName(addr, port) || port == 0)
    {
        receiver.Close();
        return false;
    }

    CAsyncSocket sender;
    if (!sender.Create(0, SOCK_DGRAM, 0, L"127.0.0.1"))
    {
        receiver.Close();
        return false;
    }
    sender.SendTo("async", 5, port, L"127.0.0.1");

    // Wait up to ~3s for the reactor (100 ms poll) to deliver OnReceive.
    for (int i = 0; i < 60 && !receiver.got; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

    const bool ok = receiver.got && receiver.n == 5 &&
                    std::memcmp(receiver.buf, "async", 5) == 0;
    // Close() before the object is destroyed: it removes the socket from
    // the reactor under the reactor lock, so it blocks out any in-flight
    // OnReceive and no callback can touch a half-destroyed object.
    receiver.Close();
    return ok;
}

int main()
{
    SilenceWindowsCrtDialogs();

    CString s = L"  simple_mfc  ";
    s.Trim();
    CHECK(s == CString(L"simple_mfc"));

    // ---------------------------------------------------------------------
    // CString::Format / AppendFormat. eMule calls these ~1500 times and
    // writes %s ~2800 times, so this is the single most-used method in the
    // library -- and it had no coverage at all until a wrong %s surfaced by
    // hand in a running application.
    //
    // The trap is that MSVC's CRT and the C standard disagree, silently,
    // about what %s means in a WIDE format string: MSVC reads a wchar_t*,
    // glibc reads a char*. Fed a wide string, glibc stops at its first
    // embedded NUL, so L"world" formats as "w" -- right length, right type,
    // no warning, no crash, just the first letter.
    //
    // These assertions encode the MFC dialect the application is written
    // against. They therefore run identically on MSVC (where the CRT already
    // behaves this way) and on GCC/Clang (where the format string has to be
    // translated first), and the Windows build is what proves the
    // expectations themselves are right rather than merely self-consistent.
    // ---------------------------------------------------------------------
    {
        CString f;

        // %s: a wide string, in every form the application writes it.
        f.Format(_T("Hello, %s!"), _T("world"));
        CHECK(f == CString(_T("Hello, world!")));

        const CString name(_T("world"));
        f.Format(_T("Hello, %s!"), (LPCTSTR)name);
        CHECK(f == CString(_T("Hello, world!")));

        // Two of them, so a truncation at the first would still show up.
        f.Format(_T("%s-%s"), _T("ab"), _T("cd"));
        CHECK(f == CString(_T("ab-cd")));

        // %S is the OTHER character type: a narrow string in a wide format.
        f.Format(_T("[%S]"), "narrow");
        CHECK(f == CString(_T("[narrow]")));

        // Characters, same rule: %c matches the format, %C is the other one.
        f.Format(_T("%c%c"), _T('o'), _T('k'));
        CHECK(f == CString(_T("ok")));
        f.Format(_T("%C"), 'x');
        CHECK(f == CString(_T("x")));

        // An explicit length modifier overrides the conversion's default.
        f.Format(_T("[%hs]"), "narrow");
        CHECK(f == CString(_T("[narrow]")));
        f.Format(_T("[%ls]"), L"wide");
        CHECK(f == CString(_T("[wide]")));

        // Integers, floats, and the flags/width/precision that ride along.
        f.Format(_T("%d %u %04d %+d"), -7, 7u, 42, 5);
        CHECK(f == CString(_T("-7 7 0042 +5")));
        f.Format(_T("%x %X %o"), 255, 255, 8);
        CHECK(f == CString(_T("ff FF 10")));
        f.Format(_T("%.2f"), 3.14159);
        CHECK(f == CString(_T("3.14")));
        f.Format(_T("%8.3s|"), _T("abcdef"));    // width AND precision on %s
        CHECK(f == CString(_T("     abc|")));
        f.Format(_T("%-6s|"), _T("ab"));         // left-justified
        CHECK(f == CString(_T("ab    |")));
        f.Format(_T("%*d"), 5, 42);              // width from an argument
        CHECK(f == CString(_T("   42")));

        // %I64: MSVC's 64-bit length modifier, which eMule uses for file
        // sizes and transfer counters (~70 times).
        // (long long, not __int64: that spelling is an MSVC keyword and this
        // test compiles on GCC too. The point here is the %I64 specifier.)
        f.Format(_T("%I64d"), -1234567890123LL);
        CHECK(f == CString(_T("-1234567890123")));
        f.Format(_T("%I64u"), 12345678901234ULL);
        CHECK(f == CString(_T("12345678901234")));

        // A literal percent must survive, and must not eat the conversion
        // that follows it.
        f.Format(_T("%d%% of %s"), 50, _T("it"));
        CHECK(f == CString(_T("50% of it")));

        // Long output: the buffer starts at 256 and doubles, so this checks
        // the growth path rather than the first attempt.
        f.Format(_T("%500s|"), _T("x"));
        CHECK(f.GetLength() == 501);
        CHECK(f.Right(2) == CString(_T("x|")));

        // AppendFormat appends rather than replacing.
        f = _T("head ");
        f.AppendFormat(_T("%s %d"), _T("tail"), 9);
        CHECK(f == CString(_T("head tail 9")));

        // Formatting nothing at all is still valid.
        f.Format(_T("plain"));
        CHECK(f == CString(_T("plain")));

        // The narrow instantiation obeys the mirrored rule: there %s is the
        // narrow string and %S the wide one.
        CStringA fa;
        fa.Format("Hello, %s!", "world");
        CHECK(fa == CStringA("Hello, world!"));
        fa.Format("[%S]", L"wide");
        CHECK(fa == CStringA("[wide]"));
        fa.Format("%d%%", 50);
        CHECK(fa == CStringA("50%"));
    }

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

    // CSimpleArray (atlsimpcoll.h): the eMule-used subset.
    CSimpleArray<int> sa;
    sa.Add(10);
    sa.Add(20);
    sa.Add(30);
    CHECK(sa.GetSize() == 3);
    CHECK(sa[1] == 20);
    CHECK(sa.Find(30) == 2);
    CHECK(sa.Find(999) == -1);
    CHECK(sa.Remove(20) != FALSE);
    CHECK(sa.GetSize() == 2);

    // CRBMap (atlcoll.h): ordered map. Guards the FindFirstKeyAfter
    // semantics specifically -- it is STRICTLY-after (upper_bound), so
    // FindFirstKeyAfter(30) with 30 present returns the NEXT key, 40, not
    // 30. Pinned here so the fix can't regress without the real-ATL CI.
    CRBMap<ULONGLONG, DWORD> rb;
    rb.SetAt(10, 100);
    rb.SetAt(30, 300);
    rb.SetAt(40, 400);
    POSITION after30 = rb.FindFirstKeyAfter(30);
    CHECK(after30 != nullptr);
    CHECK(rb.GetKeyAt(after30) == 40);
    POSITION head = rb.GetHeadPosition();
    CHECK(rb.GetKeyAt(head) == 10); // smallest key first

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

        // A schemeless URL must FAIL, matching real MFC. eMule's downloader
        // depends on this: on failure it prepends "http://" and retries.
        // Pinned here so the fix can't regress without the real-MFC CI.
        DWORD svc2 = 0;
        CString server2, object2;
        INTERNET_PORT port2 = 0;
        BOOL ok2 = AfxParseURL(L"example.com/path", svc2, server2, object2, port2);
        CHECK(ok2 == FALSE);
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
    // CAsyncSocket: a real, working implementation -- exercise it over
    // loopback (sync TCP, sync UDP, and the async reactor's OnReceive).
    {
        CHECK(AfxSocketInit() != FALSE);
        CHECK(TestSyncTcp());
        CHECK(TestSyncUdp());
        CHECK(TestAsyncUdp());
        AfxSocketTerm();
    }
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
