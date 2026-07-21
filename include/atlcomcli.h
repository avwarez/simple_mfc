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
#include "atldef.h"

// COM primitives. On Windows they all come from <windows.h>/<ole2.h>,
// which afxwin.h already pulls in. Off Windows they are named only so the
// declarations below parse (same approach as afxole.h's data-transfer
// types); repeating a typedef identically is legal, which is why some of
// these also appear in afxocc.h/afxole.h.
#ifndef _WIN32
struct IUnknown;
using LPUNKNOWN = IUnknown*;
struct GUID;
using CLSID = GUID;
using IID = GUID;
using REFCLSID = const CLSID&;
using REFIID = const IID&;
using REFGUID = const GUID&;
using HRESULT = long;
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
using LPVARIANT = VARIANT*;

// The VARTYPE values used as default arguments below.
#define VT_I4  3
#define VT_UI4 19
#define VT_R8  5
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
class CComPtrBase
{
protected:
    CComPtrBase() noexcept;
    CComPtrBase(T* lp) noexcept;
    ~CComPtrBase();

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
class CComPtr : public CComPtrBase<T>
{
public:
    CComPtr() noexcept;
    CComPtr(T* lp) noexcept;
    CComPtr(const CComPtr<T>& lp) noexcept;

    T* operator=(T* lp) noexcept;
    T* operator=(const CComPtr<T>& lp) noexcept;
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
class CComQIPtr : public CComPtr<T>
{
public:
    CComQIPtr() noexcept;
    CComQIPtr(T* lp) noexcept;
    CComQIPtr(IUnknown* lp) noexcept;
    CComQIPtr(const CComQIPtr<T, piid>& lp) noexcept;

    T* operator=(T* lp) noexcept;
    T* operator=(const CComQIPtr<T, piid>& lp) noexcept;
    T* operator=(IUnknown* lp) noexcept;
};

// ---------------------------------------------------------------------
// CComBSTR — owns a BSTR. eMule declares one empty and hands its address
// to a COM getter ("device->get_FriendlyName(&bsFriendlyName)"), builds
// one from a CString or a literal, and passes it BY VALUE as a function
// parameter ("void ProcessAsyncFind(CComBSTR bsSearchType)") -- hence the
// copy constructor.
// ---------------------------------------------------------------------
class CComBSTR
{
public:
    // Public in real ATL, and read directly by eMule.
    BSTR m_str;

    CComBSTR() noexcept;
    CComBSTR(const CComBSTR& src);
    CComBSTR(int nSize);
    CComBSTR(int nSize, LPCOLESTR sz);
    CComBSTR(LPCOLESTR pSrc);
    CComBSTR(LPCSTR pSrc);
    ~CComBSTR();

    CComBSTR& operator=(const CComBSTR& src);
    CComBSTR& operator=(LPCOLESTR pSrc);

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
class CComVariant : public tagVARIANT
{
public:
    CComVariant() noexcept;
    CComVariant(const CComVariant& varSrc);
    CComVariant(const VARIANT& varSrc);
    CComVariant(LPCOLESTR lpszSrc);
    CComVariant(LPCSTR lpszSrc);
    CComVariant(bool bSrc);
    // The integral overloads carry a VARTYPE with a default, which is
    // part of the documented signature.
    CComVariant(int nSrc, VARTYPE vtSrc = VT_I4) noexcept;
    CComVariant(long nSrc, VARTYPE vtSrc = VT_I4) noexcept;
    CComVariant(IDispatch* pSrc) noexcept;
    CComVariant(IUnknown* pSrc) noexcept;
    ~CComVariant() noexcept;

    CComVariant& operator=(const CComVariant& varSrc);
    CComVariant& operator=(const VARIANT& varSrc);
    CComVariant& operator=(LPCOLESTR lpszSrc);

    HRESULT Clear() noexcept;
    HRESULT Copy(const VARIANT* pSrc) noexcept;
    HRESULT Attach(VARIANT* pSrc);
    HRESULT Detach(VARIANT* pDest) noexcept;
    HRESULT ChangeType(VARTYPE vtNew, const VARIANT* pSrc = nullptr) noexcept;
};
