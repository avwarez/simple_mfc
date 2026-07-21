// atltypes.h — reference STUB (declarations only, no implementation).
// ATL/MFC "shared classes": CPoint/CRect/CSize. Per Microsoft Learn these
// are NOT declared in afxwin.h like the other GDI classes, but in this
// dedicated shared header (afxwin.h includes it transitively, as in real
// MFC). Self-contained: only needs the base primitive typedefs (BOOL,
// UINT) from afx.h, not the rest of the GUI hierarchy.
#pragma once
#include "afx.h"

// ---------------------------------------------------------------------
// Plain Win32 structs (normally from windef.h). On a real Windows/MSVC
// target these come from the real <windows.h> instead of being redefined
// here: discovered (2026-07-20, compiling real eMule/srchybrid against
// this header on windows-latest) that eMule also includes real Win32
// headers directly (<winsock2.h> etc., for non-MFC networking), which
// pull in the actual tagRECT/tagPOINT/tagSIZE from windef.h — defining
// our own unconditionally collided with them (C2011 "type redefinition").
// On non-Windows targets (this project's main portability point) no such
// header exists, so we still provide these ourselves.
// ---------------------------------------------------------------------
#ifdef _WIN32
#include <windows.h>
#else
struct tagPOINT
{
    long x;
    long y;
};
using POINT = tagPOINT;
using LPPOINT = POINT*;
using LPCPOINT = const POINT*;

struct tagSIZE
{
    long cx;
    long cy;
};
using SIZE = tagSIZE;
using LPSIZE = SIZE*;
using LPCSIZE = const SIZE*;

struct tagRECT
{
    long left;
    long top;
    long right;
    long bottom;
};
using RECT = tagRECT;
using LPRECT = RECT*;
using LPCRECT = const RECT*;
#endif

// ---------------------------------------------------------------------
// CPoint (header atltypes.h, derives from tagPOINT)
// ---------------------------------------------------------------------
class CPoint : public tagPOINT
{
public:
    CPoint();
    CPoint(long initX, long initY);
    CPoint(POINT initPt);
    CPoint(SIZE initSize);

    void Offset(int xOffset, int yOffset) noexcept;
    void Offset(POINT point) noexcept;
    void Offset(SIZE size) noexcept;

    BOOL operator==(POINT point) const noexcept;
    BOOL operator!=(POINT point) const noexcept;
    void operator+=(SIZE size) noexcept;
    void operator+=(POINT point) noexcept;
    void operator-=(SIZE size) noexcept;
    void operator-=(POINT point) noexcept;
    CPoint operator+(SIZE size) const noexcept;
    CPoint operator+(POINT point) const noexcept;
    class CRect operator+(const RECT* lpRect) const noexcept;
};

// ---------------------------------------------------------------------
// CSize (header atltypes.h, derives from tagSIZE)
// ---------------------------------------------------------------------
class CSize : public tagSIZE
{
public:
    CSize();
    CSize(long initCX, long initCY);
    CSize(SIZE initSize);
    CSize(POINT initPt);

    BOOL operator==(SIZE size) const noexcept;
    BOOL operator!=(SIZE size) const noexcept;
    void operator+=(SIZE size) noexcept;
    void operator-=(SIZE size) noexcept;
    CSize operator+(SIZE size) const noexcept;
    CSize operator-(SIZE size) const noexcept;
    CPoint operator+(POINT point) const noexcept;
    CPoint operator-(POINT point) const noexcept;
    CRect operator+(const RECT* lpRect) const noexcept;
    CRect operator-(const RECT* lpRect) const noexcept;
};

// ---------------------------------------------------------------------
// CRect (header atltypes.h, derives from tagRECT)
// ---------------------------------------------------------------------
class CRect : public tagRECT
{
public:
    CRect();
    CRect(int l, int t, int r, int b);
    CRect(const RECT& srcRect);
    CRect(LPCRECT lpSrcRect);
    CRect(POINT point, SIZE size);
    CRect(POINT topLeft, POINT bottomRight);

    // Real MFC's CRect converts implicitly to the raw pointer types, which
    // is what lets eMule pass a CRect straight into an API expecting an
    // LPRECT (`GetClientRect(rClient)`). Without these the call silently
    // falls through to the global ::GetClientRect(HWND, LPRECT) instead,
    // and fails there.
    operator LPRECT() noexcept { return this; }
    operator LPCRECT() const noexcept { return this; }

    int Height() const noexcept;
    int Width() const noexcept;
    BOOL PtInRect(POINT point) const noexcept;
    void OffsetRect(int x, int y) noexcept;
    void OffsetRect(POINT point) noexcept;
    void OffsetRect(SIZE size) noexcept;
    void InflateRect(int x, int y) noexcept;
    void InflateRect(SIZE size) noexcept;
    void InflateRect(LPCRECT lpRect) noexcept;
    void InflateRect(int l, int t, int r, int b) noexcept;
    CPoint& TopLeft() noexcept;
    const CPoint& TopLeft() const noexcept;
    void DeflateRect(int x, int y) noexcept;
    void DeflateRect(SIZE size) noexcept;
    void DeflateRect(LPCRECT lpRect) noexcept;
    void DeflateRect(int l, int t, int r, int b) noexcept;
    void SetRect(int x1, int y1, int x2, int y2) noexcept;
    CPoint CenterPoint() const noexcept;
    CSize Size() const noexcept;
    CPoint& BottomRight() noexcept;
    const CPoint& BottomRight() const noexcept;
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
};
