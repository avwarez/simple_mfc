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
    #include "eatltypes.h"
    #include "eatlsimpcoll.h"
    #include "eatlcoll.h"
    #include "eafxinet.h"
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
    #include <atltypes.h>
    #include <atlsimpcoll.h>
    #include <atlcoll.h>
    #include <afxinet.h>
#else
    #error "Define either SIMPLE_MFC_USE_NATIVE or SIMPLE_MFC_USE_REAL_MFC"
#endif

#ifdef _WIN32
    #include <windows.h>
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
#include <sstream>
#include <sys/stat.h>
#include <string>
#include <thread>
#include <vector>

#ifndef _WIN32
    #include <filesystem>
    #include <sys/ioctl.h>

    #define MAX_PATH 260

    #define ERROR_FILE_NOT_FOUND 2L
    #define ERROR_DISK_FULL      112L
    #define ERROR_BAD_PATHNAME   161L

    static void wcscpy_s(TCHAR* dst, size_t n, const TCHAR* src)
    {
        if (!dst || n == 0) return;
        size_t i = 0;
        for (; src && src[i] && i + 1 < n; ++i) dst[i] = src[i];
        dst[i] = _T('\0');
    }

    static void GetTempPathW(unsigned long n, TCHAR* buf)
    {
        wcscpy_s(buf, n, _T("/tmp/"));
    }

    static void CreateDirectoryW(const TCHAR* path, void*)
    {
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(path), ec);
    }

    static void RemoveDirectoryW(const TCHAR* path)
    {
        std::error_code ec;
        std::filesystem::remove(std::filesystem::path(path), ec);
    }
#endif

#ifdef _WIN32
    #define SMFC_SEP _T("\\")
#else
    #define SMFC_SEP _T("/")
#endif

namespace
{
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
}

