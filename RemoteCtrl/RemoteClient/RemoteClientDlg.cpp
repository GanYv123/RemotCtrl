﻿
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include "ClientController.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx {
public:
	CAboutDlg();

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	// 实现
protected:
	DECLARE_MESSAGE_MAP()
public:

};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX) {
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRemoteClientDlg 对话框



CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, m_server_address(0), m_nPort(_T("")) {
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_server_address);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);
	DDX_Control(pDX, IDC_TREE_DIR, m_Tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_List);
}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_TEST, &CRemoteClientDlg::OnBnClickedBtnTest)
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
	ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)
	ON_COMMAND(ID_DELETE_FILE, &CRemoteClientDlg::OnDeleteFile)
	ON_COMMAND(ID_RUN_FILE, &CRemoteClientDlg::OnRunFile)
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_IPADDRESS_SERV, &CRemoteClientDlg::OnIpnFieldchangedIpaddressServ)
	ON_EN_CHANGE(IDC_EDIT_PORT, &CRemoteClientDlg::OnEnChangeEditPort)
	ON_MESSAGE(WM_SEND_PACK_ACK, &CRemoteClientDlg::OnSendPacketAck)
END_MESSAGE_MAP()


// CRemoteClientDlg 消息处理程序

BOOL CRemoteClientDlg::OnInitDialog() {
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr) {
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty()) {
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	// TODO: 在此添加额外的初始化代码
	InitUIData();
	
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam) {
	if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else {
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint() {
	if (IsIconic()) {
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else {
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteClientDlg::OnQueryDragIcon() {
	return static_cast<HCURSOR>(m_hIcon);
}

void CRemoteClientDlg::OnBnClickedBtnTest() {
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 22233);
}


void CRemoteClientDlg::OnBnClickedBtnFileinfo() {
	std::list<CPacket> lstPackets;
	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 1, TRUE, NULL, 0);
	if (ret == 0) {
		AfxMessageBox(_T("命令处理失败!"));
		return;
	}

}

void CRemoteClientDlg::DealCommand(WORD nCmd, const std::string& strData, LPARAM lParam) {
	switch (nCmd) {
	case 1://获取驱动信息
		Str2Tree(strData, m_Tree);
		break;

	case 2://获取文件信息
		UpdateFileInfo(*((PFILEINFO)strData.c_str()), (HTREEITEM)lParam);
		break;

	case 3://运行文件
		MessageBox(_T("删除文件完成!"), _T("操作完成"), MB_ICONINFORMATION);
		break;

	case 4://下载文件
		UpdateDownloadFile(strData, (FILE*)lParam); 
		break;

	case 9://删除文件
		MessageBox(_T("删除文件完成!"), _T("操作完成"), MB_ICONINFORMATION);
		break;

	case 22233:
		MessageBox(_T("连接测试成功"), _T("连接成功"),MB_ICONINFORMATION);
		break;
	default:
		TRACE("unknow data received! %d\r\n", nCmd);
		break;
	}
}

void CRemoteClientDlg::InitUIData() {
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	UpdateData();
	m_server_address = 0x7f000001;//127.0.0.1
	//m_server_address = 0xC0A88590;//192.168.133.144
	m_nPort = _T("2233");
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_server_address, _ttoi(m_nPort));
	UpdateData(FALSE);
	m_dlgStatus.Create(IDD_DLG_STATUS, this);
	m_dlgStatus.ShowWindow(SW_HIDE);

}

void CRemoteClientDlg::LoadFileCurrent() {
	HTREEITEM hTree = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hTree);
	m_List.DeleteAllItems();
#ifdef _UNICODE
	CStringA strPathA(strPath);
	int nCmd = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, FALSE, (BYTE*)(LPCSTR)strPathA, strPathA.GetLength());
#else
	int nCmd = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, FALSE, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
#endif //#ifdef _UNICODE
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->getPacket().strData.c_str();
	while (pInfo->HasNext) {
		TRACE("OnNMDblclkTreeDir(): %s isDir:%s\r\n",
			pInfo->szFileName, pInfo->IsDirectory == 0 ? "true" : "false");
		if (!pInfo->IsDirectory) {
			CString strFormatted(pInfo->szFileName);
			m_List.InsertItem(0, strFormatted);
		}
		int cmd = CClientController::getInstance()->DealCommand();
		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0) break;
		pInfo = (PFILEINFO)CClientSocket::getInstance()->getPacket().strData.c_str();
	}

	//CClientController::getInstance()->CloseSocket();
}

