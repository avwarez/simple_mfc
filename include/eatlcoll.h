#pragma once

#include "eafxcoll.h"

#include <vector>
#include <map>
#include <memory>

#include "eatlbase.h"

template <class KEY, class VALUE, class KTraits = void, class VTraits = void>
class ECRBMap
{
public:
    struct ECPair
    {
        ECPair(const KEY& key, const VALUE& value) : m_key(key), m_value(value) {}

        const KEY m_key;
        VALUE m_value;
    };

private:
    using MapT = std::map<KEY, ECPair>;
    using Iter = typename MapT::iterator;

public:
    ECRBMap() = default;

    EPOSITION SetAt(const KEY& key, const VALUE& value)
    {
        auto it = m_map.find(key);
        if (it == m_map.end())
            it = m_map.emplace(key, ECPair(key, value)).first;
        else
            it->second.m_value = value;
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

    const KEY& GetKeyAt(EPOSITION pos) const { return PairFromPos(pos)->m_key; }
    VALUE& GetValueAt(EPOSITION pos) { return PairFromPos(pos)->m_value; }
    const VALUE& GetValueAt(EPOSITION pos) const { return PairFromPos(pos)->m_value; }

    ECPair* GetNext(EPOSITION& pos)
    {
        ECPair* pair = PairFromPos(pos);
        Advance(pos, IterFromPos(pos));
        return pair;
    }
    VALUE& GetNextValue(EPOSITION& pos)
    {
        VALUE& v = PairFromPos(pos)->m_value;
        Advance(pos, IterFromPos(pos));
        return v;
    }
    ECPair* GetPrev(EPOSITION& pos)
    {
        ECPair* pair = PairFromPos(pos);
        Retreat(pos, IterFromPos(pos));
        return pair;
    }

private:
    static EPOSITION Box(Iter it) { return &it->second; }
    ECPair* PairFromPos(EPOSITION pos) const { return static_cast<ECPair*>(pos); }
    Iter IterFromPos(EPOSITION pos) const
    {
        return const_cast<MapT&>(m_map).find(PairFromPos(pos)->m_key);
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
};
