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
class CHeaderCtrl;  // defined below; CListCtrl::GetHeaderCtrl returns it
class CToolTipCtrl; // defined below; CToolBarCtrl::GetToolTips returns it

// ---------------------------------------------------------------------
// Win32 primitive type stand-ins needed by the signatures below. Most
// are only ever used by pointer/reference in this declaration-only
// header, so an incomplete forward declaration is enough — no member
// access happens here.
// ---------------------------------------------------------------------
// HIMAGELIST/HTREEITEM are real DECLARE_HANDLE-based opaque handles in
// <commctrl.h> (an incompatible type from our void* alias), and
// PFNLVCOMPARE's real calling convention may differ from ours -- guarded
// like afxwin.h's HWND-family (see there): real <commctrl.h>, pulled in
// here on a real Windows/MSVC target, provides all three.
#ifdef _WIN32
#include <commctrl.h>
#else
using HIMAGELIST = void*;
using HTREEITEM = void*;
using PFNLVCOMPARE = int (*)(LPARAM, LPARAM, LPARAM);
#endif
// LPMSG comes from afxwin.h (included above): a real MSG/LPMSG pair was
// added there during the FRONTEND/GDI blind-spot pass (needed by
// CWnd::PreTranslateMessage), so it is no longer aliased to void* here.
using POSITION = void*; // same alias as afxcoll.h's (identical redefinition is legal, avoids depending on it)
// LPSTR_TEXTCALLBACK/LVCFMT_LEFT are real commctrl.h *macros*: #ifndef,
// not #ifdef _WIN32 (see afxwin.h's RDW_* for why the two aren't
// equivalent for a macro that's already active by this point).
#ifndef LPSTR_TEXTCALLBACK
#define LPSTR_TEXTCALLBACK ((LPTSTR)-1)
#endif

// LVITEM/TVINSERTSTRUCT/TCITEM/TBBUTTON and their LP*/TC_ITEM aliases:
// with UNICODE defined (forced by afx.h on _WIN32, see there), several of
// these are themselves real commctrl.h *macros* that textually expand to
// their ...W-suffixed name (e.g. LVITEM -> LVITEMW) or, like TBBUTTON,
// name a typedef to a differently-tagged real struct -- either way our
// own alias declaration collides with (or gets silently rewritten into a
// duplicate of) what the real, already-included <commctrl.h> (see above)
// completely defines (C2371), unlike the bare tagMSG-style forward-
// declares elsewhere in this file. On _WIN32 every use of these names
// elsewhere in this header resolves directly through the real macro/type
// from <commctrl.h>, so nothing needs to be (re)declared here at all.
#ifndef _WIN32
struct LVITEM;
struct LVCOLUMN;
struct LVFINDINFO;
struct LVHITTESTINFO;
struct TVINSERTSTRUCT;
using LPTVINSERTSTRUCT = TVINSERTSTRUCT*;
struct TVHITTESTINFO;
struct TCHITTESTINFO;
struct TCITEM;
using TC_ITEM = TCITEM; // older-SDK alias, used interchangeably with TCITEM in eMule/srchybrid
struct TBBUTTON;
using LPTBBUTTON = TBBUTTON*;
struct TBBUTTONINFO;
struct HDITEM;
struct COMBOBOXEXITEM;
#endif
#ifndef LVCFMT_LEFT
constexpr int LVCFMT_LEFT = 0;
#endif

// CHARFORMAT/CHARFORMAT2/CHARRANGE are <richedit.h> types. eMule declares
// them as by-value data members (e.g. CHTRichEditCtrl::m_cfDefault), which
// needs a COMPLETE type, not a forward declaration -> on a real Windows build
// pull in <richedit.h> (which, like <commctrl.h> above, resolves CHARFORMAT
// to its real ...W-suffixed struct under UNICODE). Only the portable build,
// where simple_mfc never instantiates them, keeps the incomplete stand-ins.
#ifdef _WIN32
#include <richedit.h>
#include <richole.h>  // IRichEditOle, which <richedit.h> does NOT declare --
                      // real MFC's afxcmn.h includes it here for the same
                      // reason: CRichEditCtrl::GetIRichEditOle returns it.
