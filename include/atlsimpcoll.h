// atlsimpcoll.h — CSimpleArray, ATL's lightweight vector. Real ATL
// declares it in this header (atlbase.h only includes it), so it lives
// here too.
//
// This one is a real implementation rather than a declaration-only stub:
// ATL's own CSimpleArray is header-only inline code. It deliberately
// keeps ATL's storage model -- a single malloc'd block in a public m_aT,
// with m_nSize/m_nAllocSize beside it -- instead of hiding a std::vector,
// because eMule/srchybrid reaches straight into it:
//
//   CKnownFile **ppRotated = (CKnownFile**)malloc(m_aFiles.m_nAllocSize * ...);
//   ...
//   m_aFiles.m_aT = ppRotated;          // SharedFileList.cpp
//
// which only works if the buffer really is one malloc'd block the array
// owns. Interface checked against the Microsoft Learn CSimpleArray page:
// GetData is const and returns a non-const T*, which is ATL's actual
// shape, not an oversight. The page omits the const subscript operator,
// but eMule indexes const arrays and compiles against real ATL, so that
// one is real as well -- the page is incomplete.
//
// eMule uses Add / GetSize / GetData / RemoveAll / RemoveAt / Remove /
// Find / operator[], across 29 files -- and only those are declared here
// (the subset discipline the rest of simple_mfc follows). Real ATL's
// CSimpleArray also has SetAtIndex; eMule never calls it (0 sites), so it
// is intentionally not reproduced.
#pragma once

#include "afx.h" // BOOL/TRUE/FALSE

#include <cstdlib>
#include <cstring>
#include <new>

// The optional second template parameter mirrors ATL's TEqual policy slot
// but is unused here: Find/Remove use operator== directly, which is only
// instantiated for element types actually searched (so struct element
// types that never call Find need no operator==, matching real ATL
// template behaviour).
template <class T, class TEqual = void>
class CSimpleArray
{
public:
    // All three are public in real ATL, and eMule assigns m_aT directly.
    T* m_aT;
    int m_nSize;
    int m_nAllocSize;

    CSimpleArray() : m_aT(nullptr), m_nSize(0), m_nAllocSize(0) {}

    CSimpleArray(const CSimpleArray<T, TEqual>& src) : m_aT(nullptr), m_nSize(0), m_nAllocSize(0)
    {
        for (int i = 0; i < src.GetSize(); ++i)
            Add(src[i]);
    }

    CSimpleArray<T, TEqual>& operator=(const CSimpleArray<T, TEqual>& src)
    {
        if (this != &src) {
            RemoveAll();
            for (int i = 0; i < src.GetSize(); ++i)
                Add(src[i]);
        }
        return *this;
    }

    ~CSimpleArray() { RemoveAll(); }

    int GetSize() const { return m_nSize; }

    T& operator[](int nIndex) { return m_aT[nIndex]; }
    const T& operator[](int nIndex) const { return m_aT[nIndex]; }

    T* GetData() const { return m_aT; }

    BOOL Add(const T& t)
    {
        if (m_nSize == m_nAllocSize && !Grow(m_nAllocSize == 0 ? 4 : m_nAllocSize * 2))
            return FALSE;
        // Placement new: the block is raw malloc'd memory.
        ::new (static_cast<void*>(m_aT + m_nSize)) T(t);
        ++m_nSize;
        return TRUE;
    }

    BOOL Remove(const T& t)
    {
        int i = Find(t);
        return (i < 0) ? FALSE : RemoveAt(i);
    }

    BOOL RemoveAt(int nIndex)
    {
        if (nIndex < 0 || nIndex >= m_nSize)
            return FALSE;
        m_aT[nIndex].~T();
        if (nIndex < m_nSize - 1)
            std::memmove(static_cast<void*>(m_aT + nIndex), static_cast<void*>(m_aT + nIndex + 1),
                         static_cast<size_t>(m_nSize - nIndex - 1) * sizeof(T));
        --m_nSize;
        return TRUE;
    }

    void RemoveAll()
    {
        for (int i = 0; i < m_nSize; ++i)
            m_aT[i].~T();
        std::free(m_aT);
        m_aT = nullptr;
        m_nSize = 0;
        m_nAllocSize = 0;
    }

    int Find(const T& t) const
    {
        for (int i = 0; i < m_nSize; ++i)
            if (m_aT[i] == t)
                return i;
        return -1;
    }

private:
    // realloc, as real ATL does: that is what makes m_aT a block the
    // caller can swap for one of its own.
    BOOL Grow(int nNewAllocSize)
    {
        if (nNewAllocSize <= m_nAllocSize)
            return TRUE;
        void* pNew = std::realloc(m_aT, static_cast<size_t>(nNewAllocSize) * sizeof(T));
        if (pNew == nullptr)
            return FALSE;
        m_aT = static_cast<T*>(pNew);
        m_nAllocSize = nNewAllocSize;
        return TRUE;
    }
};
