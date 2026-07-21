// afxcoll.h — NATIVE implementation (standard C++17 library only).
// "Concrete" MFC collections, reimplemented on top of std::list/std::vector.
//
// Note on POSITION: here it is an opaque pointer to a heap-allocated
// std::list iterator (a "box"), consistent with normal MFC usage
// (GetHeadPosition -> GetNext loop until POSITION is null). If an
// iteration is interrupted before reaching the end, the leftover box is
// not freed (a known limitation, documented in ../README.md: real MFC
// manages nodes through a block allocator (CPlex) freed when the list is
// destroyed; here, to stay on standard library only without reinventing
// an allocator, this practical limitation is accepted).
#pragma once

// Real MFC's sentinel POSITION meaning "before the first element", used
// by CListCtrl/CMap walkers to distinguish "not started" from "at end".
#ifndef BEFORE_START_POSITION
#define BEFORE_START_POSITION ((POSITION)-1L)
#endif

#include "afx.h"

#include <list>
#include <vector>

using POSITION = void*; // not a real Win32/SDK type, safe to always define

// INT_PTR is a real basetsd.h typedef on Windows; guarded the same way as
// afxwin.h's copy of this same alias (see there for why: eMule/srchybrid
// also includes real Win32 headers directly, which define this too).
#ifdef _WIN32
#include <windows.h>
#else
using INT_PTR = long long;
#endif

namespace mfc_detail
{

template <class T>
class ListImpl
{
public:
    POSITION AddHead(T v) { m_list.push_front(std::move(v)); return Box(m_list.begin()); }
    POSITION AddTail(T v) { m_list.push_back(std::move(v)); auto it = m_list.end(); --it; return Box(it); }
    T& GetHead() { return m_list.front(); }
    const T& GetHead() const { return m_list.front(); }
    T& GetTail() { return m_list.back(); }
    const T& GetTail() const { return m_list.back(); }
    T RemoveHead() { T v = std::move(m_list.front()); m_list.pop_front(); return v; }
    T RemoveTail() { T v = std::move(m_list.back()); m_list.pop_back(); return v; }

    POSITION GetHeadPosition() const { return m_list.empty() ? nullptr : Box(m_list.begin()); }
    POSITION GetTailPosition() const { if (m_list.empty()) return nullptr; auto it = m_list.end(); --it; return Box(it); }

    T& GetNext(POSITION& rPosition)
    {
        auto* box = static_cast<Iter*>(rPosition);
        T& ref = **box;
        ++(*box);
        if (*box == m_list.end()) { delete box; rPosition = nullptr; }
        return ref;
    }
    T& GetPrev(POSITION& rPosition)
    {
        auto* box = static_cast<Iter*>(rPosition);
        T& ref = **box;
        if (*box == m_list.begin()) { delete box; rPosition = nullptr; }
        else { --(*box); }
        return ref;
    }
    T& GetAt(POSITION position) { return **static_cast<Iter*>(position); }
    // const traversal overloads (real MFC has them): they advance the caller's
    // POSITION (which is external state) but never mutate the list itself.
    const T& GetNext(POSITION& rPosition) const
    {
        auto* box = static_cast<Iter*>(rPosition);
        const T& ref = **box;
        ++(*box);
        if (*box == m_list.end()) { delete box; rPosition = nullptr; }
        return ref;
    }
    const T& GetPrev(POSITION& rPosition) const
    {
        auto* box = static_cast<Iter*>(rPosition);
        const T& ref = **box;
        if (*box == m_list.begin()) { delete box; rPosition = nullptr; }
        else { --(*box); }
        return ref;
    }
    const T& GetAt(POSITION position) const { return **static_cast<Iter*>(position); }

    void SetAt(POSITION position, T v) { **static_cast<Iter*>(position) = std::move(v); }
    void RemoveAt(POSITION position)
    {
        auto* box = static_cast<Iter*>(position);
        m_list.erase(*box);
        delete box;
    }
    void RemoveAll() { m_list.clear(); }

