// gui/qt/cdc.cpp — CDC / CPaintDC / CClientDC / CWindowDC over QPainter.
//
// This is the first GDI slice: the device-context drawing surface (colours,
// text, lines, solid rectangles) that eMule's OnPaint/OnDraw/DrawItem code
// draws through. A window-owned DC renders onto an offscreen QImage keyed by
// the owner CWnd*; because the frozen interface has no ~CPaintDC hook to do the
// real BeginPaint/EndPaint RAII, the surface's lifetime is tied to the window
// instead (DestroyWindow -> ReleaseWndSurface). Each drawing call opens a
// short-lived QPainter on that QImage, which retains the pixels between calls.
//
// Slice 2 adds the GDI objects: CPen/CBrush/CFont (over QPen/QBrush/QFont) with
// SelectObject, so lines/rects/text pick up the selected colour/style/font, plus
// the brush-based FillRect/FrameRect. Each object's driver-side data hangs off
// its m_hObject via the GdiObjs() map.
//
// Deferred to later GDI slices: CBitmap + pattern brushes, BitBlt/
// CreateCompatibleDC (memory DCs), CImageList/DrawIcon, mapping modes and
// clipping (defined as faithful minimal stubs so the vtable is complete).
// CreateFontIndirect/CreateBrushIndirect/GetObject are stubs because LOGFONT/
// LOGBRUSH are opaque (forward-declared) on this non-Windows platform. The
// on-screen paintEvent->OnPaint route is a separate slice; this one renders to
// the inspectable offscreen buffer.
#include "afxwin.h"
#include "driver_internal.h"

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QRect>
#include <QString>
#include <QWidget>

#include <algorithm>
#include <unordered_map>

