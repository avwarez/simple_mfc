// afxsock.h — NATIVE, cross-platform implementation (standard C++17 +
// the platform Berkeley-sockets API only).
//
// This header used to be a declaration-only stub: MFC networking was
// treated as "inherently Windows", so CAsyncSocket had signatures but no
// bodies. It does not need to be. A socket wrapper is exactly the kind of
// thing that maps cleanly onto a portable core (WinSock2 on Windows, the
// POSIX BSD-sockets API everywhere else), so CAsyncSocket is now a real,
// working class backed by src/afxsock.cpp.
//
// What "real" means here:
//   * the synchronous surface (Create/Bind/Connect/Listen/Accept/Send/
//     Receive/SendTo/ReceiveFrom/ShutDown/Close/IOCtl/SetSockOpt/
//     GetSockOpt/GetPeerName/GetSockName/GetLastError) genuinely opens,
//     binds and moves bytes over the network on both platforms;
//   * the asynchronous surface (AsyncSelect + the OnReceive/OnSend/
//     OnAccept/OnConnect/OnClose/OnOutOfBandData virtuals) is driven by a
//     portable select()-based reactor running on one background thread
//     (see afxsock.cpp). Real MFC drives these off a hidden window and
//     WSAAsyncSelect, which needs the message pump; the reactor reproduces
//     the same event semantics (edge-triggered FD_READ/FD_WRITE that
//     re-arm on WSAEWOULDBLOCK) without any GUI dependency, so eMule's UDP
//     sockets (CUDPSocket/CClientUDPSocket, which override OnReceive/
//     OnSend) get their callbacks on a plain console/portable build too.
//     The one deliberate deviation from MFC: callbacks fire on the reactor
//     thread, not the socket's owning thread (there is no per-thread
//     message pump to marshal onto) -- documented in afxsock.cpp.
#pragma once
#include "eafx.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h> // in6_addr and the IPv6 API (ws2ipdef.h) -- winsock2.h
                      // alone does not pull these in, but eMule's
                      // AsyncSocketEx.h uses `const in6_addr&` (C4430 without).
#include <iphlpapi.h> // GetIpErrorString and the IP helper API, which
                      // eMule's Pinger.cpp calls alongside <icmpapi.h>.
#include <ipexport.h> // IPAddr and the ICMP reply/status types: <icmpapi.h>,
                      // which eMule includes directly, uses them without
                      // declaring them.
#else
// POSIX: the real Berkeley-sockets surface. SOCKET/INVALID_SOCKET/
// SOCKET_ERROR and the FD_* AsyncSelect flags are WinSock spellings that do
// not exist here, so define them over the POSIX equivalents. SOCKADDR is
// WinSock's name for `struct sockaddr`.
#include <sys/socket.h>
#include <netinet/in.h>

using SOCKET = int;
using SOCKADDR = struct sockaddr;
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR (-1)
#endif

// WSAAsyncSelect event bits (winsock2.h values), used by AsyncSelect and
// the OnX callbacks. Identical numeric values to the real ones so code
// written against MFC keeps its meaning.
#ifndef FD_READ
#define FD_READ    0x01
#define FD_WRITE   0x02
#define FD_OOB     0x04
#define FD_ACCEPT  0x08
#define FD_CONNECT 0x10
#define FD_CLOSE   0x20
#endif
#endif // _WIN32

// Internal select()-based reactor, defined in afxsock.cpp. Named here only
// so CAsyncSocket can befriend it; consumers never touch it.
class ECAsyncSocketReactor;

// ---------------------------------------------------------------------
// CAsyncSocket (header afxsock.h, hierarchy CObject -> CAsyncSocket)
// ---------------------------------------------------------------------
class ECAsyncSocket : public ECObject
{
public:
    // ShutDown's nHow values (real MFC nests these as unnamed-enum members
    // of CAsyncSocket itself -- eMule/srchybrid references them qualified,
    // e.g. `CAsyncSocket::both`, `CAsyncSocket::receives`, not as free
    // WinSock SD_* constants).
    enum { receives = 0, sends = 1, both = 2 };

    // The underlying handle. Public in real MFC, and code (including
    // eMule's own socket layer) reads/casts it directly -- e.g.
    // `(SOCKET)this` style diagnostics.
    SOCKET m_hSocket;

    ECAsyncSocket() noexcept;
    virtual ~ECAsyncSocket();

