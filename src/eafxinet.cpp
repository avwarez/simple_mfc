#include "eafxinet.h"

BOOL EAfxParseURL(LPCTSTR pstrURL, DWORD& dwServiceType, ECString& strServer,
                 ECString& strObject, INTERNET_PORT& nPort)
{
    if (pstrURL == nullptr)
        return FALSE;
    ECString url(pstrURL);
    if (url.IsEmpty())
        return FALSE;

    int nSchemeEnd = url.Find(_T("://"));
    if (nSchemeEnd < 0)
        return FALSE;
    ECString scheme = url.Left(nSchemeEnd);
    scheme.MakeLower();
    ECString rest = url.Mid(nSchemeEnd + 3);

    INTERNET_PORT nDefaultPort;
    if (scheme == ECString(_T("https"))) { dwServiceType = EAFX_INET_SERVICE_HTTPS; nDefaultPort = 443; }
    else if (scheme == ECString(_T("ftp"))) { dwServiceType = EAFX_INET_SERVICE_FTP; nDefaultPort = 21; }
    else { dwServiceType = EAFX_INET_SERVICE_HTTP; nDefaultPort = 80; }

    int nSlash = rest.Find(_T('/'));
    ECString hostPort = (nSlash >= 0) ? rest.Left(nSlash) : rest;
    strObject = (nSlash >= 0) ? rest.Mid(nSlash) : ECString(_T("/"));
    if (strObject.IsEmpty())
        strObject = _T("/");

    int nColon = hostPort.ReverseFind(_T(':'));
    if (nColon >= 0)
    {
        strServer = hostPort.Left(nColon);
        ECString portStr = hostPort.Mid(nColon + 1);
        long nParsedPort = 0;
        bool bValid = !portStr.IsEmpty();
        for (int i = 0; i < portStr.GetLength() && bValid; ++i)
        {
            TCHAR c = portStr[i];
            if (c < _T('0') || c > _T('9')) { bValid = false; break; }
            nParsedPort = nParsedPort * 10 + (c - _T('0'));
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
