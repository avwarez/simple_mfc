#include "eafx.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cwctype>
#include <cstdlib>
#include <cstring>
#ifndef _WIN32
#include <sys/stat.h> // st_atime, for CFileFind::GetLastAccessTime
#endif

// ---------------------------------------------------------------------
// RTTI
// ---------------------------------------------------------------------
const ECRuntimeClass ECObject::classCRuntimeClass = {"CObject", nullptr, nullptr};
EIMPLEMENT_DYNAMIC(ECException, ECObject)
EIMPLEMENT_DYNAMIC(ECSimpleException, ECException)
EIMPLEMENT_DYNAMIC(ECNotSupportedException, ECSimpleException)
EIMPLEMENT_DYNAMIC(ECMemoryException, ECSimpleException)
EIMPLEMENT_DYNAMIC(ECFileException, ECException)
EIMPLEMENT_DYNAMIC(ECArchiveException, ECException)
EIMPLEMENT_DYNAMIC(ECFile, ECObject)
EIMPLEMENT_DYNAMIC(ECStdioFile, ECFile)
EIMPLEMENT_DYNAMIC(ECMemFile, ECFile)
EIMPLEMENT_DYNAMIC(ECFileFind, ECObject)

// ---------------------------------------------------------------------
// CDumpContext / CObject::Dump
// ---------------------------------------------------------------------
// Each numeric overload goes through the SAME printf conversion real MFC
// uses (%d / %u / %ld / %f), not through the stream's default formatting.
// The two are not interchangeable: an iostream renders 1.5 as "1.5" where
// "%f" renders it as "1.500000", and a dump is read as text.
namespace
{
template <class T>
std::wstring DumpFormat(const wchar_t* conversion, T value)
{
    wchar_t buf[64];
    const int n = std::swprintf(buf, sizeof buf / sizeof buf[0], conversion, value);
    return n > 0 ? std::wstring(buf, static_cast<size_t>(n)) : std::wstring();
}
} // namespace

// Inserting a `const char*` into a wide stream does NOT write the text --
// it selects the pointer overload and writes an address. That is what this
// did, and since CObject::Dump below feeds it m_lpszClassName (a narrow
// literal), EVERY default Dump printed a hexadecimal address where the
// class name belonged. Widen explicitly, as real MFC's LPCSTR overload does.
ECDumpContext& ECDumpContext::operator<<(const char* lpsz)
{
    if (lpsz) m_os << mfc_detail::Widen(lpsz, std::strlen(lpsz));
    return *this;
}
ECDumpContext& ECDumpContext::operator<<(LPCTSTR lpsz) { if (lpsz) m_os << lpsz; return *this; }
// Real MFC spells the null case "NULL"; "(null)" was this branch's own
// wording and nothing but a dump reader would ever have noticed.
ECDumpContext& ECDumpContext::operator<<(const ECObject* pOb) { if (pOb) pOb->Dump(*this); else m_os << L"NULL"; return *this; }
ECDumpContext& ECDumpContext::operator<<(int n) { m_os << DumpFormat(L"%d", n); return *this; }
ECDumpContext& ECDumpContext::operator<<(unsigned int u) { m_os << DumpFormat(L"%u", u); return *this; }
ECDumpContext& ECDumpContext::operator<<(long l) { m_os << DumpFormat(L"%ld", l); return *this; }
ECDumpContext& ECDumpContext::operator<<(double d) { m_os << DumpFormat(L"%f", d); return *this; }
ECDumpContext& ECDumpContext::operator<<(const void* lp) { m_os << lp; return *this; }

void ECObject::Dump(ECDumpContext& dc) const
{
    dc << GetRuntimeClass()->m_lpszClassName;
}

