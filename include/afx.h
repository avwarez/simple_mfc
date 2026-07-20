// afx.h — part of simple_mfc. NATIVE implementation (standard C++17
// library only): same class/method names as real MFC, but with working,
// portable bodies (no Windows headers, no GUI, no PE resources). The
// GUI/socket counterparts (afxwin.h, afxsock.h...) remain declaration-only
// for now, not yet implemented — see ../README.md.
//
// What is NOT implemented here because it is inherently Windows/GUI
// specific (see ../README.md for the full list):
//   - CObject::Serialize/CArchive (MFC serialization infrastructure, tied
//     to CFile/CDocument in a Windows-specific way, out of scope)
//   - CString::LoadString (loads strings from PE .rc resources, no
//     standard C++ equivalent)
//   - CException::ReportError only prints to stderr, not a real MessageBox
#pragma once

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

// Compatibility with the headers that are still "declaration-only" (afxdd_.h,
// afxwin.h...), which use these MSVC-style macros/keywords. No-op on
// non-MSVC compilers.
#ifndef _MSC_VER
#define __cdecl
#define __stdcall
#endif
#define AFXAPI __stdcall

using UINT = unsigned int;
using DWORD = unsigned long;
using LONG = long;
using LONGLONG = long long;
using ULONGLONG = unsigned long long;
using BOOL = int;
constexpr BOOL TRUE_ = 1;
constexpr BOOL FALSE_ = 0;
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
// Real LPTSTR/LPCTSTR (winnt.h) are typedef chains through WCHAR*, not a
// bare wchar_t* alias -- MSVC's redefinition check treats that as a
// "different basic type" even though the two are layout-identical, so
// this collides with real <windows.h> the same way afxwin.h's
// SECURITY_ATTRIBUTES/CREATESTRUCT do (see there). Deferred to the real
// header on _WIN32. CString and friends are unconditionally wide-char
// (std::wstring-backed) regardless of the project's own charset setting,
// so UNICODE/_UNICODE are forced here too -- otherwise winnt.h's LPTSTR
// resolves to the ANSI (char*) alias and every wide-char call site in
// this library (wmemcpy, CDumpContext::operator<<...) stops matching.
#ifdef _WIN32
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#else
using LPCTSTR = const wchar_t*;
using LPTSTR = wchar_t*;
#endif

// ---------------------------------------------------------------------
// Lightweight MFC-style RTTI (does not use the compiler's typeid/
// dynamic_cast, exactly like real MFC: a chain of CRuntimeClass walkable
// at runtime). Pure standard C++ constructs, no Windows dependency.
// ---------------------------------------------------------------------
class CObject;
class CDumpContext; // full definition below, after CObject (needed for CObject::Dump)

struct CRuntimeClass
{
    const char* m_lpszClassName;
    const CRuntimeClass* m_pBaseClass;
    CObject* (*m_pfnCreateObject)(); // nullptr if the class is not "creatable by name" (DECLARE_DYNAMIC only)

    bool IsDerivedFrom(const CRuntimeClass* pBase) const noexcept
    {
        for (const CRuntimeClass* p = this; p; p = p->m_pBaseClass)
            if (p == pBase)
                return true;
        return false;
    }

    CObject* CreateObject() const { return m_pfnCreateObject ? m_pfnCreateObject() : nullptr; }
};

#define DECLARE_DYNAMIC(class_name)                                         \
public:                                                                     \
    static const CRuntimeClass classCRuntimeClass;                          \
    CRuntimeClass* GetRuntimeClass() const override                        \
    {                                                                       \
        return const_cast<CRuntimeClass*>(&class_name::classCRuntimeClass); \
    }

