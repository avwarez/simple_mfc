// afxcmn.h — reference STUB (declarations only, no implementation).
// Wrappers for the Windows common controls (comctl32): tree/list/tab/
// toolbar/status/tooltip/rich-edit controls, plus CImageList (a common-
// control wrapper, not "core" GDI, per Microsoft Learn — hence it lives
// here and not in afxwin.h). As in real MFC, it includes afxwin.h.
// Subset actually used by eMule/srchybrid, signatures verified against
// Microsoft Learn (see ../../mfc_scan_srchybrid.md).
#pragma once
#include "afxwin.h"

class CArchive; // real header afxx.h family; not implemented (see afx.h), only used here as a pointer

// ---------------------------------------------------------------------
// Win32 primitive type stand-ins needed by the signatures below. Most
// are only ever used by pointer/reference in this declaration-only
// header, so an incomplete forward declaration is enough — no member
// access happens here.
// ---------------------------------------------------------------------
using HIMAGELIST = void*;
using HTREEITEM = void*;
// LPMSG comes from afxwin.h (included above): a real MSG/LPMSG pair was
// added there during the FRONTEND/GDI blind-spot pass (needed by
// CWnd::PreTranslateMessage), so it is no longer aliased to void* here.
using POSITION = void*; // same alias as afxcoll.h's (identical redefinition is legal, avoids depending on it)
#define LPSTR_TEXTCALLBACK ((LPTSTR)-1)

struct LVITEM;
struct LVCOLUMN;
struct LVFINDINFO;
struct LVHITTESTINFO;
using PFNLVCOMPARE = int (*)(LPARAM, LPARAM, LPARAM);
constexpr int LVCFMT_LEFT = 0;

struct TVINSERTSTRUCT;
using LPTVINSERTSTRUCT = TVINSERTSTRUCT*;
struct TVHITTESTINFO;

struct TCHITTESTINFO;
struct TCITEM;
using TC_ITEM = TCITEM; // older-SDK alias, used interchangeably with TCITEM in eMule/srchybrid

struct TBBUTTON;
using LPTBBUTTON = TBBUTTON*;
struct TBBUTTONINFO;

struct CHARFORMAT;
struct CHARFORMAT2;
struct CHARRANGE;

// ---------------------------------------------------------------------
// CImageList (header afxcmn.h, deriva da CObject — NOT CWnd)
// ---------------------------------------------------------------------
class CImageList : public CObject
{
public:
    BOOL DeleteImageList();
    int Add(CBitmap* pbmImage, CBitmap* pbmMask);
    int Add(CBitmap* pbmImage, COLORREF crMask);
    int Add(HICON hIcon);
    BOOL Create(int cx, int cy, UINT nFlags, int nInitial, int nGrow);
    BOOL Create(UINT nBitmapID, int cx, int nGrow, COLORREF crMask);
    BOOL Create(LPCTSTR lpszBitmapID, int cx, int nGrow, COLORREF crMask);
    BOOL Create(CImageList& imagelist1, int nImage1, CImageList& imagelist2, int nImage2, int dx, int dy);
    BOOL Create(CImageList* pImageList);
    BOOL Replace(int nImage, CBitmap* pbmImage, CBitmap* pbmMask);
    int Replace(int nImage, HICON hIcon);
    BOOL Draw(CDC* pDC, int nImage, POINT pt, UINT nStyle);
    BOOL SetOverlayImage(int nImage, int nOverlay);
    int GetImageCount() const;
    BOOL Remove(int nImage);
    BOOL Read(CArchive* pArchive);
    BOOL Write(CArchive* pArchive);
    COLORREF SetBkColor(COLORREF cr);
    COLORREF GetBkColor() const;
    HICON ExtractIcon(int nImage);
    HIMAGELIST GetSafeHandle() const;
    BOOL Attach(HIMAGELIST hImageList);
    HIMAGELIST Detach();

