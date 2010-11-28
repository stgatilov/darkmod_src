/***************************************************************************
 *
 * PROJECT: The Dark Mod - Updater
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#pragma once

#include "Updater/UpdaterOptions.h"
#include "afxwin.h"

// AdvancedOptionsDialog dialog

class AdvancedOptionsDialog : public CDialog
{
	DECLARE_DYNAMIC(AdvancedOptionsDialog)

private:
	tdm::updater::UpdaterOptions& _options;

public:
	AdvancedOptionsDialog(tdm::updater::UpdaterOptions& options, CWnd* pParent = NULL);   // standard constructor
	virtual ~AdvancedOptionsDialog();

// Dialog Data
	enum { IDD = IDD_ADV_OPTIONS_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	HICON m_hIcon;

	virtual BOOL OnInitDialog();
public:
	afx_msg void OnBnClickedOk();
	CButton _keepMirrors;
	CButton _noSelfUpdate;
	CButton _keepUpdatePackages;
	CEdit _targetFolder;

private:
	void LoadValuesFromOptions();
	void SaveValuesToOptions();
public:
	CEdit _targetDir;
	CEdit _proxy;
	CStatic _labelBehaviour;
	CStatic _labelTargetDir;
	CStatic _labelProxy;
	CFont _boldFont;
};