// The function behind the DYNAMIC_DOWNCAST macro: a checked cast that
// answers null instead of undefined behaviour when the object is not of
// the asked-for class. Declared since the first version of this header and
// never defined until the conformance suite reached full method coverage —
// eMule's two DYNAMIC_DOWNCAST sites would not have linked.
ECObject* EAfxDynamicDownCast(ECRuntimeClass* pClass, ECObject* pObject)
{
    if (pObject != nullptr && pObject->IsKindOf(pClass))
        return pObject;
    return nullptr;
}

// ---------------------------------------------------------------------
// CException
// ---------------------------------------------------------------------
int ECException::ReportError(UINT /*nType*/, UINT /*nMessageID*/)
{
    // Real MFC opens a MessageBox (Win32). Here, with no GUI available,
    // we print to stderr: an equivalent "headless" behavior.
    wchar_t buf[512]{};
    GetErrorMessage(buf, 512);
    std::fwprintf(stderr, L"[CException] %ls\n", buf[0] ? buf : L"(no message)");
    return 0;
}

// Declared virtual on the base, as real MFC does -- eMule calls it
// through a plain "const CException&" (OtherFunctions.cpp:1793), which
// only compiles if the base has it. (An earlier note here claimed the
// opposite; the compile check disproved it.) The base itself knows no
// message, so it reports failure and the subclasses override.
BOOL ECException::GetErrorMessage(LPTSTR lpszError, UINT nMaxError, UINT* pnHelpContext) const
{
    if (pnHelpContext) *pnHelpContext = 0;
    if (lpszError && nMaxError) lpszError[0] = _T('\0');
    return FALSE;
}

// ---------------------------------------------------------------------
// CNotSupportedException
// ---------------------------------------------------------------------
BOOL ECNotSupportedException::GetErrorMessage(LPTSTR lpszError, UINT nMaxError, UINT* pnHelpContext) const
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
BOOL ECMemoryException::GetErrorMessage(LPTSTR lpszError, UINT nMaxError, UINT* pnHelpContext) const
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
BOOL ECFileException::GetErrorMessage(LPTSTR lpszError, UINT nMaxError, UINT* pnHelpContext) const
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
        case 2: return ECFileException::fileNotFound;         // ERROR_FILE_NOT_FOUND
        case 3: return ECFileException::badPath;               // ERROR_PATH_NOT_FOUND
        case 4: return ECFileException::tooManyOpenFiles;      // ERROR_TOO_MANY_OPEN_FILES
        case 5: return ECFileException::accessDenied;          // ERROR_ACCESS_DENIED
        case 6: return ECFileException::fileNotFound;          // ERROR_INVALID_HANDLE (real MFC's answer, verified)
        case 19: return ECFileException::accessDenied;         // ERROR_WRITE_PROTECT
        case 32: return ECFileException::sharingViolation;     // ERROR_SHARING_VIOLATION
        case 33: return ECFileException::lockViolation;        // ERROR_LOCK_VIOLATION
        case 38: return ECFileException::endOfFile;             // ERROR_HANDLE_EOF
        case 39: return ECFileException::diskFull;              // ERROR_HANDLE_DISK_FULL
        case 112: return ECFileException::diskFull;             // ERROR_DISK_FULL
        // The rest of the table was filled in from real MFC's own answers,
        // read off the conformance run that drives ThrowOsError over every
        // common Win32 file error. ERROR_BAD_PATHNAME in particular used to
        // fall through to genericException here while real MFC answers
        // badPath -- a caller switching on m_cause took the wrong branch.
        case 15: return ECFileException::badPath;               // ERROR_INVALID_DRIVE
        case 16: return ECFileException::removeCurrentDir;      // ERROR_CURRENT_DIRECTORY
        case 123: return ECFileException::badPath;              // ERROR_INVALID_NAME
        case 131: return ECFileException::badSeek;              // ERROR_NEGATIVE_SEEK
        case 161: return ECFileException::badPath;              // ERROR_BAD_PATHNAME
        case 206: return ECFileException::badPath;              // ERROR_FILENAME_EXCED_RANGE
        case 80: return ECFileException::accessDenied;          // ERROR_FILE_EXISTS
        case 145: return ECFileException::removeCurrentDir;     // ERROR_DIR_NOT_EMPTY
        case 183: return ECFileException::accessDenied;         // ERROR_ALREADY_EXISTS
        default: return ECFileException::genericException;
    }
}
} // namespace

