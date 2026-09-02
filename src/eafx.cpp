#include "eafx.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cwctype>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cerrno>
#include <map>
#include <mutex>
#ifdef _MSC_VER
#include <share.h>
#endif
#ifndef _WIN32
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#if !defined(_WIN32) && defined(F_OFD_SETLK) && defined(F_OFD_SETLKW)
#define ESIMPLE_MFC_CROSS_PROCESS_SHARE 1
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
ECDumpContext& ECDumpContext::operator<<(LPCTSTR lpsz)
{
    if (lpsz)
        m_os << mfc_detail::WideToWide<wchar_t>(lpsz, std::char_traits<TCHAR>::length(lpsz));
    return *this;
}
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
    LPCTSTR msg = _T("Unsupported operation.");
    size_t n = std::min<size_t>(nMaxError - 1, std::char_traits<TCHAR>::length(msg));
    std::char_traits<TCHAR>::copy(lpszError, msg, n);
    lpszError[n] = _T('\0');
    return TRUE;
}

BOOL ECMemoryException::GetErrorMessage(LPTSTR lpszError, UINT nMaxError, UINT* pnHelpContext) const
{
    if (pnHelpContext) *pnHelpContext = 0;
    if (!lpszError || nMaxError == 0) return FALSE;
    LPCTSTR msg = _T("Out of memory.");
    size_t n = std::min<size_t>(nMaxError - 1, std::char_traits<TCHAR>::length(msg));
    std::char_traits<TCHAR>::copy(lpszError, msg, n);
    lpszError[n] = _T('\0');
    return TRUE;
}

