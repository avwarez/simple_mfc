// afxtempl.h — NATIVE implementation (standard C++17 library only).
// Generic template collections, on top of std::vector/std::list/std::unordered_map.
// Header-only (as C++ templates generally are).
#pragma once

#include "eafx.h"
#include "eafxcoll.h" // POSITION, INT_PTR, mfc_detail::ListImpl/ArrayImpl

#include <memory>
#include <unordered_map>
#include <utility>

// ---------------------------------------------------------------------
// CArray<TYPE,ARG_TYPE> — same interface as CPtrArray/CStringArray, made generic.
// ---------------------------------------------------------------------
template <class TYPE, class ARG_TYPE = const TYPE&>
class ECArray : public ECObject
{
public:
    INT_PTR Add(ARG_TYPE e) { return m_impl.Add(e); }
    INT_PTR Append(const ECArray& src) { return m_impl.Append(src.m_impl); }
    void Copy(const ECArray& src) { m_impl.Copy(src.m_impl); }
    TYPE& ElementAt(INT_PTR i) { return m_impl.ElementAt(i); }
    void FreeExtra() { m_impl.FreeExtra(); }
    const TYPE& GetAt(INT_PTR i) const { return m_impl.GetAt(i); }
    INT_PTR GetCount() const { return m_impl.GetCount(); }
    // Both constnesses, as real MFC has (and as the concrete
    // CPtrArray/CStringArray in afxcoll.h already do). Only the const one
    // was declared here, so every call on a non-const array still picked
    // it and handed back a "const TYPE*" -- which then failed at the
    // Win32 boundary, where the buffer is written to or sorted in place
    // (WSASend's LPWSABUF in EncryptedStreamSocket.cpp:220, qsort_s's
    // void* in WebServer.cpp).
    TYPE* GetData() { return m_impl.GetData(); }
    const TYPE* GetData() const { return m_impl.GetData(); }
    INT_PTR GetSize() const { return m_impl.GetSize(); }
    INT_PTR GetUpperBound() const { return m_impl.GetUpperBound(); }
    void InsertAt(INT_PTR i, ARG_TYPE e, INT_PTR nCount = 1) { m_impl.InsertAt(i, e, nCount); }
    BOOL IsEmpty() const { return m_impl.IsEmpty() ? TRUE : FALSE; }
    void RemoveAll() { m_impl.RemoveAll(); }
    void RemoveAt(INT_PTR i, INT_PTR nCount = 1) { m_impl.RemoveAt(i, nCount); }
    void SetAt(INT_PTR i, ARG_TYPE e) { m_impl.SetAt(i, e); }
    void SetAtGrow(INT_PTR i, ARG_TYPE e) { m_impl.SetAtGrow(i, e); }
    void SetSize(INT_PTR nNewSize, INT_PTR nGrowBy = -1) { m_impl.SetSize(nNewSize, nGrowBy); }

    const TYPE& operator[](INT_PTR i) const { return GetAt(i); }
    TYPE& operator[](INT_PTR i) { return ElementAt(i); }

private:
    mfc_detail::ArrayImpl<TYPE> m_impl;
};

// ---------------------------------------------------------------------
// CList<TYPE,ARG_TYPE> — same interface as CObList, made generic.
// ---------------------------------------------------------------------
template <class TYPE, class ARG_TYPE = const TYPE&>
class ECList : public ECObject
{
public:
    // Real MFC's CList takes a block size; eMule constructs some of its
    // lists with one.
    explicit ECList(INT_PTR /*nBlockSize*/ = 10) {}

