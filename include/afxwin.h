// afxwin.h — reference STUB (declarations only, no implementation).
// Transitively includes afx.h (as in real MFC). Contains the
// window/thread/application classes, the core GDI classes, and the
// message-map macros, with signatures verified against the official
// Microsoft Learn documentation for the subset of methods actually used
// by eMule/srchybrid (see ../../mfc_scan_srchybrid.md).
#pragma once
// Real MFC's afxwin.h defines this include guard, and downstream headers
// (e.g. eMule's Emule.h) #error out with "include 'stdafx.h' before ..."
// unless it is set -- it is their proxy for "the MFC core headers are in".
#ifndef __AFXWIN_H__
#define __AFXWIN_H__
#endif
#include "afx.h"
#include "atltypes.h"
#include <cstddef> // offsetof, used by METHOD_PROLOGUE below

// ---------------------------------------------------------------------
// afx_msg is real MFC's marker keyword on every message-handler declaration
// (`afx_msg void OnPaint();`). It expands to nothing -- but it MUST be defined,
// otherwise `afx_msg void Foo();` parses as two adjacent declarations and every
// handler line becomes a C2144 syntax error. eMule uses it 816 times, so its
// absence alone was cascading through 150+ files.
#define afx_msg
// Real MFC makes the template collections (CArray/CList/CMap and the
// CTypedPtr* wrappers) and the concrete string maps available transitively
// through the standard <afxwin.h> include chain; eMule/srchybrid relies on
// that (its Stdafx.h uses CArray/CTypedPtrList without a direct
// <afxtempl.h> include), so pull them in here too.
#include "afxtempl.h"

class CWnd;
class CDC;
class CMenu;
class CBitmap;
class CRgn;
class CPalette;
class CCreateContext;
class CDataExchange;
// Real MFC's signature: UINT __cdecl f(LPVOID). Both halves matter --
// eMule's thread functions return UINT and are declared AFX_CDECL, so a
// `long (*)(void*)` rejected every one of them at AfxBeginThread.
typedef UINT(AFX_CDECL* AFX_THREADPROC)(void*);

// ---------------------------------------------------------------------
// Win32 primitive handle/type stand-ins (normally from windef.h/winnt.h
// /winuser.h). On a real Windows/MSVC target these come from the real
// <windows.h> instead of being redefined here: discovered (2026-07-20,
// compiling real eMule/srchybrid against this header on windows-latest)
// that eMule also includes real Win32 headers directly (<winsock2.h>
// etc., for non-MFC networking), which pull in the actual HWND & friends
// (windef.h), SECURITY_ATTRIBUTES/CREATESTRUCT/HELPINFO/TOOLINFO
// (winbase.h/winuser.h) — several of these are typedef-NAMES for a
// differently-tagged real struct (e.g. real SECURITY_ATTRIBUTES aliases
// `struct _SECURITY_ATTRIBUTES`, not a struct literally tagged
// SECURITY_ATTRIBUTES), so our bare forward-declarations collided with
// them (C2371 "redefinition; different basic types"). On non-Windows
// targets (this project's main portability point) none of these headers
// exist, so we still provide our own stand-ins there.
// ---------------------------------------------------------------------
#ifdef _WIN32
#include <windows.h>
#include <commctrl.h> // TOOLINFO is a commctrl.h type, not windows.h
#include <shlwapi.h>  // PathFindExtension/PathAddBackslash & co, which
                      // eMule calls unqualified expecting MFC to have
                      // pulled them in (real MFC's headers do)
#include <shlobj.h>   // CSIDL_* shell folder ids
#include <shobjidl.h> // ITaskbarList3: CEMuleDlg holds one as a CComPtr
#include <uxtheme.h>  // OpenThemeData & co, used by eMule's skinned controls
#include <vssym32.h>  // the theme part/state ids (TABP_*, BP_*, CBS_*, TMT_*)
                      // member, so all 81 TUs that see EmuleDlg.h need the
                      // interface declared. Shell COM, not MFC -- eMule
                      // never includes it itself either, it inherits it
                      // from the MFC headers exactly like this.
#else
// __stdcall on Windows, where it is part of the callback's type; nothing
// to express off it.
#define CALLBACK
using HWND = void*;
using HINSTANCE = void*;
using HDC = void*;
using HICON = void*;
using HCURSOR = void*;
using HBRUSH = void*;
using HFONT = void*;
using HMENU = void*;
using HGDIOBJ = void*;
using HPALETTE = void*;
using HPEN = void*;
using HBITMAP = void*;
using HRGN = void*;
using HGLOBAL = void*;
using LPVOID = void*;
using BYTE = unsigned char;
using COLORREF = unsigned long;
using UINT_PTR = std::uintptr_t;
using DWORD_PTR = std::uintptr_t;
using INT_PTR = long long; // matches afxcoll.h's INT_PTR (identical redefinition is legal if both headers are included together)
using LONG_PTR = std::intptr_t;
using WPARAM = UINT_PTR;
using LPARAM = LONG_PTR;
using LRESULT = LONG_PTR;
struct SECURITY_ATTRIBUTES;
struct tagCREATESTRUCT;
using CREATESTRUCT = tagCREATESTRUCT;
using LPCREATESTRUCT = CREATESTRUCT*;
struct HELPINFO;
struct TOOLINFO;
struct WINDOWPLACEMENT;
struct NCCALCSIZE_PARAMS;
struct tagMENUINFO;
using MENUINFO = tagMENUINFO;
using LPMENUINFO = MENUINFO*;
using LPCMENUINFO = const MENUINFO*;
struct MENUITEMINFOW;
using MENUITEMINFO = MENUITEMINFOW;
using LPMENUITEMINFO = MENUITEMINFO*;
struct SCROLLINFO;
struct tagTEXTMETRIC;
using TEXTMETRIC = tagTEXTMETRIC;
#endif

// ---------------------------------------------------------------------
// Additional Win32 primitive stand-ins (FRONTEND/GDI blind-spot pass,
// see ../../mfc_scan_srchybrid.md addendum): needed only by the CWnd
// message-handler declarations below, which real code reaches through
// qualified super-calls (e.g. CDialog::DoDataExchange(),
// CWnd::OnDestroy()) invisible to a plain ".Method("/"->Method(" scan.
// All incomplete/forward-declared: only ever used by pointer here.
// Unlike the block above, these do NOT collide on a real Windows target
// (their real counterparts share the exact same tag name, e.g. real
// windows.h also has a struct literally tagged "tagMSG" — forward-
// declaring the same tag twice, later completed by the real definition,
// is legal C++, not a redefinition), so no #ifdef _WIN32 needed here.
// ---------------------------------------------------------------------
#ifndef _WIN32
struct tagMSG;
using MSG = tagMSG;
using LPMSG = MSG*;
#endif
// The owner-draw callback structures. Bare-tag forward declarations, so
// they merge with the real winuser.h definitions rather than conflicting
// (same rule as tagMSG above).
struct tagMEASUREITEMSTRUCT;
using LPMEASUREITEMSTRUCT = tagMEASUREITEMSTRUCT*;
struct tagDRAWITEMSTRUCT;
using LPDRAWITEMSTRUCT = tagDRAWITEMSTRUCT*;
struct tagCOMPAREITEMSTRUCT;
using LPCOMPAREITEMSTRUCT = tagCOMPAREITEMSTRUCT*;
struct tagDELETEITEMSTRUCT;
using LPDELETEITEMSTRUCT = tagDELETEITEMSTRUCT*;
class CScrollBar; // real header afxwin.h too; only used here as a pointer parameter

// Real MFC's CWnd-derived classes intentionally hide the base Create()
// overload set with their own Create() (different signature per class):
// that's the actual API shape, not a mistake, but it trips
// -Woverloaded-virtual/C4266. Suppressed for this declaration-only
// hierarchy (afxwin.h/afxext.h/afxdlgs.h/afxcmn.h).
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4266)
#endif

// ---------------------------------------------------------------------
// Message-map data structures (header afxwin.h in real MFC too).
//
// These used to be absent, because every ON_* macro and every
// BEGIN/END_MESSAGE_MAP expanded to nothing: a message map declares
// handlers, and this library implements no dispatch, so an empty
// expansion looked sufficient. It is not. eMule writes message-map
// entries BY HAND -- its own _ON_WM_THEMECHANGED() (ButtonsTabCtrl.cpp,
// ClosableTabCtrl.cpp, DialogMinTrayBtn.cpp) expands to a braced
// AFX_MSGMAP_ENTRY aggregate ending in a comma:
//
//   { _WM_THEMECHANGED, 0, 0, 0, AfxSig_l, (AFX_PMSG)(AFX_PMSGW)
//     (static_cast<LRESULT (AFX_MSG_CALL CWnd::*)(void)>(_OnThemeChanged)) },
//
// which is only legal inside the array real BEGIN_MESSAGE_MAP opens. With
// no-op macros it landed at file scope and produced C2447/C2059. So the
// macros below reproduce real MFC's expansion faithfully -- that, and not
// a lookalike, is what makes eMule's hand-written entries compile, and it
// is also why every ON_* entry macro can keep expanding to nothing: they
// simply contribute no element to the array.
//
// Note the entry macros are the ONLY thing that stays empty. The
// demarcation macros now generate the two functions real MFC generates,
// so a class that writes BEGIN_MESSAGE_MAP without DECLARE_MESSAGE_MAP
// no longer compiles here either -- exactly as under real MFC.
// ---------------------------------------------------------------------
#ifndef _WIN32
// __stdcall on Windows (windef.h), where it is part of the function's
// type; nothing to express off it. Same treatment as CALLBACK above.
#define PASCAL
#endif
// Real MFC's marker on message handlers' calling convention: empty on
// every current target, kept because eMule spells it out in its own
// entry macros (static_cast<LRESULT (AFX_MSG_CALL CWnd::*)(void)>).
#define AFX_MSG_CALL

