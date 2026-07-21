// afxdhtml.h — reference STUB (declarations only, no implementation).
// CDHtmlDialog (a dialog hosting a DHTML/IE control). The original scan
// found zero *instance* calls (".Method("/"->Method(") on it, but the
// FRONTEND/GDI blind-spot pass (see ../../mfc_scan_srchybrid.md
// addendum) found genuine qualified super-calls in MiniMule.h/.cpp
// (CMiniMule : public CDHtmlDialog): OnBeforeNavigate/OnNavigateComplete/
// OnDocumentComplete are CDHtmlDialog-specific virtual event handlers,
// overridden there and super-called for the default (no-op) behavior,
// e.g. "CDHtmlDialog::OnDocumentComplete(pDisp, pszUrl);". The rest of
// the super-calls found (OnClose/OnDestroy/PostNcDestroy/OnTimer/
// OnNcLButtonDblClk/DoDataExchange/OnInitDialog) resolve via inherited
// CWnd/CDialog declarations (see afxwin.h), no CDHtmlDialog-specific
// declaration needed for those.
//
// The compile check against eMule/srchybrid later showed the class is
// used far more deeply than those three handlers: CMiniMule constructs it
// from an (IDD, IDH) pair, drives the DOM through it, and overrides
// CreateControlSite to install its own browser site (IESecurity.h).
// As in real MFC, it includes afxwin.h plus the control-container header.
#pragma once
#include "afxwin.h"
#include "afxocc.h"
#include "atlbase.h" // CComPtr, which m_spHtmlDoc below is; real MFC's
                     // afxdhtml.h pulls ATL in for exactly this reason.

// The browser host interfaces (IDocHostUIHandler, DOCHOSTUIFLAG_*) and
// the DOM (IHTMLDocument2, IHTMLElement). All Windows SDK headers, not
// MFC ones -- real MFC's afxdhtml.h pulls them in the same way, which is
// why eMule names those types without including anything itself.
#ifdef _WIN32
#include <mshtml.h>   // the DOM interfaces
#include <mshtmhst.h> // IDocHostUIHandler and its DOCHOSTUIFLAG_* flags
#include <exdisp.h>   // IWebBrowser2, the control being hosted
#include <urlmon.h>   // IInternetSecurityManager, URLZONE
#else
struct IDispatch; // real OLE Automation type; only used here as a pointer
using LPDISPATCH = IDispatch*;
#endif

// ---------------------------------------------------------------------
// The DHTML event map. Real MFC generates a static table mapping an
// element id + DOM event to a member function; the handlers themselves
// are ordinary member declarations, which is all a compile check needs,
// so these expand to nothing -- same treatment as the message-map macros
// in afxmsg_.h, see the rationale there.
// ---------------------------------------------------------------------
#define DECLARE_DHTML_EVENT_MAP()
#define BEGIN_DHTML_EVENT_MAP(className)
#define BEGIN_DHTML_EVENT_MAP_INLINE(className)
#define END_DHTML_EVENT_MAP()
#define DHTML_EVENT_ONCLICK(elemName, memberFxn)
#define DHTML_EVENT_ONDBLCLICK(elemName, memberFxn)
#define DHTML_EVENT_ONMOUSEOVER(elemName, memberFxn)
#define DHTML_EVENT_ONMOUSEOUT(elemName, memberFxn)
#define DHTML_EVENT_ONKEYDOWN(elemName, memberFxn)
#define DHTML_EVENT_ONKEYPRESS(elemName, memberFxn)
#define DHTML_EVENT_ONKEYUP(elemName, memberFxn)
#define DHTML_EVENT(dispid, elemName, memberFxn)

// ---------------------------------------------------------------------
// CDHtmlDialog (header afxdhtml.h, hierarchy
// CObject -> CCmdTarget -> CWnd -> CDialog -> CDHtmlDialog). Real MFC
// also has it implement IDocHostUIHandler, but that interface is
// deliberately NOT a base here: its methods are pure virtual and this
// header declares no bodies, so inheriting it would leave every derived
// eMule dialog abstract and impossible to instantiate. The derived
// class's own STDMETHOD overrides are plain member declarations and need
// nothing from the interface.
// ---------------------------------------------------------------------
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4266)
#endif

class CDHtmlDialog : public CDialog
{
public:
    CDHtmlDialog();
    // The pairing eMule uses: a dialog template for the frame, plus an
    // HTML resource for its contents.
    explicit CDHtmlDialog(UINT nIDTemplate, UINT nHtmlResID = 0, CWnd* pParentWnd = nullptr);
    explicit CDHtmlDialog(LPCTSTR lpszTemplateName, LPCTSTR szHtmlResID = nullptr, CWnd* pParentWnd = nullptr);
    virtual ~CDHtmlDialog();

    // Which HTML resource backs the dialog, in the two forms the
    // constructors above accept, and where the browser currently is.
    UINT m_nHtmlResID;
    LPCTSTR m_szHtmlResID;
    CString m_strCurrentUrl;
    // The DOCHOSTUIFLAG_* bits returned from GetHostInfo; eMule ORs in
    // DOCHOSTUIFLAG_DIALOG and DISABLE_HELP_MENU to strip browser chrome.
    DWORD m_dwHostFlags;
    // ...and puts them back through this setter rather than assigning the
    // member (MiniMule.cpp:114) -- real MFC's version also refreshes the
    // host, which a plain assignment would not.
    void SetHostFlags(DWORD dwFlags);

    void Navigate(LPCTSTR lpszURL, DWORD dwFlags = 0, LPCTSTR lpszTargetFrameName = nullptr,
                  LPCTSTR lpszHeaders = nullptr, LPVOID lpvPostData = nullptr, DWORD dwPostDataLen = 0);
    void LoadFromResource(LPCTSTR lpszResource);
    void LoadFromResource(UINT nRes);

#ifdef _WIN32
    // DOM access: fetch an element by id, then read or replace its HTML.
    HRESULT GetElement(LPCTSTR szElementId, IHTMLElement** pphtmlElement, BOOL bMustFind = TRUE);
    HRESULT GetElementInterface(LPCTSTR szElementId, REFIID riid, void** ppvObj);
    HRESULT SetElementHtml(LPCTSTR szElementId, BSTR bstrText);
    HRESULT GetElementHtml(LPCTSTR szElementId, BSTR* pbstrText);
    HRESULT SetElementProperty(LPCTSTR szElementId, DISPID dispid, VARIANT* pVar);
    IHTMLDocument2* GetHtmlDocument() const;
    // The document the control currently hosts, held for the dialog's
    // lifetime; eMule reads it directly.
    CComPtr<IHTMLDocument2> m_spHtmlDoc;
    // The template form, which deduces the interface from the pointer it
    // is given ("GetElementInterface(_T(\"openIncomingLink\"), &a)").
    template <class Q>
    HRESULT GetElementInterface(LPCTSTR szElementId, Q** ppElement);
#endif

    // Called once per hosted control so a derived dialog can substitute
    // its own site (eMule does, to install a security manager).
    virtual BOOL CreateControlSite(COleControlContainer* pContainer, COleControlSite** ppSite,
                                    UINT nID, REFCLSID clsid);

    virtual void OnBeforeNavigate(LPDISPATCH pDisp, LPCTSTR pszUrl);
    virtual void OnNavigateComplete(LPDISPATCH pDisp, LPCTSTR pszUrl);
    virtual void OnDocumentComplete(LPDISPATCH pDisp, LPCTSTR pszUrl);
};

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
#ifdef _MSC_VER
#pragma warning(pop)
#endif