    EPOSITION AddHead(ARG_TYPE e) { return m_impl.AddHead(e); }
    EPOSITION AddTail(ARG_TYPE e) { return m_impl.AddTail(e); }
    // The whole-list forms real MFC also has (CStringList in afxcoll.h
    // already carries the same pair). Kademlia's CEntry::Copy splices one
    // list onto another with them: "pEntry->m_listFileNames.AddTail(
    // &m_listFileNames);" (Entry.cpp:67). Without them the pointer was
    // matched against ARG_TYPE and reported as an unconvertible element.
    void AddHead(const ECList* pNewList)
    {
        if (pNewList == nullptr)
            return;
        // Head-insertion reverses, so walk the source backwards to keep
        // the appended block in its original order, as real MFC does.
        for (EPOSITION pos = pNewList->GetTailPosition(); pos != nullptr;)
            AddHead(pNewList->GetPrev(pos));
    }
    void AddTail(const ECList* pNewList)
    {
        if (pNewList == nullptr)
            return;
        for (EPOSITION pos = pNewList->GetHeadPosition(); pos != nullptr;)
            AddTail(pNewList->GetNext(pos));
    }
    TYPE& GetHead() { return m_impl.GetHead(); }
    TYPE& GetTail() { return m_impl.GetTail(); }
    TYPE RemoveHead() { return m_impl.RemoveHead(); }
    TYPE RemoveTail() { return m_impl.RemoveTail(); }
    EPOSITION GetHeadPosition() const { return m_impl.GetHeadPosition(); }
    EPOSITION GetTailPosition() const { return m_impl.GetTailPosition(); }
    TYPE& GetNext(EPOSITION& rPosition) { return m_impl.GetNext(rPosition); }
    // const traversal overloads (real MFC has them): selected when the
    // list is reached through a const reference, e.g. from a const getter.
    const TYPE& GetNext(EPOSITION& rPosition) const { return m_impl.GetNext(rPosition); }
    TYPE& GetPrev(EPOSITION& rPosition) { return m_impl.GetPrev(rPosition); }
    const TYPE& GetPrev(EPOSITION& rPosition) const { return m_impl.GetPrev(rPosition); }
    TYPE& GetAt(EPOSITION position) { return m_impl.GetAt(position); }
    const TYPE& GetAt(EPOSITION position) const { return m_impl.GetAt(position); }
    const TYPE& GetHead() const { return m_impl.GetHead(); }
    const TYPE& GetTail() const { return m_impl.GetTail(); }
    void SetAt(EPOSITION pos, ARG_TYPE e) { m_impl.SetAt(pos, e); }
    void RemoveAt(EPOSITION position) { m_impl.RemoveAt(position); }
    void RemoveAll() { m_impl.RemoveAll(); }
    EPOSITION Find(ARG_TYPE searchValue, EPOSITION startAfter = nullptr) const { return m_impl.Find(searchValue, startAfter); }
    EPOSITION FindIndex(INT_PTR nIndex) const { return m_impl.FindIndex(nIndex); }
    EPOSITION InsertBefore(EPOSITION position, ARG_TYPE e) { return m_impl.InsertBefore(position, e); }
    EPOSITION InsertAfter(EPOSITION position, ARG_TYPE e) { return m_impl.InsertAfter(position, e); }
    INT_PTR GetCount() const { return m_impl.GetCount(); }
    BOOL IsEmpty() const { return m_impl.IsEmpty() ? TRUE : FALSE; }

private:
    mfc_detail::ListImpl<TYPE> m_impl;
};

// ---------------------------------------------------------------------
// CMap<KEY,ARG_KEY,VALUE,ARG_VALUE> — hash map, on top of std::unordered_map.
// POSITION is a box around a std::unordered_map iterator, the same
// convention/limitation as CList (see the comment in afxcoll.h).
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
// HashKey<ARG_KEY> — real MFC's primary hashing template (afxtempl.h).
// CMap's own hasher (MfcHashKey below) forwards to it, so this is not
// just a compile-time placeholder: eMule/srchybrid provides full
// specializations for its own key types (`template<> UINT AFXAPI
// HashKey(const CCKey&)` in MapKey.h/ShaHashset.h/DeadSourceList.h),
// which are only well-formed if this primary template is visible --
// otherwise MSVC reports C2912 "not a specialization of a function
// template". The default body mirrors real MFC's identity hash and is
// valid for pointer/integral keys; class keys are always user-specialized.
// ---------------------------------------------------------------------
template <class ARG_KEY>
UINT EAFXAPI EHashKey(ARG_KEY key)
{
    // The Park-Miller "minimal standard" generator step, evaluated with
    // Schrage's trick so the intermediate never overflows 32 bits:
    //
    //     h(k) = (k * 16807) mod (2^31 - 1)
    //
    // This is what current MFC's primary HashKey template does -- NOT the
    // historical `((DWORD_PTR)key) >> 4` identity hash, which is what this
    // branch had until the conformance suite compared the numbers against
    // real MFC and every one of them differed.
    const long k = static_cast<long>(static_cast<std::intptr_t>((std::intptr_t)key));
    const long hi = k / 127773;
    const long lo = k - hi * 127773;
    const long test = 16807 * lo - 2836 * hi;
    return static_cast<UINT>(test >= 0 ? test : test + 2147483647L);
}

