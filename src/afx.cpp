#include "afx.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cwctype>
#include <cstdlib>
#include <cstring>

// ---------------------------------------------------------------------
// RTTI
// ---------------------------------------------------------------------
const CRuntimeClass CObject::classCRuntimeClass = {"CObject", nullptr, nullptr};
IMPLEMENT_DYNAMIC(CException, CObject)
IMPLEMENT_DYNAMIC(CSimpleException, CException)
IMPLEMENT_DYNAMIC(CNotSupportedException, CSimpleException)
IMPLEMENT_DYNAMIC(CMemoryException, CSimpleException)
IMPLEMENT_DYNAMIC(CFileException, CException)
IMPLEMENT_DYNAMIC(CFile, CObject)
IMPLEMENT_DYNAMIC(CStdioFile, CFile)
IMPLEMENT_DYNAMIC(CMemFile, CFile)
IMPLEMENT_DYNAMIC(CFileFind, CObject)

// ---------------------------------------------------------------------
// CDumpContext / CObject::Dump
// ---------------------------------------------------------------------
CDumpContext afxDump;

CDumpContext& CDumpContext::operator<<(const char* lpsz) { if (lpsz) m_os << lpsz; return *this; }
CDumpContext& CDumpContext::operator<<(LPCTSTR lpsz) { if (lpsz) m_os << lpsz; return *this; }
CDumpContext& CDumpContext::operator<<(const CObject* pOb) { if (pOb) pOb->Dump(*this); else m_os << L"(null)"; return *this; }
CDumpContext& CDumpContext::operator<<(int n) { m_os << n; return *this; }
CDumpContext& CDumpContext::operator<<(unsigned int u) { m_os << u; return *this; }
CDumpContext& CDumpContext::operator<<(long l) { m_os << l; return *this; }
CDumpContext& CDumpContext::operator<<(double d) { m_os << d; return *this; }
CDumpContext& CDumpContext::operator<<(const void* lp) { m_os << lp; return *this; }

void CObject::Dump(CDumpContext& dc) const
{
    dc << GetRuntimeClass()->m_lpszClassName;
}

// ---------------------------------------------------------------------
// CException
// ---------------------------------------------------------------------
int CException::ReportError(UINT /*nType*/, UINT /*nMessageID*/)
{
    // Real MFC opens a MessageBox (Win32). Here, with no GUI available,
    // we print to stderr: an equivalent "headless" behavior.
    wchar_t buf[512]{};
    // GetErrorMessage is not virtual on the CException base in real MFC;
    // the subclasses (CFileException, CSimpleException) expose it.
    std::fwprintf(stderr, L"[CException] %ls\n", buf[0] ? buf : L"(no message)");
    return 0;
}

// ---------------------------------------------------------------------
// CNotSupportedException
// ---------------------------------------------------------------------
BOOL CNotSupportedException::GetErrorMessage(LPTSTR lpszError, UINT nMaxError, UINT* pnHelpContext) const
{
    if (pnHelpContext) *pnHelpContext = 0;
    if (!lpszError || nMaxError == 0) return FALSE;
    const wchar_t* msg = L"Unsupported operation.";
    size_t n = std::min<size_t>(nMaxError - 1, std::char_traits<wchar_t>::length(msg));
    std::wmemcpy(lpszError, msg, n);
    lpszError[n] = L'\0';
    return TRUE;
}

// ---------------------------------------------------------------------
// CMemoryException
// ---------------------------------------------------------------------
BOOL CMemoryException::GetErrorMessage(LPTSTR lpszError, UINT nMaxError, UINT* pnHelpContext) const
{
    if (pnHelpContext) *pnHelpContext = 0;
    if (!lpszError || nMaxError == 0) return FALSE;
    const wchar_t* msg = L"Out of memory.";
    size_t n = std::min<size_t>(nMaxError - 1, std::char_traits<wchar_t>::length(msg));
    std::wmemcpy(lpszError, msg, n);
    lpszError[n] = L'\0';
    return TRUE;
}

