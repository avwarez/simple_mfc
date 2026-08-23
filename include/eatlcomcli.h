// atlcomcli.h — reference STUB (declarations only, no implementation).
// ATL's COM client wrappers: the smart pointers, the BSTR holder and the
// VARIANT holder. Real ATL declares them here, not in atlbase.h (which
// only includes this file), so they live here too.
//
// Every declaration below was checked against the Microsoft Learn ATL
// reference (CComPtr / CComPtrBase / CComQIPtr / CComBSTR / CComVariant
// class pages). Only the members eMule/srchybrid actually reaches are
// declared -- this is a subset, but the subset does not deviate.
//
// The interfaces these pointers carry are genuine COM (OLE, MSHTML,
// DirectShow, Windows Media, UPnP), so this header is Windows-facing by
// nature, exactly like simple_mfc's own frontend headers.
#pragma once
#include "eatldef.h"

// COM primitives. On Windows they all come from <windows.h>/<ole2.h>,
// which afxwin.h already pulls in. Off Windows they are named only so the
// declarations below parse (same approach as afxole.h's data-transfer
// types); repeating a typedef identically is legal, which is why some of
// these also appear in afxocc.h/afxole.h.
#ifndef _WIN32
struct IUnknown;
using LPUNKNOWN = IUnknown*;
// GUID is a COM type, so atlcomcli.h owns it -- single owner per symbol, and
// this header is reachable without afxwin.h, so it cannot borrow the one in
// platform/. The layout is the SDK's, with Data1 spelled `unsigned int` rather
// than the SDK's `unsigned long`: Windows' long is 32 bits and LP64's is 64,
// which would shift every field after it and double sizeof(GUID).
struct _GUID
{
    unsigned int   Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[8];
};
using GUID = _GUID;
using CLSID = GUID;
using IID = GUID;
using REFCLSID = const CLSID&;
using REFIID = const IID&;
using REFGUID = const GUID&;
using HRESULT = int;             // Win32: a LONG, 32 bits -- see afx.h
using OLECHAR = wchar_t;
using BSTR = OLECHAR*;
using LPCOLESTR = const OLECHAR*;
using LPCSTR = const char*;
using VARTYPE = unsigned short;
struct IStream;
struct IDispatch;

// CComVariant derives from this, which is how eMule can take its address
// as a VARIANT* and read .vt / .bstrVal off it directly.
struct tagVARIANT
{
    VARTYPE vt;
    union
    {
        long lVal;
        BSTR bstrVal;
        tagVARIANT* pvarVal;
        void* byref;
    };
};
using VARIANT = tagVARIANT;

// The VARTYPE values used as default arguments below.
#define VT_I4  3
#define VT_UI4 19
#endif

// ---------------------------------------------------------------------
// _NoAddRefReleaseOnCComPtr — what operator-> really returns in ATL. It
// derives from the interface and hides AddRef/Release, so that calling
// them accidentally through a smart pointer is a compile error. Declared
// faithfully rather than shortcutting operator-> to T*, because that
// return type IS the documented signature.
// ---------------------------------------------------------------------
template <class T>
class _NoAddRefReleaseOnCComPtr : public T
{
private:
    virtual unsigned long __stdcall AddRef() = 0;
    virtual unsigned long __stdcall Release() = 0;
};

// ---------------------------------------------------------------------
// CComPtrBase — the shared machinery behind CComPtr and CComQIPtr.
// eMule holds a CComPtr as a class member (CCustomAutoComplete::m_pac),
// so these must be complete types, and hands the address of one to COM
// functions that fill it in ("stream.Attach(::SHCreateMemStream(NULL,0))").
// ---------------------------------------------------------------------
template <class T>
class ECComPtrBase
{
protected:
    ECComPtrBase() noexcept;
    ECComPtrBase(T* lp) noexcept;
    ~ECComPtrBase();

public:
    // The raw pointer, a public member in real ATL; eMule reads it
    // directly rather than going through the conversion operator.
    T* p;

    operator T*() const noexcept;
    T& operator*() const noexcept;
    T** operator&() noexcept;
    _NoAddRefReleaseOnCComPtr<T>* operator->() const noexcept;
    bool operator!() const noexcept;
    bool operator<(T* pT) const noexcept;
    bool operator==(T* pT) const noexcept;

    void Release() noexcept;
    void Attach(T* p2) noexcept;
    T* Detach() noexcept;
    HRESULT CopyTo(T** ppT) noexcept;
    bool IsEqualObject(IUnknown* pOther) noexcept;

    template <class Q>
    HRESULT QueryInterface(Q** pp) const noexcept;

    // Declared on the base, not on CComPtr -- checked against the Learn
    // CComPtrBase page, which is where both overloads live.
    HRESULT CoCreateInstance(LPCOLESTR szProgID, LPUNKNOWN pUnkOuter = nullptr,
                              DWORD dwClsContext = 0x17 /*CLSCTX_ALL*/) noexcept;
    HRESULT CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter = nullptr,
                              DWORD dwClsContext = 0x17 /*CLSCTX_ALL*/) noexcept;
};