struct AFX_MSGMAP_ENTRY;

struct AFX_MSGMAP
{
    const AFX_MSGMAP* (PASCAL* pfnGetBaseMap)();
    const AFX_MSGMAP_ENTRY* lpEntries;
};

// The pointer-to-member types an entry stores its handler as. Handlers
// have every possible signature, so MFC casts them all to one type; the
// CWnd form exists because a cast has to go through the class the
// handler is actually declared in before being flattened to AFX_PMSG.
class CCmdTarget; // defined just below; named here by the handler types
typedef void (AFX_MSG_CALL CCmdTarget::*AFX_PMSG)(void);
typedef void (AFX_MSG_CALL CWnd::*AFX_PMSGW)(void);

struct AFX_MSGMAP_ENTRY
{
    UINT nMessage;   // the Windows message
    UINT nCode;      // control notification code, or WM_NOTIFY code
    UINT nID;        // control id (0 for a plain window message)
    UINT nLastID;    // the end of the id range, for the _RANGE entries
    UINT_PTR nSig;   // which handler signature pfn really has
    AFX_PMSG pfn;    // the handler itself
};

// The handler-signature tags. Real MFC's enum is far longer (one tag per
// distinct handler prototype, generated for its own dispatcher); only
// the two that eMule names are declared: AfxSig_end terminates the array
// below, AfxSig_l is what its hand-written _ON_WM_THEMECHANGED() uses
// (an LRESULT-returning, argument-less handler).
enum AfxSig
{
    AfxSig_end = 0,
    AfxSig_l
};

// What a class puts in its own body to own a message map. Real MFC's
// expansion ends with "protected:", so the handlers declared after it in
// a class body are protected -- which is what makes a derived class's
// qualified super-call (CTrayDialog::OnSysCommand(...)) legal. Expanding
// to nothing left them private and produced C2248.
// GetThisMessageMap is static: that is what lets END_MESSAGE_MAP below
// write "&TheBaseClass::GetThisMessageMap" for ANY base without every
// base having to redeclare it -- CCmdTarget declares it once and the
// whole hierarchy inherits it.
// The counterpart macros BEGIN_MESSAGE_MAP/END_MESSAGE_MAP are defined
// further down, next to afxmsg_.h's entry macros: only .cpp files expand
// them, so they need nothing declared here.
#define DECLARE_MESSAGE_MAP()                                    \
protected:                                                       \
    static const AFX_MSGMAP* PASCAL GetThisMessageMap();         \
    virtual const AFX_MSGMAP* GetMessageMap() const;

// ---------------------------------------------------------------------
// CCmdTarget — base of CWinThread for command routing (header afxwin.h)
// ---------------------------------------------------------------------
// The routing context OnCmdMsg reports back into; opaque to callers.
struct AFX_CMDHANDLERINFO;
#ifndef _WIN32
struct IUnknown;
using LPUNKNOWN = IUnknown*;
using LPVOID = void*;
#endif

class CCmdTarget : public CObject
{
public:
    // COM plumbing for the nested interface parts. "External" is the
    // reference count seen by clients; "Internal" is the object's own,
    // and they differ only when the object is aggregated.
    DWORD ExternalQueryInterface(const void* iid, LPVOID* ppvObj);
    DWORD ExternalAddRef();
    DWORD ExternalRelease();
    DWORD InternalQueryInterface(const void* iid, LPVOID* ppvObj);
    DWORD InternalAddRef();
    DWORD InternalRelease();
    // Hands out one of this object's interfaces without adding a
    // reference (eMule: "(IDataObject*)pbdo->GetInterface(&IID_IDataObject)").
    LPUNKNOWN GetInterface(const void* iid);
    LPUNKNOWN GetControllingUnknown();
    void EnableAutomation();
    void EnableAggregation();

    // Command routing, and the wait-cursor helpers real MFC puts here.
    virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
    void BeginWaitCursor();
    void EndWaitCursor();
    void RestoreWaitCursor();

    // Real MFC ends CCmdTarget's body with this too. Declaring it here
    // and nowhere else is deliberate: GetThisMessageMap is static, so
    // every class in the hierarchy inherits a usable
    // "TheBaseClass::GetThisMessageMap" for END_MESSAGE_MAP to point the
    // base-map link at, whatever class eMule names as its base.
    DECLARE_MESSAGE_MAP()
};

// ---------------------------------------------------------------------
// CWinThread (header afxwin.h, hierarchy CObject -> CCmdTarget -> CWinThread)
// ---------------------------------------------------------------------
class CWinThread : public CCmdTarget
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
    CWnd* m_pMainWnd;   // real MFC public data members
    CWnd* m_pActiveWnd;

    CWinThread();
    // The worker-thread form: eMule constructs one directly with its
    // thread procedure and parameter.
    CWinThread(AFX_THREADPROC pfnThreadProc, LPVOID pParam);
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
};

// ---------------------------------------------------------------------
// CCommandLineInfo (header afxwin.h, derives from CObject) — the parsed
// command line CWinApp::ParseCommandLine fills in. eMule reads the two
// fields that matter for a shell "open this ed2k/magnet link" launch.
// ---------------------------------------------------------------------
class CCommandLineInfo : public CObject
{
public:
    CCommandLineInfo();
    // Called once per token by ParseCommandLine; overriding it is how an
    // application adds its own switches.
    virtual void ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast);

    BOOL m_bShowSplash;
    BOOL m_bRunEmbedded;
    BOOL m_bRunAutomated;
    // Which document command the shell asked for. eMule compares against
    // FileOpen to tell "opened with a file/link" from a plain start.
    enum
    {
        FileNew,
        FileOpen,
        FilePrint,
        FilePrintTo,
        FileDDE,
        AppRegister,
        AppUnregister,
        FileNothing = -1
    } m_nShellCommand;
    CString m_strFileName;
    CString m_strPrinterName;
    CString m_strDriverName;
    CString m_strPortName;
};

// ---------------------------------------------------------------------
// CWinApp (header afxwin.h, derives from CWinThread)
// ---------------------------------------------------------------------
class CWinApp : public CWinThread
{
public:
    // Real MFC public data members (winmain sets them; app code reads them,
    // e.g. eMule's GetProfileFile() returns m_pszProfileName).
    HINSTANCE m_hInstance;
    HINSTANCE m_hPrevInstance;
    LPTSTR    m_lpCmdLine;
    int       m_nCmdShow;
    LPCTSTR   m_pszAppName;
    LPCTSTR   m_pszRegistryKey;
    LPCTSTR   m_pszExeName;
    LPCTSTR   m_pszHelpFilePath;
    LPCTSTR   m_pszProfileName;
    // The help context a message box last asked for, so that the app's
    // Help command can answer about that box instead of the app itself.
    // Public in real MFC (an implementation member, not on its
    // documented member list); eMule reads it in CemuleApp::OnHelp.
    DWORD m_dwPromptContext;

    // The name form of the constructor: eMule's app object forwards its
    // own name to it ("CemuleApp::CemuleApp(LPCTSTR lpszAppName)
    // : CWinApp(lpszAppName)"), and the default makes the no-argument
    // form real MFC also offers work.
    explicit CWinApp(LPCTSTR lpszAppName = nullptr);

    UINT GetProfileInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, int nDefault);
    BOOL WriteProfileInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, int nValue);
    CString GetProfileString(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszDefault = nullptr);
    virtual BOOL OnIdle(LONG lCount);
    // The predefined system cursors (IDC_SIZEWE etc.), as opposed to
    // LoadCursor's application resources.
    HICON LoadIcon(UINT nIDResource) const;
    HICON LoadIcon(LPCTSTR lpszResourceName) const;
    HCURSOR LoadCursor(UINT nIDResource) const;
    HCURSOR LoadCursor(LPCTSTR lpszResourceName) const;
    HCURSOR LoadStandardCursor(LPCTSTR lpszCursorName) const;
    HICON LoadStandardIcon(LPCTSTR lpszIconName) const;
    // Called from the message pump to decide whether a message counts as
    // user activity; eMule overrides it and super-calls this one.
    virtual BOOL IsIdleMessage(MSG* pMsg);

    // Help. EnableHtmlHelp switches the application from the old WinHelp
    // engine to HTML Help (eMule calls it in its constructor);
    // WinHelpInternal is the entry point the framework routes a help
    // request through once a path has been set, and is virtual because
    // EnableHtmlHelp works by overriding it.
    void EnableHtmlHelp();