#define IMPLEMENT_DYNAMIC(class_name, base_class_name) \
    const CRuntimeClass class_name::classCRuntimeClass = \
        {#class_name, &base_class_name::classCRuntimeClass, nullptr};

#define DECLARE_DYNCREATE(class_name) DECLARE_DYNAMIC(class_name)

#define IMPLEMENT_DYNCREATE(class_name, base_class_name)                      \
    static CObject* AFX_CreateObject_##class_name() { return new class_name; } \
    const CRuntimeClass class_name::classCRuntimeClass =                       \
        {#class_name, &base_class_name::classCRuntimeClass, &AFX_CreateObject_##class_name};

#define RUNTIME_CLASS(class_name) (&class_name::classCRuntimeClass)

// ---------------------------------------------------------------------
// CObject — root of the hierarchy. IsSerializable/Serialize are left as
// no-op hooks: real MFC (de)serialization depends on CArchive, tied to
// CFile/CDocument in an MFC-specific way, out of scope here.
// ---------------------------------------------------------------------
class CObject
{
public:
    static const CRuntimeClass classCRuntimeClass;
    virtual CRuntimeClass* GetRuntimeClass() const { return const_cast<CRuntimeClass*>(&classCRuntimeClass); }
    BOOL IsKindOf(const CRuntimeClass* pClass) const { return GetRuntimeClass()->IsDerivedFrom(pClass) ? TRUE : FALSE; }
    virtual void AssertValid() const {}
    // Real MFC: prints the class name if the class uses IMPLEMENT_DYNAMIC/
    // IMPLEMENT_DYNCREATE/IMPLEMENT_SERIAL, otherwise prints "CObject". Here
    // GetRuntimeClass() always resolves to a real CRuntimeClass (RTTI is
    // unconditionally available in this port, see IMPLEMENT_DYNAMIC above),
    // so the class name is always printed. Unlike real MFC, not gated on
    // _DEBUG (same design choice already made for AssertValid).
    virtual void Dump(CDumpContext& dc) const;
    virtual BOOL IsSerializable() const { return FALSE; }
    virtual ~CObject() = default;
};

// ---------------------------------------------------------------------
// CDumpContext — diagnostic dump support for CObject::Dump. On top of
// std::wostream (defaults to std::wcerr: real MFC describes afxDump's
// output as "conceptually similar to the cerr stream"). Real MFC's
// constructor instead binds to a CFile* destination (afxDump is built for
// you); not implemented here since nothing in the covered eMule call
// sites ever constructs its own CDumpContext or writes to afxDump
// directly — only the Dump(dc) super-call chain is exercised, which just
// needs a working destination to forward to.
// ---------------------------------------------------------------------
class CDumpContext
{
public:
    explicit CDumpContext(std::wostream& os = std::wcerr) : m_os(os) {}

    void SetDepth(int nNewDepth) noexcept { m_nDepth = nNewDepth; }
    int GetDepth() const noexcept { return m_nDepth; }

    CDumpContext& operator<<(const char* lpsz);
    CDumpContext& operator<<(LPCTSTR lpsz);
    CDumpContext& operator<<(const CObject* pOb);
    CDumpContext& operator<<(const CObject& ob) { return *this << &ob; }
    CDumpContext& operator<<(int n);
    CDumpContext& operator<<(unsigned int u);
    CDumpContext& operator<<(long l);
    CDumpContext& operator<<(double d);
    CDumpContext& operator<<(const void* lp);

private:
    std::wostream& m_os;
    int m_nDepth = 0;
};

// Real MFC's predeclared global dump context (Debug-only there; always
// available here, consistent with Dump/AssertValid above).
extern CDumpContext afxDump;

// ---------------------------------------------------------------------
// CString — a real wrapper around std::wstring exposing the standard MFC
// interface. LoadString (PE .rc resources) is omitted: no standard C++
// equivalent, see ../README.md. Declared here, before CException, because
// CFileException::m_strFileName is a CString (matching real MFC).
// ---------------------------------------------------------------------
class CString
{
public:
    CString() = default;
    CString(const CString&) = default;
    CString(CString&&) noexcept = default;
    CString(const wchar_t* psz) : m_data(psz ? psz : L"") {}
    CString(wchar_t ch, int nRepeat = 1) : m_data(static_cast<size_t>(nRepeat), ch) {}
    CString& operator=(const CString&) = default;
    CString& operator=(CString&&) noexcept = default;
    CString& operator=(const wchar_t* psz) { m_data = psz ? psz : L""; return *this; }

    int GetLength() const noexcept { return static_cast<int>(m_data.size()); }
    bool IsEmpty() const noexcept { return m_data.empty(); }
    void Empty() noexcept { m_data.clear(); }
    wchar_t* GetBuffer(int nMinBufferLength);
    wchar_t* GetBuffer() { return GetBuffer(GetLength()); }
    void ReleaseBuffer(int nNewLength = -1);
    wchar_t GetAt(int iChar) const { return m_data.at(static_cast<size_t>(iChar)); }
    void SetAt(int iChar, wchar_t ch) { m_data.at(static_cast<size_t>(iChar)) = ch; }

    void Format(const wchar_t* pszFormat, ...);
    void AppendFormat(const wchar_t* pszFormat, ...);
    int Compare(const wchar_t* psz) const { return m_data.compare(psz); }
    int CompareNoCase(const wchar_t* psz) const;
    int Delete(int iIndex, int nCount = 1);
    int Find(const wchar_t* pszSub, int iStart = 0) const;
    int Find(wchar_t ch, int iStart = 0) const;
    int ReverseFind(wchar_t ch) const;
    int Insert(int iIndex, const wchar_t* psz);
    int Insert(int iIndex, wchar_t ch);
    CString Left(int nCount) const;
    CString Right(int nCount) const;
    CString Mid(int iFirst, int nCount) const;
    CString Mid(int iFirst) const;
    CString& MakeLower();
    CString& MakeUpper();
    int Replace(const wchar_t* pszOld, const wchar_t* pszNew);
    int Replace(wchar_t chOld, wchar_t chNew);
    CString SpanExcluding(const wchar_t* pszCharSet) const;
    CString Tokenize(const wchar_t* pszTokens, int& iStart) const;
    CString& Trim();
    CString& Trim(wchar_t chTarget);
    CString& Trim(const wchar_t* pszTargets);
    CString& TrimRight();
    CString& TrimRight(wchar_t chTarget);
    CString& TrimRight(const wchar_t* pszTargets);

    const wchar_t* c_str() const noexcept { return m_data.c_str(); }
    operator const wchar_t*() const noexcept { return m_data.c_str(); }
    wchar_t operator[](int i) const { return m_data[static_cast<size_t>(i)]; }
    const std::wstring& AsStdString() const noexcept { return m_data; }

    CString& operator+=(const CString& s) { m_data += s.m_data; return *this; }
    CString& operator+=(wchar_t ch) { m_data += ch; return *this; }
    friend CString operator+(const CString& a, const CString& b) { CString r(a); r += b; return r; }
    friend bool operator==(const CString& a, const CString& b) { return a.m_data == b.m_data; }
    friend bool operator!=(const CString& a, const CString& b) { return a.m_data != b.m_data; }
    friend bool operator<(const CString& a, const CString& b) { return a.m_data < b.m_data; }

private:
    std::wstring m_data;
};

namespace std
{
template <>
struct hash<CString>
{
    size_t operator()(const CString& s) const noexcept { return std::hash<std::wstring>{}(s.AsStdString()); }
};
} // namespace std

// ---------------------------------------------------------------------
// CException — real hierarchy, genuine C++ exceptions (used with standard
// throw/catch, not with the original MFC pointer+Delete() pattern, even
// though Delete() is still available for interface compatibility with
// code written against real MFC).
// ---------------------------------------------------------------------
class CException : public CObject
{
    DECLARE_DYNAMIC(CException)
public:
    explicit CException(BOOL bAutoDelete) : m_bAutoDelete(bAutoDelete) {}
    void Delete() { if (m_bAutoDelete) delete this; }
    virtual int ReportError(UINT nType = 0, UINT nMessageID = 0);

private:
    BOOL m_bAutoDelete;
};

// Abstract base for "resource-critical" exceptions. In real MFC
// GetErrorMessage is virtual with a body (not pure); here it is made pure
// to enforce abstractness at compile time (an explicit design choice), so
// every concrete subclass must provide its own override.
class CSimpleException : public CException
{
    DECLARE_DYNAMIC(CSimpleException)
public:
    CSimpleException() : CException(FALSE) {}
    explicit CSimpleException(BOOL bAutoDelete) : CException(bAutoDelete) {}
    virtual BOOL GetErrorMessage(LPTSTR lpszError, UINT nMaxError, UINT* pnHelpContext = nullptr) const = 0;
};

class CMemoryException : public CSimpleException
{
    DECLARE_DYNAMIC(CMemoryException)
public:
    CMemoryException() : CSimpleException(FALSE) {}
    BOOL GetErrorMessage(LPTSTR lpszError, UINT nMaxError, UINT* pnHelpContext = nullptr) const override;
};

class CFileException : public CException
{
    DECLARE_DYNAMIC(CFileException)
public:
    enum Cause
    {
        none, genericException, fileNotFound, badPath, tooManyOpenFiles,
        accessDenied, invalidFile, removeCurrentDir, directoryFull, badSeek,
        hardIO, sharingViolation, lockViolation, diskFull, endOfFile
    };

    explicit CFileException(int cause = none, LONG lOsError = -1, LPCTSTR lpszFileName = nullptr)
        : CException(TRUE), m_cause(cause), m_lOsError(lOsError),
          m_strFileName(lpszFileName ? lpszFileName : L"") {}

    BOOL GetErrorMessage(LPTSTR lpszError, UINT nMaxError, UINT* pnHelpContext = nullptr) const;

    // Static factory: maps an OS-specific error code to a Cause (falling
    // back to genericException for anything not recognized, matching real
    // MFC's documented fallback) and throws the resulting CFileException by
    // pointer, exactly like AfxThrowFileException. Real MFC also exposes
    // OsErrorToException/ErrnoToException/ThrowErrno as separate public
    // static methods; not added here since eMule/srchybrid only ever calls
    // ThrowOsError itself (see mfc_scan_srchybrid.md blind-spot findings).
    [[noreturn]] static void ThrowOsError(LONG lOsError, LPCTSTR lpszFileName = nullptr);

    int m_cause;
    LONG m_lOsError;
    CString m_strFileName;
};

// Global exception-throwing functions. In real MFC, AfxThrowInvalidArgException,
// AfxThrowNotSupportedException, AfxThrowResourceException and
// AfxThrowUserException throw classes (CInvalidArgException, etc.) that are
// not implemented here — see ../README.md for why only the two below are
// provided.
[[noreturn]] void AfxThrowFileException(int cause, LONG lOsError = -1, LPCTSTR lpszFileName = nullptr);
[[noreturn]] void AfxThrowMemoryException();
[[noreturn]] void AfxAbort();

// ---------------------------------------------------------------------
// CFile / CStdioFile / CMemFile — built on std::fstream / an in-memory
// buffer, fully portable (no Win32 HANDLE).
// ---------------------------------------------------------------------
struct CFileStatus
{
    ULONGLONG m_size = 0;
};

class CFile : public CObject
{
    DECLARE_DYNAMIC(CFile)
public:
    enum OpenFlags
    {
        modeRead = 0x0000, modeWrite = 0x0001, modeReadWrite = 0x0002,
        modeCreate = 0x1000, modeNoTruncate = 0x2000,
        shareExclusive = 0x0010, shareDenyNone = 0x0040,
        typeBinary = 0x0000, typeText = 0x4000
    };
    enum SeekPosition { begin = 0, current = 1, end = 2 };

    CFile() = default;
    CFile(LPCTSTR lpszFileName, UINT nOpenFlags) { Open(lpszFileName, nOpenFlags); }
    virtual ~CFile() { if (m_stream.is_open()) m_stream.close(); }

    virtual BOOL Open(LPCTSTR lpszFileName, UINT nOpenFlags, CFileException* pError = nullptr);
    virtual void Abort() { if (m_stream.is_open()) m_stream.close(); }
    virtual void Close() { if (m_stream.is_open()) m_stream.close(); }
    virtual UINT Read(void* lpBuf, UINT nCount);
    virtual void Write(const void* lpBuf, UINT nCount);
    virtual ULONGLONG Seek(LONGLONG lOff, UINT nFrom);
    void SeekToBegin() { Seek(0, begin); }
    ULONGLONG SeekToEnd() { return Seek(0, end); }
    virtual ULONGLONG GetLength() const;
    virtual void SetLength(ULONGLONG dwNewLen);
    virtual ULONGLONG GetPosition() const;
    virtual void Flush() { m_stream.flush(); }
    virtual CString GetFileName() const { return CString(std::filesystem::path(m_path).filename().wstring().c_str()); }
    virtual CString GetFilePath() const { return CString(m_path.c_str()); }
    BOOL GetStatus(CFileStatus& rStatus) const;
    static BOOL GetStatus(LPCTSTR lpszFileName, CFileStatus& rStatus);
    static void Remove(LPCTSTR lpszFileName);
    static void Rename(LPCTSTR lpszOldName, LPCTSTR lpszNewName);

protected:
    std::fstream m_stream;
    std::wstring m_path;
};

class CStdioFile : public CFile
{
    DECLARE_DYNAMIC(CStdioFile)
public:
    CStdioFile() = default;
    CStdioFile(LPCTSTR lpszFileName, UINT nOpenFlags) : CFile(lpszFileName, nOpenFlags) {}

    virtual LPTSTR ReadString(LPTSTR lpsz, UINT nMax);
    virtual BOOL ReadString(CString& rString);
    virtual void WriteString(LPCTSTR lpsz);
};

// CMemFile — an entirely in-memory file (std::vector<uint8_t>), no methods
// of its own beyond the ones it inherits.
class CMemFile : public CFile
{
    DECLARE_DYNAMIC(CMemFile)
public:
    CMemFile() = default;
    UINT Read(void* lpBuf, UINT nCount) override;
    void Write(const void* lpBuf, UINT nCount) override;
    ULONGLONG Seek(LONGLONG lOff, UINT nFrom) override;
    ULONGLONG GetLength() const override { return m_buffer.size(); }
    void SetLength(ULONGLONG dwNewLen) override { m_buffer.resize(static_cast<size_t>(dwNewLen)); }
    ULONGLONG GetPosition() const override { return m_pos; }

private:
    std::vector<uint8_t> m_buffer;
    size_t m_pos = 0;
};

// ---------------------------------------------------------------------
// CFileFind — built on std::filesystem (standard C++17, no FindFirstFile).
// FindNextFile is also a real winbase.h macro (expands to FindNextFileW/A)
// -- undefined here, the same way real MFC's own afx.h does it, so the
// member keeps its true name instead of being silently rewritten to
// something CFileFind doesn't have. Once undef'd it stays gone for the
// rest of the translation unit, so later call sites (this file's own
// afx.cpp, and any including code such as eMule's) see the same name too.
// ---------------------------------------------------------------------
#undef FindNextFile
class CFileFind : public CObject
{
    DECLARE_DYNAMIC(CFileFind)
public:
    virtual BOOL FindFile(LPCTSTR pstrName = nullptr, DWORD dwUnused = 0);
    virtual BOOL FindNextFile();
    void Close() { m_it = std::filesystem::directory_iterator(); m_pending.reset(); }
    BOOL IsDirectory() const;
    virtual BOOL IsDots() const;
    virtual CString GetFileName() const;
    virtual CString GetFilePath() const;
    ULONGLONG GetLength() const;
    virtual CString GetRoot() const;

private:
    bool AdvanceToNextMatch();

    std::filesystem::path m_dir;
    std::wstring m_root;
    std::wstring m_pattern;
    std::filesystem::directory_iterator m_it;
    std::optional<std::filesystem::directory_entry> m_pending;
    std::filesystem::directory_entry m_current;
};
