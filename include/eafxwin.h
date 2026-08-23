// afxwin.h — the THREADING part only, on the `backend` branch.
//
// In real MFC (and on `main`) this header is the GUI umbrella: CWnd and its
// subclasses, the controls, the GDI objects, the message-map macros, CWinApp.
// None of that is here — it is frontend, and this branch does not carry it.
//
// What IS here is the one part of afxwin.h that the census classifies as
// background: CWinThread and AfxBeginThread, the worker-thread machinery
// (34 occurrences in eMule, in the thread/sync block alongside afxmt.h's
// CCriticalSection/CEvent/CMutex). The file keeps its real-MFC name so that
// code reaching CWinThread by its documented header still compiles.
//
// TWO DELIBERATE DIVERGENCES from real MFC, both forced by the perimeter:
//
//   * CWinThread derives from CObject, not from CCmdTarget. Real MFC's
//     hierarchy is CObject -> CCmdTarget -> CWinThread, but CCmdTarget is
//     command routing: message maps, OnCmdMsg, the wait cursor, the COM
//     nested-interface plumbing. All frontend. A thread does not need any of
//     it, so the intermediate level is dropped rather than dragged in.
//   * m_pMainWnd and m_pActiveWnd are absent. They are CWnd*, and there is
//     no CWnd here. Nothing in a worker thread reads them.
//
// Both are visible at compile time (a missing base, a missing member), never
// silently at run time.
#pragma once
#include "eafx.h" // CObject, CRuntimeClass, BOOL/DWORD/UINT, AFX_CDECL

// LPVOID's owner on this branch. On `main` afxwin.h owns it too, in its
// Win32 stand-in block; that block is gone, so the declaration comes along
// with the one type that still needs it.
#ifndef _WIN32
using LPVOID = void*;
#endif

// Forward-declared, never defined: CreateThread takes a pointer to one and
// the portable implementation ignores it. Real MFC's is a kernel security
// descriptor, which has no POSIX counterpart.
//
// POSIX only. The Windows SDK spells it `typedef struct _SECURITY_ATTRIBUTES
// {...} SECURITY_ATTRIBUTES`, i.e. the name is a TYPEDEF, not a struct tag --
// so declaring a struct by that name here does not merge with the SDK's, it
// collides with it (C2371, found by the conformance job). On Windows afx.h
// has already pulled in <windows.h>, which declares the real one.
#ifndef _WIN32
struct SECURITY_ATTRIBUTES;
#endif

// Real MFC's signature: UINT __cdecl f(LPVOID). Both halves matter --
// eMule's thread functions return UINT and are declared AFX_CDECL, so a
// `long (*)(void*)` rejected every one of them at AfxBeginThread.
typedef UINT(EAFX_CDECL* EAFX_THREADPROC)(void*);

// The kernel32 constants the threading layer needs (real target: <windows.h>).
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

// ---------------------------------------------------------------------
// CWinThread (header afxwin.h; here CObject -> CWinThread, see the banner)
// ---------------------------------------------------------------------
class ECWinThread : public ECObject
{
public:
    // Not on the Learn CWinThread page (which lists only m_bAutoDelete,
    // m_hThread, m_nThreadID, m_pActiveWnd, m_pMainWnd), but real: eMule
    // clears it directly ("m_pThread->m_pThreadParams = NULL;").
    LPVOID m_pThreadParams = nullptr;

public:
    BOOL m_bAutoDelete;
    void* m_hThread;
    DWORD m_nThreadID;

    ECWinThread();
    // The worker-thread form: eMule constructs one directly with its
    // thread procedure and parameter.
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
    // How a thread with m_bAutoDelete cleared is disposed of; eMule's
    // CGDIThread/CPreviewThread override it.
    virtual void Delete();
    // Real MFC declares ~CWinThread (it closes the OS handle and frees the
    // thread's state). Needed here so the pimpl below is released.
    virtual ~ECWinThread();

private:
    // Internal implementation state (the std::thread, the worker procedure,
    // and the create-suspended gate). Kept OUT of this frozen MFC-subset
    // interface: it is a simple_mfc implementation mechanism, not part of
    // the MFC contract, so it is private, opaque, and its name/shape are our
    // own (real MFC's equivalents -- m_pfnThreadProc, m_hThread's real HANDLE
    // -- are internal too). Defined in src/afxwin.cpp. Derived classes never
    // touch it; they use the public methods above.
    struct Impl;
    Impl* m_pImpl = nullptr;
};

// ---------------------------------------------------------------------
// Global Afx* functions (header afxwin.h)
// ---------------------------------------------------------------------
ECWinThread* EAfxBeginThread(EAFX_THREADPROC pfnThreadProc, void* pParam,
                           int nPriority = 0 /*THREAD_PRIORITY_NORMAL*/,
                           UINT nStackSize = 0, DWORD dwCreateFlags = 0,
                           SECURITY_ATTRIBUTES* lpSecurityAttrs = nullptr);
ECWinThread* EAfxBeginThread(ECRuntimeClass* pThreadClass,
                           int nPriority = 0, UINT nStackSize = 0,
                           DWORD dwCreateFlags = 0,
                           SECURITY_ATTRIBUTES* lpSecurityAttrs = nullptr);
