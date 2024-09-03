// LockInfoDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteCtrl.h"
#include "afxdialogex.h"
#include "LockInfoDialog.h"


// CLockInfoDialog 对话框

IMPLEMENT_DYNAMIC(CLockInfoDialog, CDialog)

CLockInfoDialog::CLockInfoDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DIALOG1, pParent)
{

}

CLockInfoDialog::~CLockInfoDialog()
{
}

void CLockInfoDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CLockInfoDialog, CDialog)
	ON_WM_SIZE()
END_MESSAGE_MAP()


// CLockInfoDialog 消息处理程序
void CLockInfoDialog::OnSize(UINT nType, int cx, int cy) {
	CDialog::OnSize(nType, cx, cy);

	// 确保控件已经被创建
	if (GetDlgItem(ID_STATICE_TXT1) != nullptr) {
		// 获取控件指针
		CWnd* pWnd = GetDlgItem(ID_STATICE_TXT1);

		// 获取窗口客户区大小
		CRect rectClient;
		GetClientRect(&rectClient);

		// 获取控件的当前大小
		CRect rectControl;
		pWnd->GetWindowRect(&rectControl);
		ScreenToClient(&rectControl);

		// 计算新的位置，使控件居中
		int nWidth = rectControl.Width();
		int nHeight = rectControl.Height();
		int nLeft = (rectClient.Width() - nWidth) / 2;
		int nTop = (rectClient.Height() - nHeight) / 2;

		// 调整控件的大小和位置
		pWnd->MoveWindow(nLeft, nTop, nWidth, nHeight, TRUE);
	}
}