    // Added during the FRONTEND/GDI blind-spot pass (see
    // ../../mfc_scan_srchybrid.md addendum): these are all *static*
    // methods, only ever called as "CImageList::Method(...)"
    // (SharedDirsTreeCtrl.cpp:1083-1140) — structurally invisible to the
    // original ".Method("/"->Method(" scan, which only sees instance
    // calls. BeginDrag/DragEnter are NOT added: no call site of any kind
    // (qualified or not) was found for either.
    static BOOL DragShowNolock(BOOL bShow);
    static BOOL DragMove(CPoint pt);
    static BOOL DragLeave(CWnd* pWndLock);
    static void EndDrag();
};

// Real MFC's CWnd-derived control classes below intentionally hide
// CWnd::Create with their own Create() overload(s) — same pattern/
// suppression already used in afxwin.h/afxext.h/afxdlgs.h.
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4266)
#endif

// ---------------------------------------------------------------------
// CTreeCtrl (header afxcmn.h, deriva da CWnd)
// ---------------------------------------------------------------------
// TVI_ROOT/TVI_LAST: real Win32 sentinel HTREEITEM values (commctrl.h),
// used as InsertItem's default hParent/hInsertAfter below.
#define TVI_ROOT ((HTREEITEM)0xFFFF0000)
#define TVI_LAST ((HTREEITEM)0xFFFF0002)

class CTreeCtrl : public CWnd
{
public:
    HTREEITEM InsertItem(LPTVINSERTSTRUCT lpInsertStruct);
    HTREEITEM InsertItem(UINT nMask, LPCTSTR lpszItem, int nImage, int nSelectedImage,
                          UINT nState, UINT nStateMask, LPARAM lParam,
                          HTREEITEM hParent, HTREEITEM hInsertAfter);
    // Independently re-verified against Microsoft Learn (2026-07-20): real
    // CTreeCtrl::InsertItem has exactly 4 overloads (the original scan's
    // "5 overload totali" was inaccurate). These 2 were missing despite
    // genuine real usage (StatisticsDlg.cpp's CStatisticsTree : public
    // CTreeCtrl, PPgDebug.cpp's CTreeOptionsCtrl : public CTreeCtrl).
    HTREEITEM InsertItem(LPCTSTR lpszItem, HTREEITEM hParent = TVI_ROOT, HTREEITEM hInsertAfter = TVI_LAST);
    HTREEITEM InsertItem(LPCTSTR lpszItem, int nImage, int nSelectedImage,
                          HTREEITEM hParent = TVI_ROOT, HTREEITEM hInsertAfter = TVI_LAST);
    BOOL SetItemText(HTREEITEM hItem, LPCTSTR lpszItem);
    BOOL DeleteAllItems();
    BOOL DeleteItem(HTREEITEM hItem);
    BOOL Expand(HTREEITEM hItem, UINT nCode);
    DWORD_PTR GetItemData(HTREEITEM hItem) const;
    BOOL SetItemData(HTREEITEM hItem, DWORD_PTR dwData);
    CString GetItemText(HTREEITEM hItem) const;
    UINT GetCount() const;
    HTREEITEM GetNextItem(HTREEITEM hItem, UINT nCode) const;
    HTREEITEM GetSelectedItem() const;
    BOOL SelectItem(HTREEITEM hItem);
    BOOL ItemHasChildren(HTREEITEM hItem) const;
    HTREEITEM HitTest(CPoint pt, UINT* pFlags = nullptr) const;
    HTREEITEM HitTest(TVHITTESTINFO* pHitTestInfo) const;
    HTREEITEM GetChildItem(HTREEITEM hItem) const;
    HTREEITEM GetNextSiblingItem(HTREEITEM hItem) const;
    HTREEITEM GetParentItem(HTREEITEM hItem) const;
    HTREEITEM GetRootItem() const;
    BOOL SetItemImage(HTREEITEM hItem, int nImage, int nSelectedImage);
    BOOL GetItemImage(HTREEITEM hItem, int& nImage, int& nSelectedImage) const;
    HTREEITEM GetPrevSiblingItem(HTREEITEM hItem) const;
    CImageList* GetImageList(UINT nImageList) const;
    CImageList* SetImageList(CImageList* pImageList, int nImageListType);
};

