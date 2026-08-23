#include "eatltypes.h"

#include <algorithm>

ECPoint::ECPoint() { x = 0; y = 0; }
ECPoint::ECPoint(long initX, long initY) { x = static_cast<LONG>(initX); y = static_cast<LONG>(initY); }
ECPoint::ECPoint(POINT initPt) { x = initPt.x; y = initPt.y; }
ECPoint::ECPoint(SIZE initSize) { x = initSize.cx; y = initSize.cy; }
ECPoint::ECPoint(DWORD dwPoint)
{
    x = static_cast<short>(static_cast<unsigned short>(dwPoint & 0xFFFFu));
    y = static_cast<short>(static_cast<unsigned short>((dwPoint >> 16) & 0xFFFFu));
}

void ECPoint::Offset(int xOffset, int yOffset) noexcept { x += xOffset; y += yOffset; }
void ECPoint::Offset(POINT point) noexcept { x += point.x; y += point.y; }
void ECPoint::Offset(SIZE size) noexcept { x += size.cx; y += size.cy; }

BOOL ECPoint::operator==(POINT point) const noexcept { return (x == point.x && y == point.y) ? TRUE : FALSE; }
BOOL ECPoint::operator!=(POINT point) const noexcept { return (x != point.x || y != point.y) ? TRUE : FALSE; }
void ECPoint::operator+=(SIZE size) noexcept { x += size.cx; y += size.cy; }
void ECPoint::operator+=(POINT point) noexcept { x += point.x; y += point.y; }
void ECPoint::operator-=(SIZE size) noexcept { x -= size.cx; y -= size.cy; }
void ECPoint::operator-=(POINT point) noexcept { x -= point.x; y -= point.y; }
ECPoint ECPoint::operator+(SIZE size) const noexcept { return ECPoint(x + size.cx, y + size.cy); }
ECPoint ECPoint::operator+(POINT point) const noexcept { return ECPoint(x + point.x, y + point.y); }
ECPoint ECPoint::operator-(SIZE size) const noexcept { return ECPoint(x - size.cx, y - size.cy); }
ECPoint ECPoint::operator-() const noexcept { return ECPoint(-x, -y); }
ECSize ECPoint::operator-(POINT point) const noexcept { return ECSize(x - point.x, y - point.y); }

ECRect ECPoint::operator+(const RECT* lpRect) const noexcept
{
    return ECRect(lpRect->left + x, lpRect->top + y, lpRect->right + x, lpRect->bottom + y);
}
ECRect ECPoint::operator-(const RECT* lpRect) const noexcept
{
    return ECRect(lpRect->left - x, lpRect->top - y, lpRect->right - x, lpRect->bottom - y);
}

ECSize::ECSize() { cx = 0; cy = 0; }
ECSize::ECSize(long initCX, long initCY) { cx = static_cast<LONG>(initCX); cy = static_cast<LONG>(initCY); }
ECSize::ECSize(SIZE initSize) { cx = initSize.cx; cy = initSize.cy; }
ECSize::ECSize(POINT initPt) { cx = initPt.x; cy = initPt.y; }

BOOL ECSize::operator==(SIZE size) const noexcept { return (cx == size.cx && cy == size.cy) ? TRUE : FALSE; }
BOOL ECSize::operator!=(SIZE size) const noexcept { return (cx != size.cx || cy != size.cy) ? TRUE : FALSE; }
void ECSize::operator+=(SIZE size) noexcept { cx += size.cx; cy += size.cy; }
void ECSize::operator-=(SIZE size) noexcept { cx -= size.cx; cy -= size.cy; }
ECSize ECSize::operator+(SIZE size) const noexcept { return ECSize(cx + size.cx, cy + size.cy); }
ECSize ECSize::operator-(SIZE size) const noexcept { return ECSize(cx - size.cx, cy - size.cy); }
ECSize ECSize::operator-() const noexcept { return ECSize(-cx, -cy); }
ECPoint ECSize::operator+(POINT point) const noexcept { return ECPoint(cx + point.x, cy + point.y); }
ECPoint ECSize::operator-(POINT point) const noexcept { return ECPoint(cx - point.x, cy - point.y); }
ECRect ECSize::operator+(const RECT* lpRect) const noexcept
{
    return ECRect(lpRect->left + cx, lpRect->top + cy, lpRect->right + cx, lpRect->bottom + cy);
}
ECRect ECSize::operator-(const RECT* lpRect) const noexcept
{
    return ECRect(lpRect->left - cx, lpRect->top - cy, lpRect->right - cx, lpRect->bottom - cy);
}

