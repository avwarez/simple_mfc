// afxdlgs.h — reference STUB (declarations only, no implementation).
// Standard MFC dialogs (property page/sheet and other predefined
// dialogs). As in real MFC, it includes afxwin.h.
#pragma once
#include "afxwin.h"

class CTabCtrl; // real header afxcmn.h, only used here as a pointer return type

// ---------------------------------------------------------------------
// CPropertyPage — a page of a property sheet
// (header afxdlgs.h, hierarchy CObject -> CCmdTarget -> CWnd -> CDialog -> CPropertyPage).
// ---------------------------------------------------------------------
class CPropertyPage : public CDialog
{
public:
    void SetModified(BOOL bChanged = TRUE);
    virtual BOOL OnSetActive();
    // Added during the FRONTEND/GDI blind-spot pass (see
    // ../../mfc_scan_srchybrid.md addendum): OnApply (15 qualified
    // super-calls, e.g. "CPropertyPage::OnApply()") and OnKillActive (3)
    // are genuinely reached this way across eMule/srchybrid's property
    // pages, invisible to the original ".Method("/"->Method(" scan.
    virtual BOOL OnApply();
    virtual BOOL OnKillActive();
};

// ---------------------------------------------------------------------
// CPropertySheet — container for property pages. Derives directly from
// CWnd, NOT from CDialog (header afxdlgs.h, hierarchy
// CObject -> CCmdTarget -> CWnd -> CPropertySheet). Create/DoModal/
// EndDialog hide the CWnd/CDialog overload sets of the same name — same
// real-MFC pattern noted in afxwin.h, warning suppressed likewise.
// ---------------------------------------------------------------------
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4266)
#endif

class CPropertySheet : public CWnd
{
public:
    virtual INT_PTR DoModal();
    virtual BOOL Create(CWnd* pParentWnd = nullptr, DWORD dwStyle = (DWORD)-1, DWORD dwExStyle = 0);
    void AddPage(CPropertyPage* pPage);
    void SetTitle(LPCTSTR lpszText, UINT nStyle = 0);
    int GetPageIndex(CPropertyPage* pPage);
    CTabCtrl* GetTabControl() const;
    BOOL SetActivePage(int nPage);
    BOOL SetActivePage(CPropertyPage* pPage);
    CPropertyPage* GetActivePage() const;
    void RemovePage(CPropertyPage* pPage);
    void RemovePage(int nPage);
    void EndDialog(int nEndID);
    void SetWizardButtons(DWORD dwFlags);
    // Added during the FRONTEND/GDI blind-spot pass (see
    // ../../mfc_scan_srchybrid.md addendum): unlike CDialog, real MFC
    // declares OnInitDialog directly on CPropertySheet too (it does not
    // derive from CDialog), and MiniMule-style code super-calls it as
    // "CPropertySheet::OnInitDialog()".
    virtual BOOL OnInitDialog();
};

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
#ifdef _MSC_VER
#pragma warning(pop)
#endif