// ---------------------------------------------------------------------
// CFileException
// ---------------------------------------------------------------------
BOOL CFileException::GetErrorMessage(LPTSTR lpszError, UINT nMaxError, UINT* pnHelpContext) const
{
    if (pnHelpContext) *pnHelpContext = 0;
    if (!lpszError || nMaxError == 0) return FALSE;
    std::wstring msg;
    switch (m_cause)
    {
        case fileNotFound: msg = L"File not found."; break;
        case badPath: msg = L"Invalid path."; break;
        case tooManyOpenFiles: msg = L"Too many open files."; break;
        case accessDenied: msg = L"Access denied."; break;
        case diskFull: msg = L"Disk full."; break;
        case endOfFile: msg = L"Unexpected end of file."; break;
        case sharingViolation: msg = L"Sharing violation."; break;
        default: msg = L"File error."; break;
    }
    if (!m_strFileName.IsEmpty())
    {
        msg += L" (";
        msg += m_strFileName.AsStdString();
        msg += L")";
    }
    size_t n = std::min<size_t>(nMaxError - 1, msg.size());
    std::wmemcpy(lpszError, msg.c_str(), n);
    lpszError[n] = L'\0';
    return TRUE;
}

// Best-effort mapping from a Win32 GetLastError()-style OS error code to a
// CFileException::Cause, covering the common, well-documented codes real
// MFC's own (closed-source) internal table maps; anything else falls back
// to genericException, matching the documented fallback behavior of real
// MFC's CFileException::OsErrorToException/ThrowOsError. Literal values
// used instead of <windows.h> macros to keep this file portable (same
// convention already used in afxsock.h for socket constants).
namespace
{
int OsErrorToCause(LONG lOsError)
{
    switch (lOsError)
    {
        case 2: return CFileException::fileNotFound;         // ERROR_FILE_NOT_FOUND
        case 3: return CFileException::badPath;               // ERROR_PATH_NOT_FOUND
        case 4: return CFileException::tooManyOpenFiles;      // ERROR_TOO_MANY_OPEN_FILES
        case 5: return CFileException::accessDenied;          // ERROR_ACCESS_DENIED
        case 6: return CFileException::invalidFile;           // ERROR_INVALID_HANDLE
        case 19: return CFileException::accessDenied;         // ERROR_WRITE_PROTECT
        case 32: return CFileException::sharingViolation;     // ERROR_SHARING_VIOLATION
        case 33: return CFileException::lockViolation;        // ERROR_LOCK_VIOLATION
        case 38: return CFileException::endOfFile;             // ERROR_HANDLE_EOF
        case 39: return CFileException::diskFull;              // ERROR_HANDLE_DISK_FULL
        case 112: return CFileException::diskFull;             // ERROR_DISK_FULL
        default: return CFileException::genericException;
    }
}
} // namespace

[[noreturn]] void CFileException::ThrowOsError(LONG lOsError, LPCTSTR lpszFileName)
{
    throw new CFileException(OsErrorToCause(lOsError), lOsError, lpszFileName);
}

// ---------------------------------------------------------------------
// Global AfxThrow* functions — throw by pointer, like real MFC (calling
// code catches with `catch (CFileException* e)` and then calls
// `e->Delete()`). AfxThrowMemoryException throws a preallocated STATIC
// instance: during a genuine out-of-memory condition, a `new` would fail.
// ---------------------------------------------------------------------
[[noreturn]] void AfxThrowFileException(int cause, LONG lOsError, LPCTSTR lpszFileName)
{
    throw new CFileException(cause, lOsError, lpszFileName);
}

[[noreturn]] void AfxThrowMemoryException()
{
    static CMemoryException s_oom;
    throw &s_oom;
}

[[noreturn]] void AfxAbort()
{
    std::abort();
}

// ---------------------------------------------------------------------
// CFile
// ---------------------------------------------------------------------
BOOL CFile::Open(LPCTSTR lpszFileName, UINT nOpenFlags, CFileException* pError)
{
    std::ios_base::openmode mode = std::ios::binary;
    if (nOpenFlags & modeWrite) mode |= std::ios::out;
    else if (nOpenFlags & modeReadWrite) mode |= (std::ios::in | std::ios::out);
    else mode |= std::ios::in;
    if (nOpenFlags & modeCreate)
    {
        mode |= std::ios::out;
        if (!(nOpenFlags & modeNoTruncate)) mode |= std::ios::trunc;
    }

    m_path = lpszFileName ? lpszFileName : L"";
    m_stream.open(std::filesystem::path(m_path), mode);
    if (!m_stream.is_open())
    {
        if (pError) *pError = CFileException(CFileException::fileNotFound, -1, lpszFileName);
        return FALSE;
    }
    return TRUE;
}

