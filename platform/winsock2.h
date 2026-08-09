// winsock2.h -- POSIX stand-in for the Windows Sockets 2 SDK header.
//
// PART OF THE Win32 PLATFORM SHIM, NOT OF THE MFC INTERFACE. Nothing here
// belongs to MFC or ATL; this directory reproduces the pieces of the Windows
// SDK that an MFC application reaches past MFC to use directly. It is added to
// the include path ONLY on non-Windows builds -- on Windows the real SDK
// header is found instead and this file is not even visible.
//
// The porting job is much smaller than the name suggests: WinSock2 IS the BSD
// sockets API with a Microsoft accent. socket/bind/listen/accept/connect/
// send/recv/select/htonl/ntohs and the sockaddr structures are all provided
// natively by <sys/socket.h> & co, so they are simply included below rather
// than reimplemented. What this file adds is only the Microsoft-specific
// delta: the SOCKET type (a kernel HANDLE on Windows, a plain fd here), the
// renamed calls (closesocket, ioctlsocket), the WSA error convention (a
// per-thread last-error code with its own numbering, instead of errno), and
// the WSAStartup/WSACleanup library lifecycle, which has no counterpart at
// all on a system where sockets are part of libc.
#pragma once

#ifdef _WIN32
#error "simple_mfc/platform is for non-Windows builds only -- on Windows the real SDK header must be used."
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <cstring>

// ---------------------------------------------------------------------
// The socket handle.
//
// On Windows a SOCKET is an unsigned kernel handle and the failure value is
// the all-bits-set INVALID_SOCKET, which is why Win32 code tests
// "== INVALID_SOCKET" and never "< 0". Here it is a file descriptor, so
// INVALID_SOCKET must be -1 for those same comparisons to keep working:
// making it ~0 would silently break every existing "!= INVALID_SOCKET"
// guard against a failed socket() that returned -1.
// ---------------------------------------------------------------------
using SOCKET = int;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR   (-1)

using SOCKADDR = struct sockaddr;
using PSOCKADDR = struct sockaddr *;
using LPSOCKADDR = struct sockaddr *;
using SOCKADDR_IN = struct sockaddr_in;
using PSOCKADDR_IN = struct sockaddr_in *;
using LPSOCKADDR_IN = struct sockaddr_in *;
using IN_ADDR = struct in_addr;
using PIN_ADDR = struct in_addr *;
using LPIN_ADDR = struct in_addr *;
using LINGER = struct linger;
using PLINGER = struct linger *;
using LPLINGER = struct linger *;
using TIMEVAL = struct timeval;
using PTIMEVAL = struct timeval *;
using LPTIMEVAL = struct timeval *;
using HOSTENT = struct hostent;
using PHOSTENT = struct hostent *;
using LPHOSTENT = struct hostent *;
using SERVENT = struct servent;
using LPSERVENT = struct servent *;
using ADDRINFOT = struct addrinfo;
using ADDRINFOA = struct addrinfo;
using PADDRINFOA = struct addrinfo *;
using LPFD_SET = fd_set *;

// Win32 spells the address union's members with a leading capital S_un /
// S_addr. glibc's struct in_addr has only s_addr, so code written against
// the SDK (eMule does use .S_un.S_addr) needs the alias.
#ifndef s_addr
// (glibc already macro-defines s_addr -> in_addr.s_addr; nothing to do.)
#endif

// ---------------------------------------------------------------------
// Error reporting.
//
// Win32 keeps a per-thread last-error code retrieved with WSAGetLastError()
// and numbered from WSABASEERR (10000). POSIX uses errno with entirely
// different numbers. Mapping the numbers is not enough on its own: eMule
// compares against the WSAE* names, so those names must expand to whatever
// errno actually produces here. Defining WSAEWOULDBLOCK as EAGAIN (rather
// than as 10035) is what makes "if (err == WSAEWOULDBLOCK)" true after a
// non-blocking recv, which is the whole point.
// ---------------------------------------------------------------------
#define WSABASEERR              10000

