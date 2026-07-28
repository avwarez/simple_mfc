// gui/qt/menu.cpp — CMenu on the Qt driver, backed by QMenu.
//
// eMule builds every one of its context menus through this class: 691 calls,
// AppendMenu alone 390. A Win32 menu is an object rather than a window, which
// is what makes this class self-contained -- it needs nothing from CWnd except
// a target window to send WM_COMMAND to when an item is chosen.
//
// HANDLE MODEL: m_hMenu carries the QMenu*, exactly as it carries the HMENU on
// Windows, so eMule's `if (menu)` validity tests and its habit of passing the
// handle around keep working unchanged.
//
// LIFETIME: every driver-side map here is keyed by the QMenu* (the handle),
// never by the CMenu* wrapper. CMenu has no destructor in the frozen interface,
// so a wrapper can go out of scope without telling us; a CMenu*-keyed map would
// then be inherited by the next CMenu allocated at the same address. Keying by
// handle avoids that: entries are erased when the QMenu itself is destroyed.
#include "afxwin.h"
#include "smfc_qt.h"
#include "driver_internal.h"

#include <QAction>
#include <QMenu>
#include <QPoint>
#include <QString>
#include <QVariant>
#include <QWidget>

#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

inline QMenu* AsQMenu(HMENU h) { return reinterpret_cast<QMenu*>(h); }
inline HMENU  AsHMenu(QMenu* m) { return reinterpret_cast<HMENU>(m); }

// Every menu this library created and has not destroyed yet.
//
// This is NOT an ownership model: real MFC has none. CMenu::DestroyMenu calls
// ::DestroyMenu(m_hMenu), which frees the menu whatever wrapper happens to hold
// the handle -- so Attach followed by DestroyMenu really does destroy it, and
// two CMenu objects sharing a handle can both try. Win32 leaves that second
// attempt undefined; this set makes it a clean FALSE instead of a crash, which
// is the one place the portable build is deliberately safer than Windows.
std::unordered_set<QMenu*>& LiveMenus()
{
    static std::unordered_set<QMenu*> s;
    return s;
}

// One stable CMenu wrapper per submenu QMenu, so repeated GetSubMenu(nPos)
// calls return the same pointer (callers cache it).
std::unordered_map<QMenu*, std::unique_ptr<CMenu>>& SubWrappers()
{
    static std::unordered_map<QMenu*, std::unique_ptr<CMenu>> m;
    return m;
}

void ForgetMenu(QMenu* m)
{
    LiveMenus().erase(m);
    SubWrappers().erase(m);
}

// Track a QMenu we just created, and make sure our maps drop it if anything
// else deletes it first (Qt parent ownership, say).
void TrackLive(QMenu* m)
{
    LiveMenus().insert(m);
    QObject::connect(m, &QObject::destroyed, [m] { ForgetMenu(m); });
}

QString ToQ(LPCTSTR s) { return s ? QString::fromWCharArray(s) : QString(); }

// The command id carried by an item, or 0 for separators and submenus (which
// is what Win32's GetMenuItemID reports for a separator too).
bool ActionId(const QAction* a, UINT& out)
{
    if (!a || a->isSeparator() || a->menu()) return false;
    const QVariant v = a->data();
    if (!v.isValid()) return false;
    out = v.toUInt();
    return true;
}

// Win32's item lookup. MF_BYPOSITION indexes this menu's own items; the
// default, MF_BYCOMMAND, searches by command id and RECURSES into submenus --
// which is why eMule can enable an item of a nested popup through the root.
QAction* FindAction(QMenu* m, UINT item, UINT flags)
{
    if (!m) return nullptr;
    const QList<QAction*> acts = m->actions();
    if (flags & MF_BYPOSITION)
        return (item < static_cast<UINT>(acts.size()))
                   ? acts[static_cast<int>(item)] : nullptr;

    for (QAction* a : acts) {
        UINT id = 0;
        if (ActionId(a, id) && id == item)
            return a;
        if (a->menu())
            if (QAction* found = FindAction(a->menu(), item, flags))
                return found;
    }
    return nullptr;
}