void CRemoteClientDlg::Str2Tree(const std::string& drivers, CTreeCtrl& tree) {
	std::string dr;
	tree.DeleteAllItems();
	for (size_t i = 0; i < drivers.size(); i++)//C,D,F
	{
		if (drivers[i] == ',') {
			dr += ":";
			CString cstrDr(dr.c_str());
			HTREEITEM hTemp = tree.InsertItem(cstrDr, TVI_ROOT, TVI_LAST);
			tree.InsertItem(NULL, hTemp, TVI_LAST);
			dr.clear();
			continue;
		}
		dr += drivers[i];
	}
	//处理只有一个分区的特殊情况
	if (dr.size() > 0) {
		dr += ":";
		CString cstrDr(dr.c_str());
		HTREEITEM hTemp = tree.InsertItem(cstrDr, TVI_ROOT, TVI_LAST);
	}
}

void CRemoteClientDlg::UpdateFileInfo(const FILEINFO& finfo, HTREEITEM hParent) {
	TRACE("hasNext:%d IsDir:%d fn:%s\r\n", finfo.HasNext, finfo.IsDirectory, finfo.szFileName);
	if (finfo.HasNext == FALSE) return;
	if (finfo.IsDirectory) {
		if (CString(finfo.szFileName) == "." || CString(finfo.szFileName) == "..") {
			return;
		}
		TRACE("hSelected %08X\r\n", hParent);
		CString strFileName(finfo.szFileName);
		HTREEITEM hTemp = m_Tree.InsertItem(strFileName, (HTREEITEM)hParent, TVI_LAST);
		m_Tree.InsertItem(_T(""), hTemp, TVI_LAST);
		m_Tree.Expand((HTREEITEM)hParent, TVE_EXPAND);

	}
	else {
		CString strFormatted(finfo.szFileName);
		m_List.InsertItem(0, strFormatted);
	}
}

void CRemoteClientDlg::UpdateDownloadFile(const std::string& strData, FILE* pFile) {
	static LONGLONG length = 0, index = 0;//length 文件长度  index 已经写入长度
	TRACE("length %d index %d\r\n", length, index);
	if (length == 0) {//文件长度为0 当收到第一个包 文件长度包时
		length = *(long long*)strData.c_str();
		if (length == 0)
		{
			AfxMessageBox(_T("文件长度为零或者无法读取文件!!!"));
			CClientController::getInstance()->DownloadEnd();//结束下载
			return;
		}
	}
	else if ((length > 0) && (index >= length)) {//文件全部写入完成
		fclose(pFile);
		length = 0;
		index = 0;
		CClientController::getInstance()->DownloadEnd();
	}
	else {//写入文件
		fwrite(strData.c_str(), 1, strData.size(), pFile);
		index += strData.size();
		if (index >= length) {
			fclose(pFile);//关闭文件
			length = 0;
			index = 0;
			CClientController::getInstance()->DownloadEnd();//结束下载
		}
	}
}

CString  CRemoteClientDlg::GetPath(HTREEITEM hTree) {
	CString strRet, strTmp;
	do {
		strTmp = m_Tree.GetItemText(hTree);
		strRet = strTmp + '\\' + strRet;
		hTree = m_Tree.GetParentItem(hTree);

	} while (hTree != NULL);

	// 	if (!strRet.IsEmpty() && strRet[strRet.GetLength() - 1] == '\\') {
	// 		strRet = strRet.Left(strRet.GetLength() - 1); // 去除末尾的分隔符
	// 	}

	return strRet;
}

void CRemoteClientDlg::DeleteTreeChildrenItem(HTREEITEM hTree) {
	HTREEITEM hSub = NULL;
	do {
		hSub = m_Tree.GetChildItem(hTree);
		if (hSub != NULL)
			m_Tree.DeleteItem(hSub);
	} while (hSub != NULL);

}