BOOL ECFileException::GetErrorMessage(LPTSTR lpszError, UINT nMaxError, UINT* pnHelpContext) const
{
    if (pnHelpContext) *pnHelpContext = 0;
    if (!lpszError || nMaxError == 0) return FALSE;
    std::basic_string<TCHAR> msg;
    switch (m_cause)
    {
        case fileNotFound: msg = _T("File not found."); break;
        case badPath: msg = _T("Invalid path."); break;
        case tooManyOpenFiles: msg = _T("Too many open files."); break;
        case accessDenied: msg = _T("Access denied."); break;
        case diskFull: msg = _T("Disk full."); break;
        case endOfFile: msg = _T("Unexpected end of file."); break;
        case sharingViolation: msg = _T("Sharing violation."); break;
        default: msg = _T("File error."); break;
    }
    if (!m_strFileName.IsEmpty())
    {
        msg += _T(" (");
        msg += m_strFileName.GetString();
        msg += _T(")");
    }
    size_t n = std::min<size_t>(nMaxError - 1, msg.size());
    std::char_traits<TCHAR>::copy(lpszError, msg.c_str(), n);
    lpszError[n] = _T('\0');
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

#ifdef _DEBUG
void EAFXAPI EAfxAssertValidObject(const ECObject* pOb, const char*  , int  )
{
    EASSERT(pOb != nullptr);
    if (pOb == nullptr) return;
    pOb->AssertValid();
}
#endif

[[noreturn]] void EAfxThrowFileException(int cause, LONG lOsError, LPCTSTR lpszFileName)
{
    throw new ECFileException(cause, lOsError, lpszFileName);
}

[[noreturn]] void EAfxThrowMemoryException()
{
    static ECMemoryException s_oom;
    throw &s_oom;
}

namespace mfc_detail
{
struct ShareEntry
{
    const void* owner;
    bool reads;
    bool writes;
    bool permitsRead;
    bool permitsWrite;
    int lockFd = -1;
};

#ifdef ESIMPLE_MFC_CROSS_PROCESS_SHARE
enum ShareLockByte { kShareGuard = 0, kShareHasRead, kShareHasWrite, kShareDenyRead, kShareDenyWrite };

enum ShareLockResult { kShareGranted, kShareConflict, kShareUnavailable };

const off_t kShareLockBase = 0x40000000;

ShareLockResult LockShareByte(int fd, int which, short type, bool wait)
{
    struct flock lk = {};
    lk.l_type = type;
    lk.l_whence = SEEK_SET;
    lk.l_start = kShareLockBase + which;
    lk.l_len = 1;
    while (::fcntl(fd, wait ? F_OFD_SETLKW : F_OFD_SETLK, &lk) == -1)
    {
        if (errno == EINTR) continue;
        return errno == EACCES || errno == EAGAIN ? kShareConflict : kShareUnavailable;
    }
    return kShareGranted;
}

int OpenShareLockFd(const std::filesystem::path& path)
{
    return ::open(path.c_str(), O_RDWR | O_CLOEXEC);
}

ShareLockResult ClaimShareBytes(int fd, const ShareEntry& want)
{
    const ShareLockResult guard = LockShareByte(fd, kShareGuard, F_WRLCK, true);
    if (guard != kShareGranted) return kShareUnavailable;

    const struct { bool applies; int byte; } probes[] = {
        { want.reads,         kShareDenyRead  },
        { want.writes,        kShareDenyWrite },
        { !want.permitsRead,  kShareHasRead   },
        { !want.permitsWrite, kShareHasWrite  },
    };
    ShareLockResult outcome = kShareGranted;
    for (const auto& probe : probes)
    {
        if (!probe.applies) continue;
        outcome = LockShareByte(fd, probe.byte, F_WRLCK, false);
        if (outcome != kShareGranted) break;
        LockShareByte(fd, probe.byte, F_UNLCK, false);
    }

    if (outcome == kShareGranted)
    {
        const struct { bool applies; int byte; } claims[] = {
            { want.reads,         kShareHasRead   },
            { want.writes,        kShareHasWrite  },
            { !want.permitsRead,  kShareDenyRead  },
            { !want.permitsWrite, kShareDenyWrite },
        };
        for (const auto& claim : claims)
        {
            if (!claim.applies) continue;
            outcome = LockShareByte(fd, claim.byte, F_RDLCK, false);
            if (outcome != kShareGranted) break;
        }
    }

    LockShareByte(fd, kShareGuard, F_UNLCK, false);
    return outcome;
}

bool AcquireShareLock(ShareEntry& want, const std::filesystem::path& path)
{
    want.lockFd = OpenShareLockFd(path);
    if (want.lockFd == -1) return true;

    const ShareLockResult outcome = ClaimShareBytes(want.lockFd, want);
    if (outcome == kShareGranted) return true;

    ::close(want.lockFd);
    want.lockFd = -1;
    return outcome != kShareConflict;
}
#else
bool AcquireShareLock(ShareEntry&, const std::filesystem::path&) { return true; }
#endif

class ShareRegistry
{
public:
    static ShareRegistry& Instance()
    {
        static ShareRegistry instance;
        return instance;
    }

    bool Acquire(const std::basic_string<TCHAR>& key, const std::filesystem::path& path,
                 ShareEntry want)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const auto it = m_open.find(key);
        if (it != m_open.end())
            for (const ShareEntry& held : it->second)
            {
                if (want.reads && !held.permitsRead) return false;
                if (want.writes && !held.permitsWrite) return false;
                if (held.reads && !want.permitsRead) return false;
                if (held.writes && !want.permitsWrite) return false;
            }
        if (!AcquireShareLock(want, path)) return false;
        m_open[key].push_back(want);
        return true;
    }

    bool AttachLock(const void* owner, const std::filesystem::path& path)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& entry : m_open)
            for (ShareEntry& held : entry.second)
                if (held.owner == owner && held.lockFd == -1)
                    return AcquireShareLock(held, path);
        return true;
    }

    void Release(const void* owner)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto it = m_open.begin(); it != m_open.end();)
        {
            std::vector<ShareEntry>& held = it->second;
#ifdef ESIMPLE_MFC_CROSS_PROCESS_SHARE
            for (const ShareEntry& e : held)
                if (e.owner == owner && e.lockFd != -1) ::close(e.lockFd);
#endif
            held.erase(std::remove_if(held.begin(), held.end(),
                                      [owner](const ShareEntry& e) { return e.owner == owner; }),
                       held.end());
            if (held.empty()) it = m_open.erase(it);
            else ++it;
        }
    }

private:
    std::mutex m_mutex;
    std::map<std::basic_string<TCHAR>, std::vector<ShareEntry>> m_open;
};

