#include "eafx.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cwctype>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#ifndef _WIN32
#include <sys/stat.h>
#endif

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

namespace
{
template <class T>
std::wstring DumpFormat(const wchar_t* conversion, T value)
{
    wchar_t buf[64];
    const int n = std::swprintf(buf, sizeof buf / sizeof buf[0], conversion, value);
    return n > 0 ? std::wstring(buf, static_cast<size_t>(n)) : std::wstring();
}
}

ECDumpContext& ECDumpContext::operator<<(const char* lpsz)
{
    if (lpsz) m_os << mfc_detail::Widen<wchar_t>(lpsz, std::strlen(lpsz));
    return *this;
}
ECDumpContext& ECDumpContext::operator<<(LPCTSTR lpsz) { if (lpsz) m_os << lpsz; return *this; }
ECDumpContext& ECDumpContext::operator<<(const ECObject* pOb) { if (pOb) pOb->Dump(*this); else m_os << L"NULL"; return *this; }
ECDumpContext& ECDumpContext::operator<<(int n) { m_os << DumpFormat(L"%d", n); return *this; }
ECDumpContext& ECDumpContext::operator<<(unsigned int u) { m_os << DumpFormat(L"%u", u); return *this; }
ECDumpContext& ECDumpContext::operator<<(long l) { m_os << DumpFormat(L"%ld", l); return *this; }
ECDumpContext& ECDumpContext::operator<<(double d)
{
    std::wstring text = DumpFormat(L"%.15g", d);
    if (text.find_first_of(L".eEnN") == std::wstring::npos)
        text += L'.';
    m_os << text;
    return *this;
}

ECDumpContext& ECDumpContext::operator<<(const void* lp)
{
    static const wchar_t kDigits[] = L"0123456789ABCDEF";
    const std::uintptr_t v = reinterpret_cast<std::uintptr_t>(lp);
    std::wstring text(sizeof(void*) * 2, L'0');
    for (size_t i = 0; i < sizeof(void*) * 2; ++i)
        text[sizeof(void*) * 2 - 1 - i] = kDigits[(v >> (i * 4)) & 0xF];
    m_os << text;
    return *this;
}

void ECObject::Dump(ECDumpContext& dc) const
{
    dc << "a " << GetRuntimeClass()->m_lpszClassName << " at $"
       << static_cast<const void*>(this) << "\n";
}

ECObject* EAfxDynamicDownCast(ECRuntimeClass* pClass, ECObject* pObject)
{
    if (pObject != nullptr && pObject->IsKindOf(pClass))
        return pObject;
    return nullptr;
}

BOOL ECException::GetErrorMessage(LPTSTR lpszError, UINT nMaxError, UINT* pnHelpContext) const
{
    if (pnHelpContext) *pnHelpContext = 0;
    if (lpszError && nMaxError) lpszError[0] = _T('\0');
    return FALSE;
}

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
        msg += m_strFileName.GetString();
        msg += L")";
    }
    size_t n = std::min<size_t>(nMaxError - 1, msg.size());
    std::wmemcpy(lpszError, msg.c_str(), n);
    lpszError[n] = L'\0';
    return TRUE;
}

namespace
{
int OsErrorToCause(LONG lOsError)
{
    switch (lOsError)
    {
        case 2: return ECFileException::fileNotFound;
        case 3: return ECFileException::badPath;
        case 4: return ECFileException::tooManyOpenFiles;
        case 5: return ECFileException::accessDenied;
        case 6: return ECFileException::fileNotFound;
        case 19: return ECFileException::accessDenied;
        case 32: return ECFileException::sharingViolation;
        case 33: return ECFileException::lockViolation;
        case 38: return ECFileException::endOfFile;
        case 39: return ECFileException::diskFull;
        case 112: return ECFileException::diskFull;
        case 15: return ECFileException::badPath;
        case 16: return ECFileException::removeCurrentDir;
        case 123: return ECFileException::badPath;
        case 131: return ECFileException::badSeek;
        case 161: return ECFileException::badPath;
        case 206: return ECFileException::badPath;
        case 80: return ECFileException::accessDenied;
        case 145: return ECFileException::removeCurrentDir;
        case 183: return ECFileException::accessDenied;
        default: return ECFileException::genericException;
    }
}
}