// Build one item from Win32 menu flags and splice it in before `before`
// (null == append at the end).
BOOL InsertItem(QMenu* m, QAction* before, UINT nFlags, UINT_PTR nIDNewItem,
                LPCTSTR lpszNewItem)
{
    if (!m) return FALSE;

    QAction* a = nullptr;
    if (nFlags & MF_SEPARATOR) {
        a = new QAction(m);
        a->setSeparator(true);
    } else if (nFlags & MF_POPUP) {
        // For a popup item, nIDNewItem is the submenu's HMENU, not a command
        // id. Its menuAction() is the QAction that represents it in the parent.
        QMenu* sub = AsQMenu(reinterpret_cast<HMENU>(nIDNewItem));
        if (!sub) return FALSE;
        a = sub->menuAction();
        a->setText(ToQ(lpszNewItem));
    } else {
        // MF_STRING is zero, so "a string item" is the absence of the others.
        // The text may embed '\t' before accelerator text (Win32's convention);
        // it is kept verbatim -- Qt renders shortcuts its own way, and matching
        // Windows' menu appearance is explicitly not a goal of this driver.
        a = new QAction(ToQ(lpszNewItem), m);
        a->setData(QVariant(static_cast<uint>(nIDNewItem)));
    }

    if (nFlags & (MF_GRAYED | MF_DISABLED))
        a->setEnabled(false);
    if (nFlags & MF_CHECKED) {
        a->setCheckable(true);
        a->setChecked(true);
    }

    m->insertAction(before, a);
    if (nFlags & MF_DEFAULT)
        m->setDefaultAction(a);
    return TRUE;
}

} // namespace

// --- creation / lifetime ---------------------------------------------------
BOOL CMenu::CreatePopupMenu()
{
    QMenu* m = new QMenu();
    TrackLive(m);
    m_hMenu = AsHMenu(m);
    return TRUE;
}

// Win32 distinguishes a menu BAR (CreateMenu) from a popup; Qt's QMenu covers
// both, and the distinction only becomes observable once a bar is attached to a
// frame window -- which this driver does not do yet. Same object either way.
BOOL CMenu::CreateMenu() { return CreatePopupMenu(); }

BOOL CMenu::Attach(HMENU hMenu)
{
    if (hMenu == nullptr) return FALSE;
    m_hMenu = hMenu;
    return TRUE;
}

HMENU CMenu::Detach()
{
    HMENU h = m_hMenu;
    m_hMenu = nullptr;
    return h;
}

BOOL CMenu::DestroyMenu()
{
    QMenu* m = AsQMenu(m_hMenu);
    m_hMenu = nullptr;
    if (!m) return FALSE;
    // As in real MFC, the wrapper's provenance is irrelevant: whoever holds the
    // handle can destroy it. Only an already-destroyed (or never-created) menu
    // is refused -- see LiveMenus above.
    if (LiveMenus().count(m) == 0) return FALSE;
    ForgetMenu(m);
    delete m;   // Qt deletes the QActions parented to it, and the submenus
    return TRUE;
}

// --- building --------------------------------------------------------------
BOOL CMenu::AppendMenu(UINT nFlags, UINT_PTR nIDNewItem, LPCTSTR lpszNewItem)
{
    return InsertItem(AsQMenu(m_hMenu), nullptr, nFlags, nIDNewItem, lpszNewItem);
}

// The CBitmap overload: a bitmap item needs the image put on the QAction as an
// icon, which the CImageList/CBitmap bridge does not expose yet. Refused rather
// than silently appending a blank item, so a caller sees the gap.
BOOL CMenu::AppendMenu(UINT /*nFlags*/, UINT_PTR /*nIDNewItem*/, const CBitmap* /*pBmp*/)
{
    return FALSE;
}

BOOL CMenu::InsertMenu(UINT nPosition, UINT nFlags, UINT_PTR nIDNewItem,
                       LPCTSTR lpszNewItem)
{
    QMenu* m = AsQMenu(m_hMenu);
    if (!m) return FALSE;
    // Win32: the new item goes BEFORE the identified one; an id/position that
    // matches nothing appends at the end.
    QAction* before = FindAction(m, nPosition, nFlags);
    return InsertItem(m, before, nFlags, nIDNewItem, lpszNewItem);
}