std::basic_string<TCHAR> ShareKey(const std::filesystem::path& path)
{
    std::error_code ec;
    const std::filesystem::path resolved = std::filesystem::weakly_canonical(path, ec);
    return (ec ? path : resolved).string<TCHAR>();
}

void ReleaseShare(const void* owner)
{
    ShareRegistry::Instance().Release(owner);
}

bool AttachShare(const void* owner, const std::filesystem::path& path)
{
    return ShareRegistry::Instance().AttachLock(owner, path);
}

int FileOpenCause(int err, const std::filesystem::path& path, long dosError = 0)
{
    if (dosError == ESIMPLE_MFC_ERROR_SHARING_VIOLATION)
        return ECFileException::sharingViolation;
    if (err == EACCES || err == EPERM || err == EROFS || err == EISDIR)
        return ECFileException::accessDenied;
    if (err == EMFILE || err == ENFILE)
        return ECFileException::tooManyOpenFiles;
    if (err == ENOSPC)
        return ECFileException::diskFull;
    if (err == ENAMETOOLONG || err == ENOTDIR)
        return ECFileException::badPath;

    std::error_code ec;
    const std::filesystem::path parent = path.parent_path();
    if (!parent.empty() && !std::filesystem::exists(parent, ec))
        return ECFileException::badPath;
    if (!std::filesystem::exists(path, ec))
        return ECFileException::fileNotFound;
    return ECFileException::accessDenied;
}

LONG FileOsError(int err, int cause)
{
    if (cause == ECFileException::sharingViolation)
        return ESIMPLE_MFC_ERROR_SHARING_VIOLATION;
    if (err != 0) return static_cast<LONG>(err);
    switch (cause)
    {
    case ECFileException::fileNotFound:
    case ECFileException::badPath:      return static_cast<LONG>(ENOENT);
    case ECFileException::accessDenied: return static_cast<LONG>(EACCES);
    case ECFileException::diskFull:     return static_cast<LONG>(ENOSPC);
    default:                            return 1;
    }
}
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

    mfc_detail::ReleaseShare(this);

    m_path = lpszFileName ? lpszFileName : _T("");
    const std::filesystem::path path(m_path);

    std::error_code ec;
    if (std::filesystem::is_directory(path, ec))
    {
        if (pError)
            *pError = ECFileException(ECFileException::badPath,
                                      static_cast<LONG>(EISDIR), lpszFileName);
        return FALSE;
    }

    const UINT access = nOpenFlags & 3;
    const UINT share = nOpenFlags & 0x70;
    mfc_detail::ShareEntry want;
    want.owner = this;
    want.reads = access == modeRead || access == modeReadWrite;
    want.writes = access == modeWrite || access == modeReadWrite;
    want.permitsRead = share == shareDenyWrite || share == shareDenyNone;
    want.permitsWrite = share == shareDenyRead || share == shareDenyNone;

    const std::basic_string<TCHAR> key = mfc_detail::ShareKey(path);
    if (!mfc_detail::ShareRegistry::Instance().Acquire(key, path, want))
    {
        if (pError)
            *pError = ECFileException(ECFileException::sharingViolation,
                                      ESIMPLE_MFC_ERROR_SHARING_VIOLATION, lpszFileName);
        return FALSE;
    }

    errno = 0;
    m_stream.clear();
#ifdef _MSC_VER
    m_stream.open(path, mode, share == shareCompat ? _SH_DENYRW : static_cast<int>(share));
#else
    m_stream.open(path, mode);
#endif
    if (!m_stream.is_open())
    {
        const int err = errno;
#ifdef _WIN32
        const long dosError = static_cast<long>(_doserrno);
#else
        const long dosError = 0;
#endif
        const int cause = mfc_detail::FileOpenCause(err, path, dosError);
        mfc_detail::ReleaseShare(this);
        if (pError)
            *pError = ECFileException(cause, mfc_detail::FileOsError(err, cause), lpszFileName);
        return FALSE;
    }

    if (!mfc_detail::AttachShare(this, path))
    {
        m_stream.close();
        mfc_detail::ReleaseShare(this);
        if (pError)
            *pError = ECFileException(ECFileException::sharingViolation,
                                      ESIMPLE_MFC_ERROR_SHARING_VIOLATION, lpszFileName);
        return FALSE;
    }

    m_nOpenFlags = nOpenFlags;
    m_strFileName = m_path.c_str();
    return TRUE;
}