// Real MFC's OWN built-in override for LPCTSTR keys: hashes the string's
// CONTENT, not the identity of the pointer. Without this, the primary
// template above (identity hash) applies to a CMap<CString, LPCTSTR, ...>
// too -- every CMap operation constructs its own short-lived CString from
// the LPCTSTR key argument (via unordered_map's operator[]/find), so the
// pointer being hashed is a different, unrelated buffer address on every
// single call, even for the exact same string content. GetCount() still
// looked right (insert always succeeds, into *some* bucket), but
// Lookup/RemoveKey on a key that was genuinely inserted moments earlier
// would silently report "not found" -- found via the conformance suite's
// pattern-generation work, not by inspection. Real MFC's own HashKey
// (LPCTSTR) does this same content-based multiply-and-add walk (a
// textbook string hash, functionally equivalent to real MFC's, though not
// claimed bit-for-bit identical -- nothing observable depends on the
// specific hash value, only on equal strings landing in the same bucket).
template <>
inline UINT EAFXAPI EHashKey<LPCTSTR>(LPCTSTR key)
{
    // FNV-1 (multiply THEN xor, not FNV-1a) over TCHAR units, sampling at
    // most about ten characters: the stride is 1 + length/10, so short keys
    // are hashed whole and long ones are sampled. That is precisely what
    // current MFC's HashKey<LPCWSTR> does; the classic
    // `(h << 5) + h + c` shift-add this branch used is the *old* MFC one and
    // produced a different number for every string the suite tried.
    if (key == nullptr)
        return 2166136261u;

    std::size_t n = 0;
    while (key[n] != 0) ++n;
    const std::size_t stride = 1 + n / 10;

    UINT nHash = 2166136261u; // the FNV 32-bit offset basis
    for (std::size_t i = 0; i < n; i += stride)
        nHash = (nHash * 16777619u) ^ static_cast<UINT>(key[i]);
    return nHash;
}

// The bridge from MFC's hashing convention to std::unordered_map's. Real
// MFC hashes a key by calling HashKey(), so a class used as a CMap key
// only has to specialize HashKey -- which is exactly what eMule does
// (CSKey, CAICHHash, CCKey, ...) and why it never provides std::hash
// specializations. Hashing with std::hash directly made every one of
// those maps fail to instantiate.
template <class KEY, class ARG_KEY>
struct MfcHashKey
{
    std::size_t operator()(const KEY& key) const
    {
        return static_cast<std::size_t>(EHashKey<ARG_KEY>(const_cast<KEY&>(key)));
    }
};

template <class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
class ECMap : public ECObject
{
    using MapT = std::unordered_map<KEY, VALUE, MfcHashKey<KEY, ARG_KEY>>;
    using Iter = typename MapT::iterator;

public:
    struct ECPair
    {
        // The constructor is what makes this work with eMule's key types:
        // aggregate initialisation ({k, v}) copy-initialises each member,
        // and CCKey (MapKey.h) declares its copy constructor explicit,
        // which forbids exactly that. Direct-initialising in a member
        // initialiser list is allowed and does the same thing.
        ECPair(const KEY& k, const VALUE& v) : key(k), value(v) {}

        const KEY key;
        VALUE value;
    };

    explicit ECMap(INT_PTR /*nBlockSize*/ = 10) {}

    BOOL Lookup(ARG_KEY key, VALUE& rValue) const
    {
        auto it = m_map.find(key);
        if (it == m_map.end()) return FALSE;
        rValue = it->second;
        return TRUE;
    }
    void SetAt(ARG_KEY key, ARG_VALUE newValue) { m_map[key] = newValue; }
    VALUE& operator[](ARG_KEY key) { return m_map[key]; }
    BOOL RemoveKey(ARG_KEY key) { return m_map.erase(key) > 0 ? TRUE : FALSE; }
    void RemoveAll() { m_map.clear(); }

