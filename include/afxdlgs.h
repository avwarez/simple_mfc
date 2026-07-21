// afxdlgs.h — reference STUB (declarations only, no implementation).
// Standard MFC dialogs (property page/sheet and other predefined
// dialogs). As in real MFC, it includes afxwin.h.
#pragma once
#include "afxwin.h"
#include "afxcoll.h" // CPtrArray, for CPropertySheet::m_pages

class CTabCtrl; // real header afxcmn.h, only used here as a pointer return type

// ---------------------------------------------------------------------
// CPropertyPage — a page of a property sheet
// (header afxdlgs.h, hierarchy CObject -> CCmdTarget -> CWnd -> CDialog -> CPropertyPage).
// ---------------------------------------------------------------------
class CPropertyPage : public CDialog
{
public:
    // eMule's preference pages all pass just their IDD; the caption/header
    // overloads are declared because they are the same constructor set real
    // MFC exposes, and dwSize defaults keep a plain `CPropertyPage(IDD)` call
    // unambiguous.
    CPropertyPage();
    explicit CPropertyPage(UINT nIDTemplate, UINT nIDCaption = 0);
    explicit CPropertyPage(LPCTSTR lpszTemplateName, UINT nIDCaption = 0);

    // The raw Win32 page descriptor. eMule sets the tab label through it
    // (`m_psp.pszTitle = m_strCaption;`), which is also why m_strCaption
    // has to outlive the assignment -- it owns the string m_psp points at.
#ifndef _WIN32
    struct PROPSHEETPAGE
    {
        LPCTSTR pszTitle;
    };
#endif
    PROPSHEETPAGE m_psp;

    void SetModified(BOOL bChanged = TRUE);
    virtual BOOL OnSetActive();
    // Added during the FRONTEND/GDI blind-spot pass (see
    // ../../mfc_scan_srchybrid.md addendum): OnApply (15 qualified
    // super-calls, e.g. "CPropertyPage::OnApply()") and OnKillActive (3)
    // are genuinely reached this way across eMule/srchybrid's property
    // pages, invisible to the original ".Method("/"->Method(" scan.
    virtual BOOL OnApply();
    virtual BOOL OnKillActive();

protected:
    // Protected in real MFC too -- only the derived page sets its own
    // caption, which eMule does in every OnInitDialog.
    CString m_strCaption;
    CString m_strHeaderTitle;
    CString m_strHeaderSubTitle;
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

// ---------------------------------------------------------------------
// CPropertyPageEx — the Wizard97-style page (header/subtitle band).
// eMule's CDlgPageWizard derives from it.
// ---------------------------------------------------------------------
class CPropertyPageEx : public CPropertyPage
{
public:
    CPropertyPageEx();
    explicit CPropertyPageEx(UINT nIDTemplate, UINT nIDCaption = 0,
                              UINT nIDHeaderTitle = 0, UINT nIDHeaderSubTitle = 0);
    explicit CPropertyPageEx(LPCTSTR lpszTemplateName, UINT nIDCaption = 0,
                              UINT nIDHeaderTitle = 0, UINT nIDHeaderSubTitle = 0);
};

class CPropertySheet : public CWnd
{
public:
    // eMule's sheets forward their own (caption, parent, page) constructor
    // arguments straight to the base, in both the resource-id and the string
    // form.
    CPropertySheet();
    explicit CPropertySheet(UINT nIDCaption, CWnd* pParentWnd = nullptr, UINT iSelectPage = 0);
    explicit CPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd = nullptr, UINT iSelectPage = 0);

    // The pages added so far. Public in real MFC, and eMule's
    // CListViewWalkerPropertySheet hands it straight out
    // (`CPtrArray& GetPages() { return m_pages; }`).
    CPtrArray m_pages;

    // The raw Win32 sheet descriptor, which eMule pokes directly to turn a
    // sheet into a wizard (`sheet.m_psh.dwFlags |= PSH_WIZARD`). Real
    // PROPSHEETHEADER comes from <prsht.h> via the <commctrl.h> afxwin.h
    // already includes; off Windows only the flags field is ever touched.
#ifndef _WIN32
    struct PROPSHEETHEADER
    {
        DWORD dwFlags;
    };
#endif
    PROPSHEETHEADER m_psh;

    virtual INT_PTR DoModal();
    virtual BOOL Create(CWnd* pParentWnd = nullptr, DWORD dwStyle = (DWORD)-1, DWORD dwExStyle = 0);
    void AddPage(CPropertyPage* pPage);
    void SetTitle(LPCTSTR lpszText, UINT nStyle = 0);
    int GetPageIndex(CPropertyPage* pPage);
    CPropertyPage* GetPage(int nPage) const;
    int GetPageCount() const;
    int GetActiveIndex() const;
    // Rebuilds the raw PROPSHEETPAGE array from m_pages before the sheet
    // is created; eMule's own sheet calls it after reordering its pages.
    BOOL BuildPropPageArray();
    // Wizard97 sheets: lets the tab row wrap onto several lines.
    void EnableStackedTabs(BOOL bStacked);
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

// ---------------------------------------------------------------------
// CFileDialog — the common Open/Save-As file dialog (header afxdlgs.h,
// hierarchy CObject -> CCmdTarget -> CWnd -> CDialog -> CCommonDialog ->
// CFileDialog). eMule constructs it directly (6-arg ctor), calls DoModal
// (inherited) + GetPathName, and reads m_ofn.lpstrTitle. m_ofn is the real
// OPENFILENAME (from <commdlg.h>, pulled in by <windows.h> now that
// WIN32_LEAN_AND_MEAN is gone); a minimal portable stand-in otherwise.
// ---------------------------------------------------------------------
#ifndef _WIN32
struct OPENFILENAME { LPCTSTR lpstrTitle; };
#endif
class CFileDialog : public CDialog
{
public:
    OPENFILENAME m_ofn;
    explicit CFileDialog(BOOL bOpenFileDialog, LPCTSTR lpszDefExt = nullptr,
                         LPCTSTR lpszFileName = nullptr, DWORD dwFlags = 0,
                         LPCTSTR lpszFilter = nullptr, CWnd* pParentWnd = nullptr,
                         DWORD dwSize = 0, BOOL bVistaStyle = TRUE);
    CString GetPathName() const;
    CString GetFileName() const;
    CString GetFileExt() const;
    CString GetFileTitle() const;
    POSITION GetStartPosition() const;
    CString GetNextPathName(POSITION& pos) const;
};

// ---------------------------------------------------------------------
// CFontDialog — the common Choose-Font dialog (header afxdlgs.h, same
// CCommonDialog branch as CFileDialog above). eMule opens it from the
// display preferences page and reads the chosen LOGFONT back out.
// ---------------------------------------------------------------------
class CFontDialog : public CDialog
{
public:
    explicit CFontDialog(LOGFONT* lplfInitial = nullptr, DWORD dwFlags = 0,
                         CDC* pdcPrinter = nullptr, CWnd* pParentWnd = nullptr);
    void GetCurrentFont(LOGFONT* lplf);
    CString GetFaceName() const;
    int GetSize() const;
    COLORREF GetColor() const;
    BOOL IsBold() const;
    BOOL IsItalic() const;
    BOOL IsUnderline() const;
    BOOL IsStrikeOut() const;
};