namespace {

// --- GDI constants the POSIX shim does not carry (real values from wingdi.h/
// winuser.h) ---------------------------------------------------------------
constexpr int  kTransparent = 1;   // TRANSPARENT
constexpr int  kOpaque      = 2;   // OPAQUE
// DrawText format flags.
constexpr UINT kDtRight     = 0x0002;
constexpr UINT kDtCenter    = 0x0001;
constexpr UINT kDtVCenter   = 0x0004;
constexpr UINT kDtBottom    = 0x0008;
constexpr UINT kDtWordBreak = 0x0010;
constexpr UINT kDtSingle    = 0x0020;
constexpr UINT kDtCalcRect  = 0x0400;
constexpr UINT kDtNoPrefix  = 0x0800;
// Pen styles (PS_*), hatch styles (HS_*), and the bold weight threshold (FW_BOLD).
constexpr int kPsNull  = 5;
constexpr int kFwBold  = 700;

Qt::PenStyle toPenStyle(int ps)
{
    switch (ps) {
        case 1:  return Qt::DashLine;        // PS_DASH
        case 2:  return Qt::DotLine;         // PS_DOT
        case 3:  return Qt::DashDotLine;     // PS_DASHDOT
        case 4:  return Qt::DashDotDotLine;  // PS_DASHDOTDOT
        case 5:  return Qt::NoPen;           // PS_NULL
        default: return Qt::SolidLine;       // PS_SOLID / PS_INSIDEFRAME
    }
}
Qt::BrushStyle toHatchStyle(int hs)
{
    switch (hs) {
        case 0:  return Qt::HorPattern;       // HS_HORIZONTAL
        case 1:  return Qt::VerPattern;       // HS_VERTICAL
        case 2:  return Qt::FDiagPattern;     // HS_FDIAGONAL
        case 3:  return Qt::BDiagPattern;     // HS_BDIAGONAL
        case 4:  return Qt::CrossPattern;     // HS_CROSS
        default: return Qt::DiagCrossPattern; // HS_DIAGCROSS
    }
}

// A driver-side GDI object (what a CPen/CBrush/CFont's m_hObject refers to).
struct GdiObj {
    enum Kind { Pen = 1, Brush, Font } kind = Pen;
    QPen   pen{Qt::black};
    QBrush brush{Qt::white, Qt::SolidPattern};
    QFont  font;
    bool   nullPen = false;    // PS_NULL
    bool   nullBrush = false;  // NULL_BRUSH / hollow
};

// GDI objects are keyed by the owning CGdiObject* (its m_hObject also holds
// that address as the "created" token). Keying by the object pointer bounds the
// leak the frozen interface's missing ~CGdiObject would otherwise cause: a
// stack CPen reused in a loop reuses the same key instead of growing the map.
std::unordered_map<const CGdiObject*, GdiObj>& GdiObjs()
{
    static std::unordered_map<const CGdiObject*, GdiObj> m;
    return m;
}
GdiObj* objOf(const CGdiObject* g)
{
    if (!g || !g->m_hObject) return nullptr;
    auto it = GdiObjs().find(g);
    return it == GdiObjs().end() ? nullptr : &it->second;
}
GdiObj& makeObj(CGdiObject* g, GdiObj::Kind k)
{
    g->m_hObject = reinterpret_cast<HGDIOBJ>(g);   // non-null "created" token
    GdiObj& o = GdiObjs()[g];
    o = GdiObj{};
    o.kind = k;
    return o;
}

// The per-window paint surface + current DC state.
struct GdiSurface {
    QImage image;
    COLORREF textColor = 0x00000000;   // black
    COLORREF bkColor   = 0x00FFFFFF;   // white
    int      bkMode    = kOpaque;
    UINT     textAlign = 0;            // TA_LEFT | TA_TOP
    QPoint   cur{0, 0};
    // Currently selected objects: the resolved Qt values used when drawing, and
    // the app CGdiObject*s so SelectObject can return the previously selected
    // one (the `CPen* pOld = dc.SelectObject(&pen); ...; dc.SelectObject(pOld);`
    // restore idiom). Defaults are the DC's stock BLACK_PEN / WHITE_BRUSH / font.
    QPen   curPen{Qt::black};
    QBrush curBrush{Qt::white, Qt::SolidPattern};
    QFont  curFont;
    bool   curNullPen = false;
    bool   curNullBrush = false;
    CPen*  selPen = nullptr;
    CBrush* selBrush = nullptr;
    CFont* selFont = nullptr;
};

std::unordered_map<const void*, GdiSurface>& Surfaces()
{
    static std::unordered_map<const void*, GdiSurface> m;
    return m;
}

// A window-owned DC encodes its owner CWnd* in m_hDC (see the DC ctors), so a
// method recovers its surface straight from the handle.
GdiSurface* surfOf(const CDC* dc)
{
    if (!dc || !dc->m_hDC) return nullptr;
    auto it = Surfaces().find(dc->m_hDC);
    return it == Surfaces().end() ? nullptr : &it->second;
}

QColor toQColor(COLORREF cr)
{
    return QColor(int(cr & 0xFF), int((cr >> 8) & 0xFF), int((cr >> 16) & 0xFF));
}
COLORREF toColorref(const QColor& c)
{
    return COLORREF(c.red()) | (COLORREF(c.green()) << 8) | (COLORREF(c.blue()) << 16);
}

// Build (or resize+reset) the offscreen surface for a window-owned DC and bind
// the DC to it. Used by all three CWnd-based DC ctors.
void initWndDC(CDC* dc, CWnd* pWnd)
{
    dc->m_hDC = reinterpret_cast<HDC>(pWnd);
    dc->m_hAttribDC = dc->m_hDC;
    GdiSurface& s = Surfaces()[pWnd];

    QWidget* w = smfc_qt::WidgetOf(pWnd);
    const int cw = w ? std::max(1, w->width())  : 1;
    const int ch = w ? std::max(1, w->height()) : 1;
    if (s.image.width() != cw || s.image.height() != ch)
        s.image = QImage(cw, ch, QImage::Format_ARGB32_Premultiplied);
    s.image.fill(Qt::white);   // a fresh paint surface (approximates the bg)

    // Reset to DC defaults, as a real freshly-created DC would be (stock
    // BLACK_PEN / WHITE_BRUSH / system font, no object selected).
    s.textColor = 0x00000000;
    s.bkColor   = 0x00FFFFFF;
    s.bkMode    = kOpaque;
    s.textAlign = 0;
    s.cur       = QPoint(0, 0);
    s.curPen    = QPen(Qt::black);
    s.curBrush  = QBrush(Qt::white, Qt::SolidPattern);
    s.curFont   = QFont();
    s.curNullPen = s.curNullBrush = false;
    s.selPen = nullptr; s.selBrush = nullptr; s.selFont = nullptr;
}

QRect toQRect(LPCRECT r)
{
    return QRect(r->left, r->top, r->right - r->left, r->bottom - r->top);
}

} // namespace