#ifdef _WIN32
    virtual void WinHelpInternal(DWORD_PTR dwData, UINT nCmd = HELP_CONTEXT);
#else
    virtual void WinHelpInternal(DWORD_PTR dwData, UINT nCmd = 1);
#endif

    // Splits the process command line into the fields of a
    // CCommandLineInfo, calling its ParseParam for each token.
    void ParseCommandLine(CCommandLineInfo& rCmdInfo);
};

// Enables an ActiveX control container in a dialog-based application;
// eMule calls it before creating its browser-hosting dialogs.
void AFXAPI AfxEnableControlContainer(void* pOccManager = nullptr);
// Loads the RichEdit 4.1/5.0 window class (MSFTEDIT.DLL) so that a
// dialog template's RICHEDIT50W controls can be created. Returns FALSE
// if the library is missing, which is what eMule tests.
BOOL AFXAPI AfxInitRichEdit5();

// ---------------------------------------------------------------------
// GDI classes (header afxwin.h per Microsoft Learn — CImageList is the
// one exception, it lives in afxcmn.h, see there). Declared before CWnd
// because CWnd::GetDC/ReleaseDC and CMenu::AppendMenu reference them.
// ---------------------------------------------------------------------

// CGdiObject — base of CBitmap/CBrush/CFont/CPalette/CPen/CRgn (header
// afxwin.h, hierarchy CObject -> CGdiObject). DeleteObject/Attach/
// Detach/GetSafeHandle/GetObject are genuinely defined once here, not
// re-implemented per subclass (verified against Microsoft Learn).
class CGdiObject : public CObject
{
public:
    // Public in real MFC too, and eMule reads it directly as the "is this
    // object created?" test (`if (theApp.m_fontSymbol.m_hObject)`) and to
    // pass the raw handle to SendMessage(WM_SETFONT).
    HGDIOBJ m_hObject = nullptr;

    // Real MFC's GDI wrappers convert implicitly to their own handle type,
    // which is how eMule returns a CBrush where an HBRUSH is expected.
    operator HGDIOBJ() const { return m_hObject; }

    BOOL DeleteObject();
    BOOL Attach(HGDIOBJ hObject);
    HGDIOBJ Detach();
    HGDIOBJ GetSafeHandle() const;
    int GetObject(int nCount, LPVOID lpObject) const;
};

// LOGBRUSH/LOGFONT (like CREATESTRUCT/HELPINFO/TOOLINFO above) are
// typedef-names for a real, differently-tagged ANSI/Unicode-dispatched
// struct on Windows (e.g. real LOGFONT aliases tagLOGFONTW) -- guarded
// the same way, real <windows.h> already pulled in above provides them.
#ifndef _WIN32
struct tagLOGBRUSH;
using LOGBRUSH = tagLOGBRUSH;
#endif

// CPen (header afxwin.h, deriva da CGdiObject)
class CPen : public CGdiObject
{
public:
    operator HPEN() const { return (HPEN)m_hObject; }
    CPen();
    CPen(int nPenStyle, int nWidth, COLORREF crColor);
    CPen(int nPenStyle, int nWidth, const LOGBRUSH* pLogBrush, int nStyleCount = 0, const DWORD* lpStyle = nullptr);

    BOOL CreatePen(int nPenStyle, int nWidth, COLORREF crColor);
    BOOL CreatePen(int nPenStyle, int nWidth, const LOGBRUSH* pLogBrush, int nStyleCount = 0, const DWORD* lpStyle = nullptr);
};

// CBrush (header afxwin.h, deriva da CGdiObject)
class CBrush : public CGdiObject
{
public:
    operator HBRUSH() const { return (HBRUSH)m_hObject; }
    CBrush();
    CBrush(COLORREF crColor);
    CBrush(int nIndex, COLORREF crColor);
    explicit CBrush(CBitmap* pBitmap);

    BOOL CreateSolidBrush(COLORREF crColor);
    BOOL CreateHatchBrush(int nIndex, COLORREF crColor);
    BOOL CreatePatternBrush(CBitmap* pBitmap);
    BOOL CreateDIBPatternBrush(HGLOBAL hPackedDIB, UINT nUsage);
    BOOL CreateDIBPatternBrush(const void* lpPackedDIB, UINT nUsage);
    // The LOGBRUSH form, which is how eMule builds its 8x8 pattern brush
    // (Emule.cpp:1780) after creating the bitmap by hand below.
    BOOL CreateBrushIndirect(const LOGBRUSH* lpLogBrush);
};

struct tagLOGPALETTE;
using LOGPALETTE = tagLOGPALETTE;
using LPLOGPALETTE = LOGPALETTE*;

// CPalette (header afxwin.h, deriva da CGdiObject). Was previously only
// forward-declared (used as an incomplete pointer-only type in
// CDC::SelectObject/SelectPalette); given a real definition here because
// eMule/srchybrid genuinely instantiates and uses one (ColourPopup.cpp:
// m_Palette.CreatePalette(pLogPalette)/.DeleteObject()/pDC->SelectPalette(
// &m_Palette, FALSE), m_Palette declared as a plain CPalette member in
// ColourPopup.h) — found during the FRONTEND/GDI blind-spot pass, see
// ../../mfc_scan_srchybrid.md addendum. DeleteObject is inherited from
// CGdiObject, not redeclared here.
class CPalette : public CGdiObject
{
public:
    operator HPALETTE() const { return (HPALETTE)m_hObject; }
    BOOL CreatePalette(LPLOGPALETTE lpLogPalette);
};

// CBitmap (header afxwin.h, deriva da CGdiObject)
class CBitmap : public CGdiObject
{
public:
    operator HBITMAP() const { return (HBITMAP)m_hObject; }
    // The from-scratch form (no DC involved): eMule builds a 1bpp 8x8
    // pattern from a static WORD[8] with it (Emule.cpp:1775).
    BOOL CreateBitmap(int nWidth, int nHeight, UINT nPlanes, UINT nBitcount, const void* lpBits);
    BOOL CreateCompatibleBitmap(CDC* pDC, int nWidth, int nHeight);
    int GetBitmap(struct tagBITMAP* pBitMap);
    DWORD GetBitmapBits(DWORD dwCount, void* lpBits) const;
    DWORD SetBitmapBits(DWORD dwCount, const void* lpBits);
    BOOL LoadBitmap(LPCTSTR lpszResourceName);
    BOOL LoadBitmap(UINT nIDResource);
    CSize GetBitmapDimension() const;
};

// CRgn (header afxwin.h, deriva da CGdiObject). CCreditsThread holds one
// by value (`CRgn m_rgnScreen;`), so the forward declaration at the top of
// this header is not enough -- the class has to be complete.
class CRgn : public CGdiObject
{
public:
    operator HRGN() const { return (HRGN)m_hObject; }
    BOOL CreateRectRgn(int x1, int y1, int x2, int y2);
    BOOL CreateRectRgnIndirect(const RECT* lpRect);
    int CombineRgn(CRgn* pRgn1, CRgn* pRgn2, int nCombineMode);
};

#ifndef _WIN32
struct tagLOGFONT;
using LOGFONT = tagLOGFONT;
#endif

// CFont (header afxwin.h, deriva da CGdiObject)
class CFont : public CGdiObject
{
public:
    operator HFONT() const { return (HFONT)m_hObject; }
    BOOL CreateFontIndirect(const LOGFONT* lpLogFont);
    int GetLogFont(LOGFONT* pLogFont);
    BOOL CreateFont(int nHeight, int nWidth, int nEscapement, int nOrientation, int nWeight,
                     BYTE bItalic, BYTE bUnderline, BYTE cStrikeOut, BYTE nCharSet,
                     BYTE nOutPrecision, BYTE nClipPrecision, BYTE nQuality,
                     BYTE nPitchAndFamily, LPCTSTR lpszFacename);
    BOOL CreatePointFont(int nPointSize, LPCTSTR lpszFaceName, CDC* pDC = nullptr);
};

// ---------------------------------------------------------------------
// CCreateContext (header afxwin.h, no base class) — the bundle MFC passes
// around while creating a frame/view. eMule declares one on the stack and
// fills in m_pNewViewClass to host a CFormView pane, so the forward
// declaration at the top of this header is not enough.
// ---------------------------------------------------------------------
class CDocument;
class CDocTemplate;
class CView;
class CFrameWnd;

class CCreateContext
{
public:
    CRuntimeClass* m_pNewViewClass = nullptr;
    CDocument* m_pCurrentDoc = nullptr;
    CDocTemplate* m_pNewDocTemplate = nullptr;
    CView* m_pLastView = nullptr;
    CFrameWnd* m_pCurrentFrame = nullptr;
};

// CDC (header afxwin.h, hierarchy CObject -> CDC)
class CDC : public CObject
{
public:
    // The wrapped device contexts, public in real MFC. CMemDC copies them
    // straight across (`m_hDC = pDC->m_hDC;`) and clears them on release,
    // so both have to be assignable members rather than accessors.
    HDC m_hDC = nullptr;
    HDC m_hAttribDC = nullptr;
    // TRUE while the DC is a printer DC. Public in real MFC, and eMule's
    // list controls branch on it to skip screen-only drawing.
    BOOL m_bPrinting = FALSE;

