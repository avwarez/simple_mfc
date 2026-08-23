// afxinet.h — simple_mfc (minimal subset).
// MFC's WinInet wrappers (CInternetSession/CHttpConnection/CHttpFile/...)
// are Windows-only networking and largely outside simple_mfc's portable
// scope; only the piece actually needed by consumers is provided here.
//
// eMule/srchybrid includes <afxinet.h> (from HttpDownloadDlg.h) but uses
// none of MFC's WinInet classes — its HTTP download talks to the raw
// Win32 WinInet API (::InternetOpen, INTERNET_PORT, INTERNET_FLAG_*, ...)
// straight from the Windows SDK <wininet.h>, which real MFC's afxinet.h
// pulls in too. The only MFC-level symbol referenced is the free function
// AfxParseURL, declared below -- NATIVE implementation (afxinet.cpp):
// pure string parsing (scheme/host/port/path), no WinInet dependency.
#pragma once

// The service types CInternetSession/CHttpConnection accept. Real MFC
// defines them alongside the connection classes; AfxParseURL's own
// implementation (afxinet.cpp) picks between them based on URL scheme,
// which eMule's only caller (HttpDownloadDlg.cpp) exercises for all three
// (http/https/ftp), even though eMule never spells these constants itself.
#ifndef EAFX_INET_SERVICE_HTTP
#define EAFX_INET_SERVICE_HTTP 3
#endif
#ifndef EAFX_INET_SERVICE_HTTPS
#define EAFX_INET_SERVICE_HTTPS 4
#endif
#ifndef EAFX_INET_SERVICE_FTP
#define EAFX_INET_SERVICE_FTP 1
#endif

#include "eafx.h" // CString, BOOL, DWORD

#ifdef _WIN32
#include <wininet.h> // INTERNET_PORT and the raw WinInet API
#else
// Portable stand-in so the header stays parseable off Windows (nothing in
// simple_mfc's own build includes it there; real WinInet is unavailable).
using INTERNET_PORT = unsigned short;
#endif

// Splits a URL into service type / server / object / port.
BOOL EAfxParseURL(LPCTSTR pstrURL, DWORD& dwServiceType, ECString& strServer,
                 ECString& strObject, INTERNET_PORT& nPort);
