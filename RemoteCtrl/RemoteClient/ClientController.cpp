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
	Sleep(50);
	ULONGLONG nTick = GetTickCount64();
	while (!m_isClosed) {
		if (m_watchDlg.isFull() == FALSE) {//如果缓存为空，放入缓存
			if (GetTickCount64() - nTick < 200) {
				Sleep(200 - (DWORD)(GetTickCount64() - nTick));
			}
			nTick = GetTickCount64();
			int ret = SendCommandPacket(m_watchDlg.GetSafeHwnd(),6,true,NULL,0);
			if (ret == 1) {	
				//TRACE("成功发送请求图片命令\r\n");
			}
			else {
				TRACE("获取图片失败\r\n");
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
