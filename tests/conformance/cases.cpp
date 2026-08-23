// cases.cpp — conformance test cases, compiled TWICE into two separate
// executables from this one file:
//
//   smfc_probe      (-DSIMPLE_MFC_USE_NATIVE)   this branch's E-prefixed
//                                               headers (../../include/e*.h)
//   real_mfc_probe  (-DSIMPLE_MFC_USE_REAL_MFC) the real MFC/ATL headers
//                                               shipped with Visual Studio
//
// Both probes run the exact same sequence of calls (this file is shared
// verbatim — only the #include block below differs) and print one
// canonical record per checked value to stdout, as "<case name>\t<value>"
// (see Line()). compare.py runs both and matches those records BY NAME, so
// any behavioural or result difference shows up as a named failing case,
// and a case only one side emits is reported as missing rather than as a
// positional shift.
//
// WRITTEN IN MFC VOCABULARY ON PURPOSE. The cases below say CString,
// CFile, CObList — never ECString, ECFile, ECObList — because the file has
// to compile unchanged against the real headers, where those are the only
// spellings that exist. On the native side mfc_names.h maps each MFC name
// onto this branch's E-prefixed one; that alias layer is the ONLY place
// the two vocabularies meet, which keeps the cases themselves free of
// per-side #ifdefs.
//
// WHAT IS NOT HERE, and why. This suite compares behaviour, so a symbol
// with no behaviour on our side cannot be compared:
//
//   * CComPtr / CComQIPtr / CComBSTR / CComVariant (atlcomcli.h),
//     CRegKey (atlbase.h) and COleDateTime (atlcomtime.h) are
//     declaration-only stubs on this branch — no .cpp defines a single
//     member, so a call to one would not even link. Skipped entirely.
//   * CObject::AssertValid/Dump and the ASSERT/VERIFY/TRACE macros are
//     _DEBUG-only in real MFC and expand to nothing in the Release
//     configuration this suite builds.
//   * CAsyncSocket's OnReceive/OnSend/OnAccept/OnConnect/OnClose
//     callbacks: real MFC delivers them through WSAAsyncSelect and a
//     hidden window, i.e. only to a thread running a message pump, which
//     a console probe has not got. The synchronous surface underneath
//     them is compared in full; see TestCAsyncSocket.
//
// The real-MFC half only ever builds on MSVC with the "MFC and ATL"
// Visual Studio component installed — see ../../.github/workflows/
// conformance.yml.

#if defined(SIMPLE_MFC_USE_NATIVE)
    #include "eafx.h"
    #include "eafxcoll.h"
    #include "eafxtempl.h"
    #include "eafxmt.h"
    #include "eafxwin.h"
    #include "eafxsock.h"
    #include "eatltime.h"
    #include "eatlenc.h"
    #include "eatlconv.h"
    #include "eatlalloc.h"
    #include "eatlsimpcoll.h"
    #include "eatlcoll.h"
    #include "eafxinet.h"
    // Must come last: it aliases the MFC spellings onto everything above.
    #include "mfc_names.h"
#elif defined(SIMPLE_MFC_USE_REAL_MFC)
    #include <afx.h>
    #include <afxcoll.h>
    #include <afxtempl.h>
    #include <afxmt.h>
    #include <afxwin.h>
    #include <afxsock.h>
    #include <atltime.h>
    #include <atlenc.h>
    #include <atlconv.h>
    #include <atlalloc.h>
    #include <atlsimpcoll.h>
    #include <atlcoll.h>
    #include <afxinet.h>
#else
    #error "Define either SIMPLE_MFC_USE_NATIVE or SIMPLE_MFC_USE_REAL_MFC"
#endif

// Only Windows has windows.h, and only the real-MFC probe genuinely needs
// it. The native probe is also built on POSIX (the `posix` job in
// ../../.github/workflows/conformance.yml): there it runs against no MFC at
// all, and its output is compared against the recording the Windows job
// made of the real-MFC probe.
#ifdef _WIN32
    #include <windows.h>
#endif

// windows.h #defines FindNextFile to FindNextFileW under UNICODE builds.
// Real MFC's own headers include windows.h *before* declaring CFileFind,
// so real MFC's method is itself compiled under the substituted name
// (CFileFind::FindNextFileW) — the call site below must match that.
// simple_mfc's afx.h deliberately never includes windows.h, so its
// CFileFind keeps the literal FindNextFile name, and the call site must
// NOT be macro-substituted there. Same source line, opposite requirement
// per branch: dispatch through a macro instead of calling the method
// name directly.
#if defined(SIMPLE_MFC_USE_NATIVE)
    // Macro expansion is rescanned for further substitution, so simply
    // writing FindNextFile in the replacement text below would still get
    // rewritten to FindNextFileW by the still-active windows.h macro.
    // Remove it first: simple_mfc's own CFileFind keeps the literal name
    // since afx.h never includes windows.h.
    #ifdef FindNextFile
        #undef FindNextFile
    #endif
    #define SIMPLE_MFC_FIND_NEXT_FILE(finder) (finder).FindNextFile()
#else
    #define SIMPLE_MFC_FIND_NEXT_FILE(finder) (finder).FindNextFileW()
#endif

// windows.h also #defines the zero-argument GetCurrentTime() to
// GetTickCount() (a legacy 16-bit-Windows compatibility shim in
// winuser.h). Exactly like FindNextFile above: real MFC's own
// ATL::CTime::GetCurrentTime is ALSO declared under the substituted name
// (confirmed by CI — a blanket #undef broke the real-MFC side instead),
// so this needs the same per-branch dispatch, not a plain #undef.
#if defined(SIMPLE_MFC_USE_NATIVE)
    #ifdef GetCurrentTime
        #undef GetCurrentTime
    #endif
    #define SIMPLE_MFC_GET_CURRENT_TIME() CTime::GetCurrentTime()
#else
    #define SIMPLE_MFC_GET_CURRENT_TIME() CTime::GetTickCount()
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#ifdef _WIN32
    #include <crtdbg.h>
#endif
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <sys/stat.h> // _wchmod / chmod: the harness makes a file read-only for CFileFind::IsReadOnly
#include <string>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------
// POSIX stand-ins for the handful of Win32 calls this harness itself uses.
//
// These are the TEST HARNESS's own scaffolding — creating a scratch
// directory, converting to UTF-8 for printing — not part of what is under
// test. Nothing here touches simple_mfc's own behaviour: every one of
// these is a call the harness makes *around* the MFC code, never a call
// the MFC code makes. The Win32 error constants keep their real numeric
// values, because those ARE compared (CFileException::m_lOsError).
// ---------------------------------------------------------------------
#ifndef _WIN32
    #include <filesystem>
    #include <sys/ioctl.h> // FIONREAD (winsock2.h on Windows)

    #define MAX_PATH 260

    // Real winerror.h values — these get printed and compared.
    #define ERROR_FILE_NOT_FOUND 2L
    #define ERROR_DISK_FULL      112L
    #define ERROR_BAD_PATHNAME   161L

    static void wcscpy_s(wchar_t* dst, size_t n, const wchar_t* src)
    {
        if (!dst || n == 0) return;
        size_t i = 0;
        for (; src && src[i] && i + 1 < n; ++i) dst[i] = src[i];
        dst[i] = L'\0';
    }

    static void GetTempPathW(unsigned long n, wchar_t* buf)
    {
        wcscpy_s(buf, n, L"/tmp/");
    }

    static void CreateDirectoryW(const wchar_t* path, void*)
    {
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(path), ec);
    }

    static void RemoveDirectoryW(const wchar_t* path)
    {
        std::error_code ec;
        std::filesystem::remove(std::filesystem::path(path), ec);
    }
#endif

// The native path separator, used by the harness when it builds a scratch
// path by hand. A literal '\\' is not a separator on POSIX — it is an
// ordinary character in a file name, so hard-coding it would silently
// create one weirdly-named file instead of a directory tree.
#ifdef _WIN32
    #define SMFC_SEP L"\\"
#else
    #define SMFC_SEP L"/"
#endif

