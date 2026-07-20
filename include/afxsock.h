// afxsock.h — reference STUB (declarations only, no implementation).
// MFC networking.
#pragma once
#include "afx.h"

struct SOCKADDR;

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
    BOOL Listen(int nConnectionBacklog = 5);
    virtual BOOL Accept(CAsyncSocket& rConnectedSocket,
                         SOCKADDR* lpSockAddr = nullptr, int* lpSockAddrLen = nullptr);
    BOOL SetSockOpt(int nOptionName, const void* lpOptionValue, int nOptionLen,
                     int nLevel = 0xFFFF /*SOL_SOCKET*/);
    // Static: "the error code for the last Windows Sockets API routine
    // performed by this thread" — not per-instance in real MFC either.
    static int GetLastError();
};

// CAsyncSocketEx — extended variant used by some application-level socket
// classes as a custom `: public CObject` type (not part of native MFC).
class CAsyncSocketEx : public CObject {};

// ---------------------------------------------------------------------
// Global function to initialize Windows Sockets in an MFC thread
// ---------------------------------------------------------------------
BOOL AfxSocketInit(void* lpwsaData = nullptr);
