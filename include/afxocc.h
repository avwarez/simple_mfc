// afxocc.h — reference STUB (declarations only, no implementation).
// MFC's ActiveX control container: the site objects a dialog creates for
// each hosted control. eMule reaches them only through CDHtmlDialog's
// CreateControlSite override (MiniMule.cpp), where it substitutes its own
// site to install a custom security manager (IESecurity.h).
// As in real MFC, it builds on afxwin.h.
#pragma once
#include "afxwin.h"

// COM basics. On Windows they come from <windows.h> via afxwin.h; off it
// they are named only so these declarations parse (same approach as
// afxole.h's data-transfer types).
#ifndef _WIN32
struct IUnknown;
using LPUNKNOWN = IUnknown*;
struct GUID;
using CLSID = GUID;
using REFCLSID = const CLSID&;
#endif

// ---------------------------------------------------------------------
// COleControlContainer / COleControlSite (header afxocc.h, both derive
// from CCmdTarget). The container owns one site per hosted control.
// ---------------------------------------------------------------------
class COleControlSite;

class COleControlContainer : public CCmdTarget
{
public:
    explicit COleControlContainer(CWnd* pWnd);
    CWnd* m_pWnd;
};

class COleControlSite : public CCmdTarget
{
public:
    explicit COleControlSite(COleControlContainer* pCtrlCont);
    COleControlContainer* m_pCtrlCont;
    LPUNKNOWN m_pInnerUnknown;
    UINT m_nID;
};

// ---------------------------------------------------------------------
// CBrowserControlSite — the site MFC uses for the web browser control a
// CDHtmlDialog hosts. eMule derives CMuleBrowserControlSite from it to
// answer the browser's security questions itself.
// ---------------------------------------------------------------------
class CDHtmlDialog;

class CBrowserControlSite : public COleControlSite
{
public:
    CBrowserControlSite(COleControlContainer* pCtrlCont, CDHtmlDialog* pHandler);
    CDHtmlDialog* m_pHandler;
};
