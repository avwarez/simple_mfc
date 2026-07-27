// qt_ownerdraw_test.cpp — the DrawItem owner-draw path (owner-draw payoff, half
// B). A CListCtrl subclass overrides DrawItem (as eMule's CMuleListCtrl lists
// do); the driver's item delegate must call that override once per row with a
// correctly-populated DRAWITEMSTRUCT (itemID, rcItem, a live hDC), and what it
// paints must appear on the list. Headless (QT_QPA_PLATFORM=offscreen).
#include "afxwin.h"
#include "afxcmn.h"
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

#define IDD_SAMPLE     1000
#define IDC_FILE_LIST  1006

static COLORREF RGBv(int r, int g, int b)   // COLORREF is 0x00BBGGRR
{
    return COLORREF(r) | (COLORREF(g) << 8) | (COLORREF(b) << 16);
}

// An owner-draw list: even rows painted red, odd rows blue, keyed off itemID.
class MyList : public CListCtrl
{
public:
    int  drawn = 0;
    UINT maxId = 0;
    void DrawItem(LPDRAWITEMSTRUCT dis) override
    {
        ++drawn;
        if (dis->itemID > maxId) maxId = dis->itemID;
        CDC* pDC = CDC::FromHandle(dis->hDC);
        CHECK(pDC != nullptr);
        if (!pDC) return;
        const COLORREF c = (dis->itemID % 2) ? RGBv(0, 0, 255) : RGBv(255, 0, 0);
        pDC->FillSolidRect(&dis->rcItem, c);
    }
};

class ListDlg : public CDialog
{
public:
    MyList m_list;
    ListDlg() : CDialog(IDD_SAMPLE) {}
    void DoDataExchange(CDataExchange* pDX) override
    {
        DDX_Control(pDX, IDC_FILE_LIST, m_list);
    }
};

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    ListDlg dlg;
    CHECK(dlg.Create(IDD_SAMPLE) == TRUE);   // binds m_list to the SysListView32

    // Populate + owner-draw enable.
    CHECK(dlg.m_list.InsertItem(0, _T("row0")) == 0);
    CHECK(dlg.m_list.InsertItem(1, _T("row1")) == 1);
    CHECK(dlg.m_list.InsertItem(2, _T("row2")) == 2);
    CHECK(dlg.m_list.GetItemCount() == 3);
    CHECK(dlg.m_list.SetItemData(1, 0xABCD) == TRUE);
    CHECK(dlg.m_list.GetItemData(1) == DWORD_PTR(0xABCD));

    smfc_qt::EnableOwnerDraw(&dlg.m_list);

    QWidget* lw = smfc_qt::WidgetOf(&dlg.m_list);
    CHECK(lw != nullptr);
    if (!lw) { std::printf("no list widget\n"); return 1; }
    lw->setGeometry(0, 0, 220, 140);

    // Force the list to paint its rows -> the delegate -> DrawItem per row.
    QImage shot(lw->size(), QImage::Format_ARGB32);
    shot.fill(Qt::black);
    lw->render(&shot);

    // 1) DrawItem was reached for each row (virtual dispatch to the override).
    CHECK(dlg.m_list.drawn >= 3);
    CHECK(dlg.m_list.maxId == 2);

    // 2) What DrawItem painted reached the list: even rows red, odd blue.
    int reds = 0, blues = 0;
    for (int y = 0; y < shot.height(); ++y)
        for (int x = 0; x < shot.width(); ++x) {
            const QColor c = shot.pixelColor(x, y);
            if (c.red() > 200 && c.green() < 60 && c.blue() < 60) ++reds;
            if (c.blue() > 200 && c.red() < 60 && c.green() < 60) ++blues;
        }
    CHECK(reds > 0);    // rows 0, 2
    CHECK(blues > 0);   // row 1

    dlg.DestroyWindow();

    if (g_failures == 0)
        std::printf("qt_ownerdraw_test: CListCtrl::DrawItem override reached via "
                    "delegate (DRAWITEMSTRUCT + live CDC) -> painted rows OK\n");
    return g_failures == 0 ? 0 : 1;
}