    EPOSITION GetStartPosition() const { return m_map.empty() ? nullptr : new Iter(const_cast<MapT&>(m_map).begin()); }
    void GetNextAssoc(EPOSITION& rNextPosition, KEY& rKey, VALUE& rValue) const
    {
        auto* box = static_cast<Iter*>(rNextPosition);
        rKey = (*box)->first;
        rValue = (*box)->second;
        ++(*box);
        if (*box == const_cast<MapT&>(m_map).end()) { delete box; rNextPosition = nullptr; }
    }

    void InitHashTable(UINT nHashSize, BOOL bAllocNow = TRUE) { if (!bAllocNow) return; m_map.reserve(nHashSize); }
    UINT GetHashTableSize() const { return static_cast<UINT>(m_map.bucket_count()); }
    INT_PTR GetCount() const { return static_cast<INT_PTR>(m_map.size()); }
    INT_PTR GetSize() const { return GetCount(); }
    BOOL IsEmpty() const { return m_map.empty() ? TRUE : FALSE; }

    // Note: in real MFC, CPair exposes `key`/`value` as direct fields of
    // an internal node; here, relying on std::unordered_map, we keep one
    // "live" CPair per call as a temporary view onto the found node
    // (valid until the map is modified, same as in real MFC).
    const ECPair* PGetFirstAssoc() const
    {
        if (m_map.empty()) return nullptr;
        // Built with new, not make_unique: CPair is an aggregate with a
        // const key, so it has no constructor for make_unique to forward
        // its argument to.
        m_scratch.reset(new ECPair(m_map.begin()->first, m_map.begin()->second));
        return m_scratch.get();
    }
    const ECPair* PGetNextAssoc(const ECPair* pAssocRet) const
    {
        auto it = m_map.find(pAssocRet->key);
        if (it == m_map.end() || ++it == m_map.end()) return nullptr;
        m_scratch.reset(new ECPair(it->first, it->second));
        return m_scratch.get();
    }
    const ECPair* PLookup(ARG_KEY key) const
    {
        auto it = m_map.find(key);
        if (it == m_map.end()) return nullptr;
        m_scratch.reset(new ECPair(it->first, it->second));
        return m_scratch.get();
    }
    // The non-const forms real MFC also declares; eMule walks a map it
    // holds by value and assigns the result to a plain CPair*.
    ECPair* PGetFirstAssoc() { return const_cast<ECPair*>(AsConst().PGetFirstAssoc()); }
    ECPair* PGetNextAssoc(const ECPair* pAssocRet) { return const_cast<ECPair*>(AsConst().PGetNextAssoc(pAssocRet)); }
    ECPair* PLookup(ARG_KEY key) { return const_cast<ECPair*>(AsConst().PLookup(key)); }

private:
    const ECMap& AsConst() const { return *this; }

    MapT m_map;
    mutable std::unique_ptr<ECPair> m_scratch;
};