ECFile::ECFile(LPCTSTR lpszFileName, UINT nOpenFlags)
{
    ECFileException error;
    if (!Open(lpszFileName, nOpenFlags, &error))
        EAfxThrowFileException(error.m_cause, error.m_lOsError, lpszFileName);
}

UINT ECFile::Read(void* lpBuf, UINT nCount)
{
    m_stream.read(static_cast<char*>(lpBuf), nCount);
    const std::streamsize got = m_stream.gcount();
    if (m_stream.eof()) m_stream.clear();
    return static_cast<UINT>(got);
}

void ECFile::Write(const void* lpBuf, UINT nCount)
{
    if (!(m_nOpenFlags & (modeWrite | modeReadWrite | modeCreate)))
        EAfxThrowFileException(ECFileException::accessDenied,
                               static_cast<LONG>(EACCES), m_path.c_str());
    errno = 0;
    m_stream.write(static_cast<const char*>(lpBuf), nCount);
    if (!m_stream)
    {
        const int err = errno;
        m_stream.clear();
        const int cause = err == ENOSPC   ? ECFileException::diskFull
                        : err == EACCES || err == EBADF || err == EPERM
                                          ? ECFileException::accessDenied
                                          : ECFileException::genericException;
        EAfxThrowFileException(cause, mfc_detail::FileOsError(err, cause), m_path.c_str());
    }
}

ULONGLONG ECFile::Seek(LONGLONG lOff, UINT nFrom)
{
    m_stream.clear();

    LONGLONG base = 0;
    if (nFrom == current) base = static_cast<LONGLONG>(GetPosition());
    else if (nFrom == end) base = static_cast<LONGLONG>(GetLength());
    const LONGLONG target = base + lOff;
    if (target < 0)
        EAfxThrowFileException(ECFileException::badSeek,
                               static_cast<LONG>(EINVAL), m_path.c_str());

    m_stream.clear();
    m_stream.seekg(static_cast<std::streamoff>(target), std::ios::beg);
    if (!m_stream)
    {
        m_stream.clear();
        EAfxThrowFileException(ECFileException::badSeek,
                               static_cast<LONG>(EINVAL), m_path.c_str());
    }
    m_stream.seekp(m_stream.tellg());
    return GetPosition();
}

ULONGLONG ECFile::GetLength() const
{
    auto* self = const_cast<ECFile*>(this);
    self->m_stream.clear();
    const auto cur = self->m_stream.tellg();
    self->m_stream.seekg(0, std::ios::end);
    const auto len = self->m_stream.tellg();
    self->m_stream.clear();
    self->m_stream.seekg(cur);
    return len < 0 ? 0 : static_cast<ULONGLONG>(len);
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
    self->m_stream.clear();
    const auto pos = self->m_stream.tellg();
    return pos < 0 ? 0 : static_cast<ULONGLONG>(pos);
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
    const std::filesystem::path path(lpszFileName ? lpszFileName : _T(""));
    std::error_code ec;
    if (!std::filesystem::remove(path, ec) || ec)
    {
        const int err = ec ? ec.value() : ENOENT;
        const int cause = mfc_detail::FileOpenCause(err, path);
        EAfxThrowFileException(cause, mfc_detail::FileOsError(err, cause), lpszFileName);
    }
}

void ECFile::Rename(LPCTSTR lpszOldName, LPCTSTR lpszNewName)
{
    const std::filesystem::path from(lpszOldName ? lpszOldName : _T(""));
    const std::filesystem::path to(lpszNewName ? lpszNewName : _T(""));
    std::error_code ec;
    std::filesystem::rename(from, to, ec);
    if (ec)
    {
        const int cause = mfc_detail::FileOpenCause(ec.value(), from);
        EAfxThrowFileException(cause, mfc_detail::FileOsError(ec.value(), cause), lpszOldName);
    }
}