UINT CFile::Read(void* lpBuf, UINT nCount)
{
    m_stream.read(static_cast<char*>(lpBuf), nCount);
    return static_cast<UINT>(m_stream.gcount());
}

void CFile::Write(const void* lpBuf, UINT nCount)
{
    m_stream.write(static_cast<const char*>(lpBuf), nCount);
}

ULONGLONG CFile::Seek(LONGLONG lOff, UINT nFrom)
{
    auto dir = nFrom == begin ? std::ios::beg : nFrom == end ? std::ios::end : std::ios::cur;
    m_stream.clear();
    m_stream.seekg(static_cast<std::streamoff>(lOff), dir);
    // Sync the put pointer to the SAME absolute position just resolved by
    // seekg. Re-issuing the seek with a current/end-relative offset would
    // move the shared file position a second time (doubling current-origin
    // seeks); seek the put pointer to the resolved absolute offset instead.
    m_stream.seekp(m_stream.tellg());
    return GetPosition();
}

ULONGLONG CFile::GetLength() const
{
    auto* self = const_cast<CFile*>(this);
    auto cur = self->m_stream.tellg();
    self->m_stream.seekg(0, std::ios::end);
    auto len = self->m_stream.tellg();
    self->m_stream.seekg(cur);
    return static_cast<ULONGLONG>(len);
}

void CFile::SetLength(ULONGLONG dwNewLen)
{
    m_stream.close();
    std::filesystem::resize_file(std::filesystem::path(m_path), dwNewLen);
    m_stream.open(std::filesystem::path(m_path), std::ios::binary | std::ios::in | std::ios::out);
}

ULONGLONG CFile::GetPosition() const
{
    auto* self = const_cast<CFile*>(this);
    return static_cast<ULONGLONG>(self->m_stream.tellg());
}

BOOL CFile::GetStatus(CFileStatus& rStatus) const
{
    rStatus.m_size = GetLength();
    return TRUE;
}

BOOL CFile::GetStatus(LPCTSTR lpszFileName, CFileStatus& rStatus)
{
    std::error_code ec;
    auto sz = std::filesystem::file_size(std::filesystem::path(lpszFileName), ec);
    if (ec) return FALSE;
    rStatus.m_size = sz;
    return TRUE;
}

void CFile::Remove(LPCTSTR lpszFileName)
{
    std::filesystem::remove(std::filesystem::path(lpszFileName));
}

void CFile::Rename(LPCTSTR lpszOldName, LPCTSTR lpszNewName)
{
    std::filesystem::rename(std::filesystem::path(lpszOldName), std::filesystem::path(lpszNewName));
}

// ---------------------------------------------------------------------
// CStdioFile
// ---------------------------------------------------------------------
// std::wfstream is not used here (the file is opened in binary mode): we
// read line by line as a sequence of raw wchar_t (consistent with the
// typical use of CStdioFile on UTF-16/ASCII text files written by the
// application itself, not external system files). A preceding '\r' (from
// a "\r\n" terminator) is NOT stripped in either overload below and stays
// part of the returned line, unless the file was opened with the
// typeText flag (not implemented as a distinct code path here, since
// simple_mfc never requests OS-level CRLF translation either).
//
// The two overloads differ in what they do with '\n' itself — confirmed
// against real MFC via the conformance suite (tests/conformance/): the
// LPTSTR/UINT overload is fgets()-like and keeps '\n' as the last
// character written into the buffer (if it fits), while the CString&
// overload parses a line and strips '\n' from the result.
LPTSTR CStdioFile::ReadString(LPTSTR lpsz, UINT nMax)
{
    wchar_t ch;
    UINT count = 0;
    bool any = false;
    while (count + 1 < nMax && m_stream.read(reinterpret_cast<char*>(&ch), sizeof(wchar_t)))
    {
        any = true;
        lpsz[count++] = ch;
        if (ch == L'\n') break;
    }
    lpsz[count] = L'\0';
    return any ? lpsz : nullptr;
}

