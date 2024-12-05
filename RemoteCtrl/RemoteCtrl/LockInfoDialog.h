#pragma once
#include "afxdialogex.h"


// CLockInfoDialog 对话框



class CLockInfoDialog : public CDialog
{
	DECLARE_DYNAMIC(CLockInfoDialog)

public:
	CLockInfoDialog(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CLockInfoDialog();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_INFO };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
};
