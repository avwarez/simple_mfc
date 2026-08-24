#pragma once

#ifndef EBEFORE_START_POSITION
#define EBEFORE_START_POSITION ((EPOSITION)-1L)
#endif

#include "eafx.h"

#include <list>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <vector>

using EPOSITION = void*;

#ifdef _WIN32
#include <windows.h>
#else
#include <cstdint>
using INT_PTR = std::intptr_t;
#endif

namespace mfc_detail
{

template <class T>
class ListImpl
{
public:
    EPOSITION AddHead(T v) { m_list.push_front(std::move(v)); return Box(m_list.begin()); }
    EPOSITION AddTail(T v) { m_list.push_back(std::move(v)); auto it = m_list.end(); --it; return Box(it); }
    T& GetHead() { return m_list.front(); }
    const T& GetHead() const { return m_list.front(); }
    T& GetTail() { return m_list.back(); }
    const T& GetTail() const { return m_list.back(); }
    T RemoveHead() { T v = std::move(m_list.front()); m_list.pop_front(); return v; }
    T RemoveTail() { T v = std::move(m_list.back()); m_list.pop_back(); return v; }

    EPOSITION GetHeadPosition() const { return m_list.empty() ? nullptr : Box(m_list.begin()); }
    EPOSITION GetTailPosition() const { if (m_list.empty()) return nullptr; auto it = m_list.end(); --it; return Box(it); }

    T& GetNext(EPOSITION& rPosition)
    {
        auto* box = static_cast<Iter*>(rPosition);
        T& ref = **box;
        ++(*box);
        if (*box == m_list.end()) { delete box; rPosition = nullptr; }
        return ref;
    }
    T& GetPrev(EPOSITION& rPosition)
    {
        auto* box = static_cast<Iter*>(rPosition);
        T& ref = **box;
        if (*box == m_list.begin()) { delete box; rPosition = nullptr; }
        else { --(*box); }
        return ref;
    }
    T& GetAt(EPOSITION position) { return **static_cast<Iter*>(position); }
    const T& GetNext(EPOSITION& rPosition) const
    {
        auto* box = static_cast<Iter*>(rPosition);
        const T& ref = **box;
        ++(*box);
        if (*box == m_list.end()) { delete box; rPosition = nullptr; }
        return ref;
    }
    const T& GetPrev(EPOSITION& rPosition) const
    {
        auto* box = static_cast<Iter*>(rPosition);
        const T& ref = **box;
        if (*box == m_list.begin()) { delete box; rPosition = nullptr; }
        else { --(*box); }
        return ref;
    }
    const T& GetAt(EPOSITION position) const { return **static_cast<Iter*>(position); }

    void SetAt(EPOSITION position, T v) { **static_cast<Iter*>(position) = std::move(v); }
    void RemoveAt(EPOSITION position)
    {
        auto* box = static_cast<Iter*>(position);
        m_list.erase(*box);
        delete box;
    }
    void RemoveAll() { m_list.clear(); }

    EPOSITION Find(const T& searchValue, EPOSITION startAfter = nullptr) const
    {
        auto it = startAfter ? std::next(*static_cast<Iter*>(startAfter)) : m_list.begin();
        for (; it != m_list.end(); ++it)
            if (*it == searchValue) return Box(it);
        return nullptr;
    }
    EPOSITION FindIndex(INT_PTR nIndex) const
    {
        if (nIndex < 0 || static_cast<size_t>(nIndex) >= m_list.size()) return nullptr;
        auto it = m_list.begin();
        std::advance(it, nIndex);
        return Box(it);
    }
    EPOSITION InsertBefore(EPOSITION position, T newElement)
    {
        auto it = position ? *static_cast<Iter*>(position) : m_list.begin();
        return Box(m_list.insert(it, std::move(newElement)));
    }
    EPOSITION InsertAfter(EPOSITION position, T newElement)
    {
        auto it = position ? std::next(*static_cast<Iter*>(position)) : m_list.begin();
        return Box(m_list.insert(it, std::move(newElement)));
    }

