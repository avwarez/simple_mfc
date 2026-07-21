// atlcoll.h — the ATL collection/COM surface eMule/srchybrid needs.
// Only the two templates it actually uses are covered: CSimpleArray and
// CRBMap. On Windows the real ATL is used for everything it provides (see
// the note above the #include below); elsewhere they are reimplemented on
// top of std::vector/std::map, standard C++17 only.
//
// Note this file never includes real ATL's own atlcoll.h: that is a large
// internal header (CAtlArray/CAtlList/CAtlMap/CRBTree/...) which would
// redefine POSITION on top of afx.h's -- the C2371 "redefinition;
// different basic types" that shadowing the real headers is meant to avoid.
#pragma once

#include "afxcoll.h" // POSITION (void*), INT_PTR, BOOL/UINT via afx.h

#include <vector>
#include <map>
#include <memory>

// ---------------------------------------------------------------------
// ATL is not MFC. simple_mfc exists to replace MFC, and on a real Windows
// toolchain ATL ships alongside the compiler whether or not MFC is used --
// so there the real thing is pulled in rather than reimplemented. That is
// not just economy, it is required for correctness: eMule/srchybrid also
// reaches ATL through SDK headers of its own (<atlimage.h> pulls
// <atlbase.h>), and a second CSimpleArray declared here made every use
// ambiguous (C2872) in exactly those translation units.
//
// atlbase.h also brings the COM smart pointers eMule uses in class member
// declarations -- CComPtr (CCustomAutoComplete::m_pac), CComBSTR,
// CComQIPtr, CComVariant -- which must be complete types, not stand-ins.
//
// Only the non-Windows build needs hand-written substitutes, below.
// ---------------------------------------------------------------------
#ifdef _WIN32
#include <atlbase.h>
#else

// ---------------------------------------------------------------------
// CSimpleArray — flat, contiguous array (ATL's lightweight vector).
// Backed by std::vector; GetData() hands out the contiguous buffer, the
// same guarantee real ATL makes. The optional second template parameter
// mirrors ATL's TEqual policy slot but is unused here: Find/Remove use
// operator== directly, which is only instantiated for element types that
// are actually searched (so struct element types that never call Find
// don't need an operator==, matching real ATL template behaviour).
// ---------------------------------------------------------------------
template <class T, class TEqual = void>
class CSimpleArray
{
public:
    CSimpleArray() = default;

    int GetSize() const { return static_cast<int>(m_data.size()); }

    T& operator[](int nIndex) { return m_data[static_cast<size_t>(nIndex)]; }
    const T& operator[](int nIndex) const { return m_data[static_cast<size_t>(nIndex)]; }

    T* GetData() { return m_data.data(); }
    const T* GetData() const { return m_data.data(); }

    BOOL Add(const T& t) { m_data.push_back(t); return TRUE; }

    BOOL Remove(const T& t)
    {
        int i = Find(t);
        if (i < 0) return FALSE;
        return RemoveAt(i);
    }

    BOOL RemoveAt(int nIndex)
    {
        if (nIndex < 0 || nIndex >= GetSize()) return FALSE;
        m_data.erase(m_data.begin() + nIndex);
        return TRUE;
    }

    void RemoveAll() { m_data.clear(); }

    int Find(const T& t) const
    {
        for (size_t i = 0; i < m_data.size(); ++i)
            if (m_data[i] == t) return static_cast<int>(i);
        return -1;
    }

    BOOL SetAtIndex(int nIndex, const T& t)
    {
        if (nIndex < 0 || nIndex >= GetSize()) return FALSE;
        m_data[static_cast<size_t>(nIndex)] = t;
        return TRUE;
    }

private:
    std::vector<T> m_data;
};

#endif // !_WIN32

// CRBMap is declared in real ATL's own atlcoll.h, which atlbase.h does NOT
// pull in -- so ours stays in play on Windows too. The guard is defensive:
// __ATLCOLL_H__ is real atlcoll.h's include guard, so should some other
// header ever drag it in, its CRBMap wins and this one steps aside instead
// of colliding.
#ifndef __ATLCOLL_H__