namespace
{

int g_index = 0;

template <class Ch>
std::string Utf8(const Ch* w)
{
    if (!w) return {};
#ifdef _WIN32
    int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
    if (n <= 0) return {};
    std::string out(static_cast<size_t>(n - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w, -1, out.data(), n, nullptr, nullptr);
    return out;
#else
    std::string out;
    for (const Ch* p = w; *p; ++p)
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

std::string Utf8(const CString& s) { return Utf8(s.GetString()); }

std::string Escape(const std::string& s)
{
    std::string out;
    out.reserve(s.size());
    for (char c : s)
    {
        switch (c)
        {
        case '\\': out += "\\\\"; break;
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
    ++g_index;
    std::printf("%s\t%s\n", name, Escape(value).c_str());
    std::fflush(stdout);
}
void Line(const char* name, const TCHAR* value) { Line(name, Utf8(value)); }
void Line(const char* name, const CString& value) { Line(name, Utf8((LPCTSTR)value)); }
void LineBool(const char* name, bool value) { Line(name, std::string(value ? "TRUE" : "FALSE")); }
void LineInt(const char* name, long long value) { Line(name, std::to_string(value)); }

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

int Sign(int v) { return (v > 0) - (v < 0); }

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
    TCHAR buf[MAX_PATH]{};
    GetTempPathW(MAX_PATH, buf);
    return CString(buf);
}

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

}

static void TestRTTI()
{
    CObject* fileEx = new CFileException();
    LineBool("RTTI.CFileException.IsKindOf.CException", fileEx->IsKindOf(RUNTIME_CLASS(CException)) != FALSE);
    LineBool("RTTI.CFileException.IsKindOf.CObject", fileEx->IsKindOf(RUNTIME_CLASS(CObject)) != FALSE);
    LineBool("RTTI.CFileException.IsKindOf.CMemoryException", fileEx->IsKindOf(RUNTIME_CLASS(CMemoryException)) != FALSE);
    Line("RTTI.CFileException.ClassName", std::string(fileEx->GetRuntimeClass()->m_lpszClassName));
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

static void TestExceptions()
{
    CFileException fe(CFileException::fileNotFound, ERROR_FILE_NOT_FOUND, _T("missing_file.dat"));
    TCHAR buf[256]{};
    BOOL ok = fe.GetErrorMessage(buf, 256);
    LineBool("CFileException.GetErrorMessage.returns_true", ok != FALSE);
    LineInt("CFileException.m_cause", fe.m_cause);
    LineInt("CFileException.m_lOsError", fe.m_lOsError);
    Line("CFileException.m_strFileName", fe.m_strFileName);

    CMemoryException me;
    TCHAR mbuf[256]{};
    me.GetErrorMessage(mbuf, 256);

    CFileException* heapEx = new CFileException(CFileException::badPath, ERROR_BAD_PATHNAME, _T("x"));
    LineInt("CFileException.Delete.m_cause_before", heapEx->m_cause);
    heapEx->Delete();

    try
    {
        AfxThrowFileException(CFileException::diskFull, ERROR_DISK_FULL, _T("y.dat"));
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

static void TestCString()
{
    CString s = _T("  Hello, World!  ");
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

    LineInt("CString.Find.substr", trimmed.Find(_T("World")));
    LineInt("CString.Find.missing", trimmed.Find(_T("xyz")));
    LineInt("CString.Find.char", trimmed.Find(_T('W')));
    LineInt("CString.ReverseFind", trimmed.ReverseFind(_T('o')));

    CString fmt;
    fmt.Format(_T("%d-%s-%02d"), 2026, _T("Jul"), 9);
    Line("CString.Format.result", fmt);

    CString app = _T("base");
    app.AppendFormat(_T("+%d=%s"), 42, _T("done"));
    Line("CString.AppendFormat.result", app);

    Line("CString.Left5", trimmed.Left(5));
    Line("CString.Right6", trimmed.Right(6));
    Line("CString.Mid7_5", trimmed.Mid(7, 5));
    Line("CString.Mid7_NoCount", trimmed.Mid(7));

    CString rep = trimmed;
    int nrep = rep.Replace(_T("o"), _T("0"));
    LineInt("CString.Replace.count", nrep);
    Line("CString.Replace.result", rep);

    CString repChar = trimmed;
    int nrepChar = repChar.Replace(_T('l'), _T('L'));
    LineInt("CString.ReplaceChar.count", nrepChar);
    Line("CString.ReplaceChar.result", repChar);

    LineInt("CString.Compare.equal", Sign(CString(_T("abc")).Compare(_T("abc"))));
    LineInt("CString.Compare.less", Sign(CString(_T("abc")).Compare(_T("abd"))));
    LineInt("CString.Compare.greater", Sign(CString(_T("abd")).Compare(_T("abc"))));
    LineInt("CString.CompareNoCase.equal", Sign(CString(_T("ABC")).CompareNoCase(_T("abc"))));
    LineInt("CString.CompareNoCase.less", Sign(CString(_T("ABC")).CompareNoCase(_T("abd"))));

    CString del = trimmed;
    del.Delete(0, 6);
    Line("CString.Delete.result", del);
    LineInt("CString.Delete.returned_length", del.GetLength());

    CString ins = CString(_T("Hello World"));
    ins.Insert(5, _T(","));
    Line("CString.Insert.string.result", ins);
    CString insCh = CString(_T("ac"));
    insCh.Insert(1, _T('b'));
    Line("CString.Insert.char.result", insCh);

    Line("CString.SpanExcluding", CString(_T("12345abc")).SpanExcluding(_T("abcdefg")));

    CString tok = _T("a,b,,c");
    int start = 0;
    Line("CString.Tokenize.1", tok.Tokenize(_T(","), start));
    LineInt("CString.Tokenize.pos_after_1", start);
    Line("CString.Tokenize.2", tok.Tokenize(_T(","), start));
    LineInt("CString.Tokenize.pos_after_2", start);
    Line("CString.Tokenize.3_empty", tok.Tokenize(_T(","), start));

    CString trimChar = _T("xxhelloxx");
    trimChar.Trim(_T('x'));
    Line("CString.TrimChar", trimChar);

    CString trimSet = _T("##--hello--##");
    trimSet.Trim(_T("#-"));
    Line("CString.TrimSet", trimSet);

    CString trimRightDefault = _T("hello   ");
    trimRightDefault.TrimRight();
    Line("CString.TrimRight.default", trimRightDefault);

    CString trimRightChar = _T("helloxxx");
    trimRightChar.TrimRight(_T('x'));
    Line("CString.TrimRight.char", trimRightChar);

    CString getset = _T("abc");
    LineInt("CString.GetAt1", getset.GetAt(1));
    getset.SetAt(1, _T('Z'));
    Line("CString.SetAt.result", getset);

    CString cat = CString(_T("foo")) + CString(_T("bar"));
    Line("CString.operatorPlus", cat);
    LineBool("CString.operatorEq.true", CString(_T("x")) == CString(_T("x")));
    LineBool("CString.operatorEq.false", CString(_T("x")) == CString(_T("y")));
    LineBool("CString.operatorNe", CString(_T("x")) != CString(_T("y")));

    CString buf;
    TCHAR* p = buf.GetBuffer(32);
    wcscpy_s(p, 32, _T("buffered"));
    buf.ReleaseBuffer();
    Line("CString.GetBuffer_ReleaseBuffer.result", buf);
    LineInt("CString.GetBuffer_ReleaseBuffer.length", buf.GetLength());

    CString emptied = _T("not empty");
    emptied.Empty();
    LineBool("CString.Empty.IsEmptyAfter", emptied.IsEmpty() != FALSE);

    CString repeated(_T('x'), 5);
    Line("CString.CharRepeatCtor", repeated);

    CString noarg = _T("abc");
    TCHAR* pna = noarg.GetBuffer();
    Line("CString.GetBuffer_NoArg", CString(pna));

    CString trimRightSet = _T("hello##--");
    trimRightSet.TrimRight(_T("#-"));
    Line("CString.TrimRight.set", trimRightSet);

    CString idx = _T("index");
    LineInt("CString.operatorIndex2", idx[2]);

    CString plusEqStr = _T("foo");
    plusEqStr += CString(_T("bar"));
    Line("CString.operatorPlusEqString", plusEqStr);
    CString plusEqChar = _T("foo");
    plusEqChar += _T('!');
    Line("CString.operatorPlusEqChar", plusEqChar);

    LineBool("CString.operatorLess.true", CString(_T("a")) < CString(_T("b")));
    LineBool("CString.operatorLess.false", CString(_T("b")) < CString(_T("a")));

    CString fmtHex; fmtHex.Format(_T("%08X"), 0xDEADu);
    Line("CString.Format.hex", fmtHex);
    CString fmtChar; fmtChar.Format(_T("%c|%c"), _T('Z'), _T('9'));
    Line("CString.Format.char", fmtChar);
    CString fmtFloat; fmtFloat.Format(_T("%.3f"), 3.14159265);
    Line("CString.Format.float", fmtFloat);
    CString fmtWidth; fmtWidth.Format(_T("[%5d][%-5d][%+d]"), 42, 42, 42);
    Line("CString.Format.width", fmtWidth);
    CString fmtPercent; fmtPercent.Format(_T("100%% done"));
    Line("CString.Format.percent", fmtPercent);

    CString empty;
    empty.Trim();
    LineBool("CString.Empty.TrimStaysEmpty", empty.IsEmpty() != FALSE);
    LineInt("CString.Empty.FindMissing", empty.Find(_T("x")));
    LineInt("CString.Empty.Length", empty.GetLength());

    CString abc = _T("abc");
    Line("CString.Left0", abc.Left(0));
    Line("CString.LeftBeyond", abc.Left(100));
    Line("CString.Right0", abc.Right(0));
    Line("CString.RightBeyond", abc.Right(100));
    Line("CString.MidAtEnd", abc.Mid(3));
    Line("CString.MidBeyondCount", abc.Mid(1, 100));

    CString haystack = _T("abcabcabc");
    LineInt("CString.Find.fromIndex", haystack.Find(_T("abc"), 1));
    LineInt("CString.ReverseFind.missing", haystack.ReverseFind(_T('z')));
}

static void TestCFile()
{
    CString path = TempDir() + CString(_T("simple_mfc_conformance_file.bin"));

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

    CString renamedPath = TempDir() + CString(_T("simple_mfc_conformance_file_renamed.bin"));
    CFile::Rename(path, renamedPath);
    CFileStatus statusAfterRename{};
    LineBool("CFile.Rename.thenGetStatus.ok", CFile::GetStatus(renamedPath, statusAfterRename) != FALSE);

    CFile::Remove(renamedPath);
    CFileStatus statusAfterRemove{};
    LineBool("CFile.Remove.thenGetStatus.fails", CFile::GetStatus(renamedPath, statusAfterRemove) == FALSE);

    CString path2 = TempDir() + CString(_T("simple_mfc_conformance_file2.bin"));
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
    CString path = TempDir() + CString(_T("simple_mfc_conformance_stdio.txt"));

    CStdioFile wf;
    wf.Open(path, CFile::modeCreate | CFile::modeWrite);
    wf.WriteString(_T("first line\r\n"));
    wf.WriteString(_T("second line\r\n"));
    SafeClose(wf);

    CStdioFile rf;
    rf.Open(path, CFile::modeRead);
    CString line1, line2, line3;
    BOOL got1 = rf.ReadString(line1);
    BOOL got2 = rf.ReadString(line2);
    BOOL got3 = rf.ReadString(line3);
    SafeClose(rf);

    LineBool("CStdioFile.ReadString.line1.ok", got1 != FALSE);
    Line("CStdioFile.ReadString.line1", line1);
    LineBool("CStdioFile.ReadString.line2.ok", got2 != FALSE);
    Line("CStdioFile.ReadString.line2", line2);
    LineBool("CStdioFile.ReadString.line3PastEof.fails", got3 == FALSE);

    SafeRemoveFile(path);

    CString path2 = TempDir() + CString(_T("simple_mfc_conformance_stdio2.txt"));
    {
        CStdioFile ctorWrite(path2, CFile::modeCreate | CFile::modeWrite);
        ctorWrite.WriteString(_T("buffer overload line\r\n"));
        SafeClose(ctorWrite);

        CStdioFile bufRead;
        bufRead.Open(path2, CFile::modeRead);
        TCHAR lineBuf[128]{};
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

    mf.Seek(0, CFile::begin);
    mf.Seek(5, CFile::current);
    LineInt("CMemFile.Seek.current", static_cast<long long>(mf.GetPosition()));
    mf.Seek(-3, CFile::end);
    LineInt("CMemFile.Seek.end.minus3", static_cast<long long>(mf.GetPosition()));

    mf.SetLength(4);
    LineInt("CMemFile.SetLength4.GetLength", static_cast<long long>(mf.GetLength()));
    mf.SetLength(10);
    LineInt("CMemFile.SetLength10.GetLength", static_cast<long long>(mf.GetLength()));
}

static void TestCFileFind()
{
    CString dir = TempDir() + CString(_T("simple_mfc_conformance_find") SMFC_SEP);
    CreateDirectoryW(dir, nullptr);

    const TCHAR* names[] = {_T("alpha.txt"), _T("beta.txt"), _T("gamma.dat")};
    for (const TCHAR* name : names)
    {
        CFile f;
        f.Open(dir + CString(name), CFile::modeCreate | CFile::modeWrite);
        const char payload[] = "x";
        f.Write(payload, sizeof(payload) - 1);
        SafeClose(f);
    }

    CString matched[8];
    int matchCount = 0;
    CFileFind finder;
    BOOL working = finder.FindFile(dir + CString(_T("*.txt")));
    while (working)
    {
        working = finder.FindNextFile();
        if (finder.IsDots()) continue;
        if (matchCount < 8) matched[matchCount++] = finder.GetFileName();
    }
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

    {
        CFileFind single;
        BOOL foundOne = single.FindFile(dir + CString(_T("alpha.txt")));
        BOOL hasMore = single.FindNextFile();
        (void)hasMore;
        LineBool("CFileFind.Single.foundOne", foundOne != FALSE);
        Line("CFileFind.Single.GetFilePath", single.GetFilePath());
        LineInt("CFileFind.Single.GetLength", static_cast<long long>(single.GetLength()));
        LineBool("CFileFind.Single.IsDirectory", single.IsDirectory() != FALSE);
        Line("CFileFind.Single.GetRoot", single.GetRoot());
        SafeClose(single);
    }

    for (const TCHAR* name : names)
        SafeRemoveFile(dir + CString(name));
    RemoveDirectoryW(dir);
}

namespace
{
class IntBox : public CObject
{
public:
    int v;
    explicit IntBox(int x) : v(x) {}
};
}

static void TestCObList()
{
    CObList list;
    IntBox a(1), b(2), c(3);
    list.AddTail(&a);
    list.AddTail(&b);
    list.AddHead(&c);

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

    POSITION tailPos = list.GetTailPosition();
    LineInt("CObList.GetTailPosition.value", static_cast<IntBox*>(list.GetAt(tailPos))->v);
    CObject* prevVal = list.GetPrev(tailPos);
    LineInt("CObList.GetPrev.value", static_cast<IntBox*>(prevVal)->v);

    IntBox d(999);
    POSITION headPos2 = list.GetHeadPosition();
    list.SetAt(headPos2, &d);
    LineInt("CObList.SetAt.value", static_cast<IntBox*>(list.GetHead())->v);

    IntBox e(111), g(222);
    POSITION afterHead = list.GetHeadPosition();
    list.InsertAfter(afterHead, &e);
    POSITION beforeTail = list.FindIndex(2);
    list.InsertBefore(beforeTail, &g);
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

    list.RemoveAt(list.FindIndex(0));
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
    list.AddHead(p3);

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
    POSITION idxPos = list.FindIndex(1);
    LineInt("CPtrList.FindIndex1", reinterpret_cast<intptr_t>(list.GetNext(idxPos)));

    POSITION tailPos = list.GetTailPosition();
    LineInt("CPtrList.GetTailPosition", reinterpret_cast<intptr_t>(list.GetPrev(tailPos)));

    list.InsertAfter(list.GetHeadPosition(), p4);
    LineInt("CPtrList.CountAfterInsertAfter", list.GetCount());
    list.InsertBefore(list.FindIndex(3), p1);
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
    list.AddTail(_T("one"));
    list.AddTail(_T("two"));
    list.AddHead(_T("zero"));

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

    LineBool("CStringList.Find.found", list.Find(_T("one")) != nullptr);
    LineBool("CStringList.Find.notFound", list.Find(_T("missing")) != nullptr);

    Line("CStringList.RemoveHead.value", list.RemoveHead());
    LineInt("CStringList.CountAfterRemoveHead", list.GetCount());
    Line("CStringList.RemoveTail.value", list.RemoveTail());
    LineInt("CStringList.CountAfterRemoveTail", list.GetCount());

    list.RemoveAll();
    LineBool("CStringList.IsEmptyAfterRemoveAll", list.IsEmpty() != FALSE);
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
    arr.Add(_T("aa"));
    arr.Add(_T("bb"));
    arr.Add(_T("cc"));
    LineInt("CStringArray.GetCount", arr.GetCount());
    Line("CStringArray.GetAt1", arr.GetAt(1));
    arr.SetAt(1, _T("BB"));
    Line("CStringArray.SetAt1", arr.GetAt(1));
    arr.RemoveAt(0);
    LineInt("CStringArray.CountAfterRemoveAt0", arr.GetCount());
    Line("CStringArray.GetAt0AfterRemove", arr.GetAt(0));
    LineInt("CStringArray.GetSize", static_cast<long long>(arr.GetSize()));
    LineBool("CStringArray.IsEmpty", arr.IsEmpty() != FALSE);

    arr.InsertAt(0, _T("zz"));
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
    arr.FreeExtra();
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
    list.AddTail(_T("x"));
    list.AddTail(_T("y"));
    list.AddHead(_T("w"));
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

    LineBool("CList_CString.Find.found", list.Find(_T("x")) != nullptr);
    LineBool("CList_CString.Find.notFound", list.Find(_T("missing")) != nullptr);
    POSITION idxPos = list.FindIndex(1);
    Line("CList_CString.FindIndex1.value", list.GetAt(idxPos));

    POSITION tailPos = list.GetTailPosition();
    Line("CList_CString.GetTailPosition.value", list.GetAt(tailPos));
    Line("CList_CString.GetPrev.value", list.GetPrev(tailPos));

    POSITION headPos = list.GetHeadPosition();
    list.SetAt(headPos, _T("W2"));
    Line("CList_CString.SetAt.value", list.GetHead());

    list.InsertAfter(list.GetHeadPosition(), _T("inserted"));
    LineInt("CList_CString.CountAfterInsertAfter", list.GetCount());
    list.InsertBefore(list.FindIndex(2), _T("beforeThird"));
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
    CMap<CString, LPCTSTR, int, int> map;
    map.SetAt(_T("one"), 1);
    map.SetAt(_T("two"), 2);
    map.SetAt(_T("three"), 3);

    LineInt("CMap.GetCount", map.GetCount());

    int v = 0;
    LineBool("CMap.Lookup.found", map.Lookup(_T("two"), v) != FALSE);
    LineInt("CMap.Lookup.value", v);
    LineBool("CMap.Lookup.notFound", map.Lookup(_T("missing"), v) != FALSE);

    LineBool("CMap.RemoveKey.existing", map.RemoveKey(_T("one")) != FALSE);
    LineBool("CMap.RemoveKey.missing", map.RemoveKey(_T("one")) != FALSE);
    LineInt("CMap.CountAfterRemoveKey", map.GetCount());

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

    LineBool("CMap.PLookup.found", map.PLookup(_T("three")) != nullptr);
    if (const auto* pair = map.PLookup(_T("three")))
        LineInt("CMap.PLookup.value", pair->value);

    LineInt("CMap.GetSize", static_cast<long long>(map.GetSize()));
    LineBool("CMap.IsEmpty", map.IsEmpty() != FALSE);

    CMap<CString, LPCTSTR, int, int> freshMap;
    freshMap.InitHashTable(64);
    LineBool("CMap.GetHashTableSize.nonZero", freshMap.GetHashTableSize() > 0);
    freshMap.SetAt(_T("k"), 1);
    LineInt("CMap.CountAfterInitHashTableThenSetAt", freshMap.GetCount());

    int pCount = 0;
    int pSum = 0;
    for (const auto* p = map.PGetFirstAssoc(); p; p = map.PGetNextAssoc(p))
    {
        ++pCount;
        pSum += p->value;
    }
    LineInt("CMap.PIteration.count", pCount);
    LineInt("CMap.PIteration.sum", pSum);

    const auto* hit = map.PLookup(_T("two"));
    LineBool("CMap.PLookup.hit.non_null", hit != nullptr);
    Line("CMap.PLookup.hit.key", hit ? hit->key : CString());
    LineInt("CMap.PLookup.hit.value", hit ? hit->value : -1);
    LineBool("CMap.PLookup.miss.is_null", map.PLookup(_T("nosuchkey")) == nullptr);

    {
        typedef CMap<CString, LPCTSTR, int, int> TypedMap;
        TypedMap typedMap;
        typedMap.SetAt(_T("alpha"), 10);
        typedMap.SetAt(_T("beta"), 20);
        const TypedMap& constView = typedMap;

        const TypedMap::CPair* constPair = constView.PLookup(_T("alpha"));
        LineBool("CMap.CPair.const.non_null", constPair != nullptr);
        Line("CMap.CPair.const.key", constPair ? constPair->key : CString());
        LineInt("CMap.CPair.const.value", constPair ? constPair->value : -1);

        TypedMap::CPair* pair = typedMap.PLookup(_T("beta"));
        LineBool("CMap.CPair.nonconst.non_null", pair != nullptr);
        Line("CMap.CPair.nonconst.key", pair ? pair->key : CString());
        LineInt("CMap.CPair.nonconst.value", pair ? pair->value : -1);

        const TypedMap::CPair* firstAssoc = constView.PGetFirstAssoc();
        LineBool("CMap.CPair.PGetFirstAssoc.non_null", firstAssoc != nullptr);
        LineBool("CMap.CPair.PGetNextAssoc.reaches_second",
                 firstAssoc != nullptr && constView.PGetNextAssoc(firstAssoc) != nullptr);
        LineBool("CMap.CPair.PLookup.miss.is_null",
                 constView.PLookup(_T("gamma")) == nullptr);
    }

    map.RemoveAll();
    LineBool("CMap.IsEmptyAfterRemoveAll", map.IsEmpty() != FALSE);
    LineInt("CMap.CountAfterRemoveAll", map.GetCount());

    CMap<CString, LPCTSTR, int, int> map2(20);
    map2.SetAt(_T("only"), 1);
    LineInt("CMap.ExplicitBlockSizeCtor.GetCount", map2.GetCount());

    CMap<CString, LPCTSTR, int, int> ov;
    ov.SetAt(_T("k"), 1);
    ov.SetAt(_T("k"), 99);
    LineInt("CMap.SetAt.overwrite.count", ov.GetCount());
    int ovv = 0;
    ov.Lookup(_T("k"), ovv);
    LineInt("CMap.SetAt.overwrite.value", ovv);
}

namespace
{
std::string RectStr(const RECT& r)
{
    return "(" + std::to_string(r.left) + "," + std::to_string(r.top) + "," +
           std::to_string(r.right) + "," + std::to_string(r.bottom) + ")";
}
std::string PointStr(const POINT& p)
{
    return std::to_string(p.x) + "," + std::to_string(p.y);
}
std::string SizeStr(const SIZE& s)
{
    return std::to_string(s.cx) + "," + std::to_string(s.cy);
}
}

static void TestCPointCSize()
{
    CPoint p(10, 20);
    CPoint q(3, 4);
    CSize sz(5, 7);

    LineInt("CPoint.x", p.x);
    LineInt("CPoint.y", p.y);

    Line("CPoint.plusPoint", PointStr(p + q));
    Line("CPoint.plusSize", PointStr(p + sz));
    Line("CPoint.minusPoint", SizeStr(p - q));
    Line("CPoint.minusSize", PointStr(p - sz));
    Line("CPoint.unaryMinus", PointStr(-p));

    CPoint fromSize(sz);
    Line("CPoint.ctor.fromSize", PointStr(fromSize));
    CPoint fromPoint((POINT)q);
    Line("CPoint.ctor.fromPOINT", PointStr(fromPoint));
    CPoint packed(static_cast<DWORD>(0xFFF00010u));
    Line("CPoint.ctor.packed", PointStr(packed));

    CPoint offset = p;
    offset.Offset(1, -2);
    Line("CPoint.Offset", PointStr(offset));
    CPoint offsetSz = p;
    offsetSz.Offset(sz);
    Line("CPoint.OffsetSize", PointStr(offsetSz));
    CPoint offsetPt = p;
    offsetPt.Offset((POINT)q);
    Line("CPoint.OffsetPoint", PointStr(offsetPt));

    CPoint setPt;
    setPt.SetPoint(42, -42);
    Line("CPoint.SetPoint", PointStr(setPt));
    Line("CPoint.ctor.default", PointStr(CPoint()));

    LineBool("CPoint.operatorEq.true", p == CPoint(10, 20));
    LineBool("CPoint.operatorEq.false", p == q);
    LineBool("CPoint.operatorNe", p != q);

    CPoint plusEq = p;
    plusEq += sz;
    Line("CPoint.plusEqualsSize", PointStr(plusEq));
    CPoint plusEqPt = p;
    plusEqPt += (POINT)q;
    Line("CPoint.plusEqualsPoint", PointStr(plusEqPt));
    CPoint minusEq = p;
    minusEq -= (POINT)q;
    Line("CPoint.minusEqualsPoint", PointStr(minusEq));
    CPoint minusEqSz = p;
    minusEqSz -= sz;
    Line("CPoint.minusEqualsSize", PointStr(minusEqSz));

    CRect base(1, 2, 3, 4);
    Line("CPoint.plusRect", RectStr(p + &base));
    Line("CPoint.minusRect", RectStr(p.operator-(&base)));

    Line("CSize.ctor.default", SizeStr(CSize()));
    CSize fromPt((POINT)q);
    Line("CSize.ctor.fromPOINT", SizeStr(fromPt));
    CSize fromSz((SIZE)sz);
    Line("CSize.ctor.fromSIZE", SizeStr(fromSz));
    Line("CSize.plus", SizeStr(sz + CSize(1, 2)));
    Line("CSize.minus", SizeStr(sz - CSize(1, 2)));
    Line("CSize.unaryMinus", SizeStr(-sz));
    LineBool("CSize.operatorEq.true", sz == CSize(5, 7));
    LineBool("CSize.operatorEq.false", sz == CSize(5, 8));
    LineBool("CSize.operatorNe", sz != CSize(5, 8));
    CSize plusEqSz = sz;
    plusEqSz += CSize(1, 2);
    Line("CSize.plusEquals", SizeStr(plusEqSz));
    CSize minusEqSz2 = sz;
    minusEqSz2 -= CSize(1, 2);
    Line("CSize.minusEquals", SizeStr(minusEqSz2));
    Line("CSize.plusPoint", PointStr(sz + (POINT)q));
    Line("CSize.minusPoint", PointStr(sz - (POINT)q));
    Line("CSize.plusRect", RectStr(sz + &base));
    Line("CSize.minusRect", RectStr(sz.operator-(&base)));
}

static void TestCRectMethods()
{
    CRect r(10, 20, 110, 220);
    LineInt("CRect.Width", r.Width());
    LineInt("CRect.Height", r.Height());
    Line("CRect.Size", SizeStr(r.Size()));
    Line("CRect.CenterPoint", PointStr(r.CenterPoint()));
    Line("CRect.TopLeft", PointStr(r.TopLeft()));
    Line("CRect.BottomRight", PointStr(r.BottomRight()));
    LineBool("CRect.IsRectEmpty.false", r.IsRectEmpty() != FALSE);
    LineBool("CRect.PtInRect.inside", r.PtInRect(CPoint(50, 50)) != FALSE);
    LineBool("CRect.PtInRect.onRightEdge", r.PtInRect(CPoint(110, 50)) != FALSE);
    LineBool("CRect.PtInRect.onTopLeft", r.PtInRect(CPoint(10, 20)) != FALSE);

    Line("CRect.ctor.default", RectStr(CRect()));
    CRect fromPointSize(CPoint(1, 2), CSize(30, 40));
    Line("CRect.ctor.pointSize", RectStr(fromPointSize));
    CRect fromCorners(CPoint(1, 2), CPoint(31, 42));
    Line("CRect.ctor.corners", RectStr(fromCorners));
    RECT plain{5, 6, 7, 8};
    CRect fromRect(plain);
    Line("CRect.ctor.fromRECT", RectStr(fromRect));
    CRect fromPtr(&plain);
    Line("CRect.ctor.fromLPCRECT", RectStr(fromPtr));

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
    LineBool("CRect.SubtractRect.contained.result", dst.SubtractRect(&a, &a) != FALSE);
    Line("CRect.SubtractRect.contained", RectStr(dst));

    LineBool("CRect.operatorEq.true", a == CRect(0, 0, 10, 10));
    LineBool("CRect.operatorNe", a != b);
    Line("CRect.operatorPlus.point", RectStr(a + CPoint(3, 4)));
    Line("CRect.operatorPlus.size", RectStr(a + CSize(3, 4)));
    Line("CRect.operatorMinus.point", RectStr(a - CPoint(3, 4)));
    Line("CRect.operatorMinus.size", RectStr(a - CSize(3, 4)));
    CRect inflateBy(1, 2, 3, 4);
    Line("CRect.operatorPlus.rect", RectStr(a + &inflateBy));
    Line("CRect.operatorMinus.rect", RectStr(a.operator-(&inflateBy)));

    Line("CRect.operatorAnd", RectStr(a & b));
    Line("CRect.operatorOr", RectStr(a | b));
    CRect andEq = a;
    andEq &= b;
    Line("CRect.operatorAndEquals", RectStr(andEq));
    CRect orEq = a;
    orEq |= b;
    Line("CRect.operatorOrEquals", RectStr(orEq));
    CRect plusEq = a;
    plusEq += CPoint(1, 1);
    Line("CRect.operatorPlusEquals.point", RectStr(plusEq));
    CRect plusEqSz = a;
    plusEqSz += CSize(1, 1);
    Line("CRect.operatorPlusEquals.size", RectStr(plusEqSz));
    CRect plusEqRc = a;
    plusEqRc += &inflateBy;
    Line("CRect.operatorPlusEquals.rect", RectStr(plusEqRc));
    CRect minusEq = a;
    minusEq -= CSize(1, 1);
    Line("CRect.operatorMinusEquals.size", RectStr(minusEq));
    CRect minusEqPt = a;
    minusEqPt -= CPoint(1, 1);
    Line("CRect.operatorMinusEquals.point", RectStr(minusEqPt));
    CRect minusEqRc = a;
    minusEqRc -= &inflateBy;
    Line("CRect.operatorMinusEquals.rect", RectStr(minusEqRc));

    CRect conv(1, 2, 3, 4);
    LPRECT asPtr = conv;
    Line("CRect.operatorLPRECT", RectStr(*asPtr));
    const CRect constConv(5, 6, 7, 8);
    LPCRECT asConstPtr = constConv;
    Line("CRect.operatorLPCRECT", RectStr(*asConstPtr));
}

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

    Line("CTime.Format", t1.Format(_T("%Y-%m-%d %H:%M:%S")));
    LineInt("CTime.GetTime", static_cast<long long>(t1.GetTime()));

    CTime defaultTime;
    LineInt("CTime.defaultCtor.GetTime", static_cast<long long>(defaultTime.GetTime()));
    CTime fromEpoch(static_cast<__time64_t>(t1.GetTime()));
    LineBool("CTime.explicitEpochCtor.equalsT1", fromEpoch == t1);

    CTime now = CTime::GetCurrentTime();
    LineBool("CTime.GetCurrentTime.plausibleYear", now.GetYear() >= 2020);

    CTimeSpan defaultSpan;
    LineInt("CTimeSpan.defaultCtor.GetTotalSeconds", static_cast<long long>(defaultSpan.GetTotalSeconds()));
    CTimeSpan fromSeconds(3661);
    LineInt("CTimeSpan.explicitSecondsCtor.GetHours", fromSeconds.GetHours());
    LineInt("CTimeSpan.explicitSecondsCtor.GetTotalSeconds", static_cast<long long>(fromSeconds.GetTotalSeconds()));

    CTimeSpan spanA(0, 1, 0, 0);
    CTimeSpan spanB(0, 0, 30, 0);
    CTimeSpan spanSum = spanA + spanB;
    LineInt("CTimeSpan.operatorPlus.GetTotalMinutes", static_cast<long long>(spanSum.GetTotalMinutes()));
    CTimeSpan spanDiff = spanA - spanB;
    LineInt("CTimeSpan.operatorMinus.GetTotalMinutes", static_cast<long long>(spanDiff.GetTotalMinutes()));

    CTime t2000(2000, 1, 1, 0, 0, 0);
    LineInt("CTime.2000.GetYear", t2000.GetYear());
    LineInt("CTime.2000.GetMonth", t2000.GetMonth());
    LineInt("CTime.2000.GetDay", t2000.GetDay());
    LineInt("CTime.2000.GetDayOfWeek", t2000.GetDayOfWeek());
    Line("CTime.2000.Format", t2000.Format(_T("%Y/%m/%d %H:%M:%S day%j")));

    CTime tLeap(2024, 2, 29, 23, 59, 59);
    LineInt("CTime.Leap.GetMonth", tLeap.GetMonth());
    LineInt("CTime.Leap.GetDay", tLeap.GetDay());
    LineInt("CTime.Leap.GetDayOfWeek", tLeap.GetDayOfWeek());
    Line("CTime.Leap.Format", tLeap.Format(_T("%y-%m-%dT%H:%M:%S")));

    CTimeSpan neg = t1 - t2;
    LineInt("CTimeSpan.negative.GetTotalSeconds", static_cast<long long>(neg.GetTotalSeconds()));
    LineInt("CTimeSpan.negative.GetDays", neg.GetDays());
}

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
    CEvent ev(FALSE, FALSE);
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
    CEvent ev(FALSE, TRUE);
    ev.SetEvent();
    BOOL first = ev.Lock(1000);
    BOOL second = ev.Lock(1000);
    ev.ResetEvent();
    BOOL third = ev.Lock(200);
    LineBool("CEvent.ManualReset.firstLock", first != FALSE);
    LineBool("CEvent.ManualReset.secondLock", second != FALSE);
    LineBool("CEvent.ManualReset.thirdLockTimesOut", third == FALSE);
}

static void TestEventPulseAndUnlock()
{
    CEvent ev(FALSE, FALSE);

    LineBool("CEvent.Unlock.noop", ev.Unlock() != FALSE);

    BOOL pulseResult = ev.PulseEvent();
    LineBool("CEvent.PulseEvent.returns_true", pulseResult != FALSE);

    BOOL afterPulse = ev.Lock(200);
    LineBool("CEvent.AfterPulse.timesOut", afterPulse == FALSE);
}

static void TestMutex()
{
    CMutex mtx;
    CSingleLock lk(&mtx, TRUE);
    LineBool("CMutex.SingleLock.locked", lk.IsLocked() != FALSE);
    lk.Unlock();
    LineBool("CMutex.SingleLock.unlockedAfterUnlock", lk.IsLocked() == FALSE);

    CSingleLock lk2(&mtx, TRUE);
    LONG prevCount = -1;
    BOOL unlockedWithCount = lk2.Unlock(1, &prevCount);
    LineBool("CSingleLock.Unlock2Arg", unlockedWithCount != FALSE);
}

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

    ULONGLONG len = mf.GetLength();
    LineInt("CArchive.store.byteCount", static_cast<long long>(len));
    mf.SeekToBegin();
    std::vector<unsigned char> raw(static_cast<size_t>(len));
    if (!raw.empty())
        mf.Read(raw.data(), static_cast<UINT>(raw.size()));
    Line("CArchive.store.bytes", Hex(raw.data(), raw.size()));

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
        Line("CArchive.roundTrip.float", Hex(&f, sizeof(f)));
        Line("CArchive.roundTrip.double", Hex(&d, sizeof(d)));
        LineInt("CArchive.roundTrip.ULONGLONG", static_cast<long long>(q));
    }

    CMemFile mfs;
    {
        CArchive ar(&mfs, CArchive::store);
        ar << CString(_T("archived string"));
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

static void TestCTempBuffer()
{
    CTempBuffer<int, 64> fixedBuf;
    fixedBuf.Allocate(8);
    for (size_t i = 0; i < 8; ++i)
        fixedBuf[i] = static_cast<int>(i) * 3;
    std::string fixedVals;
    for (size_t i = 0; i < 8; ++i)
        fixedVals += std::to_string(fixedBuf[i]) + (i == 7 ? "" : ",");
    Line("CTempBuffer.fixed.values", fixedVals);

    CTempBuffer<int, 16> heapBuf;
    heapBuf.Allocate(100);
    for (size_t i = 0; i < 100; ++i)
        heapBuf[i] = static_cast<int>(i);
    LineInt("CTempBuffer.heap.first", heapBuf[size_t(0)]);
    LineInt("CTempBuffer.heap.last", heapBuf[size_t(99)]);

    CTempBuffer<int, 16> growBuf;
    growBuf.Allocate(4);
    for (size_t i = 0; i < 4; ++i)
        growBuf[i] = 100 + static_cast<int>(i);
    growBuf.Reallocate(64);
    std::string preserved;
    for (size_t i = 0; i < 4; ++i)
        preserved += std::to_string(growBuf[i]) + (i == 3 ? "" : ",");
    Line("CTempBuffer.growPreservesContent", preserved);

    CTempBuffer<char, 32> byteBuf;
    byteBuf.AllocateBytes(200);
    byteBuf[size_t(0)] = 'A';
    byteBuf[size_t(199)] = 'Z';
    Line("CTempBuffer.AllocateBytes.ends",
         std::string(1, byteBuf[size_t(0)]) + std::string(1, byteBuf[size_t(199)]));
}

static void TestCSimpleArray()
{
    CSimpleArray<int> arr;
    for (int v : {10, 20, 30, 40, 20})
        arr.Add(v);
    LineInt("CSimpleArray.GetSize", arr.GetSize());
    LineInt("CSimpleArray.index0", arr[0]);
    LineInt("CSimpleArray.index4", arr[4]);

    const int* data = arr.GetData();
    std::string contents;
    for (int i = 0; i < arr.GetSize(); ++i)
        contents += std::to_string(data[i]) + (i + 1 < arr.GetSize() ? "," : "");
    Line("CSimpleArray.GetData.contents", contents);

    LineInt("CSimpleArray.Find.present", arr.Find(30));
    LineInt("CSimpleArray.Find.firstOfDup", arr.Find(20));
    LineInt("CSimpleArray.Find.absent", arr.Find(999));

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

namespace
{
std::string RbForward(CRBMap<ULONGLONG, DWORD>& m, int& count)
{
    std::string out;
    count = 0;
    POSITION pos = m.GetHeadPosition();
    while (pos)
    {
        auto* pair = m.GetNext(pos);
        if (count) out += ",";
        out += std::to_string(pair->m_key) + "=" + std::to_string(pair->m_value);
        ++count;
    }
    return out;
}
}

static void TestCRBMap()
{
    CRBMap<ULONGLONG, DWORD> m;
    m.SetAt(50, 500);
    m.SetAt(10, 100);
    m.SetAt(40, 400);
    m.SetAt(20, 200);
    m.SetAt(30, 300);

    int count = 0;
    std::string fwd = RbForward(m, count);
    LineInt("CRBMap.count", count);
    Line("CRBMap.ordered", fwd);

    m.SetAt(30, 333);
    int count2 = 0;
    std::string fwd2 = RbForward(m, count2);
    LineInt("CRBMap.count.afterOverwrite", count2);
    Line("CRBMap.ordered.afterOverwrite", fwd2);

    POSITION head = m.GetHeadPosition();
    LineInt("CRBMap.headKey", static_cast<long long>(m.GetKeyAt(head)));
    LineInt("CRBMap.headValue", static_cast<long long>(m.GetValueAt(head)));
    POSITION tail = m.GetTailPosition();
    LineInt("CRBMap.tailKey", static_cast<long long>(m.GetKeyAt(tail)));

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

    {
        typedef CRBMap<ULONGLONG, DWORD> TypedRBMap;
        TypedRBMap typedMap;
        typedMap.SetAt(1, 100);
        typedMap.SetAt(2, 200);

        POSITION forward = typedMap.GetHeadPosition();
        TypedRBMap::CPair* headPair = typedMap.GetNext(forward);
        LineBool("CRBMap.CPair.GetNext.non_null", headPair != nullptr);
        LineInt("CRBMap.CPair.GetNext.m_key",
                headPair ? static_cast<long long>(headPair->m_key) : -1);
        LineInt("CRBMap.CPair.GetNext.m_value",
                headPair ? static_cast<long long>(headPair->m_value) : -1);

        POSITION backward = typedMap.GetTailPosition();
        TypedRBMap::CPair* tailPair = typedMap.GetPrev(backward);
        LineBool("CRBMap.CPair.GetPrev.non_null", tailPair != nullptr);
        LineInt("CRBMap.CPair.GetPrev.m_key",
                tailPair ? static_cast<long long>(tailPair->m_key) : -1);
        LineInt("CRBMap.CPair.GetPrev.m_value",
                tailPair ? static_cast<long long>(tailPair->m_value) : -1);
    }

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

    POSITION exact = m.FindFirstKeyAfter(30);
    LineInt("CRBMap.FindFirstKeyAfter.exact", static_cast<long long>(m.GetKeyAt(exact)));
    POSITION gap = m.FindFirstKeyAfter(25);
    LineInt("CRBMap.FindFirstKeyAfter.gap", static_cast<long long>(m.GetKeyAt(gap)));
    POSITION past = m.FindFirstKeyAfter(1000);
    LineBool("CRBMap.FindFirstKeyAfter.pastEnd.none", past == nullptr);

    m.RemoveAt(m.GetHeadPosition());
    int count3 = 0;
    std::string fwd3 = RbForward(m, count3);
    LineInt("CRBMap.count.afterRemoveHead", count3);
    Line("CRBMap.ordered.afterRemoveHead", fwd3);

    m.RemoveAll();
    LineBool("CRBMap.emptyAfterRemoveAll", m.GetHeadPosition() == nullptr);
}

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
    const Case kCases[] = {
        {"http.explicitPort", _T("http://example.com:8080/path/to/file")},
        {"https.defaultPort", _T("https://example.com/index.html")},
        {"https.explicitPort", _T("https://secure.example.com:8443/a")},
        {"http.defaultPort", _T("http://example.com/")},
        {"http.noObject", _T("http://example.com")},
        {"ftp.explicitPort", _T("ftp://files.example.com:2121/pub/readme.txt")},
        {"ftp.defaultPort", _T("ftp://files.example.com/pub/")},
        {"http.query", _T("http://example.com/search?q=mfc&lang=en")},
        {"http.deepPath", _T("http://example.com/a/b/c/d.html")},
        {"schemeless.fails", _T("example.com/path")},
        {"empty.fails", _T("")},
    };

    for (const Case& c : kCases)
    {
        DWORD service = 0;
        CString server, object;
        INTERNET_PORT port = 0;
        BOOL ok = AfxParseURL(c.url, service, server, object, port);

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

namespace
{
constexpr unsigned kPatternSeed = 20260722u;

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
}

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
        CString wordW(wordA);

        CString fmt;
        switch (i % 5)
        {
        case 0: fmt.Format(_T("%d|%s"), n, (LPCTSTR)wordW); break;
        case 1: fmt.Format(_T("%*d"), width, n); break;
        case 2: fmt.Format(_T("%.*f"), prec, d); break;
        case 3: fmt.Format(_T("%08X"), static_cast<unsigned int>(n)); break;
        default: fmt.Format(_T("[%-*s]=%+d"), width, (LPCTSTR)wordW, n); break;
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
        std::string s = "inter=" + RectStr(r1 & r2) + " union=" + RectStr(r1 | r2) +
                        " w1=" + std::to_string(r1.Width()) + " h1=" + std::to_string(r1.Height()) +
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
        Line(labelSub, std::string(subOk ? "1:" : "0:") + RectStr(sub));

        char labelCtr[64];
        std::snprintf(labelCtr, sizeof(labelCtr), "Pattern.CRect.CenterPoint.%02d", i);
        Line(labelCtr, PointStr(r1.CenterPoint()));
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
    std::uniform_int_distribution<int> kindDist(0, 3);
    std::uniform_int_distribution<int> asciiDist(0x20, 0x7E);
    std::uniform_int_distribution<int> latin1Dist(0xA0, 0xFF);
    std::uniform_int_distribution<int> bmpDist(0x0100, 0x2FFF);
    std::uniform_int_distribution<int> highSurrDist(0xD800, 0xDBFF);
    std::uniform_int_distribution<int> lowSurrDist(0xDC00, 0xDFFF);

    for (int i = 0; i < 30; ++i)
    {
        int n = lenDist(rng);
        std::basic_string<TCHAR> w;
        w.reserve(static_cast<size_t>(n) * 2);
        for (int c = 0; c < n; ++c)
        {
            switch (kindDist(rng))
            {
            case 0: w.push_back(static_cast<TCHAR>(asciiDist(rng))); break;
            case 1: w.push_back(static_cast<TCHAR>(latin1Dist(rng))); break;
            case 2: w.push_back(static_cast<TCHAR>(bmpDist(rng))); break;
            default:
                w.push_back(static_cast<TCHAR>(highSurrDist(rng)));
                w.push_back(static_cast<TCHAR>(lowSurrDist(rng)));
                break;
            }
        }
        w.push_back(0);

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

static void TestCMapStringToPtr()
{
    CMapStringToPtr m;
    m.InitHashTable(17);
    LineInt("CMapStringToPtr.GetCount.empty", m.GetCount());
    LineBool("CMapStringToPtr.IsEmpty.empty", m.IsEmpty() != FALSE);

    static const LPCTSTR kKeys[] = {_T("alpha"), _T("beta"), _T("gamma"), _T("delta")};
    for (int i = 0; i < 4; ++i)
        m.SetAt(kKeys[i], &g_slots[i]);
    LineInt("CMapStringToPtr.GetCount.after_4", m.GetCount());

    void* found = nullptr;
    LineBool("CMapStringToPtr.Lookup.hit", m.Lookup(_T("gamma"), found) != FALSE);
    LineInt("CMapStringToPtr.Lookup.hit.value", found ? *static_cast<int*>(found) : -1);
    void* missed = nullptr;
    LineBool("CMapStringToPtr.Lookup.miss", m.Lookup(_T("epsilon"), missed) != FALSE);
    CString built = _T("ga");
    built += _T("mma");
    LineBool("CMapStringToPtr.Lookup.by_content", m.Lookup(built, found) != FALSE);

    m[_T("epsilon")] = &g_slots[4];
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

    LineBool("CMapStringToPtr.RemoveKey.present", m.RemoveKey(_T("alpha")) != FALSE);
    LineBool("CMapStringToPtr.RemoveKey.absent", m.RemoveKey(_T("alpha")) != FALSE);
    LineInt("CMapStringToPtr.GetCount.after_remove", m.GetCount());
    m.RemoveAll();
    LineInt("CMapStringToPtr.GetCount.after_RemoveAll", m.GetCount());
}

static void TestCMapStringToString()
{
    CMapStringToString m;
    LineInt("CMapStringToString.GetCount.empty", m.GetCount());

    m.SetAt(_T("one"), _T("uno"));
    m.SetAt(_T("two"), _T("due"));
    m.SetAt(_T("three"), _T("tre"));
    LineInt("CMapStringToString.GetCount.after_3", m.GetCount());

    CString value;
    LineBool("CMapStringToString.Lookup.hit", m.Lookup(_T("two"), value) != FALSE);
    Line("CMapStringToString.Lookup.hit.value", value);
    CString absent = _T("untouched");
    LineBool("CMapStringToString.Lookup.miss", m.Lookup(_T("four"), absent) != FALSE);
    Line("CMapStringToString.Lookup.miss.leaves_value", absent);

    m.SetAt(_T("two"), _T("DUE"));
    m.Lookup(_T("two"), value);
    Line("CMapStringToString.SetAt.overwrite.value", value);
    LineInt("CMapStringToString.GetCount.after_overwrite", m.GetCount());

    m[_T("four")] = _T("quattro");
    LineInt("CMapStringToString.GetCount.after_subscript", m.GetCount());
    Line("CMapStringToString.subscript.reads_back", m[_T("four")]);

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

    LineBool("CMapStringToString.RemoveKey.present", m.RemoveKey(_T("one")) != FALSE);
    LineInt("CMapStringToString.GetCount.after_remove", m.GetCount());
    m.RemoveAll();
    LineInt("CMapStringToString.GetCount.after_RemoveAll", m.GetCount());
    LineBool("CMapStringToString.IsEmpty.after_RemoveAll", m.IsEmpty() != FALSE);
}

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
    Line("CTypedPtrList.GetNext.forward", SortedJoin(forward));
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
    g_slots[5] = 15;

    arr.InsertAt(0, &g_slots[4]);
    LineInt("CTypedPtrArray.GetSize.after_InsertAt", arr.GetSize());
    LineInt("CTypedPtrArray.InsertAt.new_head", *arr.GetAt(0));

    arr.SetAtGrow(7, &g_slots[3]);
    LineInt("CTypedPtrArray.GetSize.after_SetAtGrow", arr.GetSize());
    LineBool("CTypedPtrArray.SetAtGrow.fills_gap_with_null", arr.GetAt(6) == nullptr);
    LineInt("CTypedPtrArray.SetAtGrow.reads_back", *arr.GetAt(7));

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
}

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

    LineInt("CWinThread.ResumeThread.from_suspended", static_cast<long long>(pThread->ResumeThread()));

    LineBool("CWinThread.worker.ran", PollUntil([] { return g_workerRan.load() != 0; }));
    LineInt("CWinThread.worker.received_param", g_workerParam.load());
}

static void TestCAsyncSocket()
{
#if defined(SIMPLE_MFC_USE_REAL_MFC)
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
             listener.Create(0, SOCK_STREAM, FD_ACCEPT | FD_CLOSE, _T("127.0.0.1")) != FALSE);
    LineBool("CAsyncSocket.m_hSocket.valid_after_Create", listener.m_hSocket != INVALID_SOCKET);
    LineBool("CAsyncSocket.FromHandle.finds_owner", CAsyncSocket::FromHandle(listener.m_hSocket) == &listener);

    CString boundAddress;
    UINT boundPort = 0;
    LineBool("CAsyncSocket.GetSockName.returns_true", listener.GetSockName(boundAddress, boundPort) != FALSE);
    Line("CAsyncSocket.GetSockName.address", boundAddress);
    LineBool("CAsyncSocket.GetSockName.port_is_assigned", boundPort != 0);

    LineBool("CAsyncSocket.Listen", listener.Listen(5) != FALSE);

    CAsyncSocket tooEarly;
    LineBool("CAsyncSocket.Accept.with_no_pending_connection", listener.Accept(tooEarly) != FALSE);
    LineBool("CAsyncSocket.GetLastError.reports_the_would_block", CAsyncSocket::GetLastError() != 0);

    CAsyncSocket client;
    LineBool("CAsyncSocket.Create.client",
             client.Create(0, SOCK_STREAM, FD_CONNECT | FD_READ | FD_WRITE | FD_CLOSE) != FALSE);
    client.Connect(_T("127.0.0.1"), boundPort);

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
}

static void TestCRuntimeClass()
{
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

    DynMade concrete;
    CObject* asObject = &concrete;
    LineBool("AfxDynamicDownCast.to_own_class", DYNAMIC_DOWNCAST(DynMade, asObject) != nullptr);
    LineBool("AfxDynamicDownCast.to_base_class", DYNAMIC_DOWNCAST(DynBase, asObject) != nullptr);
    LineBool("AfxDynamicDownCast.to_unrelated_class",
             DYNAMIC_DOWNCAST(CFileException, asObject) != nullptr);
    LineBool("AfxDynamicDownCast.null_input", DYNAMIC_DOWNCAST(DynMade, (CObject*)nullptr) != nullptr);
}

static void TestExceptionGaps()
{
    CNotSupportedException nse;
    LineBool("CNotSupportedException.IsKindOf.CSimpleException",
             nse.IsKindOf(RUNTIME_CLASS(CSimpleException)) != FALSE);
    LineBool("CNotSupportedException.IsKindOf.CException",
             nse.IsKindOf(RUNTIME_CLASS(CException)) != FALSE);
    LineBool("CNotSupportedException.IsKindOf.CMemoryException",
             nse.IsKindOf(RUNTIME_CLASS(CMemoryException)) != FALSE);

    CArchiveException ae(CArchiveException::badIndex, _T("stream.dat"));
    LineInt("CArchiveException.m_cause", ae.m_cause);
    LineBool("CArchiveException.IsKindOf.CException",
             ae.IsKindOf(RUNTIME_CLASS(CException)) != FALSE);
    LineBool("CArchiveException.IsKindOf.CSimpleException",
             ae.IsKindOf(RUNTIME_CLASS(CSimpleException)) != FALSE);

    {
        const UINT sizes[] = {1, 2, 8, 64, 255};
        for (UINT n : sizes)
        {
            TCHAR buf[300];
            for (TCHAR& c : buf) c = _T('#');
            CFileException fe(CFileException::fileNotFound, ERROR_FILE_NOT_FOUND, _T("nope.dat"));
            fe.GetErrorMessage(buf, n);
            std::string name = "CFileException.GetErrorMessage.respects_nMaxError." + std::to_string(n);
            LineBool(name.c_str(), buf[n] == _T('#'));
            std::string term = "CFileException.GetErrorMessage.nul_terminates." + std::to_string(n);
            bool terminated = false;
            for (UINT i = 0; i < n; ++i)
                if (buf[i] == _T('\0')) { terminated = true; break; }
            LineBool(term.c_str(), terminated);
        }
    }

    {
        struct Case { const char* label; LONG osError; };
        const Case cases[] = {
            {"file_not_found",       2L},
            {"path_not_found",       3L},
            {"too_many_open_files",  4L},
            {"access_denied",        5L},
            {"invalid_handle",       6L},
            {"invalid_drive",       15L},
            {"current_directory",   16L},
            {"write_protect",       19L},
            {"sharing_violation",   32L},
            {"lock_violation",      33L},
            {"handle_eof",          38L},
            {"handle_disk_full",    39L},
            {"file_exists",         80L},
            {"invalid_parameter",   87L},
            {"disk_full",          112L},
            {"invalid_name",       123L},
            {"negative_seek",      131L},
            {"dir_not_empty",      145L},
            {"bad_pathname",       161L},
            {"already_exists",     183L},
            {"filename_too_long",  206L},
            {"unmapped_high",    30000L},
        };
        for (const Case& c : cases)
        {
            try
            {
                CFileException::ThrowOsError(c.osError, _T("probe.dat"));
#if defined(SIMPLE_MFC_USE_REAL_MFC)
                Line((std::string("CFileException.ThrowOsError.") + c.label).c_str(),
                     std::string("NEVER (did not throw)"));
#endif
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

namespace
{
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
}

namespace
{
std::string CodeUnits(const CString& str)
{
    std::string out;
    for (int i = 0; i < str.GetLength(); ++i)
    {
        char buf[24];
        std::snprintf(buf, sizeof(buf), "%s%04lX", i ? " " : "",
                      static_cast<unsigned long>(static_cast<unsigned int>(str.GetAt(i))));
        out += buf;
    }
    return out;
}
}

static void TestNonBMP()
{
    LineInt("NonBMP.sizeof.TCHAR", (long long)sizeof(TCHAR));

    CString emoji(_T("a\U0001F600b"));
    LineInt("NonBMP.emoji.GetLength", emoji.GetLength());
    Line("NonBMP.emoji.code_units", CodeUnits(emoji));
    LineInt("NonBMP.emoji.Find.a", emoji.Find(_T('a')));
    LineInt("NonBMP.emoji.Find.b", emoji.Find(_T('b')));
    LineInt("NonBMP.emoji.ReverseFind.b", emoji.ReverseFind(_T('b')));
    LineInt("NonBMP.emoji.GetAt.1", (long long)(unsigned long)(unsigned int)emoji.GetAt(1));
    Line("NonBMP.emoji.Mid.1.1", CodeUnits(emoji.Mid(1, 1)));
    Line("NonBMP.emoji.Mid.1.2", CodeUnits(emoji.Mid(1, 2)));
    Line("NonBMP.emoji.Left.2", CodeUnits(emoji.Left(2)));
    Line("NonBMP.emoji.Right.2", CodeUnits(emoji.Right(2)));

    CString cjk(_T("\U00020000\U0002A6DF"));
    LineInt("NonBMP.cjk.GetLength", cjk.GetLength());
    Line("NonBMP.cjk.code_units", CodeUnits(cjk));

    CString deseret(_T("\U00010437"));
    LineInt("NonBMP.deseret.GetLength", deseret.GetLength());

    CString mixed(_T("é\U0001F600中"));
    LineInt("NonBMP.mixed.GetLength", mixed.GetLength());
    Line("NonBMP.mixed.code_units", CodeUnits(mixed));

    CString upper(_T("a\U0001F600b"));
    upper.MakeUpper();
    Line("NonBMP.MakeUpper.code_units", CodeUnits(upper));
    CString lower(_T("A\U0001F600B"));
    lower.MakeLower();
    Line("NonBMP.MakeLower.code_units", CodeUnits(lower));

    CString formatted;
    formatted.Format(_T("[%s]"), emoji.GetString());
    LineInt("NonBMP.Format.GetLength", formatted.GetLength());
    Line("NonBMP.Format.code_units", CodeUnits(formatted));

    CString concat = emoji + cjk;
    LineInt("NonBMP.concat.GetLength", concat.GetLength());

    LineInt("NonBMP.Compare.equal", emoji.Compare(_T("a\U0001F600b")));
    LineInt("NonBMP.Compare.ordering", emoji.Compare(_T("a\U0001F601b")) < 0 ? -1 : 1);

    CString replaced(_T("a\U0001F600b"));
    LineInt("NonBMP.Replace.count", replaced.Replace(_T("\U0001F600"), _T("X")));
    Line("NonBMP.Replace.result", CodeUnits(replaced));

    const int srcChars = emoji.GetLength() + 1;
    int needed = AtlUnicodeToUTF8(emoji.GetString(), srcChars, nullptr, 0);
    std::vector<char> dst(static_cast<size_t>(needed > 0 ? needed : 1), 0);
    int outLen = AtlUnicodeToUTF8(emoji.GetString(), srcChars, dst.data(), needed);
    LineInt("NonBMP.AtlUnicodeToUTF8.length", outLen);
    Line("NonBMP.AtlUnicodeToUTF8.hex", Hex(dst.data(), static_cast<size_t>(outLen > 0 ? outLen : 0)));
}

static void TestCStringGaps()
{
    {
        struct Case { const char* label; LPCTSTR a; LPCTSTR b; };
        const Case cases[] = {
            {"equal",        _T("alpha"),  _T("alpha")},
            {"less",         _T("alpha"),  _T("beta")},
            {"greater",      _T("beta"),   _T("alpha")},
            {"prefix",       _T("alph"),   _T("alpha")},
            {"case_differs", _T("Alpha"),  _T("alpha")},
            {"empty_vs_x",   _T(""),       _T("x")},
            {"both_empty",   _T(""),       _T("")},
            {"digits",       _T("file10"), _T("file9")},
        };
        for (const Case& c : cases)
        {
            CString s(c.a);
            LineInt((std::string("CString.Collate.") + c.label).c_str(), Sign(s.Collate(c.b)));
            LineInt((std::string("CString.CollateNoCase.") + c.label).c_str(),
                    Sign(s.CollateNoCase(c.b)));
        }
    }

    {
        struct Case { const char* label; LPCTSTR s; LPCTSTR set; };
        const Case cases[] = {
            {"first_char",   _T("abcdef"),      _T("a")},
            {"middle",       _T("abcdef"),      _T("dc")},
            {"none",         _T("abcdef"),      _T("xyz")},
            {"empty_set",    _T("abcdef"),      _T("")},
            {"empty_string", _T(""),            _T("abc")},
            {"separators",   _T("host:8080/p"), _T("/:")},
            {"non_ascii",    _T("café au lait"), _T("é")},
        };
        for (const Case& c : cases)
        {
            CString s(c.s);
            LineInt((std::string("CString.FindOneOf.") + c.label).c_str(), s.FindOneOf(c.set));
        }
    }

    {
        const int lengths[] = {0, 1, 3, 6};
        for (int n : lengths)
        {
            CString s(_T("abcdef"));
            s.Truncate(n);
            Line((std::string("CString.Truncate.") + std::to_string(n)).c_str(), s);
            LineInt((std::string("CString.Truncate.") + std::to_string(n) + ".GetLength").c_str(),
                    s.GetLength());
        }
    }

    {
        struct Case { const char* label; LPCTSTR src; };
        const Case cases[] = {
            {"plain",     _T("replacement")},
            {"empty",     _T("")},
            {"embedded",  _T("a\tb")},
            {"non_ascii", _T("äöü")},
        };
        for (const Case& c : cases)
        {
            CString s(_T("original value"));
            s.SetString(c.src);
            Line((std::string("CString.SetString.psz.") + c.label).c_str(), s);
            LineInt((std::string("CString.SetString.psz.") + c.label + ".GetLength").c_str(),
                    s.GetLength());
        }
        const int counts[] = {0, 1, 4, 11};
        for (int n : counts)
        {
            CString s(_T("original value"));
            s.SetString(_T("replacement"), n);
            Line((std::string("CString.SetString.psz_n.") + std::to_string(n)).c_str(), s);
            LineInt((std::string("CString.SetString.psz_n.") + std::to_string(n) + ".GetLength").c_str(),
                    s.GetLength());
        }
    }

    {
        const TCHAR* inputs[] = {_T(""), _T("x"), _T("a longer value with spaces"), _T("éè")};
        int i = 0;
        for (const TCHAR* in : inputs)
        {
            CString s(in);
            LPCTSTR p = s.GetString();
            Line((std::string("CString.GetString.") + std::to_string(i)).c_str(), p);
            LineBool((std::string("CString.GetString.") + std::to_string(i) + ".nul_terminated").c_str(),
                     p[s.GetLength()] == _T('\0'));
            ++i;
        }
    }

    {
        CString s;
        const TCHAR chars[] = {_T('a'), _T('B'), _T('0'), _T(' '), _T('é')};
        for (TCHAR c : chars) s.AppendChar(c);
        Line("CString.AppendChar.accumulated", s);
        LineInt("CString.AppendChar.GetLength", s.GetLength());
    }

    {
        CString s;
        CallFormatV(s, _T("%d/%s/%c"), 42, _T("mid"), _T('z'));
        Line("CString.FormatV.mixed", s);
        CallAppendFormatV(s, _T(" + %u"), 7u);
        Line("CString.AppendFormatV.appended", s);

        CString empty;
        CallFormatV(empty, _T("%s"), _T(""));
        LineInt("CString.FormatV.empty_result_length", empty.GetLength());

        CString wide;
        CallFormatV(wide, _T("%08.3f|%X"), 3.14159, 48879u);
        Line("CString.FormatV.numeric_flags", wide);
    }

    {
        const int lengths[] = {0, 1, 5};
        for (int n : lengths)
        {
            CString s;
            LPTSTR buf = s.GetBuffer(16);
            for (int i = 0; i < 8; ++i) buf[i] = static_cast<TCHAR>(_T('a') + i);
            buf[3] = _T('\0');
            s.ReleaseBufferSetLength(n);
            LineInt((std::string("CString.ReleaseBufferSetLength.") + std::to_string(n)).c_str(),
                    s.GetLength());
        }
    }

#ifdef _WIN32
    {
        const UINT ids[] = {1u, 100u, 61472u};
        for (UINT id : ids)
        {
            CString s(_T("previous content"));
            BOOL ok = s.LoadString(id);
            LineBool(("CString.LoadString.id." + std::to_string(id)).c_str(), ok != FALSE);
            LineBool(("CString.LoadString.id." + std::to_string(id) + ".empties_on_miss").c_str(),
                     s.IsEmpty() != FALSE);

            CString h(_T("previous content"));
            BOOL okh = h.LoadString(::GetModuleHandleW(nullptr), id);
            LineBool(("CString.LoadString.hinstance." + std::to_string(id)).c_str(), okh != FALSE);

            CString l(_T("previous content"));
            BOOL okl = l.LoadString(::GetModuleHandleW(nullptr), id, 0x0409);
            LineBool(("CString.LoadString.langid." + std::to_string(id)).c_str(), okl != FALSE);
        }
    }

    {
        const TCHAR* inputs[] = {_T(""), _T("x"), _T("a BSTR value"), _T("é中")};
        int i = 0;
        for (const TCHAR* in : inputs)
        {
            CString s(in);
            BSTR b = s.AllocSysString();
            LineBool(("CString.AllocSysString." + std::to_string(i) + ".non_null").c_str(), b != nullptr);
            LineInt(("CString.AllocSysString." + std::to_string(i) + ".SysStringLen").c_str(),
                    b ? static_cast<long long>(::SysStringLen(b)) : -1);
            Line(("CString.AllocSysString." + std::to_string(i) + ".content").c_str(),
                 b ? b : _T("(null)"));
            if (b) ::SysFreeString(b);
            ++i;
        }
    }
#endif
}

static void TestCFileFindAttributes()
{
    CString dir = TempDir() + CString(_T("simple_mfc_conformance_attr") SMFC_SEP);
    CreateDirectoryW(dir, nullptr);

    CString plain = dir + CString(_T("plain.bin"));
    {
        CFile f;
        f.Open(plain, CFile::modeCreate | CFile::modeWrite);
        const char payload[] = "0123456789";
        f.Write(payload, sizeof(payload) - 1);
        SafeClose(f);
    }

    {
        CFileFind finder;
        BOOL found = finder.FindFile(plain);
        BOOL more = finder.FindNextFile();
        (void)more;
        LineBool("CFileFind.Attr.found", found != FALSE);
        LineBool("CFileFind.Attr.IsHidden", finder.IsHidden() != FALSE);
        LineBool("CFileFind.Attr.IsSystem", finder.IsSystem() != FALSE);
        LineBool("CFileFind.Attr.IsReadOnly", finder.IsReadOnly() != FALSE);
        LineBool("CFileFind.Attr.IsCompressed", finder.IsCompressed() != FALSE);
        LineBool("CFileFind.Attr.IsTemporary", finder.IsTemporary() != FALSE);
        LineBool("CFileFind.Attr.IsArchived", finder.IsArchived() != FALSE);
        LineBool("CFileFind.Attr.IsDirectory", finder.IsDirectory() != FALSE);

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

#ifdef _WIN32
        FILETIME writeFt{}, createFt{}, accessFt{};
        LineBool("CFileFind.GetLastWriteTime.FILETIME.returns_true",
                 finder.GetLastWriteTime(&writeFt) != FALSE);
        LineBool("CFileFind.GetCreationTime.FILETIME.returns_true",
                 finder.GetCreationTime(&createFt) != FALSE);
        LineBool("CFileFind.GetLastAccessTime.FILETIME.returns_true",
                 finder.GetLastAccessTime(&accessFt) != FALSE);
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

        LineBool("CFileFind.times.write_not_before_creation",
                 writeTime.GetTime() + 2 >= createTime.GetTime());

        finder.Close();
    }

    {
        CString ro = dir + CString(_T("readonly.bin"));
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
        BOOL more = finder.FindNextFile();
        (void)more;
        LineBool("CFileFind.ReadOnly.found", found != FALSE);
        LineBool("CFileFind.ReadOnly.IsReadOnly", finder.IsReadOnly() != FALSE);
        LineBool("CFileFind.ReadOnly.IsHidden", finder.IsHidden() != FALSE);
        finder.Close();

        MakeWritable(ro);
        SafeRemoveFile(ro);
    }

    {
        const TCHAR* extra[] = {_T("one.seq"), _T("two.seq"), _T("three.seq")};
        for (const TCHAR* n : extra)
        {
            CFile f;
            f.Open(dir + CString(n), CFile::modeCreate | CFile::modeWrite);
            SafeClose(f);
        }
        CFileFind finder;
        BOOL working = finder.FindFile(dir + CString(_T("*.seq")));
        LineBool("CFileFind.FindFile.wildcard_found", working != FALSE);
        int seen = 0;
        int lastReturn = -1;
        while (working)
        {
            working = finder.FindNextFile();
            lastReturn = working ? 1 : 0;
            ++seen;
            if (seen > 16) break;
        }
        LineInt("CFileFind.FindNextFile.iterations", seen);
        LineInt("CFileFind.FindNextFile.last_return", lastReturn);
        finder.Close();
        for (const TCHAR* n : extra) SafeRemoveFile(dir + CString(n));
    }

    SafeRemoveFile(plain);
    RemoveDirectoryW(dir);
}

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

namespace
{
std::atomic<int> g_lifecycleRan{0};

UINT AFX_CDECL LifecycleWorker(LPVOID pParam)
{
    g_lifecycleRan.store(pParam ? *static_cast<int*>(pParam) : -1);
    return 3;
}
}

static void TestCWinThreadLifecycle()
{
    g_lifecycleRan.store(0);
    static int marker = 99;

    CWinThread* pThread = new CWinThread(LifecycleWorker, &marker);
    pThread->m_bAutoDelete = FALSE;
    LineBool("CWinThread.CreateThread.suspended",
             pThread->CreateThread(CREATE_SUSPENDED) != FALSE);
    LineBool("CWinThread.CreateThread.sets_m_hThread", pThread->m_hThread != nullptr);
    LineBool("CWinThread.CreateThread.has_not_run_yet", g_lifecycleRan.load() == 0);

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

    {
        CWinThread idle;
        idle.m_bAutoDelete = FALSE;
        LineBool("CWinThread.InitInstance.default", idle.InitInstance() != FALSE);
        LineInt("CWinThread.ExitInstance.default", idle.ExitInstance());
        idle.Delete();
        LineBool("CWinThread.Delete.without_autodelete_is_a_noop",
                 idle.m_bAutoDelete == FALSE);
    }
}

namespace
{
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
}

static void TestCAsyncSocketDatagram()
{
    LineBool("AfxSocketInit.before_datagram", AfxSocketInit(nullptr) != FALSE);

    SOCKET raw = ::socket(AF_INET, SOCK_DGRAM, 0);
    LineBool("CAsyncSocket.Bind.raw_socket_available", raw != INVALID_SOCKET);

    CAsyncSocket receiver;
    LineBool("CAsyncSocket.Bind.Attach", receiver.Attach(raw) != FALSE);
    LineBool("CAsyncSocket.Bind.to_loopback_ephemeral",
             receiver.Bind(0, _T("127.0.0.1")) != FALSE);

    CString boundAddress;
    UINT boundPort = 0;
    LineBool("CAsyncSocket.Bind.GetSockName_after_Bind",
             receiver.GetSockName(boundAddress, boundPort) != FALSE);
    Line("CAsyncSocket.Bind.address", boundAddress);
    LineBool("CAsyncSocket.Bind.port_is_assigned", boundPort != 0);

    LineBool("CAsyncSocket.Bind.second_bind_fails", receiver.Bind(0, _T("127.0.0.1")) != FALSE);

    CAsyncSocket sender;
    LineBool("CAsyncSocket.SendTo.sender_created",
             sender.Create(0, SOCK_DGRAM, FD_READ | FD_WRITE, _T("127.0.0.1")) != FALSE);

    const char* payloads[] = {"a", "datagram", "0123456789012345678901234567890123456789"};
    int idx = 0;
    for (const char* payload : payloads)
    {
        const int len = static_cast<int>(std::strlen(payload));
        const int sent = sender.SendTo(payload, len, boundPort, _T("127.0.0.1"));
        LineInt(("CAsyncSocket.SendTo." + std::to_string(idx) + ".returns_length").c_str(), sent);

        char buf[128]{};
        int got = -1;
        if (idx == 0)
        {
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

    {
        NotificationProbe probe;
        const int codes[] = {0, 10035  , -1};
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

namespace
{
class GrowableMemFile : public CMemFile
{
public:
    using CMemFile::GrowFile;
};
}

static void TestRemainingGaps()
{
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

    {
        const INT_PTR hints[] = {1, 17, 1024};
        for (INT_PTR hint : hints)
        {
            CMapStringToPtr m(hint);
            int a = 1, b = 2;
            m.SetAt(_T("first"), &a);
            m.SetAt(_T("second"), &b);
            void* found = nullptr;
            const std::string tag = std::to_string(static_cast<long long>(hint));
            LineBool(("CMapStringToPtr.sized_ctor." + tag + ".lookup").c_str(),
                     m.Lookup(_T("second"), found) != FALSE);
            LineBool(("CMapStringToPtr.sized_ctor." + tag + ".value").c_str(), found == &b);
            LineInt(("CMapStringToPtr.sized_ctor." + tag + ".count").c_str(),
                    static_cast<long long>(m.GetCount()));

            CMapStringToString ms(hint);
            ms.SetAt(_T("key"), _T("value"));
            CString out;
            LineBool(("CMapStringToString.sized_ctor." + tag + ".lookup").c_str(),
                     ms.Lookup(_T("key"), out) != FALSE);
            Line(("CMapStringToString.sized_ctor." + tag + ".value").c_str(), out);
        }
    }

    {
        LineInt("AtlGetConversionACP.value", static_cast<long long>(_AtlGetConversionACP()));
        LineBool("AtlGetConversionACP.is_stable",
                 _AtlGetConversionACP() == _AtlGetConversionACP());
    }

    {
        const TCHAR* keys[] = {_T(""), _T("a"), _T("abc"), _T("a longer key value"), _T("éè")};
        int i = 0;
        for (const TCHAR* k : keys)
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

namespace
{
template <class F>
std::string FileOutcome(F&& fn)
{
    try
    {
        fn();
        return "no-exception";
    }
    catch (CFileException* e)
    {
        const std::string cause = "CFileException.cause=" + std::to_string(e->m_cause);
        e->Delete();
        return cause;
    }
    catch (...)
    {
        return "other-exception";
    }
}
}

static void TestCStringBufferSemantics()
{
    {
        CString s(_T("abc"));
        LPTSTR p = s.GetBuffer(32);
        LineBool("CString.GetBuffer.returns_nonnull", p != nullptr);
        LineInt("CString.GetBuffer.length_unchanged_while_checked_out", s.GetLength());
        Line("CString.GetBuffer.buffer_holds_content", CString(p));
        s.ReleaseBuffer();
        LineInt("CString.GetBuffer.length_after_release", s.GetLength());
        Line("CString.GetBuffer.value_after_release", s);
    }

    {
        CString s;
        LPTSTR p = s.GetBuffer(8);
        LineBool("CString.GetBuffer.empty.returns_nonnull", p != nullptr);
        LineInt("CString.GetBuffer.empty.length_while_checked_out", s.GetLength());
        s.ReleaseBuffer();
        LineInt("CString.GetBuffer.empty.length_after_release", s.GetLength());
        LineBool("CString.GetBuffer.empty.IsEmpty_after_release", s.IsEmpty() != FALSE);
    }

    {
        CString s(_T("keepme"));
        LPTSTR p = s.GetBuffer(0);
        Line("CString.GetBuffer.zero_request.content", CString(p));
        LineInt("CString.GetBuffer.zero_request.length", s.GetLength());
        s.ReleaseBuffer();
        LineInt("CString.GetBuffer.zero_request.length_after_release", s.GetLength());
        Line("CString.GetBuffer.zero_request.value_after_release", s);
    }

    {
        CString s(_T("small"));
        LPTSTR p = s.GetBuffer(4096);
        Line("CString.GetBuffer.large_growth.content", CString(p));
        s.ReleaseBuffer();
        Line("CString.GetBuffer.large_growth.value", s);
        LineInt("CString.GetBuffer.large_growth.length", s.GetLength());
    }

    {
        CString a(_T("shared-value"));
        CString b(a);
        CString c;
        c = a;
        LPTSTR p = a.GetBuffer(a.GetLength());
        p[0] = _T('X');
        a.ReleaseBuffer();
        Line("CString.GetBuffer.copy_ctor_unaffected", b);
        Line("CString.GetBuffer.assigned_copy_unaffected", c);
        Line("CString.GetBuffer.original_modified", a);
        LineInt("CString.GetBuffer.copy_ctor_unaffected.length", b.GetLength());
        LineInt("CString.GetBuffer.original_modified.length", a.GetLength());
    }

    {
        CString s;
        LPTSTR p = s.GetBuffer(16);
        for (int i = 0; i < 10; ++i)
            p[i] = static_cast<TCHAR>(_T('a') + i);
        p[10] = _T('\0');
        s.ReleaseBuffer(4);
        LineInt("CString.ReleaseBuffer.explicit_length.length", s.GetLength());
        Line("CString.ReleaseBuffer.explicit_length.value", s);
        LineBool("CString.ReleaseBuffer.explicit_length.nul_terminated",
                 s.GetString()[4] == _T('\0'));
    }

    {
        CString s;
        LPTSTR p = s.GetBuffer(16);
        p[0] = _T('a');
        p[1] = _T('b');
        p[2] = _T('\0');
        p[3] = _T('c');
        p[4] = _T('\0');
        s.ReleaseBuffer();
        LineInt("CString.ReleaseBuffer.stops_at_embedded_nul.length", s.GetLength());
        Line("CString.ReleaseBuffer.stops_at_embedded_nul.value", s);
    }

    {
        CString s;
        LPTSTR p = s.GetBuffer(24);
        wcscpy_s(p, 24, _T("filled-by-c-api"));
        s.ReleaseBuffer();
        LineInt("CString.GetBuffer.c_api_fill.length", s.GetLength());
        Line("CString.GetBuffer.c_api_fill.value", s);
        LineBool("CString.GetBuffer.c_api_fill.roundtrips",
                 s == CString(_T("filled-by-c-api")));
    }

    {
        CString s(_T("no-release"));
        LPTSTR p = s.GetBuffer(64);
        (void)p;
        LineInt("CString.GetBuffer.without_release.length", s.GetLength());
        Line("CString.GetBuffer.without_release.value", s);
        LineBool("CString.GetBuffer.without_release.compares_equal",
                 s == CString(_T("no-release")));
    }

    {
        CString s(_T("reused"));
        LPTSTR first = s.GetBuffer(32);
        (void)first;
        s.ReleaseBuffer();
        LPTSTR second = s.GetBuffer(32);
        wcscpy_s(second, 32, _T("second-round"));
        s.ReleaseBuffer();
        Line("CString.GetBuffer.reuse.value", s);
        LineInt("CString.GetBuffer.reuse.length", s.GetLength());
    }
}

static void TestCFileOpenFlags()
{
    LineInt("CFile.OpenFlags.modeRead", CFile::modeRead);
    LineInt("CFile.OpenFlags.modeWrite", CFile::modeWrite);
    LineInt("CFile.OpenFlags.modeReadWrite", CFile::modeReadWrite);
    LineInt("CFile.OpenFlags.shareCompat", CFile::shareCompat);
    LineInt("CFile.OpenFlags.shareExclusive", CFile::shareExclusive);
    LineInt("CFile.OpenFlags.shareDenyWrite", CFile::shareDenyWrite);
    LineInt("CFile.OpenFlags.shareDenyRead", CFile::shareDenyRead);
    LineInt("CFile.OpenFlags.shareDenyNone", CFile::shareDenyNone);
    LineInt("CFile.OpenFlags.modeNoInherit", CFile::modeNoInherit);
    LineInt("CFile.OpenFlags.modeCreate", CFile::modeCreate);
    LineInt("CFile.OpenFlags.modeNoTruncate", CFile::modeNoTruncate);
    LineInt("CFile.OpenFlags.typeText", CFile::typeText);
    LineInt("CFile.OpenFlags.typeBinary", CFile::typeBinary);
    LineInt("CFile.OpenFlags.osNoBuffer", CFile::osNoBuffer);
    LineInt("CFile.OpenFlags.osWriteThrough", CFile::osWriteThrough);
    LineInt("CFile.OpenFlags.osRandomAccess", CFile::osRandomAccess);
    LineInt("CFile.OpenFlags.osSequentialScan", CFile::osSequentialScan);

    LineBool("CFile.OpenFlags.share_is_a_field_not_bits",
             (CFile::shareDenyRead & CFile::shareDenyWrite) == CFile::shareDenyWrite);
    LineInt("CFile.OpenFlags.share_field_mask",
            CFile::shareExclusive | CFile::shareDenyWrite | CFile::shareDenyRead
                | CFile::shareDenyNone);
}

static void TestCFileSharing()
{
    const CString dir = TempDir();
    const CString path = dir + CString(_T("simple_mfc_share_4711.bin"));
    {
        CFile seed;
        seed.Open(path, CFile::modeCreate | CFile::modeWrite | CFile::shareDenyNone);
        const char payload[] = "share";
        seed.Write(payload, sizeof(payload) - 1);
        SafeClose(seed);
    }

    struct Combo { const char* label; UINT first; UINT second; };
    const Combo combos[] = {
        {"default_then_default",        CFile::modeRead,                          CFile::modeRead},
        {"exclusive_then_read",         CFile::modeRead | CFile::shareExclusive,  CFile::modeRead | CFile::shareDenyNone},
        {"denyNone_then_read",          CFile::modeRead | CFile::shareDenyNone,   CFile::modeRead | CFile::shareDenyNone},
        {"denyNone_then_write",         CFile::modeRead | CFile::shareDenyNone,   CFile::modeWrite | CFile::shareDenyNone},
        {"denyWrite_then_read",         CFile::modeRead | CFile::shareDenyWrite,  CFile::modeRead | CFile::shareDenyNone},
        {"denyWrite_then_write",        CFile::modeRead | CFile::shareDenyWrite,  CFile::modeWrite | CFile::shareDenyNone},
        {"denyRead_then_read",          CFile::modeWrite | CFile::shareDenyRead,  CFile::modeRead | CFile::shareDenyNone},
        {"denyRead_then_write",         CFile::modeWrite | CFile::shareDenyRead,  CFile::modeWrite | CFile::shareDenyNone},
        {"denyNone_then_exclusive",     CFile::modeRead | CFile::shareDenyNone,   CFile::modeRead | CFile::shareExclusive},
        {"denyNone_then_denyWrite",     CFile::modeRead | CFile::shareDenyNone,   CFile::modeRead | CFile::shareDenyWrite},
        {"writer_denyNone_then_denyWrite", CFile::modeWrite | CFile::shareDenyNone, CFile::modeRead | CFile::shareDenyWrite},
    };

    for (const Combo& c : combos)
    {
        CFile first;
        const BOOL firstOk = first.Open(path, c.first);
        CFile second;
        CFileException ex;
        const BOOL secondOk = second.Open(path, c.second, &ex);

        const std::string tag = std::string("CFile.Share.") + c.label;
        LineBool((tag + ".first_opens").c_str(), firstOk != FALSE);
        LineBool((tag + ".second_opens").c_str(), secondOk != FALSE);
        if (!secondOk)
            LineInt((tag + ".second_cause").c_str(), ex.m_cause);

        if (secondOk) SafeClose(second);
        if (firstOk) SafeClose(first);

        CFile afterClose;
        const BOOL reopened = afterClose.Open(path, c.second);
        LineBool((tag + ".reopens_after_close").c_str(), reopened != FALSE);
        if (reopened) SafeClose(afterClose);
    }

    SafeRemoveFile(path);
}

static void TestCStdioFileTextMode()
{
    const CString dir = TempDir();

    struct Mode { const char* label; UINT extra; };
    const Mode modes[] = {{"binary", CFile::typeBinary}, {"text", CFile::typeText}};

    for (const Mode& m : modes)
    {
        const CString path = dir + CString(_T("simple_mfc_textmode_"))
                           + CString(m.label, static_cast<int>(std::strlen(m.label))) + CString(_T(".txt"));
        {
            CStdioFile out;
            out.Open(path, CFile::modeCreate | CFile::modeWrite | CFile::shareDenyNone | m.extra);
            out.WriteString(_T("alpha\n"));
            out.WriteString(_T("beta\n"));
            SafeClose(out);
        }

        const std::string tag = std::string("CStdioFile.") + m.label;

        {
            CFile raw;
            raw.Open(path, CFile::modeRead | CFile::shareDenyNone);
            LineInt((tag + ".bytes_on_disk").c_str(), static_cast<long long>(raw.GetLength()));
            char bytes[64]{};
            const UINT n = raw.Read(bytes, sizeof(bytes));
            Line((tag + ".raw").c_str(), Hex(bytes, n));
            SafeClose(raw);
        }

        {
            CStdioFile in;
            in.Open(path, CFile::modeRead | CFile::shareDenyNone | m.extra);
            CString line;
            int i = 0;
            while (in.ReadString(line))
            {
                Line((tag + ".line" + std::to_string(i)).c_str(), line);
                LineInt((tag + ".line" + std::to_string(i) + ".length").c_str(), line.GetLength());
                ++i;
            }
            LineInt((tag + ".lines").c_str(), i);
            SafeClose(in);
        }

        {
            CStdioFile in;
            in.Open(path, CFile::modeRead | CFile::shareDenyNone | m.extra);
            TCHAR buf[64]{};
            LPTSTR got = in.ReadString(buf, 64);
            LineBool((tag + ".buffered.non_null").c_str(), got != nullptr);
            Line((tag + ".buffered.first").c_str(), CString(buf));
            LineInt((tag + ".buffered.first.length").c_str(),
                    static_cast<long long>(std::char_traits<TCHAR>::length(buf)));
            SafeClose(in);
        }

        SafeRemoveFile(path);
    }
}

static void TestCFileErrorPaths()
{
    const CString dir = TempDir();
    const CString missing = dir + CString(_T("simple_mfc_absent_4711.bin"));
    SafeRemoveFile(missing);

    {
        CFile f;
        CFileException ex;
        const BOOL ok = f.Open(missing, CFile::modeRead, &ex);
        LineBool("CFile.Open.missing.returns_false", ok == FALSE);
        LineInt("CFile.Open.missing.m_cause", ex.m_cause);
        LineBool("CFile.Open.missing.m_lOsError_is_real", ex.m_lOsError > 0);
        LineBool("CFile.Open.missing.m_strFileName_set", ex.m_strFileName.IsEmpty() == FALSE);
    }

    Line("CFile.Open.missing.without_pError_does_not_throw",
         FileOutcome([&] { CFile f; f.Open(missing, CFile::modeRead); }));

    {
        const CString badPath =
            dir + CString(_T("simple_mfc_absent_dir_4711")) + CString(SMFC_SEP) + CString(_T("child.bin"));
        CFile f;
        CFileException ex;
        const BOOL ok = f.Open(badPath, CFile::modeCreate | CFile::modeWrite, &ex);
        LineBool("CFile.Open.missing_directory.returns_false", ok == FALSE);
        LineInt("CFile.Open.missing_directory.m_cause", ex.m_cause);
    }

    {
        CFile f;
        CFileException ex;
        const BOOL ok = f.Open(dir, CFile::modeRead, &ex);
        LineBool("CFile.Open.directory.returns_false", ok == FALSE);
        LineInt("CFile.Open.directory.m_cause", ex.m_cause);
        if (ok) SafeClose(f);
    }

    Line("CFile.ctor.missing_throws",
         FileOutcome([&] { CFile f(missing, CFile::modeRead); (void)f.GetPosition(); }));

    Line("CFile.Remove.missing_throws", FileOutcome([&] { CFile::Remove(missing); }));

    {
        const CString target = dir + CString(_T("simple_mfc_rename_target_4711.bin"));
        Line("CFile.Rename.missing_throws",
             FileOutcome([&] { CFile::Rename(missing, target); }));
        SafeRemoveFile(target);
    }

    {
        CFileStatus st{};
        LineBool("CFile.GetStatus.missing.returns_false",
                 CFile::GetStatus(missing, st) == FALSE);
    }

    const CString path = dir + CString(_T("simple_mfc_errpaths_4711.bin"));
    {
        CFile f;
        f.Open(path, CFile::modeCreate | CFile::modeWrite);
        const char payload[] = "0123";
        f.Write(payload, sizeof(payload) - 1);
        SafeClose(f);
    }

    {
        CFile f;
        const BOOL ok = f.Open(path, CFile::modeRead);
        LineBool("CFile.Open.existing.returns_true", ok != FALSE);
        LineBool("CFile.GetFilePath.nonempty_after_Open", f.GetFilePath().IsEmpty() == FALSE);

        char buf[16]{};
        LineInt("CFile.Read.more_than_available.count",
                static_cast<long long>(f.Read(buf, sizeof(buf))));
        LineInt("CFile.Read.at_eof.count", static_cast<long long>(f.Read(buf, sizeof(buf))));
        Line("CFile.Read.at_eof.does_not_throw",
             FileOutcome([&] { char again[4]; f.Read(again, sizeof(again)); }));
        LineInt("CFile.Read.at_eof.position", static_cast<long long>(f.GetPosition()));
        LineInt("CFile.Read.at_eof.GetLength", static_cast<long long>(f.GetLength()));
        f.SeekToBegin();
        LineInt("CFile.Read.at_eof.seek_recovers_position",
                static_cast<long long>(f.GetPosition()));
        LineInt("CFile.Read.after_recovery.count", static_cast<long long>(f.Read(buf, 2)));
        SafeClose(f);
    }

    {
        CFile f;
        f.Open(path, CFile::modeRead);
        Line("CFile.Write.on_read_only_handle",
             FileOutcome([&] { const char x = 'x'; f.Write(&x, 1); }));
        SafeClose(f);
    }

    {
        CFile f;
        f.Open(path, CFile::modeRead);
        f.SeekToBegin();
        Line("CFile.Seek.before_begin", FileOutcome([&] { f.Seek(-8, CFile::current); }));
        SafeClose(f);
    }

    {
        CFile f;
        f.Open(path, CFile::modeRead);
        LineInt("CFile.Seek.beyond_end.position",
                static_cast<long long>(f.Seek(4096, CFile::begin)));
        LineInt("CFile.Seek.beyond_end.GetLength", static_cast<long long>(f.GetLength()));
        char b[4]{};
        LineInt("CFile.Read.beyond_end.count", static_cast<long long>(f.Read(b, sizeof(b))));
        SafeClose(f);
    }

    {
        MakeReadOnly(path);
        CFile f;
        CFileException ex;
        const BOOL ok = f.Open(path, CFile::modeWrite, &ex);
        LineBool("CFile.Open.readonly_file_for_write.returns_false", ok == FALSE);
        LineInt("CFile.Open.readonly_file_for_write.m_cause", ex.m_cause);
        if (ok) SafeClose(f);
        MakeWritable(path);
    }

    SafeRemoveFile(path);
}

static void TestAfxSocketTerm()
{
    AfxSocketTerm();
    LineBool("AfxSocketTerm.returns", true);
    LineBool("AfxSocketTerm.reinit_after_term", AfxSocketInit(nullptr) != FALSE);
    AfxSocketTerm();
    LineBool("AfxSocketTerm.second_term_returns", true);
}

#ifdef _DEBUG
namespace
{
class DumpBuffer
{
public:
#if defined(SIMPLE_MFC_USE_REAL_MFC)
    CDumpContext Context() { return CDumpContext(&m_file); }
    std::string Take()
    {
        m_file.Flush();
        const ULONGLONG bytes = m_file.GetLength();
        if (bytes == 0) return {};
        std::basic_string<TCHAR> text(static_cast<size_t>(bytes / sizeof(TCHAR)), _T('\0'));
        m_file.SeekToBegin();
        m_file.Read(&text[0], static_cast<UINT>(bytes));
        return Utf8(text.c_str());
    }

private:
    CMemFile m_file;
#else
    CDumpContext Context() { return CDumpContext(m_stream); }
    std::string Take() { return Utf8(m_stream.str().c_str()); }

private:
    std::wostringstream m_stream;
#endif
};

class DumpSubject : public CObject
{
    DECLARE_DYNAMIC(DumpSubject)
public:
    int value = 5;
};
IMPLEMENT_DYNAMIC(DumpSubject, CObject)

int g_assertValidCalls = 0;

class AssertValidSubject : public CObject
{
    DECLARE_DYNAMIC(AssertValidSubject)
public:
    void AssertValid() const override
    {
        ++g_assertValidCalls;
        CObject::AssertValid();
    }
};
IMPLEMENT_DYNAMIC(AssertValidSubject, CObject)

class DerivedSubject : public AssertValidSubject
{
    DECLARE_DYNAMIC(DerivedSubject)
};
IMPLEMENT_DYNAMIC(DerivedSubject, AssertValidSubject)
}

static void TestDebugOnly()
{
    {
        AssertValidSubject subject;
        g_assertValidCalls = 0;
        ASSERT_VALID(&subject);
        LineInt("ASSERT_VALID.reaches_AssertValid", g_assertValidCalls);

        g_assertValidCalls = 0;
        const AssertValidSubject constSubject;
        ASSERT_VALID(&constSubject);
        LineInt("ASSERT_VALID.reaches_AssertValid.const", g_assertValidCalls);

        g_assertValidCalls = 0;
        DerivedSubject derived;
        CObject* asBase = &derived;
        ASSERT_VALID(asBase);
        LineInt("ASSERT_VALID.dispatches_virtually", g_assertValidCalls);

        g_assertValidCalls = 0;
        ASSERT_VALID(&subject);
        ASSERT_VALID(&subject);
        ASSERT_VALID(&subject);
        LineInt("ASSERT_VALID.once_per_invocation", g_assertValidCalls);

        ASSERT_KINDOF(CObject, &derived);
        LineBool("ASSERT_KINDOF.base_of_derived_passes", true);
        ASSERT_KINDOF(AssertValidSubject, &derived);
        LineBool("ASSERT_KINDOF.exact_type_passes", true);
        ASSERT_KINDOF(DerivedSubject, &derived);
        LineBool("ASSERT_KINDOF.most_derived_passes", true);
    }

    {
        DumpSubject subject;
        subject.AssertValid();
        LineBool("CObject.AssertValid.plain_object_returns", true);

        CFileException fe(CFileException::none, 0, _T("x"));
        fe.AssertValid();
        LineBool("CObject.AssertValid.library_object_returns", true);

        CString s(_T("value"));
        CFile file;
        file.AssertValid();
        LineBool("CObject.AssertValid.CFile_returns", true);
    }

    {
        DumpBuffer a;
        CDumpContext dca = a.Context();
        DumpSubject subject;
        subject.Dump(dca);
        const std::string dumpedSubject = a.Take();

        DumpBuffer b;
        CDumpContext dcb = b.Context();
        CFileException fe(CFileException::none, 0, _T("x"));
        fe.Dump(dcb);
        const std::string dumpedException = b.Take();

        const std::string prefix = "a DumpSubject at $";
        LineBool("CObject.Dump.prefix",
                 dumpedSubject.compare(0, prefix.size(), prefix) == 0);
        LineBool("CObject.Dump.ends_with_newline",
                 !dumpedSubject.empty() && dumpedSubject.back() == '\n');
        LineInt("CObject.Dump.address_digits",
                static_cast<long long>(dumpedSubject.size()) -
                    static_cast<long long>(prefix.size()) - 1);
        LineBool("CObject.Dump.differs_by_class", dumpedSubject != dumpedException);
    }

    {
        const int depths[] = {0, 1, 7, -1};
        int i = 0;
        for (int d : depths)
        {
            DumpBuffer buf;
            CDumpContext dc = buf.Context();
            dc.SetDepth(d);
            LineInt(("CDumpContext.SetDepth." + std::to_string(i)).c_str(), dc.GetDepth());
            ++i;
        }
    }

    {
        const TCHAR* wides[] = {_T(""), _T("wide"), _T("with spaces"), _T("éè")};
        int i = 0;
        for (const TCHAR* w : wides)
        {
            DumpBuffer buf;
            CDumpContext dc = buf.Context();
            dc << w;
            Line(("CDumpContext.insert.LPCTSTR." + std::to_string(i)).c_str(), buf.Take());
            ++i;
        }
    }
    {
        const char* narrows[] = {"", "narrow", "0123456789"};
        int i = 0;
        for (const char* n : narrows)
        {
            DumpBuffer buf;
            CDumpContext dc = buf.Context();
            dc << n;
            Line(("CDumpContext.insert.LPCSTR." + std::to_string(i)).c_str(), buf.Take());
            ++i;
        }
    }
    {
        const int ints[] = {0, 1, -1, 2147483647, -2147483647};
        int i = 0;
        for (int v : ints)
        {
            DumpBuffer buf;
            CDumpContext dc = buf.Context();
            dc << v;
            Line(("CDumpContext.insert.int." + std::to_string(i)).c_str(), buf.Take());
            ++i;
        }
    }
    {
        const unsigned int uints[] = {0u, 7u, 4294967295u};
        int i = 0;
        for (unsigned int v : uints)
        {
            DumpBuffer buf;
            CDumpContext dc = buf.Context();
            dc << v;
            Line(("CDumpContext.insert.uint." + std::to_string(i)).c_str(), buf.Take());
            ++i;
        }
    }
    {
        const long longs[] = {0L, -12345L, 2147483647L};
        int i = 0;
        for (long v : longs)
        {
            DumpBuffer buf;
            CDumpContext dc = buf.Context();
            dc << v;
            Line(("CDumpContext.insert.long." + std::to_string(i)).c_str(), buf.Take());
            ++i;
        }
    }
    {
        const double doubles[] = {0.0, 1.5, -0.25, 1234.5};
        int i = 0;
        for (double v : doubles)
        {
            DumpBuffer buf;
            CDumpContext dc = buf.Context();
            dc << v;
            Line(("CDumpContext.insert.double." + std::to_string(i)).c_str(), buf.Take());
            ++i;
        }
    }
    {
        DumpBuffer buf;
        CDumpContext dc = buf.Context();
        int local = 0;
        dc << static_cast<const void*>(&local);
        LineBool("CDumpContext.insert.void_pointer.non_empty", !buf.Take().empty());
    }
    {
        DumpSubject subject;

        DumpBuffer byRef;
        CDumpContext dcRef = byRef.Context();
        dcRef << subject;
        LineBool("CDumpContext.insert.CObject_ref.non_empty", !byRef.Take().empty());

        DumpBuffer byPtr;
        CDumpContext dcPtr = byPtr.Context();
        dcPtr << &subject;
        LineBool("CDumpContext.insert.CObject_ptr.non_empty", !byPtr.Take().empty());

        DumpBuffer nul;
        CDumpContext dcNul = nul.Context();
        dcNul << static_cast<const CObject*>(nullptr);
        Line("CDumpContext.insert.CObject_null.text", nul.Take());
    }
}
#endif

static void TestAliasingAndIdentity()
{
    {
        CString original(_T("aliasing subject"));
        CString copy(original);
        CString assigned;
        assigned = original;
        copy += _T("!");
        LineBool("Aliasing.CString.mutate_detaches",
                 original.GetString() != copy.GetString());
        Line("Aliasing.CString.original_unchanged", original);
        Line("Aliasing.CString.copy_after_mutation", copy);
    }

    {
        CString s(_T("abc"));
        LPTSTR buffer = s.GetBuffer(16);
        s.ReleaseBuffer();
        Line("Aliasing.ReleaseBuffer.pointer_content", CString(buffer));
    }

    {
        CString shared(_T("cow subject"));
        CString twin(shared);
        LPTSTR buffer = twin.GetBuffer(32);
        buffer[0] = _T('C');
        twin.ReleaseBuffer();
        Line("Aliasing.GetBuffer.on_shared_copy.original", shared);
        Line("Aliasing.GetBuffer.on_shared_copy.twin", twin);
    }

    {
        typedef CMap<CString, LPCTSTR, int, int> TypedMap;
        TypedMap map;
        map.SetAt(_T("k"), 1);
        if (TypedMap::CPair* pair = map.PLookup(_T("k")))
            pair->value = 42;
        int readBack = 0;
        map.Lookup(_T("k"), readBack);
        LineInt("Aliasing.CMap.CPair.write_through", readBack);

        map[_T("idx")] = 7;
        int viaIndex = 0;
        map.Lookup(_T("idx"), viaIndex);
        LineInt("Aliasing.CMap.operator_index.write_through", viaIndex);
    }

    {
        typedef CRBMap<ULONGLONG, DWORD> TypedRBMap;
        TypedRBMap map;
        map.SetAt(1, 100);
        POSITION pos = map.GetHeadPosition();
        if (TypedRBMap::CPair* pair = map.GetNext(pos))
            pair->m_value = 999;
        POSITION head = map.GetHeadPosition();
        LineInt("Aliasing.CRBMap.CPair.write_through",
                static_cast<long long>(map.GetValueAt(head)));
    }

    {
        CArray<int, int> values;
        values.SetSize(3);
        values.SetAt(0, 1);
        int* data = values.GetData();
        values.SetAt(0, 7);
        LineInt("Aliasing.CArray.GetData.sees_SetAt", data[0]);
        values.ElementAt(1) = 5;
        LineInt("Aliasing.CArray.ElementAt.write_through", values.GetAt(1));
        values[2] = 9;
        LineInt("Aliasing.CArray.operator_index.write_through", values.GetAt(2));
    }

    {
        CStringList list;
        list.AddTail(_T("first"));
        list.AddTail(_T("second"));
        POSITION pos = list.GetHeadPosition();
        list.GetAt(pos) = _T("rewritten");
        Line("Aliasing.CStringList.GetAt.write_through", list.GetHead());
    }

    {
        CPtrList list;
        int one = 1;
        int two = 2;
        list.AddTail(&one);
        POSITION pos = list.GetHeadPosition();
        list.GetAt(pos) = &two;
        LineInt("Aliasing.CPtrList.GetAt.write_through",
                *static_cast<int*>(list.GetHead()));
    }
}

int main()
{
    SilenceWindowsDialogs();

    TestRTTI();
    TestCRuntimeClass();
    TestExceptions();
    TestExceptionGaps();
    TestCString();
    TestCStringGaps();
    TestNonBMP();
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
    TestCPointCSize();
    TestCRectMethods();
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
    TestPatternCRectAndPoint();
    TestPatternCTime();
    TestPatternBase64();
    TestPatternUnicodeToUtf8();
    TestCStringBufferSemantics();
    TestCFileOpenFlags();
    TestCFileSharing();
    TestCStdioFileTextMode();
    TestCFileErrorPaths();
    TestAliasingAndIdentity();
    TestRemainingGaps();

#ifdef _DEBUG
    TestDebugOnly();
#endif

    TestAfxSocketTerm();

    Line("#END", std::to_string(g_index));
    return 0;
}