    INT_PTR GetCount() const { return static_cast<INT_PTR>(m_list.size()); }
    bool IsEmpty() const { return m_list.empty(); }

private:
    using Iter = typename std::list<T>::iterator;
    static EPOSITION Box(Iter it) { return new Iter(it); }
    mutable std::list<T> m_list;
};

template <class T>
class ArrayImpl
{
    using StoredT = std::conditional_t<std::is_same<T, bool>::value, unsigned char, T>;
    static T& AsT(StoredT& v) noexcept { return reinterpret_cast<T&>(v); }
    static const T& AsT(const StoredT& v) noexcept { return reinterpret_cast<const T&>(v); }

public:
    INT_PTR Add(T v) { m_v.push_back(std::move(v)); return static_cast<INT_PTR>(m_v.size()) - 1; }
    INT_PTR Append(const ArrayImpl& src)
    {
        INT_PTR oldSize = static_cast<INT_PTR>(m_v.size());
        m_v.insert(m_v.end(), src.m_v.begin(), src.m_v.end());
        return oldSize;
    }
    void Copy(const ArrayImpl& src) { m_v = src.m_v; }
    T& ElementAt(INT_PTR i) { return AsT(m_v.at(static_cast<size_t>(i))); }
    void FreeExtra() { m_v.shrink_to_fit(); }
    const T& GetAt(INT_PTR i) const { return AsT(m_v.at(static_cast<size_t>(i))); }
    INT_PTR GetCount() const { return static_cast<INT_PTR>(m_v.size()); }
    T* GetData() { return reinterpret_cast<T*>(m_v.data()); }
    const T* GetData() const { return reinterpret_cast<const T*>(m_v.data()); }
    INT_PTR GetSize() const { return static_cast<INT_PTR>(m_v.size()); }
    INT_PTR GetUpperBound() const { return static_cast<INT_PTR>(m_v.size()) - 1; }
    void InsertAt(INT_PTR i, T v, INT_PTR nCount = 1)
    {
        if (static_cast<size_t>(i) > m_v.size()) m_v.resize(static_cast<size_t>(i));
        m_v.insert(m_v.begin() + i, static_cast<size_t>(nCount), v);
    }
    bool IsEmpty() const { return m_v.empty(); }
    void RemoveAll() { m_v.clear(); }
    void RemoveAt(INT_PTR i, INT_PTR nCount = 1) { m_v.erase(m_v.begin() + i, m_v.begin() + i + nCount); }
    void SetAt(INT_PTR i, T v) { m_v.at(static_cast<size_t>(i)) = std::move(v); }
    void SetAtGrow(INT_PTR i, T v)
    {
        if (static_cast<size_t>(i) >= m_v.size()) m_v.resize(static_cast<size_t>(i) + 1);
        m_v[static_cast<size_t>(i)] = std::move(v);
    }
    void SetSize(INT_PTR nNewSize, INT_PTR   = -1) { m_v.resize(static_cast<size_t>(nNewSize)); }

private:
    std::vector<StoredT> m_v;
};

}

class ECObList : public ECObject
{
    EDECLARE_DYNAMIC(ECObList)
public:
    EPOSITION AddHead(ECObject* e) { return m_impl.AddHead(e); }
    EPOSITION AddTail(ECObject* e) { return m_impl.AddTail(e); }
    ECObject*& GetHead() { return m_impl.GetHead(); }
    ECObject* GetHead() const { return m_impl.GetHead(); }
    ECObject*& GetTail() { return m_impl.GetTail(); }
    ECObject* GetTail() const { return m_impl.GetTail(); }
    ECObject* RemoveHead() { return m_impl.RemoveHead(); }
    ECObject* RemoveTail() { return m_impl.RemoveTail(); }
    EPOSITION GetHeadPosition() const { return m_impl.GetHeadPosition(); }
    EPOSITION GetTailPosition() const { return m_impl.GetTailPosition(); }
    ECObject*& GetNext(EPOSITION& rPosition) { return m_impl.GetNext(rPosition); }
    ECObject* GetNext(EPOSITION& rPosition) const { return m_impl.GetNext(rPosition); }
    ECObject*& GetPrev(EPOSITION& rPosition) { return m_impl.GetPrev(rPosition); }
    ECObject* GetPrev(EPOSITION& rPosition) const { return m_impl.GetPrev(rPosition); }
    ECObject*& GetAt(EPOSITION position) { return m_impl.GetAt(position); }
    ECObject* GetAt(EPOSITION position) const { return m_impl.GetAt(position); }
    void SetAt(EPOSITION pos, ECObject* e) { m_impl.SetAt(pos, e); }
    void RemoveAt(EPOSITION position) { m_impl.RemoveAt(position); }
    void RemoveAll() { m_impl.RemoveAll(); }
    EPOSITION Find(ECObject* searchValue, EPOSITION startAfter = nullptr) const { return m_impl.Find(searchValue, startAfter); }
    EPOSITION FindIndex(INT_PTR nIndex) const { return m_impl.FindIndex(nIndex); }
    EPOSITION InsertBefore(EPOSITION position, ECObject* e) { return m_impl.InsertBefore(position, e); }
    EPOSITION InsertAfter(EPOSITION position, ECObject* e) { return m_impl.InsertAfter(position, e); }
    INT_PTR GetCount() const { return m_impl.GetCount(); }
    BOOL IsEmpty() const { return m_impl.IsEmpty() ? TRUE : FALSE; }

private:
    mfc_detail::ListImpl<ECObject*> m_impl;
};

