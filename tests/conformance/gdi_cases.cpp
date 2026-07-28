// gdi_cases.cpp — GDI conformance cases, compiled into two probes exactly
// like cases.cpp, and printing the same "<case name>\t<value>" records so
// tests/conformance/golden.cmake compares them unchanged.
//
//   simple_mfc_gdi_probe (-DSIMPLE_MFC_USE_NATIVE)   CDC over Qt (POSIX)
//   real_mfc_gdi_probe   (-DSIMPLE_MFC_USE_REAL_MFC) CDC over Win32 GDI
//
// GOLDEN-ONLY BY CONSTRUCTION. cases.cpp can run both probes side by side on
// Windows because it needs nothing but the core library. This one cannot:
// simple_mfc's CDC is implemented in gui/qt, so the only build that has it is
// the Qt one, and the only machine that has real MFC is Windows. The two
// therefore never exist at the same time and the comparison has to go through
// the recording — which is exactly what the golden mechanism is for.
//
// WHAT IS DELIBERATELY NOT TESTED HERE, and why. This suite is worth only as
// much as its honesty about the boundary:
//
//   * TEXT RENDERING. TextOut/DrawText rasterize glyphs through completely
//     different font engines, against different installed fonts. Comparing
//     those pixels compares font stacks, not MFC. GetTextExtent is out for
//     the same reason: its result is a property of the chosen face.
//   * ANYTHING ANTIALIASED OR DITHERED. Only solid fills, axis-aligned edges
//     and single-pixel probes appear below. No ellipses, no diagonal lines,
//     no hatch brushes: their rasterization is an implementation choice that
//     Win32 and Qt are each entitled to make differently.
//   * DrawFocusRect. It XORs a dotted pattern whose phase is unspecified.
//
// What IS compared is the part where a divergence would be a real bug: the
// DC's state bookkeeping (every setter returning the PREVIOUS value, which is
// the MFC idiom callers save and restore through), the exclusive/inclusive
// edge semantics of the rect and line primitives — the classic Win32 trap —
// and the resulting solid pixels.

#if defined(SIMPLE_MFC_USE_NATIVE)
    #include "afxwin.h"
#elif defined(SIMPLE_MFC_USE_REAL_MFC)
    #include <afxwin.h>
#else
    #error "Define either SIMPLE_MFC_USE_NATIVE or SIMPLE_MFC_USE_REAL_MFC"
#endif

#ifdef _WIN32
    #include <windows.h>
    #include <crtdbg.h>
#endif

#include <cstdio>
#include <string>

// Win32 constants used below. Spelled out rather than relied upon from a
// header, so both branches provably use the same numbers.
#ifndef SRCCOPY
    #define SRCCOPY 0x00CC0020
#endif
#ifndef PS_SOLID
    #define PS_SOLID 0
#endif
#ifndef TRANSPARENT
    #define TRANSPARENT 1
#endif
#ifndef OPAQUE
    #define OPAQUE 2
#endif
#ifndef TA_LEFT
    #define TA_LEFT   0
    #define TA_RIGHT  2
    #define TA_CENTER 6
#endif
// GetPixel's failure value. Win32 spells it CLR_INVALID.
#ifndef CLR_INVALID
    #define CLR_INVALID 0xFFFFFFFF
#endif