[[noreturn]] void ECFileException::ThrowOsError(LONG lOsError, LPCTSTR lpszFileName)
{
    throw new ECFileException(OsErrorToCause(lOsError), lOsError, lpszFileName);
}

// ---------------------------------------------------------------------
// Global AfxThrow* functions — throw by pointer, like real MFC (calling
// code catches with `catch (CFileException* e)` and then calls
// `e->Delete()`). AfxThrowMemoryException throws a preallocated STATIC
// instance: during a genuine out-of-memory condition, a `new` would fail.
// ---------------------------------------------------------------------
[[noreturn]] void EAfxThrowFileException(int cause, LONG lOsError, LPCTSTR lpszFileName)
{
    throw new ECFileException(cause, lOsError, lpszFileName);
}

[[noreturn]] void EAfxThrowMemoryException()
{
    static ECMemoryException s_oom;
    throw &s_oom;
}

// ---------------------------------------------------------------------
// CFile
// ---------------------------------------------------------------------
BOOL ECFile::Open(LPCTSTR lpszFileName, UINT nOpenFlags, ECFileException* pError)
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
        if (pError) *pError = ECFileException(ECFileException::fileNotFound, -1, lpszFileName);
        return FALSE;
    }
    return TRUE;
}

UINT ECFile::Read(void* lpBuf, UINT nCount)
{
    m_stream.read(static_cast<char*>(lpBuf), nCount);
    return static_cast<UINT>(m_stream.gcount());
}

void ECFile::Write(const void* lpBuf, UINT nCount)
{
    m_stream.write(static_cast<const char*>(lpBuf), nCount);
}

ULONGLONG ECFile::Seek(LONGLONG lOff, UINT nFrom)
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

ULONGLONG ECFile::GetLength() const
{
    auto* self = const_cast<ECFile*>(this);
    auto cur = self->m_stream.tellg();
    self->m_stream.seekg(0, std::ios::end);
    auto len = self->m_stream.tellg();
    self->m_stream.seekg(cur);
    return static_cast<ULONGLONG>(len);
}

void ECFile::SetLength(ULONGLONG dwNewLen)
{
    m_stream.close();
    std::filesystem::resize_file(std::filesystem::path(m_path), dwNewLen);
    m_stream.open(std::filesystem::path(m_path), std::ios::binary | std::ios::in | std::ios::out);
}

ULONGLONG ECFile::GetPosition() const
{
    auto* self = const_cast<ECFile*>(this);
    return static_cast<ULONGLONG>(self->m_stream.tellg());
}

BOOL ECFile::GetStatus(ECFileStatus& rStatus) const
{
    rStatus.m_size = GetLength();
    return TRUE;
}

BOOL ECFile::GetStatus(LPCTSTR lpszFileName, ECFileStatus& rStatus)
{
    std::error_code ec;
    auto sz = std::filesystem::file_size(std::filesystem::path(lpszFileName), ec);
    if (ec) return FALSE;
    rStatus.m_size = sz;
    return TRUE;
}

void ECFile::Remove(LPCTSTR lpszFileName)
{
    std::filesystem::remove(std::filesystem::path(lpszFileName));
}

void ECFile::Rename(LPCTSTR lpszOldName, LPCTSTR lpszNewName)
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
LPTSTR ECStdioFile::ReadString(LPTSTR lpsz, UINT nMax)
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

BOOL ECStdioFile::ReadString(ECString& rString)
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

