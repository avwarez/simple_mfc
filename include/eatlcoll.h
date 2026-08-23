#pragma once

#include "eafxcoll.h"

#include <vector>
#include <map>
#include <memory>

#include "eatlbase.h"

template <class KEY, class VALUE, class KTraits = void, class VTraits = void>
class ECRBMap
{
    using MapT = std::map<KEY, VALUE>;
    using ValueType = typename MapT::value_type;

public:
    struct ECPair
    {
        const KEY m_key;
        VALUE m_value;
    };

    ECRBMap() = default;

    EPOSITION SetAt(const KEY& key, const VALUE& value)
    {
        auto it = m_map.insert_or_assign(key, value).first;
        return Box(it);
    }

    void RemoveAll() { m_map.clear(); }

    void RemoveAt(EPOSITION pos)
    {
        auto it = IterFromPos(pos);
        if (it != m_map.end()) m_map.erase(it);
    }

    EPOSITION GetHeadPosition() const
    {
        return m_map.empty() ? nullptr : Box(const_cast<MapT&>(m_map).begin());
    }
    EPOSITION GetTailPosition() const
    {
        if (m_map.empty()) return nullptr;
        auto it = const_cast<MapT&>(m_map).end();
        --it;
        return Box(it);
    }

    EPOSITION FindFirstKeyAfter(const KEY& key) const
    {
        auto it = const_cast<MapT&>(m_map).upper_bound(key);
        return it == m_map.end() ? nullptr : Box(it);
    }

    const KEY& GetKeyAt(EPOSITION pos) const { return NodeFromPos(pos)->first; }
    VALUE& GetValueAt(EPOSITION pos) { return NodeFromPos(pos)->second; }
    const VALUE& GetValueAt(EPOSITION pos) const { return NodeFromPos(pos)->second; }

    ECPair* GetNext(EPOSITION& pos)
    {
        auto it = IterFromPos(pos);
        m_scratch = std::make_unique<ECPair>(ECPair{it->first, it->second});
        Advance(pos, it);
        return m_scratch.get();
    }
    VALUE& GetNextValue(EPOSITION& pos)
    {
        auto it = IterFromPos(pos);
        VALUE& v = it->second;
        Advance(pos, it);
        return v;
    }
    ECPair* GetPrev(EPOSITION& pos)
    {
        auto it = IterFromPos(pos);
        m_scratch = std::make_unique<ECPair>(ECPair{it->first, it->second});
        Retreat(pos, it);
        return m_scratch.get();
    }

private:
    using Iter = typename MapT::iterator;

    static EPOSITION Box(Iter it) { return const_cast<ValueType*>(&*it); }
    ValueType* NodeFromPos(EPOSITION pos) const { return static_cast<ValueType*>(pos); }
    Iter IterFromPos(EPOSITION pos) const
    {
        return const_cast<MapT&>(m_map).find(NodeFromPos(pos)->first);
    }
    void Advance(EPOSITION& pos, Iter it) const
    {
        ++it;
        pos = (it == const_cast<MapT&>(m_map).end()) ? nullptr : Box(it);
    }
    void Retreat(EPOSITION& pos, Iter it) const
    {
        if (it == const_cast<MapT&>(m_map).begin()) { pos = nullptr; return; }
        --it;
        pos = Box(it);
    }

    MapT m_map;
    mutable std::unique_ptr<ECPair> m_scratch;
};