    // Lets a CDC be handed to a raw Win32 call that wants an HDC, which
    // eMule does directly (`FillRect(*pDC, &rc, hBrush)`).
    operator HDC() const { return m_hDC; }

    static CDC* FromHandle(HDC hDC);

    // Each overload returns the *previously selected object of the same
    // kind*, not the DC -- that is what makes the idiomatic
    // `CBitmap *pOld = dc.SelectObject(&bmp); ... dc.SelectObject(pOld);`
    // restore pattern (CMemDC, CBarShader, ...) compile. Selecting a region
    // is the odd one out and returns a region-type code. CPalette goes
    // through SelectPalette in real MFC, so it has no overload here.
    CGdiObject* SelectObject(CGdiObject* pObject);
    // Not on the Learn reference page, which lists only the six
    // CGdiObject-typed forms, but real: eMule assigns this one's result
    // to an HGDIOBJ and passes a CBitmap *object* (converted through
    // CGdiObject::operator HGDIOBJ) as the argument.
    HGDIOBJ SelectObject(HGDIOBJ hObject);
    virtual CFont* SelectObject(CFont* pFont);
    CBrush* SelectObject(CBrush* pBrush);
    CPen* SelectObject(CPen* pPen);
    CBitmap* SelectObject(CBitmap* pBitmap);
    int SelectObject(CRgn* pRgn);
    BOOL Attach(HDC hDC);
    HDC Detach();
    COLORREF SetTextColor(COLORREF crColor);
    virtual int DrawText(LPCTSTR lpszString, int nCount, LPRECT lpRect, UINT nFormat);
    int DrawText(const CString& str, LPRECT lpRect, UINT nFormat);
    void FillSolidRect(LPCRECT lpRect, COLORREF clr);
    void FillSolidRect(int x, int y, int cx, int cy, COLORREF clr);
    BOOL LineTo(int x, int y);
    BOOL LineTo(POINT point);
    CPoint MoveTo(int x, int y);
    CPoint MoveTo(POINT point);
    COLORREF SetBkColor(COLORREF crColor);
    COLORREF GetBkColor() const;
    COLORREF GetTextColor() const;
    BOOL DrawFrameControl(LPRECT lpRect, UINT nType, UINT nState);
    int SetPolyFillMode(int nPolyFillMode);
    int GetPolyFillMode() const;
    BOOL DeleteDC();
    int GetMapMode() const;
    // The mapping-mode extents/origins CMemDC mirrors from the DC it wraps.
    CSize GetWindowExt() const;
    CSize GetViewportExt() const;
    CPoint GetWindowOrg() const;
    CPoint GetViewportOrg() const;
    virtual CSize SetWindowExt(int cx, int cy);
    CSize SetWindowExt(SIZE size);
    virtual CSize SetViewportExt(int cx, int cy);
    CSize SetViewportExt(SIZE size);
    virtual CPoint SetWindowOrg(int x, int y);
    CPoint SetWindowOrg(POINT point);
    virtual CPoint SetViewportOrg(int x, int y);
    CPoint SetViewportOrg(POINT point);
    virtual int SelectClipRgn(CRgn* pRgn);
    int SelectClipRgn(CRgn* pRgn, int nMode);
    int SetBkMode(int nBkMode);
    BOOL CreateCompatibleDC(CDC* pDC);
    HDC GetSafeHdc();
    BOOL BitBlt(int x, int y, int nWidth, int nHeight, CDC* pSrcDC, int xSrc, int ySrc, DWORD dwRop);
    CSize GetTextExtent(LPCTSTR lpszString, int nCount);
    CSize GetTextExtent(const CString& str);
    BOOL TextOut(int x, int y, LPCTSTR lpszString, int nCount);
    BOOL TextOut(int x, int y, const CString& str);
    BOOL DrawEdge(LPRECT lpRect, UINT nEdge, UINT nFlags);
    UINT SetTextAlign(UINT nFlags);
    int GetDeviceCaps(int nIndex);
    void FrameRect(LPCRECT lpRect, CBrush* pBrush);
    void DrawFocusRect(LPCRECT lpRect);
    int SetROP2(int nDrawMode);
    int ExcludeClipRect(int x1, int y1, int x2, int y2);
    int ExcludeClipRect(LPCRECT lpRect);
    BOOL Rectangle(int x1, int y1, int x2, int y2);
    BOOL Rectangle(LPCRECT lpRect);
    CPalette* SelectPalette(CPalette* pPalette, BOOL bForceBackground);
    // TEXTMETRIC, not `struct tagTEXTMETRIC`: under UNICODE the real name
    // resolves to tagTEXTMETRICW, so the bare tag named a different type
    // than the one eMule actually passes.
    BOOL GetTextMetrics(TEXTMETRIC* lpMetrics) const;
    int GetClipBox(LPRECT lpRect);
    // Both overloads below fixed/added during the FRONTEND/GDI blind-spot
    // pass (see ../../mfc_scan_srchybrid.md addendum): real eMule usage
    // (TrayMenuBtn.cpp:114, TrayMenuBtn.cpp:105, ListBoxST.cpp:252) passes
    // a CBrush* (e.g. "(CBrush*)NULL"), not an HBRUSH, as the trailing
    // parameter — the text overload's last-parameter type was wrong, and
    // the HICON overload was entirely missing (verified against
    // Microsoft Learn's CDC::DrawState page: the CBrush*-taking overloads
    // are the ones with no matching HBITMAP overload actually used here).
    BOOL DrawState(CPoint pt, CSize size, LPCTSTR lpszText, UINT nFlags, BOOL bPrefixText = TRUE, int nTextLen = 0, CBrush* pBrush = nullptr);
    BOOL DrawState(CPoint pt, CSize size, HICON hIcon, UINT nFlags, CBrush* pBrush = nullptr);
    UINT RealizePalette();
    BOOL Polygon(LPPOINT lpPoints, int nCount);
    int SetMapMode(int nMapMode);
    CGdiObject* SelectStockObject(int nIndex);
    COLORREF SetPixel(int x, int y, COLORREF crColor);
    COLORREF SetPixel(POINT point, COLORREF crColor);
    COLORREF GetPixel(int x, int y);
    COLORREF GetPixel(POINT point);
    long TabbedTextOut(int x, int y, LPCTSTR lpszString, int nCount, int nTabPositions, const int* lpnTabStopPositions, int nTabOrigin);
    BOOL DrawIcon(int x, int y, HICON hIcon);
    BOOL DrawIcon(POINT point, HICON hIcon);
    void DPtoLP(LPPOINT lpPoints, int nCount = 1);
    void DPtoLP(LPRECT lpRect);
    void DPtoLP(LPSIZE lpSize);
    int SaveDC();
    BOOL RestoreDC(int nSavedDC);
    void Draw3dRect(LPCRECT lpRect, COLORREF clrTopLeft, COLORREF clrBottomRight);
    void Draw3dRect(int x, int y, int cx, int cy, COLORREF clrTopLeft, COLORREF clrBottomRight);
    UINT GetTextAlign();
    CSize GetOutputTextExtent(LPCTSTR lpszString, int nCount);
    CSize GetOutputTextExtent(const CString& str);
    BOOL IsPrinting();
    void FillRect(LPCRECT lpRect, CBrush* pBrush);
    void LPtoDP(LPPOINT lpPoints, int nCount = 1);
    void LPtoDP(LPRECT lpRect);
    void LPtoDP(LPSIZE lpSize);
    BOOL SetPixelV(int x, int y, COLORREF crColor);
    BOOL SetPixelV(POINT point, COLORREF crColor);
    // Bounds accumulation, which eMule's CMemoryDC resets before drawing.
    UINT SetBoundsRect(LPCRECT lpRectBounds, UINT flags);
    UINT GetBoundsRect(LPRECT lpRectBounds, UINT flags);
    BOOL ScrollDC(int dx, int dy, LPCRECT lpRectScroll, LPCRECT lpRectClip,
                  CRgn* pRgnUpdate, LPRECT lpRectUpdate);
    // The CString-taking form, alongside the count-based one above.
    CSize TabbedTextOut(int x, int y, const CString& str, int nTabPositions,
                        int* lpnTabStopPositions, int nTabOrigin);
};

// Device-context helpers (header afxwin.h). Real MFC derives each from CDC
// and wires up/tears down the DC in ctor/dtor; eMule only ever constructs
// them from a CWnd* ("CPaintDC dc(this);") and uses them as a CDC, so a
// declaration-only CWnd* constructor is all that is needed here.
class CPaintDC : public CDC
{
public:
    explicit CPaintDC(CWnd* pWnd);
};

class CClientDC : public CDC
{
public:
    explicit CClientDC(CWnd* pWnd);
};

class CWindowDC : public CDC
{
public:
    explicit CWindowDC(CWnd* pWnd);
};