class ECPtrList : public ECObject
{
    EDECLARE_DYNAMIC(ECPtrList)
public:
    EPOSITION AddHead(void* e) { return m_impl.AddHead(e); }
    EPOSITION AddTail(void* e) { return m_impl.AddTail(e); }
    void*& GetHead() { return m_impl.GetHead(); }
    void* GetHead() const { return m_impl.GetHead(); }
    void*& GetTail() { return m_impl.GetTail(); }
    void* GetTail() const { return m_impl.GetTail(); }
    void* RemoveHead() { return m_impl.RemoveHead(); }
    void* RemoveTail() { return m_impl.RemoveTail(); }
    EPOSITION GetHeadPosition() const { return m_impl.GetHeadPosition(); }
    EPOSITION GetTailPosition() const { return m_impl.GetTailPosition(); }
    void*& GetNext(EPOSITION& rPosition) { return m_impl.GetNext(rPosition); }
    void* GetNext(EPOSITION& rPosition) const { return m_impl.GetNext(rPosition); }
    void*& GetPrev(EPOSITION& rPosition) { return m_impl.GetPrev(rPosition); }
    void* GetPrev(EPOSITION& rPosition) const { return m_impl.GetPrev(rPosition); }
    EPOSITION Find(void* searchValue, EPOSITION startAfter = nullptr) const { return m_impl.Find(searchValue, startAfter); }
    EPOSITION FindIndex(INT_PTR nIndex) const { return m_impl.FindIndex(nIndex); }
    EPOSITION InsertBefore(EPOSITION position, void* e) { return m_impl.InsertBefore(position, e); }
    EPOSITION InsertAfter(EPOSITION position, void* e) { return m_impl.InsertAfter(position, e); }
    void*& GetAt(EPOSITION position) { return m_impl.GetAt(position); }
    void* GetAt(EPOSITION position) const { return m_impl.GetAt(position); }
    void SetAt(EPOSITION pos, void* e) { m_impl.SetAt(pos, e); }
    void RemoveAt(EPOSITION position) { m_impl.RemoveAt(position); }
    INT_PTR GetCount() const { return m_impl.GetCount(); }
    BOOL IsEmpty() const { return m_impl.IsEmpty() ? TRUE : FALSE; }
    void RemoveAll() { m_impl.RemoveAll(); }

private:
    mfc_detail::ListImpl<void*> m_impl;
};

class ECMapPtrToPtr : public ECObject
{
    EDECLARE_DYNAMIC(ECMapPtrToPtr)
public:
    INT_PTR GetCount() const { return static_cast<INT_PTR>(m_map.size()); }
    BOOL IsEmpty() const { return m_map.empty() ? TRUE : FALSE; }
    void SetAt(void* key, void* newValue) { m_map[key] = newValue; }
    BOOL Lookup(void* key, void*& rValue) const
    {
        auto it = m_map.find(key);
        if (it == m_map.end())
            return FALSE;
        rValue = it->second;
        return TRUE;
    }
    BOOL RemoveKey(void* key) { return m_map.erase(key) != 0 ? TRUE : FALSE; }
    void RemoveAll() { m_map.clear(); }

    EPOSITION GetStartPosition() const
    {
        return m_map.empty() ? nullptr : new Iter(m_map.begin());
    }
    void GetNextAssoc(EPOSITION& rNextPosition, void*& rKey, void*& rValue) const
    {
        Iter* pIt = static_cast<Iter*>(rNextPosition);
        rKey = (*pIt)->first;
        rValue = (*pIt)->second;
        if (++(*pIt) == m_map.end()) {
            delete pIt;
            rNextPosition = nullptr;
        }
    }

private:
    using Map = std::unordered_map<void*, void*>;
    using Iter = Map::const_iterator;
    Map m_map;
};

