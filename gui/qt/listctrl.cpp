// gui/qt/listctrl.cpp — CListCtrl (report-mode data) + the owner-draw delegate
// that reaches an LVS_OWNERDRAWFIXED list's DrawItem override (owner-draw
// payoff, half B).
//
// eMule's lists derive from CMuleListCtrl : CListCtrl and override the virtual
// DrawItem(LPDRAWITEMSTRUCT) to paint each row themselves. Real Windows drives
// that via WM_DRAWITEM sent to the parent and reflected to the control; here the
// Qt item delegate plays that role: for each visible row it fabricates a
// DRAWITEMSTRUCT (itemID = row, rcItem = item-local rect, hDC = a reusable item
// surface, itemState from the selection) and calls the control's DrawItem, then
// blits what was drawn onto the viewport. Virtual dispatch reaches the derived
// override (DrawItem is now a real virtual on CListCtrl).
//
// Only the minimal data surface an owner-draw list needs is implemented here
// (InsertItem/GetItemCount/Set|GetItemData/DeleteAllItems/SetItemText); the rest
// of CListCtrl's large API is added as eMule needs it.
#include "afxwin.h"
#include "afxcmn.h"
#include "driver_internal.h"

#include <QImage>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QString>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <unordered_map>
#include <vector>

namespace {

// Owner-draw item states / actions / control type (real wingdi/winuser values).
constexpr UINT kOdtListview = 0x000A;   // ODT_LISTVIEW
constexpr UINT kOdaDrawEntire = 0x0001; // ODA_DRAWENTIRE
constexpr UINT kOdsSelected = 0x0001;   // ODS_SELECTED

struct ListRow {
    QString   text;
    DWORD_PTR data = 0;
};
struct ListModel {
    std::vector<ListRow> rows;
};

std::unordered_map<const CListCtrl*, ListModel>& Models()
{
    static std::unordered_map<const CListCtrl*, ListModel> m;
    return m;
}
ListModel& model(const CListCtrl* p) { return Models()[p]; }
ListModel* modelIfAny(const CListCtrl* p)
{
    auto it = Models().find(p);
    return it == Models().end() ? nullptr : &it->second;
}

QTreeWidget* treeOf(const CListCtrl* p)
{
    return qobject_cast<QTreeWidget*>(smfc_qt::WidgetOf(p));
}

// The delegate that turns a Qt row-paint into an MFC DrawItem call.
class SmfcOwnerDrawDelegate : public QStyledItemDelegate {
public:
    explicit SmfcOwnerDrawDelegate(CListCtrl* list, QObject* parent = nullptr)
        : QStyledItemDelegate(parent), m_list(list) {}

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override
    {
        if (!m_list || !index.isValid()) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }
        const int w = option.rect.width();
        const int h = option.rect.height();
        HDC hdc = smfc_qt::BeginItemSurface(w, h);

        tagDRAWITEMSTRUCT dis{};                     // POSIX shim completes this
        dis.CtlType    = kOdtListview;
        dis.CtlID      = 0;
        dis.itemID     = static_cast<UINT>(index.row());
        dis.itemAction = kOdaDrawEntire;
        dis.itemState  = (option.state & QStyle::State_Selected) ? kOdsSelected : 0;
        dis.hDC        = hdc;
        dis.rcItem.left = 0;  dis.rcItem.top = 0;
        dis.rcItem.right = w; dis.rcItem.bottom = h;
        if (ListModel* m = modelIfAny(m_list))
            if (index.row() >= 0 && index.row() < int(m->rows.size()))
                dis.itemData = m->rows[index.row()].data;

        m_list->DrawItem(&dis);                     // -> derived override

        if (QImage* img = smfc_qt::ItemSurfaceImage())
            painter->drawImage(option.rect.topLeft(), *img);
    }

private:
    CListCtrl* m_list;
};

} // namespace

namespace smfc_qt {
void EnableOwnerDraw(CListCtrl* list)
{
    QTreeWidget* tree = treeOf(list);
    if (!tree) return;
    // Parented to the tree so Qt owns the delegate's lifetime.
    tree->setItemDelegate(new SmfcOwnerDrawDelegate(list, tree));
    tree->setUniformRowHeights(true);
}
} // namespace smfc_qt

// ---------------------------------------------------------------------------
// CListCtrl — owner-draw entry point (base no-op) + the minimal data surface.
// DrawItem is CListCtrl's first virtual, so defining it here anchors the class
// vtable in this translation unit.
// ---------------------------------------------------------------------------
void CListCtrl::DrawItem(LPDRAWITEMSTRUCT) {}

int CListCtrl::InsertItem(int nItem, LPCTSTR lpszItem)
{
    ListModel& m = model(this);
    if (nItem < 0) nItem = 0;
    if (nItem > int(m.rows.size())) nItem = int(m.rows.size());
    ListRow row;
    row.text = lpszItem ? QString::fromWCharArray(lpszItem) : QString();
    m.rows.insert(m.rows.begin() + nItem, row);

    // Mirror into the bound QTreeWidget so Qt lays out a row and the delegate
    // is invoked for it (owner-draw hides the text, but the item must exist).
    if (QTreeWidget* tree = treeOf(this)) {
        if (tree->columnCount() < 1) tree->setColumnCount(1);
        auto* it = new QTreeWidgetItem();
        it->setText(0, row.text);
        tree->insertTopLevelItem(nItem, it);
    }
    return nItem;
}

int CListCtrl::GetItemCount() const
{
    const ListModel* m = modelIfAny(this);
    return m ? int(m->rows.size()) : 0;
}

BOOL CListCtrl::SetItemData(int nItem, DWORD_PTR dwData)
{
    ListModel* m = modelIfAny(this);
    if (!m || nItem < 0 || nItem >= int(m->rows.size())) return FALSE;
    m->rows[nItem].data = dwData;
    return TRUE;
}
DWORD_PTR CListCtrl::GetItemData(int nItem) const
{
    const ListModel* m = modelIfAny(this);
    if (!m || nItem < 0 || nItem >= int(m->rows.size())) return 0;
    return m->rows[nItem].data;
}

BOOL CListCtrl::SetItemText(int nItem, int nSubItem, LPCTSTR lpszText)
{
    ListModel* m = modelIfAny(this);
    if (!m || nItem < 0 || nItem >= int(m->rows.size())) return FALSE;
    if (nSubItem == 0) {   // sub-item (column) storage is a later concern
        m->rows[nItem].text = lpszText ? QString::fromWCharArray(lpszText) : QString();
        if (QTreeWidget* tree = treeOf(this))
            if (auto* it = tree->topLevelItem(nItem)) it->setText(0, m->rows[nItem].text);
    }
    return TRUE;
}

BOOL CListCtrl::DeleteAllItems()
{
    if (ListModel* m = modelIfAny(this)) m->rows.clear();
    if (QTreeWidget* tree = treeOf(this)) tree->clear();
    return TRUE;
}
