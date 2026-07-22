// atlcoll.h — CRBMap, the one ATL collection eMule/srchybrid uses from
// this header (BarShader.h's span map). Backed by std::map, standard
// C++17 only. Interface checked against the Microsoft Learn CRBMap and
// CRBTree class pages -- CRBMap derives from CRBTree there, so most of
// the members below are documented on the latter.
//
// Real ATL's atlcoll.h also carries CAtlArray/CAtlList/CAtlMap/CRBTree.
// None are declared here: eMule names none of them, and CAtlList in
// particular would redefine POSITION on top of afx.h's -- the C2371
// "redefinition; different basic types" that shadowing is meant to avoid.
#pragma once

#include "afxcoll.h" // POSITION (void*), INT_PTR, BOOL/UINT via afx.h

#include <vector>
#include <map>
#include <memory>

// ---------------------------------------------------------------------
// ATL is a separate library from MFC, and the project's rule was to defer
// to the real one on Windows rather than reimplement it. That rule had to
// change: with simple_mfc shadowing MFC's headers, real ATL's atlstr.h
// contributes ATL::CString while afx.h contributes ::CString, and ATL's
// automatic "using namespace ATL" then makes every unqualified use
// ambiguous (C2872). So the ATL surface eMule/srchybrid reaches is now
// shadowed too -- see atlbase.h for the scope and its limits.
//
// CSimpleArray moved to atlsimpcoll.h, which is where real ATL declares
// it; atlbase.h below pulls it in, exactly as real ATL does.
// ---------------------------------------------------------------------
#include "atlbase.h"



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
//
// Only the members eMule/srchybrid actually calls are declared -- the
// same subset discipline the rest of simple_mfc follows. eMule's one and
// only CRBMap is CBarShader::m_Spans (BarShader.h), which uses exactly:
// SetAt, GetHeadPosition, GetTailPosition, GetNext, GetNextValue, GetPrev,
// GetKeyAt, GetValueAt, FindFirstKeyAfter, RemoveAt, RemoveAll. Real ATL's
// CRBTree/CRBMap carry more (GetCount, IsEmpty, the CPair*-returning
// Lookup overloads, ...); those are intentionally NOT reproduced here.
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

    // First element whose key is STRICTLY GREATER than the given key
    // (std::upper_bound). The name is literal -- "after", not "at or
    // after": real ATL returns key 40 for FindFirstKeyAfter(30) when 30 is
    // present, NOT 30 itself (confirmed against real ATL by the conformance
    // suite; an earlier lower_bound here was a real divergence, since
    // CBarShader walks spans through exactly this call). NULL past the end.
    POSITION FindFirstKeyAfter(const KEY& key) const
    {
        auto it = const_cast<MapT&>(m_map).upper_bound(key);
        return it == m_map.end() ? nullptr : Box(it);
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
