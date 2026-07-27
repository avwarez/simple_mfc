// gui/qt/driver_internal.h — Qt-driver internal helpers shared across the
// driver's translation units (cwnd.cpp, ddx.cpp, controls.cpp). NOT part of
// the frozen MFC interface: these are library-internal mechanisms, so they
// live in the driver's own namespace with our own shapes/names.
#pragma once
#include "afxwin.h"

#include <vector>

class QWidget;
class QImage;
class CListCtrl;   // afxcmn.h; only used by pointer in EnableOwnerDraw below

namespace smfc_qt {

// The QWidget currently bound to a CWnd (its m_hWnd), or nullptr.
QWidget* WidgetOf(const CWnd* w);

// --- GDI (CDC) offscreen paint surfaces ------------------------------------
// A window-owned CDC (CPaintDC/CClientDC/CWindowDC) draws onto an offscreen
// QImage keyed by the owner CWnd*. Its lifetime is the window's: DestroyWindow
// releases it (there is no ~CPaintDC hook in the frozen interface to do RAII).
// WndSurfaceImage exposes that buffer (e.g. for tests / a future paintEvent
// blit); ReleaseWndSurface frees it.
QImage* WndSurfaceImage(const CWnd* owner);
void    ReleaseWndSurface(const CWnd* owner);

// The QImage behind a CBitmap handle (HBITMAP == the CGdiObject* token), or
// nullptr if the handle is not a live bitmap. Implemented in cdc.cpp, which owns
// the GdiObjs() map; lets imagelist.cpp / controls.cpp read a bitmap's pixels.
const QImage* BitmapImageFromHandle(HBITMAP h);

// Blit an image onto a window/memory DC's offscreen surface at (x,y) (optionally
// scaled to dw x dh; dw<=0 means "native size"). False if dc has no surface.
// Implemented in cdc.cpp (owns surfOf); called by CImageList::Draw/DrawEx.
bool BlitImageToDC(CDC* dc, int x, int y, const QImage& img, int dw = 0, int dh = 0);

// Owner-draw item surface (cdc.cpp): a single reusable HDC-backed QImage the
// list item delegate paints one item into. BeginItemSurface(w,h) resets+sizes
// it and returns the HDC to put in the DRAWITEMSTRUCT; ItemSurfaceImage() gets
// the pixels back to blit onto the viewport. CDC::FromHandle wraps the HDC.
HDC     BeginItemSurface(int w, int h);
QImage* ItemSurfaceImage();

// --- icon registry (imagelist.cpp) -----------------------------------------
// Icons have no Win32 producer in the portable build (no LoadIcon/CWinApp yet),
// so a drawable HICON is minted here from pixels (e.g. CImageList::ExtractIcon).
// RegisterIcon hands back an opaque HICON token; IconImage resolves it back to
// the pixels (nullptr if unknown), for CDC::DrawIcon / CStatic::SetIcon.
HICON         RegisterIcon(const QImage& img);
const QImage* IconImage(HICON hIcon);

// The radio-button group that DDX_Radio(pDX, nIDC, ...) addresses: the control
// ids, in template order, of the buttons belonging to nIDC's group. The group
// starts at nIDC and runs until the next control carrying WS_GROUP (Win32's
// group semantics), mirroring how real MFC's DDX_Radio walks GetNextDlgGroupItem.
// Resolved from the dialog's .rc template; empty if dlg has no template.
std::vector<int> RadioGroup(CWnd* dlg, int nIDC);

// DDX_Control binding: rebind control id `nIDC` in dialog `dlg` to the typed
// control object `rControl`. Detaches the builder's placeholder wrapper for
// that id, attaches rControl to the same QWidget, and makes
// dlg->GetDlgItem(nIDC) return rControl afterwards. No-op if the id is unknown.
void BindDlgControl(CWnd* dlg, int nIDC, CWnd& rControl);

// Where a window's Win32 style bits live in this driver: Qt properties on the
// host widget, seeded from the .rc template when the dialog is built. This is
// the backing store CWnd::GetStyle/GetExStyle/ModifyStyle(Ex) read and write.
inline constexpr const char* kStyleProp   = "smfc_style";
inline constexpr const char* kExStyleProp = "smfc_exstyle";

// Apply everything a window's CURRENT style bits imply, for the concrete type
// it actually is. Called when a typed control is bound (DDX_Control) and after
// any ModifyStyle/ModifyStyleEx, so a style change takes effect immediately.
// Today it drives owner-draw: a CListCtrl carrying LVS_OWNERDRAWFIXED gets the
// item delegate that routes each row through DrawItem, and loses it if the bit
// is cleared. Idempotent, and a no-op for types with no style-driven behaviour.
void ApplyStyleBehaviour(CWnd& wnd);

// Install / remove the owner-draw item delegate on `list`'s bound view: with it
// installed, each visible row is painted by calling list->DrawItem(&dis) with a
// DRAWITEMSTRUCT (itemID=row, rcItem=item-local, hDC=an item surface), which is
// the WM_DRAWITEM reflection an LVS_OWNERDRAWFIXED list gets on Windows.
// Normally reached through ApplyStyleBehaviour rather than called directly.
// No-op if the list is not bound to a QTreeWidget.
void EnableOwnerDraw(CListCtrl* list, bool enable = true);

} // namespace smfc_qt