BOOL CMenu::ModifyMenu(UINT nPosition, UINT nFlags, UINT_PTR nIDNewItem,
                       LPCTSTR lpszNewItem)
{
    QMenu* m = AsQMenu(m_hMenu);
    if (!m) return FALSE;
    QAction* old = FindAction(m, nPosition, nFlags);
    if (!old) return FALSE;
    // Win32 replaces the item wholesale (type, id and text all come from the
    // new flags), so build a fresh one in place and drop the old.
    if (!InsertItem(m, old, nFlags, nIDNewItem, lpszNewItem)) return FALSE;
    m->removeAction(old);
    return TRUE;
}

BOOL CMenu::RemoveMenu(UINT nPosition, UINT nFlags)
{
    QMenu* m = AsQMenu(m_hMenu);
    if (!m) return FALSE;
    QAction* a = FindAction(m, nPosition, nFlags);
    if (!a) return FALSE;
    // RemoveMenu (unlike DeleteMenu) leaves a submenu itself alive; removing
    // its action from the parent is exactly that.
    m->removeAction(a);
    return TRUE;
}

// --- item state ------------------------------------------------------------
UINT CMenu::EnableMenuItem(UINT nIDEnableItem, UINT nEnable)
{
    QAction* a = FindAction(AsQMenu(m_hMenu), nIDEnableItem, nEnable);
    if (!a) return static_cast<UINT>(-1);   // Win32's "no such item"
    const UINT prev = a->isEnabled() ? MF_ENABLED : MF_GRAYED;
    a->setEnabled((nEnable & (MF_GRAYED | MF_DISABLED)) == 0);
    return prev;
}

UINT CMenu::CheckMenuItem(UINT nIDCheckItem, UINT nCheck)
{
    QAction* a = FindAction(AsQMenu(m_hMenu), nIDCheckItem, nCheck);
    if (!a) return static_cast<UINT>(-1);
    const UINT prev = a->isChecked() ? MF_CHECKED : MF_UNCHECKED;
    a->setCheckable(true);
    a->setChecked((nCheck & MF_CHECKED) != 0);
    return prev;
}

BOOL CMenu::CheckMenuRadioItem(UINT nIDFirst, UINT nIDLast, UINT nIDItem, UINT nFlags)
{
    QMenu* m = AsQMenu(m_hMenu);
    if (!m) return FALSE;
    // Win32 ticks exactly one item of the range and clears the rest. By
    // position the range is an index span; by command it is an id span.
    bool any = false;
    for (UINT i = nIDFirst; i <= nIDLast; ++i) {
        QAction* a = FindAction(m, i, nFlags);
        if (!a) continue;
        any = true;
        a->setCheckable(true);
        a->setChecked(i == nIDItem);
    }
    return any ? TRUE : FALSE;
}

BOOL CMenu::SetDefaultItem(UINT uItem, BOOL fByPos)
{
    QMenu* m = AsQMenu(m_hMenu);
    if (!m) return FALSE;
    QAction* a = FindAction(m, uItem, fByPos ? MF_BYPOSITION : MF_BYCOMMAND);
    if (!a) return FALSE;
    m->setDefaultAction(a);
    return TRUE;
}

// --- queries ---------------------------------------------------------------
UINT CMenu::GetMenuItemCount() const
{
    QMenu* m = AsQMenu(m_hMenu);
    return m ? static_cast<UINT>(m->actions().size()) : static_cast<UINT>(-1);
}

UINT CMenu::GetMenuItemID(int nPos) const
{
    QMenu* m = AsQMenu(m_hMenu);
    if (!m || nPos < 0) return static_cast<UINT>(-1);
    const QList<QAction*> acts = m->actions();
    if (nPos >= acts.size()) return static_cast<UINT>(-1);
    QAction* a = acts[nPos];
    if (a->isSeparator()) return 0;                     // Win32: separators are 0
    if (a->menu()) return static_cast<UINT>(-1);        // ...and popups are -1
    UINT id = 0;
    return ActionId(a, id) ? id : 0;
}

CMenu* CMenu::GetSubMenu(int nPos) const
{
    QMenu* m = AsQMenu(m_hMenu);
    if (!m || nPos < 0) return nullptr;
    const QList<QAction*> acts = m->actions();
    if (nPos >= acts.size()) return nullptr;
    QMenu* sub = acts[nPos]->menu();
    if (!sub) return nullptr;

    // One wrapper per submenu, created on demand and kept alive for as long as
    // the submenu is, so a caller may cache the CMenu* it gets back.
    auto& slot = SubWrappers()[sub];
    if (!slot) {
        slot = std::make_unique<CMenu>();
        slot->Attach(AsHMenu(sub));
        QObject::connect(sub, &QObject::destroyed, [sub] { SubWrappers().erase(sub); });
    }
    return slot.get();
}