// --- driver-internal surface access (declared in driver_internal.h) --------
namespace smfc_qt {
QImage* WndSurfaceImage(const CWnd* owner)
{
    auto it = Surfaces().find(owner);
    return it == Surfaces().end() ? nullptr : &it->second.image;
}
void ReleaseWndSurface(const CWnd* owner)
{
    Surfaces().erase(owner);
}
} // namespace smfc_qt

// ---------------------------------------------------------------------------
// CDC — state
// ---------------------------------------------------------------------------
HDC  CDC::GetSafeHdc()          { return m_hDC; }
BOOL CDC::Attach(HDC hDC)       { m_hDC = hDC; return TRUE; }
HDC  CDC::Detach()              { HDC h = m_hDC; m_hDC = nullptr; return h; }
BOOL CDC::DeleteDC()            { m_hDC = nullptr; return TRUE; }
CDC* CDC::FromHandle(HDC)       { return nullptr; }   // reverse map: later slice

COLORREF CDC::SetTextColor(COLORREF cr)
{
    GdiSurface* s = surfOf(this);
    if (!s) return 0;
    const COLORREF prev = s->textColor;
    s->textColor = cr;
    return prev;
}
COLORREF CDC::GetTextColor() const { GdiSurface* s = surfOf(this); return s ? s->textColor : 0; }

COLORREF CDC::SetBkColor(COLORREF cr)
{
    GdiSurface* s = surfOf(this);
    if (!s) return 0;
    const COLORREF prev = s->bkColor;
    s->bkColor = cr;
    return prev;
}
COLORREF CDC::GetBkColor() const { GdiSurface* s = surfOf(this); return s ? s->bkColor : 0; }

int CDC::SetBkMode(int nBkMode)
{
    GdiSurface* s = surfOf(this);
    if (!s) return 0;
    const int prev = s->bkMode;
    s->bkMode = nBkMode;
    return prev;
}

UINT CDC::SetTextAlign(UINT nFlags)
{
    GdiSurface* s = surfOf(this);
    if (!s) return 0;
    const UINT prev = s->textAlign;
    s->textAlign = nFlags;
    return prev;
}
UINT CDC::GetTextAlign() { GdiSurface* s = surfOf(this); return s ? s->textAlign : 0; }

// ---------------------------------------------------------------------------
// CDC — solid rectangles
// ---------------------------------------------------------------------------
void CDC::FillSolidRect(int x, int y, int cx, int cy, COLORREF clr)
{
    GdiSurface* s = surfOf(this);
    if (!s) return;
    QPainter p(&s->image);
    p.fillRect(QRect(x, y, cx, cy), toQColor(clr));
}
void CDC::FillSolidRect(LPCRECT lpRect, COLORREF clr)
{
    if (lpRect) FillSolidRect(lpRect->left, lpRect->top,
                              lpRect->right - lpRect->left,
                              lpRect->bottom - lpRect->top, clr);
}

BOOL CDC::Rectangle(int x1, int y1, int x2, int y2)
{
    GdiSurface* s = surfOf(this);
    if (!s) return FALSE;
    QPainter p(&s->image);
    p.setPen(s->curNullPen ? QPen(Qt::NoPen) : s->curPen);
    p.setBrush(s->curNullBrush ? QBrush(Qt::NoBrush) : s->curBrush);
    p.drawRect(QRect(x1, y1, x2 - x1 - 1, y2 - y1 - 1));
    return TRUE;
}
BOOL CDC::Rectangle(LPCRECT lpRect)
{
    return lpRect ? Rectangle(lpRect->left, lpRect->top, lpRect->right, lpRect->bottom)
                  : FALSE;
}