// ---------------------------------------------------------------------
// CWnd — base of all windows/controls (header afxwin.h). Methods below
// are only the ones actually called on CWnd*/CWnd& in eMule/srchybrid
// (not the full real-MFC surface) — see ../../mfc_scan_srchybrid.md.
// ---------------------------------------------------------------------
// These are real Win32 *macros* (winuser.h), not just type names: on
// _WIN32 <windows.h> was already #included above in this same file, so
// by the time the preprocessor reaches these lines the macro is already
// live and would silently rewrite our own declaration's token (e.g.
// "constexpr UINT RDW_INVALIDATE = ..." becomes "constexpr UINT 0x0001 =
// ..." -- a syntax error) unless guarded with #ifndef rather than
// #ifdef _WIN32 (the two aren't equivalent for macros).
#ifndef RDW_INVALIDATE
constexpr UINT RDW_INVALIDATE = 0x0001;
#endif
#ifndef RDW_ERASE
constexpr UINT RDW_ERASE = 0x0004;
#endif
#ifndef RDW_UPDATENOW
constexpr UINT RDW_UPDATENOW = 0x0100;
#endif

// See CWnd::GetNextWindow below: winuser.h makes this name a two-argument
// macro, which would rewrite every member call into an unparsable one.
#undef GetNextWindow

class CWnd : public CCmdTarget
{
public:
    BOOL EnableWindow(BOOL bEnable = TRUE);
    BOOL ShowWindow(int nCmdShow);
    void GetWindowRect(LPRECT lpRect) const;
    virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle,
                         const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = nullptr);
    // The window handle, a public member in real MFC. eMule reads it
    // constantly, both as a validity test and to hand the raw handle to
    // Win32 calls.
    HWND m_hWnd = nullptr;
    operator HWND() const { return m_hWnd; }

    UINT_PTR SetTimer(UINT_PTR nIDEvent, UINT nElapse,
                       void(CALLBACK* lpfnTimer)(HWND, UINT, UINT_PTR, DWORD) = nullptr);
    BOOL KillTimer(UINT_PTR nIDEvent);
    UINT IsDlgButtonChecked(int nIDButton) const;

    LRESULT SendMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0);
    // Addresses a child control by id instead of by CWnd. eMule relies on
    // the trailing defaults (`SendDlgItemMessage(IDC_IP, EM_SETREADONLY,
    // TRUE)` passes no lParam).
    LRESULT SendDlgItemMessage(int nID, UINT message, WPARAM wParam = 0, LPARAM lParam = 0);
    // The message being handled right now; static because it is thread
    // state, not window state.
    static const MSG* GetCurrentMessage();
    void SetWindowText(LPCTSTR lpszString);
    void MoveWindow(int x, int y, int nWidth, int nHeight, BOOL bRepaint = TRUE);
    void MoveWindow(LPCRECT lpRect, BOOL bRepaint = TRUE);
    HWND Detach();
    BOOL Attach(HWND hWndNew);
    CWnd* SetFocus();
    void SetRedraw(BOOL bRedraw = TRUE);
    HWND GetSafeHwnd() const;
    void Invalidate(BOOL bErase = TRUE);
    BOOL ModifyStyle(DWORD dwRemove, DWORD dwAdd, UINT nFlags = 0);
    HICON SetIcon(HICON hIcon, BOOL bBigIcon);
    BOOL IsWindowVisible() const;
    virtual BOOL DestroyWindow();
    BOOL SetWindowPos(const CWnd* pWndInsertAfter, int x, int y, int cx, int cy, UINT nFlags);
    void UpdateWindow();
    void ScreenToClient(LPPOINT lpPoint) const;
    void ScreenToClient(LPRECT lpRect) const;
    BOOL ModifyStyleEx(DWORD dwRemove, DWORD dwAdd, UINT nFlags = 0);
    DWORD GetStyle() const;
    void SetDlgItemText(int nID, LPCTSTR lpszString);
    int GetWindowTextLength() const;
    void GetClientRect(LPRECT lpRect) const;
    BOOL PostMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0);
    void ClientToScreen(LPPOINT lpPoint) const;
    void ClientToScreen(LPRECT lpRect) const;
    void GetWindowText(CString& rString) const;
    int GetWindowText(LPTSTR lpszStringBuf, int nMaxCount) const;
    BOOL SetForegroundWindow();
    BOOL BringWindowToTop();
    BOOL IsWindowEnabled() const;
    CWnd* GetParent() const;
    CWnd* GetDlgItem(int nID) const;
    CMenu* GetMenu() const;
    BOOL SetMenu(CMenu* pMenu);
    CMenu* GetSystemMenu(BOOL bRevert) const;
    // The dialog-item helpers. Each one that is missing does not simply
    // fail to resolve: the unqualified call falls through to the global
    // Win32 function of the same name, which then rejects the arity
    // because it wants a leading HWND.
    int GetDlgCtrlID() const;
    // Window traversal and style queries. GetFocus/GetCapture are static
    // in real MFC and return CWnd*, not a raw HWND -- eMule assigns the
    // result straight to a CWnd* (`CWnd *pWndFocus = GetFocus();`).
    static CWnd* GetFocus();
    static CWnd* GetCapture();
    // Same shape as GetFocus/GetCapture, and missing it fails the same
    // silent way the note above describes: the unqualified call falls
    // through to the global Win32 ::FindWindowEx, which returns an HWND
    // where eMule assigns a CWnd* ("CWnd *pWnd = FindWindowEx(
    // GetSafeHwnd(), 0, _T("msctls_updown32"), 0);", TabCtrl.cpp:146).
    static CWnd* FindWindowEx(HWND hwndParent, HWND hwndChildAfter,
                              LPCTSTR lpszClass, LPCTSTR lpszWindow);
    CWnd* GetWindow(UINT nCmd) const;
    CWnd* ChildWindowFromPoint(POINT point) const;
    CWnd* ChildWindowFromPoint(POINT point, UINT nFlags) const;
    BOOL IsIconic() const;
    BOOL IsZoomed() const;
    BOOL FlashWindow(BOOL bInvert);
    BOOL IsChild(const CWnd* pWnd) const;
    CWnd* SetActiveWindow();
    CWnd* GetActiveWindow();
    void MapWindowPoints(CWnd* pwndTo, LPRECT lpRect) const;
    void MapWindowPoints(CWnd* pwndTo, LPPOINT lpPoint, UINT nCount) const;
    int SetWindowRgn(HRGN hRgn, BOOL bRedraw);
    int GetWindowRgn(HRGN hRgn) const;
    // Undocumented on the Learn CWnd page, but eMule calls it unqualified
    // from a dialog member, so MFC declares it somewhere in this chain.
    void PrepareForHelp();
    DWORD GetExStyle() const;
    BOOL GetScrollInfo(int nBar, SCROLLINFO* lpScrollInfo, UINT nMask = 0x17 /*SIF_ALL*/);
    BOOL SetScrollInfo(int nBar, SCROLLINFO* lpScrollInfo, BOOL bRedraw = TRUE);
    BOOL GetWindowPlacement(WINDOWPLACEMENT* lpwndpl) const;
    BOOL SetWindowPlacement(const WINDOWPLACEMENT* lpwndpl);
    BOOL EnableToolTips(BOOL bEnable = TRUE);
    // Routes the current message to the default window procedure.
    LRESULT Default();
    int GetDlgItemText(int nID, LPTSTR lpStr, int nMaxCount) const;
    int GetDlgItemText(int nID, CString& rString) const;
    void SetDlgItemInt(int nID, UINT nValue, BOOL bSigned = TRUE);
    UINT GetDlgItemInt(int nID, BOOL* lpTrans = nullptr, BOOL bSigned = TRUE) const;
    void CheckDlgButton(int nIDButton, UINT nCheck);
    void CheckRadioButton(int nIDFirstButton, int nIDLastButton, int nIDCheckButton);
    // Runs the DDX exchange in either direction.
    BOOL UpdateData(BOOL bSaveAndValidate = TRUE);
    void SetFont(CFont* pFont, BOOL bRedraw = TRUE);
    CFont* GetFont() const;
    CDC* GetDC();
    int ReleaseDC(CDC* pDC);
    BOOL RedrawWindow(LPCRECT lpRectUpdate = nullptr, CRgn* prgnUpdate = nullptr,
                       UINT flags = RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
    CWnd* SetCapture();
    void InvalidateRect(LPCRECT lpRect, BOOL bErase = TRUE);

    // Subclassing: eMule's control classes attach themselves to a window
    // created from the dialog template rather than creating it themselves.
    BOOL SubclassWindow(HWND hWnd);
    BOOL SubclassDlgItem(UINT nID, CWnd* pParent);
    HWND UnsubclassWindow();

    // GetNextWindow is a real winuser.h *macro* taking (hWnd, wCmd), so an
    // unqualified `pWnd->GetNextWindow()` expands to `GetWindow(,)` and
    // fails to parse. Undefined here for the same reason afx.h undefines
    // FindNextFile: keep the member's true name at every later call site.
    CWnd* GetNextWindow(UINT nFlag = 2 /*GW_HWNDNEXT*/) const;
    CWnd* GetTopWindow() const;
    CWnd* GetLastActivePopup() const;
    CWnd* GetTopLevelParent() const;
    CWnd* GetTopLevelFrame() const;
    CWnd* GetTopLevelOwner() const;
    // The owner is the window notifications go to, which is not always the
    // parent (real MFC keeps the distinction; eMule relies on it for its
    // floating bars).
    CWnd* GetOwner() const;
    void SetOwner(CWnd* pOwnerWnd);
    CDC* GetWindowDC();
    void CenterWindow(CWnd* pAlternateOwner = nullptr);
    int GetClassName(LPTSTR lpszClassName, int nMaxCount) const;

    // -----------------------------------------------------------------
    // Everything below was added during the FRONTEND/GDI blind-spot pass
    // (see ../../mfc_scan_srchybrid.md addendum): a plain textual
    // ".Method("/"->Method(" scan cannot see (a) static methods, which
    // are only ever called as "CWnd::Method(...)", or (b) qualified
    // super-calls like "CWnd::OnDestroy()" made by a derived class's own
    // override to reach the base behavior — both are pervasive in
    // eMule/srchybrid. Signatures verified against Microsoft Learn's
    // CWnd Class reference page.
    // -----------------------------------------------------------------
    static CWnd* FromHandle(HWND hWnd);
    static CWnd* FromHandlePermanent(HWND hWnd);
    static CWnd* GetDesktopWindow();
    static CWnd* WindowFromPoint(POINT point);

    virtual BOOL CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName,
                           DWORD dwStyle, int x, int y, int nWidth, int nHeight,
                           HWND hWndParent, HMENU nIDorHMenu, LPVOID lpParam = nullptr);
    virtual BOOL CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName,
                           DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID,
                           LPVOID lpParam = nullptr);

    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual void PreSubclassWindow();
    virtual void PostNcDestroy();
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    virtual void DoDataExchange(CDataExchange* pDX);
    // INT_PTR, not int: eMule's CMuleStatusBarCtrl overrides this and a
    // narrower return type is not a legal override (C2555).
    virtual INT_PTR OnToolHitTest(CPoint point, TOOLINFO* pTI) const;
    virtual BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
    virtual BOOL OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
    virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

    // WM_* message handlers actually reached via a qualified super-call
    // somewhere in eMule/srchybrid (e.g. CDialog::OnInitDialog() calling
    // through to CWnd, CStatic::OnPaint(), CTreeCtrl::OnMouseWheel(), ...).
    // Real MFC declares essentially all of these directly on CWnd, which
    // is what makes such super-calls valid regardless of which derived
    // class in the hierarchy actually names them in the call.
    virtual void OnPaint();
    virtual void OnDestroy();
    virtual void OnClose();
    virtual int OnCreate(LPCREATESTRUCT lpCreateStruct);
    virtual void OnSysColorChange();
    virtual BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    virtual void OnContextMenu(CWnd* pWnd, CPoint point);
    virtual void OnTimer(UINT_PTR nIDEvent);
    virtual void OnMouseMove(UINT nFlags, CPoint point);
    virtual BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    virtual void OnLButtonUp(UINT nFlags, CPoint point);
    virtual void OnLButtonDown(UINT nFlags, CPoint point);
    virtual void OnLButtonDblClk(UINT nFlags, CPoint point);
    virtual void OnRButtonDown(UINT nFlags, CPoint point);
    virtual void OnMButtonUp(UINT nFlags, CPoint point);
    virtual void OnNcLButtonDblClk(UINT nHitTest, CPoint point);
    virtual void OnNcDestroy();
    virtual void OnSize(UINT nType, int cx, int cy);
    virtual HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    virtual void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    virtual void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    virtual BOOL OnEraseBkgnd(CDC* pDC);
    virtual void OnSetFocus(CWnd* pOldWnd);
    virtual void OnKillFocus(CWnd* pNewWnd);
    virtual void OnActivateApp(BOOL bActive, DWORD dwThreadID);
    virtual BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    virtual BOOL OnQueryNewPalette();
    virtual void OnPaletteChanged(CWnd* pFocusWnd);
    virtual void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    virtual void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    virtual void OnSysCommand(UINT nID, LPARAM lParam);
    virtual void OnShowWindow(BOOL bShow, UINT nStatus);
    virtual void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
    virtual UINT OnGetDlgCode();
    virtual void OnCaptureChanged(CWnd* pWnd);
    virtual void OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hSysMenu);
    virtual void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
    virtual void OnCancelMode();
    afx_msg BOOL OnDeviceChange(UINT nEventType, DWORD_PTR dwData);
    // afx_msg, not virtual: real MFC dispatches these through the message
    // map, and Learn declares them exactly this way.
    afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
    afx_msg int OnCompareItem(int nIDCtl, LPCOMPAREITEMSTRUCT lpCompareItemStruct);
    afx_msg void OnDeleteItem(int nIDCtl, LPDELETEITEMSTRUCT lpDeleteItemStruct);
    virtual void OnNcPaint();
    virtual BOOL OnNcActivate(BOOL bActive);
    virtual LRESULT OnNcHitTest(CPoint point);
    virtual void OnNcLButtonDown(UINT nHitTest, CPoint point);
    virtual void OnNcRButtonDown(UINT nHitTest, CPoint point);
    virtual void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
    virtual LRESULT OnMenuChar(UINT nChar, UINT nFlags, CMenu* pMenu);
    virtual BOOL OnQueryEndSession();
    virtual void OnEndSession(BOOL bEnding);
    virtual void OnMove(int x, int y);
    virtual void OnEnable(BOOL bEnable);
    virtual void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
};

