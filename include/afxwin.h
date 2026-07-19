// afxwin.h — reference STUB (declarations only, no implementation).
// Transitively includes afx.h (as in real MFC). Contains the
// window/thread/application classes and the message-map macros, with
// signatures verified against the official Microsoft Learn documentation.
#pragma once
#include "afx.h"

class CWnd;
struct SECURITY_ATTRIBUTES;
typedef long (*AFX_THREADPROC)(void*);

// ---------------------------------------------------------------------
// CCmdTarget — base of CWinThread for command routing (header afxwin.h)
// ---------------------------------------------------------------------
class CCmdTarget : public CObject {};

// ---------------------------------------------------------------------
// CWinThread (header afxwin.h, hierarchy CObject -> CCmdTarget -> CWinThread)
// ---------------------------------------------------------------------
class CWinThread : public CCmdTarget
{
public:
    BOOL m_bAutoDelete;
    void* m_hThread;
    DWORD m_nThreadID;

    CWinThread();
    BOOL CreateThread(DWORD dwCreateFlags = 0, UINT nStackSize = 0,
                       SECURITY_ATTRIBUTES* lpSecurityAttrs = nullptr);
    DWORD ResumeThread();
    DWORD SuspendThread();
    BOOL SetThreadPriority(int nPriority);
    int GetThreadPriority();
    virtual BOOL InitInstance();
    virtual int ExitInstance();
    virtual int Run();
};

// ---------------------------------------------------------------------
// CWinApp (header afxwin.h, derives from CWinThread)
// ---------------------------------------------------------------------
class CWinApp : public CWinThread
{
public:
    UINT GetProfileInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, int nDefault);
    BOOL WriteProfileInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, int nValue);
    CString GetProfileString(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszDefault = nullptr);
    virtual BOOL OnIdle(LONG lCount);
};

// ---------------------------------------------------------------------
// CWnd — base of all windows/controls (header afxwin.h)
// Skeleton only: the concrete methods actually used live entirely on the
// subclasses (CDialog, CStatic, CEdit, ...), not directly on CWnd.
// ---------------------------------------------------------------------
class CWnd : public CCmdTarget {};

class CDialog : public CWnd {};
class CFrameWnd : public CWnd {};
class CStatic : public CWnd {};
class CEdit : public CWnd {};
class CListBox : public CWnd {};
class CComboBox : public CWnd {};
class CButton : public CWnd {};
class CDocument : public CCmdTarget {};

// CControlBar and CDialogBar are NOT declared here: they really belong to
// the afxext.h header (per Microsoft Learn), see afxext.h.
// CPropertyPage and CPropertySheet are NOT declared here: they really
// belong to the afxdlgs.h header (per Microsoft Learn), see afxdlgs.h.

// ---------------------------------------------------------------------
// Message-map demarcation macros (per Microsoft Learn: header afxwin.h).
// The "entry" macros (ON_COMMAND, ON_MESSAGE, ON_CONTROL, ON_NOTIFY, ...)
// are NOT declared here: they really belong to the afxmsg_.h header,
// which afxwin.h includes below (as in real MFC: a single
// #include "afxwin.h" also exposes the ON_* macros).
// ---------------------------------------------------------------------
#define DECLARE_MESSAGE_MAP()
#define BEGIN_MESSAGE_MAP(theClass, baseClass)
#define END_MESSAGE_MAP()

#include "afxmsg_.h"

// ---------------------------------------------------------------------
// Global Afx* functions (header afxwin.h)
// ---------------------------------------------------------------------
CWinThread* AfxBeginThread(AFX_THREADPROC pfnThreadProc, void* pParam,
                            int nPriority = 0 /*THREAD_PRIORITY_NORMAL*/,
                            UINT nStackSize = 0, DWORD dwCreateFlags = 0,
                            SECURITY_ATTRIBUTES* lpSecurityAttrs = nullptr);
CWinThread* AfxBeginThread(CRuntimeClass* pThreadClass,
                            int nPriority = 0, UINT nStackSize = 0,
                            DWORD dwCreateFlags = 0,
                            SECURITY_ATTRIBUTES* lpSecurityAttrs = nullptr);

CWinApp* AfxGetApp();
void* AfxGetInstanceHandle();
void* AfxGetResourceHandle();
CWnd* AfxGetMainWnd();
int AfxMessageBox(LPCTSTR lpszText, UINT nType = 0, UINT nIDHelp = 0);

// ---------------------------------------------------------------------
// CDataExchange — object passed to DoDataExchange (header afxwin.h per
// Microsoft Learn; the DDX_*/DDV_* functions that use it stay in
// afxdd_.h, where real MFC puts them). No base class.
// ---------------------------------------------------------------------
class CDataExchange
{
public:
    CWnd* m_pDlgWnd;
    BOOL m_bSaveAndValidate;
};