void CDC::Draw3dRect(int x, int y, int cx, int cy, COLORREF clrTopLeft, COLORREF clrBottomRight)
{
    // Two L-shaped 1px edges — the classic MFC 3D border.
    FillSolidRect(x, y, cx - 1, 1, clrTopLeft);          // top
    FillSolidRect(x, y, 1, cy - 1, clrTopLeft);          // left
    FillSolidRect(x + cx - 1, y, 1, cy, clrBottomRight); // right
    FillSolidRect(x, y + cy - 1, cx, 1, clrBottomRight); // bottom
}
void CDC::Draw3dRect(LPCRECT lpRect, COLORREF clrTopLeft, COLORREF clrBottomRight)
{
    if (lpRect)
        Draw3dRect(lpRect->left, lpRect->top, lpRect->right - lpRect->left,
                   lpRect->bottom - lpRect->top, clrTopLeft, clrBottomRight);
}

void CDC::DrawFocusRect(LPCRECT lpRect)
{
    GdiSurface* s = surfOf(this);
    if (!s || !lpRect) return;
    QPainter p(&s->image);
    QPen pen(Qt::black);
    pen.setStyle(Qt::DotLine);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    QRect r = toQRect(lpRect);
    p.drawRect(r.adjusted(0, 0, -1, -1));
}

// ---------------------------------------------------------------------------
// CDC — lines
// ---------------------------------------------------------------------------
CPoint CDC::MoveTo(int x, int y)
{
    GdiSurface* s = surfOf(this);
    const QPoint prev = s ? s->cur : QPoint(0, 0);
    if (s) s->cur = QPoint(x, y);
    return CPoint(prev.x(), prev.y());
}
CPoint CDC::MoveTo(POINT point) { return MoveTo(point.x, point.y); }

BOOL CDC::LineTo(int x, int y)
{
    GdiSurface* s = surfOf(this);
    if (!s) return FALSE;
    if (!s->curNullPen) {
        QPainter p(&s->image);
        p.setPen(s->curPen);
        p.drawLine(s->cur, QPoint(x, y));
    }
    s->cur = QPoint(x, y);   // MoveTo semantics still advance under a NULL pen
    return TRUE;
}
BOOL CDC::LineTo(POINT point) { return LineTo(point.x, point.y); }

// ---------------------------------------------------------------------------
// CDC — text
// ---------------------------------------------------------------------------
BOOL CDC::TextOut(int x, int y, LPCTSTR lpszString, int nCount)
{
    GdiSurface* s = surfOf(this);
    if (!s || !lpszString) return FALSE;
    const QString text = QString::fromWCharArray(lpszString,
                                                 nCount < 0 ? -1 : nCount);
    QPainter p(&s->image);
    p.setFont(s->curFont);
    const QFontMetrics fm(s->curFont);
    if (s->bkMode == kOpaque)
        p.fillRect(x, y, fm.horizontalAdvance(text), fm.height(), toQColor(s->bkColor));
    p.setPen(toQColor(s->textColor));
    p.drawText(x, y + fm.ascent(), text);   // TextOut's (x,y) is the top-left
    return TRUE;
}
BOOL CDC::TextOut(int x, int y, const CString& str)
{
    return TextOut(x, y, str.GetString(), str.GetLength());
}

int CDC::DrawText(LPCTSTR lpszString, int nCount, LPRECT lpRect, UINT nFormat)
{
    GdiSurface* s = surfOf(this);
    if (!s || !lpszString || !lpRect) return 0;
    const QString text = QString::fromWCharArray(lpszString,
                                                 nCount < 0 ? -1 : nCount);
    int flags = 0;
    if (nFormat & kDtCenter)      flags |= Qt::AlignHCenter;
    else if (nFormat & kDtRight)  flags |= Qt::AlignRight;
    else                          flags |= Qt::AlignLeft;
    if (nFormat & kDtVCenter)     flags |= Qt::AlignVCenter;
    else if (nFormat & kDtBottom) flags |= Qt::AlignBottom;
    else                          flags |= Qt::AlignTop;
    if (nFormat & kDtSingle)      flags |= Qt::TextSingleLine;
    if (nFormat & kDtWordBreak)   flags |= Qt::TextWordWrap;
    flags |= (nFormat & kDtNoPrefix) ? Qt::TextHideMnemonic : Qt::TextShowMnemonic;

    QPainter p(&s->image);
    p.setFont(s->curFont);
    const QRect r = toQRect(lpRect);
    if (nFormat & kDtCalcRect) {
        const QRect bb = p.boundingRect(r, flags, text);
        lpRect->right = lpRect->left + bb.width();
        lpRect->bottom = lpRect->top + bb.height();
        return bb.height();
    }
    if (s->bkMode == kOpaque)
        p.fillRect(r, toQColor(s->bkColor));
    p.setPen(toQColor(s->textColor));
    const QRect bb = p.boundingRect(r, flags, text);
    p.drawText(r, flags, text);
    return bb.height();
}
int CDC::DrawText(const CString& str, LPRECT lpRect, UINT nFormat)
{
    return DrawText(str.GetString(), str.GetLength(), lpRect, nFormat);
}

