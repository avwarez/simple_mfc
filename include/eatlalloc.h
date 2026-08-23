#pragma once
#include "eafx.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

template <typename T, int t_nFixedBytes = 128>
class ECTempBuffer
{
public:
    ECTempBuffer() noexcept : m_p(nullptr), m_nElements(0) {}
    explicit ECTempBuffer(size_t nElements) : m_p(nullptr), m_nElements(0) { Allocate(nElements); }
    ~ECTempBuffer() noexcept { Release(); }

    T* Allocate(size_t nElements)
    {
        Release();
        return Reallocate(nElements);
    }
    T* AllocateBytes(size_t nBytes) { return Allocate(nBytes / sizeof(T)); }

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

    operator T*() const noexcept { return m_p; }
    T& operator[](size_t iElement) noexcept { return m_p[iElement]; }
    const T& operator[](size_t iElement) const noexcept { return m_p[iElement]; }

private:
    void Release() noexcept
    {
        if (m_p != nullptr && m_p != FixedPtr())
            std::free(m_p);
        m_p = nullptr;
        m_nElements = 0;
    }

    T* FixedPtr() noexcept { return reinterpret_cast<T*>(m_fixed); }

    alignas(T) unsigned char m_fixed[t_nFixedBytes > 0 ? static_cast<size_t>(t_nFixedBytes) : 1];
    T* m_p;
    size_t m_nElements;
};
