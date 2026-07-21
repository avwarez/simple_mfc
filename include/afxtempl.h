// afxtempl.h — NATIVE implementation (standard C++17 library only).
// Generic template collections, on top of std::vector/std::list/std::unordered_map.
// Header-only (as C++ templates generally are).
#pragma once

#include "afx.h"
#include "afxcoll.h" // POSITION, INT_PTR, mfc_detail::ListImpl/ArrayImpl

#include <memory>
#include <unordered_map>
#include <utility>

// ---------------------------------------------------------------------
// CArray<TYPE,ARG_TYPE> — same interface as CObArray, made generic.
// ---------------------------------------------------------------------
template <class TYPE, class ARG_TYPE = const TYPE&>
class CArray : public CObject
{
public:
    INT_PTR Add(ARG_TYPE e) { return m_impl.Add(e); }
    INT_PTR Append(const CArray& src) { return m_impl.Append(src.m_impl); }
    void Copy(const CArray& src) { m_impl.Copy(src.m_impl); }
    TYPE& ElementAt(INT_PTR i) { return m_impl.ElementAt(i); }
    void FreeExtra() { m_impl.FreeExtra(); }
    const TYPE& GetAt(INT_PTR i) const { return m_impl.GetAt(i); }
    INT_PTR GetCount() const { return m_impl.GetCount(); }
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
class CList : public CObject
{
public:
    // Real MFC's CList takes a block size; eMule constructs some of its
    // lists with one.
    explicit CList(INT_PTR /*nBlockSize*/ = 10) {}

    POSITION AddHead(ARG_TYPE e) { return m_impl.AddHead(e); }
    POSITION AddTail(ARG_TYPE e) { return m_impl.AddTail(e); }
    TYPE& GetHead() { return m_impl.GetHead(); }
    TYPE& GetTail() { return m_impl.GetTail(); }
    TYPE RemoveHead() { return m_impl.RemoveHead(); }
    TYPE RemoveTail() { return m_impl.RemoveTail(); }
    POSITION GetHeadPosition() const { return m_impl.GetHeadPosition(); }
    POSITION GetTailPosition() const { return m_impl.GetTailPosition(); }
    TYPE& GetNext(POSITION& rPosition) { return m_impl.GetNext(rPosition); }
    // const traversal overloads (real MFC has them): selected when the
    // list is reached through a const reference, e.g. from a const getter.
    const TYPE& GetNext(POSITION& rPosition) const { return m_impl.GetNext(rPosition); }
    TYPE& GetPrev(POSITION& rPosition) { return m_impl.GetPrev(rPosition); }
    const TYPE& GetPrev(POSITION& rPosition) const { return m_impl.GetPrev(rPosition); }
    TYPE& GetAt(POSITION position) { return m_impl.GetAt(position); }
    const TYPE& GetAt(POSITION position) const { return m_impl.GetAt(position); }
    const TYPE& GetHead() const { return m_impl.GetHead(); }
    const TYPE& GetTail() const { return m_impl.GetTail(); }
    void SetAt(POSITION pos, ARG_TYPE e) { m_impl.SetAt(pos, e); }
    void RemoveAt(POSITION position) { m_impl.RemoveAt(position); }
    void RemoveAll() { m_impl.RemoveAll(); }
    POSITION Find(ARG_TYPE searchValue, POSITION startAfter = nullptr) const { return m_impl.Find(searchValue, startAfter); }
    POSITION FindIndex(INT_PTR nIndex) const { return m_impl.FindIndex(nIndex); }
    POSITION InsertBefore(POSITION position, ARG_TYPE e) { return m_impl.InsertBefore(position, e); }
    POSITION InsertAfter(POSITION position, ARG_TYPE e) { return m_impl.InsertAfter(position, e); }
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
// Our CMap is backed by std::unordered_map (std::hash), so simple_mfc never
// calls HashKey itself; but eMule/srchybrid provides full specializations
// (`template<> UINT AFXAPI HashKey(const CCKey&)` in MapKey.h/ShaHashset.h/
// DeadSourceList.h), which are only well-formed if this primary template is
// visible -- otherwise MSVC reports C2912 "not a specialization of a function
// template". The default body mirrors real MFC's identity hash and is valid
// for pointer/integral keys; class keys are always user-specialized.
// ---------------------------------------------------------------------
template <class ARG_KEY>
UINT AFXAPI HashKey(ARG_KEY key)
{
    return static_cast<UINT>(reinterpret_cast<std::uintptr_t>(
                                 (void*)(std::uintptr_t)key) >> 4);
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
        return static_cast<std::size_t>(HashKey<ARG_KEY>(const_cast<KEY&>(key)));
    }
};

template <class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
class CMap : public CObject
{
    using MapT = std::unordered_map<KEY, VALUE, MfcHashKey<KEY, ARG_KEY>>;
    using Iter = typename MapT::iterator;

public:
    struct CPair
    {
        // The constructor is what makes this work with eMule's key types:
        // aggregate initialisation ({k, v}) copy-initialises each member,
        // and CCKey (MapKey.h) declares its copy constructor explicit,
        // which forbids exactly that. Direct-initialising in a member
        // initialiser list is allowed and does the same thing.
        CPair(const KEY& k, const VALUE& v) : key(k), value(v) {}

