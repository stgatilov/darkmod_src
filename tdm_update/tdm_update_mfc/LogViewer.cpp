// LogViewer.cpp : implementation file
//

#include "stdafx.h"
#include "tdm_update_mfc.h"
#include "LogViewer.h"
#include <boost/algorithm/string/replace.hpp>

// LogViewer dialog

IMPLEMENT_DYNAMIC(LogViewer, CDialog)

LogViewer::LogViewer(CWnd* pParent /*=NULL*/)
	: CDialog(LogViewer::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDI_TDM_ICON);
}

LogViewer::~LogViewer()
{
}

BOOL LogViewer::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetIcon(m_hIcon, FALSE);

	// Remove the default text limit from edit controls
	_logView.SetLimitText(0);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void LogViewer::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LOGVIEW, _logView);
}

void LogViewer::WriteLog(LogClass lc, const std::string& str)
{
	if (m_hWnd == NULL) return;

	// get the initial text length
	int nLength = _logView.GetWindowTextLength();

	// put the selection at the end of text
	_logView.SetSel(nLength, nLength);

	// replace the \n character with \r\n for multiline edits
	std::string tweaked = boost::algorithm::replace_all_copy(str, "\n", "\r\n");

	_logView.ReplaceSel(CString(tweaked.c_str()));
}

BEGIN_MESSAGE_MAP(LogViewer, CDialog)
	ON_BN_CLICKED(IDOK, &LogViewer::OnBnClickedOk)
END_MESSAGE_MAP()


// LogViewer message handlers


void LogViewer::OnBnClickedOk()
{
	ShowWindow(SW_HIDE);
}
