// cases.cpp — conformance test cases, compiled TWICE into two separate
// executables:
//
//   simple_mfc_probe  (-DSIMPLE_MFC_USE_NATIVE)   uses ../../include/*.h
//   real_mfc_probe    (-DSIMPLE_MFC_USE_REAL_MFC) uses the real Visual
//                                                  Studio MFC headers
//
// Both probes run the exact same sequence of calls (this file is shared
// verbatim — only the #include block differs) and print one canonical
// record per checked value to stdout, as "<case name>\t<value>" (see
// Line() below). tests/conformance/compare.cmake runs both executables
// and compares those records BY NAME: any behavioral or result difference
// between simple_mfc and real MFC shows up as a named failing case, and a
// case that only one of the two emits at all is reported as missing
// rather than as a positional shift.
//
// Only ever built on MSVC with the "MFC and ATL" component installed —
// see ../../CMakeLists.txt and ../../.github/workflows/msvc.yml.

#if defined(SIMPLE_MFC_USE_NATIVE)
    #include "afx.h"
    #include "afxcoll.h"
    #include "afxtempl.h"
    #include "afxmt.h"
    #include "atltime.h"
    #include "atltypes.h"
    #include "atlenc.h"
    #include "atlconv.h"
    #include "atlalloc.h"
#elif defined(SIMPLE_MFC_USE_REAL_MFC)
    #include <afx.h>
    #include <afxcoll.h>
    #include <afxtempl.h>
    #include <afxmt.h>
    #include <atltime.h>
    #include <atltypes.h>
    #include <atlenc.h>
    #include <atlconv.h>
    #include <atlalloc.h>
#else
    #error "Define either SIMPLE_MFC_USE_NATIVE or SIMPLE_MFC_USE_REAL_MFC"
#endif

#include <windows.h>

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

