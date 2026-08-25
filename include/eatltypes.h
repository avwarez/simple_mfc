#pragma once
#include "eafx.h"

#ifndef _WIN32
struct tagPOINT
{
    LONG x;
    LONG y;
};
using POINT = tagPOINT;
using LPPOINT = POINT*;

struct tagSIZE
{
    LONG cx;
    LONG cy;
};
using SIZE = tagSIZE;
using LPSIZE = SIZE*;

struct tagRECT
{
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
};
using RECT = tagRECT;
using LPRECT = RECT*;
using LPCRECT = const RECT*;
#endif

class ECRect;
class ECSize;

class ECPoint : public tagPOINT
{
public:
    void SetPoint(int X, int Y) noexcept { x = X; y = Y; }
    ECPoint();
    ECPoint(long initX, long initY);
    ECPoint(POINT initPt);
    ECPoint(SIZE initSize);
    ECPoint(DWORD dwPoint);

    void Offset(int xOffset, int yOffset) noexcept;
    void Offset(POINT point) noexcept;
    void Offset(SIZE size) noexcept;

    BOOL operator==(POINT point) const noexcept;
    BOOL operator!=(POINT point) const noexcept;
    void operator+=(SIZE size) noexcept;
    void operator+=(POINT point) noexcept;
    void operator-=(SIZE size) noexcept;
    void operator-=(POINT point) noexcept;
    ECPoint operator+(SIZE size) const noexcept;
    ECPoint operator+(POINT point) const noexcept;
    ECRect operator+(const RECT* lpRect) const noexcept;
    ECPoint operator-(SIZE size) const noexcept;
    ECSize operator-(POINT point) const noexcept;
    ECRect operator-(const RECT* lpRect) const noexcept;
    ECPoint operator-() const noexcept;
};

class ECSize : public tagSIZE
{
public:
    ECSize();
    ECSize(long initCX, long initCY);
    ECSize(SIZE initSize);
    ECSize(POINT initPt);

    BOOL operator==(SIZE size) const noexcept;
    BOOL operator!=(SIZE size) const noexcept;
    void operator+=(SIZE size) noexcept;
    void operator-=(SIZE size) noexcept;
    ECSize operator+(SIZE size) const noexcept;
    ECSize operator-(SIZE size) const noexcept;
    ECSize operator-() const noexcept;
    ECPoint operator+(POINT point) const noexcept;
    ECPoint operator-(POINT point) const noexcept;
    ECRect operator+(const RECT* lpRect) const noexcept;
    ECRect operator-(const RECT* lpRect) const noexcept;
};

class ECRect : public tagRECT
{
public:
    ECRect();
    ECRect(int l, int t, int r, int b);
    ECRect(const RECT& srcRect);
    ECRect(LPCRECT lpSrcRect);
    ECRect(POINT point, SIZE size);
    ECRect(POINT topLeft, POINT bottomRight);

    operator LPRECT() noexcept { return this; }
    operator LPCRECT() const noexcept { return this; }

    int Height() const noexcept;
    int Width() const noexcept;
    BOOL PtInRect(POINT point) const noexcept;
    void MoveToX(int x) noexcept;
    void OffsetRect(int x, int y) noexcept;
    void OffsetRect(POINT point) noexcept;
    void OffsetRect(SIZE size) noexcept;
    void InflateRect(int x, int y) noexcept;
    void InflateRect(SIZE size) noexcept;
    void InflateRect(LPCRECT lpRect) noexcept;
    void InflateRect(int l, int t, int r, int b) noexcept;
    ECPoint& TopLeft() noexcept;
    const ECPoint& TopLeft() const noexcept;
    void DeflateRect(int x, int y) noexcept;
    void DeflateRect(SIZE size) noexcept;
    void DeflateRect(LPCRECT lpRect) noexcept;
    void DeflateRect(int l, int t, int r, int b) noexcept;
    void SetRect(int x1, int y1, int x2, int y2) noexcept;
    ECPoint CenterPoint() const noexcept;
    ECSize Size() const noexcept;
    ECPoint& BottomRight() noexcept;
    const ECPoint& BottomRight() const noexcept;
    BOOL IsRectEmpty() const noexcept;
    void SetRectEmpty() noexcept;
    BOOL SubtractRect(LPCRECT lpRectSrc1, LPCRECT lpRectSrc2) noexcept;
    BOOL IntersectRect(LPCRECT lpRect1, LPCRECT lpRect2) noexcept;
    BOOL UnionRect(LPCRECT lpRect1, LPCRECT lpRect2) noexcept;

    BOOL operator==(const RECT& rect) const noexcept;
    BOOL operator!=(const RECT& rect) const noexcept;
    void operator+=(POINT point) noexcept;
    void operator+=(SIZE size) noexcept;
    void operator+=(LPCRECT lpRect) noexcept;
    void operator-=(POINT point) noexcept;
    void operator-=(SIZE size) noexcept;
    void operator-=(LPCRECT lpRect) noexcept;
    void operator&=(const RECT& rect) noexcept;
    void operator|=(const RECT& rect) noexcept;
    ECRect operator+(POINT point) const noexcept;
    ECRect operator+(SIZE size) const noexcept;
    ECRect operator+(LPCRECT lpRect) const noexcept;
    ECRect operator-(POINT point) const noexcept;
    ECRect operator-(SIZE size) const noexcept;
    ECRect operator-(LPCRECT lpRect) const noexcept;
    ECRect operator&(const RECT& rect2) const noexcept;
    ECRect operator|(const RECT& rect2) const noexcept;
};