BOOL CStdioFile::ReadString(CString& rString)
{
    wchar_t ch;
    std::wstring line;
    bool any = false;
    while (m_stream.read(reinterpret_cast<char*>(&ch), sizeof(wchar_t)))
    {
        any = true;
        if (ch == L'\n') break;
        line += ch;
    }
    rString = line.c_str();
    return any ? TRUE : FALSE;
}

void CStdioFile::WriteString(LPCTSTR lpsz)
{
    m_stream.write(reinterpret_cast<const char*>(lpsz), static_cast<std::streamsize>(std::char_traits<wchar_t>::length(lpsz) * sizeof(wchar_t)));
}

// ---------------------------------------------------------------------
// CMemFile
// ---------------------------------------------------------------------
UINT CMemFile::Read(void* lpBuf, UINT nCount)
{
    size_t avail = m_buffer.size() > m_pos ? m_buffer.size() - m_pos : 0;
    size_t n = std::min<size_t>(nCount, avail);
    std::memcpy(lpBuf, m_buffer.data() + m_pos, n);
    m_pos += n;
    return static_cast<UINT>(n);
}

void CMemFile::Write(const void* lpBuf, UINT nCount)
{
    if (m_pos + nCount > m_buffer.size()) m_buffer.resize(m_pos + nCount);
    std::memcpy(m_buffer.data() + m_pos, lpBuf, nCount);
    m_pos += nCount;
}

// Real MFC reallocates the buffer so that a later write of dwNewLen bytes
// needs no further growth; a std::vector already amortises that, so this
// only has to guarantee the length.
void CMemFile::GrowFile(ULONGLONG dwNewLen)
{
    if (dwNewLen > m_buffer.size())
        m_buffer.resize(static_cast<size_t>(dwNewLen));
}

ULONGLONG CMemFile::Seek(LONGLONG lOff, UINT nFrom)
{
    long long base = nFrom == CFile::begin ? 0 : nFrom == CFile::end ? static_cast<long long>(m_buffer.size()) : static_cast<long long>(m_pos);
    long long np = base + lOff;
    m_pos = static_cast<size_t>(std::clamp<long long>(np, 0, static_cast<long long>(m_buffer.size())));
    return m_pos;
}

// ---------------------------------------------------------------------
// CFileFind
// ---------------------------------------------------------------------
namespace
{
bool WildcardMatch(const std::wstring& pattern, const std::wstring& name)
{
    size_t p = 0, n = 0, star = std::wstring::npos, mark = 0;
    while (n < name.size())
    {
        if (p < pattern.size() && (pattern[p] == L'?' || pattern[p] == name[n])) { ++p; ++n; }
        else if (p < pattern.size() && pattern[p] == L'*') { star = p++; mark = n; }
        else if (star != std::wstring::npos) { p = star + 1; n = ++mark; }
        else return false;
    }
    while (p < pattern.size() && pattern[p] == L'*') ++p;
    return p == pattern.size();
}
}

BOOL CFileFind::FindFile(LPCTSTR pstrName, DWORD /*dwUnused*/)
{
    std::filesystem::path spec = pstrName && *pstrName ? std::filesystem::path(pstrName) : std::filesystem::path(L"*");
    m_dir = spec.has_parent_path() ? spec.parent_path() : std::filesystem::path(L".");
    std::wstring pattern = spec.filename().wstring();
    if (pattern.empty()) pattern = L"*";

    // Real MFC's GetRoot returns the search string with the file-name part
    // stripped but the trailing path separator KEPT (e.g. "C:\dir\"). Derive
    // it from the original spec rather than m_dir (whose parent_path() drops
    // the separator).
    std::wstring specStr = spec.wstring();
    std::wstring fname = spec.filename().wstring();
    m_root = specStr.substr(0, specStr.size() - fname.size());

    std::error_code ec;
    m_it = std::filesystem::directory_iterator(m_dir, ec);
    m_pending.reset();
    if (ec) return FALSE;

    m_pattern = pattern;
    return AdvanceToNextMatch() ? TRUE : FALSE;
}

BOOL CFileFind::FindNextFile()
{
    if (!m_pending) return FALSE;
    m_current = *m_pending;
    m_pending.reset();
    return AdvanceToNextMatch() ? TRUE : FALSE;
}

