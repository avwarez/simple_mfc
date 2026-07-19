#include "afx.h"

#include <algorithm>
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
IMPLEMENT_DYNAMIC(CMemoryException, CSimpleException)
IMPLEMENT_DYNAMIC(CFileException, CException)
IMPLEMENT_DYNAMIC(CFile, CObject)
IMPLEMENT_DYNAMIC(CStdioFile, CFile)
IMPLEMENT_DYNAMIC(CMemFile, CFile)
IMPLEMENT_DYNAMIC(CFileFind, CObject)

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
// CString
// ---------------------------------------------------------------------
wchar_t* CString::GetBuffer(int nMinBufferLength)
{
    if (static_cast<size_t>(nMinBufferLength) > m_data.size())
        m_data.resize(static_cast<size_t>(nMinBufferLength));
    return m_data.data();
}

void CString::ReleaseBuffer(int nNewLength)
{
    if (nNewLength < 0)
        m_data.resize(std::char_traits<wchar_t>::length(m_data.c_str()));
    else
        m_data.resize(static_cast<size_t>(nNewLength));
}

void CString::Format(const wchar_t* pszFormat, ...)
{
    va_list args;
    va_start(args, pszFormat);
    size_t size = 256;
    std::vector<wchar_t> buf(size);
    for (;;)
    {
        va_list argsCopy;
        va_copy(argsCopy, args);
        int n = std::vswprintf(buf.data(), size, pszFormat, argsCopy);
        va_end(argsCopy);
        if (n >= 0 && static_cast<size_t>(n) < size)
        {
            m_data.assign(buf.data(), static_cast<size_t>(n));
            break;
        }
        if (size > (1u << 20)) { m_data.assign(buf.data(), size - 1); break; }
        size *= 2;
        buf.resize(size);
    }
    va_end(args);
}

void CString::AppendFormat(const wchar_t* pszFormat, ...)
{
    va_list args;
    va_start(args, pszFormat);
    size_t size = 256;
    std::vector<wchar_t> buf(size);
    for (;;)
    {
        va_list argsCopy;
        va_copy(argsCopy, args);
        int n = std::vswprintf(buf.data(), size, pszFormat, argsCopy);
        va_end(argsCopy);
        if (n >= 0 && static_cast<size_t>(n) < size)
        {
            m_data.append(buf.data(), static_cast<size_t>(n));
            break;
        }
        if (size > (1u << 20)) { m_data.append(buf.data(), size - 1); break; }
        size *= 2;
        buf.resize(size);
    }
    va_end(args);
}

