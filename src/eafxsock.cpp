#include "eafxsock.h"

#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <set>
#include <string>
#include <thread>

#ifdef _WIN32
#include <ws2tcpip.h>
using smfc_socklen_t = int;
static int  smfc_closesock(SOCKET s)         { return ::closesocket(s); }
static int  smfc_lasterr()                   { return ::WSAGetLastError(); }
static bool smfc_wouldblock(int e)           { return e == WSAEWOULDBLOCK; }
static bool smfc_inprogress(int e)           { return e == WSAEWOULDBLOCK; }
static void smfc_nonblock(SOCKET s, bool nb) { u_long m = nb ? 1 : 0; ::ioctlsocket(s, FIONBIO, &m); }
#else
#include <arpa/inet.h>
#include <cerrno>
#include <cwchar>
#include <fcntl.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <unistd.h>
using smfc_socklen_t = socklen_t;
static int  smfc_closesock(SOCKET s)         { return ::close(s); }
static int  smfc_lasterr()                   { return errno; }
static bool smfc_wouldblock(int e)           { return e == EWOULDBLOCK || e == EAGAIN; }
static bool smfc_inprogress(int e)           { return e == EINPROGRESS; }
static void smfc_nonblock(SOCKET s, bool nb)
{
    int f = ::fcntl(s, F_GETFL, 0);
    if (f >= 0)
        ::fcntl(s, F_SETFL, nb ? (f | O_NONBLOCK) : (f & ~O_NONBLOCK));
}
#endif

class ECAsyncSocketReactor
{
public:
    static ECAsyncSocketReactor& Instance()
    {
        static ECAsyncSocketReactor inst;
        return inst;
    }

    void Add(ECAsyncSocket* s)
    {
        std::lock_guard<std::recursive_mutex> lk(m_mtx);
        m_sockets.insert(s);
        if (!m_started)
        {
            m_started = true;
            m_running = true;
            m_thread = std::thread([this] { Run(); });
        }
    }

    void Remove(ECAsyncSocket* s)
    {
        std::lock_guard<std::recursive_mutex> lk(m_mtx);
        m_sockets.erase(s);
    }

    ECAsyncSocket* Find(SOCKET h)
    {
        std::lock_guard<std::recursive_mutex> lk(m_mtx);
        for (ECAsyncSocket* s : m_sockets)
            if (s->m_hSocket == h)
                return s;
        return nullptr;
    }

    ~ECAsyncSocketReactor()
    {
        m_running = false;
        if (m_thread.joinable())
            m_thread.join();
    }

private:
    ECAsyncSocketReactor() = default;

    bool Alive(ECAsyncSocket* s) { return m_sockets.count(s) != 0; }

    void Run()
    {
        while (m_running)
        {
            fd_set rd, wr, ex;
            FD_ZERO(&rd);
            FD_ZERO(&wr);
            FD_ZERO(&ex);
            int maxfd = -1;
            {
                std::lock_guard<std::recursive_mutex> lk(m_mtx);
                for (ECAsyncSocket* s : m_sockets)
                {
                    if (s->m_hSocket == INVALID_SOCKET || s->m_lEvent == 0)
                        continue;
                    const long ev = s->m_lEvent;
                    const SOCKET fd = s->m_hSocket;
                    if (ev & (FD_READ | FD_ACCEPT | FD_CLOSE | FD_OOB))
                        FD_SET(fd, &rd);
                    if (s->m_bConnecting || ((ev & FD_WRITE) && s->m_bWriteArmed))
                        FD_SET(fd, &wr);
                    if (ev & FD_OOB)
                        FD_SET(fd, &ex);
                    if (static_cast<int>(fd) > maxfd)
                        maxfd = static_cast<int>(fd);
                }
            }

            timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 100 * 1000;
            const int n = ::select(maxfd + 1, &rd, &wr, &ex, &tv);
            if (n <= 0)
                continue;

            std::lock_guard<std::recursive_mutex> lk(m_mtx);
            std::set<ECAsyncSocket*> snapshot = m_sockets;
            for (ECAsyncSocket* s : snapshot)
            {
                if (!Alive(s) || s->m_hSocket == INVALID_SOCKET)
                    continue;
                const SOCKET fd = s->m_hSocket;
                const bool r = FD_ISSET(fd, &rd) != 0;
                const bool w = FD_ISSET(fd, &wr) != 0;
                const bool e = FD_ISSET(fd, &ex) != 0;
                if (r || w || e)
                    Dispatch(s, r, w, e);
            }
        }
    }

