﻿
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"

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
	, m_server_address(0), m_nPort(_T("")), m_isFull(FALSE) {
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_server_address);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);
	DDX_Control(pDX, IDC_TREE_DIR, m_Tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_List);
}

BOOL CRemoteClientDlg::isFull() const { return m_isFull; }

CImage& CRemoteClientDlg::GetImage() { return m_image; }

void CRemoteClientDlg::setImageStatus(BOOL IsFull) {
	m_isFull = IsFull;
}

int CRemoteClientDlg::sendCommandPacket(int nCmd, BOOL bAutoClose, BYTE* pData, size_t nLength) {
	UpdateData();
	CClientSocket* pClient = CClientSocket::getInstance();
	BOOL ret = pClient->initSocket(m_server_address, _ttoi(m_nPort));
	if (!ret) {
		AfxMessageBox(_T("网络初始化失败"));
		return -1;
	}
	CPacket pack(nCmd, pData, nLength);
	ret = pClient->Send(pack);
	TRACE("Send ret: %d\r\n", ret);
	int cmd = pClient->DealCommand();
	TRACE("ack:%d\r\n", cmd);
	if (bAutoClose)
		pClient->CloseSocket();
	return cmd;
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
	ON_MESSAGE(WM_SEND_PACKET, &CRemoteClientDlg::OnSendPacket)
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
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
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	UpdateData();
	m_server_address = 0x7f000001;
	m_nPort = _T("2323");
	UpdateData(FALSE);
	m_dlgStatus.Create(IDD_DLG_STATUS, this);
	m_dlgStatus.ShowWindow(SW_HIDE);
	m_isFull = FALSE;

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
	sendCommandPacket(22233);
}


void CRemoteClientDlg::OnBnClickedBtnFileinfo() {
	int ret = sendCommandPacket(1);
	if (ret == -1) {
		AfxMessageBox(_T("命令处理失败!"));
		return;
	}
	CClientSocket* pClient = CClientSocket::getInstance();
	std::string drivers = pClient->getPacket().strData;
	std::string dr;
	m_Tree.DeleteAllItems();
	for (size_t i = 0; i < drivers.size(); i++)//C,D,F
	{
		if (drivers[i] == ',') {
			dr += ":";
			{
				CString cstrDr(dr.c_str());
				HTREEITEM hTemp = m_Tree.InsertItem(cstrDr, TVI_ROOT, TVI_LAST);
				m_Tree.InsertItem(NULL, hTemp, TVI_LAST);
			}
			dr.clear();
			continue;
		}
		dr += drivers[i];
	}

}

void CRemoteClientDlg::threadEntryForWatchData(void* arg) {
	CRemoteClientDlg* This = (CRemoteClientDlg*)arg;
	This->threadWatchData();
	_endthread();
}

void CRemoteClientDlg::threadWatchData() {
	Sleep(50);
	CClientSocket* pClient{ nullptr };
	do {
		pClient = CClientSocket::getInstance();
	} while (pClient == nullptr);
	ULONGLONG tick = GetTickCount64();
	for (;;) {
		if (m_isFull == FALSE) {//如果缓存为空，放入缓存
			int ret = SendMessage(WM_SEND_PACKET, 6 << 1 | 1);
			if (ret == 6) {

				BYTE* pData = (BYTE*)pClient->getPacket().strData.c_str();//todo:存入CImage
				HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
				if (hMem == NULL) {
					TRACE("内存不足");
					Sleep(1);
					continue;
				}      

				IStream* pStream = NULL;
				HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
				if (hRet == S_OK) {
					ULONG length = 0;
					pStream->Write(pData, pClient->getPacket().strData.size(), &length);
					LARGE_INTEGER bg = { 0 };
					pStream->Seek(bg, STREAM_SEEK_SET, NULL);
					m_image.Load(pStream);
					m_isFull = TRUE;
				}
				// 释放 GlobalAlloc 分配的内存
				GlobalFree(hMem);
			}
			else {
				Sleep(1);//预防CPU飙高
			}
		}
		else {
			Sleep(1);
		}

	}
}

