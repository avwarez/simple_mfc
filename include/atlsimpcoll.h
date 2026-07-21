// atlsimpcoll.h — CSimpleArray, ATL's lightweight vector. Real ATL
// declares it in this header (atlbase.h only includes it), so it lives
// here too.
//
// This one is a real implementation rather than a declaration-only stub:
// ATL's own CSimpleArray is header-only inline code, and the container is
// simple enough that backing it with std::vector is less work than
// stubbing it. Interface checked against the Microsoft Learn
// CSimpleArray class page -- note GetData is const and returns a
// non-const T*, and operator[] has no const overload; both are ATL's
// actual shape, not oversights.
//
// eMule/srchybrid uses Add / GetSize / GetData / RemoveAll / RemoveAt /
// Remove / Find / operator[], across 29 files.
#pragma once

#include "afx.h" // BOOL/TRUE/FALSE

#include <vector>

// The optional second template parameter mirrors ATL's TEqual policy slot
// but is unused here: Find/Remove use operator== directly, which is only
// instantiated for element types actually searched (so struct element
// types that never call Find need no operator==, matching real ATL
// template behaviour).
template <class T, class TEqual = void>
class CSimpleArray
{
public:
    CSimpleArray() = default;
    CSimpleArray(const CSimpleArray<T, TEqual>& src) = default;
    CSimpleArray<T, TEqual>& operator=(const CSimpleArray<T, TEqual>& src) = default;
    ~CSimpleArray() = default;

    int GetSize() const { return static_cast<int>(m_data.size()); }

    T& operator[](int nIndex) { return m_data[static_cast<size_t>(nIndex)]; }

    // const, returning a non-const pointer: that is ATL's declaration.
    T* GetData() const { return const_cast<T*>(m_data.data()); }

    BOOL Add(const T& t) { m_data.push_back(t); return TRUE; }

    BOOL Remove(const T& t)
    {
        int i = Find(t);
        return (i < 0) ? FALSE : RemoveAt(i);
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