    void Dispatch(ECAsyncSocket* s, bool r, bool w, bool e)
    {
        const long ev = s->m_lEvent;

        if (s->m_bConnecting && (w || e))
        {
            s->m_bConnecting = false;
            int err = 0;
            smfc_socklen_t len = sizeof(err);
            ::getsockopt(s->m_hSocket, SOL_SOCKET, SO_ERROR,
                         reinterpret_cast<char*>(&err), &len);
            s->m_bWriteArmed = true;
            s->OnConnect(err);
            if (!Alive(s))
                return;
        }

        bool closed = false;
        if (r && s->m_bStream && !s->m_bListening)
        {
            char c;
            const int pk = static_cast<int>(::recv(s->m_hSocket, &c, 1, MSG_PEEK));
            if (pk == 0)
                closed = true;
        }
        if (closed && (ev & FD_CLOSE))
        {
            s->OnClose(0);
            return;
        }

        if (r && (ev & FD_ACCEPT) && s->m_bListening)
        {
            s->OnAccept(0);
            if (!Alive(s))
                return;
        }
        else if (r && (ev & FD_READ) && s->m_bReadArmed)
        {
            s->m_bReadArmed = false;
            s->OnReceive(0);
            if (!Alive(s))
                return;
        }

        if (w && (ev & FD_WRITE) && s->m_bWriteArmed && !s->m_bConnecting)
        {
            s->m_bWriteArmed = false;
            s->OnSend(0);
            if (!Alive(s))
                return;
        }

        if (e && (ev & FD_OOB))
        {
            s->OnOutOfBandData(0);
            if (!Alive(s))
                return;
        }
    }

    std::recursive_mutex   m_mtx;
    std::set<ECAsyncSocket*> m_sockets;
    std::thread            m_thread;
    std::atomic<bool>      m_running{false};
    bool                   m_started = false;
};

namespace {

std::string ToNarrow(LPCTSTR s)
{
    if (s == nullptr)
        return std::string();
    return mfc_detail::Narrow(s, std::char_traits<wchar_t>::length(s));
}

bool ResolveV4(const std::string& host, unsigned short port, int socktype,
               sockaddr_in& out)
{
    std::memset(&out, 0, sizeof(out));
    out.sin_family = AF_INET;
    out.sin_port = htons(port);
    if (::inet_pton(AF_INET, host.c_str(), &out.sin_addr) == 1)
        return true;

    addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = socktype;
    addrinfo* res = nullptr;
    if (::getaddrinfo(host.c_str(), nullptr, &hints, &res) != 0 || res == nullptr)
        return false;
    out.sin_addr = reinterpret_cast<sockaddr_in*>(res->ai_addr)->sin_addr;
    ::freeaddrinfo(res);
    return true;
}

void FormatV4(const sockaddr_in& sa, ECString& addr, UINT& port)
{
    char ip[INET_ADDRSTRLEN] = {0};
    ::inet_ntop(AF_INET, &sa.sin_addr, ip, sizeof(ip));
    addr = ECString(mfc_detail::Widen(ip, std::strlen(ip)).c_str());
    port = ntohs(sa.sin_port);
}

}

ECAsyncSocket::ECAsyncSocket() noexcept
    : m_hSocket(INVALID_SOCKET), m_lEvent(0), m_bConnecting(false),
      m_bListening(false), m_bStream(false), m_bReadArmed(true),
      m_bWriteArmed(true)
{
}

ECAsyncSocket::~ECAsyncSocket()
{
    Close();
}

BOOL ECAsyncSocket::Create(UINT nSocketPort, int nSocketType, long lEvent,
                          LPCTSTR lpszSocketAddress)
{
    if (m_hSocket != INVALID_SOCKET)
        return FALSE;
    m_bStream = (nSocketType == SOCK_STREAM);
    const SOCKET h = ::socket(AF_INET, nSocketType, 0);
    if (h == INVALID_SOCKET)
        return FALSE;
    m_hSocket = h;
    if (!Bind(nSocketPort, lpszSocketAddress))
    {
        Close();
        return FALSE;
    }
    if (lEvent != 0 && !AsyncSelect(lEvent))
    {
        Close();
        return FALSE;
    }
    return TRUE;
}