void CRemoteClientDlg::threadEntryForDownFile(void* arg) {
	CRemoteClientDlg* This = (CRemoteClientDlg*)arg;
	This->threadDownFile();
	_endthread();
}

void CRemoteClientDlg::threadDownFile() {
	int nListSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nListSelected, 0);
	CFileDialog dlg(FALSE, _T("*"), strFile, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, this);

	if (dlg.DoModal() == IDOK) {
		FILE* pFile{ nullptr };
		errno_t err;
#ifdef _UNICODE
		CStringA pFileA(dlg.GetPathName());
		err = fopen_s(&pFile, pFileA, "wb+");
#else
		err = fopen_s(&pFile, dlg.GetPathName(), "wb+");
#endif// _UNICODE
		if (err != 0) {
			AfxMessageBox(_T("本地无权限，文件无法创建!"));
			m_dlgStatus.ShowWindow(SW_HIDE);
			EndWaitCursor();
			return;
		}
		HTREEITEM hSelected = m_Tree.GetSelectedItem();
		strFile = GetPath(hSelected) + strFile;
		/*
		TRACE("%s\r\n", strFile);
		int ret{ -1 };
#ifdef _UNICODE
		CStringA strFileA(strFile);
		ret = sendCommandPacket(4, FALSE, (BYTE*)(LPCSTR)strFileA, strFileA.GetLength());
#else
		ret = sendCommandPacket(4, FALSE, (BYTE*)(LPCTSTR)strFile, strFile.GetLength());
#endif // _UNICODE
	//*/
		int ret = SendMessage(WM_SEND_PACKET, 4 << 1 | 0, (LPARAM)(LPCTSTR)strFile);
		CClientSocket* pClient = CClientSocket::getInstance();
		do {
			if (ret < 0) {
				AfxMessageBox(_T("执行下载命令失败!!！"));
				TRACE("执行下载失败:ret = %d\r\n", ret);
				break;
			}
			long long nLength = *(long long*)pClient->getPacket().strData.c_str();
			if (nLength == 0) {
				AfxMessageBox(_T("文件长度为0，无法读取文件!"));
				break;
			}
			long long nCount = 0;
			//----------添加线程函数

			while (nCount < nLength) {
				pClient->DealCommand();
				if (ret < 0) {
					AfxMessageBox(_T("传输失败"));
					TRACE("传输失败:ret = %d\r\n", ret);
					break;
				}
				//pClient->getPacket().strData.c_str();
				fwrite(pClient->getPacket().strData.c_str(), 1, pClient->getPacket().strData.size(), pFile);
				nCount += pClient->getPacket().strData.size();
			}
		} while (FALSE);
		fclose(pFile);
		pClient->CloseSocket();
	}
	m_dlgStatus.ShowWindow(SW_HIDE);
	EndWaitCursor();
	MessageBox(_T("下载完成!"), _T("完成"));
}

void CRemoteClientDlg::LoadFileCurrent() {
	HTREEITEM hTree = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hTree);
	m_List.DeleteAllItems();
#ifdef _UNICODE
	CStringA strPathA(strPath);
	int nCmd = sendCommandPacket(2, FALSE, (BYTE*)(LPCSTR)strPathA, strPathA.GetLength());
#else
	int nCmd = sendCommandPacket(2, FALSE, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
#endif //#ifdef _UNICODE

	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->getPacket().strData.c_str();
	CClientSocket* pClient = CClientSocket::getInstance();
	while (pInfo->HasNext) {
		TRACE("OnNMDblclkTreeDir(): %s isDir:%s\r\n",
			pInfo->szFileName, pInfo->IsDirectory == 0 ? "true" : "false");
		if (!pInfo->IsDirectory) {
			CString strFormatted(pInfo->szFileName);
			m_List.InsertItem(0, strFormatted);
		}
		int cmd = pClient->DealCommand();
		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0) break;
		pInfo = (PFILEINFO)CClientSocket::getInstance()->getPacket().strData.c_str();
	}

	pClient->CloseSocket();
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
	if (m_Tree.GetChildItem(hTreeSelected) == NULL)
		return;
	DeleteTreeChildrenItem(hTreeSelected);
	m_List.DeleteAllItems();
	CString strPath = GetPath(hTreeSelected);

