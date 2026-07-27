// gui/qt/imagelist.cpp — CImageList + the driver's icon registry (GDI slice 4).
//
// CImageList is eMule's store of same-sized images (toolbar glyphs, list/tree
// icons). It backs onto a std::vector<QImage> per list, held in the ImageLists()
// map keyed by the CImageList* (m_hImageList also holds that address as the
// non-null "created" token eMule tests with `piml->m_hImageList == NULL`).
// Keying by the object pointer bounds the leak the frozen interface's missing
// ~CImageList would cause (same rationale as the GdiObjs()/memory-DC keying);
// DeleteImageList frees eagerly.
//
// Icons have no Win32 producer in the portable build yet (no LoadIcon/CWinApp
// bootstrap), so a drawable HICON is minted from pixels via RegisterIcon (e.g.
// CImageList::ExtractIcon) and resolved back by IconImage — that is what gives
// CDC::DrawIcon / CStatic::SetIcon something to paint.
#include "afxwin.h"
#include "driver_internal.h"

#include <QColor>
#include <QImage>

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace {

constexpr COLORREF kClrNone = 0xFFFFFFFF;   // CLR_NONE (no transparency colour)

QColor toQColor(COLORREF cr)
{
    return QColor(int(cr & 0xFF), int((cr >> 8) & 0xFF), int((cr >> 16) & 0xFF));
}

// A driver-side image list: fixed cell size + the images, in index order.
struct ImgList {
    int    cx = 0, cy = 0;
    COLORREF bk = kClrNone;
    std::vector<QImage> images;
};

std::unordered_map<const CImageList*, ImgList>& ImageLists()
{
    static std::unordered_map<const CImageList*, ImgList> m;
    return m;
}
ImgList* ilOf(const CImageList* p)
{
    if (!p || !p->m_hImageList) return nullptr;
    auto it = ImageLists().find(p);
    return it == ImageLists().end() ? nullptr : &it->second;
}
ImgList& makeIl(CImageList* p)
{
    p->m_hImageList = reinterpret_cast<HIMAGELIST>(p);   // non-null created token
    ImgList& il = ImageLists()[p];
    il = ImgList{};
    return il;
}

// Apply a colour-key: every pixel equal to crMask becomes transparent.
QImage keyOut(QImage img, COLORREF crMask)
{
    if (crMask == kClrNone) return img;
    img = img.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    const QColor key = toQColor(crMask);
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x)
            if (img.pixelColor(x, y).rgb() == key.rgb())
                img.setPixelColor(x, y, Qt::transparent);
    return img;
}

// Apply a 1bpp AND-mask bitmap: where the mask is set (white), the image pixel
// is transparent — the classic ImageList image+mask pairing.
QImage applyMask(QImage img, const QImage* mask)
{
    if (!mask || mask->isNull()) return img;
    img = img.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    for (int y = 0; y < img.height() && y < mask->height(); ++y)
        for (int x = 0; x < img.width() && x < mask->width(); ++x)
            if (mask->pixelColor(x, y).lightness() > 127)   // set/white = transparent
                img.setPixelColor(x, y, Qt::transparent);
    return img;
}

} // namespace

// --- icon registry ----------------------------------------------------------
namespace smfc_qt {

std::unordered_map<HICON, QImage>& Icons()
{
    static std::unordered_map<HICON, QImage> m;
    return m;
}
HICON RegisterIcon(const QImage& img)
{
    // A monotonic counter is a unique key within Icons(); no heap token to leak.
    static std::uintptr_t next = 1;
    HICON tok = reinterpret_cast<HICON>(next++);
    Icons()[tok] = img;
    return tok;
}
const QImage* IconImage(HICON hIcon)
{
    auto it = Icons().find(hIcon);
    return it == Icons().end() ? nullptr : &it->second;
}

} // namespace smfc_qt

// ---------------------------------------------------------------------------
// CImageList
// ---------------------------------------------------------------------------
BOOL CImageList::Create(int cx, int cy, UINT /*nFlags*/, int /*nInitial*/, int /*nGrow*/)
{
    ImgList& il = makeIl(this);
    il.cx = cx;
    il.cy = cy;
    return TRUE;
}
BOOL CImageList::Create(UINT, int cx, int, COLORREF)
{
    // Loads a strip bitmap from resources and slices it into cx-wide images; the
    // portable resource compiler carries no bitmap bytes yet, so there is nothing
    // to slice. Create the (empty) list so later Add()s still work.
    ImgList& il = makeIl(this);
    il.cx = cx;
    return FALSE;
}
BOOL CImageList::Create(LPCTSTR, int cx, int, COLORREF)
{
    ImgList& il = makeIl(this);
    il.cx = cx;
    return FALSE;
}
BOOL CImageList::Create(CImageList&, int, CImageList&, int, int, int)
{
    makeIl(this);
    return FALSE;   // image-merge: a later concern
}
BOOL CImageList::Create(CImageList* pImageList)
{
    ImgList& il = makeIl(this);
    if (const ImgList* src = ilOf(pImageList)) il = *src;   // copy
    return TRUE;
}

