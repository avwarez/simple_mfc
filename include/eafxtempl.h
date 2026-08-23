#pragma once

#include "eafx.h"
#include "eafxcoll.h"

#include <memory>
#include <unordered_map>
#include <utility>

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

template <class TYPE, class ARG_TYPE = const TYPE&>
class ECList : public ECObject
{
public:
    explicit ECList(INT_PTR   = 10) {}

    EPOSITION AddHead(ARG_TYPE e) { return m_impl.AddHead(e); }
    EPOSITION AddTail(ARG_TYPE e) { return m_impl.AddTail(e); }
    void AddHead(const ECList* pNewList)
    {
        if (pNewList == nullptr)
            return;
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

template <class ARG_KEY>
UINT EAFXAPI EHashKey(ARG_KEY key)
{
    const long k = static_cast<long>(static_cast<std::intptr_t>((std::intptr_t)key));
    const long hi = k / 127773;
    const long lo = k - hi * 127773;
    const long test = 16807 * lo - 2836 * hi;
    return static_cast<UINT>(test >= 0 ? test : test + 2147483647L);
}

template <>
inline UINT EAFXAPI EHashKey<LPCTSTR>(LPCTSTR key)
{
    if (key == nullptr)
        return 2166136261u;

    std::size_t n = 0;
    while (key[n] != 0) ++n;
    const std::size_t stride = 1 + n / 10;

    UINT nHash = 2166136261u;
    for (std::size_t i = 0; i < n; i += stride)
        nHash = (nHash * 16777619u) ^ static_cast<UINT>(key[i]);
    return nHash;
}

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
        ECPair(const KEY& k, const VALUE& v) : key(k), value(v) {}

        const KEY key;
        VALUE value;
    };

    explicit ECMap(INT_PTR   = 10) {}

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

    const ECPair* PGetFirstAssoc() const
    {
        if (m_map.empty()) return nullptr;
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
    ECPair* PGetFirstAssoc() { return const_cast<ECPair*>(AsConst().PGetFirstAssoc()); }
    ECPair* PGetNextAssoc(const ECPair* pAssocRet) { return const_cast<ECPair*>(AsConst().PGetNextAssoc(pAssocRet)); }
    ECPair* PLookup(ARG_KEY key) { return const_cast<ECPair*>(AsConst().PLookup(key)); }

private:
    const ECMap& AsConst() const { return *this; }

    MapT m_map;
    mutable std::unique_ptr<ECPair> m_scratch;
};

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
    TYPE GetNext(EPOSITION& rPosition) const { return reinterpret_cast<TYPE>(BASE_CLASS::GetNext(rPosition)); }
    TYPE& GetPrev(EPOSITION& rPosition) { return reinterpret_cast<TYPE&>(BASE_CLASS::GetPrev(rPosition)); }
    TYPE GetPrev(EPOSITION& rPosition) const { return reinterpret_cast<TYPE>(BASE_CLASS::GetPrev(rPosition)); }
    TYPE& GetAt(EPOSITION position) { return reinterpret_cast<TYPE&>(BASE_CLASS::GetAt(position)); }
    TYPE GetAt(EPOSITION position) const { return reinterpret_cast<TYPE>(BASE_CLASS::GetAt(position)); }
    void SetAt(EPOSITION pos, TYPE newElement) { BASE_CLASS::SetAt(pos, newElement); }
    EPOSITION InsertBefore(EPOSITION position, TYPE newElement) { return BASE_CLASS::InsertBefore(position, newElement); }
    EPOSITION InsertAfter(EPOSITION position, TYPE newElement) { return BASE_CLASS::InsertAfter(position, newElement); }
};

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
    TYPE operator[](INT_PTR nIndex) const { return reinterpret_cast<TYPE>(BASE_CLASS::GetAt(nIndex)); }
    TYPE& operator[](INT_PTR nIndex) { return reinterpret_cast<TYPE&>(BASE_CLASS::ElementAt(nIndex)); }
};
