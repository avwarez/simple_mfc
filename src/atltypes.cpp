// atltypes.cpp — CPoint/CSize/CRect. NATIVE implementation: pure integer
// coordinate arithmetic, no GDI/Win32 dependency (real MFC implements
// these the same way -- they are value types, not windows or GDI objects,
// so a stock desktop/laptop/server C++17 toolchain is all this needs).
#include "atltypes.h"

#include <algorithm>
#include <cstring>

// ---------------------------------------------------------------------
// CPoint
// ---------------------------------------------------------------------
CPoint::CPoint() { x = 0; y = 0; }
CPoint::CPoint(long initX, long initY) { x = initX; y = initY; }
CPoint::CPoint(POINT initPt) { x = initPt.x; y = initPt.y; }
CPoint::CPoint(SIZE initSize) { x = initSize.cx; y = initSize.cy; }
// Real MFC's packed-lParam constructor (MAKEPOINTS-style): low word is x,
// high word is y, each sign-extended from 16 bits.
CPoint::CPoint(DWORD dwPoint)
{
    x = static_cast<short>(static_cast<unsigned short>(dwPoint & 0xFFFFu));
    y = static_cast<short>(static_cast<unsigned short>((dwPoint >> 16) & 0xFFFFu));
}

void CPoint::Offset(int xOffset, int yOffset) noexcept { x += xOffset; y += yOffset; }
void CPoint::Offset(POINT point) noexcept { x += point.x; y += point.y; }
void CPoint::Offset(SIZE size) noexcept { x += size.cx; y += size.cy; }

BOOL CPoint::operator==(POINT point) const noexcept { return (x == point.x && y == point.y) ? TRUE : FALSE; }
BOOL CPoint::operator!=(POINT point) const noexcept { return (x != point.x || y != point.y) ? TRUE : FALSE; }
void CPoint::operator+=(SIZE size) noexcept { x += size.cx; y += size.cy; }
void CPoint::operator+=(POINT point) noexcept { x += point.x; y += point.y; }
void CPoint::operator-=(SIZE size) noexcept { x -= size.cx; y -= size.cy; }
void CPoint::operator-=(POINT point) noexcept { x -= point.x; y -= point.y; }
CPoint CPoint::operator+(SIZE size) const noexcept { return CPoint(x + size.cx, y + size.cy); }
CPoint CPoint::operator+(POINT point) const noexcept { return CPoint(x + point.x, y + point.y); }
CPoint CPoint::operator-(SIZE size) const noexcept { return CPoint(x - size.cx, y - size.cy); }
CPoint CPoint::operator-() const noexcept { return CPoint(-x, -y); }

// CSize operator-(POINT): the difference between two points is a vector
// (a size), matching real MFC's documented return type.
CSize CPoint::operator-(POINT point) const noexcept { return CSize(x - point.x, y - point.y); }

// The two CRect-returning overloads: offset/shrink every corner of
// lpRect by this point, matching real MFC's documented behavior.
CRect CPoint::operator+(const RECT* lpRect) const noexcept
{
    return CRect(lpRect->left + x, lpRect->top + y, lpRect->right + x, lpRect->bottom + y);
}
CRect CPoint::operator-(const RECT* lpRect) const noexcept
{
    return CRect(lpRect->left - x, lpRect->top - y, lpRect->right - x, lpRect->bottom - y);
}

// ---------------------------------------------------------------------
// CSize
// ---------------------------------------------------------------------
CSize::CSize() { cx = 0; cy = 0; }
CSize::CSize(long initCX, long initCY) { cx = initCX; cy = initCY; }
CSize::CSize(SIZE initSize) { cx = initSize.cx; cy = initSize.cy; }
CSize::CSize(POINT initPt) { cx = initPt.x; cy = initPt.y; }

BOOL CSize::operator==(SIZE size) const noexcept { return (cx == size.cx && cy == size.cy) ? TRUE : FALSE; }
BOOL CSize::operator!=(SIZE size) const noexcept { return (cx != size.cx || cy != size.cy) ? TRUE : FALSE; }
void CSize::operator+=(SIZE size) noexcept { cx += size.cx; cy += size.cy; }
void CSize::operator-=(SIZE size) noexcept { cx -= size.cx; cy -= size.cy; }
CSize CSize::operator+(SIZE size) const noexcept { return CSize(cx + size.cx, cy + size.cy); }
CSize CSize::operator-(SIZE size) const noexcept { return CSize(cx - size.cx, cy - size.cy); }
CSize CSize::operator-() const noexcept { return CSize(-cx, -cy); }
CPoint CSize::operator+(POINT point) const noexcept { return CPoint(cx + point.x, cy + point.y); }
CPoint CSize::operator-(POINT point) const noexcept { return CPoint(cx - point.x, cy - point.y); }
CRect CSize::operator+(const RECT* lpRect) const noexcept
{
    return CRect(lpRect->left + cx, lpRect->top + cy, lpRect->right + cx, lpRect->bottom + cy);
}
CRect CSize::operator-(const RECT* lpRect) const noexcept
{
    return CRect(lpRect->left - cx, lpRect->top - cy, lpRect->right - cx, lpRect->bottom - cy);
}

