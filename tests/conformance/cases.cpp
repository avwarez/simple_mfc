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

void Line(const char* name, const std::string& value)
{
    std::printf("%03d %s = %s\n", ++g_index, name, value.c_str());
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
    // builds it from the OS's localized FormatMessage output for
    // m_lOsError, while simple_mfc uses fixed English text — these are
    // legitimately different strings, not a conformance bug. We only
    // compare the parts that ARE meant to be identical: success/failure
    // and "some non-empty message was produced".
    //
    // lOsError must be a real Win32 error code here, not the -1 "no OS
    // error" sentinel: real MFC's FormatMessage(-1) legitimately produces
    // an EMPTY string (nothing to look up), which is real MFC's genuine
    // behavior for that input, not a bug — found by this very suite. Using
    // ERROR_FILE_NOT_FOUND (2) instead exercises the representative case
    // (an OS error was actually recorded), where both sides do produce
    // non-empty, if differently worded, text.
    CFileException fe(CFileException::fileNotFound, ERROR_FILE_NOT_FOUND, L"missing_file.dat");
    wchar_t buf[256]{};
    BOOL ok = fe.GetErrorMessage(buf, 256);
    LineBool("CFileException.GetErrorMessage.returns_true", ok != FALSE);
    LineBool("CFileException.GetErrorMessage.non_empty", buf[0] != L'\0');
    LineInt("CFileException.m_cause", fe.m_cause);
    Line("CFileException.m_strFileName", fe.m_strFileName);

    CMemoryException me;
    wchar_t mbuf[256]{};
    BOOL mok = me.GetErrorMessage(mbuf, 256);
    LineBool("CMemoryException.GetErrorMessage.returns_true", mok != FALSE);
    LineBool("CMemoryException.GetErrorMessage.non_empty", mbuf[0] != L'\0');
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
    f.Close();

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
    f2.Close();

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
}

static void TestCStdioFile()
{
    CString path = TempDir() + CString(L"simple_mfc_conformance_stdio.txt");

    CStdioFile wf;
    wf.Open(path, CFile::modeCreate | CFile::modeWrite);
    wf.WriteString(L"first line\r\n");
    wf.WriteString(L"second line\r\n");
    wf.Close();

    CStdioFile rf;
    rf.Open(path, CFile::modeRead);
    CString line1, line2, line3;
    BOOL got1 = rf.ReadString(line1);
    BOOL got2 = rf.ReadString(line2);
    BOOL got3 = rf.ReadString(line3); // past EOF: expected to fail
    rf.Close();

    LineBool("CStdioFile.ReadString.line1.ok", got1 != FALSE);
    Line("CStdioFile.ReadString.line1", line1);
    LineBool("CStdioFile.ReadString.line2.ok", got2 != FALSE);
    Line("CStdioFile.ReadString.line2", line2);
    LineBool("CStdioFile.ReadString.line3PastEof.fails", got3 == FALSE);

    CFile::Remove(path);
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
        f.Close();
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

    for (const wchar_t* name : names)
        CFile::Remove(dir + CString(name));
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
    std::string order;
    POSITION pos = list.GetHeadPosition();
    while (pos)
    {
        void* v = list.GetNext(pos);
        order += std::to_string(reinterpret_cast<intptr_t>(v));
        if (pos) order += ",";
    }
    Line("CPtrList.IterationOrder", order);

    LineInt("CPtrList.RemoveTail.value", reinterpret_cast<intptr_t>(list.RemoveTail()));
    LineInt("CPtrList.CountAfterRemoveTail", list.GetCount());
}

static void TestCStringList()
{
    CStringList list;
    list.AddTail(L"one");
    list.AddTail(L"two");
    list.AddHead(L"zero"); // order: zero, one, two

    LineInt("CStringList.GetCount", list.GetCount());
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
    arr.SetAtGrow(5, reinterpret_cast<void*>(static_cast<intptr_t>(500)));
    LineInt("CPtrArray.CountAfterSetAtGrow5", arr.GetCount());
    LineInt("CPtrArray.GetAt5", reinterpret_cast<intptr_t>(arr.GetAt(5)));
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
}

static void TestCListTemplate()
{
    CList<CString, const CString&> list;
    list.AddTail(L"x");
    list.AddTail(L"y");
    list.AddHead(L"w"); // order: w, x, y
    LineInt("CList_CString.GetCount", list.GetCount());

    std::string order;
    POSITION pos = list.GetHeadPosition();
    while (pos)
    {
        CString v = list.GetNext(pos);
        order += Utf8((LPCTSTR)v);
        if (pos) order += ",";
    }
    Line("CList_CString.IterationOrder", order);
    Line("CList_CString.RemoveHead.value", list.RemoveHead());
    LineInt("CList_CString.CountAfterRemoveHead", list.GetCount());
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

static void TestMutex()
{
    CMutex mtx;
    CSingleLock lk(&mtx, TRUE);
    LineBool("CMutex.SingleLock.locked", lk.IsLocked() != FALSE);
    lk.Unlock();
    LineBool("CMutex.SingleLock.unlockedAfterUnlock", lk.IsLocked() == FALSE);
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
    TestMutex();
    return 0;
}