template <class T>
class ECComPtr : public ECComPtrBase<T>
{
public:
    ECComPtr() noexcept;
    ECComPtr(T* lp) noexcept;
    ECComPtr(const ECComPtr<T>& lp) noexcept;

    T* operator=(T* lp) noexcept;
    T* operator=(const ECComPtr<T>& lp) noexcept;
};

// ---------------------------------------------------------------------
// CComQIPtr — a CComPtr that queries for its interface on assignment.
// eMule builds one from an already-held pointer to a different interface
// ("CComQIPtr<IWMHeaderInfo2> pIWMHeaderInfo2(pIWMHeaderInfo);"), which
// is the IUnknown* constructor below. The default template argument is
// real ATL's &__uuidof(T), an MSVC extension, so it is only spelled that
// way where the compiler has it.
// ---------------------------------------------------------------------
#ifdef _MSC_VER
template <class T, const IID* piid = &__uuidof(T)>
#else
template <class T, const IID* piid = nullptr>
#endif
class ECComQIPtr : public ECComPtr<T>
{
public:
    ECComQIPtr() noexcept;
    ECComQIPtr(T* lp) noexcept;
    ECComQIPtr(IUnknown* lp) noexcept;
    ECComQIPtr(const ECComQIPtr<T, piid>& lp) noexcept;

    T* operator=(T* lp) noexcept;
    T* operator=(const ECComQIPtr<T, piid>& lp) noexcept;
    T* operator=(IUnknown* lp) noexcept;
};

// ---------------------------------------------------------------------
// CComBSTR — owns a BSTR. eMule declares one empty and hands its address
// to a COM getter ("device->get_FriendlyName(&bsFriendlyName)"), builds
// one from a CString or a literal, and passes it BY VALUE as a function
// parameter ("void ProcessAsyncFind(CComBSTR bsSearchType)") -- hence the
// copy constructor.
// ---------------------------------------------------------------------
class ECComBSTR
{
public:
    // Public in real ATL, and read directly by eMule.
    BSTR m_str;

    ECComBSTR() noexcept;
    ECComBSTR(const ECComBSTR& src);
    ECComBSTR(int nSize);
    ECComBSTR(int nSize, LPCOLESTR sz);
    ECComBSTR(LPCOLESTR pSrc);
    ECComBSTR(LPCSTR pSrc);
    ~ECComBSTR();

    ECComBSTR& operator=(const ECComBSTR& src);
    ECComBSTR& operator=(LPCOLESTR pSrc);

    operator BSTR() const noexcept;
    BSTR* operator&() noexcept;

    unsigned int Length() const noexcept;
    unsigned int ByteLength() const noexcept;
    void Empty() noexcept;
    void Attach(BSTR src) noexcept;
    BSTR Detach() noexcept;
    BSTR Copy() const noexcept;
    // Not const in real ATL, unlike Copy above.
    HRESULT CopyTo(BSTR* pbstr) noexcept;
};

// ---------------------------------------------------------------------
// CComVariant — owns a VARIANT. It DERIVES from VARIANT in real ATL,
// which is what lets eMule take its address as a VARIANT*
// ("GetVariantElement(psa, nIndex, &vaOutElement)") and then read the
// inherited fields straight off it ("switch (vaOutElement.vt)").
// ---------------------------------------------------------------------
class ECComVariant : public tagVARIANT
{
public:
    ECComVariant() noexcept;
    ECComVariant(const ECComVariant& varSrc);
    ECComVariant(const VARIANT& varSrc);
    ECComVariant(LPCOLESTR lpszSrc);
    ECComVariant(LPCSTR lpszSrc);
    ECComVariant(bool bSrc);
    // The integral overloads carry a VARTYPE with a default, which is
    // part of the documented signature.
    ECComVariant(int nSrc, VARTYPE vtSrc = VT_I4) noexcept;
    ECComVariant(long nSrc, VARTYPE vtSrc = VT_I4) noexcept;
    ECComVariant(IDispatch* pSrc) noexcept;
    ECComVariant(IUnknown* pSrc) noexcept;
    ~ECComVariant() noexcept;

    ECComVariant& operator=(const ECComVariant& varSrc);
    ECComVariant& operator=(const VARIANT& varSrc);
    ECComVariant& operator=(LPCOLESTR lpszSrc);

    HRESULT Clear() noexcept;
    HRESULT Copy(const VARIANT* pSrc) noexcept;
    HRESULT Attach(VARIANT* pSrc);
    HRESULT Detach(VARIANT* pDest) noexcept;
    HRESULT ChangeType(VARTYPE vtNew, const VARIANT* pSrc = nullptr) noexcept;
};
