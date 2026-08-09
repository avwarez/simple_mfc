// io.h -- POSIX stand-in for the MSVC CRT's low-level I/O header.
//
// PART OF THE Win32 PLATFORM SHIM, NOT OF THE MFC INTERFACE.
//
// Every function here resolves to the NATIVE Linux call. MSVC's <io.h> is
// essentially POSIX with an underscore bolted on (_read/_write/_close are
// literally read/write/close), so this file is mostly a renaming layer -- and
// where it is more than that, it is because the two platforms genuinely
// disagree, never because a behaviour was faked. The three cases that are
// more than a rename are called out individually below.
#pragma once

#ifdef _WIN32
#error "simple_mfc/platform is for non-Windows builds only."
#endif

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstdio>
#include <cwchar>
#include <string>

// ---------------------------------------------------------------------
// Straight renames onto the native calls.
// ---------------------------------------------------------------------
#ifndef _read
#define _read       ::read
#define _write      ::write
#define _close      ::close
#define _lseek      ::lseek
#define _tell(fd)   ::lseek((fd), 0, SEEK_CUR)
#define _dup        ::dup
#define _dup2       ::dup2
#define _isatty     ::isatty
#define _unlink     ::unlink
#define _access     ::access
#define _fileno     ::fileno
#define _chsize     ::ftruncate
#define _umask      ::umask
#endif

// Windows tracks 64-bit file offsets through a separate _i64 family because
// its `long` is 32 bits. On LP64 Linux off_t is already 64 bits, so the wide
// variants are the same call.
#ifndef _lseeki64
#define _lseeki64   ::lseek
#define _telli64(fd) ::lseek((fd), 0, SEEK_CUR)
#define _chsize_s   ::ftruncate
#endif

// ---------------------------------------------------------------------
// _commit -- flush this descriptor's data to the disk.
//
// fsync is the exact native counterpart, including the return convention
// (0 / -1), so this is a rename too.
// ---------------------------------------------------------------------
inline int _commit(int fd) { return ::fsync(fd); }

// ---------------------------------------------------------------------
// The three genuine divergences.
// ---------------------------------------------------------------------

// 1. _filelength has no POSIX twin: the native way to ask a descriptor's size
//    is fstat, not a seek-based dance. Note it must NOT be implemented by
//    seeking to the end -- that would move the file pointer, which
//    _filelength does not do, and eMule calls it mid-read.
inline long long _filelengthi64(int fd)
{
    struct stat st;
    if (::fstat(fd, &st) != 0)
        return -1;
    return static_cast<long long>(st.st_size);
}
inline long _filelength(int fd)
{
    const long long n = _filelengthi64(fd);
    return (n < 0) ? -1L : static_cast<long>(n);
}

// 2. _setmode switches a descriptor between text and binary mode, i.e. it
//    turns CRLF translation on and off. Linux does no translation at all, so
//    there is nothing to switch: the honest implementation is to accept the
//    call and report the mode that is always in force. Returning the previous
//    mode (_O_BINARY) rather than 0 matters -- 0 is a legal mode value and
//    callers that save and restore would get it wrong.
#ifndef _O_BINARY
#define _O_BINARY 0
#define _O_TEXT   0
#define _O_RDONLY O_RDONLY
#define _O_WRONLY O_WRONLY
#define _O_RDWR   O_RDWR
#define _O_APPEND O_APPEND
#define _O_CREAT  O_CREAT
#define _O_TRUNC  O_TRUNC
#define _O_EXCL   O_EXCL
#endif
inline int _setmode(int fd, int /*mode*/)
{
    return (::fcntl(fd, F_GETFL) < 0) ? -1 : _O_BINARY;
}

// 3. _get_osfhandle converts a CRT descriptor into the underlying Win32 kernel
//    HANDLE. On Linux the descriptor IS the kernel object -- there is no
//    second representation to convert to -- so this is the identity, widened
//    to the handle type. It is declared here rather than left out because
//    eMule passes the result straight back into calls that take a descriptor.
inline std::intptr_t _get_osfhandle(int fd)
{
    return (::fcntl(fd, F_GETFD) < 0) ? -1 : static_cast<std::intptr_t>(fd);
}