int CString::CompareNoCase(const wchar_t* psz) const
{
    std::wstring a = m_data, b = psz;
    std::transform(a.begin(), a.end(), a.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
    std::transform(b.begin(), b.end(), b.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
    return a.compare(b);
}

int CString::Delete(int iIndex, int nCount)
{
    if (iIndex < 0 || static_cast<size_t>(iIndex) >= m_data.size()) return GetLength();
    m_data.erase(static_cast<size_t>(iIndex), static_cast<size_t>(nCount));
    return GetLength();
}

int CString::Find(const wchar_t* pszSub, int iStart) const
{
    auto pos = m_data.find(pszSub, static_cast<size_t>(iStart));
    return pos == std::wstring::npos ? -1 : static_cast<int>(pos);
}

int CString::Find(wchar_t ch, int iStart) const
{
    auto pos = m_data.find(ch, static_cast<size_t>(iStart));
    return pos == std::wstring::npos ? -1 : static_cast<int>(pos);
}

int CString::ReverseFind(wchar_t ch) const
{
    auto pos = m_data.rfind(ch);
    return pos == std::wstring::npos ? -1 : static_cast<int>(pos);
}

int CString::Insert(int iIndex, const wchar_t* psz)
{
    size_t i = std::min<size_t>(static_cast<size_t>(iIndex), m_data.size());
    m_data.insert(i, psz);
    return GetLength();
}

int CString::Insert(int iIndex, wchar_t ch)
{
    size_t i = std::min<size_t>(static_cast<size_t>(iIndex), m_data.size());
    m_data.insert(i, 1, ch);
    return GetLength();
}

CString CString::Left(int nCount) const
{
    nCount = std::clamp(nCount, 0, GetLength());
    return CString(m_data.substr(0, static_cast<size_t>(nCount)).c_str());
}

CString CString::Right(int nCount) const
{
    nCount = std::clamp(nCount, 0, GetLength());
    return CString(m_data.substr(m_data.size() - static_cast<size_t>(nCount)).c_str());
}

CString CString::Mid(int iFirst, int nCount) const
{
    iFirst = std::clamp(iFirst, 0, GetLength());
    nCount = std::clamp(nCount, 0, GetLength() - iFirst);
    return CString(m_data.substr(static_cast<size_t>(iFirst), static_cast<size_t>(nCount)).c_str());
}

CString CString::Mid(int iFirst) const { return Mid(iFirst, GetLength() - iFirst); }

CString& CString::MakeLower()
{
    std::transform(m_data.begin(), m_data.end(), m_data.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
    return *this;
}

CString& CString::MakeUpper()
{
    std::transform(m_data.begin(), m_data.end(), m_data.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towupper(c)); });
    return *this;
}

int CString::Replace(const wchar_t* pszOld, const wchar_t* pszNew)
{
    std::wstring oldS = pszOld, newS = pszNew;
    if (oldS.empty()) return 0;
    int count = 0;
    size_t pos = 0;
    while ((pos = m_data.find(oldS, pos)) != std::wstring::npos)
    {
        m_data.replace(pos, oldS.size(), newS);
        pos += newS.size();
        ++count;
    }
    return count;
}

int CString::Replace(wchar_t chOld, wchar_t chNew)
{
    int count = 0;
    for (auto& c : m_data) if (c == chOld) { c = chNew; ++count; }
    return count;
}

CString CString::SpanExcluding(const wchar_t* pszCharSet) const
{
    auto pos = m_data.find_first_of(pszCharSet);
    return pos == std::wstring::npos ? *this : Left(static_cast<int>(pos));
}

CString CString::Tokenize(const wchar_t* pszTokens, int& iStart) const
{
    if (iStart < 0 || static_cast<size_t>(iStart) >= m_data.size()) { iStart = -1; return CString(); }
    size_t begin = m_data.find_first_not_of(pszTokens, static_cast<size_t>(iStart));
    if (begin == std::wstring::npos) { iStart = -1; return CString(); }
    size_t end = m_data.find_first_of(pszTokens, begin);
    CString tok(m_data.substr(begin, end == std::wstring::npos ? std::wstring::npos : end - begin).c_str());
    iStart = (end == std::wstring::npos) ? -1 : static_cast<int>(end + 1);
    return tok;
}

CString& CString::Trim()
{
    return Trim(L" \t\r\n");
}

CString& CString::Trim(wchar_t chTarget)
{
    wchar_t set[2] = {chTarget, 0};
    return Trim(set);
}

CString& CString::Trim(const wchar_t* pszTargets)
{
    size_t b = m_data.find_first_not_of(pszTargets);
    if (b == std::wstring::npos) m_data.clear();
    else m_data.erase(0, b);
    return TrimRight(pszTargets);
}

CString& CString::TrimRight()
{
    return TrimRight(L" \t\r\n");
}

CString& CString::TrimRight(wchar_t chTarget)
{
    wchar_t set[2] = {chTarget, 0};
    return TrimRight(set);
}

CString& CString::TrimRight(const wchar_t* pszTargets)
{
    size_t e = m_data.find_last_not_of(pszTargets);
    if (e == std::wstring::npos) m_data.clear();
    else m_data.erase(e + 1);
    return *this;
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