class CDialog : public CWnd
{
public:
    // Every eMule dialog passes its IDD (and usually a parent) straight to
    // the base, e.g. `CAddSourceDlg::CAddSourceDlg(CWnd *pParent)
    // : CDialog(CAddSourceDlg::IDD, pParent)`. The template can be named by
    // resource id or by string, and the default constructor exists for the
    // dialogs created through DYNCREATE.
    CDialog();
    explicit CDialog(UINT nIDTemplate, CWnd* pParentWnd = nullptr);
    explicit CDialog(LPCTSTR lpszTemplateName, CWnd* pParentWnd = nullptr);
    virtual INT_PTR DoModal();
    void EndDialog(int nResult);
    virtual BOOL Create(LPCTSTR lpszTemplateName, CWnd* pParentWnd = nullptr);
    virtual BOOL Create(UINT nIDTemplate, CWnd* pParentWnd = nullptr);
    virtual BOOL OnInitDialog();

protected:
    virtual void OnOK();
    virtual void OnCancel();
};

extern const CRect rectDefault;
#ifndef WS_OVERLAPPEDWINDOW
constexpr DWORD WS_OVERLAPPEDWINDOW = 0x00CF0000;
#endif

class CControlBar; // real header afxext.h; only a pointer parameter here

class CFrameWnd : public CWnd
{
public:
    virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName,
                         DWORD dwStyle = WS_OVERLAPPEDWINDOW, const RECT& rect = rectDefault,
                         CWnd* pParentWnd = nullptr, LPCTSTR lpszMenuName = nullptr,
                         DWORD dwExStyle = 0, CCreateContext* pContext = nullptr);
    virtual void RecalcLayout(BOOL bNotify = TRUE);
    // CControlBar lives in afxext.h (which includes us, not vice versa), so
    // forward-declared just above for this pointer parameter.
    void ShowControlBar(CControlBar* pBar, BOOL bShow, BOOL bDelay);
    CWnd* CreateView(CCreateContext* pContext, UINT nID = 0xE900 /*AFX_IDW_PANE_FIRST*/);

    // Docking. The frame is the dock site: it decides which edges accept
    // bars (EnableDocking), performs the docking (DockControlBar) and
    // persists the layout to the registry (Load/SaveBarState). Declared
    // here rather than in afxext.h because real MFC declares them on
    // CFrameWnd, whose definition lives in this header.
    void EnableDocking(DWORD dwDockStyle);
    void DockControlBar(CControlBar* pBar, UINT nDockBarID = 0, LPCRECT lpRect = nullptr);
    void FloatControlBar(CControlBar* pBar, CPoint point, DWORD dwStyle = 0x2000L /*CBRS_ALIGN_TOP*/);
    void LoadBarState(LPCTSTR lpszProfileName);
    void SaveBarState(LPCTSTR lpszProfileName) const;
    CControlBar* GetControlBar(UINT nID);
    virtual BOOL OnCreateClient(CREATESTRUCT* lpcs, CCreateContext* pContext);
};

// ---------------------------------------------------------------------
// CView — base of the document/view classes (header afxwin.h, hierarchy
// CObject -> CCmdTarget -> CWnd -> CView). eMule/srchybrid uses no
// document/view architecture at all: it only needs CView as the base of
// CFormView (afxext.h), which CTransferWnd/CSearchResultsWnd derive from
// to host a dialog template inside the main frame. So the document half
// (GetDocument/OnUpdate/OnDraw against CDocument) is deliberately absent
// -- every message handler those two override already comes from CWnd.
// ---------------------------------------------------------------------
class CView : public CWnd
{
public:
    virtual void OnInitialUpdate();
};