        const KEY key;
        VALUE value;
    };

    explicit CMap(INT_PTR /*nBlockSize*/ = 10) {}

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

    POSITION GetStartPosition() const { return m_map.empty() ? nullptr : new Iter(const_cast<MapT&>(m_map).begin()); }
    void GetNextAssoc(POSITION& rNextPosition, KEY& rKey, VALUE& rValue) const
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
    const CPair* PGetFirstAssoc() const
    {
        if (m_map.empty()) return nullptr;
        // Built with new, not make_unique: CPair is an aggregate with a
        // const key, so it has no constructor for make_unique to forward
        // its argument to.
        m_scratch.reset(new CPair(m_map.begin()->first, m_map.begin()->second));
        return m_scratch.get();
    }
    const CPair* PGetNextAssoc(const CPair* pAssocRet) const
    {
        auto it = m_map.find(pAssocRet->key);
        if (it == m_map.end() || ++it == m_map.end()) return nullptr;
        m_scratch.reset(new CPair(it->first, it->second));
        return m_scratch.get();
    }
    const CPair* PLookup(ARG_KEY key) const
    {
        auto it = m_map.find(key);
        if (it == m_map.end()) return nullptr;
        m_scratch.reset(new CPair(it->first, it->second));
        return m_scratch.get();
    }
    // The non-const forms real MFC also declares; eMule walks a map it
    // holds by value and assigns the result to a plain CPair*.
    CPair* PGetFirstAssoc() { return const_cast<CPair*>(AsConst().PGetFirstAssoc()); }
    CPair* PGetNextAssoc(const CPair* pAssocRet) { return const_cast<CPair*>(AsConst().PGetNextAssoc(pAssocRet)); }
    CPair* PLookup(ARG_KEY key) { return const_cast<CPair*>(AsConst().PLookup(key)); }

private:
    const CMap& AsConst() const { return *this; }

