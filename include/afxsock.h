// afxsock.h — reference STUB (declarations only, no implementation).
// MFC networking.
#pragma once
#include "afx.h"

// Real SOCKADDR is a typedef-name for `struct sockaddr` (winsock2.h),
// not a struct literally tagged SOCKADDR -- guarded like afxwin.h's
// SECURITY_ATTRIBUTES/CREATESTRUCT (see there): on a real Windows/MSVC
// target this collides with our own bare forward-declaration once
// eMule/srchybrid's own direct <winsock2.h> include is processed.
#ifdef _WIN32
#include <winsock2.h>
#include <iphlpapi.h> // GetIpErrorString and the IP helper API, which
                      // eMule's Pinger.cpp calls alongside <icmpapi.h>.
#include <ipexport.h> // IPAddr and the ICMP reply/status types: <icmpapi.h>,
                      // which eMule includes directly, uses them without
                      // declaring them.
#include <ws2tcpip.h> // in6_addr and the IPv6 API (ws2ipdef.h) -- winsock2.h
                      // alone does not pull these in, but eMule's
                      // AsyncSocketEx.h uses `const in6_addr&` (C4430 without).
#else
struct SOCKADDR;
#endif

// ---------------------------------------------------------------------
// CAsyncSocket (header afxsock.h, hierarchy CObject -> CAsyncSocket)
// ---------------------------------------------------------------------
class CAsyncSocket : public CObject
{
public:
    // ShutDown's nHow values (real MFC nests these as unnamed-enum members
    // of CAsyncSocket itself — eMule/srchybrid references them qualified,
    // e.g. `CAsyncSocket::both`, `CAsyncSocket::receives`, not as free
    // WinSock SD_* constants).
    enum { receives = 0, sends = 1, both = 2 };

    virtual void Close();
    BOOL Create(UINT nSocketPort = 0, int nSocketType = 1 /*SOCK_STREAM*/,
                long lEvent = 0x3F /*FD_READ|FD_WRITE|FD_OOB|FD_ACCEPT|FD_CONNECT|FD_CLOSE*/,
                LPCTSTR lpszSocketAddress = nullptr);
    BOOL Connect(LPCTSTR lpszHostAddress, UINT nHostPort);
    BOOL GetPeerName(CString& rPeerAddress, UINT& rPeerPort);
    BOOL GetSockName(CString& rSocketAddress, UINT& rSocketPort);
    BOOL AsyncSelect(long lEvent = 0x3F);
    BOOL ShutDown(int nHow = sends);
    BOOL IOCtl(long lCommand, DWORD* lpArgument);
    virtual int Send(const void* lpBuf, int nBufLen, int nFlags = 0);
    virtual int Receive(void* lpBuf, int nBufLen, int nFlags = 0);
    int SendTo(const void* lpBuf, int nBufLen, UINT nHostPort,
               LPCTSTR lpszHostAddress = nullptr, int nFlags = 0);
    // eMule's UDP sockets take the raw-sockaddr form so they can read the
    // peer address without going through a CString round-trip.
    virtual int ReceiveFrom(void* lpBuf, int nBufLen, SOCKADDR* lpSockAddr,
                             int* lpSockAddrLen, int nFlags = 0);
    virtual int ReceiveFrom(void* lpBuf, int nBufLen, CString& rSocketAddress,
                             UINT& rSocketPort, int nFlags = 0);
    BOOL Listen(int nConnectionBacklog = 5);
    virtual BOOL Accept(CAsyncSocket& rConnectedSocket,
                         SOCKADDR* lpSockAddr = nullptr, int* lpSockAddrLen = nullptr);
    BOOL SetSockOpt(int nOptionName, const void* lpOptionValue, int nOptionLen,
                     int nLevel = 0xFFFF /*SOL_SOCKET*/);
    // Static: "the error code for the last Windows Sockets API routine
    // performed by this thread" — not per-instance in real MFC either.
    static int GetLastError();
};

// NOTE: CAsyncSocketEx is deliberately NOT declared here. Despite the name it
// is NOT a real-MFC class -- it is eMule/srchybrid's OWN custom class (defined
// in its AsyncSocketEx.h as `class CAsyncSocketEx : public CObject`). Declaring
// an empty one here collided with eMule's real definition in the same TU
// (C2011 "type redefinition"), so real MFC (which has no such class) is matched
// by simply not providing it.

// ---------------------------------------------------------------------
// Global function to initialize Windows Sockets in an MFC thread
// ---------------------------------------------------------------------
BOOL AfxSocketInit(void* lpwsaData = nullptr);
