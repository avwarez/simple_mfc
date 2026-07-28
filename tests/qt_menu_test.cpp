// qt_menu_test.cpp — CMenu on the Qt driver.
//
// eMule builds every context menu through this class: 691 calls, AppendMenu
// alone 390. A Win32 menu is an object rather than a window, so this exercises
// the class directly, with no dialog needed except as a WM_COMMAND target.
//
// Same caveat as qt_dlgitem_test: these assertions encode the documented Win32
// semantics (what EnableMenuItem returns, that MF_BYCOMMAND recurses into
// submenus, that separators report id 0 and popups -1), not a real-MFC
// recording. They pin behaviour and catch regressions; they cannot catch a
// wrong reading of Win32.
//
// Runs headless (QT_QPA_PLATFORM=offscreen).
#include "afxwin.h"

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QTimer>
#include <cstdio>

static int g_failures = 0;
#define CHECK(cond)                                                      \
    do {                                                                 \
        if (!(cond)) {                                                   \
            std::printf("FAIL: %s (line %d)\n", #cond, __LINE__);        \
            ++g_failures;                                                \
        }                                                                \
    } while (0)

#define ID_ALPHA   100
#define ID_BETA    101
#define ID_GAMMA   102
#define ID_NESTED  200

static UINT g_lastCommand = 0;

// A window that just records the WM_COMMAND a menu choice delivers.
class CmdSink : public CWnd
{
public:
    BOOL OnCommand(WPARAM wParam, LPARAM) override
    {
        g_lastCommand = static_cast<UINT>(wParam & 0xFFFF);
        return TRUE;
    }
};

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    // --- creation ----------------------------------------------------------
    CMenu menu;
    CHECK(menu.m_hMenu == nullptr);
    CHECK(static_cast<HMENU>(menu) == nullptr);   // the implicit conversion
    CHECK(menu.CreatePopupMenu() == TRUE);
    CHECK(menu.m_hMenu != nullptr);
    CHECK(menu.GetMenuItemCount() == 0);

    // --- AppendMenu: strings, separator, submenu ---------------------------
    CHECK(menu.AppendMenu(MF_STRING, ID_ALPHA, _T("&Alpha")) == TRUE);
    CHECK(menu.AppendMenu(MF_STRING, ID_BETA, _T("Beta")) == TRUE);
    CHECK(menu.AppendMenu(MF_SEPARATOR) == TRUE);
    CHECK(menu.AppendMenu(MF_STRING | MF_GRAYED, ID_GAMMA, _T("Gamma")) == TRUE);
    CHECK(menu.GetMenuItemCount() == 4);

    // GetMenuItemID: real ids for string items, 0 for a separator.
    CHECK(menu.GetMenuItemID(0) == ID_ALPHA);
    CHECK(menu.GetMenuItemID(1) == ID_BETA);
    CHECK(menu.GetMenuItemID(2) == 0);
    CHECK(menu.GetMenuItemID(3) == ID_GAMMA);
    CHECK(menu.GetMenuItemID(99) == static_cast<UINT>(-1));   // out of range
    CHECK(menu.GetMenuItemID(-1) == static_cast<UINT>(-1));

    // MF_GRAYED at append time really disabled the item.
    CHECK((menu.GetMenuState(ID_GAMMA, MF_BYCOMMAND) & MF_GRAYED) != 0);
    CHECK((menu.GetMenuState(ID_ALPHA, MF_BYCOMMAND) & MF_GRAYED) == 0);
    CHECK((menu.GetMenuState(2, MF_BYPOSITION) & MF_SEPARATOR) != 0);
    CHECK(menu.GetMenuState(9999, MF_BYCOMMAND) == static_cast<UINT>(-1));

    // A submenu is appended by handle, with MF_POPUP.
    CMenu sub;
    CHECK(sub.CreatePopupMenu() == TRUE);
    CHECK(sub.AppendMenu(MF_STRING, ID_NESTED, _T("Nested")) == TRUE);
    CHECK(menu.AppendMenu(MF_POPUP, reinterpret_cast<UINT_PTR>(sub.m_hMenu),
                          _T("More")) == TRUE);
    CHECK(menu.GetMenuItemCount() == 5);
    CHECK(menu.GetMenuItemID(4) == static_cast<UINT>(-1));   // a popup, not a command
    CHECK((menu.GetMenuState(4, MF_BYPOSITION) & MF_POPUP) != 0);

    // GetSubMenu returns a usable wrapper, and the SAME one each time (callers
    // cache it).
    CMenu* got = menu.GetSubMenu(4);
    CHECK(got != nullptr);
    CHECK(menu.GetSubMenu(4) == got);
    CHECK(menu.GetSubMenu(0) == nullptr);   // a plain item is not a submenu
    CHECK(menu.GetSubMenu(99) == nullptr);
    if (got) {
        CHECK(got->GetMenuItemCount() == 1);
        CHECK(got->GetMenuItemID(0) == ID_NESTED);
    }

    // --- EnableMenuItem ----------------------------------------------------
    // Returns the PREVIOUS state, which is what callers save and restore.
    CHECK(menu.EnableMenuItem(ID_ALPHA, MF_BYCOMMAND | MF_GRAYED) == MF_ENABLED);
    CHECK((menu.GetMenuState(ID_ALPHA, MF_BYCOMMAND) & MF_GRAYED) != 0);
    CHECK(menu.EnableMenuItem(ID_ALPHA, MF_BYCOMMAND | MF_ENABLED) == MF_GRAYED);
    CHECK((menu.GetMenuState(ID_ALPHA, MF_BYCOMMAND) & MF_GRAYED) == 0);

    // By position addresses this menu's own items.
    CHECK(menu.EnableMenuItem(1, MF_BYPOSITION | MF_GRAYED) == MF_ENABLED);
    CHECK((menu.GetMenuState(ID_BETA, MF_BYCOMMAND) & MF_GRAYED) != 0);

    // An id that matches nothing yields Win32's -1 rather than a false success.
    CHECK(menu.EnableMenuItem(9999, MF_BYCOMMAND | MF_ENABLED) == static_cast<UINT>(-1));
    CHECK(menu.EnableMenuItem(99, MF_BYPOSITION | MF_ENABLED) == static_cast<UINT>(-1));

    // MF_BYCOMMAND searches submenus too: this is why eMule can disable a
    // nested item through the root menu.
    CHECK(menu.EnableMenuItem(ID_NESTED, MF_BYCOMMAND | MF_GRAYED) == MF_ENABLED);
    CHECK((menu.GetMenuState(ID_NESTED, MF_BYCOMMAND) & MF_GRAYED) != 0);
    // ...whereas MF_BYPOSITION does not leave this menu.
    CHECK(menu.EnableMenuItem(ID_NESTED, MF_BYPOSITION | MF_ENABLED)
          == static_cast<UINT>(-1));

    // --- CheckMenuItem -----------------------------------------------------
    CHECK(menu.CheckMenuItem(ID_ALPHA, MF_BYCOMMAND | MF_CHECKED) == MF_UNCHECKED);
    CHECK((menu.GetMenuState(ID_ALPHA, MF_BYCOMMAND) & MF_CHECKED) != 0);
    CHECK(menu.CheckMenuItem(ID_ALPHA, MF_BYCOMMAND | MF_UNCHECKED) == MF_CHECKED);
    CHECK((menu.GetMenuState(ID_ALPHA, MF_BYCOMMAND) & MF_CHECKED) == 0);
    CHECK(menu.CheckMenuItem(9999, MF_BYCOMMAND | MF_CHECKED) == static_cast<UINT>(-1));

    // --- CheckMenuRadioItem ------------------------------------------------
    CHECK(menu.CheckMenuRadioItem(ID_ALPHA, ID_GAMMA, ID_BETA, MF_BYCOMMAND) == TRUE);
    CHECK((menu.GetMenuState(ID_BETA, MF_BYCOMMAND) & MF_CHECKED) != 0);
    CHECK((menu.GetMenuState(ID_ALPHA, MF_BYCOMMAND) & MF_CHECKED) == 0);
    CHECK((menu.GetMenuState(ID_GAMMA, MF_BYCOMMAND) & MF_CHECKED) == 0);
    // Moving the tick clears the previous one.
    CHECK(menu.CheckMenuRadioItem(ID_ALPHA, ID_GAMMA, ID_GAMMA, MF_BYCOMMAND) == TRUE);
    CHECK((menu.GetMenuState(ID_GAMMA, MF_BYCOMMAND) & MF_CHECKED) != 0);
    CHECK((menu.GetMenuState(ID_BETA, MF_BYCOMMAND) & MF_CHECKED) == 0);
    // A range matching nothing fails rather than reporting success.
    CHECK(menu.CheckMenuRadioItem(9000, 9001, 9000, MF_BYCOMMAND) == FALSE);

    // --- SetDefaultItem ----------------------------------------------------
    CHECK(menu.SetDefaultItem(ID_ALPHA, FALSE) == TRUE);
    CHECK(menu.SetDefaultItem(0, TRUE) == TRUE);
    CHECK(menu.SetDefaultItem(9999, FALSE) == FALSE);

    // --- InsertMenu / ModifyMenu / RemoveMenu ------------------------------
    {
        CMenu edit;
        CHECK(edit.CreatePopupMenu() == TRUE);
        edit.AppendMenu(MF_STRING, 1, _T("one"));
        edit.AppendMenu(MF_STRING, 3, _T("three"));

        // InsertMenu puts the new item BEFORE the identified one.
        CHECK(edit.InsertMenu(3, MF_BYCOMMAND | MF_STRING, 2, _T("two")) == TRUE);
        CHECK(edit.GetMenuItemCount() == 3);
        CHECK(edit.GetMenuItemID(0) == 1);
        CHECK(edit.GetMenuItemID(1) == 2);
        CHECK(edit.GetMenuItemID(2) == 3);

        // By position, before index 0 means "at the front".
        CHECK(edit.InsertMenu(0, MF_BYPOSITION | MF_STRING, 0, _T("zero")) == TRUE);
        CHECK(edit.GetMenuItemID(0) == 0);
        CHECK(edit.GetMenuItemCount() == 4);

        // ModifyMenu replaces the item in place: same slot, new id and state.
        CHECK(edit.ModifyMenu(2, MF_BYCOMMAND | MF_STRING | MF_GRAYED, 22,
                              _T("twenty-two")) == TRUE);
        CHECK(edit.GetMenuItemCount() == 4);
        CHECK(edit.GetMenuItemID(2) == 22);
        CHECK((edit.GetMenuState(22, MF_BYCOMMAND) & MF_GRAYED) != 0);
        CHECK(edit.GetMenuState(2, MF_BYCOMMAND) == static_cast<UINT>(-1)); // old id gone
        CHECK(edit.ModifyMenu(9999, MF_BYCOMMAND | MF_STRING, 5, _T("no")) == FALSE);

        // RemoveMenu drops one item.
        CHECK(edit.RemoveMenu(22, MF_BYCOMMAND) == TRUE);
        CHECK(edit.GetMenuItemCount() == 3);
        CHECK(edit.GetMenuState(22, MF_BYCOMMAND) == static_cast<UINT>(-1));
        CHECK(edit.RemoveMenu(9999, MF_BYCOMMAND) == FALSE);
        CHECK(edit.DestroyMenu() == TRUE);
    }

    // --- handle lifetime ---------------------------------------------------
    {
        CMenu owned;
        CHECK(owned.CreatePopupMenu() == TRUE);
        CHECK(owned.DestroyMenu() == TRUE);
        CHECK(owned.m_hMenu == nullptr);
        CHECK(owned.DestroyMenu() == FALSE);   // already gone

        // Real MFC has NO ownership notion: DestroyMenu frees whatever handle
        // the wrapper holds, so a second CMenu that merely attached to the
        // handle destroys the menu just the same.
        CMenu maker;
        CHECK(maker.CreatePopupMenu() == TRUE);
        CMenu borrower;
        CHECK(borrower.Attach(maker.m_hMenu) == TRUE);
        CHECK(borrower.DestroyMenu() == TRUE);
        // The first wrapper is now holding a dead handle. Win32 leaves a second
        // destroy undefined; here it is a clean FALSE rather than a crash.
        CHECK(maker.DestroyMenu() == FALSE);

        CMenu empty;
        CHECK(empty.Attach(nullptr) == FALSE);
        CHECK(empty.Detach() == nullptr);
        CHECK(empty.GetMenuItemCount() == static_cast<UINT>(-1));
        CHECK(empty.AppendMenu(MF_STRING, 1, _T("x")) == FALSE);
    }

    // Detach hands the handle over without freeing it.
    {
        CMenu m;
        CHECK(m.CreatePopupMenu() == TRUE);
        HMENU h = m.Detach();
        CHECK(h != nullptr);
        CHECK(m.m_hMenu == nullptr);
        CMenu again;
        CHECK(again.Attach(h) == TRUE);
        CHECK(again.GetMenuItemCount() == 0);   // the menu survived the handover
        delete reinterpret_cast<QMenu*>(h);
    }

    // --- TrackPopupMenu ----------------------------------------------------
    // Modal, like Win32: it runs until an item is chosen or the menu closes.
    // Headless there is no user, so the choice is made from a queued callback.
    {
        CMenu pop;
        CHECK(pop.CreatePopupMenu() == TRUE);
        pop.AppendMenu(MF_STRING, ID_ALPHA, _T("Alpha"));
        pop.AppendMenu(MF_STRING, ID_BETA, _T("Beta"));

        CmdSink sink;
        QMenu* qm = reinterpret_cast<QMenu*>(pop.m_hMenu);

        // Choosing an item delivers WM_COMMAND to the owner window.
        g_lastCommand = 0;
        QTimer::singleShot(0, qm, [qm] { qm->actions().at(1)->trigger(); qm->close(); });
        CHECK(pop.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, 10, 10, &sink) == TRUE);
        CHECK(g_lastCommand == ID_BETA);

        // Dismissing without choosing sends nothing.
        g_lastCommand = 0;
        QTimer::singleShot(0, qm, [qm] { qm->close(); });
        CHECK(pop.TrackPopupMenu(TPM_LEFTALIGN, 10, 10, &sink) == TRUE);
        CHECK(g_lastCommand == 0);

        // TPM_RETURNCMD returns the id instead of posting it.
        g_lastCommand = 0;
        QTimer::singleShot(0, qm, [qm] { qm->actions().at(0)->trigger(); qm->close(); });
        CHECK(pop.TrackPopupMenu(TPM_RETURNCMD, 10, 10, &sink)
              == static_cast<BOOL>(ID_ALPHA));
        CHECK(g_lastCommand == 0);   // not delivered as a command

        // An item chosen in a SUBMENU still reaches the owner window: Win32
        // delivers the command however deeply the item is nested.
        CMenu nested;
        CHECK(nested.CreatePopupMenu() == TRUE);
        CHECK(nested.AppendMenu(MF_STRING, ID_NESTED, _T("Deep")) == TRUE);
        CHECK(pop.AppendMenu(MF_POPUP, reinterpret_cast<UINT_PTR>(nested.m_hMenu),
                             _T("More")) == TRUE);
        QMenu* qnested = reinterpret_cast<QMenu*>(nested.m_hMenu);
        g_lastCommand = 0;
        QTimer::singleShot(0, qm, [qm, qnested] {
            qnested->actions().at(0)->trigger();
            qm->close();
        });
        CHECK(pop.TrackPopupMenu(TPM_LEFTALIGN, 10, 10, &sink) == TRUE);
        CHECK(g_lastCommand == ID_NESTED);

        CHECK(pop.DestroyMenu() == TRUE);
    }

    CHECK(menu.DestroyMenu() == TRUE);

    if (g_failures == 0)
        std::printf("qt_menu_test: build, submenus, item state, edit ops, "
                    "handle lifetime and popup tracking OK\n");
    return g_failures == 0 ? 0 : 1;
}