    POSITION Find(const T& searchValue, POSITION startAfter = nullptr) const
    {
        auto it = startAfter ? std::next(*static_cast<Iter*>(startAfter)) : m_list.begin();
        for (; it != m_list.end(); ++it)
            if (*it == searchValue) return Box(it);
        return nullptr;
    }
    POSITION FindIndex(INT_PTR nIndex) const
    {
        if (nIndex < 0 || static_cast<size_t>(nIndex) >= m_list.size()) return nullptr;
        auto it = m_list.begin();
        std::advance(it, nIndex);
        return Box(it);
    }
    POSITION InsertBefore(POSITION position, T newElement)
    {
        auto it = position ? *static_cast<Iter*>(position) : m_list.begin();
        return Box(m_list.insert(it, std::move(newElement)));
    }
    POSITION InsertAfter(POSITION position, T newElement)
    {
        auto it = position ? std::next(*static_cast<Iter*>(position)) : m_list.begin();
        return Box(m_list.insert(it, std::move(newElement)));
    }

    INT_PTR GetCount() const { return static_cast<INT_PTR>(m_list.size()); }
    bool IsEmpty() const { return m_list.empty(); }

private:
    using Iter = typename std::list<T>::iterator;
    static POSITION Box(Iter it) { return new Iter(it); }
    mutable std::list<T> m_list;
};

template <class T>
class ArrayImpl
{
public:
    INT_PTR Add(T v) { m_v.push_back(std::move(v)); return static_cast<INT_PTR>(m_v.size()) - 1; }
    INT_PTR Append(const ArrayImpl& src)
    {
        // Real MFC returns the index of the FIRST appended element (i.e. the
        // old size), not the new total size.
        INT_PTR oldSize = static_cast<INT_PTR>(m_v.size());
        m_v.insert(m_v.end(), src.m_v.begin(), src.m_v.end());
        return oldSize;
    }
    void Copy(const ArrayImpl& src) { m_v = src.m_v; }
    T& ElementAt(INT_PTR i) { return m_v.at(static_cast<size_t>(i)); }
    void FreeExtra() { m_v.shrink_to_fit(); }
    const T& GetAt(INT_PTR i) const { return m_v.at(static_cast<size_t>(i)); }
    INT_PTR GetCount() const { return static_cast<INT_PTR>(m_v.size()); }
    T* GetData() { return m_v.data(); }
    const T* GetData() const { return m_v.data(); }
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
    void SetSize(INT_PTR nNewSize, INT_PTR /*nGrowBy*/ = -1) { m_v.resize(static_cast<size_t>(nNewSize)); }

private:
    std::vector<T> m_v;
};

} // namespace mfc_detail

// ---------------------------------------------------------------------
// CObList / CPtrList / CStringList
// ---------------------------------------------------------------------
class CObList : public CObject
{
    DECLARE_DYNAMIC(CObList)
public:
    POSITION AddHead(CObject* e) { return m_impl.AddHead(e); }
    POSITION AddTail(CObject* e) { return m_impl.AddTail(e); }
    CObject*& GetHead() { return m_impl.GetHead(); }
    CObject* GetHead() const { return m_impl.GetHead(); }
    CObject*& GetTail() { return m_impl.GetTail(); }
    CObject* GetTail() const { return m_impl.GetTail(); }
    CObject* RemoveHead() { return m_impl.RemoveHead(); }
    CObject* RemoveTail() { return m_impl.RemoveTail(); }
    POSITION GetHeadPosition() const { return m_impl.GetHeadPosition(); }
    POSITION GetTailPosition() const { return m_impl.GetTailPosition(); }
    CObject*& GetNext(POSITION& rPosition) { return m_impl.GetNext(rPosition); }
    CObject* GetNext(POSITION& rPosition) const { return m_impl.GetNext(rPosition); }
    CObject*& GetPrev(POSITION& rPosition) { return m_impl.GetPrev(rPosition); }
    CObject* GetPrev(POSITION& rPosition) const { return m_impl.GetPrev(rPosition); }
    CObject*& GetAt(POSITION position) { return m_impl.GetAt(position); }
    CObject* GetAt(POSITION position) const { return m_impl.GetAt(position); }
    void SetAt(POSITION pos, CObject* e) { m_impl.SetAt(pos, e); }
    void RemoveAt(POSITION position) { m_impl.RemoveAt(position); }
    void RemoveAll() { m_impl.RemoveAll(); }
    POSITION Find(CObject* searchValue, POSITION startAfter = nullptr) const { return m_impl.Find(searchValue, startAfter); }
    POSITION FindIndex(INT_PTR nIndex) const { return m_impl.FindIndex(nIndex); }
    POSITION InsertBefore(POSITION position, CObject* e) { return m_impl.InsertBefore(position, e); }
    POSITION InsertAfter(POSITION position, CObject* e) { return m_impl.InsertAfter(position, e); }
    INT_PTR GetCount() const { return m_impl.GetCount(); }
    BOOL IsEmpty() const { return m_impl.IsEmpty() ? TRUE : FALSE; }

private:
    mfc_detail::ListImpl<CObject*> m_impl;
};

