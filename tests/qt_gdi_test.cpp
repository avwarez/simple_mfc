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

    // --- surface released with the window ---------------------------------
    dlg.DestroyWindow();
    CHECK(smfc_qt::WndSurfaceImage(&dlg) == nullptr);

    if (g_failures == 0)
        std::printf("qt_gdi_test: CDC/CPaintDC fill/pixel/line/text/3drect OK\n");
    return g_failures == 0 ? 0 : 1;
}
