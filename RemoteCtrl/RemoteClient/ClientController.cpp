#include "pch.h"
#include "ClientSocket.h"
#include "ClientController.h"

std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc;
CClientController* CClientController::m_instance = NULL;
CClientController::CHelper CClientController::m_helper;

CClientController* CClientController::getInstance() {
	if (m_instance == nullptr) {
		m_instance = new CClientController();
		struct {
			UINT nMsg;
			MSGFUNC func;
		}MsgFuncs[] =
		{
			{WM_SHOW_STATUS,&CClientController::OnShowStatus},
			{WM_SHOW_WATCH,&CClientController::OnShowWatch},
			{(UINT)-1,NULL}
		};
		for (int i = 0; MsgFuncs[i].func != NULL; i++) {
			m_mapFunc.insert(std::pair<UINT, MSGFUNC>(MsgFuncs[i].nMsg, MsgFuncs[i].func));
		}
	}
	return m_instance;
}

int CClientController::InitController() {
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientController::threadEntry, this, 0, &m_nThreadId);
	//CreateThread
	m_statusDlg.Create(IDD_DLG_STATUS, &m_remoteDlg);
	return 0;
}

int CClientController::Invoke(CWnd*& pMainWnd) {
	pMainWnd = &m_remoteDlg;
	return m_remoteDlg.DoModal();
}

LRESULT CClientController::SendMessage(MSG msg) {
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (hEvent == NULL)return -2;
	MSGINFO info(msg);
	PostThreadMessage(m_nThreadId, WM_SEND_MESSAGE, (WPARAM)&info, (LPARAM)hEvent);
	WaitForSingleObject(hEvent, INFINITE);//直到指定的事件被触发再返回结果
	CloseHandle(hEvent);
	//确保当前线程在继续执行之前，消息已经得到了处理，并且处理结果是可用的
	return info.result;
}

BOOL CClientController::SendCommandPacket(HWND hWnd, int nCmd,
	bool bAutoClose, BYTE* pData, size_t nLength, WPARAM wParam) 
{
	//TRACE("cmd:%d %s\r\n",nCmd);
	CClientSocket* pClient = CClientSocket::getInstance();
	BOOL ret = pClient->SendPacket(hWnd, CPacket(nCmd, pData, nLength), bAutoClose, wParam);
	return ret;
}

void CClientController::DownloadEnd() {
	m_statusDlg.ShowWindow(SW_HIDE);
	m_remoteDlg.EndWaitCursor();
	m_remoteDlg.MessageBox(_T("下载完成!"), _T("完成"));
}