#ifdef _UNICODE
	CStringA strPathA(strPath);
	int nCmd = sendCommandPacket(2, FALSE, (BYTE*)(LPCSTR)strPathA, strPathA.GetLength());
#else
	int nCmd = sendCommandPacket(2, FALSE, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
#endif //#ifdef _UNICODE

	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->getPacket().strData.c_str();
	CClientSocket* pClient = CClientSocket::getInstance();
	int RECV_COUNT_FILE{ 0 };
	while (pInfo->HasNext) {
		TRACE("Client OnNMDblclkTreeDir(): %s isDir:%s\r\n",
			pInfo->szFileName, pInfo->IsDirectory == 0 ? "true" : "false");
		if (pInfo->IsDirectory) {
			if (CString(pInfo->szFileName) == "." || CString(pInfo->szFileName) == "..") {
				int cmd = pClient->DealCommand();
				TRACE("ack:%d\r\n", cmd);
				if (cmd < 0) break;
				pInfo = (PFILEINFO)CClientSocket::getInstance()->getPacket().strData.c_str();
				continue;
			}
			CString strFileName(pInfo->szFileName);
			HTREEITEM hTemp = m_Tree.InsertItem(strFileName, hTreeSelected, TVI_LAST);
			m_Tree.InsertItem(_T(""), hTemp, TVI_LAST);
		}
		else {
			CString strFormatted(pInfo->szFileName);
			m_List.InsertItem(0, strFormatted);
		}
		RECV_COUNT_FILE++;
		int cmd = pClient->DealCommand();
		//		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0) break;
		pInfo = (PFILEINFO)CClientSocket::getInstance()->getPacket().strData.c_str();
	}
	TRACE("RECV_COUNT_FILE = %d\r\n", RECV_COUNT_FILE);
	pClient->CloseSocket();
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
	// TODO: 下载文件 执行线程函数
	_beginthread(CRemoteClientDlg::threadEntryForDownFile, 0, this);
	BeginWaitCursor();
	m_dlgStatus.m_info.SetWindowText(_T("命令正在执行中！..."));
	m_dlgStatus.ShowWindow(SW_SHOW);
	m_dlgStatus.CenterWindow(this);
	m_dlgStatus.SetActiveWindow();
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
	ret = sendCommandPacket(9, TRUE, (BYTE*)(LPCSTR)strFileA, strFileA.GetLength());
#else
	ret = sendCommandPacket(9, TRUE, (BYTE*)(LPCTSTR)strFile, strFile.GetLength());
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
	ret = sendCommandPacket(3, TRUE, (BYTE*)(LPCSTR)strFileA, strFileA.GetLength());
#else
	ret = sendCommandPacket(3, TRUE, (BYTE*)(LPCTSTR)strFile, strFile.GetLength());
#endif // _UNICODE
	if (ret < 0) {
		AfxMessageBox(_T("打开文件失败"));
	}
}

/**
 * WM_SENDPACKET的响应函数
 */
LRESULT CRemoteClientDlg::OnSendPacket(WPARAM wParam, LPARAM lParam) {
	int ret{ 0 };
	int cmd = wParam >> 1;
	switch (cmd) {
	case 4: {
		CString strFile = (LPCTSTR)lParam;
		CStringA strFileA(strFile);
#ifdef _UNICODE
		ret = sendCommandPacket(cmd, wParam & 1, (BYTE*)(LPCSTR)strFileA, strFileA.GetLength());
#else
		ret = sendCommandPacket(cmd, wParam & 1, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
#endif // _UNICODE
	}
		  break;
	case 5: {//鼠标操作
		ret = sendCommandPacket(cmd, wParam & 1, (BYTE*)lParam, sizeof(MOUSEEV));
	}
		break;
	case 6: {
		ret = sendCommandPacket(cmd, wParam & 1);
	}
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}

void CRemoteClientDlg::OnBnClickedBtnStartWatch() {
	CWatchDialog dlg(this);
	_beginthread(CRemoteClientDlg::threadEntryForWatchData, 0, this);
	dlg.DoModal();
}