CSize CDC::GetTextExtent(LPCTSTR lpszString, int nCount)
{
    const QString text = QString::fromWCharArray(lpszString,
                                                 nCount < 0 ? -1 : nCount);
    GdiSurface* s = surfOf(this);
    const QFontMetrics fm(s ? s->curFont : QFont());
    return CSize(fm.horizontalAdvance(text), fm.height());
}
CSize CDC::GetTextExtent(const CString& str)
{
    return GetTextExtent(str.GetString(), str.GetLength());
}

// ---------------------------------------------------------------------------
// CDC — pixels
// ---------------------------------------------------------------------------
COLORREF CDC::SetPixel(int x, int y, COLORREF crColor)
{
    GdiSurface* s = surfOf(this);
    if (!s || x < 0 || y < 0 || x >= s->image.width() || y >= s->image.height())
        return COLORREF(-1);
    s->image.setPixelColor(x, y, toQColor(crColor));
    return crColor;
}
COLORREF CDC::SetPixel(POINT point, COLORREF crColor) { return SetPixel(point.x, point.y, crColor); }
COLORREF CDC::GetPixel(int x, int y)
{
    GdiSurface* s = surfOf(this);
    if (!s || x < 0 || y < 0 || x >= s->image.width() || y >= s->image.height())
        return COLORREF(-1);
    return toColorref(s->image.pixelColor(x, y));
}
COLORREF CDC::GetPixel(POINT point) { return GetPixel(point.x, point.y); }

// ---------------------------------------------------------------------------
// CDC — virtuals (defined so CDC's vtable is complete). SelectObject(CFont*)
// is the key function; the mapping-mode/clip ones are faithful minimal stubs
// until their own slices land.
// ---------------------------------------------------------------------------
CFont* CDC::SelectObject(CFont* pFont)
{
    GdiSurface* s = surfOf(this);
    if (!s) return nullptr;
    CFont* prev = s->selFont;
    s->selFont = pFont;
    const GdiObj* o = objOf(pFont);
    s->curFont = o ? o->font : QFont();   // null selects the stock font
    return prev;
}
CPen* CDC::SelectObject(CPen* pPen)
{
    GdiSurface* s = surfOf(this);
    if (!s) return nullptr;
    CPen* prev = s->selPen;
    s->selPen = pPen;
    if (const GdiObj* o = objOf(pPen)) { s->curPen = o->pen; s->curNullPen = o->nullPen; }
    else { s->curPen = QPen(Qt::black); s->curNullPen = false; }
    return prev;
}
CBrush* CDC::SelectObject(CBrush* pBrush)
{
    GdiSurface* s = surfOf(this);
    if (!s) return nullptr;
    CBrush* prev = s->selBrush;
    s->selBrush = pBrush;
    if (const GdiObj* o = objOf(pBrush)) { s->curBrush = o->brush; s->curNullBrush = o->nullBrush; }
    else { s->curBrush = QBrush(Qt::white, Qt::SolidPattern); s->curNullBrush = false; }
    return prev;
}
CGdiObject* CDC::SelectObject(CGdiObject* pObject)
{
    // Dispatch on the object's kind to the typed overload, returning the
    // previously selected object of that kind.
    const GdiObj* o = objOf(pObject);
    if (!o) return nullptr;
    switch (o->kind) {
        case GdiObj::Pen:   return SelectObject(static_cast<CPen*>(pObject));
        case GdiObj::Brush: return SelectObject(static_cast<CBrush*>(pObject));
        case GdiObj::Font:  return SelectObject(static_cast<CFont*>(pObject));
    }
    return nullptr;
}