int CClientController::DownFile(CString strPath) {
	CFileDialog dlg(FALSE, _T("*"), strPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, &m_remoteDlg);
	if (dlg.DoModal() == IDOK) {
		m_strRemote = strPath;
		m_strLocal = dlg.GetPathName();
		FILE* pFile{ nullptr };
		errno_t err;
#ifdef _UNICODE
		// 将 Unicode CString 转换为 ANSI CStringA
		CStringA pFileA(m_strLocal);
		err = fopen_s(&pFile, pFileA.GetString(), "wb+"); // 获取 ANSI 字符串
#else
		// 非 Unicode 版本，直接使用 CString
		err = fopen_s(&pFile, dlg.GetPathName().GetString(), "wb+");
#endif // _UNICODE
		if (err != 0) {
			AfxMessageBox(_T("本地无权限，文件无法创建!"));
			return -1;
		}
		// 文件处理逻辑
#ifdef _UNICODE
		// 将 Unicode CString 转换为 ANSI CStringA
		CStringA strRemoteA(m_strRemote);
		SendCommandPacket(m_remoteDlg, 4, FALSE,
			(BYTE*)(LPCSTR)strRemoteA, strRemoteA.GetLength(), (WPARAM)pFile);
#else
		SendCommandPacket(m_remoteDlg, 4, FALSE,
			(BYTE*)(LPCTSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);
#endif // _UNICODE
		//m_hThreadDownload = (HANDLE)_beginthread(&CClientController::threadDownloadEntry, 0, this);
		//检测线程是否被创建
// 		if (WaitForSingleObject(m_hThreadDownload, 0) != WAIT_TIMEOUT) {
// 			return -1;
// 		}
		m_remoteDlg.BeginWaitCursor();
		m_statusDlg.m_info.SetWindowText(_T("命令正在执行中！..."));
		m_statusDlg.ShowWindow(SW_SHOW);
		m_statusDlg.CenterWindow(&m_remoteDlg);
		m_statusDlg.SetActiveWindow();
	}
	return 0;
}

void CClientController::StartWatchScreen() {
	m_isClosed = FALSE;
	//m_watchDlg.SetParent(&m_remoteDlg);
	m_hThreadWatch = (HANDLE)_beginthread(&CClientController::threadWatchScreen, 0, this);
	m_watchDlg.DoModal();
	m_isClosed = TRUE;
	WaitForSingleObject(m_hThreadWatch, 500);

}

void CClientController::threadWatchScreen() {
	//可能存在异步问题导致程序崩溃
	Sleep(50);
	while (!m_isClosed) {
		if (m_watchDlg.isFull() == FALSE) {//如果缓存为空，放入缓存
			std::list<CPacket> lstPacks;
			int ret = SendCommandPacket(m_watchDlg.GetSafeHwnd(),6,true,NULL,0);
			//TODO:添加消息响应函数WM_SEND_PACKACK
			//TODO:控制发送频率
			if (ret == 6) {	
				if (CEdoyunTool::Byte2Image(m_watchDlg.GetImage(),
					lstPacks.front().strData) == 0) 
				{
					m_watchDlg.setImageStatus(TRUE);
					TRACE("成功加载图片： %08x\r\n",(HBITMAP)m_watchDlg.GetImage());
					TRACE("和校验： %08x\r\n",lstPacks.front().sSum);
				}
				else {
					TRACE(_T("获取图片失败!\r\n"));
				}
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

void CClientController::threadWatchScreen(void* arg) {
	CClientController* thiz = (CClientController*)arg;
	thiz->threadWatchScreen();
	_endthread();
}

void CClientController::threadDownloadFile() {
	FILE* pFile{ nullptr };
	errno_t err;

#ifdef _UNICODE
	// 将 Unicode CString 转换为 ANSI CStringA
	CStringA pFileA(m_strLocal);
	err = fopen_s(&pFile, pFileA.GetString(), "wb+"); // 获取 ANSI 字符串
#else
	// 非 Unicode 版本，直接使用 CString
	err = fopen_s(&pFile, dlg.GetPathName().GetString(), "wb+");
#endif // _UNICODE

	if (err != 0) {
		AfxMessageBox(_T("本地无权限，文件无法创建!"));
		m_statusDlg.ShowWindow(SW_HIDE);
		m_remoteDlg.EndWaitCursor();
		return;
	}
	// 文件处理逻辑
	CClientSocket* pClient = CClientSocket::getInstance();
	int ret{ -1 };
	do {
#ifdef _UNICODE
		// 将 Unicode CString 转换为 ANSI CStringA
		CStringA strRemoteA(m_strRemote);
		ret = SendCommandPacket(m_remoteDlg, 4, FALSE,
			(BYTE*)(LPCSTR)strRemoteA, strRemoteA.GetLength(),(WPARAM)pFile);
#else
		ret = SendCommandPacket(m_remoteDlg, 4, FALSE,
			(BYTE*)(LPCTSTR)m_strRemote, m_strRemote.GetLength(),(WPARAM)pFile);
#endif // _UNICODE

		long long nLength = *(long long*)pClient->getPacket().strData.c_str();
		if (nLength == 0) {
			AfxMessageBox(_T("文件长度为0，无法读取文件!"));
			break;
		}
		long long nCount = 0;

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
	m_statusDlg.ShowWindow(SW_HIDE);
	m_remoteDlg.EndWaitCursor();
	m_remoteDlg.MessageBox(_T("下载完成!"), _T("完成"));
}


void CClientController::threadDownloadEntry(void* arg) {
	CClientController* thiz = (CClientController*)arg;
	thiz->threadDownloadFile();
	_endthread();
}

void CClientController::threadFunc() {
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);//翻译键盘等输入的 msg
		DispatchMessage(&msg);//分发消息
		if (msg.message == WM_SEND_MESSAGE) {
			MSGINFO* pmsg = (MSGINFO*)msg.wParam;
			HANDLE hEvent = (HANDLE)msg.lParam;
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
			if (it != m_mapFunc.end()) {
				pmsg->result = (this->*it->second)(pmsg->msg.message, pmsg->msg.wParam, pmsg->msg.lParam);
			}
			else {
				pmsg->result = -1;
			}
			SetEvent(hEvent);//设置事件
		}
		else {
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
			if (it != m_mapFunc.end()) {
				(this->*it->second)(msg.message, msg.wParam, msg.lParam);
			}
		}

	}
}

unsigned int __stdcall CClientController::threadEntry(void* arg) {
	CClientController* thiz = (CClientController*)arg;
	thiz->threadFunc();
	_endthreadex(0);
	return 0;
}
LRESULT CClientController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam) {

	return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatch(UINT nMsg, WPARAM wParam, LPARAM lParam) {

	return m_watchDlg.DoModal();
}
