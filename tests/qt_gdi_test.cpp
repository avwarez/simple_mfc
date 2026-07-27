// qt_gdi_test.cpp — CDC / CPaintDC over QPainter (first GDI slice).
//
// Builds a sized dialog, opens a CPaintDC on it, exercises the colour/text/
// line/rect primitives, and inspects the resulting pixels on the driver's
// offscreen paint surface. Headless (QT_QPA_PLATFORM=offscreen).
#include "afxwin.h"
#include "driver_internal.h"

#include <QApplication>
#include <QColor>
#include <QImage>
#include <cstdio>

static int g_failures = 0;
#define CHECK(cond)                                                      \
    do {                                                                 \
        if (!(cond)) {                                                   \
            std::printf("FAIL: %s (line %d)\n", #cond, __LINE__);        \
            ++g_failures;                                                \
        }                                                                \
    } while (0)

#define IDD_SAMPLE     1000
#define DT_CALCRECT    0x0400
#define DT_SINGLELINE  0x0020

// COLORREF is 0x00BBGGRR, so RGB(r,g,b) packs r in the low byte.
static COLORREF RGBv(int r, int g, int b)
{
    return COLORREF(r) | (COLORREF(g) << 8) | (COLORREF(b) << 16);
}

class GdiDlg : public CDialog
{
public:
    GdiDlg() : CDialog(IDD_SAMPLE) {}
};

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    GdiDlg dlg;
    CHECK(dlg.Create(IDD_SAMPLE) == TRUE);
    dlg.MoveWindow(0, 0, 300, 200);   // deterministic canvas size

    // No paint surface exists until a DC is constructed for the window.
    CHECK(smfc_qt::WndSurfaceImage(&dlg) == nullptr);

    CPaintDC dc(&dlg);
    QImage* img = smfc_qt::WndSurfaceImage(&dlg);
    CHECK(img != nullptr);
    if (!img) { std::printf("no surface\n"); return 1; }
    CHECK(img->width() == 300 && img->height() == 200);

    // --- FillSolidRect -----------------------------------------------------
    const COLORREF red = RGBv(255, 0, 0);
    dc.FillSolidRect(10, 10, 40, 30, red);
    QColor c = img->pixelColor(15, 15);
    CHECK(c.red() == 255 && c.green() == 0 && c.blue() == 0);   // filled
    QColor bg = img->pixelColor(5, 5);
    CHECK(bg.red() == 255 && bg.green() == 255 && bg.blue() == 255); // untouched bg

    // --- SetPixel / GetPixel ----------------------------------------------
    const COLORREF green = RGBv(0, 255, 0);
    dc.SetPixel(60, 60, green);
    CHECK(dc.GetPixel(60, 60) == green);

    // --- MoveTo / LineTo (default black pen) ------------------------------
    dc.MoveTo(70, 10);
    CHECK(dc.LineTo(70, 50) == TRUE);
    QColor line = img->pixelColor(70, 30);
    CHECK(line.red() < 128 && line.green() < 128 && line.blue() < 128);

    // --- TextOut honours SetTextColor -------------------------------------
    dc.SetTextColor(red);
    dc.SetBkMode(1 /*TRANSPARENT*/);
    dc.TextOut(10, 80, _T("Hi"));
    bool drewText = false, redText = false;
    for (int yy = 78; yy < 104 && !drewText; ++yy)
        for (int xx = 8; xx < 60 && !drewText; ++xx) {
            const QColor t = img->pixelColor(xx, yy);
            if (t.red() != 255 || t.green() != 255 || t.blue() != 255) {
                drewText = true;
                redText = (t.red() > t.green());   // red text blends toward red
            }
        }
    CHECK(drewText);
    CHECK(redText);

    // --- DrawText DT_CALCRECT + GetTextExtent -----------------------------
    RECT rc = {0, 0, 200, 20};
    const int h = dc.DrawText(_T("Measure"), -1, &rc, DT_CALCRECT | DT_SINGLELINE);
    CHECK(h > 0);
    CHECK(rc.right > 0 && rc.bottom > 0);
    CSize sz = dc.GetTextExtent(_T("Hello"));
    CHECK(sz.cx > 0 && sz.cy > 0);

    // --- Draw3dRect (top-left vs bottom-right colours) --------------------
    const COLORREF blue = RGBv(0, 0, 255);
    dc.Draw3dRect(100, 100, 30, 20, red /*TL*/, blue /*BR*/);
    QColor topEdge = img->pixelColor(110, 100);
    QColor botEdge = img->pixelColor(110, 119);
    CHECK(topEdge.red() == 255 && topEdge.blue() == 0);    // TL red
    CHECK(botEdge.blue() == 255 && botEdge.red() == 0);    // BR blue

    // --- CPen: SelectObject gives a coloured, wider line ------------------
    CPen penRed;
    CHECK(penRed.CreatePen(0 /*PS_SOLID*/, 3, red) == TRUE);
    CHECK(penRed.m_hObject != nullptr);
    CPen* pOldPen = dc.SelectObject(&penRed);   // previous == stock (null)
    dc.MoveTo(10, 140);
    dc.LineTo(60, 140);
    QColor penLine = img->pixelColor(30, 140);
    CHECK(penLine.red() > 150 && penLine.green() < 80 && penLine.blue() < 80);
    dc.SelectObject(pOldPen);                   // restore stock pen

    // --- CBrush fill + PS_NULL pen (fill only, no outline) ----------------
    CPen penNull;
    penNull.CreatePen(5 /*PS_NULL*/, 1, RGBv(0, 0, 0));
    CBrush brGreen;
    CHECK(brGreen.CreateSolidBrush(RGBv(0, 180, 0)) == TRUE);
    dc.SelectObject(&penNull);
    dc.SelectObject(&brGreen);
    dc.Rectangle(80, 130, 140, 170);
    QColor fillc = img->pixelColor(110, 150);
    CHECK(fillc.green() > 120 && fillc.red() < 100 && fillc.blue() < 100);

    // --- CFont: selecting a bigger font grows the measured text -----------
    CSize small = dc.GetTextExtent(_T("Wg"));
    CFont big;
    CHECK(big.CreatePointFont(240, _T("Sans"), &dc) == TRUE);   // 24pt
    dc.SelectObject(&big);
    CSize large = dc.GetTextExtent(_T("Wg"));
    CHECK(large.cx > small.cx && large.cy > small.cy);

    // --- FillRect with an explicit brush ----------------------------------
    RECT fr = {150, 130, 190, 170};
    CBrush brBlue(RGBv(0, 0, 200));
    dc.FillRect(&fr, &brBlue);
    QColor frc = img->pixelColor(170, 150);
    CHECK(frc.blue() > 150 && frc.red() < 80 && frc.green() < 80);

    // --- Slice 3: CMemDC round-trip (memory DC + bitmap + BitBlt) ---------
    // Draw a magenta fill into an offscreen bitmap through a memory DC, then
    // BitBlt it onto the window surface — eMule's double-buffer pattern.
    {
        const COLORREF magenta = RGBv(200, 0, 200);
        CDC memDC;
        CHECK(memDC.CreateCompatibleDC(&dc) == TRUE);
        CBitmap bmp;
        CHECK(bmp.CreateCompatibleBitmap(&dc, 40, 40) == TRUE);
        CHECK(bmp.m_hObject != nullptr);
        CHECK(bmp.GetBitmapDimension() == CSize(40, 40));

        CBitmap* pOldBmp = memDC.SelectObject(&bmp);   // memDC now draws into bmp
        memDC.FillSolidRect(0, 0, 40, 40, magenta);
        // BitBlt the bitmap block onto the window at (200, 20).
        CHECK(dc.BitBlt(200, 20, 40, 40, &memDC, 0, 0, 0x00CC0020 /*SRCCOPY*/) == TRUE);
        QColor blt = img->pixelColor(220, 40);
        CHECK(blt.red() > 150 && blt.blue() > 150 && blt.green() < 80);   // magenta landed

        memDC.SelectObject(pOldBmp);   // flush pixels back into bmp
        memDC.DeleteDC();
        bmp.DeleteObject();
    }

    // --- Slice 4: CImageList (Add/Draw/ExtractIcon) + CDC::DrawIcon -------
    {
        const COLORREF teal = RGBv(0, 200, 200);
        // Paint a 16x16 image into a bitmap through a memory DC.
        CDC memDC;
        memDC.CreateCompatibleDC(&dc);
        CBitmap glyph;
        glyph.CreateCompatibleBitmap(&dc, 16, 16);
        CBitmap* pOld = memDC.SelectObject(&glyph);
        memDC.FillSolidRect(0, 0, 16, 16, teal);
        memDC.SelectObject(pOld);

        CImageList il;
        CHECK(il.Create(16, 16, 0, 1, 1) == TRUE);
        CHECK(il.m_hImageList != nullptr);
        const int idx = il.Add(&glyph, RGBv(255, 0, 255) /*magenta key: absent*/);
        CHECK(idx == 0);
        CHECK(il.GetImageCount() == 1);

        // Draw image 0 onto the window at (250, 100).
        POINT at = {250, 100};
        CHECK(il.Draw(&dc, 0, at, 0) == TRUE);
        QColor drawn = img->pixelColor(258, 108);
        CHECK(drawn.green() > 120 && drawn.blue() > 120 && drawn.red() < 80);

        // ExtractIcon mints a drawable HICON; DrawIcon blits it at (250, 130).
        HICON hIco = il.ExtractIcon(0);
        CHECK(hIco != nullptr);
        CHECK(dc.DrawIcon(250, 130, hIco) == TRUE);
        QColor ico = img->pixelColor(258, 138);
        CHECK(ico.green() > 120 && ico.blue() > 120 && ico.red() < 80);

        // CStatic image accessors: handle round-trip (unbound control).
        CStatic stat;
        CHECK(stat.GetBitmap() == nullptr);
        HBITMAP hbm = (HBITMAP)glyph.m_hObject;
        CHECK(stat.SetBitmap(hbm) == nullptr);   // no previous
        CHECK(stat.GetBitmap() == hbm);
        CHECK(stat.SetIcon(hIco) == nullptr);    // switching to icon clears bitmap
        CHECK(stat.GetIcon() == hIco);
        CHECK(stat.GetBitmap() == nullptr);

        il.DeleteImageList();
        memDC.DeleteDC();
        glyph.DeleteObject();
    }

    penRed.DeleteObject();
    penNull.DeleteObject();
    brGreen.DeleteObject();
    big.DeleteObject();
    brBlue.DeleteObject();

    // --- surface released with the window ---------------------------------
    dlg.DestroyWindow();
    CHECK(smfc_qt::WndSurfaceImage(&dlg) == nullptr);

    if (g_failures == 0)
        std::printf("qt_gdi_test: CDC/CPaintDC fill/pixel/line/text/3drect + "
                    "CPen/CBrush/CFont SelectObject + CBitmap/CreateCompatibleDC/"
                    "BitBlt CMemDC round-trip + CImageList/DrawIcon/CStatic OK\n");
    return g_failures == 0 ? 0 : 1;
}
