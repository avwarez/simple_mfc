#pragma once
#include "eafx.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <ipexport.h>
#else
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

#ifndef FD_READ
#define FD_READ    0x01
#define FD_WRITE   0x02
#define FD_OOB     0x04
#define FD_ACCEPT  0x08
#define FD_CONNECT 0x10
#define FD_CLOSE   0x20
#endif
#endif

class ECAsyncSocketReactor;

class ECAsyncSocket : public ECObject
{
public:
    enum { receives = 0, sends = 1, both = 2 };

    SOCKET m_hSocket;

    ECAsyncSocket() noexcept;
    virtual ~ECAsyncSocket();

    BOOL Create(UINT nSocketPort = 0, int nSocketType = 1  ,
                long lEvent = FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE,
                LPCTSTR lpszSocketAddress = nullptr);
    BOOL Attach(SOCKET hSocket,
                long lEvent = FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);
    SOCKET Detach();
    virtual void Close();

    BOOL Bind(UINT nSocketPort, LPCTSTR lpszSocketAddress = nullptr);
    BOOL Connect(LPCTSTR lpszHostAddress, UINT nHostPort);
    BOOL Connect(const SOCKADDR* lpSockAddr, int nSockAddrLen);
    BOOL Listen(int nConnectionBacklog = 5);
    virtual BOOL Accept(ECAsyncSocket& rConnectedSocket,
                        SOCKADDR* lpSockAddr = nullptr, int* lpSockAddrLen = nullptr);
    BOOL GetPeerName(ECString& rPeerAddress, UINT& rPeerPort);
    BOOL GetSockName(ECString& rSocketAddress, UINT& rSocketPort);

    virtual int Send(const void* lpBuf, int nBufLen, int nFlags = 0);
    virtual int Receive(void* lpBuf, int nBufLen, int nFlags = 0);
    int SendTo(const void* lpBuf, int nBufLen, UINT nHostPort,
               LPCTSTR lpszHostAddress = nullptr, int nFlags = 0);
    virtual int ReceiveFrom(void* lpBuf, int nBufLen, SOCKADDR* lpSockAddr,
                            int* lpSockAddrLen, int nFlags = 0);
    virtual int ReceiveFrom(void* lpBuf, int nBufLen, ECString& rSocketAddress,
                            UINT& rSocketPort, int nFlags = 0);

    BOOL ShutDown(int nHow = sends);
    BOOL IOCtl(long lCommand, DWORD* lpArgument);
    BOOL SetSockOpt(int nOptionName, const void* lpOptionValue, int nOptionLen,
                    int nLevel = 0xFFFF  );
    BOOL GetSockOpt(int nOptionName, void* lpOptionValue, int* lpOptionLength,
                    int nLevel = 0xFFFF  );
    BOOL AsyncSelect(long lEvent = FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);

    static int GetLastError();
    static ECAsyncSocket* FromHandle(SOCKET hSocket);

protected:
    virtual void OnReceive(int nErrorCode);
    virtual void OnSend(int nErrorCode);
    virtual void OnOutOfBandData(int nErrorCode);
    virtual void OnAccept(int nErrorCode);
    virtual void OnConnect(int nErrorCode);
    virtual void OnClose(int nErrorCode);

private:
    friend class ECAsyncSocketReactor;

    long m_lEvent;
    bool m_bConnecting;
    bool m_bListening;
    bool m_bStream;
    bool m_bReadArmed;
    bool m_bWriteArmed;
};

BOOL EAfxSocketInit(void* lpwsaData = nullptr);
void EAFX_CDECL EAfxSocketTerm();