// --- brush-based fills (now that CBrush resolves) --------------------------
void CDC::FillRect(LPCRECT lpRect, CBrush* pBrush)
{
    GdiSurface* s = surfOf(this);
    if (!s || !lpRect) return;
    const GdiObj* o = objOf(pBrush);
    QPainter p(&s->image);
    p.fillRect(toQRect(lpRect), o ? o->brush : s->curBrush);
}
void CDC::FrameRect(LPCRECT lpRect, CBrush* pBrush)
{
    GdiSurface* s = surfOf(this);
    if (!s || !lpRect) return;
    const GdiObj* o = objOf(pBrush);
    const QColor col = (o ? o->brush.color() : s->curBrush.color());
    const QRect r = toQRect(lpRect);
    QPainter p(&s->image);
    // A 1px border in the brush colour (real FrameRect strokes with the brush).
    p.fillRect(QRect(r.left(), r.top(), r.width(), 1), col);
    p.fillRect(QRect(r.left(), r.bottom(), r.width(), 1), col);
    p.fillRect(QRect(r.left(), r.top(), 1, r.height()), col);
    p.fillRect(QRect(r.right(), r.top(), 1, r.height()), col);
}

CSize  CDC::SetWindowExt(int, int)   { return CSize(0, 0); }
CSize  CDC::SetViewportExt(int, int) { return CSize(0, 0); }
CPoint CDC::SetWindowOrg(int, int)   { return CPoint(0, 0); }
CPoint CDC::SetViewportOrg(int, int) { return CPoint(0, 0); }
int    CDC::SelectClipRgn(CRgn*)     { return 1; /* SIMPLEREGION */ }

// ---------------------------------------------------------------------------
// CPaintDC / CClientDC / CWindowDC — all construct over the owner's surface.
// (A CClientDC/CWindowDC only differs from CPaintDC by which area it maps; for
// this offscreen slice they share the window-sized buffer.)
// ---------------------------------------------------------------------------
CPaintDC::CPaintDC(CWnd* pWnd)  : CDC() { initWndDC(this, pWnd); }
CClientDC::CClientDC(CWnd* pWnd) : CDC() { initWndDC(this, pWnd); }
CWindowDC::CWindowDC(CWnd* pWnd) : CDC() { initWndDC(this, pWnd); }

// ---------------------------------------------------------------------------
// CGdiObject — the driver-side object referenced by m_hObject lives in the
// GdiObjs() map; DeleteObject frees it. (No ~CGdiObject in the frozen interface
// to auto-free, so an object not explicitly deleted persists until its address
// is reused — see the map-keying note above.)
// ---------------------------------------------------------------------------
BOOL CGdiObject::DeleteObject()
{
    GdiObjs().erase(this);
    m_hObject = nullptr;
    return TRUE;
}
BOOL    CGdiObject::Attach(HGDIOBJ hObject) { m_hObject = hObject; return TRUE; }
HGDIOBJ CGdiObject::Detach() { HGDIOBJ h = m_hObject; m_hObject = nullptr; return h; }
HGDIOBJ CGdiObject::GetSafeHandle() const { return m_hObject; }
int     CGdiObject::GetObject(int, LPVOID) const { return 0; }  // needs LOGPEN/LOGBRUSH/LOGFONT