// ---------------------------------------------------------------------
// CTypedPtrList<BASE_CLASS, TYPE> — type-safe wrapper over a pointer list
// (BASE_CLASS is CPtrList or CObList). Each accessor forwards to the base
// and casts the base's void*/CObject* element to TYPE, exactly like real
// MFC (which uses the same C-style reference cast on the base result).
// ---------------------------------------------------------------------
template <class BASE_CLASS, class TYPE>
class ECTypedPtrList : public BASE_CLASS
{
public:
    EPOSITION AddHead(TYPE newElement) { return BASE_CLASS::AddHead(newElement); }
    EPOSITION AddTail(TYPE newElement) { return BASE_CLASS::AddTail(newElement); }
    TYPE& GetHead() { return reinterpret_cast<TYPE&>(BASE_CLASS::GetHead()); }
    TYPE GetHead() const { return reinterpret_cast<TYPE>(BASE_CLASS::GetHead()); }
    TYPE& GetTail() { return reinterpret_cast<TYPE&>(BASE_CLASS::GetTail()); }
    TYPE GetTail() const { return reinterpret_cast<TYPE>(BASE_CLASS::GetTail()); }
    TYPE RemoveHead() { return reinterpret_cast<TYPE>(BASE_CLASS::RemoveHead()); }
    TYPE RemoveTail() { return reinterpret_cast<TYPE>(BASE_CLASS::RemoveTail()); }
    TYPE& GetNext(EPOSITION& rPosition) { return reinterpret_cast<TYPE&>(BASE_CLASS::GetNext(rPosition)); }
    // const traversal overloads (real MFC has them): selected when the list is
    // accessed through a const reference (e.g. inside eMule's const getters).
    TYPE GetNext(EPOSITION& rPosition) const { return reinterpret_cast<TYPE>(BASE_CLASS::GetNext(rPosition)); }
    TYPE& GetPrev(EPOSITION& rPosition) { return reinterpret_cast<TYPE&>(BASE_CLASS::GetPrev(rPosition)); }
    TYPE GetPrev(EPOSITION& rPosition) const { return reinterpret_cast<TYPE>(BASE_CLASS::GetPrev(rPosition)); }
    TYPE& GetAt(EPOSITION position) { return reinterpret_cast<TYPE&>(BASE_CLASS::GetAt(position)); }
    TYPE GetAt(EPOSITION position) const { return reinterpret_cast<TYPE>(BASE_CLASS::GetAt(position)); }
    void SetAt(EPOSITION pos, TYPE newElement) { BASE_CLASS::SetAt(pos, newElement); }
    // Find is deliberately NOT redeclared here: real MFC lets it come
    // from the base, so the parameter stays void* and eMule's
    // "filelist.Find((void*)file)" resolves without a cast to TYPE.
    EPOSITION InsertBefore(EPOSITION position, TYPE newElement) { return BASE_CLASS::InsertBefore(position, newElement); }
    EPOSITION InsertAfter(EPOSITION position, TYPE newElement) { return BASE_CLASS::InsertAfter(position, newElement); }
};

// ---------------------------------------------------------------------
// CTypedPtrArray<BASE_CLASS, TYPE> — type-safe wrapper over a pointer array
// (BASE_CLASS is CPtrArray); same casting scheme as above.
// ---------------------------------------------------------------------
template <class BASE_CLASS, class TYPE>
class ECTypedPtrArray : public BASE_CLASS
{
public:
    INT_PTR Add(TYPE newElement) { return BASE_CLASS::Add(newElement); }
    TYPE GetAt(INT_PTR nIndex) const { return reinterpret_cast<TYPE>(BASE_CLASS::GetAt(nIndex)); }
    TYPE& ElementAt(INT_PTR nIndex) { return reinterpret_cast<TYPE&>(BASE_CLASS::ElementAt(nIndex)); }
    void SetAt(INT_PTR nIndex, TYPE newElement) { BASE_CLASS::SetAt(nIndex, newElement); }
    void SetAtGrow(INT_PTR nIndex, TYPE newElement) { BASE_CLASS::SetAtGrow(nIndex, newElement); }
    void InsertAt(INT_PTR nIndex, TYPE newElement, INT_PTR nCount = 1) { BASE_CLASS::InsertAt(nIndex, newElement, nCount); }
    // GetData is deliberately NOT redeclared: real MFC's CTypedPtrArray does
    // not override it either, so it comes from the base and keeps the base's
    // `void**` return type. Redeclaring it as TYPE* compiled here but not
    // against real MFC -- the conformance job caught the difference.
    TYPE operator[](INT_PTR nIndex) const { return reinterpret_cast<TYPE>(BASE_CLASS::GetAt(nIndex)); }
    TYPE& operator[](INT_PTR nIndex) { return reinterpret_cast<TYPE&>(BASE_CLASS::ElementAt(nIndex)); }
};

// ---------------------------------------------------------------------
// CMapStringToPtr / CMapStringToString live in afxcoll.h (their real MFC
// home, as standalone classes), included by this header -- they used to be
// defined here as CMap<CString,LPCTSTR,...> subclasses, but real MFC keeps
// them self-contained in afxcoll.h, which is also the only placement that
// avoids an afxcoll.h -> afxtempl.h include cycle. See afxcoll.h.