namespace mfc_detail
{
struct CStringContentHash
{
    std::size_t operator()(const ECString& s) const
    {
        UINT nHash = 0;
        for (LPCTSTR p = s.GetString(); p && *p; ++p)
            nHash = (nHash << 5) + nHash + static_cast<UINT>(*p);
        return static_cast<std::size_t>(nHash);
    }
};

template <class VALUE, class ARG_VALUE>
class CStringKeyMapImpl : public ECObject
{
    using MapT = std::unordered_map<ECString, VALUE, CStringContentHash>;
    using Iter = typename MapT::iterator;

public:
    struct ECPair
    {
        ECPair(const ECString& k, const VALUE& v) : key(k), value(v) {}
        const ECString key;
        VALUE value;
    };

    explicit CStringKeyMapImpl(INT_PTR   = 10) {}

    BOOL Lookup(LPCTSTR key, VALUE& rValue) const
    {
        auto it = m_map.find(ECString(key));
        if (it == m_map.end()) return FALSE;
        rValue = it->second;
        return TRUE;
    }
    void SetAt(LPCTSTR key, ARG_VALUE newValue) { m_map[ECString(key)] = newValue; }
    VALUE& operator[](LPCTSTR key) { return m_map[ECString(key)]; }
    BOOL RemoveKey(LPCTSTR key) { return m_map.erase(ECString(key)) > 0 ? TRUE : FALSE; }
    void RemoveAll() { m_map.clear(); }

    EPOSITION GetStartPosition() const { return m_map.empty() ? nullptr : new Iter(const_cast<MapT&>(m_map).begin()); }
    void GetNextAssoc(EPOSITION& rNextPosition, ECString& rKey, VALUE& rValue) const
    {
        auto* box = static_cast<Iter*>(rNextPosition);
        rKey = (*box)->first;
        rValue = (*box)->second;
        ++(*box);
        if (*box == const_cast<MapT&>(m_map).end()) { delete box; rNextPosition = nullptr; }
    }

    void InitHashTable(UINT nHashSize, BOOL bAllocNow = TRUE) { if (!bAllocNow) return; m_map.reserve(nHashSize); }
    INT_PTR GetCount() const { return static_cast<INT_PTR>(m_map.size()); }
    INT_PTR GetSize() const { return GetCount(); }
    BOOL IsEmpty() const { return m_map.empty() ? TRUE : FALSE; }

    const ECPair* PLookup(LPCTSTR key) const
    {
        auto it = m_map.find(ECString(key));
        if (it == m_map.end()) return nullptr;
        m_scratch.reset(new ECPair(it->first, it->second));
        return m_scratch.get();
    }
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
    ECPair* PLookup(LPCTSTR key) { return const_cast<ECPair*>(AsConst().PLookup(key)); }
    ECPair* PGetFirstAssoc() { return const_cast<ECPair*>(AsConst().PGetFirstAssoc()); }
    ECPair* PGetNextAssoc(const ECPair* pAssocRet) { return const_cast<ECPair*>(AsConst().PGetNextAssoc(pAssocRet)); }

private:
    const CStringKeyMapImpl& AsConst() const { return *this; }
    MapT m_map;
    mutable std::unique_ptr<ECPair> m_scratch;
};
}

class ECMapStringToPtr : public mfc_detail::CStringKeyMapImpl<void*, void*>
{
public:
    using mfc_detail::CStringKeyMapImpl<void*, void*>::CStringKeyMapImpl;
};

class ECMapStringToString : public mfc_detail::CStringKeyMapImpl<ECString, LPCTSTR>
{
public:
    using mfc_detail::CStringKeyMapImpl<ECString, LPCTSTR>::CStringKeyMapImpl;
};

