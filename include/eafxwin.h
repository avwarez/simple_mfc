#pragma once
#include "eafx.h"

#ifndef _WIN32
using LPVOID = void*;
#endif

#ifndef _WIN32
struct SECURITY_ATTRIBUTES;
#endif

typedef UINT(EAFX_CDECL* EAFX_THREADPROC)(void*);

#ifndef _WIN32
#define CREATE_SUSPENDED        0x00000004
#define INFINITE                0xFFFFFFFF
#define THREAD_PRIORITY_IDLE           (-15)
#define THREAD_PRIORITY_LOWEST         (-2)
#define THREAD_PRIORITY_BELOW_NORMAL   (-1)
#define THREAD_PRIORITY_NORMAL         0
#define THREAD_PRIORITY_ABOVE_NORMAL   1
#define THREAD_PRIORITY_HIGHEST        2
#define THREAD_PRIORITY_TIME_CRITICAL  15
#endif

class ECWinThread : public ECObject
{
public:
    LPVOID m_pThreadParams = nullptr;

public:
    BOOL m_bAutoDelete;
    void* m_hThread;
    DWORD m_nThreadID;

    ECWinThread();
    ECWinThread(EAFX_THREADPROC pfnThreadProc, LPVOID pParam);
    BOOL CreateThread(DWORD dwCreateFlags = 0, UINT nStackSize = 0,
                      SECURITY_ATTRIBUTES* lpSecurityAttrs = nullptr);
    DWORD ResumeThread();
    DWORD SuspendThread();
    BOOL SetThreadPriority(int nPriority);
    int GetThreadPriority();
    virtual BOOL InitInstance();
    virtual int ExitInstance();
    virtual int Run();
    virtual void Delete();
    virtual ~ECWinThread();

private:
    struct Impl;
    Impl* m_pImpl = nullptr;
};

ECWinThread* EAfxBeginThread(EAFX_THREADPROC pfnThreadProc, void* pParam,
                           int nPriority = 0  ,
                           UINT nStackSize = 0, DWORD dwCreateFlags = 0,
                           SECURITY_ATTRIBUTES* lpSecurityAttrs = nullptr);
ECWinThread* EAfxBeginThread(ECRuntimeClass* pThreadClass,
                           int nPriority = 0, UINT nStackSize = 0,
                           DWORD dwCreateFlags = 0,
                           SECURITY_ATTRIBUTES* lpSecurityAttrs = nullptr);
