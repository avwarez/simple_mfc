// atlenc.cpp — Base64Encode/Base64EncodeGetRequiredLength. Textbook
// Base64 (RFC 4648), pure integer/byte arithmetic, no platform
// dependency.
#include "atlenc.h"

namespace
{
constexpr char kBase64Chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Groups of 4 output characters per 76-column MIME line (76 / 4 = 19).
constexpr int kGroupsPerLine = 19;
} // namespace

int Base64EncodeGetRequiredLength(int nSrcLen, DWORD dwFlags) noexcept
{
    if (nSrcLen <= 0)
        return 0;
    int nGroups = (nSrcLen + 2) / 3;
    int nChars = nGroups * 4;
    if (dwFlags & ATL_BASE64_FLAG_NOPAD)
    {
        int nRem = nSrcLen % 3;
        if (nRem == 1) nChars -= 2;
        else if (nRem == 2) nChars -= 1;
    }
    if (!(dwFlags & ATL_BASE64_FLAG_NOCRLF) && nGroups > 0)
    {
        int nLines = (nGroups - 1) / kGroupsPerLine;
        nChars += nLines * 2;
    }
    return nChars;
}

BOOL Base64Encode(const BYTE* pbSrcData, int nSrcLen, LPSTR szDest,
                  int* pnDestLen, DWORD dwFlags) noexcept
{
    if (pnDestLen == nullptr || nSrcLen < 0)
        return FALSE;

    int nNeeded = Base64EncodeGetRequiredLength(nSrcLen, dwFlags);
    if (szDest == nullptr)
    {
        *pnDestLen = nNeeded;
        return TRUE;
    }
    if (*pnDestLen < nNeeded)
        return FALSE;

    const bool bPad = !(dwFlags & ATL_BASE64_FLAG_NOPAD);
    const bool bCrlf = !(dwFlags & ATL_BASE64_FLAG_NOCRLF);

    int nOut = 0;
    int nGroupsWritten = 0;
    for (int i = 0; i < nSrcLen; i += 3)
    {
        unsigned int b0 = pbSrcData[i];
        unsigned int b1 = (i + 1 < nSrcLen) ? pbSrcData[i + 1] : 0;
        unsigned int b2 = (i + 2 < nSrcLen) ? pbSrcData[i + 2] : 0;
        int nAvail = nSrcLen - i; // 1, 2, or >=3 bytes left in this group

        szDest[nOut++] = kBase64Chars[(b0 >> 2) & 0x3F];
        szDest[nOut++] = kBase64Chars[((b0 << 4) | (b1 >> 4)) & 0x3F];
        if (nAvail >= 3)
        {
            szDest[nOut++] = kBase64Chars[((b1 << 2) | (b2 >> 6)) & 0x3F];
            szDest[nOut++] = kBase64Chars[b2 & 0x3F];
        }
        else if (nAvail == 2)
        {
            szDest[nOut++] = kBase64Chars[(b1 << 2) & 0x3F];
            if (bPad) szDest[nOut++] = '=';
        }
        else
        {
            if (bPad) { szDest[nOut++] = '='; szDest[nOut++] = '='; }
        }
        ++nGroupsWritten;

        if (bCrlf && nGroupsWritten % kGroupsPerLine == 0 && i + 3 < nSrcLen)
        {
            szDest[nOut++] = '\r';
            szDest[nOut++] = '\n';
        }
    }
    *pnDestLen = nOut;
    return TRUE;
}