bool ECStdioFile::IsTextMode() const
{
    return (m_nOpenFlags & typeText) != 0;
}

LPTSTR ECStdioFile::ReadString(LPTSTR lpsz, UINT nMax)
{
    if (nMax == 0) return nullptr;

    UINT count = 0;
    bool any = false;
    if (IsTextMode())
    {
        char c;
        while (count + 1 < nMax && m_stream.read(&c, 1))
        {
            any = true;
            if (c == '\r' && m_stream.peek() == '\n') continue;
            lpsz[count++] = static_cast<TCHAR>(static_cast<unsigned char>(c));
            if (c == '\n') break;
        }
        lpsz[count] = _T('\0');
        return any ? lpsz : nullptr;
    }

    TCHAR ch;
    while (count + 1 < nMax && m_stream.read(reinterpret_cast<char*>(&ch), sizeof(TCHAR)))
    {
        any = true;
        lpsz[count++] = ch;
        if (ch == _T('\n')) break;
    }
    lpsz[count] = _T('\0');
    return any ? lpsz : nullptr;
}

BOOL ECStdioFile::ReadString(ECString& rString)
{
    if (IsTextMode())
    {
        std::string line;
        char c;
        bool any = false;
        while (m_stream.read(&c, 1))
        {
            any = true;
            if (c == '\r')
            {
                if (m_stream.peek() == '\n') continue;
                line += c;
                continue;
            }
            if (c == '\n') break;
            line += c;
        }
        const std::basic_string<TCHAR> wide = mfc_detail::Widen<TCHAR>(line.data(), line.size());
        rString = wide.c_str();
        return any ? TRUE : FALSE;
    }

    TCHAR ch;
    std::basic_string<TCHAR> line;
    bool any = false;
    while (m_stream.read(reinterpret_cast<char*>(&ch), sizeof(TCHAR)))
    {
        any = true;
        if (ch == _T('\n')) break;
        line += ch;
    }
    rString = line.c_str();
    return any ? TRUE : FALSE;
}

void ECStdioFile::WriteString(LPCTSTR lpsz)
{
    if (!lpsz) return;
    const size_t length = std::char_traits<TCHAR>::length(lpsz);
    if (!IsTextMode())
    {
        m_stream.write(reinterpret_cast<const char*>(lpsz),
                       static_cast<std::streamsize>(length * sizeof(TCHAR)));
        return;
    }

    const std::string narrow = mfc_detail::Narrow(lpsz, length);
    std::string translated;
    translated.reserve(narrow.size() + 8);
    for (size_t i = 0; i < narrow.size(); ++i)
    {
        if (narrow[i] == '\n' && (i == 0 || narrow[i - 1] != '\r'))
            translated += '\r';
        translated += narrow[i];
    }
    m_stream.write(translated.data(), static_cast<std::streamsize>(translated.size()));
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
using TString = std::basic_string<TCHAR>;

bool WildcardMatch(const TString& pattern, const TString& name)
{
    size_t p = 0, n = 0, star = TString::npos, mark = 0;
    while (n < name.size())
    {
        if (p < pattern.size() && (pattern[p] == _T('?') || pattern[p] == name[n])) { ++p; ++n; }
        else if (p < pattern.size() && pattern[p] == _T('*')) { star = p++; mark = n; }
        else if (star != TString::npos) { p = star + 1; n = ++mark; }
        else return false;
    }
    while (p < pattern.size() && pattern[p] == _T('*')) ++p;
    return p == pattern.size();
}
}