class ECStringList : public ECObject
{
    EDECLARE_DYNAMIC(ECStringList)
public:
    EPOSITION AddHead(const ECString& e) { return m_impl.AddHead(e); }
    EPOSITION AddTail(const ECString& e) { return m_impl.AddTail(e); }
    ECString& GetHead() { return m_impl.GetHead(); }
    ECString& GetTail() { return m_impl.GetTail(); }
    ECString RemoveHead() { return m_impl.RemoveHead(); }
    ECString RemoveTail() { return m_impl.RemoveTail(); }
    EPOSITION GetHeadPosition() const { return m_impl.GetHeadPosition(); }
    ECString& GetNext(EPOSITION& rPosition) { return m_impl.GetNext(rPosition); }
    const ECString& GetNext(EPOSITION& rPosition) const { return m_impl.GetNext(rPosition); }
    const ECString& GetHead() const { return m_impl.GetHead(); }
    const ECString& GetTail() const { return m_impl.GetTail(); }
    ECString& GetAt(EPOSITION position) { return m_impl.GetAt(position); }
    const ECString& GetAt(EPOSITION position) const { return m_impl.GetAt(position); }
    void SetAt(EPOSITION pos, const ECString& e) { m_impl.SetAt(pos, e); }
    void RemoveAt(EPOSITION position) { m_impl.RemoveAt(position); }
    EPOSITION InsertBefore(EPOSITION position, const ECString& e) { return m_impl.InsertBefore(position, e); }
    EPOSITION InsertAfter(EPOSITION position, const ECString& e) { return m_impl.InsertAfter(position, e); }
    EPOSITION GetTailPosition() const { return m_impl.GetTailPosition(); }
    void AddTail(ECStringList* pNewList)
    {
        if (pNewList == nullptr) return;
        for (EPOSITION pos = pNewList->GetHeadPosition(); pos != nullptr;)
            AddTail(pNewList->GetNext(pos));
    }
    void AddHead(ECStringList* pNewList)
    {
        if (pNewList == nullptr) return;
        EPOSITION posInsert = nullptr;
        for (EPOSITION pos = pNewList->GetHeadPosition(); pos != nullptr;) {
            const ECString& s = pNewList->GetNext(pos);
            posInsert = (posInsert == nullptr) ? AddHead(s) : InsertAfter(posInsert, s);
        }
    }
    EPOSITION Find(const ECString& searchValue, EPOSITION startAfter = nullptr) const { return m_impl.Find(searchValue, startAfter); }
    INT_PTR GetCount() const { return m_impl.GetCount(); }
    BOOL IsEmpty() const { return m_impl.IsEmpty() ? TRUE : FALSE; }
    void RemoveAll() { m_impl.RemoveAll(); }

private:
    mfc_detail::ListImpl<ECString> m_impl;
};

class ECPtrArray : public ECObject
{
    EDECLARE_DYNAMIC(ECPtrArray)
public:
    INT_PTR Add(void* e) { return m_impl.Add(e); }
    INT_PTR Append(const ECPtrArray& src) { return m_impl.Append(src.m_impl); }
    void Copy(const ECPtrArray& src) { m_impl.Copy(src.m_impl); }
    void*& ElementAt(INT_PTR i) { return m_impl.ElementAt(i); }
    void* GetAt(INT_PTR i) const { return m_impl.GetAt(i); }
    INT_PTR GetCount() const { return m_impl.GetCount(); }
    void** GetData() { return m_impl.GetData(); }
    INT_PTR GetSize() const { return m_impl.GetSize(); }
    INT_PTR GetUpperBound() const { return m_impl.GetUpperBound(); }
    void InsertAt(INT_PTR i, void* e, INT_PTR nCount = 1) { m_impl.InsertAt(i, e, nCount); }
    BOOL IsEmpty() const { return m_impl.IsEmpty() ? TRUE : FALSE; }
    void RemoveAll() { m_impl.RemoveAll(); }
    void RemoveAt(INT_PTR i, INT_PTR nCount = 1) { m_impl.RemoveAt(i, nCount); }
    void SetAt(INT_PTR i, void* e) { m_impl.SetAt(i, e); }
    void SetAtGrow(INT_PTR i, void* e) { m_impl.SetAtGrow(i, e); }
    void SetSize(INT_PTR nNewSize, INT_PTR nGrowBy = -1) { m_impl.SetSize(nNewSize, nGrowBy); }
    void* operator[](INT_PTR i) const { return m_impl.GetAt(i); }
    void*& operator[](INT_PTR i) { return m_impl.ElementAt(i); }

private:
    mfc_detail::ArrayImpl<void*> m_impl;
};

