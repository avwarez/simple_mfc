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
// AfxParseURL, declared below.
#pragma once

// The service types CInternetSession/CHttpConnection accept. Real MFC
// defines them alongside the connection classes; eMule selects HTTPS
// explicitly when it fetches over TLS.
#ifndef AFX_INET_SERVICE_HTTP
#define AFX_INET_SERVICE_HTTP 3
#endif
#ifndef AFX_INET_SERVICE_HTTPS
#define AFX_INET_SERVICE_HTTPS 4
#endif
#ifndef AFX_INET_SERVICE_FTP
#define AFX_INET_SERVICE_FTP 1
#endif

#include "afx.h" // CString, BOOL, DWORD

#ifdef _WIN32
#include <wininet.h> // INTERNET_PORT and the raw WinInet API
#else
// Portable stand-in so the header stays parseable off Windows (nothing in
// simple_mfc's own build includes it there; real WinInet is unavailable).
using INTERNET_PORT = unsigned short;
#endif

// Splits a URL into service type / server / object / port. Compile-time
// declaration only; the WinInet-backed definition lives in the platform's
// real MFC and is never linked in simple_mfc's declaration-only frontend.
BOOL AfxParseURL(LPCTSTR pstrURL, DWORD& dwServiceType, CString& strServer,
                 CString& strObject, INTERNET_PORT& nPort);