// ---------------------------------------------------------------------
// CScrollBar (header afxwin.h, deriva da CWnd). Reaches eMule as the
// third parameter of OnHScroll/OnVScroll, where it needs to be complete.
// ---------------------------------------------------------------------
class CScrollBar : public CWnd
{
public:
    int SetScrollPos(int nPos, BOOL bRedraw = TRUE);
    int GetScrollPos() const;
    void SetScrollRange(int nMinPos, int nMaxPos, BOOL bRedraw = TRUE);
    void GetScrollRange(int* lpMinPos, int* lpMaxPos) const;
    BOOL EnableScrollBar(UINT nArrowFlags = 0);
    void ShowScrollBar(BOOL bShow = TRUE);
};

class CStatic : public CWnd
{
public:
    virtual BOOL Create(LPCTSTR lpszText, DWORD dwStyle, const RECT& rect,
                         CWnd* pParentWnd, UINT nID = 0xffff);
    HBITMAP SetBitmap(HBITMAP hBitmap);
    HBITMAP GetBitmap() const;
    HICON SetIcon(HICON hIcon);
    HICON GetIcon() const;
};

class CEdit : public CWnd
{
public:
    // The edit control's formatting rectangle.
    void SetRect(LPCRECT lpRect);
    void SetRectNP(LPCRECT lpRect);
    void GetRect(LPRECT lpRect) const;
    virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
    void SetSel(DWORD dwSelection, BOOL bNoScroll = FALSE);
    void SetSel(int nStartChar, int nEndChar, BOOL bNoScroll = FALSE);
    void LimitText(int nChars = 0);
    void SetLimitText(UINT nMax);
    UINT GetLimitText() const;
    BOOL ShowBalloonTip(LPCTSTR lpszTitle, LPCTSTR lpszText, int ttiIcon = 0);
    DWORD GetSel() const;
    void GetSel(int& nStartChar, int& nEndChar) const;
    void ReplaceSel(LPCTSTR lpszNewText, BOOL bCanUndo = FALSE);
};

class CListBox : public CWnd
{
public:
    virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
    // Hit-testing by point; the high word of the result says whether the
    // point actually fell inside the item.
    UINT ItemFromPoint(POINT pt, BOOL& bOutside) const;
    int GetCount() const;
    int GetCurSel() const;
    int SetCurSel(int nSelect);
    int AddString(LPCTSTR lpszItem);
    DWORD_PTR GetItemData(int nIndex) const;
    int SetItemData(int nIndex, DWORD_PTR dwItemData);
    void ResetContent();
    int GetText(int nIndex, LPTSTR lpszBuffer) const;
    void GetText(int nIndex, CString& rString) const;
    int DeleteString(UINT nIndex);
    // Added during the FRONTEND/GDI blind-spot pass (see
    // ../../mfc_scan_srchybrid.md addendum): ListBoxST.cpp calls these
    // exclusively as "CListBox::GetItemDataPtr(...)"/etc. (qualified
    // super-calls from CListBoxST : public CListBox), invisible to the
    // original ".Method("/"->Method(" scan, which had marked all three
    // as "0 occurrences". Signatures verified against Microsoft Learn.
    void* GetItemDataPtr(int nIndex) const;
    int SetItemDataPtr(int nIndex, void* pData);
    int InsertString(int nIndex, LPCTSTR lpszItem);
    int GetTopIndex() const;
    int SetTopIndex(int nIndex);
    int GetItemRect(int nIndex, LPRECT lpRect) const;
    int GetItemHeight(int nIndex) const;
    int SetItemHeight(int nIndex, UINT cyItemHeight);
};

class CComboBox : public CWnd
{
public:
    virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
    int GetCount() const;
    int GetCurSel() const;
    int SetCurSel(int nSelect);
    int AddString(LPCTSTR lpszString);
    DWORD_PTR GetItemData(int nIndex) const;
    int SetItemData(int nIndex, DWORD_PTR dwItemData);
    // The same per-item slot as GetItemData, typed as a pointer.
    void* GetItemDataPtr(int nIndex) const;
    int SetItemDataPtr(int nIndex, void* pData);
    void SetHorizontalExtent(UINT nExtent);
    UINT GetHorizontalExtent() const;
    int SetDroppedWidth(UINT nWidth);
    int GetDroppedWidth() const;
    void ResetContent();
    int GetLBText(int nIndex, LPTSTR lpszText) const;
    void GetLBText(int nIndex, CString& rString) const;
    int DeleteString(UINT nIndex);
    int SelectString(int nStartAfter, LPCTSTR lpszString) const;
};

class CButton : public CWnd
{
public:
    virtual BOOL Create(LPCTSTR lpszCaption, DWORD dwStyle, const RECT& rect,
                         CWnd* pParentWnd, UINT nID);
    HICON SetIcon(HICON hIcon);
    UINT GetState() const;
    void SetState(BOOL bHighlight);
    int GetCheck() const;
    void SetCheck(int nCheck);
    HBITMAP SetBitmap(HBITMAP hBitmap);
    HBITMAP GetBitmap() const;
};

// CMenu (header afxwin.h, deriva da CObject — NOT CWnd/CCmdTarget)
class CMenu : public CObject
{
public:
    // eMule tests a menu for validity directly (`if (menu) ...`) and hands
    // it to raw Win32 calls, both of which real MFC supports through the
    // handle member and its implicit conversion.
    HMENU m_hMenu = nullptr;
    operator HMENU() const { return m_hMenu; }

    BOOL AppendMenu(UINT nFlags, UINT_PTR nIDNewItem = 0, LPCTSTR lpszNewItem = nullptr);
    BOOL AppendMenu(UINT nFlags, UINT_PTR nIDNewItem, const CBitmap* pBmp);
    UINT EnableMenuItem(UINT nIDEnableItem, UINT nEnable);
    BOOL DestroyMenu();
    HMENU Detach();
    BOOL Attach(HMENU hMenu);
    BOOL CreateMenu();
    BOOL CreatePopupMenu();
    BOOL TrackPopupMenu(UINT nFlags, int x, int y, CWnd* pWnd, LPCRECT lpRect = nullptr);
    UINT CheckMenuItem(UINT nIDCheckItem, UINT nCheck);
    BOOL SetDefaultItem(UINT uItem, BOOL fByPos = FALSE);
    BOOL RemoveMenu(UINT nPosition, UINT nFlags);
    UINT GetMenuItemCount() const;
    BOOL InsertMenu(UINT nPosition, UINT nFlags, UINT_PTR nIDNewItem = 0, LPCTSTR lpszNewItem = nullptr);
    BOOL LoadMenu(LPCTSTR lpszResourceName);
    BOOL LoadMenu(UINT nIDResource);
    CMenu* GetSubMenu(int nPos) const;
    BOOL ModifyMenu(UINT nPosition, UINT nFlags, UINT_PTR nIDNewItem = 0, LPCTSTR lpszNewItem = nullptr);
    // Radio-style check marks (only one item in the range ticked, with a
    // bullet instead of a check), and the MENUITEMINFO-based insert.
    BOOL CheckMenuRadioItem(UINT nIDFirst, UINT nIDLast, UINT nIDItem, UINT nFlags);
    BOOL InsertMenuItem(UINT uItem, LPMENUITEMINFO lpMenuItemInfo, BOOL fByPos = FALSE);
    BOOL GetMenuItemInfo(UINT uItem, LPMENUITEMINFO lpMenuItemInfo, BOOL fByPos = FALSE) const;
    BOOL SetMenuItemInfo(UINT uItem, LPMENUITEMINFO lpMenuItemInfo, BOOL fByPos = FALSE);
    UINT GetMenuItemID(int nPos) const;
    UINT GetMenuState(UINT nID, UINT nFlags) const;
    BOOL GetMenuInfo(LPMENUINFO lpcmi) const;
    BOOL SetMenuInfo(LPCMENUINFO lpcmi);

    // Added during the FRONTEND/GDI blind-spot pass (see
    // ../../mfc_scan_srchybrid.md addendum): CTitledMenu (TitledMenu.h),
    // an owner-draw CMenu subclass, overrides MeasureItem and calls
    // "CMenu::MeasureItem(lpMIS)" (TitledMenu.cpp:132) for the default
    // behavior — a qualified super-call, invisible to the original
    // ".Method("/"->Method(" scan. DrawItem is also overridden there but
    // never super-called, so it is intentionally NOT added (same
    // "framework-invoked only, no super-call found" rule already applied
    // elsewhere in this document, e.g. CDialog::OnOK originally).
    virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
};

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
#ifdef _MSC_VER
#pragma warning(pop)
#endif

class CDocument : public CCmdTarget
{
public:
    // Added during the FRONTEND/GDI blind-spot pass (see
    // ../../mfc_scan_srchybrid.md addendum): EmuleDlg.h:321 overrides and
    // super-calls "CDocument::OnNewDocument()" — same pattern already
    // noted for CDocument in the BACKEND group ("probably an
    // architectural container without real application logic").
    virtual BOOL OnNewDocument();
};