void ECStdioFile::WriteString(LPCTSTR lpsz)
{
    m_stream.write(reinterpret_cast<const char*>(lpsz), static_cast<std::streamsize>(std::char_traits<wchar_t>::length(lpsz) * sizeof(wchar_t)));
}

// ---------------------------------------------------------------------
// CMemFile
// ---------------------------------------------------------------------
UINT ECMemFile::Read(void* lpBuf, UINT nCount)
{
    size_t avail = m_buffer.size() > m_pos ? m_buffer.size() - m_pos : 0;
    size_t n = std::min<size_t>(nCount, avail);
    std::memcpy(lpBuf, m_buffer.data() + m_pos, n);
    m_pos += n;
    return static_cast<UINT>(n);
}

void ECMemFile::Write(const void* lpBuf, UINT nCount)
{
    if (m_pos + nCount > m_buffer.size()) m_buffer.resize(m_pos + nCount);
    std::memcpy(m_buffer.data() + m_pos, lpBuf, nCount);
    m_pos += nCount;
}

// Real MFC reallocates the buffer so that a later write of dwNewLen bytes
// needs no further growth; a std::vector already amortises that, so this
// only has to guarantee the length.
void ECMemFile::GrowFile(ULONGLONG dwNewLen)
{
    if (dwNewLen > m_buffer.size())
        m_buffer.resize(static_cast<size_t>(dwNewLen));
}

ULONGLONG ECMemFile::Seek(LONGLONG lOff, UINT nFrom)
{
    long long base = nFrom == ECFile::begin ? 0 : nFrom == ECFile::end ? static_cast<long long>(m_buffer.size()) : static_cast<long long>(m_pos);
    long long np = base + lOff;
    m_pos = static_cast<size_t>(std::clamp<long long>(np, 0, static_cast<long long>(m_buffer.size())));
    return m_pos;
}

// Hands the contents over as a malloc'd block, matching real MFC's
// documented contract ("the caller becomes responsible for freeing the
// buffer"). The vector-backed storage has no malloc'd block to detach in
// place, so this copies out into a freshly malloc'd one -- externally
// indistinguishable from a true detach, since either way the caller ends
// up owning a malloc'd buffer and this CMemFile ends up empty.
BYTE* ECMemFile::Detach()
{
    size_t n = m_buffer.size();
    BYTE* p = static_cast<BYTE*>(std::malloc(n > 0 ? n : 1));
    if (p != nullptr && n > 0)
        std::memcpy(p, m_buffer.data(), n);
    m_buffer.clear();
    m_pos = 0;
    return p;
}

