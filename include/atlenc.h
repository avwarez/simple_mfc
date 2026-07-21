// atlenc.h — reference STUB (declarations only, no implementation).
// ATL's text encoding helpers. Only the Base64 encoder is declared: it is
// all eMule/srchybrid uses (SendMail.cpp twice, AsyncProxySocketLayer.cpp
// once, always the Encode direction; the Base64Encoder/Base64Decoder that
// appear in ClientCredits.cpp are Crypto++ classes, not these).
//
// These functions have NO Microsoft Learn reference page -- a search of
// the documentation source returns nothing for Base64Encode. The
// signatures below are therefore taken from the real call sites, which
// compile against real ATL:
//
//   int iLength = Base64EncodeGetRequiredLength(srcA.GetLength(),
//                                               ATL_BASE64_FLAG_NOCRLF);
//   Base64Encode((const BYTE*)(LPCSTR)srcA, srcA.GetLength(),
//                dst.GetBuffer(iLength), &iLength, ATL_BASE64_FLAG_NOCRLF);
//
// and from the second, shorter form in SendMail.cpp:464, which omits the
// flags argument on both calls -- hence the defaults.
#pragma once
#include "afx.h" // BYTE, BOOL, DWORD, LPSTR

// The flag values real atlenc.h defines. NOCRLF is the only one eMule
// passes; NONE is the default the no-flag call sites rely on.
#ifndef ATL_BASE64_FLAG_NONE
#define ATL_BASE64_FLAG_NONE   0
#define ATL_BASE64_FLAG_NOPAD  1
#define ATL_BASE64_FLAG_NOCRLF 2
#endif

// Size of the buffer Base64Encode needs for nSrcLen input bytes.
int Base64EncodeGetRequiredLength(int nSrcLen, DWORD dwFlags = ATL_BASE64_FLAG_NONE) noexcept;

// pnDestLen is in/out: the caller passes the buffer size and gets back the
// number of characters written.
BOOL Base64Encode(const BYTE* pbSrcData, int nSrcLen, LPSTR szDest,
                  int* pnDestLen, DWORD dwFlags = ATL_BASE64_FLAG_NONE) noexcept;