int CImageList::Add(CBitmap* pbmImage, CBitmap* pbmMask)
{
    ImgList* il = ilOf(this);
    const QImage* img = pbmImage ? smfc_qt::BitmapImageFromHandle(*pbmImage) : nullptr;
    if (!il || !img) return -1;
    const QImage* mask = pbmMask ? smfc_qt::BitmapImageFromHandle(*pbmMask) : nullptr;
    il->images.push_back(applyMask(*img, mask));
    return static_cast<int>(il->images.size()) - 1;
}
int CImageList::Add(CBitmap* pbmImage, COLORREF crMask)
{
    ImgList* il = ilOf(this);
    const QImage* img = pbmImage ? smfc_qt::BitmapImageFromHandle(*pbmImage) : nullptr;
    if (!il || !img) return -1;
    il->images.push_back(keyOut(*img, crMask));
    return static_cast<int>(il->images.size()) - 1;
}
int CImageList::Add(HICON hIcon)
{
    ImgList* il = ilOf(this);
    if (!il) return -1;
    const QImage* img = smfc_qt::IconImage(hIcon);
    // Unknown icon (no producer): add a transparent cell so indices stay valid.
    QImage cell = img ? *img
                      : QImage(std::max(1, il->cx), std::max(1, il->cy),
                               QImage::Format_ARGB32_Premultiplied);
    if (!img) cell.fill(Qt::transparent);
    il->images.push_back(cell);
    return static_cast<int>(il->images.size()) - 1;
}

BOOL CImageList::Replace(int nImage, CBitmap* pbmImage, CBitmap* pbmMask)
{
    ImgList* il = ilOf(this);
    const QImage* img = pbmImage ? smfc_qt::BitmapImageFromHandle(*pbmImage) : nullptr;
    if (!il || !img || nImage < 0 || nImage >= int(il->images.size())) return FALSE;
    const QImage* mask = pbmMask ? smfc_qt::BitmapImageFromHandle(*pbmMask) : nullptr;
    il->images[nImage] = applyMask(*img, mask);
    return TRUE;
}
int CImageList::Replace(int nImage, HICON hIcon)
{
    ImgList* il = ilOf(this);
    const QImage* img = smfc_qt::IconImage(hIcon);
    if (!il || !img || nImage < 0 || nImage >= int(il->images.size())) return -1;
    il->images[nImage] = *img;
    return nImage;
}

BOOL CImageList::Draw(CDC* pDC, int nImage, POINT pt, UINT /*nStyle*/)
{
    ImgList* il = ilOf(this);
    if (!il || nImage < 0 || nImage >= int(il->images.size())) return FALSE;
    return smfc_qt::BlitImageToDC(pDC, pt.x, pt.y, il->images[nImage]) ? TRUE : FALSE;
}
BOOL CImageList::DrawEx(CDC* pDC, int nImage, POINT pt, SIZE sz, COLORREF,
                        COLORREF, UINT /*nStyle*/)
{
    ImgList* il = ilOf(this);
    if (!il || nImage < 0 || nImage >= int(il->images.size())) return FALSE;
    return smfc_qt::BlitImageToDC(pDC, pt.x, pt.y, il->images[nImage], sz.cx, sz.cy)
               ? TRUE : FALSE;
}

int  CImageList::GetImageCount() const { const ImgList* il = ilOf(this); return il ? int(il->images.size()) : 0; }
BOOL CImageList::Remove(int nImage)
{
    ImgList* il = ilOf(this);
    if (!il) return FALSE;
    if (nImage < 0) { il->images.clear(); return TRUE; }   // -1 removes all
    if (nImage >= int(il->images.size())) return FALSE;
    il->images.erase(il->images.begin() + nImage);
    return TRUE;
}
COLORREF CImageList::SetBkColor(COLORREF cr) { ImgList* il = ilOf(this); if (!il) return kClrNone; COLORREF p = il->bk; il->bk = cr; return p; }
COLORREF CImageList::GetBkColor() const { const ImgList* il = ilOf(this); return il ? il->bk : kClrNone; }

HICON CImageList::ExtractIcon(int nImage)
{
    const ImgList* il = ilOf(this);
    if (!il || nImage < 0 || nImage >= int(il->images.size())) return nullptr;
    return smfc_qt::RegisterIcon(il->images[nImage]);
}

CImageList::~CImageList()
{
    // Real MFC's ~CImageList destroys the underlying image list; the images are
    // held in a map keyed on this object's address, so they have to go with it.
    DeleteImageList();
}

BOOL CImageList::DeleteImageList()
{
    ImageLists().erase(this);
    m_hImageList = nullptr;
    return TRUE;
}
HIMAGELIST CImageList::GetSafeHandle() const { return m_hImageList; }
BOOL       CImageList::Attach(HIMAGELIST hImageList) { m_hImageList = hImageList; return TRUE; }
HIMAGELIST CImageList::Detach() { HIMAGELIST h = m_hImageList; m_hImageList = nullptr; return h; }
CImageList* CImageList::FromHandle(HIMAGELIST hImageList)
{
    // Every live handle is a CImageList* (its own address, see makeIl), so the
    // handle round-trips straight back to the object.
    return reinterpret_cast<CImageList*>(hImageList);
}

BOOL CImageList::SetOverlayImage(int, int) { return TRUE; }   // overlays: cosmetic, later
BOOL CImageList::GetImageInfo(int, IMAGEINFO*) const { return FALSE; }  // IMAGEINFO opaque on POSIX
BOOL CImageList::Read(CArchive*)  { return FALSE; }
BOOL CImageList::Write(CArchive*) { return FALSE; }

// Drag imagery is a UI nicety with no offscreen surface to affect yet.
BOOL CImageList::BeginDrag(int, CPoint) { return FALSE; }
BOOL CImageList::DragEnter(CWnd*, CPoint) { return FALSE; }
BOOL CImageList::DragShowNolock(BOOL) { return FALSE; }
BOOL CImageList::DragMove(CPoint) { return FALSE; }
BOOL CImageList::DragLeave(CWnd*) { return FALSE; }
void CImageList::EndDrag() {}