#define WSAEINTR                EINTR
#define WSAEBADF                EBADF
#define WSAEACCES               EACCES
#define WSAEFAULT               EFAULT
#define WSAEINVAL               EINVAL
#define WSAEMFILE               EMFILE
#define WSAEWOULDBLOCK          EAGAIN
#define WSAEINPROGRESS          EINPROGRESS
#define WSAEALREADY             EALREADY
#define WSAENOTSOCK             ENOTSOCK
#define WSAEDESTADDRREQ         EDESTADDRREQ
#define WSAEMSGSIZE             EMSGSIZE
#define WSAEPROTOTYPE           EPROTOTYPE
#define WSAENOPROTOOPT          ENOPROTOOPT
#define WSAEPROTONOSUPPORT      EPROTONOSUPPORT
#define WSAESOCKTNOSUPPORT      ESOCKTNOSUPPORT
#define WSAEOPNOTSUPP           EOPNOTSUPP
#define WSAEPFNOSUPPORT         EPFNOSUPPORT
#define WSAEAFNOSUPPORT         EAFNOSUPPORT
#define WSAEADDRINUSE           EADDRINUSE
#define WSAEADDRNOTAVAIL        EADDRNOTAVAIL
#define WSAENETDOWN             ENETDOWN
#define WSAENETUNREACH          ENETUNREACH
#define WSAENETRESET            ENETRESET
#define WSAECONNABORTED         ECONNABORTED
#define WSAECONNRESET           ECONNRESET
#define WSAENOBUFS              ENOBUFS
#define WSAEISCONN              EISCONN
#define WSAENOTCONN             ENOTCONN
#define WSAESHUTDOWN            ESHUTDOWN
#define WSAETOOMANYREFS         ETOOMANYREFS
#define WSAETIMEDOUT            ETIMEDOUT
#define WSAECONNREFUSED         ECONNREFUSED
#define WSAELOOP                ELOOP
#define WSAENAMETOOLONG         ENAMETOOLONG
#define WSAEHOSTDOWN            EHOSTDOWN
#define WSAEHOSTUNREACH         EHOSTUNREACH
#define WSAENOTEMPTY            ENOTEMPTY
#define WSAEUSERS               EUSERS
#define WSAEDQUOT               EDQUOT
#define WSAESTALE               ESTALE
#define WSAEREMOTE              EREMOTE

// These have no errno counterpart at all -- they describe the Winsock DLL
// itself, which does not exist here. They keep their real Win32 numbers so
// that nothing can collide with an errno value.
#define WSASYSNOTREADY          (WSABASEERR + 91)
#define WSAVERNOTSUPPORTED      (WSABASEERR + 92)
#define WSANOTINITIALISED       (WSABASEERR + 93)
#define WSAEDISCON              (WSABASEERR + 101)

// Resolver errors: on Win32 these live in the same space as the socket
// errors and are returned by WSAGetLastError() after gethostbyname().
#define WSAHOST_NOT_FOUND       (WSABASEERR + 1001)
#define WSATRY_AGAIN            (WSABASEERR + 1002)
#define WSANO_RECOVERY          (WSABASEERR + 1003)
#define WSANO_DATA              (WSABASEERR + 1004)

inline int WSAGetLastError() { return errno; }
inline void WSASetLastError(int iError) { errno = iError; }

// ---------------------------------------------------------------------
// Renamed calls.
// ---------------------------------------------------------------------
inline int closesocket(SOCKET s) { return ::close(s); }

