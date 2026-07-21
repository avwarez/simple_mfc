// atlalloc.h — NATIVE implementation (standard C++17 library only).
// CTempBuffer, ATL's "small on the stack, large on the heap" scratch
// buffer. eMule/srchybrid uses it in MediaInfo.cpp to receive variable
// length attribute data from the Windows Media header interfaces:
// declared empty, sized with Allocate(), then indexed.
//
// Real ATL takes the stack-allocation threshold as a second template
// parameter with a default, which is why eMule can write CTempBuffer<WORD>
// with one argument. A template, so (like afxtempl.h) it is necessarily
// header-only -- there is no fixed set of T's to instantiate in a .cpp.
#pragma once
#include "afx.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

template <typename T, int t_nFixedBytes = 128>
class CTempBuffer
{
public:
    CTempBuffer() noexcept : m_p(nullptr), m_nElements(0) {}
    explicit CTempBuffer(size_t nElements) : m_p(nullptr), m_nElements(0) { Allocate(nElements); }
    ~CTempBuffer() noexcept { Free(); }

    T* Allocate(size_t nElements)
    {
        Free();
        return Reallocate(nElements);
    }
    T* AllocateBytes(size_t nBytes) { return Allocate(nBytes / sizeof(T)); }

    // Grows or shrinks in place, preserving as much of the existing
    // content as still fits -- real ATL's Reallocate has the same
    // preserve-on-resize contract.
    T* Reallocate(size_t nElements)
    {
        size_t nBytes = nElements * sizeof(T);
        bool wasFixed = (m_p == FixedPtr());
        if (nBytes <= static_cast<size_t>(t_nFixedBytes))
        {
            if (!wasFixed && m_p != nullptr)
            {
                std::memcpy(m_fixed, m_p, std::min<size_t>(nElements, m_nElements) * sizeof(T));
                std::free(m_p);
            }
            m_p = FixedPtr();
        }
        else
        {
            T* pNew = static_cast<T*>(wasFixed || m_p == nullptr ? std::malloc(nBytes) : std::realloc(m_p, nBytes));
            if (pNew == nullptr)
                return nullptr;
            if (wasFixed && m_p != nullptr)
                std::memcpy(pNew, m_fixed, std::min<size_t>(nElements, m_nElements) * sizeof(T));
            m_p = pNew;
        }
        m_nElements = nElements;
        return m_p;
    }

    void Free() noexcept
    {
        if (m_p != nullptr && m_p != FixedPtr())
            std::free(m_p);
        m_p = nullptr;
        m_nElements = 0;
    }

    operator T*() const noexcept { return m_p; }
    T& operator[](size_t iElement) noexcept { return m_p[iElement]; }
    const T& operator[](size_t iElement) const noexcept { return m_p[iElement]; }

private:
    T* FixedPtr() noexcept { return reinterpret_cast<T*>(m_fixed); }

    alignas(T) unsigned char m_fixed[t_nFixedBytes > 0 ? static_cast<size_t>(t_nFixedBytes) : 1];
    T* m_p;
    size_t m_nElements;
};
