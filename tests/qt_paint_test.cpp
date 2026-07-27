// qt_paint_test.cpp — the paintEvent -> WM_PAINT -> OnPaint route (owner-draw
// payoff, half A). A CDialog subclass paints in OnPaint via a live CPaintDC;
// the host widget's paintEvent must dispatch WM_PAINT (reaching the OnPaint
// override through the virtual), and blit the CDC surface onto the widget.
// Headless (QT_QPA_PLATFORM=offscreen); the paint is forced via QWidget::render.
#include "afxwin.h"
#include "driver_internal.h"

#include <QApplication>
#include <QColor>
#include <QImage>
#include <QWidget>
#include <cstdio>

static int g_failures = 0;
#define CHECK(cond)                                                      \
    do {                                                                 \
        if (!(cond)) {                                                   \
            std::printf("FAIL: %s (line %d)\n", #cond, __LINE__);        \
            ++g_failures;                                                \
        }                                                                \
    } while (0)

#define IDD_SAMPLE 1000

// COLORREF is 0x00BBGGRR.
static COLORREF RGBv(int r, int g, int b)
{
    return COLORREF(r) | (COLORREF(g) << 8) | (COLORREF(b) << 16);
}

class PaintDlg : public CDialog
{
public:
    PaintDlg() : CDialog(IDD_SAMPLE) {}
    int paints = 0;
    void OnPaint() override
    {
        ++paints;
        CPaintDC dc(this);                              // live DC on this window
        dc.FillSolidRect(0,  0, 60, 40, RGBv(255, 0, 0));   // red   band (left)
        dc.FillSolidRect(60, 0, 60, 40, RGBv(0, 0, 255));   // blue  band (right)
    }
};

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    PaintDlg dlg;
    CHECK(dlg.Create(IDD_SAMPLE) == TRUE);
    dlg.MoveWindow(0, 0, 200, 120);

    QWidget* w = smfc_qt::WidgetOf(&dlg);
    CHECK(w != nullptr);
    if (!w) { std::printf("no widget\n"); return 1; }

    // Force a repaint: this drives paintEvent -> WM_PAINT -> OnPaint -> blit.
    QImage shot(w->size(), QImage::Format_ARGB32);
    shot.fill(Qt::black);
    w->render(&shot);

    // 1) OnPaint was reached through the WM_PAINT route (virtual dispatch).
    CHECK(dlg.paints > 0);

    // 2) OnPaint drew through a live CDC onto the window's offscreen surface.
    QImage* surf = smfc_qt::WndSurfaceImage(&dlg);
    CHECK(surf != nullptr);
    if (surf) {
        QColor l = surf->pixelColor(20, 20);
        QColor r = surf->pixelColor(90, 20);
        CHECK(l.red() == 255 && l.green() == 0 && l.blue() == 0);   // red band
        CHECK(r.blue() == 255 && r.red() == 0 && r.green() == 0);   // blue band
    }

    // 3) The blit carried those pixels onto the real widget. Child controls
    //    occupy the top band, so scan for the presence of saturated red/blue
    //    (which no plain control produces) rather than fixed coordinates.
    int reds = 0, blues = 0;
    for (int y = 0; y < 40; ++y)
        for (int x = 0; x < 120; ++x) {
            const QColor c = shot.pixelColor(x, y);
            if (c.red() > 200 && c.green() < 60 && c.blue() < 60) ++reds;
            if (c.blue() > 200 && c.red() < 60 && c.green() < 60) ++blues;
        }
    CHECK(reds > 0);
    CHECK(blues > 0);

    dlg.DestroyWindow();

    if (g_failures == 0)
        std::printf("qt_paint_test: paintEvent -> WM_PAINT -> OnPaint (live "
                    "CPaintDC) -> blit-to-widget OK\n");
    return g_failures == 0 ? 0 : 1;
}
