#pragma once
#include "afxdialogex.h"


// CWatchDialog 对话框

class CWatchDialog : public CDialog
{
	DECLARE_DYNAMIC(CWatchDialog)

public:
	CWatchDialog(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CWatchDialog();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIGWATCH };
#endif
public:
	int m_nObjWidth;
	int m_nObjHeight;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	BOOL m_isFull;//缓存是否满 true->有 false->无
	DECLARE_MESSAGE_MAP()
public:
	BOOL isFull() const { return m_isFull; }
	void setImageStatus(BOOL isFull = FALSE) {
		m_isFull = isFull;
	}
	CPoint UserPoint2RemoteScreenPoint(CPoint& point, BOOL isScreen = FALSE);
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	// 屏幕图片显示
	CStatic m_picture;
	//afx_msg void OnStnClickedWatch();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnStnClickedWatch();
	//virtual void OnOK();
	afx_msg void OnBnClickedBtnLock();
	afx_msg void OnBnClickedBtnUnlock();
};
