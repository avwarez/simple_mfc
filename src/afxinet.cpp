// afxinet.cpp — AfxParseURL. Pure string parsing (scheme/host/port/
// object), no WinInet dependency.
#include "afxinet.h"

BOOL AfxParseURL(LPCTSTR pstrURL, DWORD& dwServiceType, CString& strServer,
                 CString& strObject, INTERNET_PORT& nPort)
{
    if (pstrURL == nullptr)
        return FALSE;
    CString url(pstrURL);
    if (url.IsEmpty())
        return FALSE;

    // Scheme (defaults to HTTP if none is present, matching real MFC's
    // documented fallback for a schemeless URL).
    int nSchemeEnd = url.Find(L"://");
    CString scheme;
    CString rest = url;
    if (nSchemeEnd >= 0)
    {
        scheme = url.Left(nSchemeEnd);
        scheme.MakeLower();
        rest = url.Mid(nSchemeEnd + 3);
    }

    INTERNET_PORT nDefaultPort;
    if (scheme == CString(L"https")) { dwServiceType = AFX_INET_SERVICE_HTTPS; nDefaultPort = 443; }
    else if (scheme == CString(L"ftp")) { dwServiceType = AFX_INET_SERVICE_FTP; nDefaultPort = 21; }
    else { dwServiceType = AFX_INET_SERVICE_HTTP; nDefaultPort = 80; }

    // host[:port] up to the first '/', which starts the object (path).
    int nSlash = rest.Find(L'/');
    CString hostPort = (nSlash >= 0) ? rest.Left(nSlash) : rest;
    strObject = (nSlash >= 0) ? rest.Mid(nSlash) : CString(L"/");
    if (strObject.IsEmpty())
        strObject = L"/";

    int nColon = hostPort.ReverseFind(L':');
    if (nColon >= 0)
    {
        strServer = hostPort.Left(nColon);
        CString portStr = hostPort.Mid(nColon + 1);
        long nParsedPort = 0;
        bool bValid = !portStr.IsEmpty();
        for (int i = 0; i < portStr.GetLength() && bValid; ++i)
        {
            wchar_t c = portStr[i];
            if (c < L'0' || c > L'9') { bValid = false; break; }
            nParsedPort = nParsedPort * 10 + (c - L'0');
        }
        nPort = (bValid && nParsedPort > 0) ? static_cast<INTERNET_PORT>(nParsedPort) : nDefaultPort;
    }
    else
    {
        strServer = hostPort;
        nPort = nDefaultPort;
    }

    return strServer.IsEmpty() ? FALSE : TRUE;
}