#else
struct CHARFORMAT;
struct CHARFORMAT2;
struct CHARRANGE;
struct EDITSTREAM;
struct PARAFORMAT;
struct PARAFORMAT2;
struct IMAGEINFO;
struct LVBKIMAGE;
struct TVITEM;
struct IRichEditOle;
#endif

// ---------------------------------------------------------------------
// CImageList (header afxcmn.h, deriva da CObject — NOT CWnd)
// ---------------------------------------------------------------------
class CImageList : public CObject
{
public:
    // The wrapped handle, public in real MFC; eMule tests it directly
    // (`piml == NULL || piml->m_hImageList == NULL`).
    HIMAGELIST m_hImageList = nullptr;

    // Real MFC converts implicitly to the raw handle and wraps one back
    // up; eMule passes a CImageList straight to APIs taking HIMAGELIST.
    operator HIMAGELIST() const { return m_hImageList; }
    static CImageList* FromHandle(HIMAGELIST hImageList);

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
    // calls. BeginDrag/DragEnter were originally left out on the strength
    // of that same scan; the compile check found real call sites for both,
    // so the drag API is complete here.
    BOOL BeginDrag(int nImage, CPoint ptHotSpot);
    static BOOL DragEnter(CWnd* pWndLock, CPoint point);
    BOOL GetImageInfo(int nImage, IMAGEINFO* pImageInfo) const;
    BOOL DrawEx(CDC* pDC, int nImage, POINT pt, SIZE sz, COLORREF clrBk,
                COLORREF clrFg, UINT nStyle);
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
// used as InsertItem's default hParent/hInsertAfter below. Real macros:
// #ifndef, not #ifdef _WIN32 (see afxwin.h's RDW_* for why).
#ifndef TVI_ROOT
#define TVI_ROOT ((HTREEITEM)0xFFFF0000)
#endif
#ifndef TVI_LAST
#define TVI_LAST ((HTREEITEM)0xFFFF0002)
#endif

class CTreeCtrl : public CWnd
{
public:
    CToolTipCtrl* GetToolTips() const;
    // Checkbox state, which is really item state bits; eMule's directory
    // tree toggles them (`SetCheck(hItem, !GetCheck(hItem))`).
    HTREEITEM GetFirstVisibleItem() const;
    BOOL SelectSetFirstVisible(HTREEITEM hItem);
    COLORREF SetBkColor(COLORREF clr);
    COLORREF GetBkColor() const;
    BOOL GetCheck(HTREEITEM hItem) const;
    BOOL SetCheck(HTREEITEM hItem, BOOL fCheck = TRUE);

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
    BOOL GetItem(TVITEM* pItem) const;
    BOOL SetItem(TVITEM* pItem);
    // Item state/geometry. Select() is the general form the single-purpose
    // SelectItem/SelectSetFirstVisible above are shorthands for.
    BOOL Select(HTREEITEM hItem, UINT nCode);
    UINT GetItemState(HTREEITEM hItem, UINT nStateMask) const;
    BOOL SetItemState(HTREEITEM hItem, UINT nState, UINT nStateMask);
    BOOL EnsureVisible(HTREEITEM hItem);
    BOOL GetItemRect(HTREEITEM hItem, LPRECT lpRect, BOOL bTextOnly) const;
    short GetItemHeight() const;
    short SetItemHeight(short cyHeight);
    UINT GetIndent() const;
    void SetIndent(UINT nIndent);
    COLORREF GetTextColor() const;
    COLORREF SetTextColor(COLORREF clr);
    // Drag & drop of tree items (eMule's shared-directories tree).
    HTREEITEM GetDropHilightItem() const;
    BOOL SelectDropTarget(HTREEITEM hItem);
    CImageList* CreateDragImage(HTREEITEM hItem);
};

// ---------------------------------------------------------------------
// CListCtrl (header afxcmn.h, deriva da CWnd)
// ---------------------------------------------------------------------
class CListCtrl : public CWnd
{
public:
    CToolTipCtrl* GetToolTips() const;
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
    // Starts the POSITION-based walk over the selection that the already
    // declared GetNextSelectedItem continues.
    POSITION GetFirstSelectedItemPosition() const;
    CHeaderCtrl* GetHeaderCtrl() const;
    BOOL SetItemCount(int nItems);
    BOOL GetSubItemRect(int iItem, int iSubItem, int nArea, CRect& ref) const;
    // The long form real MFC offers alongside the LVITEM one; eMule uses
    // it to set text, image, state and indent in a single call.
    BOOL SetItem(int nItem, int nSubItem, UINT nMask, LPCTSTR lpszItem, int nImage,
                 UINT nState, UINT nStateMask, LPARAM lParam, int nIndent);
    int GetSelectionMark() const;
    int SetSelectionMark(int iIndex);
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
    // Sub-item (column-aware) hit testing, which the report-mode lists use
    // to know which cell the mouse is over.
    int SubItemHitTest(LVHITTESTINFO* pInfo);
    BOOL SetColumnOrderArray(int iCount, int* piArray);
    BOOL GetColumnOrderArray(int* piArray, int iCount = -1) const;
    int GetTopIndex() const;
    int GetCountPerPage() const;
    int GetStringWidth(LPCTSTR lpsz) const;
    BOOL RedrawItems(int nFirst, int nLast);
    BOOL Scroll(CSize size);
    BOOL GetItemPosition(int nItem, LPPOINT lpPoint) const;
    BOOL SetItemPosition(int nItem, POINT pt);
    BOOL SelectDropTarget(int nIndex);
    CImageList* CreateDragImage(int nItem, LPPOINT lpPoint);
    // Colours: eMule repaints its lists to follow the current skin.
    COLORREF GetBkColor() const;
    BOOL SetBkColor(COLORREF cr);
    COLORREF GetTextColor() const;
    BOOL SetTextColor(COLORREF cr);
    COLORREF GetTextBkColor() const;
    BOOL SetTextBkColor(COLORREF cr);
    BOOL SetBkImage(LVBKIMAGE* plvbkImage);
    BOOL SetBkImage(HBITMAP hbm, BOOL bTile = TRUE, int xOffsetPercent = 0, int yOffsetPercent = 0);
    // The LVITEM form of SetItemState, alongside the mask/flags one above.
    BOOL SetItemState(int nItem, LVITEM* pItem);
    // The check-box column that LVS_EX_CHECKBOXES turns on.
    BOOL GetCheck(int nItem) const;
    BOOL SetCheck(int nItem, BOOL fCheck = TRUE);
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
    DWORD GetEventMask() const;
    BOOL SetParaFormat(PARAFORMAT& pf);
    BOOL SetParaFormat(PARAFORMAT2& pf);
    DWORD GetParaFormat(PARAFORMAT& pf) const;
    BOOL SetAutoURLDetect(BOOL bEnable = TRUE);
    UINT GetLimitText() const;
    long StreamOut(int nFormat, EDITSTREAM& es);
    long StreamIn(int nFormat, EDITSTREAM& es);
    COLORREF SetBackgroundColor(BOOL bSysColor, COLORREF cr);
    void LimitText(long nChars = 0);
    DWORD GetSelectionCharFormat(CHARFORMAT& cf) const;
    DWORD GetSelectionCharFormat(CHARFORMAT2& cf) const;
    void ReplaceSel(LPCTSTR lpszNewText, BOOL bCanUndo = FALSE);
    void GetSel(CHARRANGE& cr) const;
    void GetSel(long& nStartChar, long& nEndChar) const;
    // Clipboard/undo, which CHTRichEditCtrl exposes to its own callers
    // (`void CopySelectedItems() { Copy(); }`).
    void Copy();
    void Cut();
    void Paste();
    BOOL Undo();
    BOOL CanUndo() const;
    void Clear();
    void EmptyUndoBuffer();
    BOOL CanPaste(UINT nFormat = 0) const;
    DWORD GetDefaultCharFormat(CHARFORMAT& cf) const;
    DWORD GetDefaultCharFormat(CHARFORMAT2& cf) const;
    BOOL SetDefaultCharFormat(CHARFORMAT& cf);
    BOOL SetDefaultCharFormat(CHARFORMAT2& cf);
    long GetTextRange(int nFirst, int nLast, CString& refString) const;
    long GetTextLength() const;
    // Line-oriented access, used by the log/IRC views to scroll and to
    // pull a single line back out.
    int GetLineCount() const;
    int GetFirstVisibleLine() const;
    void LineScroll(int nLines, int nChars = 0);
    int LineLength(int nLine = -1) const;
    int GetLine(int nIndex, LPTSTR lpszBuffer) const;
    int GetLine(int nIndex, LPTSTR lpszBuffer, int nMaxLength) const;
    // The OLE interface behind the control, which eMule queries to embed
    // images in the rich text.
    IRichEditOle* GetIRichEditOle() const;
    // Keeps (or drops) the selection highlight when focus leaves.
    void HideSelection(BOOL bHide, BOOL bPerm);
    void SetOptions(WORD wOp, DWORD dwFlags);
};

// ---------------------------------------------------------------------
// CTabCtrl (header afxcmn.h, deriva da CWnd)
// ---------------------------------------------------------------------
class CTabCtrl : public CWnd
{
public:
    // The common-control Create: 4 arguments, no class name (the control
    // class is implied). Without it the call resolves to the 7-argument
    // CWnd::Create and fails on arity -- TreePropSheet.cpp:652 creates a
    // throwaway tab control just to measure its caption height.
    virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
    CToolTipCtrl* GetToolTips() const;
    CImageList* SetImageList(CImageList* pImageList);
    CImageList* GetImageList() const;
    void SetPadding(CSize size);
    int SetCurFocus(int nItem);
    void HighlightItem(int idItem, BOOL fHighlight = TRUE);
    BOOL GetItem(int nItem, TCITEM* pTabCtrlItem) const;
    BOOL SetItem(int nItem, TCITEM* pTabCtrlItem);
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
    void SetToolTips(CToolTipCtrl* pWndTip);
    // Converts between the whole-control rectangle and the display area
    // inside the tabs; eMule sizes its embedded views with it.
    void AdjustRect(BOOL bLarger, LPRECT lpRect);
    int SetMinTabWidth(int cx);
    // TCIS_* item state (highlighted/buttonpressed).
    DWORD GetItemState(int nItem, DWORD dwMask) const;
    BOOL SetItemState(int nItem, DWORD dwMask, DWORD dwState);
};

// ---------------------------------------------------------------------
// CToolBarCtrl (header afxcmn.h, deriva da CWnd)
// ---------------------------------------------------------------------
class CToolBarCtrl : public CWnd
{
public:
    virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
    BOOL CheckButton(int nID, BOOL bCheck = TRUE);
    // The query that goes with CheckButton. eMule's toolbar buttons act
    // as toggles for its panes and it reads their state back
    // (KademliaWnd.cpp, through its own CToolBarCtrlX/CDropDownButton).
    BOOL IsButtonChecked(int nID) const;
    BOOL SetButtonInfo(int nID, TBBUTTONINFO* ptbbi);
    BOOL GetItemRect(int nIndex, LPRECT lpRect) const;
    int GetButtonCount() const;
    UINT CommandToIndex(UINT nID) const;
    BOOL EnableButton(int nID, BOOL bEnable = TRUE);
    BOOL GetButton(int nIndex, LPTBBUTTON lpButton) const;
    CImageList* GetImageList() const;
    CImageList* SetImageList(CImageList* pImageList);
    BOOL AddButtons(int nNumButtons, LPTBBUTTON lpButtons);
    BOOL InsertButton(int nIndex, LPTBBUTTON lpButton);
    BOOL DeleteButton(int nIndex);
    DWORD SetExtendedStyle(DWORD dwExStyle);
    DWORD GetExtendedStyle() const;
    BOOL GetMaxSize(SIZE* pSize) const;
    void SetBorders(int iLeft, int iTop, int iRight, int iBottom);
    void GetBorders(int* piLeft, int* piTop, int* piRight, int* piBottom) const;
    CToolTipCtrl* GetToolTips() const;
    // Added during the FRONTEND/GDI blind-spot pass (see
    // ../../mfc_scan_srchybrid.md addendum): both are called via
    // qualified super-call in a CToolBarCtrl-derived class.
    CSize GetMaxSize();
    BOOL AutoSize();
    int GetButtonInfo(int nID, TBBUTTONINFO* ptbbi) const;
    void SetRows(int nRows, BOOL bLarger, LPRECT lpRect);
    BOOL SetMaxTextRows(int iMaxRows);
    int GetMaxTextRows() const;
    CImageList* SetDisabledImageList(CImageList* pImageList);
    BOOL SetButtonSize(CSize size);
    // Packed: LOWORD is the width, HIWORD the height.
    DWORD GetButtonSize() const;
    int GetRows() const;
    BOOL SetButtonWidth(int cxMin, int cxMax);
    BOOL IsButtonHidden(int nID) const;
    BOOL MapAccelerator(TCHAR chAccel, UINT* pIDBtn);
    void Customize();
    // Hides CWnd::GetStyle's counterpart deliberately (same pattern as
    // Create above): this is the toolbar's TBSTYLE_*, not the window style.
    void SetStyle(DWORD dwStyle);
    // The button label pool: a single '\0'-separated, '\0\0'-terminated
    // block, which is why eMule appends an extra NUL before calling.
    int AddStrings(LPCTSTR lpszStrings);
};

// ---------------------------------------------------------------------
// CStatusBarCtrl (header afxcmn.h, deriva da CWnd)
// ---------------------------------------------------------------------
class CStatusBarCtrl : public CWnd
{
public:
    BOOL SetBorders(int iHorzWidth, int iVertWidth, int iSpacing);
    void GetBorders(int* pBorders) const;
    virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
    BOOL SetText(LPCTSTR lpszText, int nPane, int nType);
    CString GetText(int nPane, int* pType = nullptr) const;
    int GetText(LPCTSTR lpszText, int nPane, int* pType = nullptr) const;
    BOOL SetParts(int nParts, int* pWidths);
    int GetParts(int nParts, int* pParts) const;
    BOOL GetRect(int nPane, LPRECT lpRect) const;
    // Hides CWnd::SetIcon deliberately (real MFC does the same): this one
    // addresses a pane, not the window.
    BOOL SetIcon(int nPane, HICON hIcon);
    HICON GetIcon(int nPane) const;
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
    // Timing and layout. The two-argument SetDelayTime is the TTDT_* form
    // (which delay to set); the one-argument one sets them all at once.
    void SetDelayTime(UINT nDelay);
    void SetDelayTime(DWORD dwDuration, int iTime);
    int GetDelayTime(DWORD dwDuration) const;
    void SetMargin(LPRECT lprc);
    void GetMargin(LPRECT lprc) const;
    int SetMaxTipWidth(int iWidth);
    int GetMaxTipWidth() const;
    int GetToolCount() const;
    // Forces an immediate repaint of the tip currently showing.
    void Update();
    // Converts between the tip's text rectangle and its window rectangle.
    void AdjustRect(LPRECT lprc, BOOL bLarger = TRUE);
};

// ---------------------------------------------------------------------
// CHeaderCtrl (header afxcmn.h, deriva da CWnd). eMule reaches it mostly
// via CListCtrl::GetHeaderCtrl() and calls SetItem/GetItemCount/
// Set-GetImageList/Set-GetOrderArray; Attach/Detach are inherited CWnd.
// ---------------------------------------------------------------------
class CHeaderCtrl : public CWnd
{
public:
    int OrderToIndex(int nOrder) const;
    int GetBitmapMargin() const;
    int SetBitmapMargin(int nWidth);
    int GetItemCount() const;
    BOOL GetItem(int nPos, HDITEM* pHeaderItem) const;
    BOOL SetItem(int nPos, HDITEM* pHeaderItem);
    int InsertItem(int nPos, HDITEM* pHeaderItem);
    BOOL DeleteItem(int nPos);
    BOOL GetItemRect(int nIndex, LPRECT lpRect) const;
    CImageList* SetImageList(CImageList* pImageList);
    CImageList* GetImageList() const;
    BOOL SetOrderArray(int iCount, int* piArray);
    BOOL GetOrderArray(int* piArray, int iCount);
};

// ---------------------------------------------------------------------
// CSpinButtonCtrl (header afxcmn.h, deriva da CWnd)
// ---------------------------------------------------------------------
class CSpinButtonCtrl : public CWnd
{
public:
    virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
    int SetPos(int nPos);
    int GetPos() const;
    void SetRange(short nLower, short nUpper);
    void SetRange32(int nLower, int nUpper);
    DWORD GetRange() const;
    void GetRange32(int& lower, int& upper) const;
    CWnd* SetBuddy(CWnd* pWndBuddy);
    CWnd* GetBuddy() const;
    UINT SetBase(int nBase);
    UINT GetBase() const;
};

// ---------------------------------------------------------------------
// CComboBoxEx (header afxcmn.h, deriva da CComboBox in real MFC)
// ---------------------------------------------------------------------
class CComboBoxEx : public CComboBox
{
public:
    int InsertItem(const COMBOBOXEXITEM* pCBItem);
    int DeleteItem(int iIndex);
    BOOL GetItem(COMBOBOXEXITEM* pCBItem) const;
    BOOL SetItem(const COMBOBOXEXITEM* pCBItem);
    int GetCount() const;
    CComboBox* GetComboBoxCtrl();
    CEdit* GetEditCtrl();
    CImageList* SetImageList(CImageList* pImageList);
    CImageList* GetImageList() const;
};

// ---------------------------------------------------------------------
// CSliderCtrl (header afxcmn.h, deriva da CWnd)
// ---------------------------------------------------------------------
class CSliderCtrl : public CWnd
{
public:
    void SetPos(int nPos);
    int GetPos() const;
    void SetRange(int nMin, int nMax, BOOL bRedraw = FALSE);
    void SetRangeMin(int nMin, BOOL bRedraw = FALSE);
    void SetRangeMax(int nMax, BOOL bRedraw = FALSE);
    void GetRange(int& nMin, int& nMax) const;
    int SetPageSize(int nSize);
    int GetPageSize() const;
    void SetTicFreq(int nFreq);
    BOOL SetTic(int nTic);
    void SetLineSize(int nSize);
    void ClearTics(BOOL bRedraw = FALSE);
    int GetNumTics() const;
};

// ---------------------------------------------------------------------
// CIPAddressCtrl (header afxcmn.h, deriva da CWnd)
// ---------------------------------------------------------------------
class CIPAddressCtrl : public CWnd
{
public:
    virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
    int GetAddress(BYTE& nField0, BYTE& nField1, BYTE& nField2, BYTE& nField3);
    int GetAddress(DWORD& dwAddress);
    void SetAddress(DWORD dwAddress);
    void SetAddress(BYTE nField0, BYTE nField1, BYTE nField2, BYTE nField3);
    void ClearAddress();
    BOOL IsBlank() const;
    BOOL SetFieldRange(int nField, BYTE nLower, BYTE nUpper);
    void SetFieldFocus(WORD nField);
};

// ---------------------------------------------------------------------
// CProgressCtrl (header afxcmn.h, deriva da CWnd)
// ---------------------------------------------------------------------
class CProgressCtrl : public CWnd
{
public:
    virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
    void SetRange(short nLower, short nUpper);
    void SetRange32(int nLower, int nUpper);
    void GetRange(int& nLower, int& nUpper);
    int SetPos(int nPos);
    int GetPos();
    int OffsetPos(int nPos);
    int SetStep(int nStep);
    int StepIt();
    BOOL SetMarquee(BOOL fMarqueeMode, int nInterval);
};

// ---------------------------------------------------------------------
// CAnimateCtrl (header afxcmn.h, deriva da CWnd)
// ---------------------------------------------------------------------
class CAnimateCtrl : public CWnd
{
public:
    virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
    BOOL Open(LPCTSTR lpszFileName);
    BOOL Open(UINT nID);
    BOOL Play(UINT nFrom, UINT nTo, UINT nRep);
    BOOL Stop();
    BOOL Close();
    BOOL Seek(UINT nTo);
};

// ---------------------------------------------------------------------
// CReBarCtrl — the rebar (band container) common control, host of the
// main toolbar. CEMuleDlg holds one by value (`CReBarCtrl
// m_ctlMainTopReBar;`), so until this existed every translation unit that
// saw EmuleDlg.h -- 68 of them -- died on that one member declaration.
//
// REBARBANDINFO comes from the real <commctrl.h> included at the top of
// this header; the non-Windows stand-in only needs the tag to exist,
// since it is used by pointer here.
// ---------------------------------------------------------------------
#ifndef _WIN32
struct REBARBANDINFO;
#endif

class CReBarCtrl : public CWnd
{
public:
    virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
    BOOL InsertBand(UINT uIndex, REBARBANDINFO* prbbi);
    BOOL GetBandInfo(UINT uBand, REBARBANDINFO* prbbi) const;
    BOOL SetBandInfo(UINT uBand, REBARBANDINFO* prbbi);
};

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
#ifdef _MSC_VER
#pragma warning(pop)
#endif