class CPtrList : public CObject
{
    DECLARE_DYNAMIC(CPtrList)
public:
    POSITION AddHead(void* e) { return m_impl.AddHead(e); }
    POSITION AddTail(void* e) { return m_impl.AddTail(e); }
    void*& GetHead() { return m_impl.GetHead(); }
    void* GetHead() const { return m_impl.GetHead(); }
    void*& GetTail() { return m_impl.GetTail(); }
    void* GetTail() const { return m_impl.GetTail(); }
    void* RemoveHead() { return m_impl.RemoveHead(); }
    void* RemoveTail() { return m_impl.RemoveTail(); }
    POSITION GetHeadPosition() const { return m_impl.GetHeadPosition(); }
    POSITION GetTailPosition() const { return m_impl.GetTailPosition(); }
    void*& GetNext(POSITION& rPosition) { return m_impl.GetNext(rPosition); }
    void* GetNext(POSITION& rPosition) const { return m_impl.GetNext(rPosition); }
    void*& GetPrev(POSITION& rPosition) { return m_impl.GetPrev(rPosition); }
    void* GetPrev(POSITION& rPosition) const { return m_impl.GetPrev(rPosition); }
    POSITION Find(void* searchValue, POSITION startAfter = nullptr) const { return m_impl.Find(searchValue, startAfter); }
    POSITION FindIndex(INT_PTR nIndex) const { return m_impl.FindIndex(nIndex); }
    POSITION InsertBefore(POSITION position, void* e) { return m_impl.InsertBefore(position, e); }
    POSITION InsertAfter(POSITION position, void* e) { return m_impl.InsertAfter(position, e); }
    void*& GetAt(POSITION position) { return m_impl.GetAt(position); }
    void* GetAt(POSITION position) const { return m_impl.GetAt(position); }
    void SetAt(POSITION pos, void* e) { m_impl.SetAt(pos, e); }
    void RemoveAt(POSITION position) { m_impl.RemoveAt(position); }
    INT_PTR GetCount() const { return m_impl.GetCount(); }
    BOOL IsEmpty() const { return m_impl.IsEmpty() ? TRUE : FALSE; }
    void RemoveAll() { m_impl.RemoveAll(); }

private:
    mfc_detail::ListImpl<void*> m_impl;
};

class CStringList : public CObject
{
    DECLARE_DYNAMIC(CStringList)
public:
    POSITION AddHead(const CString& e) { return m_impl.AddHead(e); }
    POSITION AddTail(const CString& e) { return m_impl.AddTail(e); }
    CString& GetHead() { return m_impl.GetHead(); }
    CString& GetTail() { return m_impl.GetTail(); }
    CString RemoveHead() { return m_impl.RemoveHead(); }
    CString RemoveTail() { return m_impl.RemoveTail(); }
    POSITION GetHeadPosition() const { return m_impl.GetHeadPosition(); }
    CString& GetNext(POSITION& rPosition) { return m_impl.GetNext(rPosition); }
    POSITION Find(const CString& searchValue, POSITION startAfter = nullptr) const { return m_impl.Find(searchValue, startAfter); }
    INT_PTR GetCount() const { return m_impl.GetCount(); }
    BOOL IsEmpty() const { return m_impl.IsEmpty() ? TRUE : FALSE; }
    void RemoveAll() { m_impl.RemoveAll(); }

private:
    mfc_detail::ListImpl<CString> m_impl;
};