namespace
{

// Nothing in this harness may ever wait for a human. A probe that stops on a
// modal dialog does not fail -- it HANGS, and the recording step hangs with
// it until the runner's multi-hour limit. This suite drives real MFC's GDI
// wrappers, whose ASSERT macros feed the debug CRT's message box, so it is
// not hypothetical. Same three sources as cases.cpp's SilenceWindowsDialogs.
void SilenceWindowsDialogs()
{
#ifdef _WIN32
    for (int report : {_CRT_WARN, _CRT_ERROR, _CRT_ASSERT})
    {
        _CrtSetReportMode(report, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(report, _CRTDBG_FILE_STDERR);
    }
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
#endif
}

// ---------------------------------------------------------------------
// Canonical output. Identical contract to cases.cpp: one record per line,
// "<name>\t<value>\n", names unique, values ASCII only.
// ---------------------------------------------------------------------
void Line(const char* name, const std::string& value)
{
    std::printf("%s\t%s\n", name, value.c_str());
}

void LineInt(const char* name, long long v)
{
    std::printf("%s\t%lld\n", name, v);
}

void LineBool(const char* name, bool v)
{
    std::printf("%s\t%s\n", name, v ? "TRUE" : "FALSE");
}

// COLORREF is 0x00BBGGRR. Printed as fixed 8 hex digits so CLR_INVALID
// (0xFFFFFFFF) is distinguishable from an ordinary colour rather than being
// silently truncated to 0x00FFFFFF, i.e. white.
void LineColor(const char* name, COLORREF c)
{
    std::printf("%s\t%08lX\n", name, static_cast<unsigned long>(c));
}

void LinePoint(const char* name, POINT p)
{
    std::printf("%s\t%ld,%ld\n", name, static_cast<long>(p.x), static_cast<long>(p.y));
}

COLORREF Rgb(int r, int g, int b)
{
    return static_cast<COLORREF>(r)
         | (static_cast<COLORREF>(g) << 8)
         | (static_cast<COLORREF>(b) << 16);
}

constexpr int kW = 64;
constexpr int kH = 48;

// A memory DC with a colour bitmap selected into it, filled with a known
// background so nothing below ever reads undefined bits.
//
// The two branches differ HERE, and only here. Win32's CreateCompatibleBitmap
// takes its colour depth from the reference DC, and a fresh memory DC still
// holds the default 1x1 MONOCHROME bitmap -- so passing the memory DC to
// itself would yield a 1bpp surface and turn every colour case below into
// black-and-white noise. The screen DC is the standard way out. simple_mfc's
// memory DCs are 32-bit from the start and have no such notion, so there is
// nothing to work around. What gets compared is what is DRAWN, not how the
// surface was obtained.
struct Canvas
{
    CDC      dc;
    CBitmap  bmp;
    CBitmap* pOldBmp = nullptr;

    bool Init()
    {
        if (!dc.CreateCompatibleDC(nullptr))
            return false;
#ifdef _WIN32
        HDC  hScreen = ::GetDC(nullptr);
        CDC* pRef    = CDC::FromHandle(hScreen);
        const bool ok = bmp.CreateCompatibleBitmap(pRef, kW, kH) != FALSE;
        ::ReleaseDC(nullptr, hScreen);
        if (!ok)
            return false;
#else
        if (!bmp.CreateCompatibleBitmap(&dc, kW, kH))
            return false;
#endif
        pOldBmp = dc.SelectObject(&bmp);
        dc.FillSolidRect(0, 0, kW, kH, Rgb(0, 0, 0));
        return true;
    }

    ~Canvas()
    {
        if (pOldBmp)
            dc.SelectObject(pOldBmp);
    }
};

// ---------------------------------------------------------------------
// DC state. Every one of these setters returns the PREVIOUS value -- the
// property the save/restore idiom rests on, and the one most likely to be
// quietly wrong in a reimplementation that only ever gets read back through
// its own getter.
// ---------------------------------------------------------------------
void TestDcState()
{
    Canvas cv;
    LineBool("Gdi.Canvas.Init", cv.Init());
    CDC& dc = cv.dc;

    const COLORREF c1 = Rgb(10, 20, 30);
    const COLORREF c2 = Rgb(200, 100, 50);

    LineColor("Gdi.SetTextColor.first_returns_previous", dc.SetTextColor(c1));
    LineColor("Gdi.SetTextColor.second_returns_first", dc.SetTextColor(c2));
    LineColor("Gdi.GetTextColor.after", dc.GetTextColor());

    LineColor("Gdi.SetBkColor.first_returns_previous", dc.SetBkColor(c1));
    LineColor("Gdi.SetBkColor.second_returns_first", dc.SetBkColor(c2));
    LineColor("Gdi.GetBkColor.after", dc.GetBkColor());

    LineInt("Gdi.SetBkMode.returns_previous", dc.SetBkMode(TRANSPARENT));
    LineInt("Gdi.SetBkMode.returns_transparent", dc.SetBkMode(OPAQUE));

    LineInt("Gdi.SetTextAlign.returns_previous", static_cast<long long>(dc.SetTextAlign(TA_RIGHT)));
    LineInt("Gdi.SetTextAlign.returns_right", static_cast<long long>(dc.SetTextAlign(TA_CENTER)));
    LineInt("Gdi.GetTextAlign.after", static_cast<long long>(dc.GetTextAlign()));
}

// ---------------------------------------------------------------------
// SetPixel / GetPixel, including the out-of-bounds contract.
// ---------------------------------------------------------------------
void TestPixels()
{
    Canvas cv;
    if (!cv.Init()) { Line("Gdi.Pixels.SKIPPED", "canvas init failed"); return; }
    CDC& dc = cv.dc;

    const COLORREF c = Rgb(1, 2, 3);
    LineColor("Gdi.SetPixel.returns_previous", dc.SetPixel(5, 5, c));
    LineColor("Gdi.GetPixel.after_SetPixel", dc.GetPixel(5, 5));
    LineColor("Gdi.GetPixel.neighbour_untouched", dc.GetPixel(6, 5));

    // Reading outside the surface. Win32 answers CLR_INVALID rather than
    // failing; a reimplementation that returns black here would be wrong in
    // a way no round-trip test could see.
    LineColor("Gdi.GetPixel.out_of_range_negative", dc.GetPixel(-1, -1));
    LineColor("Gdi.GetPixel.out_of_range_beyond", dc.GetPixel(kW, kH));
    LineColor("Gdi.GetPixel.last_valid", dc.GetPixel(kW - 1, kH - 1));
}

// ---------------------------------------------------------------------
// FillSolidRect. The whole question is which edges are painted: Win32 rects
// are right/bottom EXCLUSIVE, and getting that wrong is an off-by-one that
// only shows at the boundary.
// ---------------------------------------------------------------------
void TestFillSolidRect()
{
    Canvas cv;
    if (!cv.Init()) { Line("Gdi.FillSolidRect.SKIPPED", "canvas init failed"); return; }
    CDC& dc = cv.dc;

    const COLORREF fill = Rgb(255, 0, 0);
    dc.FillSolidRect(10, 10, 20, 15, fill);      // x, y, cx, cy

    LineColor("Gdi.FillSolidRect.inside_topleft", dc.GetPixel(10, 10));
    LineColor("Gdi.FillSolidRect.inside_middle", dc.GetPixel(19, 17));
    LineColor("Gdi.FillSolidRect.inside_bottomright", dc.GetPixel(29, 24));
    LineColor("Gdi.FillSolidRect.just_outside_right", dc.GetPixel(30, 17));
    LineColor("Gdi.FillSolidRect.just_outside_bottom", dc.GetPixel(19, 25));
    LineColor("Gdi.FillSolidRect.just_outside_left", dc.GetPixel(9, 17));
    LineColor("Gdi.FillSolidRect.just_outside_top", dc.GetPixel(19, 9));

    // The LPCRECT overload must agree with the x/y/cx/cy one.
    Canvas cv2;
    if (!cv2.Init()) { Line("Gdi.FillSolidRect.Rect.SKIPPED", "canvas init failed"); return; }
    RECT r{ 10, 10, 30, 25 };                    // same area, as a RECT
    cv2.dc.FillSolidRect(&r, fill);
    LineColor("Gdi.FillSolidRect.Rect.inside_bottomright", cv2.dc.GetPixel(29, 24));
    LineColor("Gdi.FillSolidRect.Rect.just_outside_right", cv2.dc.GetPixel(30, 17));
    LineColor("Gdi.FillSolidRect.Rect.just_outside_bottom", cv2.dc.GetPixel(19, 25));

    // Degenerate rects: nothing may be painted.
    Canvas cv3;
    if (!cv3.Init()) { Line("Gdi.FillSolidRect.Empty.SKIPPED", "canvas init failed"); return; }
    cv3.dc.FillSolidRect(5, 5, 0, 10, fill);
    LineColor("Gdi.FillSolidRect.zero_width_paints_nothing", cv3.dc.GetPixel(5, 5));
    cv3.dc.FillSolidRect(5, 5, 10, 0, fill);
    LineColor("Gdi.FillSolidRect.zero_height_paints_nothing", cv3.dc.GetPixel(5, 5));
}

// ---------------------------------------------------------------------
// FillRect through a CBrush, and FrameRect's one-pixel border.
// ---------------------------------------------------------------------
void TestBrushRects()
{
    Canvas cv;
    if (!cv.Init()) { Line("Gdi.BrushRects.SKIPPED", "canvas init failed"); return; }
    CDC& dc = cv.dc;

    CBrush brush;
    LineBool("Gdi.CreateSolidBrush.ok", brush.CreateSolidBrush(Rgb(0, 200, 0)) != FALSE);

    RECT r{ 8, 8, 24, 20 };
    dc.FillRect(&r, &brush);
    LineColor("Gdi.FillRect.inside_topleft", dc.GetPixel(8, 8));
    LineColor("Gdi.FillRect.inside_bottomright", dc.GetPixel(23, 19));
    LineColor("Gdi.FillRect.just_outside_right", dc.GetPixel(24, 12));
    LineColor("Gdi.FillRect.just_outside_bottom", dc.GetPixel(12, 20));

    // FrameRect paints the border only, leaving the interior alone.
    Canvas cv2;
    if (!cv2.Init()) { Line("Gdi.FrameRect.SKIPPED", "canvas init failed"); return; }
    CBrush frameBrush;
    frameBrush.CreateSolidBrush(Rgb(0, 0, 255));
    RECT fr{ 8, 8, 24, 20 };
    cv2.dc.FrameRect(&fr, &frameBrush);
    LineColor("Gdi.FrameRect.top_edge", cv2.dc.GetPixel(12, 8));
    LineColor("Gdi.FrameRect.left_edge", cv2.dc.GetPixel(8, 12));
    LineColor("Gdi.FrameRect.interior_untouched", cv2.dc.GetPixel(12, 12));
}

// ---------------------------------------------------------------------
// Rectangle(): outlined with the current pen, filled with the current brush,
// and -- the classic -- the right/bottom coordinates behave differently from
// a fill rect.
// ---------------------------------------------------------------------
void TestRectanglePrimitive()
{
    Canvas cv;
    if (!cv.Init()) { Line("Gdi.Rectangle.SKIPPED", "canvas init failed"); return; }
    CDC& dc = cv.dc;

    CPen pen;
    LineBool("Gdi.CreatePen.ok", pen.CreatePen(PS_SOLID, 1, Rgb(255, 255, 0)) != FALSE);
    CBrush brush;
    brush.CreateSolidBrush(Rgb(0, 0, 255));

    CPen*   pOldPen   = dc.SelectObject(&pen);
    CBrush* pOldBrush = dc.SelectObject(&brush);
    LineBool("Gdi.SelectObject.pen_returned_non_null", pOldPen != nullptr);
    LineBool("Gdi.SelectObject.brush_returned_non_null", pOldBrush != nullptr);

    LineBool("Gdi.Rectangle.returns_true", dc.Rectangle(10, 10, 30, 25) != FALSE);
    LineColor("Gdi.Rectangle.border_topleft", dc.GetPixel(10, 10));
    LineColor("Gdi.Rectangle.border_top_mid", dc.GetPixel(20, 10));
    LineColor("Gdi.Rectangle.border_left_mid", dc.GetPixel(10, 17));
    LineColor("Gdi.Rectangle.interior", dc.GetPixel(20, 17));
    // In Win32 the bottom-right corner is exclusive for Rectangle too: the
    // border sits at x2-1 / y2-1, not at x2 / y2.
    LineColor("Gdi.Rectangle.border_right_at_x2_minus_1", dc.GetPixel(29, 17));
    LineColor("Gdi.Rectangle.at_x2_is_outside", dc.GetPixel(30, 17));
    LineColor("Gdi.Rectangle.border_bottom_at_y2_minus_1", dc.GetPixel(20, 24));
    LineColor("Gdi.Rectangle.at_y2_is_outside", dc.GetPixel(20, 25));

    dc.SelectObject(pOldBrush);
    dc.SelectObject(pOldPen);
}

// ---------------------------------------------------------------------
// MoveTo / LineTo: the return values, and LineTo's exclusive endpoint.
// Only axis-aligned lines -- a diagonal is a rasterizer's own business.
// ---------------------------------------------------------------------
void TestLines()
{
    Canvas cv;
    if (!cv.Init()) { Line("Gdi.Lines.SKIPPED", "canvas init failed"); return; }
    CDC& dc = cv.dc;

    CPen pen;
    pen.CreatePen(PS_SOLID, 1, Rgb(255, 0, 255));
    CPen* pOldPen = dc.SelectObject(&pen);

    // MoveTo returns the PREVIOUS current position.
    LinePoint("Gdi.MoveTo.first_returns_previous", dc.MoveTo(5, 5));
    LinePoint("Gdi.MoveTo.second_returns_first", dc.MoveTo(10, 20));

    LineBool("Gdi.LineTo.returns_true", dc.LineTo(30, 20) != FALSE);
    LinePoint("Gdi.MoveTo.after_LineTo_returns_endpoint", dc.MoveTo(0, 0));

    LineColor("Gdi.LineTo.horizontal.start", dc.GetPixel(10, 20));
    LineColor("Gdi.LineTo.horizontal.middle", dc.GetPixel(20, 20));
    // LineTo does not paint its endpoint.
    LineColor("Gdi.LineTo.horizontal.endpoint_excluded", dc.GetPixel(30, 20));
    LineColor("Gdi.LineTo.horizontal.last_painted", dc.GetPixel(29, 20));
    LineColor("Gdi.LineTo.horizontal.off_line", dc.GetPixel(20, 21));

    dc.MoveTo(40, 5);
    dc.LineTo(40, 25);
    LineColor("Gdi.LineTo.vertical.start", dc.GetPixel(40, 5));
    LineColor("Gdi.LineTo.vertical.middle", dc.GetPixel(40, 15));
    LineColor("Gdi.LineTo.vertical.endpoint_excluded", dc.GetPixel(40, 25));
    LineColor("Gdi.LineTo.vertical.last_painted", dc.GetPixel(40, 24));

    dc.SelectObject(pOldPen);
}

// ---------------------------------------------------------------------
// Viewport / window origin: the return values, and that a shifted origin
// actually moves what is drawn.
// ---------------------------------------------------------------------
void TestOrigins()
{
    Canvas cv;
    if (!cv.Init()) { Line("Gdi.Origins.SKIPPED", "canvas init failed"); return; }
    CDC& dc = cv.dc;

    LinePoint("Gdi.SetViewportOrg.first_returns_previous", dc.SetViewportOrg(5, 7));
    LinePoint("Gdi.SetViewportOrg.second_returns_first", dc.SetViewportOrg(0, 0));
    LinePoint("Gdi.SetWindowOrg.first_returns_previous", dc.SetWindowOrg(3, 4));
    LinePoint("Gdi.SetWindowOrg.second_returns_first", dc.SetWindowOrg(0, 0));

    // With the viewport shifted, a fill at logical (0,0) lands at device
    // (8,6). Probed through GetPixel, which is itself in logical units, so
    // the shift has to cancel out.
    dc.SetViewportOrg(8, 6);
    dc.FillSolidRect(0, 0, 4, 4, Rgb(0, 255, 255));
    LineColor("Gdi.SetViewportOrg.shifted_fill_at_logical_origin", dc.GetPixel(0, 0));
    LineColor("Gdi.SetViewportOrg.shifted_fill_before_origin", dc.GetPixel(-1, -1));
    dc.SetViewportOrg(0, 0);
    LineColor("Gdi.SetViewportOrg.restored_sees_device_offset", dc.GetPixel(8, 6));
    LineColor("Gdi.SetViewportOrg.restored_device_origin_clear", dc.GetPixel(0, 0));
}

// ---------------------------------------------------------------------
// BitBlt SRCCOPY between two memory DCs.
// ---------------------------------------------------------------------
void TestBitBlt()
{
    Canvas dst, src;
    if (!dst.Init() || !src.Init()) { Line("Gdi.BitBlt.SKIPPED", "canvas init failed"); return; }

    src.dc.FillSolidRect(0, 0, kW, kH, Rgb(0, 0, 0));
    src.dc.FillSolidRect(0, 0, 10, 10, Rgb(255, 128, 64));

    LineBool("Gdi.BitBlt.returns_true",
             dst.dc.BitBlt(20, 20, 10, 10, &src.dc, 0, 0, SRCCOPY) != FALSE);
    LineColor("Gdi.BitBlt.copied_topleft", dst.dc.GetPixel(20, 20));
    LineColor("Gdi.BitBlt.copied_bottomright", dst.dc.GetPixel(29, 29));
    LineColor("Gdi.BitBlt.just_outside", dst.dc.GetPixel(30, 20));
    LineColor("Gdi.BitBlt.source_unchanged", src.dc.GetPixel(5, 5));

    // Copying from an offset inside the source.
    LineBool("Gdi.BitBlt.offset.returns_true",
             dst.dc.BitBlt(40, 20, 4, 4, &src.dc, 8, 8, SRCCOPY) != FALSE);
    LineColor("Gdi.BitBlt.offset.inside_source_patch", dst.dc.GetPixel(40, 20));
    LineColor("Gdi.BitBlt.offset.outside_source_patch", dst.dc.GetPixel(42, 22));
}

// ---------------------------------------------------------------------
// Draw3dRect: two solid two-pixel-wide L shapes, fully deterministic.
// ---------------------------------------------------------------------
void TestDraw3dRect()
{
    Canvas cv;
    if (!cv.Init()) { Line("Gdi.Draw3dRect.SKIPPED", "canvas init failed"); return; }
    CDC& dc = cv.dc;

    const COLORREF tl = Rgb(255, 255, 255);
    const COLORREF br = Rgb(128, 128, 128);
    dc.Draw3dRect(10, 10, 20, 15, tl, br);

    LineColor("Gdi.Draw3dRect.top_edge", dc.GetPixel(15, 10));
    LineColor("Gdi.Draw3dRect.left_edge", dc.GetPixel(10, 15));
    LineColor("Gdi.Draw3dRect.bottom_edge", dc.GetPixel(15, 24));
    LineColor("Gdi.Draw3dRect.right_edge", dc.GetPixel(29, 15));
    LineColor("Gdi.Draw3dRect.interior_untouched", dc.GetPixel(15, 15));
}

} // namespace

int main()
{
    SilenceWindowsDialogs();

    TestDcState();
    TestPixels();
    TestFillSolidRect();
    TestBrushRects();
    TestRectanglePrimitive();
    TestLines();
    TestOrigins();
    TestBitBlt();
    TestDraw3dRect();

    // Terminator, mirroring cases.cpp: proves the probe reached the end
    // rather than dying quietly partway through.
    std::printf("#END\t0\n");
    std::fflush(stdout);
    return 0;
}