class ECStringArray : public ECObject
{
    EDECLARE_DYNAMIC(ECStringArray)
public:
    ECString* GetData() { return m_impl.GetData(); }
    const ECString* GetData() const { return m_impl.GetData(); }
    INT_PTR Add(const ECString& e) { return m_impl.Add(e); }
    INT_PTR Append(const ECStringArray& src) { return m_impl.Append(src.m_impl); }
    void Copy(const ECStringArray& src) { m_impl.Copy(src.m_impl); }
    ECString& ElementAt(INT_PTR i) { return m_impl.ElementAt(i); }
    const ECString& GetAt(INT_PTR i) const { return m_impl.GetAt(i); }
    INT_PTR GetCount() const { return m_impl.GetCount(); }
    INT_PTR GetSize() const { return m_impl.GetSize(); }
    INT_PTR GetUpperBound() const { return m_impl.GetUpperBound(); }
    void InsertAt(INT_PTR i, const ECString& e, INT_PTR nCount = 1) { m_impl.InsertAt(i, e, nCount); }
    BOOL IsEmpty() const { return m_impl.IsEmpty() ? TRUE : FALSE; }
    void RemoveAll() { m_impl.RemoveAll(); }
    void RemoveAt(INT_PTR i, INT_PTR nCount = 1) { m_impl.RemoveAt(i, nCount); }
    void SetAt(INT_PTR i, const ECString& e) { m_impl.SetAt(i, e); }
    void SetAtGrow(INT_PTR i, const ECString& e) { m_impl.SetAtGrow(i, e); }
    void SetSize(INT_PTR nNewSize, INT_PTR nGrowBy = -1) { m_impl.SetSize(nNewSize, nGrowBy); }
    const ECString& operator[](INT_PTR i) const { return m_impl.GetAt(i); }
    ECString& operator[](INT_PTR i) { return m_impl.ElementAt(i); }

private:
    mfc_detail::ArrayImpl<ECString> m_impl;
};

#define SIMPLE_MFC_DECLARE_NUM_ARRAY(ClassName, ElemType)                        \
class ClassName : public ECObject                                                 \
{                                                                               \
    EDECLARE_DYNAMIC(ClassName)                                                   \
public:                                                                          \
    INT_PTR Add(ElemType e) { return m_impl.Add(e); }                           \
    INT_PTR Append(const ClassName& src) { return m_impl.Append(src.m_impl); }   \
    void Copy(const ClassName& src) { m_impl.Copy(src.m_impl); }                 \
    ElemType& ElementAt(INT_PTR i) { return m_impl.ElementAt(i); }               \
    void FreeExtra() { m_impl.FreeExtra(); }                                     \
    ElemType GetAt(INT_PTR i) const { return m_impl.GetAt(i); }                  \
    INT_PTR GetCount() const { return m_impl.GetCount(); }                       \
    ElemType* GetData() { return m_impl.GetData(); }                            \
    const ElemType* GetData() const { return m_impl.GetData(); }                \
    INT_PTR GetSize() const { return m_impl.GetSize(); }                         \
    INT_PTR GetUpperBound() const { return m_impl.GetUpperBound(); }             \
    void InsertAt(INT_PTR i, ElemType e, INT_PTR nCount = 1) { m_impl.InsertAt(i, e, nCount); } \
    BOOL IsEmpty() const { return m_impl.IsEmpty() ? TRUE : FALSE; }             \
    void RemoveAll() { m_impl.RemoveAll(); }                                     \
    void RemoveAt(INT_PTR i, INT_PTR nCount = 1) { m_impl.RemoveAt(i, nCount); } \
    void SetAt(INT_PTR i, ElemType e) { m_impl.SetAt(i, e); }                    \
    void SetAtGrow(INT_PTR i, ElemType e) { m_impl.SetAtGrow(i, e); }            \
    void SetSize(INT_PTR nNewSize, INT_PTR nGrowBy = -1) { m_impl.SetSize(nNewSize, nGrowBy); } \
    ElemType operator[](INT_PTR i) const { return m_impl.GetAt(i); }             \
    ElemType& operator[](INT_PTR i) { return m_impl.ElementAt(i); }              \
private:                                                                         \
    mfc_detail::ArrayImpl<ElemType> m_impl;                                      \
};

SIMPLE_MFC_DECLARE_NUM_ARRAY(ECByteArray, unsigned char)
SIMPLE_MFC_DECLARE_NUM_ARRAY(ECWordArray, WORD)
SIMPLE_MFC_DECLARE_NUM_ARRAY(ECDWordArray, DWORD)
SIMPLE_MFC_DECLARE_NUM_ARRAY(ECUIntArray, UINT)