// ---------------------------------------------------------------------
// CListCtrl (header afxcmn.h, deriva da CWnd)
// ---------------------------------------------------------------------
class CListCtrl : public CWnd
{
public:
    int InsertItem(const LVITEM* pItem);
    int InsertItem(int nItem, LPCTSTR lpszItem);
    int InsertItem(int nItem, LPCTSTR lpszItem, int nImage);
    int InsertItem(UINT nMask, int nItem, LPCTSTR lpszItem, UINT nState, UINT nStateMask, int nImage, LPARAM lParam);
    BOOL SetItemText(int nItem, int nSubItem, LPCTSTR lpszText);
    int GetItemCount() const;
    DWORD_PTR GetItemData(int nItem) const;
    BOOL SetItemState(int nItem, UINT nState, UINT nMask);
    BOOL SetItemData(int nItem, DWORD_PTR dwData);
    BOOL DeleteAllItems();
    BOOL SetItem(LVITEM* pItem);
    BOOL GetItem(LVITEM* pItem) const;
    CString GetItemText(int nItem, int nSubItem) const;
    int GetItemText(int nItem, int nSubItem, LPTSTR lpszText, int nLen) const;
    CImageList* SetImageList(CImageList* pImageList, int nImageListType);
    int GetNextSelectedItem(POSITION& pos) const;
    int InsertColumn(int nCol, const LVCOLUMN* pColumn);
    int InsertColumn(int nCol, LPCTSTR lpszColumnHeading, int nFormat = LVCFMT_LEFT, int nWidth = -1, int nSubItem = -1);
    BOOL DeleteItem(int nItem);
    DWORD SetExtendedStyle(DWORD dwNewStyle);
    int GetColumnWidth(int nCol) const;
    BOOL GetItemRect(int nItem, LPRECT lpRect, UINT nCode) const;
    BOOL SetColumnWidth(int nCol, int cx);
    int GetNextItem(int nItem, int nFlags) const;
    DWORD GetExtendedStyle();
    CImageList* GetImageList(int nImageList) const;
    BOOL Update(int nItem);
    int FindItem(LVFINDINFO* pFindInfo, int nStart = -1) const;
    UINT GetSelectedCount() const;
    int HitTest(LVHITTESTINFO* pHitTestInfo) const;
    int HitTest(CPoint pt, UINT* pFlags = nullptr) const;
    BOOL EnsureVisible(int nItem, BOOL bPartialOK);
    UINT GetItemState(int nItem, UINT nMask) const;
    BOOL SortItems(PFNLVCOMPARE pfnCompare, DWORD_PTR dwData);
    BOOL DeleteColumn(int nCol);
    // Added during the FRONTEND/GDI blind-spot pass (see
    // ../../mfc_scan_srchybrid.md addendum): MuleListCtrl.h:81 calls
    // "CListCtrl::GetColumn(iColumn, &lvcol)" — a qualified super-call.
    BOOL GetColumn(int nCol, LVCOLUMN* pColumn) const;
};

// ---------------------------------------------------------------------
// CRichEditCtrl (header afxcmn.h — NOT afxwin.h, correction found during
// the scan; deriva da CWnd)
// ---------------------------------------------------------------------
class CRichEditCtrl : public CWnd
{
public:
    BOOL SetSelectionCharFormat(CHARFORMAT& cf);
    BOOL SetSelectionCharFormat(CHARFORMAT2& cf);
    void SetSel(long nStartChar, long nEndChar);
    void SetSel(CHARRANGE& cr);
    DWORD SetEventMask(DWORD dwEventMask);
    COLORREF SetBackgroundColor(BOOL bSysColor, COLORREF cr);
    void LimitText(long nChars = 0);
    DWORD GetSelectionCharFormat(CHARFORMAT& cf) const;
    DWORD GetSelectionCharFormat(CHARFORMAT2& cf) const;
    void ReplaceSel(LPCTSTR lpszNewText, BOOL bCanUndo = FALSE);
    void GetSel(CHARRANGE& cr) const;
    void GetSel(long& nStartChar, long& nEndChar) const;
};

