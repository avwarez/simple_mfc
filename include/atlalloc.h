// atlalloc.h — reference STUB (declarations only, no implementation).
// CTempBuffer, ATL's "small on the stack, large on the heap" scratch
// buffer. eMule/srchybrid uses it in MediaInfo.cpp to receive variable
// length attribute data from the Windows Media header interfaces:
// declared empty, sized with Allocate(), then indexed.
//
// Real ATL takes the stack-allocation threshold as a second template
// parameter with a default, which is why eMule can write CTempBuffer<WORD>
// with one argument.
#pragma once
#include "afx.h"

template <typename T, int t_nFixedBytes = 128>
class CTempBuffer
{
public:
    CTempBuffer() noexcept;
    explicit CTempBuffer(size_t nElements);
    ~CTempBuffer() noexcept;

    T* Allocate(size_t nElements);
    T* AllocateBytes(size_t nBytes);
    T* Reallocate(size_t nElements);
    void Free() noexcept;

    operator T*() const noexcept;
    T& operator[](size_t iElement) noexcept;
    const T& operator[](size_t iElement) const noexcept;
};
