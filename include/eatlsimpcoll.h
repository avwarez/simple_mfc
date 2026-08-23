#pragma once

#include "eafx.h"

#include <cstdlib>
#include <cstring>
#include <new>

template <class T, class TEqual = void>
class ECSimpleArray
{
public:
    T* m_aT;
    int m_nSize;
    int m_nAllocSize;

    ECSimpleArray() : m_aT(nullptr), m_nSize(0), m_nAllocSize(0) {}

    ECSimpleArray(const ECSimpleArray<T, TEqual>& src) : m_aT(nullptr), m_nSize(0), m_nAllocSize(0)
    {
        for (int i = 0; i < src.GetSize(); ++i)
            Add(src[i]);
    }

    ECSimpleArray<T, TEqual>& operator=(const ECSimpleArray<T, TEqual>& src)
    {
        if (this != &src) {
            RemoveAll();
            for (int i = 0; i < src.GetSize(); ++i)
                Add(src[i]);
        }
        return *this;
    }

    ~ECSimpleArray() { RemoveAll(); }

    int GetSize() const { return m_nSize; }

    T& operator[](int nIndex) { return m_aT[nIndex]; }
    const T& operator[](int nIndex) const { return m_aT[nIndex]; }

    T* GetData() const { return m_aT; }

    BOOL Add(const T& t)
    {
        if (m_nSize == m_nAllocSize && !Grow(m_nAllocSize == 0 ? 4 : m_nAllocSize * 2))
            return FALSE;
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