// ---------------------------------------------------------------------
// CRect
// ---------------------------------------------------------------------
CRect::CRect() { left = top = right = bottom = 0; }
CRect::CRect(int l, int t, int r, int b) { left = l; top = t; right = r; bottom = b; }
CRect::CRect(const RECT& srcRect) { left = srcRect.left; top = srcRect.top; right = srcRect.right; bottom = srcRect.bottom; }
CRect::CRect(LPCRECT lpSrcRect) { left = lpSrcRect->left; top = lpSrcRect->top; right = lpSrcRect->right; bottom = lpSrcRect->bottom; }
CRect::CRect(POINT point, SIZE size)
{
    left = point.x; top = point.y;
    right = point.x + size.cx; bottom = point.y + size.cy;
}
CRect::CRect(POINT topLeft, POINT bottomRight)
{
    left = topLeft.x; top = topLeft.y;
    right = bottomRight.x; bottom = bottomRight.y;
}

int CRect::Height() const noexcept { return static_cast<int>(bottom - top); }
int CRect::Width() const noexcept { return static_cast<int>(right - left); }
BOOL CRect::PtInRect(POINT point) const noexcept
{
    return (point.x >= left && point.x < right && point.y >= top && point.y < bottom) ? TRUE : FALSE;
}

void CRect::MoveToX(int x) noexcept { int w = Width(); left = x; right = x + w; }
void CRect::MoveToY(int y) noexcept { int h = Height(); top = y; bottom = y + h; }
void CRect::MoveToXY(int x, int y) noexcept { MoveToX(x); MoveToY(y); }
void CRect::MoveToXY(POINT point) noexcept { MoveToXY(point.x, point.y); }

void CRect::OffsetRect(int x, int y) noexcept { left += x; right += x; top += y; bottom += y; }
void CRect::OffsetRect(POINT point) noexcept { OffsetRect(point.x, point.y); }
void CRect::OffsetRect(SIZE size) noexcept { OffsetRect(size.cx, size.cy); }

void CRect::InflateRect(int x, int y) noexcept { left -= x; top -= y; right += x; bottom += y; }
void CRect::InflateRect(SIZE size) noexcept { InflateRect(size.cx, size.cy); }
void CRect::InflateRect(LPCRECT lpRect) noexcept
{
    left -= lpRect->left; top -= lpRect->top; right += lpRect->right; bottom += lpRect->bottom;
}
void CRect::InflateRect(int l, int t, int r, int b) noexcept { left -= l; top -= t; right += r; bottom += b; }

void CRect::DeflateRect(int x, int y) noexcept { InflateRect(-x, -y); }
void CRect::DeflateRect(SIZE size) noexcept { InflateRect(-size.cx, -size.cy); }
void CRect::DeflateRect(LPCRECT lpRect) noexcept
{
    left += lpRect->left; top += lpRect->top; right -= lpRect->right; bottom -= lpRect->bottom;
}
void CRect::DeflateRect(int l, int t, int r, int b) noexcept { InflateRect(-l, -t, -r, -b); }

void CRect::SetRect(int x1, int y1, int x2, int y2) noexcept { left = x1; top = y1; right = x2; bottom = y2; }
CPoint CRect::CenterPoint() const noexcept { return CPoint(left + Width() / 2, top + Height() / 2); }
CSize CRect::Size() const noexcept { return CSize(Width(), Height()); }
BOOL CRect::IsRectEmpty() const noexcept { return (left >= right || top >= bottom) ? TRUE : FALSE; }
void CRect::SetRectEmpty() noexcept { left = top = right = bottom = 0; }

// TopLeft()/BottomRight() reinterpret this rect's own storage as two
// consecutive POINTs -- exactly real MFC's implementation, made valid by
// tagRECT's {left,top,right,bottom} layout matching two tagPOINT{x,y}
// pairs back to back, with CPoint/CRect adding no extra data of their
// own (no virtuals, no extra members) on top of their POD bases.
CPoint& CRect::TopLeft() noexcept { return *reinterpret_cast<CPoint*>(this); }
const CPoint& CRect::TopLeft() const noexcept { return *reinterpret_cast<const CPoint*>(this); }
CPoint& CRect::BottomRight() noexcept { return reinterpret_cast<CPoint*>(this)[1]; }
const CPoint& CRect::BottomRight() const noexcept { return reinterpret_cast<const CPoint*>(this)[1]; }

BOOL CRect::operator==(const RECT& rect) const noexcept
{
    return (left == rect.left && top == rect.top && right == rect.right && bottom == rect.bottom) ? TRUE : FALSE;
}
BOOL CRect::operator!=(const RECT& rect) const noexcept { return (*this == rect) ? FALSE : TRUE; }