// Takes ownership of the caller's malloc'd buffer. Real MFC keeps the
// pointer directly as its internal storage; this vector-backed CMemFile
// copies it in and immediately frees the original -- the caller must not
// touch lpBuffer after this call either way, so the two are
// indistinguishable from outside the class.
void ECMemFile::Attach(BYTE* lpBuffer, UINT nBufferSize, UINT /*nGrowBytes*/)
{
    if (lpBuffer != nullptr && nBufferSize > 0)
        m_buffer.assign(lpBuffer, lpBuffer + nBufferSize);
    else
        m_buffer.clear();
    std::free(lpBuffer);
    m_pos = 0;
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

BOOL ECFileFind::FindFile(LPCTSTR pstrName, DWORD /*dwUnused*/)
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

BOOL ECFileFind::FindNextFile()
{
    if (!m_pending) return FALSE;
    m_current = *m_pending;
    m_pending.reset();
    return AdvanceToNextMatch() ? TRUE : FALSE;
}

bool ECFileFind::AdvanceToNextMatch()
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

BOOL ECFileFind::IsDirectory() const
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
BOOL ECFileFind::GetLastWriteTime(ECTime& refTime) const
{
    std::error_code ec;
    auto ftime = std::filesystem::last_write_time(m_current.path(), ec);
    if (ec)
        return FALSE;
    const auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
    refTime = ECTime(static_cast<__time64_t>(std::chrono::system_clock::to_time_t(sctp)));
    return TRUE;
}

// On Windows both of these have a real source (the same
// WIN32_FILE_ATTRIBUTE_DATA the FILETIME overloads below read), and real
// MFC answers TRUE for both. They returned a flat FALSE on every platform
// until the conformance suite compared them: on Windows that was simply
// wrong, not a portability limit.
#ifdef _WIN32
namespace
{
// A FILETIME is 100 ns ticks since 1601-01-01; a CTime is seconds since
// 1970-01-01. 11644473600 is the gap between the two epochs.
ECTime TimeFromFileTime(const FILETIME& ft)
{
    const unsigned long long ticks =
        (static_cast<unsigned long long>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    return ECTime(static_cast<__time64_t>(ticks / 10000000ULL) - 11644473600LL);
}
} // namespace

BOOL ECFileFind::GetCreationTime(ECTime& refTime) const
{
    FILETIME ft{};
    if (!GetCreationTime(&ft))
        return FALSE;
    refTime = TimeFromFileTime(ft);
    return TRUE;
}
#else
// POSIX has no portable creation ("birth") time: statx/STATX_BTIME exists
// on recent Linux but is not answered by every filesystem, so reporting
// failure is the honest answer rather than substituting another stamp.
BOOL ECFileFind::GetCreationTime(ECTime& /*refTime*/) const { return FALSE; }
#endif

// The FILETIME forms. FILETIME is a Windows type, so off Windows these
// have nothing to fill in and report failure, exactly like the creation/
// access times above.
#ifdef _WIN32
BOOL ECFileFind::GetLastWriteTime(FILETIME* pTimeStamp) const
{
    if (pTimeStamp == nullptr)
        return FALSE;
    WIN32_FILE_ATTRIBUTE_DATA data{};
    if (!::GetFileAttributesExW(m_current.path().wstring().c_str(), GetFileExInfoStandard, &data))
        return FALSE;
    *pTimeStamp = data.ftLastWriteTime;
    return TRUE;
}
BOOL ECFileFind::GetCreationTime(FILETIME* pTimeStamp) const
{
    if (pTimeStamp == nullptr)
        return FALSE;
    WIN32_FILE_ATTRIBUTE_DATA data{};
    if (!::GetFileAttributesExW(m_current.path().wstring().c_str(), GetFileExInfoStandard, &data))
        return FALSE;
    *pTimeStamp = data.ftCreationTime;
    return TRUE;
}
BOOL ECFileFind::GetLastAccessTime(FILETIME* pTimeStamp) const
{
    if (pTimeStamp == nullptr)
        return FALSE;
    WIN32_FILE_ATTRIBUTE_DATA data{};
    if (!::GetFileAttributesExW(m_current.path().wstring().c_str(), GetFileExInfoStandard, &data))
        return FALSE;
    *pTimeStamp = data.ftLastAccessTime;
    return TRUE;
}
#else
BOOL ECFileFind::GetLastWriteTime(FILETIME*) const { return FALSE; }
BOOL ECFileFind::GetCreationTime(FILETIME*) const { return FALSE; }
BOOL ECFileFind::GetLastAccessTime(FILETIME*) const { return FALSE; }
#endif
#ifdef _WIN32
BOOL ECFileFind::GetLastAccessTime(ECTime& refTime) const
{
    FILETIME ft{};
    if (!GetLastAccessTime(&ft))
        return FALSE;
    refTime = TimeFromFileTime(ft);
    return TRUE;
}
#else
// Unlike the creation time, the last-access time IS universally available
// off Windows -- st_atime has been in stat(2) since the beginning. It is
// only std::filesystem that does not surface it.
BOOL ECFileFind::GetLastAccessTime(ECTime& refTime) const
{
    struct stat st{};
    if (::stat(m_current.path().c_str(), &st) != 0)
        return FALSE;
    refTime = ECTime(static_cast<__time64_t>(st.st_atime));
    return TRUE;
}
#endif

BOOL ECFileFind::IsTemporary() const
{
#ifdef _WIN32
    return HasFileAttribute(m_current.path(), FILE_ATTRIBUTE_TEMPORARY);
#else
    return FALSE;
#endif
}

BOOL ECFileFind::IsArchived() const
{
#ifdef _WIN32
    return HasFileAttribute(m_current.path(), FILE_ATTRIBUTE_ARCHIVE);
#else
    return FALSE;
#endif
}

BOOL ECFileFind::IsCompressed() const
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
BOOL ECFileFind::IsSystem() const { return HasFileAttribute(m_current.path(), FILE_ATTRIBUTE_SYSTEM); }
BOOL ECFileFind::IsHidden() const { return HasFileAttribute(m_current.path(), FILE_ATTRIBUTE_HIDDEN); }
BOOL ECFileFind::IsReadOnly() const { return HasFileAttribute(m_current.path(), FILE_ATTRIBUTE_READONLY); }
#else
BOOL ECFileFind::IsSystem() const { return FALSE; }
BOOL ECFileFind::IsHidden() const { return FALSE; }
BOOL ECFileFind::IsReadOnly() const
{
    std::error_code ec;
    auto perms = std::filesystem::status(m_current.path(), ec).permissions();
    if (ec)
        return FALSE;
    return (perms & std::filesystem::perms::owner_write) == std::filesystem::perms::none ? TRUE : FALSE;
}
#endif

BOOL ECFileFind::IsDots() const
{
    auto name = m_current.path().filename().wstring();
    return (name == L"." || name == L"..") ? TRUE : FALSE;
}

ECString ECFileFind::GetFileName() const { return ECString(m_current.path().filename().wstring().c_str()); }
ECString ECFileFind::GetFilePath() const { return ECString(m_current.path().wstring().c_str()); }

ULONGLONG ECFileFind::GetLength() const
{
    std::error_code ec;
    auto sz = std::filesystem::file_size(m_current.path(), ec);
    return ec ? 0 : sz;
}

// Real MFC's GetRoot returns the directory that is being searched (the
// search string with the file-name/wildcard part stripped but the trailing
// separator kept), NOT the filesystem root (C:\). Captured in FindFile.
ECString ECFileFind::GetRoot() const { return ECString(m_root.c_str()); }

// ---------------------------------------------------------------------
// CArchive — see the class comment in afx.h for the wire-format notes.
// ---------------------------------------------------------------------
ECArchive::ECArchive(ECFile* pFile, UINT nMode, int /*nBufSize*/, void* /*lpBuf*/)
    : m_pFile(pFile), m_nMode(nMode)
{
}

ECArchive::~ECArchive() { Close(); }

BOOL ECArchive::IsLoading() const { return (m_nMode & static_cast<UINT>(ECArchive::load)) ? TRUE : FALSE; }
BOOL ECArchive::IsStoring() const { return IsLoading() ? FALSE : TRUE; }
ECFile* ECArchive::GetFile() const { return m_pFile; }

// Real MFC: "Close does not close the file; it flushes the archive's
// buffer." The underlying CFile stays open and is the caller's to close.
void ECArchive::Close() { Flush(); }
void ECArchive::Flush() { if (m_pFile != nullptr) m_pFile->Flush(); }

UINT ECArchive::Read(void* lpBuf, UINT nMax) { return m_pFile != nullptr ? m_pFile->Read(lpBuf, nMax) : 0; }
void ECArchive::Write(const void* lpBuf, UINT nMax) { if (m_pFile != nullptr) m_pFile->Write(lpBuf, nMax); }

namespace
{
template <class T>
ECArchive& ArReadRaw(ECArchive& ar, T& v) { ar.Read(&v, sizeof(T)); return ar; }
template <class T>
ECArchive& ArWriteRaw(ECArchive& ar, T v) { ar.Write(&v, sizeof(T)); return ar; }
} // namespace

ECArchive& ECArchive::operator>>(BYTE& by) { return ArReadRaw(*this, by); }
ECArchive& ECArchive::operator>>(WORD& w) { return ArReadRaw(*this, w); }
ECArchive& ECArchive::operator>>(int& i) { return ArReadRaw(*this, i); }
ECArchive& ECArchive::operator>>(UINT& u) { return ArReadRaw(*this, u); }
// MFC's archive slot for `long` is 32 bits wide, because on Windows `long`
// IS 32 bits. Off Windows the C++ type is 64 bits, but the WIRE FORMAT is
// not ours to redefine: a file written here has to be readable by a real
// MFC build, and vice versa. So the slot stays 32 bits and the value is
// narrowed/sign-extended across it, exactly as it would be on Windows.
ECArchive& ECArchive::operator>>(long& l)
{
#ifdef _WIN32
    return ArReadRaw(*this, l);
#else
    std::int32_t v = 0;
    Read(&v, sizeof(v));
    l = v;                  // sign-extends, matching a 32-bit LONG load
    return *this;
#endif
}
#ifdef _WIN32
ECArchive& ECArchive::operator>>(DWORD& dw) { return ArReadRaw(*this, dw); }
#endif
ECArchive& ECArchive::operator>>(float& f) { return ArReadRaw(*this, f); }
ECArchive& ECArchive::operator>>(double& d) { return ArReadRaw(*this, d); }
ECArchive& ECArchive::operator>>(ULONGLONG& dwdw) { return ArReadRaw(*this, dwdw); }

ECArchive& ECArchive::operator<<(BYTE by) { return ArWriteRaw(*this, by); }
ECArchive& ECArchive::operator<<(WORD w) { return ArWriteRaw(*this, w); }
ECArchive& ECArchive::operator<<(int i) { return ArWriteRaw(*this, i); }
ECArchive& ECArchive::operator<<(UINT u) { return ArWriteRaw(*this, u); }
ECArchive& ECArchive::operator<<(long l)
{
#ifdef _WIN32
    return ArWriteRaw(*this, l);
#else
    return ArWriteRaw(*this, static_cast<std::int32_t>(l));   // see operator>> above
#endif
}
#ifdef _WIN32
ECArchive& ECArchive::operator<<(DWORD dw) { return ArWriteRaw(*this, dw); }
#endif
ECArchive& ECArchive::operator<<(float f) { return ArWriteRaw(*this, f); }
ECArchive& ECArchive::operator<<(double d) { return ArWriteRaw(*this, d); }
ECArchive& ECArchive::operator<<(ULONGLONG dwdw) { return ArWriteRaw(*this, dwdw); }

// See the wire-format note on the CArchive class in afx.h: a 32-bit
// length prefix followed by that many raw wchar_t, round-trip-correct
// within simple_mfc but not a byte-exact match for real MFC's
// CString::Serialize.
ECArchive& ECArchive::operator>>(ECString& str)
{
    UINT nLen = 0;
    Read(&nLen, sizeof(nLen));
    if (nLen == 0) { str.Empty(); return *this; }
    std::vector<wchar_t> buf(nLen);
    Read(buf.data(), static_cast<UINT>(nLen * sizeof(wchar_t)));
    str.SetString(buf.data(), static_cast<int>(nLen));
    return *this;
}

ECArchive& ECArchive::operator<<(const ECString& str)
{
    UINT nLen = static_cast<UINT>(str.GetLength());
    Write(&nLen, sizeof(nLen));
    if (nLen > 0)
        Write(str.GetString(), static_cast<UINT>(nLen * sizeof(wchar_t)));
    return *this;
}

ECArchiveException::ECArchiveException(int cause, LPCTSTR lpszArchiveName)
    : ECException(TRUE), m_cause(cause), m_strFileName(lpszArchiveName ? lpszArchiveName : L"")
{
}
