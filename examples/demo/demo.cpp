// examples/demo/demo.cpp — a complete little MFC application, written the way
// an MFC application is written, that links against simple_mfc and runs on Qt.
//
// There is deliberately NOTHING here that knows about Qt, about a driver, or
// about this port at all. No main(): the framework supplies it, as MFC's
// library supplies WinMain. If this file compiled against real MFC on Windows
// it would be an ordinary dialog-based application, and that is the whole
// point of the exercise.
//
// What it exercises, end to end:
//   - CWinApp + InitInstance + the message pump + ExitInstance;
//   - a dialog built from a .rc template, with DDX/DDV moving data;
//   - a message map routing a button click to a handler;
//   - an owner-draw CListCtrl painting its own rows through DrawItem;
//   - GDI: CDC, CBrush/CPen/CFont, FillSolidRect, TextOut.
#include "afxwin.h"
#include "afxcmn.h"
#include "resource.h"

// ---------------------------------------------------------------------------
// An owner-draw list, the shape eMule's CMuleListCtrl-derived lists take: it
// overrides DrawItem and paints every row itself. Nothing asks for owner-draw;
// the template's LVS_OWNERDRAWFIXED is what turns it on.
// ---------------------------------------------------------------------------
class CTaskList : public CListCtrl
{
public:
    void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) override
    {
        CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
        if (pDC == nullptr)
            return;

        const CRect rc(lpDrawItemStruct->rcItem);
        const bool  bSelected = (lpDrawItemStruct->itemState & 0x0001) != 0;
        const bool  bAlt = (lpDrawItemStruct->itemID % 2) != 0;

        // Alternating row shading, and a highlight when selected.
        const COLORREF crBack = bSelected ? RGB(51, 103, 179)
                                          : (bAlt ? RGB(245, 245, 245) : RGB(255, 255, 255));
        pDC->FillSolidRect(&rc, crBack);

        // A progress-style bar whose width comes from the row's item data,
        // which is how a real owner-draw list carries per-row state.
        const int nPercent = static_cast<int>(lpDrawItemStruct->itemData);
        CRect bar(rc);
        bar.left += 4;
        bar.top += 3;
        bar.bottom -= 3;
        bar.right = bar.left + ((rc.Width() - 8) * nPercent) / 100;
        pDC->FillSolidRect(&bar, bSelected ? RGB(140, 180, 240) : RGB(120, 190, 120));

        pDC->SetBkMode(1 /*TRANSPARENT*/);
        pDC->SetTextColor(bSelected ? RGB(255, 255, 255) : RGB(20, 20, 20));
        CString strText;
        strText.Format(_T("task %u  -  %d%%"),
                       static_cast<unsigned>(lpDrawItemStruct->itemID), nPercent);
        pDC->TextOut(rc.left + 8, rc.top + 2, strText);
    }
};

// ---------------------------------------------------------------------------
// The main dialog.
// ---------------------------------------------------------------------------
class CDemoDlg : public CDialog
{
public:
    CDemoDlg() : CDialog(IDD_DEMO_DIALOG) {}

    CString   m_strName;
    BOOL      m_bEnabled = TRUE;
    CTaskList m_wndTasks;

protected:
    void DoDataExchange(CDataExchange* pDX) override
    {
        CDialog::DoDataExchange(pDX);
        DDX_Text(pDX, IDC_NAME_EDIT, m_strName);
        DDX_Check(pDX, IDC_ENABLE_CHECK, m_bEnabled);
        DDX_Control(pDX, IDC_TASK_LIST, m_wndTasks);
    }

    BOOL OnInitDialog() override
    {
        CDialog::OnInitDialog();
        m_strName = _T("world");
        UpdateData(FALSE);              // members -> controls

        // Fill the list. The per-row percentage rides along as item data and
        // is what DrawItem above reads back.
        static const int kPercent[] = { 100, 72, 45, 18, 5 };
        for (int i = 0; i < 5; ++i) {
            CString s;
            s.Format(_T("task %d"), i);
            m_wndTasks.InsertItem(i, s);
            m_wndTasks.SetItemData(i, static_cast<DWORD_PTR>(kPercent[i]));
        }
        return TRUE;
    }

    // Painted through the WM_PAINT -> OnPaint route, with a live CDC.
    void OnPaint() override
    {
        CPaintDC dc(this);
        CRect rc;
        GetClientRect(&rc);
        // A subtle band behind the header row, drawn with real GDI calls.
        dc.FillSolidRect(0, 0, rc.Width(), 24, RGB(238, 242, 248));
    }

    // ON_BN_CLICKED(IDC_GREET_BUTTON, OnGreet) in the map below.
    afx_msg void OnGreet()
    {
        UpdateData(TRUE);               // controls -> members
        CString strMsg;
        strMsg.Format(_T("Hello, %s!"), (LPCTSTR)m_strName);
        AfxMessageBox(strMsg, MB_OK | MB_ICONINFORMATION);
    }

    DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CDemoDlg, CDialog)
    ON_BN_CLICKED(IDC_GREET_BUTTON, OnGreet)
END_MESSAGE_MAP()

// ---------------------------------------------------------------------------
// The application object. Constructing it at file scope is the whole of the
// registration - AfxGetApp() answers with it from that moment on.
// ---------------------------------------------------------------------------
class CDemoApp : public CWinApp
{
public:
    CDemoApp() : CWinApp(_T("simple_mfc demo")) {}

    BOOL InitInstance() override
    {
        m_pDlg = new CDemoDlg();
        if (!m_pDlg->Create(IDD_DEMO_DIALOG))
            return FALSE;               // refusing to start: MFC skips the pump
        m_pDlg->ShowWindow(SW_SHOW);
        m_pMainWnd = m_pDlg;            // destroying it ends the application
        return TRUE;
    }

    int ExitInstance() override
    {
        delete m_pDlg;
        m_pDlg = nullptr;
        return CWinApp::ExitInstance();
    }

private:
    CDemoDlg* m_pDlg = nullptr;
};

CDemoApp theApp;
