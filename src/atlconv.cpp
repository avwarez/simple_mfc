// atlconv.cpp — AtlUnicodeToUTF8/_AtlGetConversionACP. UTF-16 -> UTF-8 is
// pure bit manipulation on code points (no iconv/MultiByteToWideChar
// dependency), so it needs nothing beyond standard C++.
#include "atlconv.h"

#include <cstdint>

int AtlUnicodeToUTF8(LPCWSTR wszSrc, int nSrc, LPSTR szDest, int nDest) noexcept
{
    if (wszSrc == nullptr)
        return 0;

    // nSrc == -1 means "null-terminated, measure it myself", matching
    // WideCharToMultiByte's convention (which real ATL forwards to) --
    // the terminator itself is included in the count, same as there.
    int nChars = nSrc;
    if (nChars < 0)
    {
        nChars = 0;
        while (wszSrc[nChars] != 0)
            ++nChars;
        ++nChars;
    }

    int nOut = 0;
    for (int i = 0; i < nChars; ++i)
    {
        uint32_t cp = static_cast<uint16_t>(wszSrc[i]);
        // A leading surrogate followed by a trailing one decodes to a
        // single astral code point (this is UTF-16, not UCS-2).
        if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < nChars)
        {
            uint32_t lo = static_cast<uint16_t>(wszSrc[i + 1]);
            if (lo >= 0xDC00 && lo <= 0xDFFF)
            {
                cp = 0x10000u + ((cp - 0xD800u) << 10) + (lo - 0xDC00u);
                ++i;
            }
        }

        unsigned char buf[4];
        int n;
        if (cp < 0x80u) { buf[0] = static_cast<unsigned char>(cp); n = 1; }
        else if (cp < 0x800u)
        {
            buf[0] = static_cast<unsigned char>(0xC0u | (cp >> 6));
            buf[1] = static_cast<unsigned char>(0x80u | (cp & 0x3Fu));
            n = 2;
        }
        else if (cp < 0x10000u)
        {
            buf[0] = static_cast<unsigned char>(0xE0u | (cp >> 12));
            buf[1] = static_cast<unsigned char>(0x80u | ((cp >> 6) & 0x3Fu));
            buf[2] = static_cast<unsigned char>(0x80u | (cp & 0x3Fu));
            n = 3;
        }
        else
        {
            buf[0] = static_cast<unsigned char>(0xF0u | (cp >> 18));
            buf[1] = static_cast<unsigned char>(0x80u | ((cp >> 12) & 0x3Fu));
            buf[2] = static_cast<unsigned char>(0x80u | ((cp >> 6) & 0x3Fu));
            buf[3] = static_cast<unsigned char>(0x80u | (cp & 0x3Fu));
            n = 4;
        }

        if (szDest != nullptr)
        {
            // Matches WideCharToMultiByte's "insufficient buffer" failure
            // (it returns 0 rather than a partial conversion).
            if (nOut + n > nDest)
                return 0;
            for (int k = 0; k < n; ++k)
                szDest[nOut + k] = static_cast<char>(buf[k]);
        }
        nOut += n;
    }
    return nOut;
}

UINT _AtlGetConversionACP() noexcept
{
    // There is no portable notion of a Windows "ANSI code page" off
    // Windows. 0 doubles as CP_ACP's real numeric value, so this reads
    // the same way on both sides: "the system default ANSI code page."
    return 0;
}