UINT CMenu::GetMenuState(UINT nID, UINT nFlags) const
{
    QAction* a = FindAction(AsQMenu(m_hMenu), nID, nFlags);
    if (!a) return static_cast<UINT>(-1);
    UINT state = 0;
    if (!a->isEnabled()) state |= MF_GRAYED;
    if (a->isChecked())  state |= MF_CHECKED;
    if (a->isSeparator()) state |= MF_SEPARATOR;
    if (a->menu())        state |= MF_POPUP;
    return state;
}

// --- tracking --------------------------------------------------------------
BOOL CMenu::TrackPopupMenu(UINT nFlags, int x, int y, CWnd* pWnd, LPCRECT /*lpRect*/)
{
    QMenu* m = AsQMenu(m_hMenu);
    if (!m) return FALSE;

    // Win32's TrackPopupMenu is MODAL: it runs its own message loop and returns
    // once the user picks an item or dismisses the menu. QMenu::exec has the
    // same contract.
    //
    // The choice is captured by hooking every command item in the whole menu
    // TREE, rather than by taking exec()'s return value or QMenu::triggered.
    // Both of those only report what the menu itself activated: an item chosen
    // in a submenu, or triggered programmatically, does not reach them. Win32
    // delivers the command however deeply the item is nested, so the hook has
    // to sit on the actions.
    QAction* chosen = nullptr;
    std::vector<QMetaObject::Connection> conns;
    std::function<void(QMenu*)> hook = [&](QMenu* menu) {
        for (QAction* a : menu->actions()) {
            if (a->menu()) { hook(a->menu()); continue; }
            if (a->isSeparator()) continue;
            conns.push_back(QObject::connect(a, &QAction::triggered,
                                             [&chosen, a] { chosen = a; }));
        }
    };
    hook(m);

    m->exec(QPoint(x, y));

    for (const QMetaObject::Connection& c : conns)
        QObject::disconnect(c);

    UINT id = 0;
    const bool haveId = ActionId(chosen, id);

    if (nFlags & TPM_RETURNCMD)
        return static_cast<BOOL>(haveId ? id : 0);   // the id IS the return value

    // Otherwise the choice is delivered as a WM_COMMAND to the owner window,
    // which is how eMule's ON_COMMAND handlers see it.
    if (haveId && pWnd)
        pWnd->SendMessage(WM_COMMAND, static_cast<WPARAM>(id), 0);
    return TRUE;
}

// --- not implemented, deliberately -----------------------------------------
// MENUITEMINFO is an incomplete type off Windows (afxwin.h forward-declares it
// so the tag merges with winuser.h's on Windows), so these three cannot even
// read their argument in the portable build. eMule uses them 6 times in total.
// Returning FALSE is honest; inventing a local struct layout would make the
// portable build disagree with Windows on the one thing that matters here.
BOOL CMenu::InsertMenuItem(UINT, LPMENUITEMINFO, BOOL) { return FALSE; }
BOOL CMenu::GetMenuItemInfo(UINT, LPMENUITEMINFO, BOOL) const { return FALSE; }
BOOL CMenu::SetMenuItemInfo(UINT, LPMENUITEMINFO, BOOL) { return FALSE; }
BOOL CMenu::GetMenuInfo(LPMENUINFO) const { return FALSE; }
BOOL CMenu::SetMenuInfo(LPCMENUINFO) { return FALSE; }

// Menu resources are not carried by the .rc compiler yet (it emits dialog
// templates only), so there is nothing to load from.
BOOL CMenu::LoadMenu(LPCTSTR) { return FALSE; }
BOOL CMenu::LoadMenu(UINT) { return FALSE; }

// Owner-draw measurement: the framework calls this, and eMule's CTitledMenu
// super-calls it for the default behaviour. Real MFC's default leaves the
// struct untouched, and so does this.
void CMenu::MeasureItem(LPMEASUREITEMSTRUCT) {}