    MapT m_map;
    mutable std::unique_ptr<CPair> m_scratch;
};

// ---------------------------------------------------------------------
// CTypedPtrList<BASE_CLASS, TYPE> — type-safe wrapper over a pointer list
// (BASE_CLASS is CPtrList or CObList). Each accessor forwards to the base
// and casts the base's void*/CObject* element to TYPE, exactly like real
// MFC (which uses the same C-style reference cast on the base result).
// ---------------------------------------------------------------------
template <class BASE_CLASS, class TYPE>
class CTypedPtrList : public BASE_CLASS
{
public:
    POSITION AddHead(TYPE newElement) { return BASE_CLASS::AddHead(newElement); }
    POSITION AddTail(TYPE newElement) { return BASE_CLASS::AddTail(newElement); }
    TYPE& GetHead() { return reinterpret_cast<TYPE&>(BASE_CLASS::GetHead()); }
    TYPE GetHead() const { return reinterpret_cast<TYPE>(BASE_CLASS::GetHead()); }
    TYPE& GetTail() { return reinterpret_cast<TYPE&>(BASE_CLASS::GetTail()); }
    TYPE GetTail() const { return reinterpret_cast<TYPE>(BASE_CLASS::GetTail()); }
    TYPE RemoveHead() { return reinterpret_cast<TYPE>(BASE_CLASS::RemoveHead()); }
    TYPE RemoveTail() { return reinterpret_cast<TYPE>(BASE_CLASS::RemoveTail()); }
    TYPE& GetNext(POSITION& rPosition) { return reinterpret_cast<TYPE&>(BASE_CLASS::GetNext(rPosition)); }
    // const traversal overloads (real MFC has them): selected when the list is
    // accessed through a const reference (e.g. inside eMule's const getters).
    TYPE GetNext(POSITION& rPosition) const { return reinterpret_cast<TYPE>(BASE_CLASS::GetNext(rPosition)); }
    TYPE& GetPrev(POSITION& rPosition) { return reinterpret_cast<TYPE&>(BASE_CLASS::GetPrev(rPosition)); }
    TYPE GetPrev(POSITION& rPosition) const { return reinterpret_cast<TYPE>(BASE_CLASS::GetPrev(rPosition)); }
    TYPE& GetAt(POSITION position) { return reinterpret_cast<TYPE&>(BASE_CLASS::GetAt(position)); }
    TYPE GetAt(POSITION position) const { return reinterpret_cast<TYPE>(BASE_CLASS::GetAt(position)); }
    void SetAt(POSITION pos, TYPE newElement) { BASE_CLASS::SetAt(pos, newElement); }
    // Find is deliberately NOT redeclared here: real MFC lets it come
    // from the base, so the parameter stays void* and eMule's
    // "filelist.Find((void*)file)" resolves without a cast to TYPE.
    POSITION InsertBefore(POSITION position, TYPE newElement) { return BASE_CLASS::InsertBefore(position, newElement); }
    POSITION InsertAfter(POSITION position, TYPE newElement) { return BASE_CLASS::InsertAfter(position, newElement); }
};

// ---------------------------------------------------------------------
// CTypedPtrArray<BASE_CLASS, TYPE> — type-safe wrapper over a pointer array
// (BASE_CLASS is CPtrArray or CObArray); same casting scheme as above.
// ---------------------------------------------------------------------
template <class BASE_CLASS, class TYPE>
class CTypedPtrArray : public BASE_CLASS
{
public:
    INT_PTR Add(TYPE newElement) { return BASE_CLASS::Add(newElement); }
    TYPE GetAt(INT_PTR nIndex) const { return reinterpret_cast<TYPE>(BASE_CLASS::GetAt(nIndex)); }
    TYPE& ElementAt(INT_PTR nIndex) { return reinterpret_cast<TYPE&>(BASE_CLASS::ElementAt(nIndex)); }
    void SetAt(INT_PTR nIndex, TYPE newElement) { BASE_CLASS::SetAt(nIndex, newElement); }
    void SetAtGrow(INT_PTR nIndex, TYPE newElement) { BASE_CLASS::SetAtGrow(nIndex, newElement); }
    void InsertAt(INT_PTR nIndex, TYPE newElement, INT_PTR nCount = 1) { BASE_CLASS::InsertAt(nIndex, newElement, nCount); }
    TYPE* GetData() { return reinterpret_cast<TYPE*>(BASE_CLASS::GetData()); }
    TYPE operator[](INT_PTR nIndex) const { return reinterpret_cast<TYPE>(BASE_CLASS::GetAt(nIndex)); }
    TYPE& operator[](INT_PTR nIndex) { return reinterpret_cast<TYPE&>(BASE_CLASS::ElementAt(nIndex)); }
};

// ---------------------------------------------------------------------
// CMapStringToPtr / CMapStringToString — concrete string-keyed maps. Real
// MFC declares these in afxcoll.h; here they build on the CMap template
// above (which lives in this header), so they are defined here instead.
// ARG_KEY is LPCTSTR: a CString key converts implicitly at every call.
// ---------------------------------------------------------------------
class CMapStringToPtr : public CMap<CString, LPCTSTR, void*, void*>
{
};

class CMapStringToString : public CMap<CString, LPCTSTR, CString, LPCTSTR>
{
};