    // --- lifecycle --------------------------------------------------------
    BOOL Create(UINT nSocketPort = 0, int nSocketType = 1 /*SOCK_STREAM*/,
                long lEvent = FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE,
                LPCTSTR lpszSocketAddress = nullptr);
    // Wrap an already-created handle; lEvent registers the same async
    // interests Create would. Detach relinquishes it without closing.
    BOOL Attach(SOCKET hSocket,
                long lEvent = FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);
    SOCKET Detach();
    virtual void Close();

    // --- addressing / connection -----------------------------------------
    BOOL Bind(UINT nSocketPort, LPCTSTR lpszSocketAddress = nullptr);
    BOOL Connect(LPCTSTR lpszHostAddress, UINT nHostPort);
    BOOL Connect(const SOCKADDR* lpSockAddr, int nSockAddrLen);
    BOOL Listen(int nConnectionBacklog = 5);
    virtual BOOL Accept(ECAsyncSocket& rConnectedSocket,
                        SOCKADDR* lpSockAddr = nullptr, int* lpSockAddrLen = nullptr);
    BOOL GetPeerName(ECString& rPeerAddress, UINT& rPeerPort);
    BOOL GetSockName(ECString& rSocketAddress, UINT& rSocketPort);

    // --- data transfer ----------------------------------------------------
    virtual int Send(const void* lpBuf, int nBufLen, int nFlags = 0);
    virtual int Receive(void* lpBuf, int nBufLen, int nFlags = 0);
    int SendTo(const void* lpBuf, int nBufLen, UINT nHostPort,
               LPCTSTR lpszHostAddress = nullptr, int nFlags = 0);
    // eMule's UDP sockets take the raw-sockaddr form so they can read the
    // peer address without going through a CString round-trip.
    virtual int ReceiveFrom(void* lpBuf, int nBufLen, SOCKADDR* lpSockAddr,
                            int* lpSockAddrLen, int nFlags = 0);
    virtual int ReceiveFrom(void* lpBuf, int nBufLen, ECString& rSocketAddress,
                            UINT& rSocketPort, int nFlags = 0);

    // --- options / control ------------------------------------------------
    BOOL ShutDown(int nHow = sends);
    BOOL IOCtl(long lCommand, DWORD* lpArgument);
    BOOL SetSockOpt(int nOptionName, const void* lpOptionValue, int nOptionLen,
                    int nLevel = 0xFFFF /*SOL_SOCKET*/);
    BOOL GetSockOpt(int nOptionName, void* lpOptionValue, int* lpOptionLength,
                    int nLevel = 0xFFFF /*SOL_SOCKET*/);
    BOOL AsyncSelect(long lEvent = FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);

    // Static: "the error code for the last Windows Sockets API routine
    // performed by this thread" -- not per-instance in real MFC either. On
    // POSIX it returns errno from the last failing call.
    static int GetLastError();
    // Look up the CAsyncSocket currently wrapping a handle (real MFC's
    // handle->object map). Returns nullptr if the handle is not attached.
    static ECAsyncSocket* FromHandle(SOCKET hSocket);

    // --- async notification callbacks (override in a derived class) -------
    // Real MFC declares these virtual on CAsyncSocket; the reactor calls
    // them exactly as WSAAsyncSelect + the hidden window would.
    virtual void OnReceive(int nErrorCode);
    virtual void OnSend(int nErrorCode);
    virtual void OnOutOfBandData(int nErrorCode);
    virtual void OnAccept(int nErrorCode);
    virtual void OnConnect(int nErrorCode);
    virtual void OnClose(int nErrorCode);

private:
    friend class ECAsyncSocketReactor;

    long m_lEvent;      // AsyncSelect mask (0 = synchronous / blocking)
    bool m_bConnecting; // a non-blocking connect() is in flight
    bool m_bListening;  // Listen() called -> readable means "incoming"
    bool m_bStream;     // SOCK_STREAM (vs datagram) -- gates FD_CLOSE detection
    bool m_bReadArmed;  // FD_READ edge state (re-armed on EWOULDBLOCK)
    bool m_bWriteArmed; // FD_WRITE edge state (re-armed on EWOULDBLOCK)
};

// ---------------------------------------------------------------------
// Windows Sockets startup / teardown. On Windows these wrap WSAStartup/
// WSACleanup (real MFC's AfxSocketInit does exactly that); on POSIX there
// is no per-process sockets init, so they are successful no-ops.
// ---------------------------------------------------------------------
BOOL EAfxSocketInit(void* lpwsaData = nullptr);
// The matching teardown. eMule takes its address to install it as the
// module's socket-termination hook (see sockimpl.h).
void EAFX_CDECL EAfxSocketTerm();
