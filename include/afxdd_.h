// afxdd_.h — reference STUB for Dialog Data Exchange/Validation
// (DDX_*/DDV_* functions, header per Microsoft Learn: afxdd_.h).
// CDataExchange itself is NOT declared here: its real header is afxwin.h
// (see afxwin.h) — Microsoft Learn: "defined in afxdd_.h, however
// applications should include afxwin.h", consistent with afxdd_.h acting
// as a "service" header included by afxwin.h in real-world practice.
#pragma once
#include "afx.h"
#include "afxwin.h"

class COleCurrency;
class COleDateTime;

// ---------------------------------------------------------------------
// DDX functions (Dialog Data Exchange) — full signatures verified
// ---------------------------------------------------------------------
void AFXAPI DDX_Control(CDataExchange* pDX, int nIDC, CWnd& rControl);

void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, unsigned char& value);
void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, short& value);
void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, int& value);
void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, UINT& value);
void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, long& value);
void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, DWORD& value);
void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, CString& value);
void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, float& value);
void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, double& value);
void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, COleCurrency& value);
void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, COleDateTime& value);

void AFXAPI DDX_Check(CDataExchange* pDX, int nIDC, int& value);
void AFXAPI DDX_Radio(CDataExchange* pDX, int nIDC, int& value);
void AFXAPI DDX_CBIndex(CDataExchange* pDX, int nIDC, int& index);
void AFXAPI DDX_CBString(CDataExchange* pDX, int nIDC, CString& value);
void AFXAPI DDX_CBStringExact(CDataExchange* pDX, int nIDC, CString& value);
void AFXAPI DDX_LBIndex(CDataExchange* pDX, int nIDC, int& index);
void AFXAPI DDX_LBString(CDataExchange* pDX, int nIDC, CString& value);
void AFXAPI DDX_LBStringExact(CDataExchange* pDX, int nIDC, CString& value);
void AFXAPI DDX_Scroll(CDataExchange* pDX, int nIDC, int& value);
void AFXAPI DDX_Slider(CDataExchange* pDX, int nIDC, int& value);

// ---------------------------------------------------------------------
// DDV functions (Dialog Data Validation)
// ---------------------------------------------------------------------
void AFXAPI DDV_MinMaxInt(CDataExchange* pDX, int value, int minVal, int maxVal);
void AFXAPI DDV_MinMaxFloat(CDataExchange* pDX, float value, float minVal, float maxVal);
void AFXAPI DDV_MinMaxDouble(CDataExchange* pDX, double value, double minVal, double maxVal);
void AFXAPI DDV_MinMaxUInt(CDataExchange* pDX, UINT value, UINT minVal, UINT maxVal);
void AFXAPI DDV_MinMaxLong(CDataExchange* pDX, long value, long minVal, long maxVal);