BOOL ECAsyncSocket::Attach(SOCKET hSocket, long lEvent)
{
    if (hSocket == INVALID_SOCKET)
        return FALSE;
    m_hSocket = hSocket;
    int type = 0;
    smfc_socklen_t len = sizeof(type);
    if (::getsockopt(hSocket, SOL_SOCKET, SO_TYPE,
                     reinterpret_cast<char*>(&type), &len) == 0)
        m_bStream = (type == SOCK_STREAM);
    if (lEvent != 0)
        return AsyncSelect(lEvent);
    return TRUE;
}

SOCKET ECAsyncSocket::Detach()
{
    const SOCKET h = m_hSocket;
    if (m_hSocket != INVALID_SOCKET)
        ECAsyncSocketReactor::Instance().Remove(this);
    m_hSocket = INVALID_SOCKET;
    m_lEvent = 0;
    m_bConnecting = m_bListening = false;
    return h;
}

void ECAsyncSocket::Close()
{
    if (m_hSocket != INVALID_SOCKET)
    {
        ECAsyncSocketReactor::Instance().Remove(this);
        smfc_closesock(m_hSocket);
        m_hSocket = INVALID_SOCKET;
    }
    m_lEvent = 0;
    m_bConnecting = m_bListening = false;
}

BOOL ECAsyncSocket::Bind(UINT nSocketPort, LPCTSTR lpszSocketAddress)
{
    if (m_hSocket == INVALID_SOCKET)
        return FALSE;
    sockaddr_in addr;
    if (lpszSocketAddress == nullptr)
    {
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<unsigned short>(nSocketPort));
        addr.sin_addr.s_addr = INADDR_ANY;
    }
    else if (!ResolveV4(ToNarrow(lpszSocketAddress),
                        static_cast<unsigned short>(nSocketPort),
                        m_bStream ? SOCK_STREAM : SOCK_DGRAM, addr))
    {
        return FALSE;
    }
    return ::bind(m_hSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0
               ? TRUE : FALSE;
}

BOOL ECAsyncSocket::Connect(LPCTSTR lpszHostAddress, UINT nHostPort)
{
    if (m_hSocket == INVALID_SOCKET || lpszHostAddress == nullptr)
        return FALSE;
    sockaddr_in addr;
    if (!ResolveV4(ToNarrow(lpszHostAddress),
                   static_cast<unsigned short>(nHostPort),
                   m_bStream ? SOCK_STREAM : SOCK_DGRAM, addr))
        return FALSE;
    return Connect(reinterpret_cast<const SOCKADDR*>(&addr), sizeof(addr));
}

BOOL ECAsyncSocket::Connect(const SOCKADDR* lpSockAddr, int nSockAddrLen)
{
    if (m_hSocket == INVALID_SOCKET)
        return FALSE;
    if (::connect(m_hSocket, lpSockAddr,
                  static_cast<smfc_socklen_t>(nSockAddrLen)) == 0)
        return TRUE;
    if (smfc_inprogress(smfc_lasterr()))
        m_bConnecting = true;
    return FALSE;
}

BOOL ECAsyncSocket::Listen(int nConnectionBacklog)
{
    if (m_hSocket == INVALID_SOCKET)
        return FALSE;
    m_bListening = true;
    return ::listen(m_hSocket, nConnectionBacklog) == 0 ? TRUE : FALSE;
}

BOOL ECAsyncSocket::Accept(ECAsyncSocket& rConnectedSocket, SOCKADDR* lpSockAddr,
                          int* lpSockAddrLen)
{
    if (m_hSocket == INVALID_SOCKET || rConnectedSocket.m_hSocket != INVALID_SOCKET)
        return FALSE;
    smfc_socklen_t len = lpSockAddrLen ? static_cast<smfc_socklen_t>(*lpSockAddrLen) : 0;
    const SOCKET h = ::accept(m_hSocket, lpSockAddr, lpSockAddrLen ? &len : nullptr);
    if (h == INVALID_SOCKET)
        return FALSE;
    if (lpSockAddrLen)
        *lpSockAddrLen = static_cast<int>(len);
    rConnectedSocket.m_hSocket = h;
    rConnectedSocket.m_bStream = true;
    return TRUE;
}