[[noreturn]] void ECFileException::ThrowOsError(LONG lOsError, LPCTSTR lpszFileName)
{
    throw new ECFileException(OsErrorToCause(lOsError), lOsError, lpszFileName);
}

[[noreturn]] void EAfxThrowFileException(int cause, LONG lOsError, LPCTSTR lpszFileName)
{
    throw new ECFileException(cause, lOsError, lpszFileName);
}

[[noreturn]] void EAfxThrowMemoryException()
{
    static ECMemoryException s_oom;
    throw &s_oom;
}

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

void ECMemFile::Attach(BYTE* lpBuffer, UINT nBufferSize, UINT  )
{
    if (lpBuffer != nullptr && nBufferSize > 0)
        m_buffer.assign(lpBuffer, lpBuffer + nBufferSize);
    else
        m_buffer.clear();
    std::free(lpBuffer);
    m_pos = 0;
}

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

BOOL ECFileFind::FindFile(LPCTSTR pstrName, DWORD  )
{
    std::filesystem::path spec = pstrName && *pstrName ? std::filesystem::path(pstrName) : std::filesystem::path(L"*");
    m_dir = spec.has_parent_path() ? spec.parent_path() : std::filesystem::path(L".");
    std::wstring pattern = spec.filename().wstring();
    if (pattern.empty()) pattern = L"*";

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
static BOOL HasFileAttribute(const std::filesystem::path& p, DWORD dwAttr);
#endif

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

#ifdef _WIN32
namespace
{
ECTime TimeFromFileTime(const FILETIME& ft)
{
    const unsigned long long ticks =
        (static_cast<unsigned long long>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    return ECTime(static_cast<__time64_t>(ticks / 10000000ULL) - 11644473600LL);
}
}

BOOL ECFileFind::GetCreationTime(ECTime& refTime) const
{
    FILETIME ft{};
    if (!GetCreationTime(&ft))
        return FALSE;
    refTime = TimeFromFileTime(ft);
    return TRUE;
}
#else
BOOL ECFileFind::GetCreationTime(ECTime&  ) const { return FALSE; }
#endif

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

ECString ECFileFind::GetRoot() const { return ECString(m_root.c_str()); }

ECArchive::ECArchive(ECFile* pFile, UINT nMode, int  , void*  )
    : m_pFile(pFile), m_nMode(nMode)
{
}

ECArchive::~ECArchive() { Close(); }

BOOL ECArchive::IsLoading() const { return (m_nMode & static_cast<UINT>(ECArchive::load)) ? TRUE : FALSE; }
BOOL ECArchive::IsStoring() const { return IsLoading() ? FALSE : TRUE; }
ECFile* ECArchive::GetFile() const { return m_pFile; }

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
}

ECArchive& ECArchive::operator>>(BYTE& by) { return ArReadRaw(*this, by); }
ECArchive& ECArchive::operator>>(WORD& w) { return ArReadRaw(*this, w); }
ECArchive& ECArchive::operator>>(int& i) { return ArReadRaw(*this, i); }
ECArchive& ECArchive::operator>>(UINT& u) { return ArReadRaw(*this, u); }
ECArchive& ECArchive::operator>>(long& l)
{
#ifdef _WIN32
    return ArReadRaw(*this, l);
#else
    std::int32_t v = 0;
    Read(&v, sizeof(v));
    l = v;
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
    return ArWriteRaw(*this, static_cast<std::int32_t>(l));
#endif
}
#ifdef _WIN32
ECArchive& ECArchive::operator<<(DWORD dw) { return ArWriteRaw(*this, dw); }
#endif
ECArchive& ECArchive::operator<<(float f) { return ArWriteRaw(*this, f); }
ECArchive& ECArchive::operator<<(double d) { return ArWriteRaw(*this, d); }
ECArchive& ECArchive::operator<<(ULONGLONG dwdw) { return ArWriteRaw(*this, dwdw); }

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
