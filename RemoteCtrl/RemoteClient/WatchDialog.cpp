// WatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "WatchDialog.h"
#include "RemoteClientDlg.h"


// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DIGWATCH, pParent)
{

}

CWatchDialog::~CWatchDialog()
{
}

void CWatchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}


BEGIN_MESSAGE_MAP(CWatchDialog, CDialog)
	ON_WM_TIMER()
	//ON_STN_CLICKED(IDC_WATCH, &CWatchDialog::OnStnClickedWatch)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_STN_CLICKED(IDC_WATCH, &CWatchDialog::OnStnClickedWatch)
END_MESSAGE_MAP()


CPoint CWatchDialog::UserPoint2RemoteScreenPoint(CPoint& point,BOOL isScreen) {
/* 800`450 / 2560`1440 -> 3.2  1920->2.4*/
	CRect clientRect;
	if(isScreen)
		ScreenToClient(&point);
	m_picture.GetWindowRect(clientRect);
	return CPoint(point.x * 2560 / clientRect.Width(), point.y * 1440 / clientRect.Height());
}

// CWatchDialog 消息处理程序
BOOL CWatchDialog::OnInitDialog() {
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	SetTimer(0, 45, NULL);
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CWatchDialog::OnTimer(UINT_PTR nIDEvent) {
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nIDEvent == 0) {
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		if (pParent->isFull()) {
			CRect rect;
			m_picture.GetWindowRect(rect);
			//pParent->GetImage().BitBlt(m_picture.GetDC()->GetSafeHdc(),0,0,SRCCOPY);
			pParent->GetImage().StretchBlt(
				m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(),rect.Height(),SRCCOPY);
			m_picture.InvalidateRect(NULL);
			pParent->GetImage().Destroy();
			pParent->setImageStatus();//FALSE
		}
	}
	CDialog::OnTimer(nIDEvent);
}


//void CWatchDialog::OnStnClickedWatch() {
//	// TODO: 在此添加控件通知处理程序代码
//	
//}

/// \左键双击
void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point) {
	//坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 0;	//左键
	event.nAction = 1;	//双击
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	CDialog::OnLButtonDblClk(nFlags, point);
}

/// \左键按下
void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point) {
	//坐标转换
	TRACE("<<OnLButtonDown>> x=%d  y=%d\r\n",point.x,point.y);
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	TRACE("<<remote changed>> x=%d  y=%d\r\n", point.x, point.y);

	//封装
 	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 0;	//左键
	event.nAction = 2;	//按下
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	CDialog::OnLButtonDown(nFlags, point);
}

/// \左键弹起
void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point) {
	//坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 0;	//左键
	event.nAction = 3;	//弹起
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);

	CDialog::OnLButtonUp(nFlags, point);
}

/// \右键双击
void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point) {
	//坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 1;	//右键
	event.nAction = 1;	//双击
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);

	CDialog::OnRButtonDblClk(nFlags, point);
}

/// \右键按下
void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point) {
	//坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 1;	//右键
	event.nAction = 2;	//按下 //TODO：服务端做对应修改
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	CDialog::OnRButtonDown(nFlags, point);
}

/// \右键弹起
void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point) {
	//坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 1;	//右键
	event.nAction = 3;	//弹起
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);

	CDialog::OnRButtonUp(nFlags, point);
}

/// \鼠标移动
void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point) {
	//坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 8;	//没有按键
	event.nAction = 0;	//移动
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	//TODO:网络通信和对话框有耦合
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	CDialog::OnMouseMove(nFlags, point);
}


void CWatchDialog::OnStnClickedWatch() {
	CPoint point;
	GetCursorPos(&point);
	//坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(point,TRUE);
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 0;	//左键
	event.nAction = 0;	//单击
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
}