ECRect::ECRect() { left = top = right = bottom = 0; }
ECRect::ECRect(int l, int t, int r, int b) { left = l; top = t; right = r; bottom = b; }
ECRect::ECRect(const RECT& srcRect) { left = srcRect.left; top = srcRect.top; right = srcRect.right; bottom = srcRect.bottom; }
ECRect::ECRect(LPCRECT lpSrcRect) { left = lpSrcRect->left; top = lpSrcRect->top; right = lpSrcRect->right; bottom = lpSrcRect->bottom; }
ECRect::ECRect(POINT point, SIZE size)
{
    left = point.x; top = point.y;
    right = point.x + size.cx; bottom = point.y + size.cy;
}
ECRect::ECRect(POINT topLeft, POINT bottomRight)
{
    left = topLeft.x; top = topLeft.y;
    right = bottomRight.x; bottom = bottomRight.y;
}

int ECRect::Height() const noexcept { return static_cast<int>(bottom - top); }
int ECRect::Width() const noexcept { return static_cast<int>(right - left); }
BOOL ECRect::PtInRect(POINT point) const noexcept
{
    return (point.x >= left && point.x < right && point.y >= top && point.y < bottom) ? TRUE : FALSE;
}

void ECRect::MoveToX(int x) noexcept { int w = Width(); left = x; right = x + w; }
void ECRect::MoveToY(int y) noexcept { int h = Height(); top = y; bottom = y + h; }
void ECRect::MoveToXY(int x, int y) noexcept { MoveToX(x); MoveToY(y); }
void ECRect::MoveToXY(POINT point) noexcept { MoveToXY(point.x, point.y); }

void ECRect::OffsetRect(int x, int y) noexcept { left += x; right += x; top += y; bottom += y; }
void ECRect::OffsetRect(POINT point) noexcept { OffsetRect(point.x, point.y); }
void ECRect::OffsetRect(SIZE size) noexcept { OffsetRect(size.cx, size.cy); }

void ECRect::InflateRect(int x, int y) noexcept { left -= x; top -= y; right += x; bottom += y; }
void ECRect::InflateRect(SIZE size) noexcept { InflateRect(size.cx, size.cy); }
void ECRect::InflateRect(LPCRECT lpRect) noexcept
{
    left -= lpRect->left; top -= lpRect->top; right += lpRect->right; bottom += lpRect->bottom;
}
void ECRect::InflateRect(int l, int t, int r, int b) noexcept { left -= l; top -= t; right += r; bottom += b; }

void ECRect::DeflateRect(int x, int y) noexcept { InflateRect(-x, -y); }
void ECRect::DeflateRect(SIZE size) noexcept { InflateRect(-size.cx, -size.cy); }
void ECRect::DeflateRect(LPCRECT lpRect) noexcept
{
    left += lpRect->left; top += lpRect->top; right -= lpRect->right; bottom -= lpRect->bottom;
}
void ECRect::DeflateRect(int l, int t, int r, int b) noexcept { InflateRect(-l, -t, -r, -b); }

void ECRect::SetRect(int x1, int y1, int x2, int y2) noexcept { left = x1; top = y1; right = x2; bottom = y2; }
ECPoint ECRect::CenterPoint() const noexcept { return ECPoint(left + Width() / 2, top + Height() / 2); }
ECSize ECRect::Size() const noexcept { return ECSize(Width(), Height()); }
BOOL ECRect::IsRectEmpty() const noexcept { return (left >= right || top >= bottom) ? TRUE : FALSE; }
void ECRect::SetRectEmpty() noexcept { left = top = right = bottom = 0; }

static_assert(sizeof(ECPoint) == sizeof(tagPOINT), "ECPoint adds no storage to POINT");
static_assert(sizeof(ECRect) == 2 * sizeof(ECPoint), "a RECT is two POINTs back to back");

ECPoint& ECRect::TopLeft() noexcept { return *reinterpret_cast<ECPoint*>(this); }
const ECPoint& ECRect::TopLeft() const noexcept { return *reinterpret_cast<const ECPoint*>(this); }
ECPoint& ECRect::BottomRight() noexcept { return reinterpret_cast<ECPoint*>(this)[1]; }
const ECPoint& ECRect::BottomRight() const noexcept { return reinterpret_cast<const ECPoint*>(this)[1]; }