// CControlBar and CDialogBar are NOT declared here: they really belong to
// the afxext.h header (per Microsoft Learn), see afxext.h.
// CPropertyPage and CPropertySheet are NOT declared here: they really
// belong to the afxdlgs.h header (per Microsoft Learn), see afxdlgs.h.
// CImageList/CTreeCtrl/CListCtrl/CRichEditCtrl/CTabCtrl/CToolBarCtrl/
// CStatusBarCtrl/CToolTipCtrl are NOT declared here: they belong to
// afxcmn.h. COleDropTarget belongs to afxole.h. CDHtmlDialog belongs to
// afxdhtml.h.

// ---------------------------------------------------------------------
// Message-map demarcation macros (per Microsoft Learn: header afxwin.h).
// The "entry" macros (ON_COMMAND, ON_MESSAGE, ON_CONTROL, ON_NOTIFY, ...)
// are NOT declared here: they really belong to the afxmsg_.h header,
// which afxwin.h includes below (as in real MFC: a single
// #include "afxwin.h" also exposes the ON_* macros).
// DECLARE_MESSAGE_MAP is defined much earlier in this header, right after
// the AFX_MSGMAP structures, because CCmdTarget itself uses it.
//
// BEGIN/END_MESSAGE_MAP open and close the entry array. They are
// reproduced from real MFC rather than stubbed because eMule writes
// entries by hand and they have to land inside that array -- see the
// long note next to AFX_MSGMAP above. The two typedefs are not
// incidental: ThisClass is what makes an unqualified handler name in a
// hand-written entry resolve (the array lives inside a member function
// of the class), and TheBaseClass is what END_MESSAGE_MAP links the
// base map through.
#define BEGIN_MESSAGE_MAP(theClass, baseClass)                             \
    const AFX_MSGMAP* theClass::GetMessageMap() const                      \
        { return GetThisMessageMap(); }                                    \
    const AFX_MSGMAP* PASCAL theClass::GetThisMessageMap()                 \
    {                                                                      \
        typedef theClass ThisClass;                                        \
        typedef baseClass TheBaseClass;                                    \
        static const AFX_MSGMAP_ENTRY _messageEntries[] =                  \
        {

// The template form, for a message map on a class template. eMule needs
// it for CDialogMinTrayBtn<BASE> (DialogMinTrayBtn.cpp:98,
// "BEGIN_TEMPLATE_MESSAGE_MAP(CDialogMinTrayBtn, BASE, BASE)"): the
// definitions have to be templates themselves, and ThisClass has to name
// the specialization, not the bare template.
#define BEGIN_TEMPLATE_MESSAGE_MAP(theClass, type_name, baseClass)         \
    template <typename type_name>                                          \
    const AFX_MSGMAP* theClass<type_name>::GetMessageMap() const           \
        { return GetThisMessageMap(); }                                    \
    template <typename type_name>                                          \
    const AFX_MSGMAP* PASCAL theClass<type_name>::GetThisMessageMap()      \
    {                                                                      \
        typedef theClass<type_name> ThisClass;                             \
        typedef baseClass TheBaseClass;                                    \
        static const AFX_MSGMAP_ENTRY _messageEntries[] =                  \
        {

// Closes both forms. The trailing element is the terminator real MFC's
// dispatcher scans for; it is also what keeps the array non-empty when
// every ON_* entry in between expanded to nothing.
#define END_MESSAGE_MAP()                                                  \
            { 0, 0, 0, 0, AfxSig_end, (AFX_PMSG)0 }                        \
        };                                                                 \
        static const AFX_MSGMAP messageMap =                               \
            { &TheBaseClass::GetThisMessageMap, &_messageEntries[0] };     \
        return &messageMap;                                                \
    }

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
// Returns HINSTANCE in real MFC. As void* the result could not be passed
// to any Win32 call expecting a module handle (LoadAccelerators, ...)
// without an explicit cast eMule does not write.
HINSTANCE AfxGetInstanceHandle();
HINSTANCE AfxGetResourceHandle();
void AfxSetResourceHandle(HINSTANCE hInstResource);
CWnd* AfxGetMainWnd();
LPCTSTR AfxGetAppName();
int AfxMessageBox(LPCTSTR lpszText, UINT nType = 0, UINT nIDHelp = 0);
// The resource-id form, which real MFC declares alongside the string one.
int AfxMessageBox(UINT nIDPrompt, UINT nType = 0, UINT nIDHelp = (UINT)-1);
// Real MFC provides BOTH char-widths (afx.h): eMule's kademlia/Tag.h passes a
// narrow LPCSTR, other call sites the wide LPCTSTR, so declare both overloads.
BOOL AfxIsValidString(LPCTSTR lpsz, int nLength = -1);
BOOL AfxIsValidString(const char* lpsz, int nLength = -1);
BOOL AfxIsValidAddress(const void* lp, UINT nBytes, BOOL bReadWrite = TRUE);
// Registers a window class on the fly, for windows that need a cursor or
// background brush of their own (eMule's CColourPopup, CDownloadQueue).
LPCTSTR AFXAPI AfxRegisterWndClass(UINT nClassStyle, HCURSOR hCursor = nullptr,
                                    HBRUSH hbrBackground = nullptr, HICON hIcon = nullptr);
// The module a resource id should be looked up in: real MFC searches the
// app, then any loaded language DLL, which is exactly why eMule (which
// ships localisation DLLs) calls this instead of AfxGetResourceHandle.
HINSTANCE AFXAPI AfxFindResourceHandle(LPCTSTR lpszName, LPCTSTR lpszType);
BOOL AFXAPI AfxHtmlHelp(HWND hWnd, LPCTSTR szHelpFilePath, UINT nCmd, DWORD_PTR dwData);
// The window procedure MFC installs on every window it owns; eMule
// compares against it to tell its own subclassed windows apart.
LRESULT CALLBACK AfxWndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam);

// The four special CWnd values SetWindowPos takes as pWndInsertAfter.
// Real MFC declares them as const CWnd objects, not as HWND constants,
// so that `&wndTop` has the parameter's type.
extern const CWnd wndTop;
extern const CWnd wndBottom;
extern const CWnd wndTopMost;
extern const CWnd wndNoTopMost;

// WM_INITIALUPDATE is MFC's own private message (not a Win32 one), sent
// to a view once its frame is fully created.
#ifndef WM_INITIALUPDATE
#define WM_INITIALUPDATE (WM_USER + 0x0364)
#endif
// Base of the id range MFC maps to status-bar prompt strings.
#ifndef HID_BASE_PROMPT
#define HID_BASE_PROMPT 0x00030000UL
#endif

// ---------------------------------------------------------------------
// Nested-interface support for CCmdTarget (real MFC puts these macros in
// afxwin.h too). A class exposes each COM interface it implements as a
// nested XName object whose this-pointer is a known offset from the
// outer object's; METHOD_PROLOGUE recovers the outer object from it.
// eMule uses them for its IDataObject / IServiceProvider /
// IInternetSecurityManager / IDocHostUIHandler implementations.
// ---------------------------------------------------------------------
#ifndef AFX_MANAGE_STATE
#define AFX_MANAGE_STATE(p) ((void)0)
#endif
#define METHOD_PROLOGUE(theClass, localClass)                                  \
    theClass* pThis = ((theClass*)((BYTE*)this - offsetof(theClass, m_x##localClass)))

#define METHOD_PROLOGUE_(theClass, localClass)                                 \
    theClass* pThis = ((theClass*)((BYTE*)this - offsetof(theClass, m_x##localClass)));

#define BEGIN_INTERFACE_PART(localClass, baseClass)                            \
    class X##localClass : public baseClass                                     \
    {                                                                          \
    public:                                                                    \
        STDMETHOD_(ULONG, AddRef)();                                           \
        STDMETHOD_(ULONG, Release)();                                          \
        STDMETHOD(QueryInterface)(REFIID iid, LPVOID* ppvObj);

#define END_INTERFACE_PART(localClass)                                         \
    } m_x##localClass;                                                         \
    friend class X##localClass;

#define BEGIN_INTERFACE_MAP(theClass, theBase)
#define INTERFACE_PART(theClass, iid, localClass)
#define END_INTERFACE_MAP()
#define DECLARE_INTERFACE_MAP()

// ---------------------------------------------------------------------
// CWaitCursor — shows the hourglass for as long as the object is alive;
// eMule declares one at the top of its slow operations and lets scope
// exit restore the cursor. No base class in real MFC either.
// ---------------------------------------------------------------------
class CWaitCursor
{
public:
    CWaitCursor();
    ~CWaitCursor();
    void Restore();
};

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

    // Called by every DDX_ routine to resolve a control id to its HWND;
    // eMule's own DDX helpers (CColorButton) call it directly.
    HWND PrepareCtrl(int nIDC);
    HWND PrepareEditCtrl(int nIDC);
    void Fail();
};

// Real MFC's afxwin.h ends by pulling in the DDX_*/DDV_* routines, which
// is why applications only ever include afxwin.h. Placed last because
// afxdd_.h takes CDataExchange (declared just above) by pointer.
#include "afxdd_.h"
// Real MFC exposes the standard resource symbols through the same chain.
#include "afxres.h"