// ---------------------------------------------------------------------
// CObArray / CPtrArray / CStringArray / CByteArray / CUIntArray
// ---------------------------------------------------------------------
class CObArray : public CObject
{
    DECLARE_DYNAMIC(CObArray)
public:
    INT_PTR Add(CObject* e) { return m_impl.Add(e); }
    INT_PTR Append(const CObArray& src) { return m_impl.Append(src.m_impl); }
    void Copy(const CObArray& src) { m_impl.Copy(src.m_impl); }
    CObject*& ElementAt(INT_PTR i) { return m_impl.ElementAt(i); }
    void FreeExtra() { m_impl.FreeExtra(); }
    CObject* GetAt(INT_PTR i) const { return m_impl.GetAt(i); }
    INT_PTR GetCount() const { return m_impl.GetCount(); }
    CObject** GetData() { return m_impl.GetData(); }
    INT_PTR GetSize() const { return m_impl.GetSize(); }
    INT_PTR GetUpperBound() const { return m_impl.GetUpperBound(); }
    void InsertAt(INT_PTR i, CObject* e, INT_PTR nCount = 1) { m_impl.InsertAt(i, e, nCount); }
    BOOL IsEmpty() const { return m_impl.IsEmpty() ? TRUE : FALSE; }
    void RemoveAll() { m_impl.RemoveAll(); }
    void RemoveAt(INT_PTR i, INT_PTR nCount = 1) { m_impl.RemoveAt(i, nCount); }
    void SetAt(INT_PTR i, CObject* e) { m_impl.SetAt(i, e); }
    void SetAtGrow(INT_PTR i, CObject* e) { m_impl.SetAtGrow(i, e); }
    void SetSize(INT_PTR nNewSize, INT_PTR nGrowBy = -1) { m_impl.SetSize(nNewSize, nGrowBy); }
    CObject* operator[](INT_PTR i) const { return m_impl.GetAt(i); }
    CObject*& operator[](INT_PTR i) { return m_impl.ElementAt(i); }

private:
    mfc_detail::ArrayImpl<CObject*> m_impl;
};

class CPtrArray : public CObject
{
    DECLARE_DYNAMIC(CPtrArray)
public:
    INT_PTR Add(void* e) { return m_impl.Add(e); }
    INT_PTR Append(const CPtrArray& src) { return m_impl.Append(src.m_impl); }
    void Copy(const CPtrArray& src) { m_impl.Copy(src.m_impl); }
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

class CStringArray : public CObject
{
    DECLARE_DYNAMIC(CStringArray)
public:
    INT_PTR Add(const CString& e) { return m_impl.Add(e); }
    INT_PTR Append(const CStringArray& src) { return m_impl.Append(src.m_impl); }
    void Copy(const CStringArray& src) { m_impl.Copy(src.m_impl); }
    CString& ElementAt(INT_PTR i) { return m_impl.ElementAt(i); }
    const CString& GetAt(INT_PTR i) const { return m_impl.GetAt(i); }
    INT_PTR GetCount() const { return m_impl.GetCount(); }
    INT_PTR GetSize() const { return m_impl.GetSize(); }
    INT_PTR GetUpperBound() const { return m_impl.GetUpperBound(); }
    void InsertAt(INT_PTR i, const CString& e, INT_PTR nCount = 1) { m_impl.InsertAt(i, e, nCount); }
    BOOL IsEmpty() const { return m_impl.IsEmpty() ? TRUE : FALSE; }
    void RemoveAll() { m_impl.RemoveAll(); }
    void RemoveAt(INT_PTR i, INT_PTR nCount = 1) { m_impl.RemoveAt(i, nCount); }
    void SetAt(INT_PTR i, const CString& e) { m_impl.SetAt(i, e); }
    void SetAtGrow(INT_PTR i, const CString& e) { m_impl.SetAtGrow(i, e); }
    void SetSize(INT_PTR nNewSize, INT_PTR nGrowBy = -1) { m_impl.SetSize(nNewSize, nGrowBy); }
    const CString& operator[](INT_PTR i) const { return m_impl.GetAt(i); }
    CString& operator[](INT_PTR i) { return m_impl.ElementAt(i); }

private:
    mfc_detail::ArrayImpl<CString> m_impl;
};

// Numeric arrays. Real MFC generates these from the same CArray template, so
// they share the full array surface (Add/operator[]/ElementAt/SetAtGrow/...).
#define SIMPLE_MFC_DECLARE_NUM_ARRAY(ClassName, ElemType)                        \
class ClassName : public CObject                                                 \
{                                                                               \
    DECLARE_DYNAMIC(ClassName)                                                   \
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

SIMPLE_MFC_DECLARE_NUM_ARRAY(CByteArray, unsigned char)
SIMPLE_MFC_DECLARE_NUM_ARRAY(CWordArray, WORD)
SIMPLE_MFC_DECLARE_NUM_ARRAY(CDWordArray, DWORD)
SIMPLE_MFC_DECLARE_NUM_ARRAY(CUIntArray, UINT)