BOOL ECAsyncSocket::GetPeerName(ECString& rPeerAddress, UINT& rPeerPort)
{
    if (m_hSocket == INVALID_SOCKET)
        return FALSE;
    sockaddr_in sa;
    smfc_socklen_t len = sizeof(sa);
    if (::getpeername(m_hSocket, reinterpret_cast<sockaddr*>(&sa), &len) != 0)
        return FALSE;
    FormatV4(sa, rPeerAddress, rPeerPort);
    return TRUE;
}

BOOL ECAsyncSocket::GetSockName(ECString& rSocketAddress, UINT& rSocketPort)
{
    if (m_hSocket == INVALID_SOCKET)
        return FALSE;
    sockaddr_in sa;
    smfc_socklen_t len = sizeof(sa);
    if (::getsockname(m_hSocket, reinterpret_cast<sockaddr*>(&sa), &len) != 0)
        return FALSE;
    FormatV4(sa, rSocketAddress, rSocketPort);
    return TRUE;
}

int ECAsyncSocket::Send(const void* lpBuf, int nBufLen, int nFlags)
{
    if (m_hSocket == INVALID_SOCKET)
        return SOCKET_ERROR;
    int f = nFlags;
#ifdef MSG_NOSIGNAL
    f |= MSG_NOSIGNAL;
#endif
    const int n = static_cast<int>(
        ::send(m_hSocket, static_cast<const char*>(lpBuf),
               static_cast<size_t>(nBufLen), f));
    if (n == SOCKET_ERROR && smfc_wouldblock(smfc_lasterr()))
        m_bWriteArmed = true;
    return n;
}

int ECAsyncSocket::Receive(void* lpBuf, int nBufLen, int nFlags)
{
    if (m_hSocket == INVALID_SOCKET)
        return SOCKET_ERROR;
    const int n = static_cast<int>(
        ::recv(m_hSocket, static_cast<char*>(lpBuf),
               static_cast<size_t>(nBufLen), nFlags));
    if (n == SOCKET_ERROR && smfc_wouldblock(smfc_lasterr()))
        m_bReadArmed = true;
    return n;
}

int ECAsyncSocket::SendTo(const void* lpBuf, int nBufLen, UINT nHostPort,
                         LPCTSTR lpszHostAddress, int nFlags)
{
    if (m_hSocket == INVALID_SOCKET)
        return SOCKET_ERROR;
    sockaddr_in addr;
    if (lpszHostAddress == nullptr)
    {
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<unsigned short>(nHostPort));
        addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    }
    else if (!ResolveV4(ToNarrow(lpszHostAddress),
                        static_cast<unsigned short>(nHostPort), SOCK_DGRAM, addr))
    {
        return SOCKET_ERROR;
    }
    int f = nFlags;
#ifdef MSG_NOSIGNAL
    f |= MSG_NOSIGNAL;
#endif
    const int n = static_cast<int>(
        ::sendto(m_hSocket, static_cast<const char*>(lpBuf),
                 static_cast<size_t>(nBufLen), f,
                 reinterpret_cast<sockaddr*>(&addr), sizeof(addr)));
    if (n == SOCKET_ERROR && smfc_wouldblock(smfc_lasterr()))
        m_bWriteArmed = true;
    return n;
}

int ECAsyncSocket::ReceiveFrom(void* lpBuf, int nBufLen, SOCKADDR* lpSockAddr,
                              int* lpSockAddrLen, int nFlags)
{
    if (m_hSocket == INVALID_SOCKET)
        return SOCKET_ERROR;
    smfc_socklen_t sl = lpSockAddrLen ? static_cast<smfc_socklen_t>(*lpSockAddrLen) : 0;
    const int n = static_cast<int>(
        ::recvfrom(m_hSocket, static_cast<char*>(lpBuf),
                   static_cast<size_t>(nBufLen), nFlags, lpSockAddr,
                   lpSockAddrLen ? &sl : nullptr));
    if (lpSockAddrLen)
        *lpSockAddrLen = static_cast<int>(sl);
    if (n == SOCKET_ERROR && smfc_wouldblock(smfc_lasterr()))
        m_bReadArmed = true;
    return n;
}