void CRect::operator+=(POINT point) noexcept { OffsetRect(point); }
void CRect::operator+=(SIZE size) noexcept { OffsetRect(size); }
void CRect::operator+=(LPCRECT lpRect) noexcept { InflateRect(lpRect); }
void CRect::operator-=(POINT point) noexcept { OffsetRect(-point.x, -point.y); }
void CRect::operator-=(SIZE size) noexcept { OffsetRect(-size.cx, -size.cy); }
void CRect::operator-=(LPCRECT lpRect) noexcept { DeflateRect(lpRect); }

CRect CRect::operator+(POINT point) const noexcept { CRect r(*this); r.OffsetRect(point); return r; }
CRect CRect::operator+(SIZE size) const noexcept { CRect r(*this); r.OffsetRect(size); return r; }
CRect CRect::operator+(LPCRECT lpRect) const noexcept { CRect r(*this); r.InflateRect(lpRect); return r; }
CRect CRect::operator-(POINT point) const noexcept { CRect r(*this); r.OffsetRect(-point.x, -point.y); return r; }
CRect CRect::operator-(SIZE size) const noexcept { CRect r(*this); r.OffsetRect(-size.cx, -size.cy); return r; }
CRect CRect::operator-(LPCRECT lpRect) const noexcept { CRect r(*this); r.DeflateRect(lpRect); return r; }

// SubtractRect/IntersectRect/UnionRect reproduce the documented Win32
// RECT-function algorithms (which real MFC's CRect members forward to):
// see e.g. the Win32 SubtractRect/IntersectRect/UnionRect reference pages.
namespace
{
bool IsRectEmptyRaw(LPCRECT r) { return r->left >= r->right || r->top >= r->bottom; }
} // namespace

BOOL CRect::IntersectRect(LPCRECT lpRect1, LPCRECT lpRect2) noexcept
{
    long l = std::max(lpRect1->left, lpRect2->left);
    long t = std::max(lpRect1->top, lpRect2->top);
    long r = std::min(lpRect1->right, lpRect2->right);
    long b = std::min(lpRect1->bottom, lpRect2->bottom);
    if (IsRectEmptyRaw(lpRect1) || IsRectEmptyRaw(lpRect2) || l >= r || t >= b)
    {
        SetRectEmpty();
        return FALSE;
    }
    left = l; top = t; right = r; bottom = b;
    return TRUE;
}

BOOL CRect::UnionRect(LPCRECT lpRect1, LPCRECT lpRect2) noexcept
{
    bool empty1 = IsRectEmptyRaw(lpRect1);
    bool empty2 = IsRectEmptyRaw(lpRect2);
    if (empty1 && empty2) { SetRectEmpty(); return FALSE; }
    if (empty1) { *this = CRect(*lpRect2); return TRUE; }
    if (empty2) { *this = CRect(*lpRect1); return TRUE; }
    long l = std::min(lpRect1->left, lpRect2->left);
    long t = std::min(lpRect1->top, lpRect2->top);
    long r = std::max(lpRect1->right, lpRect2->right);
    long b = std::max(lpRect1->bottom, lpRect2->bottom);
    left = l; top = t; right = r; bottom = b;
    return TRUE;
}

BOOL CRect::SubtractRect(LPCRECT lpRectSrc1, LPCRECT lpRectSrc2) noexcept
{
    if (IsRectEmptyRaw(lpRectSrc1))
    {
        SetRectEmpty();
        return FALSE;
    }

    CRect dest(*lpRectSrc1);
    CRect tmp;
    if (tmp.IntersectRect(lpRectSrc1, lpRectSrc2))
    {
        if (tmp.left == dest.left && tmp.top == dest.top && tmp.right == dest.right && tmp.bottom == dest.bottom)
        {
            SetRectEmpty();
            return FALSE;
        }
        if (tmp.top == dest.top && tmp.bottom == dest.bottom)
        {
            if (tmp.left == dest.left) dest.left = tmp.right;
            else if (tmp.right == dest.right) dest.right = tmp.left;
        }
        else if (tmp.left == dest.left && tmp.right == dest.right)
        {
            if (tmp.top == dest.top) dest.top = tmp.bottom;
            else if (tmp.bottom == dest.bottom) dest.bottom = tmp.top;
        }
    }
    *this = dest;
    return TRUE;
}

void CRect::operator&=(const RECT& rect) noexcept { CRect r1(*this); IntersectRect(&r1, &rect); }
void CRect::operator|=(const RECT& rect) noexcept { CRect r1(*this); UnionRect(&r1, &rect); }
CRect CRect::operator&(const RECT& rect2) const noexcept { CRect r; r.IntersectRect(this, &rect2); return r; }
CRect CRect::operator|(const RECT& rect2) const noexcept { CRect r; r.UnionRect(this, &rect2); return r; }