namespace
{
// Nothing in this harness may ever wait for a human. A probe that stops
// on a modal dialog does not fail -- it HANGS, and compare.py (which
// runs both probes as subprocesses) would hang with it, taking the
// whole CI job to the runner's multi-hour limit. Windows has three
// separate dialogs that can appear without anyone asking for one:
//
//   * the debug CRT's assertion box (_CrtDbgReport's default
//     _CRTDBG_MODE_WNDW destination for _CRT_ASSERT/_CRT_ERROR),
//   * the abort() "This application has requested the Runtime to
//     terminate it in an unusual way" box,
//   * the OS's own crash / critical-error box (Windows Error Reporting).
//
// Real MFC's ASSERT macros feed the first of those, and this suite runs
// real MFC code by construction, so it is not hypothetical here. Redirect
// all three to stderr / immediate exit.
//
// None of these exist off Windows, and none of them have a POSIX analogue
// worth writing: a POSIX process that aborts just dies, which is exactly
// the outcome this function works to obtain on Windows.
void SilenceWindowsDialogs()
{
#ifdef _WIN32
    for (int report : {_CRT_WARN, _CRT_ERROR, _CRT_ASSERT})
    {
        _CrtSetReportMode(report, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(report, _CRTDBG_FILE_STDERR);
    }
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
#endif
}
} // namespace

namespace
{

// ---------------------------------------------------------------------
// Canonical output helpers. Every printed value goes through one of
// these, so both probes emit an identical text format when behavior
// matches. Wide strings are converted to UTF-8 explicitly (never printed
// via wprintf) so console code-page/locale quirks cannot introduce a
// spurious mismatch between the two probes.
// ---------------------------------------------------------------------
int g_index = 0;

std::string Utf8(const wchar_t* w)
{
    if (!w) return {};
#ifdef _WIN32
    int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
    if (n <= 0) return {};
    std::string out(static_cast<size_t>(n - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w, -1, out.data(), n, nullptr, nullptr);
    return out;
#else
    // Encoded by hand rather than through the locale: the whole point of
    // this function is that the two probes' bytes must be comparable, and
    // a locale-dependent conversion is exactly the kind of thing that
    // would make them differ for reasons that have nothing to do with MFC.
    // wchar_t is UTF-32 here; surrogate pairs (which a UTF-16 Windows
    // wchar_t would carry) are recombined so both sides encode the same
    // code point to the same bytes.
    std::string out;
    for (const wchar_t* p = w; *p; ++p)
    {
        unsigned long cp = static_cast<unsigned long>(*p);
        if (cp >= 0xD800 && cp <= 0xDBFF && p[1] >= 0xDC00 && p[1] <= 0xDFFF)
        {
            cp = 0x10000 + ((cp - 0xD800) << 10) + (static_cast<unsigned long>(p[1]) - 0xDC00);
            ++p;
        }
        if (cp < 0x80)
            out += static_cast<char>(cp);
        else if (cp < 0x800)
        {
            out += static_cast<char>(0xC0 | (cp >> 6));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
        else if (cp < 0x10000)
        {
            out += static_cast<char>(0xE0 | (cp >> 12));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
        else
        {
            out += static_cast<char>(0xF0 | (cp >> 18));
            out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }
    return out;
#endif
}

// A handful of values under test (CStdioFile::ReadString results in
// particular) legitimately contain a raw '\r' — that's the exact behavior
// being verified. Escape control characters rather than printing them
// literally: the canonical output line must always end in exactly one
// real '\n', so nothing about the *value* can ever interact with the
// pipe/CRT text-mode translation of the line terminator itself. The
// escaped form still lets a byte-for-byte diff catch any real difference.
std::string Escape(const std::string& s)
{
    std::string out;
    out.reserve(s.size());
    for (char c : s)
    {
        switch (c)
        {
        case '\\': out += "\\\\"; break; // paths contain backslashes; escaping
                                         // them keeps a trailing '\' from
                                         // corrupting the line-based diff
        case '\r': out += "\\r"; break;
        case '\n': out += "\\n"; break;
        case '\t': out += "\\t"; break;
        default: out += c; break;
        }
    }
    return out;
}

// One record per checked value, as exactly two tab-separated fields:
//
//     <case name>\t<escaped value>
//
// The case NAME is the key compare.py matches on -- deliberately not
// the line number. The previous format led with a running counter and was
// diffed positionally, which meant a single genuine divergence that
// changed how many lines a section emits (CFileFind.MatchCount is the
// obvious one: it drives a print loop) renumbered and shifted every
// record after it, so one real difference was reported as hundreds. Keyed
// on the name, an extra or missing case is reported as exactly that, and
// the surrounding cases still compare normally.
void Line(const char* name, const std::string& value)
{
    ++g_index;
    std::printf("%s\t%s\n", name, Escape(value).c_str());
    // Flush immediately: if the process later ends earlier than expected
    // (crash, or anything else), every line printed so far must already
    // be visible to whoever captured stdout, not stuck in a buffer.
    std::fflush(stdout);
}
void Line(const char* name, const wchar_t* value) { Line(name, Utf8(value)); }
void Line(const char* name, const CString& value) { Line(name, Utf8((LPCTSTR)value)); }
void LineBool(const char* name, bool value) { Line(name, std::string(value ? "TRUE" : "FALSE")); }
void LineInt(const char* name, long long value) { Line(name, std::to_string(value)); }

// Renders raw bytes as uppercase hex. Used where the BYTES themselves are
// the contract, not just the values round-tripped through them: CArchive
// writes a format that outlives the process that produced it, so "reads
// back correctly" is a weaker claim than "produces the same stream real
// MFC produces".
std::string Hex(const void* data, size_t n)
{
    static const char kDigits[] = "0123456789ABCDEF";
    const unsigned char* p = static_cast<const unsigned char*>(data);
    std::string out;
    out.reserve(n * 2);
    for (size_t i = 0; i < n; ++i)
    {
        out.push_back(kDigits[p[i] >> 4]);
        out.push_back(kDigits[p[i] & 0x0F]);
    }
    return out;
}

// Normalizes comparator return values to {-1,0,1}: the exact magnitude of
// Compare()/CompareNoCase() is implementation-defined (different CRT
// comparison routines legitimately return different magnitudes for the
// same relative ordering), only the sign is a meaningful, portable result.
int Sign(int v) { return (v > 0) - (v < 0); }

// Best-effort cleanup: CFile::Remove and CFile::Close are both documented
// to be able to throw CFileException on failure. On a shared CI runner,
// this suite found real MFC's process ending early right around a
// CStdioFile close/cleanup sequence — plausibly a transient Windows
// file-lock race (antivirus/indexing on the runner) rather than a real
// conformance difference. None of these administrative calls are
// themselves the subject of an assertion (Close() returns void; the one
// Remove() call whose outcome IS checked, via a follow-up GetStatus, is
// left calling CFile::Remove directly so a genuine failure still shows).
template <class TFile>
void SafeClose(TFile& file)
{
    try
    {
        file.Close();
    }
    catch (CFileException* e)
    {
        e->Delete();
    }
}

void SafeRemoveFile(LPCTSTR path)
{
    try
    {
        CFile::Remove(path);
    }
    catch (CFileException* e)
    {
        e->Delete();
    }
}

CString TempDir()
{
    wchar_t buf[MAX_PATH]{};
    GetTempPathW(MAX_PATH, buf);
    return CString(buf);
}

// Read-only / writable, for CFileFind::IsReadOnly. _wchmod means the same
// thing on both platforms at the level this test cares about: on Windows
// it sets and clears FILE_ATTRIBUTE_READONLY, on POSIX it sets and clears
// the owner write bit — and CFileFind::IsReadOnly reads whichever of the
// two its platform keeps. Harness scaffolding: the call is made *around*
// the code under test, never by it.
void MakeReadOnly(LPCTSTR path)
{
#ifdef _WIN32
    ::_wchmod(path, _S_IREAD);
#else
    std::error_code ec;
    std::filesystem::permissions(std::filesystem::path(path),
                                 std::filesystem::perms::owner_write
                                     | std::filesystem::perms::group_write
                                     | std::filesystem::perms::others_write,
                                 std::filesystem::perm_options::remove, ec);
#endif
}

void MakeWritable(LPCTSTR path)
{
#ifdef _WIN32
    ::_wchmod(path, _S_IREAD | _S_IWRITE);
#else
    std::error_code ec;
    std::filesystem::permissions(std::filesystem::path(path),
                                 std::filesystem::perms::owner_write,
                                 std::filesystem::perm_options::add, ec);
#endif
}

} // namespace

// ---------------------------------------------------------------------
// RTTI (CObject / DECLARE_DYNAMIC / IsKindOf / RUNTIME_CLASS)
// ---------------------------------------------------------------------
static void TestRTTI()
{
    CObject* fileEx = new CFileException();
    LineBool("RTTI.CFileException.IsKindOf.CException", fileEx->IsKindOf(RUNTIME_CLASS(CException)) != FALSE);
    LineBool("RTTI.CFileException.IsKindOf.CObject", fileEx->IsKindOf(RUNTIME_CLASS(CObject)) != FALSE);
    LineBool("RTTI.CFileException.IsKindOf.CMemoryException", fileEx->IsKindOf(RUNTIME_CLASS(CMemoryException)) != FALSE);
    Line("RTTI.CFileException.ClassName", std::string(fileEx->GetRuntimeClass()->m_lpszClassName));
    // CObject::AssertValid() is intentionally NOT exercised: real MFC
    // compiles it (like Dump()) only under #ifdef _DEBUG — it doesn't
    // exist at all as a member in a Release build, which is what this
    // conformance job builds (found by this very suite).
    LineBool("RTTI.CFileException.IsSerializable", fileEx->IsSerializable() != FALSE);
    delete fileEx;

    CObject* memEx = new CMemoryException();
    LineBool("RTTI.CMemoryException.IsKindOf.CSimpleException", memEx->IsKindOf(RUNTIME_CLASS(CSimpleException)) != FALSE);
    LineBool("RTTI.CMemoryException.IsKindOf.CException", memEx->IsKindOf(RUNTIME_CLASS(CException)) != FALSE);
    LineBool("RTTI.CMemoryException.IsKindOf.CFileException", memEx->IsKindOf(RUNTIME_CLASS(CFileException)) != FALSE);
    delete memEx;

    CFile file;
    LineBool("RTTI.CFile.IsKindOf.CObject", file.IsKindOf(RUNTIME_CLASS(CObject)) != FALSE);
    LineBool("RTTI.CFile.IsKindOf.CFileException", file.IsKindOf(RUNTIME_CLASS(CFileException)) != FALSE);
}

// ---------------------------------------------------------------------
// CException / CFileException / CMemoryException
// ---------------------------------------------------------------------
static void TestExceptions()
{
    // GetErrorMessage's exact TEXT is intentionally NOT compared: real MFC
    // builds it from MFC's own string resources (via AfxLoadString) plus
    // the OS's localized FormatMessage output for m_lOsError, while
    // simple_mfc uses fixed English text — legitimately different
    // strings, not a conformance bug.
    //
    // The message's non-emptiness ALSO isn't compared: this suite found
    // that real MFC's resource lookup requires a live CWinApp (it loads
    // strings through AfxGetResourceHandle(), which is only wired up once
    // an application object exists). In this bare console harness — with
    // no CWinApp, by design, since these are the non-GUI classes — real
    // MFC's CFileException::GetErrorMessage still returns TRUE but with
    // an EMPTY message, regardless of the error code; for
    // CMemoryException — whose message comes ONLY from a string
    // resource, with no FormatMessage fallback — the missing resource
    // makes GetErrorMessage() itself return FALSE. Both are properties of
    // running without a GUI app object, not simple_mfc bugs, so for
    // CMemoryException we don't compare GetErrorMessage's outcome at all;
    // for CFileException, only the boolean return value (which does
    // match) is compared.
    CFileException fe(CFileException::fileNotFound, ERROR_FILE_NOT_FOUND, L"missing_file.dat");
    wchar_t buf[256]{};
    BOOL ok = fe.GetErrorMessage(buf, 256);
    LineBool("CFileException.GetErrorMessage.returns_true", ok != FALSE);
    LineInt("CFileException.m_cause", fe.m_cause);
    LineInt("CFileException.m_lOsError", fe.m_lOsError);
    Line("CFileException.m_strFileName", fe.m_strFileName);

    CMemoryException me;
    wchar_t mbuf[256]{};
    me.GetErrorMessage(mbuf, 256); // outcome not compared, see comment above

    // CException::Delete(): the real MFC pointer+Delete() pattern. Safe to
    // call (no UI). CFileException's constructor always passes
    // bAutoDelete=TRUE to CException, so Delete() actually frees *this —
    // read m_cause before calling it.
    CFileException* heapEx = new CFileException(CFileException::badPath, ERROR_BAD_PATHNAME, L"x");
    LineInt("CFileException.Delete.m_cause_before", heapEx->m_cause);
    heapEx->Delete();

    // AfxThrowFileException / AfxThrowMemoryException: throw by pointer,
    // caught the same way real MFC code does (catch (CFileException* e),
    // then e->Delete()). CException::ReportError() is intentionally NOT
    // exercised: on the real-MFC side it opens a genuine Win32 MessageBox,
    // which would hang a non-interactive CI runner waiting for a dismissal
    // that never comes.
    try
    {
        AfxThrowFileException(CFileException::diskFull, ERROR_DISK_FULL, L"y.dat");
        Line("AfxThrowFileException.caught", std::string("NEVER (did not throw)"));
    }
    catch (CFileException* e)
    {
        LineInt("AfxThrowFileException.caught.m_cause", e->m_cause);
        e->Delete();
    }

    try
    {
        // Both branches emit the SAME case name on purpose, and exactly
        // one of them ever runs. compare.py keys on the name and would
        // reject a genuine duplicate, so this stays legal — and it means a
        // side that failed to throw shows up as a value difference
        // ("NEVER (did not throw)" against "TRUE") rather than as a case
        // one probe simply omitted.
        AfxThrowMemoryException();
        Line("AfxThrowMemoryException.caught", std::string("NEVER (did not throw)"));
    }
    catch (CMemoryException* e)
    {
        LineBool("AfxThrowMemoryException.caught", e != nullptr);
    }
}

// ---------------------------------------------------------------------
// CString
// ---------------------------------------------------------------------
static void TestCString()
{
    CString s = L"  Hello, World!  ";
    LineBool("CString.IsEmpty.initial", s.IsEmpty() != FALSE);
    LineInt("CString.GetLength.initial", s.GetLength());

    CString trimmed = s;
    trimmed.Trim();
    Line("CString.Trim.result", trimmed);

    CString upper = trimmed;
    upper.MakeUpper();
    Line("CString.MakeUpper.result", upper);

    CString lower = trimmed;
    lower.MakeLower();
    Line("CString.MakeLower.result", lower);

    LineInt("CString.Find.substr", trimmed.Find(L"World"));
    LineInt("CString.Find.missing", trimmed.Find(L"xyz"));
    LineInt("CString.Find.char", trimmed.Find(L'W'));
    LineInt("CString.ReverseFind", trimmed.ReverseFind(L'o'));

    CString fmt;
    fmt.Format(L"%d-%s-%02d", 2026, L"Jul", 9);
    Line("CString.Format.result", fmt);

    CString app = L"base";
    app.AppendFormat(L"+%d=%s", 42, L"done");
    Line("CString.AppendFormat.result", app);

    Line("CString.Left5", trimmed.Left(5));
    Line("CString.Right6", trimmed.Right(6));
    Line("CString.Mid7_5", trimmed.Mid(7, 5));
    Line("CString.Mid7_NoCount", trimmed.Mid(7));

    CString rep = trimmed;
    int nrep = rep.Replace(L"o", L"0");
    LineInt("CString.Replace.count", nrep);
    Line("CString.Replace.result", rep);

    CString repChar = trimmed;
    int nrepChar = repChar.Replace(L'l', L'L');
    LineInt("CString.ReplaceChar.count", nrepChar);
    Line("CString.ReplaceChar.result", repChar);

    LineInt("CString.Compare.equal", Sign(CString(L"abc").Compare(L"abc")));
    LineInt("CString.Compare.less", Sign(CString(L"abc").Compare(L"abd")));
    LineInt("CString.Compare.greater", Sign(CString(L"abd").Compare(L"abc")));
    LineInt("CString.CompareNoCase.equal", Sign(CString(L"ABC").CompareNoCase(L"abc")));
    LineInt("CString.CompareNoCase.less", Sign(CString(L"ABC").CompareNoCase(L"abd")));

    CString del = trimmed;
    del.Delete(0, 6); // remove "Hello,"
    Line("CString.Delete.result", del);
    LineInt("CString.Delete.returned_length", del.GetLength());

    CString ins = CString(L"Hello World");
    ins.Insert(5, L",");
    Line("CString.Insert.string.result", ins);
    CString insCh = CString(L"ac");
    insCh.Insert(1, L'b');
    Line("CString.Insert.char.result", insCh);

    Line("CString.SpanExcluding", CString(L"12345abc").SpanExcluding(L"abcdefg"));

    CString tok = L"a,b,,c";
    int start = 0;
    Line("CString.Tokenize.1", tok.Tokenize(L",", start));
    LineInt("CString.Tokenize.pos_after_1", start);
    Line("CString.Tokenize.2", tok.Tokenize(L",", start));
    LineInt("CString.Tokenize.pos_after_2", start);
    Line("CString.Tokenize.3_empty", tok.Tokenize(L",", start));

    CString trimChar = L"xxhelloxx";
    trimChar.Trim(L'x');
    Line("CString.TrimChar", trimChar);

    CString trimSet = L"##--hello--##";
    trimSet.Trim(L"#-");
    Line("CString.TrimSet", trimSet);

    CString trimRightDefault = L"hello   ";
    trimRightDefault.TrimRight();
    Line("CString.TrimRight.default", trimRightDefault);

    CString trimRightChar = L"helloxxx";
    trimRightChar.TrimRight(L'x');
    Line("CString.TrimRight.char", trimRightChar);

    CString getset = L"abc";
    LineInt("CString.GetAt1", getset.GetAt(1));
    getset.SetAt(1, L'Z');
    Line("CString.SetAt.result", getset);

    CString cat = CString(L"foo") + CString(L"bar");
    Line("CString.operatorPlus", cat);
    LineBool("CString.operatorEq.true", CString(L"x") == CString(L"x"));
    LineBool("CString.operatorEq.false", CString(L"x") == CString(L"y"));
    LineBool("CString.operatorNe", CString(L"x") != CString(L"y"));

    // GetBuffer/ReleaseBuffer round trip (never reads uninitialized memory).
    CString buf;
    wchar_t* p = buf.GetBuffer(32);
    wcscpy_s(p, 32, L"buffered");
    buf.ReleaseBuffer();
    Line("CString.GetBuffer_ReleaseBuffer.result", buf);
    LineInt("CString.GetBuffer_ReleaseBuffer.length", buf.GetLength());

    CString emptied = L"not empty";
    emptied.Empty();
    LineBool("CString.Empty.IsEmptyAfter", emptied.IsEmpty() != FALSE);

    // Char-repeat constructor
    CString repeated(L'x', 5);
    Line("CString.CharRepeatCtor", repeated);

    // GetBuffer() with no argument
    CString noarg = L"abc";
    wchar_t* pna = noarg.GetBuffer();
    Line("CString.GetBuffer_NoArg", CString(pna));

    // TrimRight with a multi-character set (not a single char)
    CString trimRightSet = L"hello##--";
    trimRightSet.TrimRight(L"#-");
    Line("CString.TrimRight.set", trimRightSet);

    // operator[]
    CString idx = L"index";
    LineInt("CString.operatorIndex2", idx[2]);

    // operator+= (CString and single wchar_t)
    CString plusEqStr = L"foo";
    plusEqStr += CString(L"bar");
    Line("CString.operatorPlusEqString", plusEqStr);
    CString plusEqChar = L"foo";
    plusEqChar += L'!';
    Line("CString.operatorPlusEqChar", plusEqChar);

    // operator<
    LineBool("CString.operatorLess.true", CString(L"a") < CString(L"b"));
    LineBool("CString.operatorLess.false", CString(L"b") < CString(L"a"));

    // --- Higher-variability inputs -------------------------------------
    // Format: more conversions/flags than the single "%d-%s-%02d" above.
    CString fmtHex; fmtHex.Format(L"%08X", 0xDEADu);
    Line("CString.Format.hex", fmtHex);
    CString fmtChar; fmtChar.Format(L"%c|%c", L'Z', L'9');
    Line("CString.Format.char", fmtChar);
    CString fmtFloat; fmtFloat.Format(L"%.3f", 3.14159265);
    Line("CString.Format.float", fmtFloat);
    CString fmtWidth; fmtWidth.Format(L"[%5d][%-5d][%+d]", 42, 42, 42);
    Line("CString.Format.width", fmtWidth);
    CString fmtPercent; fmtPercent.Format(L"100%% done");
    Line("CString.Format.percent", fmtPercent);

    // Empty-string edge cases.
    CString empty;
    empty.Trim();
    LineBool("CString.Empty.TrimStaysEmpty", empty.IsEmpty() != FALSE);
    LineInt("CString.Empty.FindMissing", empty.Find(L"x"));
    LineInt("CString.Empty.Length", empty.GetLength());

    // Left/Right/Mid boundary values (all in valid, non-UB range).
    CString abc = L"abc";
    Line("CString.Left0", abc.Left(0));
    Line("CString.LeftBeyond", abc.Left(100));   // clamps to whole string
    Line("CString.Right0", abc.Right(0));
    Line("CString.RightBeyond", abc.Right(100));
    Line("CString.MidAtEnd", abc.Mid(3));         // iFirst == length -> empty
    Line("CString.MidBeyondCount", abc.Mid(1, 100));

    // Find with a non-zero start index, and ReverseFind of a missing char.
    CString haystack = L"abcabcabc";
    LineInt("CString.Find.fromIndex", haystack.Find(L"abc", 1));
    LineInt("CString.ReverseFind.missing", haystack.ReverseFind(L'z'));
}

// ---------------------------------------------------------------------
// CFile / CStdioFile / CMemFile
// ---------------------------------------------------------------------
static void TestCFile()
{
    CString path = TempDir() + CString(L"simple_mfc_conformance_file.bin");

    CFile f;
    BOOL opened = f.Open(path, CFile::modeCreate | CFile::modeWrite);
    LineBool("CFile.Open.create", opened != FALSE);
    const char data[] = "Hello, MFC conformance suite!";
    f.Write(data, sizeof(data) - 1);
    SafeClose(f);

    CFile f2;
    BOOL opened2 = f2.Open(path, CFile::modeRead);
    LineBool("CFile.Open.read", opened2 != FALSE);
    LineInt("CFile.GetLength", static_cast<long long>(f2.GetLength()));
    char rbuf[128]{};
    UINT n = f2.Read(rbuf, sizeof(rbuf) - 1);
    LineInt("CFile.Read.count", n);
    Line("CFile.Read.content", std::string(rbuf, n));

    f2.Seek(7, CFile::begin);
    LineInt("CFile.Seek.position", static_cast<long long>(f2.GetPosition()));
    char seekBuf[6]{};
    f2.Read(seekBuf, 5);
    Line("CFile.ReadAfterSeek", std::string(seekBuf, 5));

    // Seek relative to the current position (origin=current), which the
    // rest of the suite never exercises (only begin/end).
    f2.SeekToBegin();
    f2.Seek(3, CFile::current);
    LineInt("CFile.Seek.current.after3", static_cast<long long>(f2.GetPosition()));
    f2.Seek(2, CFile::current);
    LineInt("CFile.Seek.current.after3plus2", static_cast<long long>(f2.GetPosition()));
    f2.Seek(-1, CFile::current);
    LineInt("CFile.Seek.current.backward", static_cast<long long>(f2.GetPosition()));

    f2.SeekToBegin();
    LineInt("CFile.SeekToBegin.position", static_cast<long long>(f2.GetPosition()));
    f2.SeekToEnd();
    LineInt("CFile.SeekToEnd.position", static_cast<long long>(f2.GetPosition()));
    Line("CFile.GetFileName", f2.GetFileName());
    Line("CFile.GetFilePath", f2.GetFilePath());

    CFileStatus instStatus{};
    LineBool("CFile.GetStatus.instance.ok", f2.GetStatus(instStatus) != FALSE);
    LineInt("CFile.GetStatus.instance.size", static_cast<long long>(instStatus.m_size));
    SafeClose(f2);

    CFileStatus status{};
    BOOL statusOk = CFile::GetStatus(path, status);
    LineBool("CFile.GetStatus.static.ok", statusOk != FALSE);
    LineInt("CFile.GetStatus.static.size", static_cast<long long>(status.m_size));

    CString renamedPath = TempDir() + CString(L"simple_mfc_conformance_file_renamed.bin");
    CFile::Rename(path, renamedPath);
    CFileStatus statusAfterRename{};
    LineBool("CFile.Rename.thenGetStatus.ok", CFile::GetStatus(renamedPath, statusAfterRename) != FALSE);

    CFile::Remove(renamedPath);
    CFileStatus statusAfterRemove{};
    LineBool("CFile.Remove.thenGetStatus.fails", CFile::GetStatus(renamedPath, statusAfterRemove) == FALSE);

    // Combined constructor (path + flags) + SetLength + Flush + Abort, on
    // a fresh dedicated path so it doesn't disturb the read-back checks above.
    CString path2 = TempDir() + CString(L"simple_mfc_conformance_file2.bin");
    {
        CFile ctorFile(path2, CFile::modeCreate | CFile::modeWrite);
        const char data2[] = "ctor-opened-file";
        ctorFile.Write(data2, sizeof(data2) - 1);
        ctorFile.Flush();
        LineInt("CFile.CtorOpen.GetLength", static_cast<long long>(ctorFile.GetLength()));
        ctorFile.SetLength(4);
        LineInt("CFile.SetLength4.GetLength", static_cast<long long>(ctorFile.GetLength()));
        ctorFile.Abort();
    }
    SafeRemoveFile(path2);
}

static void TestCStdioFile()
{
    CString path = TempDir() + CString(L"simple_mfc_conformance_stdio.txt");

    CStdioFile wf;
    wf.Open(path, CFile::modeCreate | CFile::modeWrite);
    wf.WriteString(L"first line\r\n");
    wf.WriteString(L"second line\r\n");
    SafeClose(wf);

    CStdioFile rf;
    rf.Open(path, CFile::modeRead);
    CString line1, line2, line3;
    BOOL got1 = rf.ReadString(line1);
    BOOL got2 = rf.ReadString(line2);
    BOOL got3 = rf.ReadString(line3); // past EOF: expected to fail
    SafeClose(rf);

    LineBool("CStdioFile.ReadString.line1.ok", got1 != FALSE);
    Line("CStdioFile.ReadString.line1", line1);
    LineBool("CStdioFile.ReadString.line2.ok", got2 != FALSE);
    Line("CStdioFile.ReadString.line2", line2);
    LineBool("CStdioFile.ReadString.line3PastEof.fails", got3 == FALSE);

    SafeRemoveFile(path);

    // Combined constructor (path + flags) — write mode only — plus the
    // LPTSTR/UINT ReadString overload (the buffer-based one; CString& is
    // already covered above). The read side uses the two-step default-ctor
    // + Open() pattern, matching every other read in this suite.
    CString path2 = TempDir() + CString(L"simple_mfc_conformance_stdio2.txt");
    {
        CStdioFile ctorWrite(path2, CFile::modeCreate | CFile::modeWrite);
        ctorWrite.WriteString(L"buffer overload line\r\n");
        SafeClose(ctorWrite);

        CStdioFile bufRead;
        bufRead.Open(path2, CFile::modeRead);
        wchar_t lineBuf[128]{};
        LPTSTR got = bufRead.ReadString(lineBuf, 64);
        LineBool("CStdioFile.ReadString.buffer.nonNull", got != nullptr);
        Line("CStdioFile.ReadString.buffer.content", lineBuf);
        SafeClose(bufRead);
    }
    SafeRemoveFile(path2);
}

static void TestCMemFile()
{
    CMemFile mf;
    const char payload[] = "in-memory payload";
    mf.Write(payload, sizeof(payload) - 1);
    LineInt("CMemFile.GetLength", static_cast<long long>(mf.GetLength()));

    mf.Seek(0, CFile::begin);
    char rbuf[64]{};
    UINT n = mf.Read(rbuf, sizeof(rbuf) - 1);
    LineInt("CMemFile.Read.count", n);
    Line("CMemFile.Read.content", std::string(rbuf, n));

    mf.Seek(3, CFile::begin);
    LineInt("CMemFile.Seek.position", static_cast<long long>(mf.GetPosition()));

    // Seek relative to current and to end (never exercised elsewhere).
    mf.Seek(0, CFile::begin);
    mf.Seek(5, CFile::current);
    LineInt("CMemFile.Seek.current", static_cast<long long>(mf.GetPosition()));
    mf.Seek(-3, CFile::end);
    LineInt("CMemFile.Seek.end.minus3", static_cast<long long>(mf.GetPosition()));

    // SetLength grows/shrinks the in-memory buffer.
    mf.SetLength(4);
    LineInt("CMemFile.SetLength4.GetLength", static_cast<long long>(mf.GetLength()));
    mf.SetLength(10);
    LineInt("CMemFile.SetLength10.GetLength", static_cast<long long>(mf.GetLength()));
}

// ---------------------------------------------------------------------
// CFileFind
// ---------------------------------------------------------------------
static void TestCFileFind()
{
    CString dir = TempDir() + CString(L"simple_mfc_conformance_find" SMFC_SEP);
    CreateDirectoryW(dir, nullptr);

    const wchar_t* names[] = {L"alpha.txt", L"beta.txt", L"gamma.dat"};
    for (const wchar_t* name : names)
    {
        CFile f;
        f.Open(dir + CString(name), CFile::modeCreate | CFile::modeWrite);
        const char payload[] = "x"; // non-empty, so CFileFind::GetLength() is meaningfully non-zero
        f.Write(payload, sizeof(payload) - 1);
        SafeClose(f);
    }

    CString matched[8];
    int matchCount = 0;
    CFileFind finder;
    BOOL working = finder.FindFile(dir + CString(L"*.txt"));
    while (working)
    {
        working = SIMPLE_MFC_FIND_NEXT_FILE(finder);
        if (finder.IsDots()) continue;
        if (matchCount < 8) matched[matchCount++] = finder.GetFileName();
    }
    // Sort the results before printing: filesystem enumeration order is
    // not part of the documented contract, only the *set* of matches is.
    for (int i = 0; i < matchCount; ++i)
        for (int j = i + 1; j < matchCount; ++j)
            if (matched[j].Compare(matched[i]) < 0)
            {
                CString t = matched[i];
                matched[i] = matched[j];
                matched[j] = t;
            }

    LineInt("CFileFind.MatchCount.txt", matchCount);
    for (int i = 0; i < matchCount; ++i)
    {
        std::string label = "CFileFind.Match." + std::to_string(i);
        Line(label.c_str(), matched[i]);
    }

    // A find on a single, known file (not a wildcard) to exercise
    // GetFilePath/GetLength/IsDirectory/GetRoot/Close deterministically.
    {
        CFileFind single;
        BOOL foundOne = single.FindFile(dir + CString(L"alpha.txt"));
        BOOL hasMore = SIMPLE_MFC_FIND_NEXT_FILE(single);
        (void)hasMore;
        LineBool("CFileFind.Single.foundOne", foundOne != FALSE);
        Line("CFileFind.Single.GetFilePath", single.GetFilePath());
        LineInt("CFileFind.Single.GetLength", static_cast<long long>(single.GetLength()));
        LineBool("CFileFind.Single.IsDirectory", single.IsDirectory() != FALSE);
        Line("CFileFind.Single.GetRoot", single.GetRoot());
        SafeClose(single);
    }

    for (const wchar_t* name : names)
        SafeRemoveFile(dir + CString(name));
    RemoveDirectoryW(dir);
}

// ---------------------------------------------------------------------
// CObList / CPtrList / CStringList
// ---------------------------------------------------------------------
namespace
{
class IntBox : public CObject
{
public:
    int v;
    explicit IntBox(int x) : v(x) {}
};
} // namespace

static void TestCObList()
{
    CObList list;
    IntBox a(1), b(2), c(3);
    list.AddTail(&a);
    list.AddTail(&b);
    list.AddHead(&c); // order: c(3), a(1), b(2)

    LineInt("CObList.GetCount", list.GetCount());
    LineBool("CObList.IsEmpty", list.IsEmpty() != FALSE);

    std::string order;
    POSITION pos = list.GetHeadPosition();
    while (pos)
    {
        CObject* o = list.GetNext(pos);
        order += std::to_string(static_cast<IntBox*>(o)->v);
        if (pos) order += ",";
    }
    Line("CObList.IterationOrder", order);

    LineInt("CObList.GetHead.value", static_cast<IntBox*>(list.GetHead())->v);
    LineInt("CObList.GetTail.value", static_cast<IntBox*>(list.GetTail())->v);

    LineBool("CObList.Find.found", list.Find(&b) != nullptr);
    LineBool("CObList.Find.notFound", list.Find(reinterpret_cast<CObject*>(&order)) != nullptr);

    POSITION idxPos = list.FindIndex(1);
    LineInt("CObList.FindIndex1.value", static_cast<IntBox*>(list.GetAt(idxPos))->v);

    CObject* removedHead = list.RemoveHead();
    LineInt("CObList.RemoveHead.value", static_cast<IntBox*>(removedHead)->v);
    LineInt("CObList.CountAfterRemoveHead", list.GetCount());
    // list is now [a(1), b(2)]

    POSITION tailPos = list.GetTailPosition();
    LineInt("CObList.GetTailPosition.value", static_cast<IntBox*>(list.GetAt(tailPos))->v);
    CObject* prevVal = list.GetPrev(tailPos); // returns the tail's own value, then moves position backward
    LineInt("CObList.GetPrev.value", static_cast<IntBox*>(prevVal)->v);

    IntBox d(999);
    POSITION headPos2 = list.GetHeadPosition();
    list.SetAt(headPos2, &d); // [d(999), b(2)]
    LineInt("CObList.SetAt.value", static_cast<IntBox*>(list.GetHead())->v);

    IntBox e(111), g(222);
    POSITION afterHead = list.GetHeadPosition();
    list.InsertAfter(afterHead, &e); // [d(999), e(111), b(2)]
    POSITION beforeTail = list.FindIndex(2);
    list.InsertBefore(beforeTail, &g); // [d(999), e(111), g(222), b(2)]
    LineInt("CObList.CountAfterInserts", list.GetCount());
    std::string order2;
    POSITION p2 = list.GetHeadPosition();
    while (p2)
    {
        CObject* o = list.GetNext(p2);
        order2 += std::to_string(static_cast<IntBox*>(o)->v);
        if (p2) order2 += ",";
    }
    Line("CObList.IterationOrderAfterInserts", order2);

    list.RemoveAt(list.FindIndex(0)); // removes d(999): [e(111), g(222), b(2)]
    LineInt("CObList.CountAfterRemoveAt", list.GetCount());
    LineInt("CObList.RemoveTail.value", static_cast<IntBox*>(list.RemoveTail())->v);
    LineInt("CObList.CountAfterRemoveTail", list.GetCount());

    list.RemoveAll();
    LineBool("CObList.IsEmptyAfterRemoveAll", list.IsEmpty() != FALSE);
}

static void TestCPtrList()
{
    CPtrList list;
    void* p1 = reinterpret_cast<void*>(static_cast<intptr_t>(11));
    void* p2 = reinterpret_cast<void*>(static_cast<intptr_t>(22));
    void* p3 = reinterpret_cast<void*>(static_cast<intptr_t>(33));
    list.AddTail(p1);
    list.AddTail(p2);
    list.AddHead(p3); // order: 33, 11, 22

    LineInt("CPtrList.GetCount", list.GetCount());
    LineBool("CPtrList.IsEmpty", list.IsEmpty() != FALSE);
    LineInt("CPtrList.GetHead", reinterpret_cast<intptr_t>(list.GetHead()));
    LineInt("CPtrList.GetTail", reinterpret_cast<intptr_t>(list.GetTail()));

    std::string order;
    POSITION pos = list.GetHeadPosition();
    while (pos)
    {
        void* v = list.GetNext(pos);
        order += std::to_string(reinterpret_cast<intptr_t>(v));
        if (pos) order += ",";
    }
    Line("CPtrList.IterationOrder", order);

    void* p4 = reinterpret_cast<void*>(static_cast<intptr_t>(44));
    LineBool("CPtrList.Find.found", list.Find(p2) != nullptr);
    LineBool("CPtrList.Find.notFound", list.Find(p4) != nullptr);
    // CPtrList has no GetAt (matching real MFC: only CObList does), so read
    // the found index via GetNext, which returns the current element before
    // advancing the position.
    POSITION idxPos = list.FindIndex(1);
    LineInt("CPtrList.FindIndex1", reinterpret_cast<intptr_t>(list.GetNext(idxPos)));

    POSITION tailPos = list.GetTailPosition();
    LineInt("CPtrList.GetTailPosition", reinterpret_cast<intptr_t>(list.GetPrev(tailPos)));

    list.InsertAfter(list.GetHeadPosition(), p4); // [33, 44, 11, 22]
    LineInt("CPtrList.CountAfterInsertAfter", list.GetCount());
    list.InsertBefore(list.FindIndex(3), p1); // insert 11 again before index 3 (22): [33, 44, 11, 11, 22]
    LineInt("CPtrList.CountAfterInsertBefore", list.GetCount());

    LineInt("CPtrList.RemoveHead.value", reinterpret_cast<intptr_t>(list.RemoveHead()));
    LineInt("CPtrList.CountAfterRemoveHead", list.GetCount());
    LineInt("CPtrList.RemoveTail.value", reinterpret_cast<intptr_t>(list.RemoveTail()));
    LineInt("CPtrList.CountAfterRemoveTail", list.GetCount());

    list.RemoveAll();
    LineBool("CPtrList.IsEmptyAfterRemoveAll", list.IsEmpty() != FALSE);
}

static void TestCStringList()
{
    CStringList list;
    list.AddTail(L"one");
    list.AddTail(L"two");
    list.AddHead(L"zero"); // order: zero, one, two

    LineInt("CStringList.GetCount", list.GetCount());
    LineBool("CStringList.IsEmpty", list.IsEmpty() != FALSE);
    Line("CStringList.GetHead", list.GetHead());
    Line("CStringList.GetTail", list.GetTail());

    std::string order;
    POSITION pos = list.GetHeadPosition();
    while (pos)
    {
        CString v = list.GetNext(pos);
        order += Utf8((LPCTSTR)v);
        if (pos) order += ",";
    }
    Line("CStringList.IterationOrder", order);

    LineBool("CStringList.Find.found", list.Find(L"one") != nullptr);
    LineBool("CStringList.Find.notFound", list.Find(L"missing") != nullptr);

    Line("CStringList.RemoveHead.value", list.RemoveHead());
    LineInt("CStringList.CountAfterRemoveHead", list.GetCount());
    Line("CStringList.RemoveTail.value", list.RemoveTail());
    LineInt("CStringList.CountAfterRemoveTail", list.GetCount());

    list.RemoveAll();
    LineBool("CStringList.IsEmptyAfterRemoveAll", list.IsEmpty() != FALSE);
}

// ---------------------------------------------------------------------
// CPtrArray / CStringArray / CByteArray / CUIntArray
// ---------------------------------------------------------------------
static void TestCPtrArray()
{
    CPtrArray arr;
    arr.Add(reinterpret_cast<void*>(static_cast<intptr_t>(100)));
    arr.Add(reinterpret_cast<void*>(static_cast<intptr_t>(200)));
    LineInt("CPtrArray.GetCount", arr.GetCount());
    LineInt("CPtrArray.GetAt0", reinterpret_cast<intptr_t>(arr.GetAt(0)));
    LineInt("CPtrArray.GetUpperBound", static_cast<long long>(arr.GetUpperBound()));
    arr.SetAtGrow(5, reinterpret_cast<void*>(static_cast<intptr_t>(500)));
    LineInt("CPtrArray.CountAfterSetAtGrow5", arr.GetCount());
    LineInt("CPtrArray.GetAt5", reinterpret_cast<intptr_t>(arr.GetAt(5)));

    arr.SetAt(0, reinterpret_cast<void*>(static_cast<intptr_t>(999)));
    LineInt("CPtrArray.GetAt0AfterSetAt", reinterpret_cast<intptr_t>(arr.GetAt(0)));

    arr.InsertAt(0, reinterpret_cast<void*>(static_cast<intptr_t>(1)));
    LineInt("CPtrArray.CountAfterInsertAt0", arr.GetCount());
    LineInt("CPtrArray.GetAt0AfterInsert", reinterpret_cast<intptr_t>(arr.GetAt(0)));

    arr.RemoveAt(0);
    LineInt("CPtrArray.CountAfterRemoveAt0", arr.GetCount());

    CPtrArray src;
    src.Add(reinterpret_cast<void*>(static_cast<intptr_t>(777)));
    INT_PTR appendedResult = arr.Append(src);
    LineInt("CPtrArray.Append.result", static_cast<long long>(appendedResult));
    LineInt("CPtrArray.CountAfterAppend", arr.GetCount());

    CPtrArray copyDst;
    copyDst.Copy(arr);
    LineInt("CPtrArray.Copy.count", copyDst.GetCount());

    arr.SetSize(2);
    LineInt("CPtrArray.CountAfterSetSize2", arr.GetCount());
    LineBool("CPtrArray.IsEmpty", arr.IsEmpty() != FALSE);

    arr.RemoveAll();
    LineBool("CPtrArray.IsEmptyAfterRemoveAll", arr.IsEmpty() != FALSE);
}

static void TestCStringArray()
{
    CStringArray arr;
    arr.Add(L"aa");
    arr.Add(L"bb");
    arr.Add(L"cc");
    LineInt("CStringArray.GetCount", arr.GetCount());
    Line("CStringArray.GetAt1", arr.GetAt(1));
    arr.SetAt(1, L"BB");
    Line("CStringArray.SetAt1", arr.GetAt(1));
    arr.RemoveAt(0);
    LineInt("CStringArray.CountAfterRemoveAt0", arr.GetCount());
    Line("CStringArray.GetAt0AfterRemove", arr.GetAt(0));
    LineInt("CStringArray.GetSize", static_cast<long long>(arr.GetSize()));
    LineBool("CStringArray.IsEmpty", arr.IsEmpty() != FALSE);

    arr.InsertAt(0, L"zz");
    LineInt("CStringArray.CountAfterInsertAt0", arr.GetCount());
    Line("CStringArray.GetAt0AfterInsert", arr.GetAt(0));

    arr.SetSize(1);
    LineInt("CStringArray.CountAfterSetSize1", arr.GetCount());

    arr.RemoveAll();
    LineBool("CStringArray.IsEmptyAfterRemoveAll", arr.IsEmpty() != FALSE);
}

static void TestCByteArray()
{
    CByteArray arr;
    arr.Add(10);
    arr.Add(20);
    arr.Add(30);
    LineInt("CByteArray.GetSize", static_cast<long long>(arr.GetSize()));
    LineInt("CByteArray.GetAt1", arr.GetAt(1));
    arr.SetSize(5);
    LineInt("CByteArray.SizeAfterSetSize5", static_cast<long long>(arr.GetSize()));
}

static void TestCUIntArray()
{
    CUIntArray arr;
    arr.Add(111);
    arr.Add(222);
    LineInt("CUIntArray.GetSize", static_cast<long long>(arr.GetSize()));
    LineInt("CUIntArray.GetAt0", arr.GetAt(0));
    LineInt("CUIntArray.GetAt1", arr.GetAt(1));
}

// ---------------------------------------------------------------------
// CArray<> / CList<> / CMap<> (afxtempl.h)
// ---------------------------------------------------------------------
static void TestCArrayTemplate()
{
    CArray<int> arr;
    arr.Add(1);
    arr.Add(2);
    arr.Add(3);
    LineInt("CArray_int.GetCount", arr.GetCount());
    LineInt("CArray_int.operatorIndex1", arr[1]);
    arr[1] = 42;
    LineInt("CArray_int.AfterAssignIndex1", arr[1]);
    arr.InsertAt(0, 100);
    LineInt("CArray_int.CountAfterInsertAt0", arr.GetCount());
    LineInt("CArray_int.GetAt0AfterInsert", arr.GetAt(0));
    arr.RemoveAt(0);
    LineInt("CArray_int.CountAfterRemoveAt0", arr.GetCount());

    LineInt("CArray_int.GetUpperBound", static_cast<long long>(arr.GetUpperBound()));
    const int* data = arr.GetData();
    LineInt("CArray_int.GetData.first", data[0]);
    arr.FreeExtra(); // no printable effect beyond exercising the call
    LineInt("CArray_int.CountAfterFreeExtra", arr.GetCount());

    arr.SetAt(0, 999);
    LineInt("CArray_int.GetAt0AfterSetAt", arr.GetAt(0));
    arr.SetAtGrow(5, 555);
    LineInt("CArray_int.CountAfterSetAtGrow5", arr.GetCount());
    LineInt("CArray_int.GetAt5", arr.GetAt(5));

    CArray<int> src;
    src.Add(777);
    INT_PTR appendedResult = arr.Append(src);
    LineInt("CArray_int.Append.result", static_cast<long long>(appendedResult));
    LineInt("CArray_int.CountAfterAppend", arr.GetCount());

    CArray<int> copyDst;
    copyDst.Copy(arr);
    LineInt("CArray_int.Copy.count", copyDst.GetCount());

    arr.SetSize(2);
    LineInt("CArray_int.CountAfterSetSize2", arr.GetCount());
    LineBool("CArray_int.IsEmpty", arr.IsEmpty() != FALSE);

    arr.RemoveAll();
    LineBool("CArray_int.IsEmptyAfterRemoveAll", arr.IsEmpty() != FALSE);
}

static void TestCListTemplate()
{
    CList<CString, const CString&> list;
    list.AddTail(L"x");
    list.AddTail(L"y");
    list.AddHead(L"w"); // order: w, x, y
    LineInt("CList_CString.GetCount", list.GetCount());
    LineBool("CList_CString.IsEmpty", list.IsEmpty() != FALSE);
    Line("CList_CString.GetHead", list.GetHead());
    Line("CList_CString.GetTail", list.GetTail());

    std::string order;
    POSITION pos = list.GetHeadPosition();
    while (pos)
    {
        CString v = list.GetNext(pos);
        order += Utf8((LPCTSTR)v);
        if (pos) order += ",";
    }
    Line("CList_CString.IterationOrder", order);

    LineBool("CList_CString.Find.found", list.Find(L"x") != nullptr);
    LineBool("CList_CString.Find.notFound", list.Find(L"missing") != nullptr);
    POSITION idxPos = list.FindIndex(1);
    Line("CList_CString.FindIndex1.value", list.GetAt(idxPos));

    POSITION tailPos = list.GetTailPosition();
    Line("CList_CString.GetTailPosition.value", list.GetAt(tailPos));
    Line("CList_CString.GetPrev.value", list.GetPrev(tailPos));

    POSITION headPos = list.GetHeadPosition();
    list.SetAt(headPos, L"W2");
    Line("CList_CString.SetAt.value", list.GetHead());

    list.InsertAfter(list.GetHeadPosition(), L"inserted");
    LineInt("CList_CString.CountAfterInsertAfter", list.GetCount());
    list.InsertBefore(list.FindIndex(2), L"beforeThird");
    LineInt("CList_CString.CountAfterInsertBefore", list.GetCount());

    std::string order2;
    POSITION p2 = list.GetHeadPosition();
    while (p2)
    {
        CString v = list.GetNext(p2);
        order2 += Utf8((LPCTSTR)v);
        if (p2) order2 += ",";
    }
    Line("CList_CString.IterationOrderAfterInserts", order2);

    list.RemoveAt(list.FindIndex(0));
    LineInt("CList_CString.CountAfterRemoveAt", list.GetCount());

    Line("CList_CString.RemoveHead.value", list.RemoveHead());
    LineInt("CList_CString.CountAfterRemoveHead", list.GetCount());
    Line("CList_CString.RemoveTail.value", list.RemoveTail());
    LineInt("CList_CString.CountAfterRemoveTail", list.GetCount());

    list.RemoveAll();
    LineBool("CList_CString.IsEmptyAfterRemoveAll", list.IsEmpty() != FALSE);
}

static void TestCMapTemplate()
{
    // ARG_KEY must be LPCTSTR, not "const CString&": real MFC only
    // provides a HashKey(LPCTSTR) overload, and with ARG_KEY=const
    // CString& the generic fallback template (which casts the key to
    // `long`) is an exact match and gets picked instead, which doesn't
    // compile for a class type. CMap<CString, LPCTSTR, ...> is the
    // standard, documented MFC idiom for CString-keyed maps.
    CMap<CString, LPCTSTR, int, int> map;
    map.SetAt(L"one", 1);
    map.SetAt(L"two", 2);
    map.SetAt(L"three", 3);

    LineInt("CMap.GetCount", map.GetCount());

    int v = 0;
    LineBool("CMap.Lookup.found", map.Lookup(L"two", v) != FALSE);
    LineInt("CMap.Lookup.value", v);
    LineBool("CMap.Lookup.notFound", map.Lookup(L"missing", v) != FALSE);

    LineBool("CMap.RemoveKey.existing", map.RemoveKey(L"one") != FALSE);
    LineBool("CMap.RemoveKey.missing", map.RemoveKey(L"one") != FALSE);
    LineInt("CMap.CountAfterRemoveKey", map.GetCount());

    // Sum via GetStartPosition/GetNextAssoc (order-independent check).
    int sum = 0;
    int count = 0;
    POSITION pos = map.GetStartPosition();
    while (pos)
    {
        CString k;
        int val = 0;
        map.GetNextAssoc(pos, k, val);
        sum += val;
        ++count;
    }
    LineInt("CMap.IterationCount", count);
    LineInt("CMap.IterationSum", sum);

    LineBool("CMap.PLookup.found", map.PLookup(L"three") != nullptr);
    if (const auto* pair = map.PLookup(L"three"))
        LineInt("CMap.PLookup.value", pair->value);

    LineInt("CMap.GetSize", static_cast<long long>(map.GetSize()));
    LineBool("CMap.IsEmpty", map.IsEmpty() != FALSE);

    // InitHashTable is conventionally called right after construction,
    // before any entries are added (some implementations may not
    // guarantee preserving existing entries otherwise) — exercised here
    // on a fresh map, not the already-populated one above.
    CMap<CString, LPCTSTR, int, int> freshMap;
    freshMap.InitHashTable(64);
    LineBool("CMap.GetHashTableSize.nonZero", freshMap.GetHashTableSize() > 0);
    freshMap.SetAt(L"k", 1);
    LineInt("CMap.CountAfterInitHashTableThenSetAt", freshMap.GetCount());

    // PGetFirstAssoc/PGetNextAssoc: like GetStartPosition/GetNextAssoc
    // above, hash-table iteration order is unspecified and WILL differ
    // between std::unordered_map and real MFC's own hash table, so only
    // the aggregate (count/sum) is compared, never per-position order.
    int pCount = 0;
    int pSum = 0;
    for (const auto* p = map.PGetFirstAssoc(); p; p = map.PGetNextAssoc(p))
    {
        ++pCount;
        pSum += p->value;
    }
    LineInt("CMap.PIteration.count", pCount);
    LineInt("CMap.PIteration.sum", pSum);

    // PLookup: eMule uses it as a plain "is this key present" test
    // (SharedFileList.cpp's m_UnsharedFiles_map / m_mapPseudoDirNames), so
    // both the hit's contents and the miss's null are contract.
    const auto* hit = map.PLookup(L"two");
    LineBool("CMap.PLookup.hit.non_null", hit != nullptr);
    Line("CMap.PLookup.hit.key", hit ? hit->key : CString());
    LineInt("CMap.PLookup.hit.value", hit ? hit->value : -1);
    LineBool("CMap.PLookup.miss.is_null", map.PLookup(L"nosuchkey") == nullptr);

    map.RemoveAll();
    LineBool("CMap.IsEmptyAfterRemoveAll", map.IsEmpty() != FALSE);
    LineInt("CMap.CountAfterRemoveAll", map.GetCount());

    // Constructor with an explicit nBlockSize argument (vs. the default
    // used by `map` above).
    CMap<CString, LPCTSTR, int, int> map2(20);
    map2.SetAt(L"only", 1);
    LineInt("CMap.ExplicitBlockSizeCtor.GetCount", map2.GetCount());

    // SetAt on an already-present key must overwrite (not insert a
    // duplicate): count stays 1, value updates.
    CMap<CString, LPCTSTR, int, int> ov;
    ov.SetAt(L"k", 1);
    ov.SetAt(L"k", 99);
    LineInt("CMap.SetAt.overwrite.count", ov.GetCount());
    int ovv = 0;
    ov.Lookup(L"k", ovv);
    LineInt("CMap.SetAt.overwrite.value", ovv);
}

// ---------------------------------------------------------------------
// CTime / CTimeSpan
// ---------------------------------------------------------------------
static void TestTime()
{
    CTime t1(2026, 7, 19, 14, 30, 45);
    LineInt("CTime.GetYear", t1.GetYear());
    LineInt("CTime.GetMonth", t1.GetMonth());
    LineInt("CTime.GetDay", t1.GetDay());
    LineInt("CTime.GetHour", t1.GetHour());
    LineInt("CTime.GetMinute", t1.GetMinute());
    LineInt("CTime.GetSecond", t1.GetSecond());
    LineInt("CTime.GetDayOfWeek", t1.GetDayOfWeek());

    CTime t2(2026, 7, 20, 14, 30, 45);
    LineBool("CTime.operatorLess.true", t1 < t2);
    LineBool("CTime.operatorLess.false", t2 < t1);
    LineBool("CTime.operatorEq.true", t1 == t1);
    LineBool("CTime.operatorEq.false", t1 == t2);

    CTimeSpan diff = t2 - t1;
    LineInt("CTimeSpan.FromDiff.GetDays", diff.GetDays());
    LineInt("CTimeSpan.FromDiff.GetTotalSeconds", static_cast<long long>(diff.GetTotalSeconds()));

    CTime t3 = t1 + diff;
    LineBool("CTime.PlusSpan.equalsT2", t3 == t2);

    CTimeSpan span(1, 2, 3, 4);
    LineInt("CTimeSpan.ctor.GetDays", span.GetDays());
    LineInt("CTimeSpan.ctor.GetHours", span.GetHours());
    LineInt("CTimeSpan.ctor.GetMinutes", span.GetMinutes());
    LineInt("CTimeSpan.ctor.GetSeconds", span.GetSeconds());
    LineInt("CTimeSpan.ctor.GetTotalSeconds", static_cast<long long>(span.GetTotalSeconds()));
    LineInt("CTimeSpan.ctor.GetTotalHours", static_cast<long long>(span.GetTotalHours()));
    LineInt("CTimeSpan.ctor.GetTotalMinutes", static_cast<long long>(span.GetTotalMinutes()));

    Line("CTime.Format", t1.Format(L"%Y-%m-%d %H:%M:%S"));
    LineInt("CTime.GetTime", static_cast<long long>(t1.GetTime()));

    // Default and explicit(__time64_t) constructors.
    CTime defaultTime;
    LineInt("CTime.defaultCtor.GetTime", static_cast<long long>(defaultTime.GetTime()));
    CTime fromEpoch(static_cast<__time64_t>(t1.GetTime()));
    LineBool("CTime.explicitEpochCtor.equalsT1", fromEpoch == t1);

    // GetCurrentTime(): the exact instant is inherently non-deterministic
    // (the two probes run moments apart, not simultaneously), so only a
    // structural property is compared, never the raw value.
    CTime now = SIMPLE_MFC_GET_CURRENT_TIME();
    LineBool("CTime.GetCurrentTime.plausibleYear", now.GetYear() >= 2020);

    // Default and explicit(long long) CTimeSpan constructors, plus
    // operator+/operator- between two spans.
    CTimeSpan defaultSpan;
    LineInt("CTimeSpan.defaultCtor.GetTotalSeconds", static_cast<long long>(defaultSpan.GetTotalSeconds()));
    CTimeSpan fromSeconds(3661); // 1h 1m 1s
    LineInt("CTimeSpan.explicitSecondsCtor.GetHours", fromSeconds.GetHours());
    LineInt("CTimeSpan.explicitSecondsCtor.GetTotalSeconds", static_cast<long long>(fromSeconds.GetTotalSeconds()));

    CTimeSpan spanA(0, 1, 0, 0);  // 1 hour
    CTimeSpan spanB(0, 0, 30, 0); // 30 minutes
    CTimeSpan spanSum = spanA + spanB;
    LineInt("CTimeSpan.operatorPlus.GetTotalMinutes", static_cast<long long>(spanSum.GetTotalMinutes()));
    CTimeSpan spanDiff = spanA - spanB;
    LineInt("CTimeSpan.operatorMinus.GetTotalMinutes", static_cast<long long>(spanDiff.GetTotalMinutes()));

    // --- Higher-variability dates -------------------------------------
    // A second, structurally different date (leap year, midnight, first of
    // the year) plus additional numeric-only Format patterns. Weekday/month
    // *names* (%A/%B/%p) are deliberately avoided: they are locale-dependent
    // and not the subject of this comparison.
    CTime t2000(2000, 1, 1, 0, 0, 0);
    LineInt("CTime.2000.GetYear", t2000.GetYear());
    LineInt("CTime.2000.GetMonth", t2000.GetMonth());
    LineInt("CTime.2000.GetDay", t2000.GetDay());
    LineInt("CTime.2000.GetDayOfWeek", t2000.GetDayOfWeek());
    Line("CTime.2000.Format", t2000.Format(L"%Y/%m/%d %H:%M:%S day%j"));

    CTime tLeap(2024, 2, 29, 23, 59, 59); // valid only in a leap year
    LineInt("CTime.Leap.GetMonth", tLeap.GetMonth());
    LineInt("CTime.Leap.GetDay", tLeap.GetDay());
    LineInt("CTime.Leap.GetDayOfWeek", tLeap.GetDayOfWeek());
    Line("CTime.Leap.Format", tLeap.Format(L"%y-%m-%dT%H:%M:%S"));

    // A negative span (earlier minus later) exercises the sign handling of
    // every accessor.
    CTimeSpan neg = t1 - t2; // t1 < t2, so this is negative
    LineInt("CTimeSpan.negative.GetTotalSeconds", static_cast<long long>(neg.GetTotalSeconds()));
    LineInt("CTimeSpan.negative.GetDays", neg.GetDays());
}

// ---------------------------------------------------------------------
// CCriticalSection / CEvent / CMutex / CSingleLock (behavioral)
// ---------------------------------------------------------------------
static void TestCriticalSection()
{
    CCriticalSection cs;
    long counter = 0;
    auto worker = [&]
    {
        for (int i = 0; i < 5000; ++i)
        {
            CSingleLock lock(&cs, TRUE);
            ++counter;
        }
    };
    std::thread t1(worker), t2(worker), t3(worker), t4(worker);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    LineInt("CCriticalSection.counter_after_4x5000", counter);

    // Direct (uncontended) Lock()/Unlock() calls, not just through
    // CSingleLock, plus the Lock(DWORD) overload with an explicit timeout.
    BOOL directLocked = cs.Lock();
    LineBool("CCriticalSection.Lock.direct", directLocked != FALSE);
    BOOL directUnlocked = cs.Unlock();
    LineBool("CCriticalSection.Unlock.direct", directUnlocked != FALSE);
    BOOL directLockedTimeout = cs.Lock(1000);
    LineBool("CCriticalSection.LockWithTimeout.direct", directLockedTimeout != FALSE);
    cs.Unlock();
}

static void TestEventAutoReset()
{
    CEvent ev(FALSE, FALSE); // not signaled, auto-reset
    std::atomic<int> woken{0};
    std::thread waiter([&] { if (ev.Lock(3000)) ++woken; });
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    LineInt("CEvent.AutoReset.woken_before_set", woken.load());
    ev.SetEvent();
    waiter.join();
    LineInt("CEvent.AutoReset.woken_after_set", woken.load());
}

static void TestEventManualReset()
{
    CEvent ev(FALSE, TRUE); // not signaled, manual-reset
    ev.SetEvent();
    BOOL first = ev.Lock(1000);
    BOOL second = ev.Lock(1000); // still signaled: must also succeed
    ev.ResetEvent();
    BOOL third = ev.Lock(200); // now times out
    LineBool("CEvent.ManualReset.firstLock", first != FALSE);
    LineBool("CEvent.ManualReset.secondLock", second != FALSE);
    LineBool("CEvent.ManualReset.thirdLockTimesOut", third == FALSE);
}

static void TestEventPulseAndUnlock()
{
    CEvent ev(FALSE, FALSE); // not signaled, auto-reset

    // CEvent::Unlock(): a documented no-op on real MFC (events have no
    // true "unlock" concept) — just verify it returns TRUE, matching.
    LineBool("CEvent.Unlock.noop", ev.Unlock() != FALSE);

    // PulseEvent: signals waiters then immediately un-signals again,
    // unlike SetEvent (which stays signaled until consumed/reset). Real
    // Win32 PulseEvent has a well-known, documented race: a waiter only
    // catches the pulse if it is already blocked in the wait *and* gets
    // scheduled before the un-signal happens right after — this is
    // exactly why Microsoft deprecated it. Since that race exists on real
    // MFC too (not just here), whether a given waiter catches a given
    // pulse is not a fair byte-for-byte comparison; only PulseEvent's
    // deterministic return value and its guaranteed after-effect (not
    // left signaled) are compared.
    BOOL pulseResult = ev.PulseEvent();
    LineBool("CEvent.PulseEvent.returns_true", pulseResult != FALSE);

    BOOL afterPulse = ev.Lock(200); // must time out: not left signaled
    LineBool("CEvent.AfterPulse.timesOut", afterPulse == FALSE);
}

static void TestMutex()
{
    CMutex mtx;
    CSingleLock lk(&mtx, TRUE);
    LineBool("CMutex.SingleLock.locked", lk.IsLocked() != FALSE);
    lk.Unlock();
    LineBool("CMutex.SingleLock.unlockedAfterUnlock", lk.IsLocked() == FALSE);

    // CSingleLock::Unlock(LONG, LONG*): the release-count overload.
    CSingleLock lk2(&mtx, TRUE);
    LONG prevCount = -1;
    BOOL unlockedWithCount = lk2.Unlock(1, &prevCount);
    LineBool("CSingleLock.Unlock2Arg", unlockedWithCount != FALSE);
}

// ---------------------------------------------------------------------
// CArchive (afx.h). The one class here whose output outlives the process:
// eMule stores part-file metadata through it, so a divergence in the byte
// stream silently corrupts real user data rather than merely returning a
// wrong value. The raw bytes are therefore compared, not just the values
// read back out of them.
//
// CString is deliberately excluded from the byte-level comparison: afx.h
// documents simple_mfc's CString serialization as a plain 32-bit length
// prefix plus raw wchar_t payload, which is NOT real MFC's format (MFC
// uses a variable-width length prefix inherited from its 16-bit past).
// That is a known, recorded divergence, not something this suite should
// re-discover on every run -- so only the round trip is checked for it.
// ---------------------------------------------------------------------
static void TestCArchive()
{
    CMemFile mf;
    {
        CArchive ar(&mf, CArchive::store);
        LineBool("CArchive.store.IsStoring", ar.IsStoring() != FALSE);
        LineBool("CArchive.store.IsLoading", ar.IsLoading() != FALSE);

        ar << static_cast<BYTE>(0xAB);
        ar << static_cast<WORD>(0x1234);
        ar << static_cast<int>(-123456);
        ar << static_cast<UINT>(3000000000u);
        ar << static_cast<long>(-1);
        ar << static_cast<DWORD>(0xDEADBEEF);
        ar << 1.5f;
        ar << -2.25;
        ar << static_cast<ULONGLONG>(0x0102030405060708ull);
        ar.Close();
    }

    // The stream itself, byte for byte.
    ULONGLONG len = mf.GetLength();
    LineInt("CArchive.store.byteCount", static_cast<long long>(len));
    mf.SeekToBegin();
    std::vector<unsigned char> raw(static_cast<size_t>(len));
    if (!raw.empty())
        mf.Read(raw.data(), static_cast<UINT>(raw.size()));
    Line("CArchive.store.bytes", Hex(raw.data(), raw.size()));

    // ...and the values it reads back.
    mf.SeekToBegin();
    {
        CArchive ar(&mf, CArchive::load);
        LineBool("CArchive.load.IsLoading", ar.IsLoading() != FALSE);
        LineBool("CArchive.load.IsStoring", ar.IsStoring() != FALSE);

        BYTE by = 0;
        WORD w = 0;
        int i = 0;
        UINT u = 0;
        long l = 0;
        DWORD dw = 0;
        float f = 0.0f;
        double d = 0.0;
        ULONGLONG q = 0;
        ar >> by >> w >> i >> u >> l >> dw >> f >> d >> q;
        ar.Close();

        LineInt("CArchive.roundTrip.BYTE", by);
        LineInt("CArchive.roundTrip.WORD", w);
        LineInt("CArchive.roundTrip.int", i);
        LineInt("CArchive.roundTrip.UINT", u);
        LineInt("CArchive.roundTrip.long", l);
        LineInt("CArchive.roundTrip.DWORD", static_cast<long long>(dw));
        // Printed as bytes rather than as text: a decimal rendering would
        // compare the CRT's float formatting, not the archived value.
        Line("CArchive.roundTrip.float", Hex(&f, sizeof(f)));
        Line("CArchive.roundTrip.double", Hex(&d, sizeof(d)));
        LineInt("CArchive.roundTrip.ULONGLONG", static_cast<long long>(q));
    }

    // CString round trip only (format divergence documented above).
    CMemFile mfs;
    {
        CArchive ar(&mfs, CArchive::store);
        ar << CString(L"archived string");
        ar.Close();
    }
    mfs.SeekToBegin();
    {
        CArchive ar(&mfs, CArchive::load);
        CString s;
        ar >> s;
        ar.Close();
        Line("CArchive.roundTrip.CString", s);
    }
}

// ---------------------------------------------------------------------
// CMemFile::Detach / Attach (afx.h). simple_mfc's storage is a vector, so
// it cannot literally hand over a malloc'd block the way real MFC does --
// it copies instead. That is an ownership difference, not a behavioral
// one, and only the observable behavior is compared here.
// ---------------------------------------------------------------------
static void TestCMemFileDetachAttach()
{
    CMemFile mf;
    const char payload[] = "detach-and-reattach";
    const UINT payloadLen = static_cast<UINT>(sizeof(payload) - 1);
    mf.Write(payload, payloadLen);
    LineInt("CMemFile.Detach.lengthBefore", static_cast<long long>(mf.GetLength()));

    BYTE* raw = mf.Detach();
    LineBool("CMemFile.Detach.nonNull", raw != nullptr);
    LineInt("CMemFile.Detach.lengthAfter", static_cast<long long>(mf.GetLength()));

    CMemFile mf2;
    mf2.Attach(raw, payloadLen);
    LineInt("CMemFile.Attach.length", static_cast<long long>(mf2.GetLength()));
    mf2.SeekToBegin();
    char buf[64]{};
    UINT n = mf2.Read(buf, payloadLen);
    LineInt("CMemFile.Attach.readCount", n);
    Line("CMemFile.Attach.content", std::string(buf, n));
}

// ---------------------------------------------------------------------
// CTempBuffer (atlalloc.h): the fixed/stack path, the heap path, and the
// grow-and-preserve path across the boundary between them.
// ---------------------------------------------------------------------
static void TestCTempBuffer()
{
    // 64 fixed bytes == 16 ints: this stays on the stack throughout.
    CTempBuffer<int, 64> fixedBuf;
    fixedBuf.Allocate(8);
    for (int i = 0; i < 8; ++i)
        fixedBuf[i] = i * 3;
    std::string fixedVals;
    for (int i = 0; i < 8; ++i)
        fixedVals += std::to_string(fixedBuf[i]) + (i == 7 ? "" : ",");
    Line("CTempBuffer.fixed.values", fixedVals);

    // 16 fixed bytes == 4 ints: asking for 100 forces the heap path.
    CTempBuffer<int, 16> heapBuf;
    heapBuf.Allocate(100);
    for (int i = 0; i < 100; ++i)
        heapBuf[i] = i;
    LineInt("CTempBuffer.heap.first", heapBuf[0]);
    LineInt("CTempBuffer.heap.last", heapBuf[99]);

    // Reallocate across the fixed->heap boundary, reading back values
    // written BEFORE the move. atlalloc.h's own comment claims real ATL
    // preserves the contents here; this case is what turns that claim
    // from an assertion into something CI verifies, and it does hold.
    //
    // It is worth knowing that ATL documents no contract either way (there
    // is no CTempBuffer reference page, and the memory-management overview
    // says nothing about Reallocate and existing contents). So if this
    // case ever starts failing, read it as "Microsoft changed an
    // undocumented implementation detail", not as a simple_mfc regression
    // -- and expect the real-MFC side to print garbage rather than a clean
    // difference, since not preserving means reading uninitialized memory.
    CTempBuffer<int, 16> growBuf;
    growBuf.Allocate(4);
    for (int i = 0; i < 4; ++i)
        growBuf[i] = 100 + i;
    growBuf.Reallocate(64);
    std::string preserved;
    for (int i = 0; i < 4; ++i)
        preserved += std::to_string(growBuf[i]) + (i == 3 ? "" : ",");
    Line("CTempBuffer.growPreservesContent", preserved);

    CTempBuffer<char, 32> byteBuf;
    byteBuf.AllocateBytes(200);
    byteBuf[0] = 'A';
    byteBuf[199] = 'Z';
    Line("CTempBuffer.AllocateBytes.ends", std::string(1, byteBuf[0]) + std::string(1, byteBuf[199]));
}

// ---------------------------------------------------------------------
// CSimpleArray (atlsimpcoll.h). ATL's lightweight vector. Only the subset
// eMule uses is exercised (Add/GetSize/operator[]/GetData/Find/Remove/
// RemoveAt/RemoveAll) -- the same methods simple_mfc declares. int
// elements keep every result fully comparable.
// ---------------------------------------------------------------------
static void TestCSimpleArray()
{
    CSimpleArray<int> arr;
    for (int v : {10, 20, 30, 40, 20})
        arr.Add(v);
    LineInt("CSimpleArray.GetSize", arr.GetSize());
    LineInt("CSimpleArray.index0", arr[0]);
    LineInt("CSimpleArray.index4", arr[4]);

    // GetData() returns the raw block; read it back element by element.
    const int* data = arr.GetData();
    std::string contents;
    for (int i = 0; i < arr.GetSize(); ++i)
        contents += std::to_string(data[i]) + (i + 1 < arr.GetSize() ? "," : "");
    Line("CSimpleArray.GetData.contents", contents);

    // Find returns the index of the FIRST match, or -1.
    LineInt("CSimpleArray.Find.present", arr.Find(30));
    LineInt("CSimpleArray.Find.firstOfDup", arr.Find(20)); // two 20s -> first index
    LineInt("CSimpleArray.Find.absent", arr.Find(999));

    // Remove(value) drops the first match; RemoveAt(index) drops by position.
    LineInt("CSimpleArray.Remove.present", arr.Remove(20) != FALSE ? 1 : 0);
    LineInt("CSimpleArray.GetSize.afterRemove", arr.GetSize());
    LineInt("CSimpleArray.Remove.absent", arr.Remove(999) != FALSE ? 1 : 0);
    LineInt("CSimpleArray.RemoveAt.mid", arr.RemoveAt(1) != FALSE ? 1 : 0);

    std::string afterRemovals;
    for (int i = 0; i < arr.GetSize(); ++i)
        afterRemovals += std::to_string(arr[i]) + (i + 1 < arr.GetSize() ? "," : "");
    Line("CSimpleArray.afterRemovals", afterRemovals);

    arr.RemoveAll();
    LineInt("CSimpleArray.GetSize.afterRemoveAll", arr.GetSize());
}

// ---------------------------------------------------------------------
// CRBMap (atlcoll.h). ATL's ordered map. eMule's only instance is
// CBarShader::m_Spans, a CRBMap<uint64, COLORREF>; this mirrors that
// (ULONGLONG key, DWORD value). Crucially the map is ORDERED, so unlike
// CMap's hash iteration this traversal order IS deterministic and can be
// compared in full. POSITION values themselves are node addresses and
// differ between the two probes by construction, so they are never
// printed -- only the keys/values reached through them. State (count and
// contents) is derived purely from the eMule-used traversal API
// (GetHeadPosition + GetNext), never from GetCount/IsEmpty, which eMule
// does not use and simple_mfc therefore does not declare.
// ---------------------------------------------------------------------
namespace
{
// Forward traversal of the whole map as "k=v,k=v,..." in key order, plus
// the element count, both from GetHeadPosition/GetNext alone.
std::string RbForward(CRBMap<ULONGLONG, DWORD>& m, int& count)
{
    std::string out;
    count = 0;
    POSITION pos = m.GetHeadPosition();
    while (pos)
    {
        // GetNext returns the pair at pos, then advances pos.
        auto* pair = m.GetNext(pos);
        if (count) out += ",";
        out += std::to_string(pair->m_key) + "=" + std::to_string(pair->m_value);
        ++count;
    }
    return out;
}
} // namespace

static void TestCRBMap()
{
    CRBMap<ULONGLONG, DWORD> m;
    // Insert out of key order; the map must present them sorted.
    m.SetAt(50, 500);
    m.SetAt(10, 100);
    m.SetAt(40, 400);
    m.SetAt(20, 200);
    m.SetAt(30, 300);

    int count = 0;
    std::string fwd = RbForward(m, count);
    LineInt("CRBMap.count", count);
    Line("CRBMap.ordered", fwd);

    // SetAt on an existing key overwrites in place (no duplicate, no
    // reorder): count stays, value updates.
    m.SetAt(30, 333);
    int count2 = 0;
    std::string fwd2 = RbForward(m, count2);
    LineInt("CRBMap.count.afterOverwrite", count2);
    Line("CRBMap.ordered.afterOverwrite", fwd2);

    // Head/Tail keys, and the value at the head, via the used accessors.
    POSITION head = m.GetHeadPosition();
    LineInt("CRBMap.headKey", static_cast<long long>(m.GetKeyAt(head)));
    LineInt("CRBMap.headValue", static_cast<long long>(m.GetValueAt(head)));
    POSITION tail = m.GetTailPosition();
    LineInt("CRBMap.tailKey", static_cast<long long>(m.GetKeyAt(tail)));

    // Backward traversal from the tail via GetPrev (the exact BarShader
    // idiom), rendered as "k=v,..." in descending key order.
    {
        std::string back;
        int n = 0;
        POSITION pos = m.GetTailPosition();
        while (pos)
        {
            auto* pair = m.GetPrev(pos);
            if (n) back += ",";
            back += std::to_string(pair->m_key) + "=" + std::to_string(pair->m_value);
            ++n;
        }
        Line("CRBMap.reversed", back);
    }

    // GetNextValue: like GetNext but yields the value and advances.
    {
        std::string vals;
        int n = 0;
        POSITION pos = m.GetHeadPosition();
        while (pos)
        {
            DWORD v = m.GetNextValue(pos);
            if (n) vals += ",";
            vals += std::to_string(v);
            ++n;
        }
        Line("CRBMap.valuesInOrder", vals);
    }

    // FindFirstKeyAfter = std::upper_bound: first key STRICTLY GREATER than
    // the argument (the name is literal). Probe an exact hit (30 is
    // present, so the answer is the NEXT key, 40), a gap (25 -> 30), and
    // past the end (no position).
    POSITION exact = m.FindFirstKeyAfter(30);
    LineInt("CRBMap.FindFirstKeyAfter.exact", static_cast<long long>(m.GetKeyAt(exact))); // -> 40
    POSITION gap = m.FindFirstKeyAfter(25); // -> 30
    LineInt("CRBMap.FindFirstKeyAfter.gap", static_cast<long long>(m.GetKeyAt(gap)));
    POSITION past = m.FindFirstKeyAfter(1000); // beyond max key -> none
    LineBool("CRBMap.FindFirstKeyAfter.pastEnd.none", past == nullptr);

    // RemoveAt(head) drops the smallest key; re-traverse.
    m.RemoveAt(m.GetHeadPosition());
    int count3 = 0;
    std::string fwd3 = RbForward(m, count3);
    LineInt("CRBMap.count.afterRemoveHead", count3);
    Line("CRBMap.ordered.afterRemoveHead", fwd3);

    m.RemoveAll();
    LineBool("CRBMap.emptyAfterRemoveAll", m.GetHeadPosition() == nullptr);
}

// ---------------------------------------------------------------------
// AfxParseURL (afxinet.h). Pure string parsing -- no socket is opened and
// no WinInet call is made -- which is what makes it comparable at all;
// the rest of afxinet.h is a live network surface this suite has no
// business touching.
// ---------------------------------------------------------------------
// The dwServiceType AfxParseURL writes is an OPAQUE, header-defined token,
// not a portable number: real MFC's AFX_INET_SERVICE_HTTPS is 4107, while
// simple_mfc's afxinet.h defines it as 4, and the two probes include
// different headers by construction -- so the raw value differs even when
// the classification is identical. What is actually meaningful, and what
// eMule itself relies on (it compares m_dwServiceType against the
// AFX_INET_SERVICE_* names), is *which* service was recognized. Map the
// value back to a name through each side's own constants, and compare
// that. FTP(1)/HTTP(3) happen to share a value across both header sets;
// HTTPS is exactly where they don't, which is why the raw form failed.
static std::string ServiceName(DWORD service)
{
    if (service == AFX_INET_SERVICE_HTTP) return "HTTP";
    if (service == AFX_INET_SERVICE_HTTPS) return "HTTPS";
    if (service == AFX_INET_SERVICE_FTP) return "FTP";
    return "OTHER(" + std::to_string(service) + ")";
}

static void TestAfxParseURL()
{
    struct Case
    {
        const char* label;
        LPCTSTR url;
    };
    // Only schemes eMule actually feeds AfxParseURL (http/https, plus ftp
    // which shares MFC's classification cleanly). Exotic schemes real MFC
    // happens to recognize via InternetCrackUrl (gopher, file, ...) are
    // deliberately not tested: eMule never passes them, and real MFC's
    // handling of them is not a contract simple_mfc set out to reproduce.
    const Case kCases[] = {
        {"http.explicitPort", L"http://example.com:8080/path/to/file"},
        {"https.defaultPort", L"https://example.com/index.html"},
        {"https.explicitPort", L"https://secure.example.com:8443/a"},
        {"http.defaultPort", L"http://example.com/"},
        {"http.noObject", L"http://example.com"},
        {"ftp.explicitPort", L"ftp://files.example.com:2121/pub/readme.txt"},
        {"ftp.defaultPort", L"ftp://files.example.com/pub/"},
        {"http.query", L"http://example.com/search?q=mfc&lang=en"},
        {"http.deepPath", L"http://example.com/a/b/c/d.html"},
        // The failure cases eMule's retry logic depends on. A schemeless
        // URL MUST fail (HttpDownloadDlg.cpp then prepends "http://" and
        // retries); an empty one likewise.
        {"schemeless.fails", L"example.com/path"},
        {"empty.fails", L""},
    };

    for (const Case& c : kCases)
    {
        DWORD service = 0;
        CString server, object;
        INTERNET_PORT port = 0;
        BOOL ok = AfxParseURL(c.url, service, server, object, port);

        // One record per case: these outputs are a single parse result and
        // are only meaningful together. On failure only the boolean is
        // compared -- AfxParseURL is documented as "nonzero if successful;
        // otherwise 0" and says nothing about the out-parameters when it
        // fails, so their contents there are unspecified (same reasoning
        // this suite already applies to GetErrorMessage's text).
        std::string label = std::string("AfxParseURL.") + c.label;
        std::string value = "0";
        if (ok)
        {
            value = std::string("1 service=") + ServiceName(service) +
                    " server=" + Utf8((LPCTSTR)server) +
                    " object=" + Utf8((LPCTSTR)object) +
                    " port=" + std::to_string(port);
        }
        Line(label.c_str(), value);
    }
}

// ---------------------------------------------------------------------
// Pattern-driven cases: instead of one hand-picked value per assertion,
// generate N inputs from a fixed-seed PRNG and run the same call on each.
// std::mt19937's algorithm is fully specified by the standard, so a given
// seed produces a byte-identical draw sequence in both probes -- they are
// two separate processes/binaries, but compiled from this same source
// file (see the file header), so "the same seed" really does mean "the
// same inputs" here, with no need to serialize/replay anything between
// them. Each case gets a unique, self-describing label (e.g.
// "Pattern.CString.Format.03") so a mismatch in compare.py's output
// names the exact iteration to reproduce, without re-running anything.
// ---------------------------------------------------------------------
namespace
{
constexpr unsigned kPatternSeed = 20260722u;

// A short, printable-ASCII-only random word: CString/CRC-style tests
// intentionally avoid non-ASCII input, matching the documented,
// deliberately-deferred MakeUpper/MakeLower/CompareNoCase gap (see
// README.md "Known conformance gaps") -- this generator is not the place
// to accidentally re-open it.
std::string RandomAsciiWord(std::mt19937& rng, int minLen, int maxLen)
{
    std::uniform_int_distribution<int> lenDist(minLen, maxLen);
    std::uniform_int_distribution<int> chDist('a', 'z');
    int n = lenDist(rng);
    std::string s;
    s.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i)
        s.push_back(static_cast<char>(chDist(rng)));
    return s;
}

bool IsLeapYear(int y) { return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0); }

int DaysInMonth(int y, int m)
{
    static const int kDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m == 2 && IsLeapYear(y)) return 29;
    return kDays[m - 1];
}
} // namespace

static void TestPatternCString()
{
    std::mt19937 rng(kPatternSeed);
    std::uniform_int_distribution<int> intDist(-100000, 100000);
    std::uniform_real_distribution<double> fltDist(-10000.0, 10000.0);
    std::uniform_int_distribution<int> widthDist(0, 12);
    std::uniform_int_distribution<int> precDist(0, 6);

    for (int i = 0; i < 40; ++i)
    {
        char label[64];
        std::snprintf(label, sizeof(label), "Pattern.CString.Format.%02d", i);

        int n = intDist(rng);
        double d = fltDist(rng);
        std::string word = RandomAsciiWord(rng, 1, 10);
        int width = widthDist(rng);
        int prec = precDist(rng);

        CStringA wordA(word.c_str());
        CString wordW(wordA); // explicit cross-width (narrow->wide) converting ctor

        CString fmt;
        switch (i % 5)
        {
        case 0: fmt.Format(L"%d|%s", n, (LPCTSTR)wordW); break;
        case 1: fmt.Format(L"%*d", width, n); break;
        case 2: fmt.Format(L"%.*f", prec, d); break;
        case 3: fmt.Format(L"%08X", static_cast<unsigned int>(n)); break;
        default: fmt.Format(L"[%-*s]=%+d", width, (LPCTSTR)wordW, n); break;
        }
        Line(label, fmt);
    }
}

static void TestPatternCTime()
{
    std::mt19937 rng(kPatternSeed + 2);
    std::uniform_int_distribution<int> yearDist(1970, 2099);
    std::uniform_int_distribution<int> monthDist(1, 12);
    std::uniform_int_distribution<int> hourDist(0, 23);
    std::uniform_int_distribution<int> minSecDist(0, 59);

    CTime prev;
    bool havePrev = false;
    for (int i = 0; i < 24; ++i)
    {
        int y = yearDist(rng);
        int mo = monthDist(rng);
        std::uniform_int_distribution<int> dayDist(1, DaysInMonth(y, mo));
        int d = dayDist(rng);
        int h = hourDist(rng);
        int mi = minSecDist(rng);
        int se = minSecDist(rng);

        CTime t(y, mo, d, h, mi, se);

        char label[64];
        std::snprintf(label, sizeof(label), "Pattern.CTime.%02d", i);
        std::string s = std::to_string(t.GetYear()) + "-" + std::to_string(t.GetMonth()) + "-" +
                         std::to_string(t.GetDay()) + " " + std::to_string(t.GetHour()) + ":" +
                         std::to_string(t.GetMinute()) + ":" + std::to_string(t.GetSecond()) +
                         " dow=" + std::to_string(t.GetDayOfWeek());
        Line(label, s);

        if (havePrev)
        {
            CTimeSpan diff = t - prev;
            char labelDiff[64];
            std::snprintf(labelDiff, sizeof(labelDiff), "Pattern.CTimeSpan.Diff.%02d", i);
            LineInt(labelDiff, static_cast<long long>(diff.GetTotalSeconds()));
        }
        prev = t;
        havePrev = true;
    }
}

static void TestPatternBase64()
{
    std::mt19937 rng(kPatternSeed + 3);
    std::uniform_int_distribution<int> lenDist(0, 300);
    std::uniform_int_distribution<int> byteDist(0, 255);

    for (int i = 0; i < 30; ++i)
    {
        int n = lenDist(rng);
        std::vector<BYTE> buf(static_cast<size_t>(n));
        for (auto& b : buf) b = static_cast<BYTE>(byteDist(rng));

        // Odd iterations exercise the CRLF-wrapping path (real MIME line
        // breaks every 76 output chars) against real ATL -- eMule itself
        // only ever passes NOCRLF, so this is otherwise unvalidated logic.
        DWORD flags = (i % 2 == 0) ? ATL_BASE64_FLAG_NOCRLF : ATL_BASE64_FLAG_NONE;

        int needed = Base64EncodeGetRequiredLength(n, flags);
        std::vector<char> dst(static_cast<size_t>(needed) + 1, 0);
        int outLen = needed;
        BOOL ok = Base64Encode(buf.empty() ? nullptr : buf.data(), n, dst.data(), &outLen, flags);

        char label[64];
        std::snprintf(label, sizeof(label), "Pattern.Base64.%02d", i);
        std::string s = std::string(ok ? "1:" : "0:") + std::to_string(outLen) + ":" +
                         std::string(dst.data(), static_cast<size_t>(outLen));
        Line(label, s);
    }
}

static void TestPatternUnicodeToUtf8()
{
    std::mt19937 rng(kPatternSeed + 4);
    std::uniform_int_distribution<int> lenDist(1, 20);
    // Weighted ranges: plain ASCII, Latin-1 supplement, general BMP
    // (avoiding the surrogate range D800-DFFF on its own), and a
    // surrogate-pair marker handled specially below.
    std::uniform_int_distribution<int> kindDist(0, 3);
    std::uniform_int_distribution<int> asciiDist(0x20, 0x7E);
    std::uniform_int_distribution<int> latin1Dist(0xA0, 0xFF);
    std::uniform_int_distribution<int> bmpDist(0x0100, 0x2FFF);
    std::uniform_int_distribution<int> highSurrDist(0xD800, 0xDBFF);
    std::uniform_int_distribution<int> lowSurrDist(0xDC00, 0xDFFF);

    for (int i = 0; i < 30; ++i)
    {
        int n = lenDist(rng);
        std::wstring w;
        w.reserve(static_cast<size_t>(n) * 2);
        for (int c = 0; c < n; ++c)
        {
            switch (kindDist(rng))
            {
            case 0: w.push_back(static_cast<wchar_t>(asciiDist(rng))); break;
            case 1: w.push_back(static_cast<wchar_t>(latin1Dist(rng))); break;
            case 2: w.push_back(static_cast<wchar_t>(bmpDist(rng))); break;
            default:
                w.push_back(static_cast<wchar_t>(highSurrDist(rng)));
                w.push_back(static_cast<wchar_t>(lowSurrDist(rng)));
                break;
            }
        }
        w.push_back(0); // include the terminator, matching eMule's own two-pass usage

        int srcChars = static_cast<int>(w.size());
        int needed = AtlUnicodeToUTF8(w.c_str(), srcChars, nullptr, 0);
        std::vector<char> dst(static_cast<size_t>(needed > 0 ? needed : 1), 0);
        int outLen = AtlUnicodeToUTF8(w.c_str(), srcChars, dst.data(), needed);

        char label[64];
        std::snprintf(label, sizeof(label), "Pattern.AtlUnicodeToUTF8.%02d", i);
        std::string s = std::to_string(outLen) + ":" + std::string(dst.data(), static_cast<size_t>(outLen > 0 ? outLen : 0));
        Line(label, s);
    }
}

// ---------------------------------------------------------------------
// CMapPtrToPtr (afxcoll.h). The hash-map surface: SetAt/Lookup/RemoveKey/
// RemoveAll plus the GetStartPosition/GetNextAssoc walk.
//
// Keys and values are addresses, which differ between runs, so nothing
// prints a pointer: every key/value is a slot in one fixed array and what
// is compared is its INDEX. Bucket ORDER is not compared either -- real
// MFC's hash table and this branch's std::unordered_map are free to lay a
// map out differently, and do; the walk below is sorted so that what the
// case asserts is the set of associations, which IS a contract.
// ---------------------------------------------------------------------
static int g_slots[6] = {10, 11, 12, 13, 14, 15};

static std::string SortedJoin(std::vector<std::string> v)
{
    std::sort(v.begin(), v.end());
    std::string out;
    for (size_t i = 0; i < v.size(); ++i)
    {
        if (i) out += ",";
        out += v[i];
    }
    return out;
}

static void TestCMapPtrToPtr()
{
    CMapPtrToPtr m;
    LineInt("CMapPtrToPtr.GetCount.empty", m.GetCount());
    LineBool("CMapPtrToPtr.IsEmpty.empty", m.IsEmpty() != FALSE);
    LineBool("CMapPtrToPtr.GetStartPosition.empty_is_null", m.GetStartPosition() == nullptr);

    for (int i = 0; i < 5; ++i)
        m.SetAt(&g_slots[i], &g_slots[i + 1]);
    LineInt("CMapPtrToPtr.GetCount.after_5", m.GetCount());
    LineBool("CMapPtrToPtr.IsEmpty.after_5", m.IsEmpty() != FALSE);

    void* found = nullptr;
    LineBool("CMapPtrToPtr.Lookup.hit", m.Lookup(&g_slots[2], found) != FALSE);
    LineInt("CMapPtrToPtr.Lookup.hit.value", found ? *static_cast<int*>(found) : -1);
    void* missed = nullptr;
    LineBool("CMapPtrToPtr.Lookup.miss", m.Lookup(&g_slots[5], missed) != FALSE);

    // SetAt on an existing key replaces rather than inserts.
    m.SetAt(&g_slots[2], &g_slots[0]);
    LineInt("CMapPtrToPtr.GetCount.after_overwrite", m.GetCount());
    m.Lookup(&g_slots[2], found);
    LineInt("CMapPtrToPtr.Lookup.after_overwrite.value", found ? *static_cast<int*>(found) : -1);

    std::vector<std::string> assoc;
    POSITION pos = m.GetStartPosition();
    while (pos != nullptr)
    {
        void* key = nullptr;
        void* value = nullptr;
        m.GetNextAssoc(pos, key, value);
        assoc.push_back(std::to_string(*static_cast<int*>(key)) + ">" +
                        std::to_string(*static_cast<int*>(value)));
    }
    Line("CMapPtrToPtr.walk.sorted", SortedJoin(assoc));

    LineBool("CMapPtrToPtr.RemoveKey.present", m.RemoveKey(&g_slots[0]) != FALSE);
    LineBool("CMapPtrToPtr.RemoveKey.absent", m.RemoveKey(&g_slots[0]) != FALSE);
    LineInt("CMapPtrToPtr.GetCount.after_remove", m.GetCount());
    m.RemoveAll();
    LineInt("CMapPtrToPtr.GetCount.after_RemoveAll", m.GetCount());
    LineBool("CMapPtrToPtr.IsEmpty.after_RemoveAll", m.IsEmpty() != FALSE);
}

// ---------------------------------------------------------------------
// CMapStringToPtr (afxcoll.h). Same surface as above with a CString key,
// plus the CPair-based walk (PGetFirstAssoc/PGetNextAssoc/PLookup) that
// real MFC added alongside GetNextAssoc.
// ---------------------------------------------------------------------
static void TestCMapStringToPtr()
{
    CMapStringToPtr m;
    m.InitHashTable(17);
    LineInt("CMapStringToPtr.GetCount.empty", m.GetCount());
    LineBool("CMapStringToPtr.IsEmpty.empty", m.IsEmpty() != FALSE);

    static const LPCTSTR kKeys[] = {L"alpha", L"beta", L"gamma", L"delta"};
    for (int i = 0; i < 4; ++i)
        m.SetAt(kKeys[i], &g_slots[i]);
    LineInt("CMapStringToPtr.GetCount.after_4", m.GetCount());

    void* found = nullptr;
    LineBool("CMapStringToPtr.Lookup.hit", m.Lookup(L"gamma", found) != FALSE);
    LineInt("CMapStringToPtr.Lookup.hit.value", found ? *static_cast<int*>(found) : -1);
    void* missed = nullptr;
    LineBool("CMapStringToPtr.Lookup.miss", m.Lookup(L"epsilon", missed) != FALSE);
    // Lookup is by string CONTENT, not by buffer identity: a key built at
    // run time must find the entry inserted under a literal.
    CString built = L"ga";
    built += L"mma";
    LineBool("CMapStringToPtr.Lookup.by_content", m.Lookup(built, found) != FALSE);

    // operator[] inserts a default-constructed value for an absent key.
    m[L"epsilon"] = &g_slots[4];
    LineInt("CMapStringToPtr.GetCount.after_subscript", m.GetCount());

    std::vector<std::string> assoc;
    POSITION pos = m.GetStartPosition();
    while (pos != nullptr)
    {
        CString key;
        void* value = nullptr;
        m.GetNextAssoc(pos, key, value);
        assoc.push_back(Utf8(key) + ">" + std::to_string(*static_cast<int*>(value)));
    }
    Line("CMapStringToPtr.walk.sorted", SortedJoin(assoc));

    // No CPair walk here on purpose: real MFC gives PLookup/PGetFirstAssoc/
    // PGetNextAssoc to CMap and to CMapStringToString, but NOT to
    // CMapStringToPtr -- and eMule's own CPair walks are all over CMap
    // typedefs (CKnownFilesMap, CServerSocketMap, CClientVersionMap), which
    // TestCMapTemplate covers.

    LineBool("CMapStringToPtr.RemoveKey.present", m.RemoveKey(L"alpha") != FALSE);
    LineBool("CMapStringToPtr.RemoveKey.absent", m.RemoveKey(L"alpha") != FALSE);
    LineInt("CMapStringToPtr.GetCount.after_remove", m.GetCount());
    m.RemoveAll();
    LineInt("CMapStringToPtr.GetCount.after_RemoveAll", m.GetCount());
}

// ---------------------------------------------------------------------
// CMapStringToString (afxcoll.h). The one collection eMule uses as a
// plain string->string dictionary.
// ---------------------------------------------------------------------
static void TestCMapStringToString()
{
    CMapStringToString m;
    LineInt("CMapStringToString.GetCount.empty", m.GetCount());

    m.SetAt(L"one", L"uno");
    m.SetAt(L"two", L"due");
    m.SetAt(L"three", L"tre");
    LineInt("CMapStringToString.GetCount.after_3", m.GetCount());

    CString value;
    LineBool("CMapStringToString.Lookup.hit", m.Lookup(L"two", value) != FALSE);
    Line("CMapStringToString.Lookup.hit.value", value);
    CString absent = L"untouched";
    LineBool("CMapStringToString.Lookup.miss", m.Lookup(L"four", absent) != FALSE);
    Line("CMapStringToString.Lookup.miss.leaves_value", absent);

    // Overwriting an existing key.
    m.SetAt(L"two", L"DUE");
    m.Lookup(L"two", value);
    Line("CMapStringToString.SetAt.overwrite.value", value);
    LineInt("CMapStringToString.GetCount.after_overwrite", m.GetCount());

    m[L"four"] = L"quattro";
    LineInt("CMapStringToString.GetCount.after_subscript", m.GetCount());
    Line("CMapStringToString.subscript.reads_back", m[L"four"]);

    std::vector<std::string> assoc;
    POSITION pos = m.GetStartPosition();
    while (pos != nullptr)
    {
        CString key;
        CString val;
        m.GetNextAssoc(pos, key, val);
        assoc.push_back(Utf8(key) + "=" + Utf8(val));
    }
    Line("CMapStringToString.walk.sorted", SortedJoin(assoc));

    LineBool("CMapStringToString.RemoveKey.present", m.RemoveKey(L"one") != FALSE);
    LineInt("CMapStringToString.GetCount.after_remove", m.GetCount());
    m.RemoveAll();
    LineInt("CMapStringToString.GetCount.after_RemoveAll", m.GetCount());
    LineBool("CMapStringToString.IsEmpty.after_RemoveAll", m.IsEmpty() != FALSE);
}

// ---------------------------------------------------------------------
// CTypedPtrList<CPtrList, TYPE> / CTypedPtrArray<CPtrArray, TYPE>
// (afxtempl.h). Thin typed wrappers: every method forwards to the untyped
// base and casts. What is under test is that the forwarding preserves the
// base's ORDER and RETURN VALUES exactly -- so the elements are printed by
// the int they point at, never by address.
// ---------------------------------------------------------------------
static void TestCTypedPtrList()
{
    CTypedPtrList<CPtrList, int*> list;
    LineInt("CTypedPtrList.GetCount.empty", list.GetCount());
    LineBool("CTypedPtrList.IsEmpty.empty", list.IsEmpty() != FALSE);

    list.AddTail(&g_slots[1]);
    list.AddTail(&g_slots[2]);
    POSITION headPos = list.AddHead(&g_slots[0]);
    list.AddTail(&g_slots[3]);
    LineInt("CTypedPtrList.GetCount.after_4", list.GetCount());
    LineInt("CTypedPtrList.GetHead", *list.GetHead());
    LineInt("CTypedPtrList.GetTail", *list.GetTail());
    LineInt("CTypedPtrList.GetAt.head_position", *list.GetAt(headPos));

    std::vector<std::string> forward;
    for (POSITION pos = list.GetHeadPosition(); pos != nullptr;)
        forward.push_back(std::to_string(*list.GetNext(pos)));
    Line("CTypedPtrList.GetNext.forward", SortedJoin(forward)); // order-independent set
    std::string ordered;
    for (POSITION pos = list.GetHeadPosition(); pos != nullptr;)
    {
        if (!ordered.empty()) ordered += ",";
        ordered += std::to_string(*list.GetNext(pos));
    }
    Line("CTypedPtrList.GetNext.in_order", ordered);

    std::string reverse;
    for (POSITION pos = list.GetTailPosition(); pos != nullptr;)
    {
        if (!reverse.empty()) reverse += ",";
        reverse += std::to_string(*list.GetPrev(pos));
    }
    Line("CTypedPtrList.GetPrev.in_order", reverse);

    POSITION second = list.FindIndex(1);
    list.SetAt(second, &g_slots[5]);
    LineInt("CTypedPtrList.SetAt.reads_back", *list.GetAt(second));
    list.InsertBefore(second, &g_slots[4]);
    list.InsertAfter(second, &g_slots[4]);
    LineInt("CTypedPtrList.GetCount.after_inserts", list.GetCount());

    // Find comes from the untyped base on purpose (see the note in
    // afxtempl.h) -- so it takes a void*, in real MFC too.
    LineBool("CTypedPtrList.Find.present", list.Find((void*)&g_slots[5]) != nullptr);
    LineBool("CTypedPtrList.Find.absent", list.Find((void*)&g_slots[2]) != nullptr);

    LineInt("CTypedPtrList.RemoveHead", *list.RemoveHead());
    LineInt("CTypedPtrList.RemoveTail", *list.RemoveTail());
    LineInt("CTypedPtrList.GetCount.after_removes", list.GetCount());
    list.RemoveAll();
    LineInt("CTypedPtrList.GetCount.after_RemoveAll", list.GetCount());
}

static void TestCTypedPtrArray()
{
    CTypedPtrArray<CPtrArray, int*> arr;
    LineInt("CTypedPtrArray.GetSize.empty", arr.GetSize());

    // Add returns the index it stored at, so every iteration needs its own
    // case name -- compare.py matches on the name and rejects duplicates.
    for (int i = 0; i < 4; ++i)
    {
        char label[64];
        std::snprintf(label, sizeof(label), "CTypedPtrArray.Add.returns_index.%d", i);
        LineInt(label, arr.Add(&g_slots[i]));
    }
    LineInt("CTypedPtrArray.GetSize.after_4", arr.GetSize());
    LineInt("CTypedPtrArray.GetUpperBound", arr.GetUpperBound());
    LineInt("CTypedPtrArray.GetAt.0", *arr.GetAt(0));
    LineInt("CTypedPtrArray.operator_subscript.2", *arr[2]);

    arr.SetAt(1, &g_slots[5]);
    LineInt("CTypedPtrArray.SetAt.reads_back", *arr.GetAt(1));
    *arr.ElementAt(1) = 99;
    LineInt("CTypedPtrArray.ElementAt.is_writable", g_slots[5]);
    g_slots[5] = 15; // restore, later cases read this array too

    arr.InsertAt(0, &g_slots[4]);
    LineInt("CTypedPtrArray.GetSize.after_InsertAt", arr.GetSize());
    LineInt("CTypedPtrArray.InsertAt.new_head", *arr.GetAt(0));

    arr.SetAtGrow(7, &g_slots[3]);
    LineInt("CTypedPtrArray.GetSize.after_SetAtGrow", arr.GetSize());
    LineBool("CTypedPtrArray.SetAtGrow.fills_gap_with_null", arr.GetAt(6) == nullptr);
    LineInt("CTypedPtrArray.SetAtGrow.reads_back", *arr.GetAt(7));

    // void**, not int**: GetData comes from the untyped base on both sides
    // (real MFC's CTypedPtrArray does not redeclare it).
    void** data = arr.GetData();
    LineBool("CTypedPtrArray.GetData.matches_GetAt", data != nullptr && data[0] == arr.GetAt(0));

    std::string contents;
    for (INT_PTR i = 0; i < arr.GetSize(); ++i)
    {
        if (i) contents += ",";
        contents += arr.GetAt(i) ? std::to_string(*arr.GetAt(i)) : std::string("null");
    }
    Line("CTypedPtrArray.contents", contents);

    arr.RemoveAt(0);
    LineInt("CTypedPtrArray.GetSize.after_RemoveAt", arr.GetSize());
    arr.RemoveAll();
    LineInt("CTypedPtrArray.GetSize.after_RemoveAll", arr.GetSize());
}

// ---------------------------------------------------------------------
// CWinThread / AfxBeginThread (afxwin.h).
//
// Everything read off the CWinThread object is read BEFORE the thread is
// resumed: AfxBeginThread leaves m_bAutoDelete set, so the object frees
// itself the moment its procedure returns, and touching it after that is
// undefined on both sides alike. Completion is observed through an atomic
// the procedure writes, never through the thread object.
//
// The thread is created suspended for the same reason -- it makes the
// "before it runs" window deterministic instead of a race.
// ---------------------------------------------------------------------
namespace
{
std::atomic<int> g_workerRan{0};
std::atomic<int> g_workerParam{0};

UINT AFX_CDECL ConformanceWorker(LPVOID pParam)
{
    g_workerParam.store(pParam ? *static_cast<int*>(pParam) : -1);
    g_workerRan.store(1);
    return 7;
}

// Bounded wait. Nothing in this harness may block a CI runner for longer
// than it takes to notice something is wrong.
template <class Pred>
bool PollUntil(Pred pred, int timeoutMs = 5000)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    for (;;)
    {
        if (pred()) return true;
        if (std::chrono::steady_clock::now() >= deadline) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}
} // namespace

static void TestCWinThread()
{
    g_workerRan.store(0);
    g_workerParam.store(0);
    static int param = 4242;

    CWinThread* pThread = AfxBeginThread(ConformanceWorker, &param,
                                         THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
    LineBool("AfxBeginThread.returns_object", pThread != nullptr);
    if (pThread == nullptr)
        return;

    LineBool("CWinThread.m_bAutoDelete.default", pThread->m_bAutoDelete != FALSE);
    LineBool("CWinThread.m_nThreadID.nonzero", pThread->m_nThreadID != 0);
    LineBool("CWinThread.m_hThread.non_null", pThread->m_hThread != nullptr);
    LineBool("CWinThread.suspended.has_not_run_yet", g_workerRan.load() == 0);

    LineInt("CWinThread.GetThreadPriority.after_begin_normal", pThread->GetThreadPriority());
    LineBool("CWinThread.SetThreadPriority.highest", pThread->SetThreadPriority(THREAD_PRIORITY_HIGHEST) != FALSE);
    LineInt("CWinThread.GetThreadPriority.after_set_highest", pThread->GetThreadPriority());
    LineBool("CWinThread.SetThreadPriority.back_to_normal", pThread->SetThreadPriority(THREAD_PRIORITY_NORMAL) != FALSE);
    LineInt("CWinThread.GetThreadPriority.after_set_normal", pThread->GetThreadPriority());

    // Win32 returns the PREVIOUS suspend count; a thread created suspended
    // has one, so the first resume reports 1 and there is nothing left to
    // resume afterwards.
    LineInt("CWinThread.ResumeThread.from_suspended", static_cast<long long>(pThread->ResumeThread()));
    // pThread is unusable from here on (auto-delete).

    LineBool("CWinThread.worker.ran", PollUntil([] { return g_workerRan.load() != 0; }));
    LineInt("CWinThread.worker.received_param", g_workerParam.load());
}

// ---------------------------------------------------------------------
// CAsyncSocket (afxsock.h) -- the SYNCHRONOUS surface, over loopback.
//
// The async surface (AsyncSelect + OnReceive/OnSend/OnAccept/OnConnect/
// OnClose) is deliberately not compared: real MFC delivers those through
// WSAAsyncSelect and a hidden window, i.e. only to a thread that runs a
// message pump, and a console probe has none. That is a property of MFC's
// delivery mechanism, not a difference in socket behaviour.
//
// Every value printed here is platform-NEUTRAL by construction: no raw
// handle, no port number, and no errno/WSA error code (10035 on Windows,
// EWOULDBLOCK/EINPROGRESS elsewhere) is ever printed -- only whether an
// error was reported. Sockets created for async notification are
// non-blocking on both sides, so each transfer step polls to a deadline.
// ---------------------------------------------------------------------
static void TestCAsyncSocket()
{
#if defined(SIMPLE_MFC_USE_REAL_MFC)
    // Real MFC's CAsyncSocket reaches for the module's instance handle when
    // it creates the hidden notification window, and that handle is only
    // filled in by AfxWinInit -- which a console application has to call
    // itself. Done here rather than in main() so the rest of the run stays
    // in the exact state the other sections were written against.
    static bool inited = false;
    if (!inited)
    {
        AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0);
        inited = true;
    }
#endif
    LineBool("AfxSocketInit.returns_true", AfxSocketInit(nullptr) != FALSE);

    CAsyncSocket listener;
    LineBool("CAsyncSocket.Create.listener",
             listener.Create(0, SOCK_STREAM, FD_ACCEPT | FD_CLOSE, L"127.0.0.1") != FALSE);
    LineBool("CAsyncSocket.m_hSocket.valid_after_Create", listener.m_hSocket != INVALID_SOCKET);
    LineBool("CAsyncSocket.FromHandle.finds_owner", CAsyncSocket::FromHandle(listener.m_hSocket) == &listener);

    CString boundAddress;
    UINT boundPort = 0;
    LineBool("CAsyncSocket.GetSockName.returns_true", listener.GetSockName(boundAddress, boundPort) != FALSE);
    Line("CAsyncSocket.GetSockName.address", boundAddress);
    LineBool("CAsyncSocket.GetSockName.port_is_assigned", boundPort != 0);

    LineBool("CAsyncSocket.Listen", listener.Listen(5) != FALSE);

    // Nothing has connected yet: a non-blocking accept must fail rather
    // than wait, and must report an error for it.
    CAsyncSocket tooEarly;
    LineBool("CAsyncSocket.Accept.with_no_pending_connection", listener.Accept(tooEarly) != FALSE);
    LineBool("CAsyncSocket.GetLastError.reports_the_would_block", CAsyncSocket::GetLastError() != 0);

    CAsyncSocket client;
    LineBool("CAsyncSocket.Create.client",
             client.Create(0, SOCK_STREAM, FD_CONNECT | FD_READ | FD_WRITE | FD_CLOSE) != FALSE);
    // A non-blocking connect() reports "in progress" rather than success,
    // so its return value is not the interesting part -- the accept is.
    client.Connect(L"127.0.0.1", boundPort);

    CAsyncSocket server;
    LineBool("CAsyncSocket.Accept.after_a_connect",
             PollUntil([&] { return listener.Accept(server) != FALSE; }));

    CString peerAddress;
    UINT peerPort = 0;
    LineBool("CAsyncSocket.GetPeerName.returns_true", server.GetPeerName(peerAddress, peerPort) != FALSE);
    Line("CAsyncSocket.GetPeerName.address", peerAddress);
    LineBool("CAsyncSocket.GetPeerName.port_is_assigned", peerPort != 0);

    static const char kPayload[] = "conformance-payload";
    const int kPayloadLen = static_cast<int>(sizeof(kPayload) - 1);
    int sent = 0;
    LineBool("CAsyncSocket.Send.completes",
             PollUntil([&] { sent = client.Send(kPayload, kPayloadLen); return sent > 0; }));
    LineInt("CAsyncSocket.Send.bytes", sent);

    char received[64] = {};
    int got = 0;
    LineBool("CAsyncSocket.Receive.completes",
             PollUntil([&] { got = server.Receive(received, static_cast<int>(sizeof(received) - 1)); return got > 0; }));
    LineInt("CAsyncSocket.Receive.bytes", got);
    Line("CAsyncSocket.Receive.payload", std::string(received, static_cast<size_t>(got > 0 ? got : 0)));

    // Options round-trip. SO_REUSEADDR's numeric value differs per
    // platform, so only the round-trip itself is compared.
    int reuse = 1;
    LineBool("CAsyncSocket.SetSockOpt.SO_REUSEADDR",
             server.SetSockOpt(SO_REUSEADDR, &reuse, static_cast<int>(sizeof(reuse))) != FALSE);
    int readBack = 0;
    int readBackLen = static_cast<int>(sizeof(readBack));
    LineBool("CAsyncSocket.GetSockOpt.SO_REUSEADDR", server.GetSockOpt(SO_REUSEADDR, &readBack, &readBackLen) != FALSE);
    LineBool("CAsyncSocket.GetSockOpt.SO_REUSEADDR.reads_back_set", readBack != 0);

    DWORD pending = 0;
    LineBool("CAsyncSocket.IOCtl.FIONREAD", server.IOCtl(FIONREAD, &pending) != FALSE);

    LineBool("CAsyncSocket.ShutDown.sends", client.ShutDown(CAsyncSocket::sends) != FALSE);

    // Detach hands the handle back without closing it, and unregisters the
    // object -- so FromHandle must stop finding it.
    SOCKET detached = server.Detach();
    LineBool("CAsyncSocket.Detach.returns_the_handle", detached != INVALID_SOCKET);
    LineBool("CAsyncSocket.Detach.clears_m_hSocket", server.m_hSocket == INVALID_SOCKET);
    LineBool("CAsyncSocket.FromHandle.after_Detach", CAsyncSocket::FromHandle(detached) == nullptr);

    CAsyncSocket adopted;
    LineBool("CAsyncSocket.Attach", adopted.Attach(detached) != FALSE);
    LineBool("CAsyncSocket.FromHandle.after_Attach", CAsyncSocket::FromHandle(detached) == &adopted);
    adopted.Close();
    LineBool("CAsyncSocket.Close.clears_m_hSocket", adopted.m_hSocket == INVALID_SOCKET);
    adopted.Close();
    LineBool("CAsyncSocket.Close.is_idempotent", adopted.m_hSocket == INVALID_SOCKET);

    client.Close();
    listener.Close();
}

// =====================================================================
// FULL-COVERAGE SECTIONS
//
// Everything below was added to close the gap between "the classes eMule
// leans on are compared" and "every method that CAN be compared IS".
// Each section drives its subject from a table of varying inputs and
// prints one record per input, so a difference shows up as a named case
// rather than as a single pass/fail.
//
// The methods still not compared, and why they cannot be:
//
//   * CComPtr / CComQIPtr / CComPtrBase / CComBSTR / CComVariant
//     (atlcomcli.h), CRegKey (atlbase.h), COleDateTime (atlcomtime.h):
//     declaration-only on this branch, not one member is defined. A case
//     calling any of them would not link, let alone compare.
//   * CObject::AssertValid / CObject::Dump and the whole of CDumpContext:
//     real MFC declares them under #ifdef _DEBUG only. They do not exist
//     as members in the Release configuration this suite builds.
//   * CException::ReportError: on real MFC it opens a Win32 MessageBox,
//     which would hang a non-interactive runner forever.
//   * CWinThread::Run: real MFC's implementation IS the message pump --
//     it returns only on WM_QUIT, so calling it deadlocks the probe.
//   * CAsyncSocket's OnReceive/OnSend/OnAccept/OnConnect/OnClose/
//     OnOutOfBandData as *notifications*: real MFC delivers them through
//     WSAAsyncSelect and a hidden window, i.e. only to a thread running a
//     message pump. Their default implementations are still called
//     directly and compared below -- that part is comparable.
//   * CString::c_str / CString::AsStdString: simple_mfc's own additions.
//     Real MFC has no such members, so there is nothing to compare
//     against; emitting a native-only case would be reported as EXTRA.
// =====================================================================

// ---------------------------------------------------------------------
// CRuntimeClass: IsDerivedFrom / CreateObject, plus DYNAMIC_DOWNCAST
// (which is AfxDynamicDownCast behind a macro, the only spelling eMule
// uses).
// ---------------------------------------------------------------------
namespace
{
class DynBase : public CObject
{
    DECLARE_DYNAMIC(DynBase)
public:
    int tag = 1;
};
IMPLEMENT_DYNAMIC(DynBase, CObject)

class DynMade : public DynBase
{
    DECLARE_DYNCREATE(DynMade)
public:
    DynMade() { tag = 2; }
};
IMPLEMENT_DYNCREATE(DynMade, DynBase)
} // namespace

static void TestCRuntimeClass()
{
    // IsDerivedFrom over the full matrix of the four classes in play, so
    // both the TRUE and the FALSE half of the relation is compared.
    struct Pair { const char* label; CRuntimeClass* derived; CRuntimeClass* base; };
    const Pair pairs[] = {
        {"made_from_base",    RUNTIME_CLASS(DynMade), RUNTIME_CLASS(DynBase)},
        {"made_from_object",  RUNTIME_CLASS(DynMade), RUNTIME_CLASS(CObject)},
        {"made_from_made",    RUNTIME_CLASS(DynMade), RUNTIME_CLASS(DynMade)},
        {"base_from_made",    RUNTIME_CLASS(DynBase), RUNTIME_CLASS(DynMade)},
        {"base_from_object",  RUNTIME_CLASS(DynBase), RUNTIME_CLASS(CObject)},
        {"object_from_base",  RUNTIME_CLASS(CObject), RUNTIME_CLASS(DynBase)},
        {"fileex_from_except", RUNTIME_CLASS(CFileException), RUNTIME_CLASS(CException)},
        {"except_from_fileex", RUNTIME_CLASS(CException), RUNTIME_CLASS(CFileException)},
    };
    for (const Pair& p : pairs)
    {
        std::string name = std::string("CRuntimeClass.IsDerivedFrom.") + p.label;
        LineBool(name.c_str(), p.derived->IsDerivedFrom(p.base) != FALSE);
    }

    // CreateObject: DECLARE_DYNCREATE makes a class creatable by name,
    // DECLARE_DYNAMIC alone does not -- and the difference is observable.
    CObject* made = RUNTIME_CLASS(DynMade)->CreateObject();
    LineBool("CRuntimeClass.CreateObject.dyncreate_returns_object", made != nullptr);
    LineBool("CRuntimeClass.CreateObject.result_is_the_class",
             made != nullptr && made->IsKindOf(RUNTIME_CLASS(DynMade)) != FALSE);
    LineInt("CRuntimeClass.CreateObject.constructor_ran",
            made != nullptr ? static_cast<DynMade*>(made)->tag : -1);
    delete made;

    CObject* notCreatable = RUNTIME_CLASS(DynBase)->CreateObject();
    LineBool("CRuntimeClass.CreateObject.dynamic_only_returns_null", notCreatable == nullptr);
    delete notCreatable;

    // DYNAMIC_DOWNCAST -> AfxDynamicDownCast. Succeeds down the chain,
    // returns null across it.
    DynMade concrete;
    CObject* asObject = &concrete;
    LineBool("AfxDynamicDownCast.to_own_class", DYNAMIC_DOWNCAST(DynMade, asObject) != nullptr);
    LineBool("AfxDynamicDownCast.to_base_class", DYNAMIC_DOWNCAST(DynBase, asObject) != nullptr);
    LineBool("AfxDynamicDownCast.to_unrelated_class",
             DYNAMIC_DOWNCAST(CFileException, asObject) != nullptr);
    LineBool("AfxDynamicDownCast.null_input", DYNAMIC_DOWNCAST(DynMade, (CObject*)nullptr) != nullptr);
}

// ---------------------------------------------------------------------
// The exception types the earlier sections did not reach, and the
// buffer contract of GetErrorMessage.
//
// The message TEXT is not compared, for the reason TestExceptions()
// already documents (real MFC builds it from its own string resources,
// which need a CWinApp this console harness deliberately does not have).
// What IS compared is the part that is a documented contract regardless
// of the text: GetErrorMessage must never write past nMaxError.
// ---------------------------------------------------------------------
static void TestExceptionGaps()
{
    CNotSupportedException nse;
    LineBool("CNotSupportedException.IsKindOf.CSimpleException",
             nse.IsKindOf(RUNTIME_CLASS(CSimpleException)) != FALSE);
    LineBool("CNotSupportedException.IsKindOf.CException",
             nse.IsKindOf(RUNTIME_CLASS(CException)) != FALSE);
    LineBool("CNotSupportedException.IsKindOf.CMemoryException",
             nse.IsKindOf(RUNTIME_CLASS(CMemoryException)) != FALSE);

    CArchiveException ae(CArchiveException::badIndex, L"stream.dat");
    LineInt("CArchiveException.m_cause", ae.m_cause);
    LineBool("CArchiveException.IsKindOf.CException",
             ae.IsKindOf(RUNTIME_CLASS(CException)) != FALSE);
    LineBool("CArchiveException.IsKindOf.CSimpleException",
             ae.IsKindOf(RUNTIME_CLASS(CSimpleException)) != FALSE);

    // The buffer contract, over a range of buffer sizes including the
    // degenerate ones. A sentinel is written one past the limit each
    // time: whatever text lands in the buffer, that sentinel must survive.
    {
        const UINT sizes[] = {1, 2, 8, 64, 255};
        for (UINT n : sizes)
        {
            wchar_t buf[300];
            for (wchar_t& c : buf) c = L'#';
            CFileException fe(CFileException::fileNotFound, ERROR_FILE_NOT_FOUND, L"nope.dat");
            fe.GetErrorMessage(buf, n);
            std::string name = "CFileException.GetErrorMessage.respects_nMaxError." + std::to_string(n);
            LineBool(name.c_str(), buf[n] == L'#');
            std::string term = "CFileException.GetErrorMessage.nul_terminates." + std::to_string(n);
            bool terminated = false;
            for (UINT i = 0; i < n; ++i)
                if (buf[i] == L'\0') { terminated = true; break; }
            LineBool(term.c_str(), terminated);
        }
    }

    // ThrowOsError maps an OS error code onto a CFileException::Cause.
    // The mapping is the behaviour under test, so drive it with several
    // codes rather than one.
    {
        struct Case { const char* label; LONG osError; };
        const Case cases[] = {
            {"file_not_found", ERROR_FILE_NOT_FOUND},
            {"bad_pathname",   ERROR_BAD_PATHNAME},
            {"disk_full",      ERROR_DISK_FULL},
            {"zero",           0},
        };
        for (const Case& c : cases)
        {
            try
            {
                CFileException::ThrowOsError(c.osError, L"probe.dat");
                Line((std::string("CFileException.ThrowOsError.") + c.label).c_str(),
                     std::string("NEVER (did not throw)"));
            }
            catch (CFileException* e)
            {
                LineInt((std::string("CFileException.ThrowOsError.") + c.label + ".m_cause").c_str(),
                        e->m_cause);
                LineInt((std::string("CFileException.ThrowOsError.") + c.label + ".m_lOsError").c_str(),
                        e->m_lOsError);
                Line((std::string("CFileException.ThrowOsError.") + c.label + ".m_strFileName").c_str(),
                     e->m_strFileName);
                e->Delete();
            }
        }
    }
}

// ---------------------------------------------------------------------
// CString: the members no earlier section reached.
// ---------------------------------------------------------------------
namespace
{
// FormatV/AppendFormatV take a va_list, which only a variadic function
// can produce -- these are the harness's way of building one.
void CallFormatV(CString& s, LPCTSTR fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    s.FormatV(fmt, args);
    va_end(args);
}
void CallAppendFormatV(CString& s, LPCTSTR fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    s.AppendFormatV(fmt, args);
    va_end(args);
}
} // namespace

static void TestCStringGaps()
{
    // --- Collate / CollateNoCase ----------------------------------------
    // Only the SIGN is compared: Collate goes through the CRT's locale
    // collation, whose magnitudes are implementation-defined.
    {
        struct Case { const char* label; LPCTSTR a; LPCTSTR b; };
        const Case cases[] = {
            {"equal",        L"alpha",  L"alpha"},
            {"less",         L"alpha",  L"beta"},
            {"greater",      L"beta",   L"alpha"},
            {"prefix",       L"alph",   L"alpha"},
            {"case_differs", L"Alpha",  L"alpha"},
            {"empty_vs_x",   L"",       L"x"},
            {"both_empty",   L"",       L""},
            {"digits",       L"file10", L"file9"},
        };
        for (const Case& c : cases)
        {
            CString s(c.a);
            LineInt((std::string("CString.Collate.") + c.label).c_str(), Sign(s.Collate(c.b)));
            LineInt((std::string("CString.CollateNoCase.") + c.label).c_str(),
                    Sign(s.CollateNoCase(c.b)));
        }
    }

    // --- FindOneOf -------------------------------------------------------
    {
        struct Case { const char* label; LPCTSTR s; LPCTSTR set; };
        const Case cases[] = {
            {"first_char",   L"abcdef",      L"a"},
            {"middle",       L"abcdef",      L"dc"},
            {"none",         L"abcdef",      L"xyz"},
            {"empty_set",    L"abcdef",      L""},
            {"empty_string", L"",            L"abc"},
            {"separators",   L"host:8080/p", L"/:"},
            {"non_ascii",    L"café au lait", L"é"},
        };
        for (const Case& c : cases)
        {
            CString s(c.s);
            LineInt((std::string("CString.FindOneOf.") + c.label).c_str(), s.FindOneOf(c.set));
        }
    }

    // --- Truncate --------------------------------------------------------
    // Real ATL asserts on a length above the current one, so every case
    // here stays within bounds -- what is compared is the resulting
    // string AND the resulting length, which must agree.
    {
        const int lengths[] = {0, 1, 3, 6};
        for (int n : lengths)
        {
            CString s(L"abcdef");
            s.Truncate(n);
            Line((std::string("CString.Truncate.") + std::to_string(n)).c_str(), s);
            LineInt((std::string("CString.Truncate.") + std::to_string(n) + ".GetLength").c_str(),
                    s.GetLength());
        }
    }

    // --- SetString (both overloads) --------------------------------------
    {
        struct Case { const char* label; LPCTSTR src; };
        const Case cases[] = {
            {"plain",     L"replacement"},
            {"empty",     L""},
            {"embedded",  L"a\tb"},
            {"non_ascii", L"äöü"},
        };
        for (const Case& c : cases)
        {
            CString s(L"original value");
            s.SetString(c.src);
            Line((std::string("CString.SetString.psz.") + c.label).c_str(), s);
            LineInt((std::string("CString.SetString.psz.") + c.label + ".GetLength").c_str(),
                    s.GetLength());
        }
        const int counts[] = {0, 1, 4, 11};
        for (int n : counts)
        {
            CString s(L"original value");
            s.SetString(L"replacement", n);
            Line((std::string("CString.SetString.psz_n.") + std::to_string(n)).c_str(), s);
            LineInt((std::string("CString.SetString.psz_n.") + std::to_string(n) + ".GetLength").c_str(),
                    s.GetLength());
        }
    }

    // --- GetString -------------------------------------------------------
    {
        const wchar_t* inputs[] = {L"", L"x", L"a longer value with spaces", L"éè"};
        int i = 0;
        for (const wchar_t* in : inputs)
        {
            CString s(in);
            LPCTSTR p = s.GetString();
            Line((std::string("CString.GetString.") + std::to_string(i)).c_str(), p);
            LineBool((std::string("CString.GetString.") + std::to_string(i) + ".nul_terminated").c_str(),
                     p[s.GetLength()] == L'\0');
            ++i;
        }
    }

    // --- AppendChar ------------------------------------------------------
    {
        CString s;
        const wchar_t chars[] = {L'a', L'B', L'0', L' ', L'é'};
        for (wchar_t c : chars) s.AppendChar(c);
        Line("CString.AppendChar.accumulated", s);
        LineInt("CString.AppendChar.GetLength", s.GetLength());
    }

    // --- FormatV / AppendFormatV -----------------------------------------
    {
        CString s;
        CallFormatV(s, L"%d/%s/%c", 42, L"mid", L'z');
        Line("CString.FormatV.mixed", s);
        CallAppendFormatV(s, L" + %u", 7u);
        Line("CString.AppendFormatV.appended", s);

        CString empty;
        CallFormatV(empty, L"%s", L"");
        LineInt("CString.FormatV.empty_result_length", empty.GetLength());

        CString wide;
        CallFormatV(wide, L"%08.3f|%X", 3.14159, 48879u);
        Line("CString.FormatV.numeric_flags", wide);
    }

    // --- ReleaseBufferSetLength -------------------------------------------
    // The point of the SetLength form is that it takes the length from the
    // caller rather than scanning for a NUL, so it keeps text that has one
    // embedded -- which is exactly what is compared here.
    {
        const int lengths[] = {0, 1, 5};
        for (int n : lengths)
        {
            CString s;
            LPTSTR buf = s.GetBuffer(16);
            for (int i = 0; i < 8; ++i) buf[i] = static_cast<wchar_t>(L'a' + i);
            buf[3] = L'\0'; // a NUL a plain ReleaseBuffer would stop at
            s.ReleaseBufferSetLength(n);
            LineInt((std::string("CString.ReleaseBufferSetLength.") + std::to_string(n)).c_str(),
                    s.GetLength());
        }
    }

#ifdef _WIN32
    // --- LoadString (Windows only, as in real MFC) ------------------------
    // A console probe carries no string table, so every id misses. That
    // outcome is still a comparable contract: FALSE, and the string left
    // empty rather than untouched.
    {
        const UINT ids[] = {1u, 100u, 61472u};
        for (UINT id : ids)
        {
            CString s(L"previous content");
            BOOL ok = s.LoadString(id);
            LineBool(("CString.LoadString.id." + std::to_string(id)).c_str(), ok != FALSE);
            LineBool(("CString.LoadString.id." + std::to_string(id) + ".empties_on_miss").c_str(),
                     s.IsEmpty() != FALSE);

            CString h(L"previous content");
            BOOL okh = h.LoadString(::GetModuleHandleW(nullptr), id);
            LineBool(("CString.LoadString.hinstance." + std::to_string(id)).c_str(), okh != FALSE);

            CString l(L"previous content");
            BOOL okl = l.LoadString(::GetModuleHandleW(nullptr), id, 0x0409); // en-US
            LineBool(("CString.LoadString.langid." + std::to_string(id)).c_str(), okl != FALSE);
        }
    }

    // --- AllocSysString ---------------------------------------------------
    {
        const wchar_t* inputs[] = {L"", L"x", L"a BSTR value", L"é中"};
        int i = 0;
        for (const wchar_t* in : inputs)
        {
            CString s(in);
            BSTR b = s.AllocSysString();
            LineBool(("CString.AllocSysString." + std::to_string(i) + ".non_null").c_str(), b != nullptr);
            LineInt(("CString.AllocSysString." + std::to_string(i) + ".SysStringLen").c_str(),
                    b ? static_cast<long long>(::SysStringLen(b)) : -1);
            Line(("CString.AllocSysString." + std::to_string(i) + ".content").c_str(),
                 b ? b : L"(null)");
            if (b) ::SysFreeString(b);
            ++i;
        }
    }
#endif
}

// ---------------------------------------------------------------------
// CFileFind: the attribute predicates and the three timestamps.
// ---------------------------------------------------------------------
static void TestCFileFindAttributes()
{
    CString dir = TempDir() + CString(L"simple_mfc_conformance_attr" SMFC_SEP);
    CreateDirectoryW(dir, nullptr);

    CString plain = dir + CString(L"plain.bin");
    {
        CFile f;
        f.Open(plain, CFile::modeCreate | CFile::modeWrite);
        const char payload[] = "0123456789";
        f.Write(payload, sizeof(payload) - 1);
        SafeClose(f);
    }

    // Every predicate on an ordinary, freshly written file. All of them
    // are FALSE on both platforms except IsArchived, which Windows sets on
    // any newly written file and POSIX has no notion of -- that one case
    // is listed in platform_dependent.txt.
    {
        CFileFind finder;
        BOOL found = finder.FindFile(plain);
        BOOL more = SIMPLE_MFC_FIND_NEXT_FILE(finder);
        (void)more;
        LineBool("CFileFind.Attr.found", found != FALSE);
        LineBool("CFileFind.Attr.IsHidden", finder.IsHidden() != FALSE);
        LineBool("CFileFind.Attr.IsSystem", finder.IsSystem() != FALSE);
        LineBool("CFileFind.Attr.IsReadOnly", finder.IsReadOnly() != FALSE);
        LineBool("CFileFind.Attr.IsCompressed", finder.IsCompressed() != FALSE);
        LineBool("CFileFind.Attr.IsTemporary", finder.IsTemporary() != FALSE);
        LineBool("CFileFind.Attr.IsArchived", finder.IsArchived() != FALSE);
        LineBool("CFileFind.Attr.IsDirectory", finder.IsDirectory() != FALSE);

        // The three timestamps, in both the CTime and the FILETIME form.
        // The instants themselves are non-deterministic, so what is
        // compared is what IS deterministic: that the call succeeds, that
        // the two overloads agree with each other, and that the value
        // lands in a sane range rather than at the epoch.
        CTime writeTime;
        LineBool("CFileFind.GetLastWriteTime.CTime.returns_true",
                 finder.GetLastWriteTime(writeTime) != FALSE);
        LineBool("CFileFind.GetLastWriteTime.plausible_year",
                 writeTime.GetYear() >= 2020 && writeTime.GetYear() < 2100);

        CTime createTime;
        LineBool("CFileFind.GetCreationTime.CTime.returns_true",
                 finder.GetCreationTime(createTime) != FALSE);
        LineBool("CFileFind.GetCreationTime.plausible_year",
                 createTime.GetYear() >= 2020 && createTime.GetYear() < 2100);

        CTime accessTime;
        LineBool("CFileFind.GetLastAccessTime.CTime.returns_true",
                 finder.GetLastAccessTime(accessTime) != FALSE);
        LineBool("CFileFind.GetLastAccessTime.plausible_year",
                 accessTime.GetYear() >= 2020 && accessTime.GetYear() < 2100);

        // The raw-FILETIME forms only exist where FILETIME does. Off
        // Windows the type is an incomplete declaration and these three
        // methods are documented no-ops, so a case here would compare the
        // platform, not the library. The Windows side-by-side comparison
        // -- the authoritative one -- still covers them in full; on POSIX
        // compare.py simply reports them as present in the recording only.
#ifdef _WIN32
        FILETIME writeFt{}, createFt{}, accessFt{};
        LineBool("CFileFind.GetLastWriteTime.FILETIME.returns_true",
                 finder.GetLastWriteTime(&writeFt) != FALSE);
        LineBool("CFileFind.GetCreationTime.FILETIME.returns_true",
                 finder.GetCreationTime(&createFt) != FALSE);
        LineBool("CFileFind.GetLastAccessTime.FILETIME.returns_true",
                 finder.GetLastAccessTime(&accessFt) != FALSE);
        // The two spellings of the same instant must agree: a FILETIME is
        // 100 ns ticks since 1601, a CTime is seconds since 1970.
        auto toUnix = [](const FILETIME& ft) {
            unsigned long long ticks =
                (static_cast<unsigned long long>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
            return static_cast<long long>(ticks / 10000000ULL) - 11644473600LL;
        };
        LineBool("CFileFind.GetLastWriteTime.overloads_agree",
                 toUnix(writeFt) == static_cast<long long>(writeTime.GetTime()));
        LineBool("CFileFind.GetCreationTime.overloads_agree",
                 toUnix(createFt) == static_cast<long long>(createTime.GetTime()));
        LineBool("CFileFind.GetLastAccessTime.overloads_agree",
                 toUnix(accessFt) == static_cast<long long>(accessTime.GetTime()));
#endif

        // Written last, so the write time cannot precede the creation time
        // by more than the clock's own granularity.
        LineBool("CFileFind.times.write_not_before_creation",
                 writeTime.GetTime() + 2 >= createTime.GetTime());

        finder.Close();
    }

    // A read-only file, so IsReadOnly has a TRUE case too. _wchmod is the
    // one spelling that means the same thing on both platforms: it sets
    // FILE_ATTRIBUTE_READONLY on Windows and clears the write bits on
    // POSIX, and CFileFind::IsReadOnly reads whichever of the two the
    // platform keeps.
    {
        CString ro = dir + CString(L"readonly.bin");
        {
            CFile f;
            f.Open(ro, CFile::modeCreate | CFile::modeWrite);
            const char payload[] = "ro";
            f.Write(payload, sizeof(payload) - 1);
            SafeClose(f);
        }
        MakeReadOnly(ro);

        CFileFind finder;
        BOOL found = finder.FindFile(ro);
        BOOL more = SIMPLE_MFC_FIND_NEXT_FILE(finder);
        (void)more;
        LineBool("CFileFind.ReadOnly.found", found != FALSE);
        LineBool("CFileFind.ReadOnly.IsReadOnly", finder.IsReadOnly() != FALSE);
        LineBool("CFileFind.ReadOnly.IsHidden", finder.IsHidden() != FALSE);
        finder.Close();

        MakeWritable(ro);
        SafeRemoveFile(ro);
    }

    // FindNextFile's own return value on a directory with a known number
    // of matches: TRUE while another entry follows, FALSE on the last one.
    {
        const wchar_t* extra[] = {L"one.seq", L"two.seq", L"three.seq"};
        for (const wchar_t* n : extra)
        {
            CFile f;
            f.Open(dir + CString(n), CFile::modeCreate | CFile::modeWrite);
            SafeClose(f);
        }
        CFileFind finder;
        BOOL working = finder.FindFile(dir + CString(L"*.seq"));
        LineBool("CFileFind.FindFile.wildcard_found", working != FALSE);
        int seen = 0;
        int lastReturn = -1;
        while (working)
        {
            working = SIMPLE_MFC_FIND_NEXT_FILE(finder);
            lastReturn = working ? 1 : 0;
            ++seen;
            if (seen > 16) break; // never loop unboundedly in CI
        }
        LineInt("CFileFind.FindNextFile.iterations", seen);
        LineInt("CFileFind.FindNextFile.last_return", lastReturn);
        finder.Close();
        for (const wchar_t* n : extra) SafeRemoveFile(dir + CString(n));
    }

    SafeRemoveFile(plain);
    RemoveDirectoryW(dir);
}

// ---------------------------------------------------------------------
// CSyncObject: the abstract base's own surface, reached through a base
// pointer so it is the base's declarations that are exercised.
// ---------------------------------------------------------------------
static void TestCSyncObjectBase()
{
    CEvent manualEvent(TRUE, TRUE);
    CMutex mutex(FALSE);
    CCriticalSection section;

    struct Subject { const char* label; CSyncObject* obj; };
    const Subject subjects[] = {
        {"CEvent", &manualEvent},
        {"CMutex", &mutex},
        {"CCriticalSection", &section},
    };
    for (const Subject& s : subjects)
    {
        std::string base = std::string("CSyncObject.") + s.label;
        LineBool((base + ".Lock.via_base").c_str(), s.obj->Lock(2000) != FALSE);
        LineBool((base + ".Unlock.via_base").c_str(), s.obj->Unlock() != FALSE);
        LineBool((base + ".operator_HANDLE.non_null").c_str(),
                 static_cast<HANDLE>(*s.obj) != nullptr);
    }
}

// ---------------------------------------------------------------------
// CWinThread: the parts of the lifecycle AfxBeginThread does not reach.
// ---------------------------------------------------------------------
namespace
{
std::atomic<int> g_lifecycleRan{0};

UINT AFX_CDECL LifecycleWorker(LPVOID pParam)
{
    g_lifecycleRan.store(pParam ? *static_cast<int*>(pParam) : -1);
    return 3;
}
} // namespace

static void TestCWinThreadLifecycle()
{
    g_lifecycleRan.store(0);
    static int marker = 99;

    // The constructor + CreateThread path, which is what AfxBeginThread
    // does internally and what code that wants the object first uses.
    CWinThread* pThread = new CWinThread(LifecycleWorker, &marker);
    pThread->m_bAutoDelete = FALSE; // this test owns it, so it can outlive the run
    LineBool("CWinThread.CreateThread.suspended",
             pThread->CreateThread(CREATE_SUSPENDED) != FALSE);
    LineBool("CWinThread.CreateThread.sets_m_hThread", pThread->m_hThread != nullptr);
    LineBool("CWinThread.CreateThread.has_not_run_yet", g_lifecycleRan.load() == 0);

    // Suspend counts nest. Created suspended the count is 1; suspending
    // again makes it 2, and each call reports the count BEFORE it acted.
    LineInt("CWinThread.SuspendThread.from_one",
            static_cast<long long>(pThread->SuspendThread()));
    LineInt("CWinThread.ResumeThread.from_two",
            static_cast<long long>(pThread->ResumeThread()));
    LineInt("CWinThread.ResumeThread.from_one",
            static_cast<long long>(pThread->ResumeThread()));

    LineBool("CWinThread.CreateThread.worker_ran",
             PollUntil([] { return g_lifecycleRan.load() != 0; }));
    LineInt("CWinThread.CreateThread.worker_param", g_lifecycleRan.load());
    delete pThread;

    // The three virtuals a worker thread never calls, on an object that
    // was never started -- the only state in which calling them is safe.
    // CWinThread::Run is deliberately absent: its real MFC implementation
    // IS the message pump and would never return.
    {
        CWinThread idle;
        idle.m_bAutoDelete = FALSE;
        LineBool("CWinThread.InitInstance.default", idle.InitInstance() != FALSE);
        LineInt("CWinThread.ExitInstance.default", idle.ExitInstance());
        idle.Delete(); // m_bAutoDelete is FALSE, so this must NOT free it
        LineBool("CWinThread.Delete.without_autodelete_is_a_noop",
                 idle.m_bAutoDelete == FALSE);
    }
}

// ---------------------------------------------------------------------
// CAsyncSocket: the datagram surface (Bind / SendTo / both ReceiveFrom
// overloads), AsyncSelect, and the default notification handlers.
// ---------------------------------------------------------------------
namespace
{
// The On* notifications are protected on CAsyncSocket -- in real MFC and,
// since the conformance suite found the difference, here too. A derived
// class is the only place they can be called from, which is exactly how a
// consumer reaches them.
class NotificationProbe : public CAsyncSocket
{
public:
    void CallOnReceive(int e) { OnReceive(e); }
    void CallOnSend(int e) { OnSend(e); }
    void CallOnAccept(int e) { OnAccept(e); }
    void CallOnConnect(int e) { OnConnect(e); }
    void CallOnClose(int e) { OnClose(e); }
    void CallOnOutOfBandData(int e) { OnOutOfBandData(e); }
};
} // namespace

static void TestCAsyncSocketDatagram()
{
    LineBool("AfxSocketInit.before_datagram", AfxSocketInit(nullptr) != FALSE);

    // Bind wants a socket that Create() has not already bound, so the
    // handle comes from the platform API and is adopted with Attach --
    // harness scaffolding, not part of what is compared.
    SOCKET raw = ::socket(AF_INET, SOCK_DGRAM, 0);
    LineBool("CAsyncSocket.Bind.raw_socket_available", raw != INVALID_SOCKET);

    CAsyncSocket receiver;
    LineBool("CAsyncSocket.Bind.Attach", receiver.Attach(raw) != FALSE);
    LineBool("CAsyncSocket.Bind.to_loopback_ephemeral",
             receiver.Bind(0, L"127.0.0.1") != FALSE);

    CString boundAddress;
    UINT boundPort = 0;
    LineBool("CAsyncSocket.Bind.GetSockName_after_Bind",
             receiver.GetSockName(boundAddress, boundPort) != FALSE);
    Line("CAsyncSocket.Bind.address", boundAddress);
    LineBool("CAsyncSocket.Bind.port_is_assigned", boundPort != 0);

    // Binding a second time must fail: the socket already has a name.
    LineBool("CAsyncSocket.Bind.second_bind_fails", receiver.Bind(0, L"127.0.0.1") != FALSE);

    CAsyncSocket sender;
    LineBool("CAsyncSocket.SendTo.sender_created",
             sender.Create(0, SOCK_DGRAM, FD_READ | FD_WRITE, L"127.0.0.1") != FALSE);

    // Several payload sizes, so the return value is compared against a
    // varying expectation rather than one.
    const char* payloads[] = {"a", "datagram", "0123456789012345678901234567890123456789"};
    int idx = 0;
    for (const char* payload : payloads)
    {
        const int len = static_cast<int>(std::strlen(payload));
        const int sent = sender.SendTo(payload, len, boundPort, L"127.0.0.1");
        LineInt(("CAsyncSocket.SendTo." + std::to_string(idx) + ".returns_length").c_str(), sent);

        char buf[128]{};
        // Non-blocking on both sides: poll to a deadline rather than wait.
        int got = -1;
        if (idx == 0)
        {
            // The CString/UINT overload.
            CString fromAddress;
            UINT fromPort = 0;
            PollUntil([&] {
                got = receiver.ReceiveFrom(buf, static_cast<int>(sizeof(buf)), fromAddress, fromPort);
                return got > 0;
            });
            LineInt("CAsyncSocket.ReceiveFrom.CString.returns_length", got);
            Line("CAsyncSocket.ReceiveFrom.CString.payload",
                 std::string(buf, got > 0 ? static_cast<size_t>(got) : 0u));
            Line("CAsyncSocket.ReceiveFrom.CString.from_address", fromAddress);
            LineBool("CAsyncSocket.ReceiveFrom.CString.from_port_assigned", fromPort != 0);
        }
        else
        {
            // The raw SOCKADDR overload.
            sockaddr_in from{};
            int fromLen = static_cast<int>(sizeof(from));
            PollUntil([&] {
                got = receiver.ReceiveFrom(buf, static_cast<int>(sizeof(buf)),
                                           reinterpret_cast<SOCKADDR*>(&from), &fromLen);
                return got > 0;
            });
            LineInt(("CAsyncSocket.ReceiveFrom.SOCKADDR." + std::to_string(idx) + ".returns_length").c_str(),
                    got);
            Line(("CAsyncSocket.ReceiveFrom.SOCKADDR." + std::to_string(idx) + ".payload").c_str(),
                 std::string(buf, got > 0 ? static_cast<size_t>(got) : 0u));
            LineBool(("CAsyncSocket.ReceiveFrom.SOCKADDR." + std::to_string(idx) + ".family_is_inet").c_str(),
                     from.sin_family == AF_INET);
            LineBool(("CAsyncSocket.ReceiveFrom.SOCKADDR." + std::to_string(idx) + ".length_written").c_str(),
                     fromLen >= static_cast<int>(sizeof(sockaddr_in)));
        }
        ++idx;
    }

    // AsyncSelect: what is comparable is whether the request is accepted,
    // not whether a notification arrives (which needs a message pump).
    {
        const long masks[] = {FD_READ, FD_READ | FD_WRITE, 0};
        int i = 0;
        for (long mask : masks)
        {
            LineBool(("CAsyncSocket.AsyncSelect." + std::to_string(i)).c_str(),
                     receiver.AsyncSelect(mask) != FALSE);
            ++i;
        }
    }

    // The default notification handlers. They are PROTECTED -- the library
    // calls them, a caller does not -- so the only way to reach them is the
    // way a consumer would: from a derived class. Real MFC's defaults are
    // empty virtuals and so are this branch's; what is compared is that
    // each returns, accepts any error code, and leaves the socket alone.
    {
        NotificationProbe probe;
        const int codes[] = {0, 10035 /*WSAEWOULDBLOCK*/, -1};
        for (int code : codes)
        {
            const std::string suffix = "." + std::to_string(code);
            probe.CallOnReceive(code);
            LineBool(("CAsyncSocket.OnReceive.default_returns" + suffix).c_str(), true);
            probe.CallOnSend(code);
            LineBool(("CAsyncSocket.OnSend.default_returns" + suffix).c_str(), true);
            probe.CallOnAccept(code);
            LineBool(("CAsyncSocket.OnAccept.default_returns" + suffix).c_str(), true);
            probe.CallOnConnect(code);
            LineBool(("CAsyncSocket.OnConnect.default_returns" + suffix).c_str(), true);
            probe.CallOnClose(code);
            LineBool(("CAsyncSocket.OnClose.default_returns" + suffix).c_str(), true);
            probe.CallOnOutOfBandData(code);
            LineBool(("CAsyncSocket.OnOutOfBandData.default_returns" + suffix).c_str(), true);
        }
        LineBool("CAsyncSocket.On_handlers.left_socket_untouched",
                 probe.m_hSocket == INVALID_SOCKET);
        LineBool("CAsyncSocket.On_handlers.receiver_still_valid",
                 receiver.m_hSocket != INVALID_SOCKET);
    }

    sender.Close();
    receiver.Close();
}

// ---------------------------------------------------------------------
// The remaining one-off members: CMemFile::GrowFile, CArchive::GetFile,
// CTempBuffer::Free, CTime::GetLocalTm, the sized map constructors, the
// HashKey overloads, and AfxSocketTerm.
// ---------------------------------------------------------------------
namespace
{
// GrowFile is protected in real MFC (public here). A using-declaration in
// a derived class re-exports it under both, which is how the same source
// can call it on either side.
class GrowableMemFile : public CMemFile
{
public:
    using CMemFile::GrowFile;
};
} // namespace

static void TestRemainingGaps()
{
    // --- CMemFile::GrowFile ------------------------------------------------
    // Growing must extend the buffer without moving the position or
    // changing what has already been written.
    {
        const ULONGLONG sizes[] = {0, 1, 64, 4096};
        for (ULONGLONG want : sizes)
        {
            GrowableMemFile mf;
            const char seed[] = "seed";
            mf.Write(seed, sizeof(seed) - 1);
            const ULONGLONG posBefore = mf.GetPosition();
            mf.GrowFile(want);
            const std::string tag = std::to_string(want);
            LineBool(("CMemFile.GrowFile." + tag + ".position_unchanged").c_str(),
                     mf.GetPosition() == posBefore);
            LineBool(("CMemFile.GrowFile." + tag + ".length_at_least_written").c_str(),
                     mf.GetLength() >= sizeof(seed) - 1);
            mf.SeekToBegin();
            char readBack[8]{};
            const UINT n = mf.Read(readBack, sizeof(seed) - 1);
            LineInt(("CMemFile.GrowFile." + tag + ".readback_count").c_str(), n);
            Line(("CMemFile.GrowFile." + tag + ".readback").c_str(),
                 std::string(readBack, n));
            mf.Close();
        }
    }

    // --- CArchive::GetFile -------------------------------------------------
    {
        CMemFile backing;
        CArchive ar(&backing, CArchive::store);
        LineBool("CArchive.GetFile.is_the_backing_file", ar.GetFile() == &backing);
        LineBool("CArchive.GetFile.non_null", ar.GetFile() != nullptr);
        ar << static_cast<DWORD>(7);
        ar.Flush();
        LineBool("CArchive.GetFile.same_after_write", ar.GetFile() == &backing);
        ar.Close();
        backing.Close();
    }

    // --- CTempBuffer::Free -------------------------------------------------
    // NOT compared, and the Windows job is what established why: real
    // ATL's CTempBuffer has no Free() at all -- it frees in its destructor
    // and nowhere else ("error C2039: 'Free': is not a member of
    // ATL::CTempBuffer<int,128,ATL::CCRTAllocator>"). Free() is a
    // simple_mfc addition, so there is no counterpart to compare it
    // against; it is still exercised on every destruction above.

    // --- CTime::GetLocalTm --------------------------------------------------
    // A fixed instant, so every field is a compared constant rather than
    // whatever the clock happened to say. Real MFC also accepts a null
    // pointer (returning shared per-thread storage); that form has no
    // thread-safe equivalent here and is not part of the comparison.
    {
        struct Case { const char* label; int y, mo, d, h, mi, s; };
        const Case cases[] = {
            {"epoch_plus",  1970, 1,  2,  3, 4, 5},
            {"leap_day",    2024, 2, 29, 12, 0, 0},
            {"year_end",    1999, 12, 31, 23, 59, 59},
            {"recent",      2023, 7, 14,  6, 30, 15},
        };
        for (const Case& c : cases)
        {
            CTime t(c.y, c.mo, c.d, c.h, c.mi, c.s);
            std::tm tm{};
            std::tm* got = t.GetLocalTm(&tm);
            const std::string base = std::string("CTime.GetLocalTm.") + c.label;
            LineBool((base + ".returns_the_buffer").c_str(), got == &tm);
            LineInt((base + ".tm_year").c_str(), tm.tm_year);
            LineInt((base + ".tm_mon").c_str(), tm.tm_mon);
            LineInt((base + ".tm_mday").c_str(), tm.tm_mday);
            LineInt((base + ".tm_hour").c_str(), tm.tm_hour);
            LineInt((base + ".tm_min").c_str(), tm.tm_min);
            LineInt((base + ".tm_sec").c_str(), tm.tm_sec);
            LineInt((base + ".tm_wday").c_str(), tm.tm_wday);
            LineInt((base + ".tm_yday").c_str(), tm.tm_yday);
        }
    }

    // --- the sized map constructors -----------------------------------------
    // The hint only sizes the hash table; behaviour must not depend on it.
    {
        const INT_PTR hints[] = {1, 17, 1024};
        for (INT_PTR hint : hints)
        {
            CMapStringToPtr m(hint);
            int a = 1, b = 2;
            m.SetAt(L"first", &a);
            m.SetAt(L"second", &b);
            void* found = nullptr;
            const std::string tag = std::to_string(static_cast<long long>(hint));
            LineBool(("CMapStringToPtr.sized_ctor." + tag + ".lookup").c_str(),
                     m.Lookup(L"second", found) != FALSE);
            LineBool(("CMapStringToPtr.sized_ctor." + tag + ".value").c_str(), found == &b);
            LineInt(("CMapStringToPtr.sized_ctor." + tag + ".count").c_str(),
                    static_cast<long long>(m.GetCount()));

            CMapStringToString ms(hint);
            ms.SetAt(L"key", L"value");
            CString out;
            LineBool(("CMapStringToString.sized_ctor." + tag + ".lookup").c_str(),
                     ms.Lookup(L"key", out) != FALSE);
            Line(("CMapStringToString.sized_ctor." + tag + ".value").c_str(), out);
        }
    }

    // --- _AtlGetConversionACP ------------------------------------------------
    // The code page ATL's conversion macros convert through. Its VALUE is a
    // platform fact (Windows answers with a real code-page id, POSIX has no
    // ANSI code page at all), so the cross-platform comparison skips it --
    // the Windows side-by-side one does not, and that is where it matters.
    {
        LineInt("AtlGetConversionACP.value", static_cast<long long>(_AtlGetConversionACP()));
        LineBool("AtlGetConversionACP.is_stable",
                 _AtlGetConversionACP() == _AtlGetConversionACP());
    }

    // --- HashKey -------------------------------------------------------------
    // The string specialisation's exact value IS the contract (CMap's
    // bucket assignment depends on it), so it is compared as a number.
    {
        const wchar_t* keys[] = {L"", L"a", L"abc", L"a longer key value", L"éè"};
        int i = 0;
        for (const wchar_t* k : keys)
        {
            LineInt(("HashKey.LPCTSTR." + std::to_string(i)).c_str(),
                    static_cast<long long>(HashKey<LPCTSTR>(k)));
            ++i;
        }
        const int ints[] = {0, 1, -1, 65536, 1234567};
        i = 0;
        for (int v : ints)
        {
            LineInt(("HashKey.int." + std::to_string(i)).c_str(),
                    static_cast<long long>(HashKey<int>(v)));
            ++i;
        }
    }
}

// ---------------------------------------------------------------------
// AfxSocketTerm, in its own section and called LAST: it tears down the
// thread's socket state, so nothing that uses a socket may follow it.
// ---------------------------------------------------------------------
static void TestAfxSocketTerm()
{
    AfxSocketTerm();
    LineBool("AfxSocketTerm.returns", true);
    // Re-initialising afterwards must work: the pair is reference-counted
    // per thread, not a one-way door.
    LineBool("AfxSocketTerm.reinit_after_term", AfxSocketInit(nullptr) != FALSE);
    AfxSocketTerm();
    LineBool("AfxSocketTerm.second_term_returns", true);
}

// ---------------------------------------------------------------------
int main()
{
    SilenceWindowsDialogs();

    TestRTTI();
    TestCRuntimeClass();
    TestExceptions();
    TestExceptionGaps();
    TestCString();
    TestCStringGaps();
    TestCFile();
    TestCStdioFile();
    TestCMemFile();
    TestCMemFileDetachAttach();
    TestCArchive();
    TestCFileFind();
    TestCFileFindAttributes();
    TestCObList();
    TestCPtrList();
    TestCStringList();
    TestCPtrArray();
    TestCStringArray();
    TestCByteArray();
    TestCUIntArray();
    TestCArrayTemplate();
    TestCListTemplate();
    TestCMapTemplate();
    TestCMapPtrToPtr();
    TestCMapStringToPtr();
    TestCMapStringToString();
    TestCTypedPtrList();
    TestCTypedPtrArray();
    TestTime();
    TestCTempBuffer();
    TestCSimpleArray();
    TestCRBMap();
    TestAfxParseURL();
    TestCriticalSection();
    TestEventAutoReset();
    TestEventManualReset();
    TestEventPulseAndUnlock();
    TestMutex();
    TestCSyncObjectBase();
    TestCWinThread();
    TestCWinThreadLifecycle();
    TestCAsyncSocket();
    TestCAsyncSocketDatagram();

    TestPatternCString();
    TestPatternCTime();
    TestPatternBase64();
    TestPatternUnicodeToUtf8();
    TestRemainingGaps();

    // Last: it tears down the thread's socket state.
    TestAfxSocketTerm();

    // Explicit end-of-run marker. A probe that dies partway through still
    // exits with a code compare.py checks, but a truncated run that
    // somehow exits 0 anyway would otherwise look like "the last N cases
    // are missing" rather than "this probe never finished".
    Line("#END", std::to_string(g_index));
    return 0;
}
