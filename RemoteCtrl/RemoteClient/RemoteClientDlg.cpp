
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

void CRemoteClientDlg::LoadFileList() {

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
	while (pInfo->HasNext) {
		TRACE("OnNMDblclkTreeDir(): %s isDir:%s\r\n",
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

		int cmd = pClient->DealCommand();
		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0) break;
		pInfo = (PFILEINFO)CClientSocket::getInstance()->getPacket().strData.c_str();
	}

	pClient->CloseSocket();
}



void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult) {
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileList();
}


void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult) {
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileList();

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

void CRemoteClientDlg::OnDownloadFile() {
	// TODO: 下载文件
	int nListSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nListSelected, 0);
	CFileDialog dlg(FALSE, _T("*"),strFile,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, this);
	
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
			return;
		}
		HTREEITEM hSelected = m_Tree.GetSelectedItem();
		strFile = GetPath(hSelected) + strFile;
		TRACE("%s\r\n", strFile);
		int ret{ -1 };
#ifdef _UNICODE
		CStringA strFileA(strFile);
		ret = sendCommandPacket(4, FALSE, (BYTE*)(LPCSTR)strFileA, strFileA.GetLength());
#else
		ret = sendCommandPacket(4, FALSE, (BYTE*)(LPCTSTR)strFile, strFile.GetLength());
#endif // _UNICODE
		if (ret < 0) {
			AfxMessageBox(_T("执行下载命令失败!!！"));
			TRACE("执行下载失败:ret = %d\r\n", ret);
			return;
		}
		CClientSocket* pClient = CClientSocket::getInstance();
		long long nLength = *(long long*)pClient->getPacket().strData.c_str();
		if (nLength == 0) {
			AfxMessageBox(_T("文件长度为0，无法读取文件!"));
			return;
		}
		long long nCount = 0;

		while (nCount < nLength) {
			pClient->DealCommand();
			if (ret < 0) {
				AfxMessageBox(_T("传输失败"));
				TRACE("传输失败:ret = %d\r\n", ret);
				break;
			}
			pClient->getPacket().strData.c_str();
			fwrite(pClient->getPacket().strData.c_str(), 1, pClient->getPacket().strData.size(), pFile);
			nCount += pClient->getPacket().strData.size();
		}
		fclose(pFile);
		pClient->CloseSocket();
	}
}


void CRemoteClientDlg::OnDeleteFile() {
	// TODO: 删除文件
}


void CRemoteClientDlg::OnRunFile() {
	// TODO: 运行文件
}