// --- CPen ------------------------------------------------------------------
CPen::CPen() {}
CPen::CPen(int nPenStyle, int nWidth, COLORREF crColor) { CreatePen(nPenStyle, nWidth, crColor); }
CPen::CPen(int nPenStyle, int nWidth, const LOGBRUSH*, int, const DWORD*)
{
    CreatePen(nPenStyle, nWidth, static_cast<const LOGBRUSH*>(nullptr));
}
BOOL CPen::CreatePen(int nPenStyle, int nWidth, COLORREF crColor)
{
    GdiObj& o = makeObj(this, GdiObj::Pen);
    QPen pen(toQColor(crColor));
    pen.setStyle(toPenStyle(nPenStyle));
    pen.setWidth(nWidth <= 0 ? 1 : nWidth);
    o.pen = pen;
    o.nullPen = (nPenStyle == kPsNull);
    return TRUE;
}
BOOL CPen::CreatePen(int nPenStyle, int nWidth, const LOGBRUSH*, int, const DWORD*)
{
    // LOGBRUSH is opaque on POSIX (forward-declared), so the colour is unknown;
    // create the styled/width pen in black. Real colour arrives with a LOGBRUSH
    // that this platform can read (a later concern).
    GdiObj& o = makeObj(this, GdiObj::Pen);
    QPen pen(Qt::black);
    pen.setStyle(toPenStyle(nPenStyle));
    pen.setWidth(nWidth <= 0 ? 1 : nWidth);
    o.pen = pen;
    o.nullPen = (nPenStyle == kPsNull);
    return TRUE;
}

// --- CBrush ----------------------------------------------------------------
CBrush::CBrush() {}
CBrush::CBrush(COLORREF crColor) { CreateSolidBrush(crColor); }
CBrush::CBrush(int nIndex, COLORREF crColor) { CreateHatchBrush(nIndex, crColor); }
CBrush::CBrush(CBitmap*) { makeObj(this, GdiObj::Brush); }  // pattern brush: bitmap slice
BOOL CBrush::CreateSolidBrush(COLORREF crColor)
{
    GdiObj& o = makeObj(this, GdiObj::Brush);
    o.brush = QBrush(toQColor(crColor), Qt::SolidPattern);
    return TRUE;
}
BOOL CBrush::CreateHatchBrush(int nIndex, COLORREF crColor)
{
    GdiObj& o = makeObj(this, GdiObj::Brush);
    o.brush = QBrush(toQColor(crColor), toHatchStyle(nIndex));
    return TRUE;
}
BOOL CBrush::CreatePatternBrush(CBitmap*) { return FALSE; }            // bitmap slice
BOOL CBrush::CreateDIBPatternBrush(HGLOBAL, UINT) { return FALSE; }    // bitmap slice
BOOL CBrush::CreateDIBPatternBrush(const void*, UINT) { return FALSE; }
BOOL CBrush::CreateBrushIndirect(const LOGBRUSH*) { return FALSE; }    // LOGBRUSH opaque on POSIX

// --- CFont -----------------------------------------------------------------
BOOL CFont::CreateFontIndirect(const LOGFONT*)
{
    // LOGFONT is opaque on POSIX (forward-declared), so its fields can't be
    // read; register a default font so selection has something valid.
    makeObj(this, GdiObj::Font);
    return TRUE;
}
int CFont::GetLogFont(LOGFONT*) { return 0; }   // LOGFONT opaque on POSIX
BOOL CFont::CreateFont(int nHeight, int /*nWidth*/, int /*nEscapement*/, int /*nOrientation*/,
                       int nWeight, BYTE bItalic, BYTE bUnderline, BYTE cStrikeOut,
                       BYTE /*nCharSet*/, BYTE /*nOutPrecision*/, BYTE /*nClipPrecision*/,
                       BYTE /*nQuality*/, BYTE /*nPitchAndFamily*/, LPCTSTR lpszFacename)
{
    GdiObj& o = makeObj(this, GdiObj::Font);
    QFont f;
    if (lpszFacename) f.setFamily(QString::fromWCharArray(lpszFacename));
    const int h = nHeight < 0 ? -nHeight : nHeight;   // <0 = char height, >0 = cell height
    if (h > 0) f.setPixelSize(h);
    f.setBold(nWeight >= kFwBold);
    f.setItalic(bItalic != 0);
    f.setUnderline(bUnderline != 0);
    f.setStrikeOut(cStrikeOut != 0);
    o.font = f;
    return TRUE;
}
BOOL CFont::CreatePointFont(int nPointSize, LPCTSTR lpszFaceName, CDC*)
{
    GdiObj& o = makeObj(this, GdiObj::Font);
    QFont f;
    if (lpszFaceName) f.setFamily(QString::fromWCharArray(lpszFaceName));
    f.setPointSizeF(nPointSize / 10.0);   // MFC point size is in tenths of a point
    o.font = f;
    return TRUE;
}