int ECAsyncSocket::ReceiveFrom(void* lpBuf, int nBufLen, ECString& rSocketAddress,
                              UINT& rSocketPort, int nFlags)
{
    sockaddr_in from;
    std::memset(&from, 0, sizeof(from));
    int fl = sizeof(from);
    const int n = ReceiveFrom(lpBuf, nBufLen, reinterpret_cast<SOCKADDR*>(&from),
                              &fl, nFlags);
    if (n != SOCKET_ERROR)
        FormatV4(from, rSocketAddress, rSocketPort);
    return n;
}

BOOL ECAsyncSocket::ShutDown(int nHow)
{
    if (m_hSocket == INVALID_SOCKET)
        return FALSE;
#ifdef _WIN32
    const int how = nHow;
#else
    const int how = (nHow == receives) ? SHUT_RD
                    : (nHow == sends)   ? SHUT_WR
                                        : SHUT_RDWR;
#endif
    return ::shutdown(m_hSocket, how) == 0 ? TRUE : FALSE;
}

BOOL ECAsyncSocket::IOCtl(long lCommand, DWORD* lpArgument)
{
    if (m_hSocket == INVALID_SOCKET)
        return FALSE;
#ifdef _WIN32
    return ::ioctlsocket(m_hSocket, lCommand,
                         reinterpret_cast<u_long*>(lpArgument)) == 0 ? TRUE : FALSE;
#else
    return ::ioctl(m_hSocket, static_cast<unsigned long>(lCommand), lpArgument) == 0
               ? TRUE : FALSE;
#endif
}

BOOL ECAsyncSocket::SetSockOpt(int nOptionName, const void* lpOptionValue,
                              int nOptionLen, int nLevel)
{
    if (m_hSocket == INVALID_SOCKET)
        return FALSE;
    const int level = (nLevel == 0xFFFF) ? SOL_SOCKET : nLevel;
    return ::setsockopt(m_hSocket, level, nOptionName,
                        static_cast<const char*>(lpOptionValue),
                        static_cast<smfc_socklen_t>(nOptionLen)) == 0 ? TRUE : FALSE;
}

BOOL ECAsyncSocket::GetSockOpt(int nOptionName, void* lpOptionValue,
                              int* lpOptionLength, int nLevel)
{
    if (m_hSocket == INVALID_SOCKET)
        return FALSE;
    const int level = (nLevel == 0xFFFF) ? SOL_SOCKET : nLevel;
    smfc_socklen_t sl = lpOptionLength ? static_cast<smfc_socklen_t>(*lpOptionLength) : 0;
    const int rc = ::getsockopt(m_hSocket, level, nOptionName,
                                static_cast<char*>(lpOptionValue),
                                lpOptionLength ? &sl : nullptr);
    if (lpOptionLength)
        *lpOptionLength = static_cast<int>(sl);
    return rc == 0 ? TRUE : FALSE;
}

BOOL ECAsyncSocket::AsyncSelect(long lEvent)
{
    if (m_hSocket == INVALID_SOCKET)
        return FALSE;
    m_lEvent = lEvent;
    m_bReadArmed = true;
    m_bWriteArmed = true;
    smfc_nonblock(m_hSocket, lEvent != 0);
    if (lEvent != 0)
        ECAsyncSocketReactor::Instance().Add(this);
    else
        ECAsyncSocketReactor::Instance().Remove(this);
    return TRUE;
}

int ECAsyncSocket::GetLastError()
{
    return smfc_lasterr();
}

ECAsyncSocket* ECAsyncSocket::FromHandle(SOCKET hSocket)
{
    if (hSocket == INVALID_SOCKET)
        return nullptr;
    return ECAsyncSocketReactor::Instance().Find(hSocket);
}

void ECAsyncSocket::OnReceive(int  ) {}
void ECAsyncSocket::OnSend(int  ) {}
void ECAsyncSocket::OnOutOfBandData(int  ) {}
void ECAsyncSocket::OnAccept(int  ) {}
void ECAsyncSocket::OnConnect(int  ) {}
void ECAsyncSocket::OnClose(int  ) {}

BOOL EAfxSocketInit(void* lpwsaData)
{
#ifdef _WIN32
    WSADATA local;
    WSADATA* p = lpwsaData ? static_cast<WSADATA*>(lpwsaData) : &local;
    return ::WSAStartup(MAKEWORD(2, 2), p) == 0 ? TRUE : FALSE;
#else
    (void)lpwsaData;
    return TRUE;
#endif
}

void EAFX_CDECL EAfxSocketTerm()
{
#ifdef _WIN32
    ::WSACleanup();
#endif
}