// ---------------------------------------------------------------------
// CTabCtrl (header afxcmn.h, deriva da CWnd)
// ---------------------------------------------------------------------
class CTabCtrl : public CWnd
{
public:
    // Doc: "6 overload, il più semplice: ..." — only this simplest one
    // was captured with a full signature; the other 5 are not declared,
    // except the TCITEM* overload below (added: FRONTEND/GDI blind-spot
    // pass, see ../../mfc_scan_srchybrid.md addendum — TabCtrl.cpp:298
    // and IrcChannelTabCtrl.cpp:293 both call InsertItem with a
    // TCITEM*/TC_ITEM* argument, the fixed-signature struct overload,
    // not the simple-string one).
    LONG InsertItem(int nItem, TCITEM* pTabCtrlItem);
    LONG InsertItem(int nItem, LPCTSTR lpszItem);
    int GetCurSel() const;
    int SetCurSel(int nItem);
    int GetItemCount() const;
    BOOL DeleteAllItems();
    BOOL DeleteItem(int nItem);
    BOOL GetItemRect(int nItem, LPRECT lpRect) const;
    int HitTest(TCHITTESTINFO* pHitTestInfo) const;
    int GetCurFocus() const;
};

// ---------------------------------------------------------------------
// CToolBarCtrl (header afxcmn.h, deriva da CWnd)
// ---------------------------------------------------------------------
class CToolBarCtrl : public CWnd
{
public:
    virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
    BOOL CheckButton(int nID, BOOL bCheck = TRUE);
    BOOL SetButtonInfo(int nID, TBBUTTONINFO* ptbbi);
    BOOL GetItemRect(int nIndex, LPRECT lpRect) const;
    int GetButtonCount() const;
    UINT CommandToIndex(UINT nID) const;
    BOOL EnableButton(int nID, BOOL bEnable = TRUE);
    BOOL GetButton(int nIndex, LPTBBUTTON lpButton) const;
    // Added during the FRONTEND/GDI blind-spot pass (see
    // ../../mfc_scan_srchybrid.md addendum): both are called via
    // qualified super-call in a CToolBarCtrl-derived class.
    CSize GetMaxSize();
    BOOL AutoSize();
};

// ---------------------------------------------------------------------
// CStatusBarCtrl (header afxcmn.h, deriva da CWnd)
// ---------------------------------------------------------------------
class CStatusBarCtrl : public CWnd
{
public:
    virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
    BOOL SetText(LPCTSTR lpszText, int nPane, int nType);
    CString GetText(int nPane, int* pType = nullptr) const;
    int GetText(LPCTSTR lpszText, int nPane, int* pType = nullptr) const;
    BOOL SetParts(int nParts, int* pWidths);
};

// ---------------------------------------------------------------------
// CToolTipCtrl (header afxcmn.h, deriva da CWnd)
// ---------------------------------------------------------------------
class CToolTipCtrl : public CWnd
{
public:
    virtual BOOL Create(CWnd* pParentWnd, DWORD dwStyle = 0);
    BOOL AddTool(CWnd* pWnd, UINT nIDText, LPCRECT lpRectTool = nullptr, UINT_PTR nIDTool = 0);
    BOOL AddTool(CWnd* pWnd, LPCTSTR lpszText = LPSTR_TEXTCALLBACK, LPCRECT lpRectTool = nullptr, UINT_PTR nIDTool = 0);
    void UpdateTipText(LPCTSTR lpszText, CWnd* pWnd, UINT_PTR nIDTool = 0);
    void UpdateTipText(UINT nIDText, CWnd* pWnd, UINT_PTR nIDTool = 0);
    void Activate(BOOL bActivate);
    void RelayEvent(LPMSG lpMsg);
    void DelTool(CWnd* pWnd, UINT_PTR nIDTool = 0);
};

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
#ifdef _MSC_VER
#pragma warning(pop)
#endif