void CRemoteClientDlg::LoadFileInfo() {
	CPoint ptMouse;
	GetCursorPos(&ptMouse);
	m_Tree.ScreenToClient(&ptMouse);
	HTREEITEM hTreeSelected = m_Tree.HitTest(ptMouse, 0);
	if (hTreeSelected == NULL)
		return;

	DeleteTreeChildrenItem(hTreeSelected);
	m_List.DeleteAllItems();
	CString strPath = GetPath(hTreeSelected);
	TRACE("hSelected %08X\r\n", hTreeSelected);
#ifdef _UNICODE
	CStringA strPathA(strPath);
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, FALSE,
		(BYTE*)(LPCSTR)strPathA, strPathA.GetLength(), (WPARAM)hTreeSelected);
#else
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, FALSE,
		(BYTE*)(LPCTSTR)strPath, strPath.GetLength(), (WPARAM)hTreeSelected);
#endif //#ifdef _UNICODE
}



void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult) {
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult) {
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();

}


void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult) {
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	CPoint ptMouse, ptList;
	GetCursorPos(&ptMouse);
	ptList = ptMouse;
	m_List.ScreenToClient(&ptList);
	int listSelected = m_List.HitTest(ptList);
	if (listSelected < 0) return;
	CMenu menu;
	menu.LoadMenu(IDR_MENU_RCLICK);
	CMenu* pPupup = menu.GetSubMenu(0);
	if (pPupup != NULL) {
		pPupup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);
	}
}

//WARIN:下载文件时 若点击了目录信息会导致下载失败
void CRemoteClientDlg::OnDownloadFile() {
	int nListSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nListSelected, 0);
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	strFile = GetPath(hSelected) + strFile;
	int ret = CClientController::getInstance()->DownFile(strFile);
	if (ret != 0) {
		MessageBox(_T("下载失败"));
		TRACE("下载失败 ret = % d\r\n", ret);
	}
}

void CRemoteClientDlg::OnDeleteFile() {
	// TODO: 删除文件
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);
	strFile = strPath + strFile;
	int ret{ -1 };
#ifdef _UNICODE
	CStringA strFileA(strFile);
	ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 9,
		TRUE, (BYTE*)(LPCSTR)strFileA, strFileA.GetLength());
#else
	ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 9,
		TRUE, (BYTE*)(LPCTSTR)strFile, strFile.GetLength());
#endif // _UNICODE
	if (ret < 0) {
		AfxMessageBox(_T("删除文件失败!"));
	}
	LoadFileCurrent();
}


void CRemoteClientDlg::OnRunFile() {
	// TODO: 运行文件
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);
	int ret{ -1 };
#ifdef _UNICODE
	CStringA strFileA(strFile);
	ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 3,
		TRUE, (BYTE*)(LPCSTR)strFileA, strFileA.GetLength());
#else
	ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 3,
		TRUE, (BYTE*)(LPCTSTR)strFile, strFile.GetLength());
#endif // _UNICODE
	if (ret < 0) {
		AfxMessageBox(_T("打开文件失败"));
	}
}

void CRemoteClientDlg::OnBnClickedBtnStartWatch() {
	CClientController::getInstance()->StartWatchScreen();
}

void CRemoteClientDlg::OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult) {
	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	UpdateData();
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_server_address, _ttoi(m_nPort));

	*pResult = 0;
}


void CRemoteClientDlg::OnEnChangeEditPort() {
	UpdateData();
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_server_address, _ttoi(m_nPort));
}

LRESULT CRemoteClientDlg::OnSendPacketAck(WPARAM wParam, LPARAM lParam) {

	if ((lParam == -1) || (lParam == -2)) {
		//TODO:错误处理
		TRACE("socket is error!%d\r\n",lParam);
	}
	else if (lParam == 1) {
		//对方关闭了套接字
		TRACE("socket is closed!\r\n");
	}
	else {
		if (wParam != NULL) {
			CPacket head = *(CPacket*)wParam;
			delete (CPacket*)wParam;
			DealCommand(head.sCmd,head.strData,lParam);
		}
	}
	return 0;
}