BOOL ECRect::operator==(const RECT& rect) const noexcept
{
    return (left == rect.left && top == rect.top && right == rect.right && bottom == rect.bottom) ? TRUE : FALSE;
}
BOOL ECRect::operator!=(const RECT& rect) const noexcept { return (*this == rect) ? FALSE : TRUE; }

void ECRect::operator+=(POINT point) noexcept { OffsetRect(point); }
void ECRect::operator+=(SIZE size) noexcept { OffsetRect(size); }
void ECRect::operator+=(LPCRECT lpRect) noexcept { InflateRect(lpRect); }
void ECRect::operator-=(POINT point) noexcept { OffsetRect(-point.x, -point.y); }
void ECRect::operator-=(SIZE size) noexcept { OffsetRect(-size.cx, -size.cy); }
void ECRect::operator-=(LPCRECT lpRect) noexcept { DeflateRect(lpRect); }

ECRect ECRect::operator+(POINT point) const noexcept { ECRect r(*this); r.OffsetRect(point); return r; }
ECRect ECRect::operator+(SIZE size) const noexcept { ECRect r(*this); r.OffsetRect(size); return r; }
ECRect ECRect::operator+(LPCRECT lpRect) const noexcept { ECRect r(*this); r.InflateRect(lpRect); return r; }
ECRect ECRect::operator-(POINT point) const noexcept { ECRect r(*this); r.OffsetRect(-point.x, -point.y); return r; }
ECRect ECRect::operator-(SIZE size) const noexcept { ECRect r(*this); r.OffsetRect(-size.cx, -size.cy); return r; }
ECRect ECRect::operator-(LPCRECT lpRect) const noexcept { ECRect r(*this); r.DeflateRect(lpRect); return r; }

namespace
{
bool IsRectEmptyRaw(LPCRECT r) { return r->left >= r->right || r->top >= r->bottom; }
}

BOOL ECRect::IntersectRect(LPCRECT lpRect1, LPCRECT lpRect2) noexcept
{
    LONG l = std::max<LONG>(lpRect1->left, lpRect2->left);
    LONG t = std::max<LONG>(lpRect1->top, lpRect2->top);
    LONG r = std::min<LONG>(lpRect1->right, lpRect2->right);
    LONG b = std::min<LONG>(lpRect1->bottom, lpRect2->bottom);
    if (IsRectEmptyRaw(lpRect1) || IsRectEmptyRaw(lpRect2) || l >= r || t >= b)
    {
        SetRectEmpty();
        return FALSE;
    }
    left = l; top = t; right = r; bottom = b;
    return TRUE;
}

BOOL ECRect::UnionRect(LPCRECT lpRect1, LPCRECT lpRect2) noexcept
{
    bool empty1 = IsRectEmptyRaw(lpRect1);
    bool empty2 = IsRectEmptyRaw(lpRect2);
    if (empty1 && empty2) { SetRectEmpty(); return FALSE; }
    if (empty1) { *this = ECRect(*lpRect2); return TRUE; }
    if (empty2) { *this = ECRect(*lpRect1); return TRUE; }
    LONG l = std::min<LONG>(lpRect1->left, lpRect2->left);
    LONG t = std::min<LONG>(lpRect1->top, lpRect2->top);
    LONG r = std::max<LONG>(lpRect1->right, lpRect2->right);
    LONG b = std::max<LONG>(lpRect1->bottom, lpRect2->bottom);
    left = l; top = t; right = r; bottom = b;
    return TRUE;
}

BOOL ECRect::SubtractRect(LPCRECT lpRectSrc1, LPCRECT lpRectSrc2) noexcept
{
    if (IsRectEmptyRaw(lpRectSrc1))
    {
        SetRectEmpty();
        return FALSE;
    }

    ECRect dest(*lpRectSrc1);
    ECRect tmp;
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

void ECRect::operator&=(const RECT& rect) noexcept { ECRect r1(*this); IntersectRect(&r1, &rect); }
void ECRect::operator|=(const RECT& rect) noexcept { ECRect r1(*this); UnionRect(&r1, &rect); }
ECRect ECRect::operator&(const RECT& rect2) const noexcept { ECRect r; r.IntersectRect(this, &rect2); return r; }
ECRect ECRect::operator|(const RECT& rect2) const noexcept { ECRect r; r.UnionRect(this, &rect2); return r; }