#include <atomic>
#include <chrono>
#include <crtdbg.h>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace
{
// Nothing in this harness may ever wait for a human. A probe that stops
// on a modal dialog does not fail -- it HANGS, and compare.cmake (which
// runs both probes with execute_process) would hang with it, taking the
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
void SilenceWindowsDialogs()
{
    for (int report : {_CRT_WARN, _CRT_ERROR, _CRT_ASSERT})
    {
        _CrtSetReportMode(report, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(report, _CRTDBG_FILE_STDERR);
    }
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
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
    int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
    if (n <= 0) return {};
    std::string out(static_cast<size_t>(n - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w, -1, out.data(), n, nullptr, nullptr);
    return out;
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
// The case NAME is the key compare.cmake matches on -- deliberately not
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
    // then e->Delete()). AfxAbort() is intentionally NOT exercised here:
    // it terminates the process outright (real MFC's equivalent of
    // std::abort()), leaving nothing to print and no "rest of the suite"
    // to run afterward in this same-process harness. CException::ReportError()
    // is also intentionally NOT exercised: on the real-MFC side it opens a
    // genuine Win32 MessageBox, which would hang a non-interactive CI
    // runner waiting for a dismissal that never comes.
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
        // one of them ever runs. compare.cmake keys on the name and would
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
    CString dir = TempDir() + CString(L"simple_mfc_conformance_find\\");
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
// CObArray / CPtrArray / CStringArray / CByteArray / CUIntArray
// ---------------------------------------------------------------------
static void TestCObArray()
{
    CObArray arr;
    IntBox a(10), b(20), c(30);
    arr.Add(&a);
    arr.Add(&b);
    arr.Add(&c);

    LineInt("CObArray.GetCount", arr.GetCount());
    LineInt("CObArray.GetAt1.value", static_cast<IntBox*>(arr.GetAt(1))->v);

    IntBox d(99);
    arr.SetAt(1, &d);
    LineInt("CObArray.SetAt1.value", static_cast<IntBox*>(arr.GetAt(1))->v);

    IntBox e(55);
    arr.InsertAt(0, &e);
    LineInt("CObArray.CountAfterInsertAt0", arr.GetCount());
    LineInt("CObArray.GetAt0AfterInsert.value", static_cast<IntBox*>(arr.GetAt(0))->v);

    arr.RemoveAt(0);
    LineInt("CObArray.CountAfterRemoveAt0", arr.GetCount());
    LineInt("CObArray.GetUpperBound", static_cast<long long>(arr.GetUpperBound()));

    LineInt("CObArray.ElementAt0.value", static_cast<IntBox*>(arr.ElementAt(0))->v);
    CObject** data = arr.GetData();
    LineInt("CObArray.GetData.first.value", static_cast<IntBox*>(data[0])->v);
    arr.FreeExtra(); // no printable effect beyond exercising the call
    LineInt("CObArray.CountAfterFreeExtra", arr.GetCount());

    IntBox f(77);
    arr.SetAtGrow(5, &f);
    LineInt("CObArray.CountAfterSetAtGrow5", arr.GetCount());
    LineInt("CObArray.GetAt5.value", static_cast<IntBox*>(arr.GetAt(5))->v);

    CObArray src;
    IntBox g(88);
    src.Add(&g);
    INT_PTR appendedResult = arr.Append(src);
    LineInt("CObArray.Append.result", static_cast<long long>(appendedResult));
    LineInt("CObArray.CountAfterAppend", arr.GetCount());

    CObArray copyDst;
    copyDst.Copy(arr);
    LineInt("CObArray.Copy.count", copyDst.GetCount());

    arr.SetSize(2);
    LineInt("CObArray.CountAfterSetSize2", arr.GetCount());

    arr.RemoveAll();
    LineBool("CObArray.IsEmptyAfterRemoveAll", arr.IsEmpty() != FALSE);
}

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
// CPoint / CSize (atltypes.h) — pure coordinate arithmetic.
// ---------------------------------------------------------------------
static void TestCPointCSize()
{
    CPoint p(10, 20);
    CPoint q(3, 4);
    CSize sz(5, 7);

    LineInt("CPoint.x", p.x);
    LineInt("CPoint.y", p.y);

    CPoint sum = p + q;
    Line("CPoint.plusPoint", std::to_string(sum.x) + "," + std::to_string(sum.y));
    CPoint sumSz = p + sz;
    Line("CPoint.plusSize", std::to_string(sumSz.x) + "," + std::to_string(sumSz.y));
    CSize diff = p - q; // point - point yields a CSize
    Line("CPoint.minusPoint", std::to_string(diff.cx) + "," + std::to_string(diff.cy));
    CPoint negated = -p;
    Line("CPoint.unaryMinus", std::to_string(negated.x) + "," + std::to_string(negated.y));

    CPoint offset = p;
    offset.Offset(1, -2);
    Line("CPoint.Offset", std::to_string(offset.x) + "," + std::to_string(offset.y));
    CPoint offsetSz = p;
    offsetSz.Offset(sz);
    Line("CPoint.OffsetSize", std::to_string(offsetSz.x) + "," + std::to_string(offsetSz.y));

    CPoint setPt;
    setPt.SetPoint(42, -42);
    Line("CPoint.SetPoint", std::to_string(setPt.x) + "," + std::to_string(setPt.y));

    LineBool("CPoint.operatorEq.true", p == CPoint(10, 20));
    LineBool("CPoint.operatorEq.false", p == q);
    LineBool("CPoint.operatorNe", p != q);

    CPoint plusEq = p;
    plusEq += sz;
    Line("CPoint.plusEqualsSize", std::to_string(plusEq.x) + "," + std::to_string(plusEq.y));
    CPoint minusEq = p;
    minusEq -= q;
    Line("CPoint.minusEqualsPoint", std::to_string(minusEq.x) + "," + std::to_string(minusEq.y));

    CSize szSum = sz + CSize(1, 2);
    Line("CSize.plus", std::to_string(szSum.cx) + "," + std::to_string(szSum.cy));
    CSize szDiff = sz - CSize(1, 2);
    Line("CSize.minus", std::to_string(szDiff.cx) + "," + std::to_string(szDiff.cy));
    CSize szNeg = -sz;
    Line("CSize.unaryMinus", std::to_string(szNeg.cx) + "," + std::to_string(szNeg.cy));
    LineBool("CSize.operatorEq.true", sz == CSize(5, 7));
    LineBool("CSize.operatorNe", sz != CSize(5, 8));
    CPoint szPlusPt = sz + q;
    Line("CSize.plusPoint", std::to_string(szPlusPt.x) + "," + std::to_string(szPlusPt.y));
}

// ---------------------------------------------------------------------
// CRect (atltypes.h). The pattern generator further down already hammers
// the intersect/union/subtract operators with random geometry; this
// section covers the named members it never touches — every mutator, the
// accessors, and the rect/point/size operator overloads.
// ---------------------------------------------------------------------
namespace
{
std::string RectStr(const RECT& r)
{
    return "(" + std::to_string(r.left) + "," + std::to_string(r.top) + "," +
           std::to_string(r.right) + "," + std::to_string(r.bottom) + ")";
}
} // namespace

static void TestCRectMethods()
{
    CRect r(10, 20, 110, 220);
    LineInt("CRect.Width", r.Width());
    LineInt("CRect.Height", r.Height());
    Line("CRect.Size", std::to_string(r.Size().cx) + "," + std::to_string(r.Size().cy));
    Line("CRect.CenterPoint", std::to_string(r.CenterPoint().x) + "," + std::to_string(r.CenterPoint().y));
    Line("CRect.TopLeft", std::to_string(r.TopLeft().x) + "," + std::to_string(r.TopLeft().y));
    Line("CRect.BottomRight", std::to_string(r.BottomRight().x) + "," + std::to_string(r.BottomRight().y));
    LineBool("CRect.IsRectEmpty.false", r.IsRectEmpty() != FALSE);
    LineBool("CRect.PtInRect.inside", r.PtInRect(CPoint(50, 50)) != FALSE);
    LineBool("CRect.PtInRect.onRightEdge", r.PtInRect(CPoint(110, 50)) != FALSE);
    LineBool("CRect.PtInRect.onTopLeft", r.PtInRect(CPoint(10, 20)) != FALSE);

    // Constructors other than the four-int one.
    CRect fromPointSize(CPoint(1, 2), CSize(30, 40));
    Line("CRect.ctor.pointSize", RectStr(fromPointSize));
    CRect fromCorners(CPoint(1, 2), CPoint(31, 42));
    Line("CRect.ctor.corners", RectStr(fromCorners));
    RECT plain{5, 6, 7, 8};
    CRect fromRect(plain);
    Line("CRect.ctor.fromRECT", RectStr(fromRect));

    CRect empty;
    empty.SetRectEmpty();
    Line("CRect.SetRectEmpty", RectStr(empty));
    LineBool("CRect.IsRectEmpty.true", empty.IsRectEmpty() != FALSE);

    CRect setr;
    setr.SetRect(1, 2, 3, 4);
    Line("CRect.SetRect", RectStr(setr));

    CRect moved = r;
    moved.MoveToXY(0, 0);
    Line("CRect.MoveToXY", RectStr(moved));
    CRect movedX = r;
    movedX.MoveToX(-5);
    Line("CRect.MoveToX", RectStr(movedX));
    CRect movedY = r;
    movedY.MoveToY(-5);
    Line("CRect.MoveToY", RectStr(movedY));
    CRect movedPt = r;
    movedPt.MoveToXY(CPoint(7, 9));
    Line("CRect.MoveToXY.point", RectStr(movedPt));

    CRect off = r;
    off.OffsetRect(5, -5);
    Line("CRect.OffsetRect.xy", RectStr(off));
    CRect offPt = r;
    offPt.OffsetRect(CPoint(2, 3));
    Line("CRect.OffsetRect.point", RectStr(offPt));
    CRect offSz = r;
    offSz.OffsetRect(CSize(2, 3));
    Line("CRect.OffsetRect.size", RectStr(offSz));

    CRect inf = r;
    inf.InflateRect(5, 10);
    Line("CRect.InflateRect.xy", RectStr(inf));
    CRect inf4 = r;
    inf4.InflateRect(1, 2, 3, 4);
    Line("CRect.InflateRect.ltrb", RectStr(inf4));
    CRect infSz = r;
    infSz.InflateRect(CSize(4, 6));
    Line("CRect.InflateRect.size", RectStr(infSz));
    CRect infRc = r;
    CRect infBy(1, 2, 3, 4);
    infRc.InflateRect(&infBy);
    Line("CRect.InflateRect.rect", RectStr(infRc));

    CRect def = r;
    def.DeflateRect(5, 10);
    Line("CRect.DeflateRect.xy", RectStr(def));
    CRect def4 = r;
    def4.DeflateRect(1, 2, 3, 4);
    Line("CRect.DeflateRect.ltrb", RectStr(def4));
    CRect defSz = r;
    defSz.DeflateRect(CSize(4, 6));
    Line("CRect.DeflateRect.size", RectStr(defSz));
    CRect defRc = r;
    CRect defBy(1, 2, 3, 4);
    defRc.DeflateRect(&defBy);
    Line("CRect.DeflateRect.rect", RectStr(defRc));

    // Named set operations (the operator forms are covered by the pattern
    // generator), including the disjoint cases where the return value is
    // the whole point.
    CRect a(0, 0, 10, 10), b(5, 5, 15, 15), disjoint(100, 100, 110, 110);
    CRect dst;
    LineBool("CRect.IntersectRect.overlapping.result", dst.IntersectRect(&a, &b) != FALSE);
    Line("CRect.IntersectRect.overlapping", RectStr(dst));
    LineBool("CRect.IntersectRect.disjoint.result", dst.IntersectRect(&a, &disjoint) != FALSE);
    Line("CRect.IntersectRect.disjoint", RectStr(dst));
    LineBool("CRect.UnionRect.result", dst.UnionRect(&a, &b) != FALSE);
    Line("CRect.UnionRect", RectStr(dst));
    CRect emptySrc(0, 0, 0, 0);
    LineBool("CRect.UnionRect.withEmpty.result", dst.UnionRect(&a, &emptySrc) != FALSE);
    Line("CRect.UnionRect.withEmpty", RectStr(dst));

    // Operators on whole rects.
    LineBool("CRect.operatorEq.true", a == CRect(0, 0, 10, 10));
    LineBool("CRect.operatorNe", a != b);
    CRect opPlus = a + CPoint(3, 4);
    Line("CRect.operatorPlus.point", RectStr(opPlus));
    CRect opPlusSz = a + CSize(3, 4);
    Line("CRect.operatorPlus.size", RectStr(opPlusSz));
    CRect opMinus = a - CPoint(3, 4);
    Line("CRect.operatorMinus.point", RectStr(opMinus));
    CRect inflateBy(1, 2, 3, 4);
    CRect opPlusRect = a + &inflateBy;
    Line("CRect.operatorPlus.rect", RectStr(opPlusRect));
    // Spelled as an explicit member call, not "a - &inflateBy": CRect
    // converts implicitly to LPRECT, so the built-in pointer-minus-pointer
    // operator becomes a candidate too and the expression is formally
    // ambiguous (GCC warns and picks the member anyway; there is no reason
    // to find out what every other compiler does). Real MFC's CRect has the
    // same conversion operators, so this is not a simple_mfc quirk. The
    // sibling operator+ has no such problem -- built-in pointer arithmetic
    // has no pointer-plus-pointer form.
    CRect opMinusRect = a.operator-(&inflateBy);
    Line("CRect.operatorMinus.rect", RectStr(opMinusRect));

    CRect andEq = a;
    andEq &= b;
    Line("CRect.operatorAndEquals", RectStr(andEq));
    CRect orEq = a;
    orEq |= b;
    Line("CRect.operatorOrEquals", RectStr(orEq));
    CRect plusEq = a;
    plusEq += CPoint(1, 1);
    Line("CRect.operatorPlusEquals.point", RectStr(plusEq));
    CRect minusEq = a;
    minusEq -= CSize(1, 1);
    Line("CRect.operatorMinusEquals.size", RectStr(minusEq));
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

    // Growing from the fixed buffer into the heap must preserve what was
    // written before the move.
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
// Pattern-driven cases: instead of one hand-picked value per assertion,
// generate N inputs from a fixed-seed PRNG and run the same call on each.
// std::mt19937's algorithm is fully specified by the standard, so a given
// seed produces a byte-identical draw sequence in both probes -- they are
// two separate processes/binaries, but compiled from this same source
// file (see the file header), so "the same seed" really does mean "the
// same inputs" here, with no need to serialize/replay anything between
// them. Each case gets a unique, self-describing label (e.g.
// "Pattern.CString.Format.03") so a mismatch in compare.cmake's output
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

static void TestPatternCRectAndPoint()
{
    std::mt19937 rng(kPatternSeed + 1);
    std::uniform_int_distribution<int> coordDist(-500, 500);
    std::uniform_int_distribution<int> sizeDist(0, 300);

    for (int i = 0; i < 40; ++i)
    {
        // Every rng draw is its own statement, deliberately: argument
        // evaluation order is unspecified by the standard, and while both
        // probes are built by the same compiler (so it would be
        // consistent in practice), there is no reason to depend on that.
        int l1 = coordDist(rng);
        int t1v = coordDist(rng);
        int w1 = sizeDist(rng);
        int h1 = sizeDist(rng);
        CRect r1(l1, t1v, l1 + w1, t1v + h1);
        int l2 = coordDist(rng);
        int t2v = coordDist(rng);
        int w2 = sizeDist(rng);
        int h2 = sizeDist(rng);
        CRect r2(l2, t2v, l2 + w2, t2v + h2);

        char label[64];
        std::snprintf(label, sizeof(label), "Pattern.CRect.%02d", i);

        CRect inter = r1 & r2;
        CRect uni = r1 | r2;
        std::string s = "inter=(" + std::to_string(inter.left) + "," + std::to_string(inter.top) + "," +
                         std::to_string(inter.right) + "," + std::to_string(inter.bottom) + ") union=(" +
                         std::to_string(uni.left) + "," + std::to_string(uni.top) + "," +
                         std::to_string(uni.right) + "," + std::to_string(uni.bottom) + ") w1=" +
                         std::to_string(r1.Width()) + " h1=" + std::to_string(r1.Height()) +
                         " empty1=" + (r1.IsRectEmpty() ? "1" : "0");
        Line(label, s);

        int px = coordDist(rng);
        int py = coordDist(rng);
        CPoint p(px, py);
        char labelPt[64];
        std::snprintf(labelPt, sizeof(labelPt), "Pattern.CRect.PtInRect.%02d", i);
        LineBool(labelPt, r1.PtInRect(p) != FALSE);

        char labelSub[64];
        std::snprintf(labelSub, sizeof(labelSub), "Pattern.CRect.Subtract.%02d", i);
        CRect sub;
        BOOL subOk = sub.SubtractRect(&r1, &r2);
        std::string subStr = std::string(subOk ? "1:" : "0:") + "(" + std::to_string(sub.left) + "," +
                              std::to_string(sub.top) + "," + std::to_string(sub.right) + "," +
                              std::to_string(sub.bottom) + ")";
        Line(labelSub, subStr);
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
int main()
{
    SilenceWindowsDialogs();

    TestRTTI();
    TestExceptions();
    TestCString();
    TestCFile();
    TestCStdioFile();
    TestCMemFile();
    TestCMemFileDetachAttach();
    TestCArchive();
    TestCFileFind();
    TestCObList();
    TestCPtrList();
    TestCStringList();
    TestCObArray();
    TestCPtrArray();
    TestCStringArray();
    TestCByteArray();
    TestCUIntArray();
    TestCArrayTemplate();
    TestCListTemplate();
    TestCMapTemplate();
    TestTime();
    TestCPointCSize();
    TestCRectMethods();
    TestCTempBuffer();
    TestCriticalSection();
    TestEventAutoReset();
    TestEventManualReset();
    TestEventPulseAndUnlock();
    TestMutex();

    TestPatternCString();
    TestPatternCRectAndPoint();
    TestPatternCTime();
    TestPatternBase64();
    TestPatternUnicodeToUtf8();

    // Explicit end-of-run marker. A probe that dies partway through still
    // exits with a code compare.cmake checks, but a truncated run that
    // somehow exits 0 anyway would otherwise look like "the last N cases
    // are missing" rather than "this probe never finished".
    Line("#END", std::to_string(g_index));
    return 0;
}
