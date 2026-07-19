// cases.cpp — conformance test cases, compiled TWICE into two separate
// executables:
//
//   simple_mfc_probe  (-DSIMPLE_MFC_USE_NATIVE)   uses ../../include/*.h
//   real_mfc_probe    (-DSIMPLE_MFC_USE_REAL_MFC) uses the real Visual
//                                                  Studio MFC headers
//
// Both probes run the exact same sequence of calls (this file is shared
// verbatim — only the #include block differs) and print one canonical
// line per checked value to stdout. tests/conformance/compare.cmake runs
// both executables and diffs their output byte-for-byte: any behavioral
// or result difference between simple_mfc and real MFC shows up as a
// failing line.
//
// Only ever built on MSVC with the "MFC and ATL" component installed —
// see ../../CMakeLists.txt and ../../.github/workflows/msvc.yml.

#if defined(SIMPLE_MFC_USE_NATIVE)
    #include "afx.h"
    #include "afxcoll.h"
    #include "afxtempl.h"
    #include "afxmt.h"
    #include "atltime.h"
#elif defined(SIMPLE_MFC_USE_REAL_MFC)
    #include <afx.h>
    #include <afxcoll.h>
    #include <afxtempl.h>
    #include <afxmt.h>
    #include <atltime.h>
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
#include <cstdio>
#include <string>
#include <thread>

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
        case '\r': out += "\\r"; break;
        case '\n': out += "\\n"; break;
        case '\t': out += "\\t"; break;
        default: out += c; break;
        }
    }
    return out;
}

void Line(const char* name, const std::string& value)
{
    std::printf("%03d %s = %s\n", ++g_index, name, Escape(value).c_str());
    // Flush immediately: if the process later ends earlier than expected
    // (crash, or anything else), every line printed so far must already
    // be visible to whoever captured stdout, not stuck in a buffer.
    std::fflush(stdout);
}
void Line(const char* name, const wchar_t* value) { Line(name, Utf8(value)); }
void Line(const char* name, const CString& value) { Line(name, Utf8((LPCTSTR)value)); }
void LineBool(const char* name, bool value) { Line(name, std::string(value ? "TRUE" : "FALSE")); }
void LineInt(const char* name, long long value) { Line(name, std::to_string(value)); }

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
int main()
{
    TestRTTI();
    TestExceptions();
    TestCString();
    TestCFile();
    TestCStdioFile();
    TestCMemFile();
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
    TestCriticalSection();
    TestEventAutoReset();
    TestEventManualReset();
    TestEventPulseAndUnlock();
    TestMutex();
    return 0;
}