BOOL ECFileFind::FindFile(LPCTSTR pstrName, DWORD  )
{
    std::filesystem::path spec = pstrName && *pstrName ? std::filesystem::path(pstrName) : std::filesystem::path(_T("*"));
    m_dir = spec.has_parent_path() ? spec.parent_path() : std::filesystem::path(_T("."));
    std::basic_string<TCHAR> pattern = spec.filename().string<TCHAR>();
    if (pattern.empty()) pattern = _T("*");

    std::basic_string<TCHAR> specStr = spec.string<TCHAR>();
    std::basic_string<TCHAR> fname = spec.filename().string<TCHAR>();
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
        if (WildcardMatch(m_pattern, entry.path().filename().string<TCHAR>()))
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
#else
BOOL ECFileFind::GetLastWriteTime(FILETIME*) const { return FALSE; }
#endif
BOOL ECFileFind::IsTemporary() const
{
#ifdef _WIN32
    return HasFileAttribute(m_current.path(), FILE_ATTRIBUTE_TEMPORARY);
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
#else
BOOL ECFileFind::IsSystem() const { return FALSE; }
BOOL ECFileFind::IsHidden() const { return FALSE; }
#endif

BOOL ECFileFind::IsDots() const
{
    auto name = m_current.path().filename().string<TCHAR>();
    return (name == _T(".") || name == _T("..")) ? TRUE : FALSE;
}

ECString ECFileFind::GetFileName() const { return ECString(m_current.path().filename().string<TCHAR>().c_str()); }
ECString ECFileFind::GetFilePath() const { return ECString(m_current.path().string<TCHAR>().c_str()); }

ULONGLONG ECFileFind::GetLength() const
{
    std::error_code ec;
    auto sz = std::filesystem::file_size(m_current.path(), ec);
    return ec ? 0 : sz;
}

ECString ECFileFind::GetRoot() const { return ECString(m_root.c_str()); }

ECArchive::ECArchive(ECFile* pFile, UINT nMode, int nBufSize, void* lpBuf)
    : m_pFile(pFile), m_nMode(nMode), m_bUserBuf(lpBuf != nullptr),
      m_nBufSize(nBufSize > 0 ? nBufSize : 4096),
      m_lpBufStart(nullptr), m_lpBufCur(nullptr), m_lpBufMax(nullptr)
{
    m_lpBufStart = m_bUserBuf ? static_cast<BYTE*>(lpBuf)
                              : new BYTE[static_cast<size_t>(m_nBufSize)];
    m_lpBufCur = m_lpBufStart;
    m_lpBufMax = IsStoring() ? m_lpBufStart + m_nBufSize : m_lpBufStart;
}

ECArchive::~ECArchive()
{
    if (m_pFile != nullptr)
        Close();
    if (!m_bUserBuf)
        delete[] m_lpBufStart;
    m_lpBufStart = nullptr;
    m_lpBufCur = nullptr;
    m_lpBufMax = nullptr;
}

ECFile* ECArchive::GetFile() const { return m_pFile; }

void ECArchive::Close()
{
    Flush();
    m_pFile = nullptr;
}

void ECArchive::FlushBuffer()
{
    if (m_pFile == nullptr)
        return;
    if (IsStoring() && m_lpBufCur != m_lpBufStart)
        m_pFile->Write(m_lpBufStart, static_cast<UINT>(m_lpBufCur - m_lpBufStart));
    m_lpBufCur = m_lpBufStart;
    m_lpBufMax = IsStoring() ? m_lpBufStart + m_nBufSize : m_lpBufStart;
}

void ECArchive::Flush()
{
    if (m_pFile == nullptr)
        return;
    FlushBuffer();
    m_pFile->Flush();
}

bool ECArchive::FillBuffer()
{
    UINT got = m_pFile->Read(m_lpBufStart, static_cast<UINT>(m_nBufSize));
    m_lpBufCur = m_lpBufStart;
    m_lpBufMax = m_lpBufStart + got;
    return got != 0;
}

UINT ECArchive::Read(void* lpBuf, UINT nMax)
{
    if (m_pFile == nullptr || nMax == 0)
        return 0;

    BYTE* p = static_cast<BYTE*>(lpBuf);
    UINT nMostBytes = static_cast<UINT>(m_lpBufMax - m_lpBufCur);
    UINT nTemp = nMax < nMostBytes ? nMax : nMostBytes;
    if (nTemp != 0)
    {
        std::memcpy(p, m_lpBufCur, nTemp);
        m_lpBufCur += nTemp;
    }
    if (nTemp == nMax)
        return nMax;

    p += nTemp;
    UINT nTotal = nTemp;
    UINT nLeft = nMax - nTemp;

    if (nLeft >= static_cast<UINT>(m_nBufSize))
    {
        UINT got = m_pFile->Read(p, nLeft);
        m_lpBufCur = m_lpBufStart;
        m_lpBufMax = m_lpBufStart;
        return nTotal + got;
    }

    if (!FillBuffer())
        return nTotal;

    nMostBytes = static_cast<UINT>(m_lpBufMax - m_lpBufCur);
    nTemp = nLeft < nMostBytes ? nLeft : nMostBytes;
    if (nTemp != 0)
    {
        std::memcpy(p, m_lpBufCur, nTemp);
        m_lpBufCur += nTemp;
    }
    return nTotal + nTemp;
}

void ECArchive::Write(const void* lpBuf, UINT nMax)
{
    if (m_pFile == nullptr || nMax == 0)
        return;

    const BYTE* p = static_cast<const BYTE*>(lpBuf);
    if (m_lpBufCur + nMax > m_lpBufMax)
    {
        FlushBuffer();
        if (nMax >= static_cast<UINT>(m_nBufSize))
        {
            m_pFile->Write(p, nMax);
            return;
        }
    }
    std::memcpy(m_lpBufCur, p, nMax);
    m_lpBufCur += nMax;
}

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

namespace
{
void ArWriteStringLength(ECArchive& ar, ULONGLONG nLength, bool bUnicode)
{
    if (bUnicode)
    {
        ar << static_cast<BYTE>(0xFF);
        ar << static_cast<WORD>(0xFFFE);
    }
    if (nLength < 255)
    {
        ar << static_cast<BYTE>(nLength);
    }
    else if (nLength < 0xFFFE)
    {
        ar << static_cast<BYTE>(0xFF);
        ar << static_cast<WORD>(nLength);
    }
    else if (nLength < 0xFFFFFFFF)
    {
        ar << static_cast<BYTE>(0xFF);
        ar << static_cast<WORD>(0xFFFF);
        ar << static_cast<UINT>(nLength);
    }
    else
    {
        ar << static_cast<BYTE>(0xFF);
        ar << static_cast<WORD>(0xFFFF);
        ar << static_cast<UINT>(0xFFFFFFFF);
        ar << nLength;
    }
}

ULONGLONG ArReadStringLength(ECArchive& ar, bool& bUnicode)
{
    BYTE bLen = 0;
    WORD wLen = 0;
    UINT dwLen = 0;
    ULONGLONG nLen = 0;

    bUnicode = false;
    ar >> bLen;
    if (bLen < 0xFF)
        return bLen;

    ar >> wLen;
    if (wLen == 0xFFFE)
    {
        bUnicode = true;
        ar >> bLen;
        if (bLen < 0xFF)
            return bLen;
        ar >> wLen;
    }
    if (wLen == 0xFFFF)
    {
        ar >> dwLen;
        if (dwLen == 0xFFFFFFFF)
        {
            ar >> nLen;
            return nLen;
        }
        return dwLen;
    }
    return wLen;
}
}

ECArchive& ECArchive::operator>>(ECString& str)
{
    bool bUnicode = false;
    ULONGLONG nLen = ArReadStringLength(*this, bUnicode);
    if (nLen == 0)
    {
        str.Empty();
        return *this;
    }
    size_t n = static_cast<size_t>(nLen);
    if (bUnicode)
    {
        std::vector<TCHAR> buf(n);
        Read(buf.data(), static_cast<UINT>(n * sizeof(TCHAR)));
        str.SetString(buf.data(), static_cast<int>(n));
    }
    else
    {
        std::vector<char> buf(n);
        Read(buf.data(), static_cast<UINT>(n));
        str = ECString(buf.data(), static_cast<int>(n));
    }
    return *this;
}

ECArchive& ECArchive::operator<<(const ECString& str)
{
    int nLen = str.GetLength();
    ArWriteStringLength(*this, static_cast<ULONGLONG>(nLen), sizeof(TCHAR) > 1);
    if (nLen > 0)
        Write(str.GetString(), static_cast<UINT>(static_cast<size_t>(nLen) * sizeof(TCHAR)));
    return *this;
}

ECArchiveException::ECArchiveException(int cause, LPCTSTR lpszArchiveName)
    : ECException(TRUE), m_cause(cause), m_strFileName(lpszArchiveName ? lpszArchiveName : _T(""))
{
}
