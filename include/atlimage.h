// atlimage.h — reference STUB (declarations only, no implementation).
// CImage, ATL's facade over GDI+ image loading/saving. Documented as an
// ATL/MFC *shared* class (same category as CString/CPoint/CRect), which
// is why its Learn page lives under atl-mfc-shared, not under atl.
//
// eMule/srchybrid reaches only four of its methods, at five call sites:
// EnBitmap.cpp loads an image file into a CBitmap, OtherFunctions.cpp
// converts an HBITMAP to and from an IStream, and FrameGrabThread.cpp /
// CaptchaGenerator.cpp dump a grabbed frame to disk (both inside
// #if TEST_FRAMEGRABBER). Signatures verified against the Learn CImage
// class page.
#pragma once
#include "atlcomcli.h" // IStream, HRESULT, REFGUID

#ifdef _WIN32
#include <windows.h> // HBITMAP
// CImage is a facade over GDI+, and real atlimage.h pulls GDI+ in with
// it. eMule relies on that: MuleListCtrl.cpp uses Gdiplus::Bitmap and
// GdiplusStartup, and FrameGrabThread.cpp names Gdiplus::ImageFormatBMP,
// neither of them including <gdiplus.h> themselves.
#include <gdiplus.h>
#else
using HBITMAP = void*;
using LPCTSTR = const wchar_t*;
#endif

class CImage
{
public:
    // How Attach should read the bitmap's row order. eMule passes
    // DIBOR_DEFAULT explicitly.
    enum DIBOrientation
    {
        DIBOR_DEFAULT,
        DIBOR_TOPDOWN,
        DIBOR_BOTTOMUP
    };

    CImage() noexcept;
    ~CImage();

    void Attach(HBITMAP hBitmap, DIBOrientation eOrientation = DIBOR_DEFAULT) noexcept;
    HBITMAP Detach() noexcept;

    HRESULT Load(LPCTSTR pszFileName) noexcept;
    HRESULT Load(IStream* pStream) noexcept;

    // GUID_NULL as the default file type means "infer it from the
    // extension"; eMule always passes one explicitly.
    HRESULT Save(IStream* pStream, REFGUID guidFileType) const noexcept;
    HRESULT Save(LPCTSTR pszFileName, REFGUID guidFileType) const noexcept;
};