// Win32's ioctlsocket takes the argument by pointer and only ever handles
// FIONBIO/FIONREAD in practice. FIONBIO is routed to fcntl rather than to
// ioctl: ioctl(FIONBIO) exists on Linux but is not portable across the BSDs,
// while O_NONBLOCK is POSIX.
inline int ioctlsocket(SOCKET s, long cmd, unsigned long *argp)
{
    if (argp == nullptr)
        return SOCKET_ERROR;
    if (cmd == FIONBIO)
    {
        int flags = ::fcntl(s, F_GETFL, 0);
        if (flags < 0)
            return SOCKET_ERROR;
        flags = (*argp != 0) ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
        return (::fcntl(s, F_SETFL, flags) < 0) ? SOCKET_ERROR : 0;
    }
    int value = 0;
    if (::ioctl(s, cmd, &value) < 0)
        return SOCKET_ERROR;
    *argp = static_cast<unsigned long>(value);
    return 0;
}

// ---------------------------------------------------------------------
// Library lifecycle. Sockets are part of libc here, so there is nothing to
// start up or clean up -- but the calls must exist and must succeed, because
// application code treats a WSAStartup failure as fatal.
// ---------------------------------------------------------------------
struct WSAData
{
    unsigned short wVersion;
    unsigned short wHighVersion;
    unsigned short iMaxSockets;
    unsigned short iMaxUdpDg;
    char          *lpVendorInfo;
    char           szDescription[257];
    char           szSystemStatus[129];
};
using WSADATA = WSAData;
using LPWSADATA = WSAData *;

#ifndef MAKEWORD
#define MAKEWORD(a, b) ((unsigned short)(((unsigned char)(a)) | (((unsigned short)((unsigned char)(b))) << 8)))
#endif

inline int WSAStartup(unsigned short wVersionRequested, LPWSADATA lpWSAData)
{
    if (lpWSAData == nullptr)
        return WSAEFAULT;
    std::memset(lpWSAData, 0, sizeof(*lpWSAData));
    lpWSAData->wVersion = wVersionRequested;
    lpWSAData->wHighVersion = MAKEWORD(2, 2);
    std::strcpy(lpWSAData->szDescription, "simple_mfc POSIX sockets");
    std::strcpy(lpWSAData->szSystemStatus, "Running");
    return 0;
}

inline int WSACleanup() { return 0; }

// ---------------------------------------------------------------------
// Asynchronous notification.
//
// WSAAsyncSelect asks Winsock to post a window message when a socket becomes
// readable/writable/etc. It has no POSIX counterpart -- the equivalent here
// is the select() reactor CAsyncSocket already runs -- so only the event bits
// and the message-packing macros are declared. The function itself is
// implemented by the socket layer, not here, because it needs the reactor.
// ---------------------------------------------------------------------
#define FD_READ_BIT       0
#define FD_WRITE_BIT      1
#define FD_OOB_BIT        2
#define FD_ACCEPT_BIT     3
#define FD_CONNECT_BIT    4
#define FD_CLOSE_BIT      5

#define FD_READ           (1 << FD_READ_BIT)
#define FD_WRITE          (1 << FD_WRITE_BIT)
#define FD_OOB            (1 << FD_OOB_BIT)
#define FD_ACCEPT         (1 << FD_ACCEPT_BIT)
#define FD_CONNECT        (1 << FD_CONNECT_BIT)
#define FD_CLOSE          (1 << FD_CLOSE_BIT)

// Win32 packs the event in the low word of lParam and the error in the high
// word; these are the accessors application code uses on the notification.
#ifndef WSAGETSELECTEVENT
#define WSAGETSELECTEVENT(lParam) ((int)((lParam) & 0xFFFF))
#endif
#ifndef WSAGETSELECTERROR
#define WSAGETSELECTERROR(lParam) ((int)(((lParam) >> 16) & 0xFFFF))
#endif

// ---------------------------------------------------------------------
// Socket options / level names Win32 spells differently or adds.
// ---------------------------------------------------------------------
#ifndef SD_RECEIVE
#define SD_RECEIVE  SHUT_RD
#define SD_SEND     SHUT_WR
#define SD_BOTH     SHUT_RDWR
#endif

#ifndef INADDR_NONE
#define INADDR_NONE ((unsigned long)0xFFFFFFFF)
#endif