bool CFileFind::AdvanceToNextMatch()
{
    std::filesystem::directory_iterator endIt;
    while (m_it != endIt)
    {
        auto entry = *m_it;
        ++m_it;
        if (WildcardMatch(m_pattern, entry.path().filename().wstring()))
        {
            m_pending = entry;
            return true;
        }
    }
    return false;
}

BOOL CFileFind::IsDirectory() const
{
    std::error_code ec;
    return std::filesystem::is_directory(m_current.path(), ec) ? TRUE : FALSE;
}

#ifdef _WIN32
static BOOL HasFileAttribute(const std::filesystem::path& p, DWORD dwAttr); // defined below
#endif

// Timestamps. std::filesystem exposes only the modification time, and
// only as a file_time_type whose epoch is unspecified before C++20's
// clock_cast -- hence the now()-difference conversion below. Creation and
// last-access times have no portable source at all, so they answer FALSE
// rather than inventing a value (same rule as the attribute bits below).
BOOL CFileFind::GetLastWriteTime(CTime& refTime) const
{
    std::error_code ec;
    auto ftime = std::filesystem::last_write_time(m_current.path(), ec);
    if (ec)
        return FALSE;
    const auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
    refTime = CTime(static_cast<__time64_t>(std::chrono::system_clock::to_time_t(sctp)));
    return TRUE;
}

BOOL CFileFind::GetCreationTime(CTime& /*refTime*/) const { return FALSE; }
BOOL CFileFind::GetLastAccessTime(CTime& /*refTime*/) const { return FALSE; }

BOOL CFileFind::IsTemporary() const
{
#ifdef _WIN32
    return HasFileAttribute(m_current.path(), FILE_ATTRIBUTE_TEMPORARY);
#else
    return FALSE;
#endif
}

BOOL CFileFind::IsArchived() const
{
#ifdef _WIN32
    return HasFileAttribute(m_current.path(), FILE_ATTRIBUTE_ARCHIVE);
#else
    return FALSE;
#endif
}

BOOL CFileFind::IsCompressed() const
{
#ifdef _WIN32
    return HasFileAttribute(m_current.path(), FILE_ATTRIBUTE_COMPRESSED);
#else
    return FALSE;
#endif
}

// The Windows file-attribute bits. std::filesystem models none of them,
// so they are read from the real API where there is one and reported as
// absent everywhere else.
#ifdef _WIN32
static BOOL HasFileAttribute(const std::filesystem::path& p, DWORD dwAttr)
{
    DWORD attrs = ::GetFileAttributesW(p.wstring().c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES && (attrs & dwAttr)) ? TRUE : FALSE;
}
BOOL CFileFind::IsSystem() const { return HasFileAttribute(m_current.path(), FILE_ATTRIBUTE_SYSTEM); }
BOOL CFileFind::IsHidden() const { return HasFileAttribute(m_current.path(), FILE_ATTRIBUTE_HIDDEN); }
BOOL CFileFind::IsReadOnly() const { return HasFileAttribute(m_current.path(), FILE_ATTRIBUTE_READONLY); }
#else
BOOL CFileFind::IsSystem() const { return FALSE; }
BOOL CFileFind::IsHidden() const { return FALSE; }
BOOL CFileFind::IsReadOnly() const
{
    std::error_code ec;
    auto perms = std::filesystem::status(m_current.path(), ec).permissions();
    if (ec)
        return FALSE;
    return (perms & std::filesystem::perms::owner_write) == std::filesystem::perms::none ? TRUE : FALSE;
}
#endif

BOOL CFileFind::IsDots() const
{
    auto name = m_current.path().filename().wstring();
    return (name == L"." || name == L"..") ? TRUE : FALSE;
}

CString CFileFind::GetFileName() const { return CString(m_current.path().filename().wstring().c_str()); }
CString CFileFind::GetFilePath() const { return CString(m_current.path().wstring().c_str()); }

ULONGLONG CFileFind::GetLength() const
{
    std::error_code ec;
    auto sz = std::filesystem::file_size(m_current.path(), ec);
    return ec ? 0 : sz;
}

// Real MFC's GetRoot returns the directory that is being searched (the
// search string with the file-name/wildcard part stripped but the trailing
// separator kept), NOT the filesystem root (C:\). Captured in FindFile.
CString CFileFind::GetRoot() const { return CString(m_root.c_str()); }