// ---------------------------------------------------------------------
// CRBMap — ordered (red-black-tree) key/value map. Backed by std::map,
// which keeps keys sorted and, crucially, keeps element addresses stable
// across insert/erase of *other* elements. That stability lets POSITION
// be the address of the map node's pair rather than a freshly boxed
// iterator (as CList/CMap do): callers such as CBarShader compare two
// POSITIONs for identity (`pos != endpos`), which only works if the same
// node always maps to the same POSITION value — exactly real ATL's
// node-pointer POSITION semantics. An iterator is recovered from a
// POSITION in O(log n) via find() on the stored key.
// ---------------------------------------------------------------------
template <class KEY, class VALUE, class KTraits = void, class VTraits = void>
class CRBMap
{
    using MapT = std::map<KEY, VALUE>;
    using ValueType = typename MapT::value_type; // std::pair<const KEY, VALUE>

public:
    struct CPair
    {
        const KEY m_key;
        VALUE m_value;
    };

    CRBMap() = default;

    INT_PTR GetCount() const { return static_cast<INT_PTR>(m_map.size()); }
    bool IsEmpty() const { return m_map.empty(); }

    POSITION SetAt(const KEY& key, const VALUE& value)
    {
        auto it = m_map.insert_or_assign(key, value).first;
        return Box(it);
    }

    void RemoveAll() { m_map.clear(); }

    void RemoveAt(POSITION pos)
    {
        auto it = IterFromPos(pos);
        if (it != m_map.end()) m_map.erase(it);
    }

    POSITION GetHeadPosition() const
    {
        return m_map.empty() ? nullptr : Box(const_cast<MapT&>(m_map).begin());
    }
    POSITION GetTailPosition() const
    {
        if (m_map.empty()) return nullptr;
        auto it = const_cast<MapT&>(m_map).end();
        --it;
        return Box(it);
    }

    // First element whose key is >= the given key (ATL: lower_bound).
    POSITION FindFirstKeyAfter(const KEY& key) const
    {
        auto it = const_cast<MapT&>(m_map).lower_bound(key);
        return it == m_map.end() ? nullptr : Box(it);
    }

    POSITION Lookup(const KEY& key) const
    {
        auto it = const_cast<MapT&>(m_map).find(key);
        return it == m_map.end() ? nullptr : Box(it);
    }
    bool Lookup(const KEY& key, VALUE& value) const
    {
        auto it = m_map.find(key);
        if (it == m_map.end()) return false;
        value = it->second;
        return true;
    }

    const KEY& GetKeyAt(POSITION pos) const { return NodeFromPos(pos)->first; }
    VALUE& GetValueAt(POSITION pos) { return NodeFromPos(pos)->second; }
    const VALUE& GetValueAt(POSITION pos) const { return NodeFromPos(pos)->second; }

    // Return the pair at pos, then advance pos to the next node (null at end).
    CPair* GetNext(POSITION& pos)
    {
        auto it = IterFromPos(pos);
        m_scratch = std::make_unique<CPair>(CPair{it->first, it->second});
        Advance(pos, it);
        return m_scratch.get();
    }
    // Return the value at pos, then advance pos to the next node.
    VALUE& GetNextValue(POSITION& pos)
    {
        auto it = IterFromPos(pos);
        VALUE& v = it->second;
        Advance(pos, it);
        return v;
    }
    // Return the pair at pos, then step pos back to the previous node.
    CPair* GetPrev(POSITION& pos)
    {
        auto it = IterFromPos(pos);
        m_scratch = std::make_unique<CPair>(CPair{it->first, it->second});
        Retreat(pos, it);
        return m_scratch.get();
    }

private:
    using Iter = typename MapT::iterator;

    // POSITION is the stable address of the map node's pair.
    static POSITION Box(Iter it) { return const_cast<ValueType*>(&*it); }
    ValueType* NodeFromPos(POSITION pos) const { return static_cast<ValueType*>(pos); }
    Iter IterFromPos(POSITION pos) const
    {
        return const_cast<MapT&>(m_map).find(NodeFromPos(pos)->first);
    }
    void Advance(POSITION& pos, Iter it) const
    {
        ++it;
        pos = (it == const_cast<MapT&>(m_map).end()) ? nullptr : Box(it);
    }
    void Retreat(POSITION& pos, Iter it) const
    {
        if (it == const_cast<MapT&>(m_map).begin()) { pos = nullptr; return; }
        --it;
        pos = Box(it);
    }

    MapT m_map;
    // One "live" CPair view for GetNext/GetPrev, mirroring CMap's scratch
    // pattern (valid until the next such call or a map modification).
    mutable std::unique_ptr<CPair> m_scratch;
};

#endif // __ATLCOLL_H__
