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
// Deferred to later GDI slices (not needed for this one, and drawn here with
// the DC's default black pen / colour): GDI objects (CPen/CBrush/CFont/CBitmap
// with SelectObject), FillRect/FrameRect (brush-based), BitBlt/CreateCompatibleDC
// (memory DCs), mapping modes and clipping (defined as faithful minimal stubs so
// the vtable is complete). The on-screen paintEvent->OnPaint route is a separate
// slice; this one renders to the inspectable offscreen buffer.
#include "afxwin.h"
#include "driver_internal.h"

#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QPainter>
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

// The per-window paint surface + current DC state.
struct GdiSurface {
    QImage image;
    COLORREF textColor = 0x00000000;   // black
    COLORREF bkColor   = 0x00FFFFFF;   // white
    int      bkMode    = kOpaque;
    UINT     textAlign = 0;            // TA_LEFT | TA_TOP
    QPoint   cur{0, 0};
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

    // Reset to DC defaults, as a real freshly-created DC would be.
    s.textColor = 0x00000000;
    s.bkColor   = 0x00FFFFFF;
    s.bkMode    = kOpaque;
    s.textAlign = 0;
    s.cur       = QPoint(0, 0);
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
    // Default DC pen/brush: BLACK_PEN outline, WHITE_BRUSH fill (real GDI's
    // startup objects). Coloured pens/brushes arrive with the CPen/CBrush slice.
    p.setPen(Qt::black);
    p.setBrush(Qt::white);
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
    QPainter p(&s->image);
    p.setPen(Qt::black);   // current pen defaults to BLACK_PEN (CPen slice adds colour)
    p.drawLine(s->cur, QPoint(x, y));
    s->cur = QPoint(x, y);
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
    const QFontMetrics fm(p.font());
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
    QFont f;   // default DC font until the CFont slice; per-DC font is honoured then
    const QFontMetrics fm(f);
    (void)s;
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
CFont* CDC::SelectObject(CFont* /*pFont*/) { return nullptr; }  // CFont slice binds the QFont
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
