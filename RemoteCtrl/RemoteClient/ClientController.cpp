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
	WaitForSingleObject(hEvent, INFINITE);//ֱ��ָ�����¼��������ٷ��ؽ��
	CloseHandle(hEvent);
	//ȷ����ǰ�߳��ڼ���ִ��֮ǰ����Ϣ�Ѿ��õ��˴������Ҵ������ǿ��õ�
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
	m_remoteDlg.MessageBox(_T("�������!"), _T("���"));
}

int CClientController::DownFile(CString strPath) {
	CFileDialog dlg(FALSE, _T("*"), strPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, &m_remoteDlg);
	if (dlg.DoModal() == IDOK) {
		m_strRemote = strPath;
		m_strLocal = dlg.GetPathName();
		FILE* pFile{ nullptr };
		errno_t err;
#ifdef _UNICODE
		// �� Unicode CString ת��Ϊ ANSI CStringA
		CStringA pFileA(m_strLocal);
		err = fopen_s(&pFile, pFileA.GetString(), "wb+"); // ��ȡ ANSI �ַ���
#else
		// �� Unicode �汾��ֱ��ʹ�� CString
		err = fopen_s(&pFile, dlg.GetPathName().GetString(), "wb+");
#endif // _UNICODE
		if (err != 0) {
			AfxMessageBox(_T("������Ȩ�ޣ��ļ��޷�����!"));
			return -1;
		}
		// �ļ������߼�
#ifdef _UNICODE
		// �� Unicode CString ת��Ϊ ANSI CStringA
		CStringA strRemoteA(m_strRemote);
		SendCommandPacket(m_remoteDlg, 4, FALSE,
			(BYTE*)(LPCSTR)strRemoteA, strRemoteA.GetLength(), (WPARAM)pFile);
#else
		SendCommandPacket(m_remoteDlg, 4, FALSE,
			(BYTE*)(LPCTSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);
#endif // _UNICODE
		//m_hThreadDownload = (HANDLE)_beginthread(&CClientController::threadDownloadEntry, 0, this);
		//����߳��Ƿ񱻴���
// 		if (WaitForSingleObject(m_hThreadDownload, 0) != WAIT_TIMEOUT) {
// 			return -1;
// 		}
		m_remoteDlg.BeginWaitCursor();
		m_statusDlg.m_info.SetWindowText(_T("��������ִ���У�..."));
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
	//���ܴ����첽���⵼�³������
	Sleep(50);
	while (!m_isClosed) {
		if (m_watchDlg.isFull() == FALSE) {//�������Ϊ�գ����뻺��
			std::list<CPacket> lstPacks;
			int ret = SendCommandPacket(m_watchDlg.GetSafeHwnd(),6,true,NULL,0);
			//TODO:�����Ϣ��Ӧ����WM_SEND_PACKACK
			//TODO:���Ʒ���Ƶ��
			if (ret == 6) {	
				if (CEdoyunTool::Byte2Image(m_watchDlg.GetImage(),
					lstPacks.front().strData) == 0) 
				{
					m_watchDlg.setImageStatus(TRUE);
					TRACE("�ɹ�����ͼƬ�� %08x\r\n",(HBITMAP)m_watchDlg.GetImage());
					TRACE("��У�飺 %08x\r\n",lstPacks.front().sSum);
				}
				else {
					TRACE(_T("��ȡͼƬʧ��!\r\n"));
				}
			}
			else {
				Sleep(1);//Ԥ��CPU쭸�
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
	// �� Unicode CString ת��Ϊ ANSI CStringA
	CStringA pFileA(m_strLocal);
	err = fopen_s(&pFile, pFileA.GetString(), "wb+"); // ��ȡ ANSI �ַ���
#else
	// �� Unicode �汾��ֱ��ʹ�� CString
	err = fopen_s(&pFile, dlg.GetPathName().GetString(), "wb+");
#endif // _UNICODE

	if (err != 0) {
		AfxMessageBox(_T("������Ȩ�ޣ��ļ��޷�����!"));
		m_statusDlg.ShowWindow(SW_HIDE);
		m_remoteDlg.EndWaitCursor();
		return;
	}
	// �ļ������߼�
	CClientSocket* pClient = CClientSocket::getInstance();
	int ret{ -1 };
	do {
#ifdef _UNICODE
		// �� Unicode CString ת��Ϊ ANSI CStringA
		CStringA strRemoteA(m_strRemote);
		ret = SendCommandPacket(m_remoteDlg, 4, FALSE,
			(BYTE*)(LPCSTR)strRemoteA, strRemoteA.GetLength(),(WPARAM)pFile);
#else
		ret = SendCommandPacket(m_remoteDlg, 4, FALSE,
			(BYTE*)(LPCTSTR)m_strRemote, m_strRemote.GetLength(),(WPARAM)pFile);
#endif // _UNICODE

		long long nLength = *(long long*)pClient->getPacket().strData.c_str();
		if (nLength == 0) {
			AfxMessageBox(_T("�ļ�����Ϊ0���޷���ȡ�ļ�!"));
			break;
		}
		long long nCount = 0;

		while (nCount < nLength) {
			pClient->DealCommand();
			if (ret < 0) {
				AfxMessageBox(_T("����ʧ��"));
				TRACE("����ʧ��:ret = %d\r\n", ret);
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
	m_remoteDlg.MessageBox(_T("�������!"), _T("���"));
}


void CClientController::threadDownloadEntry(void* arg) {
	CClientController* thiz = (CClientController*)arg;
	thiz->threadDownloadFile();
	_endthread();
}

void CClientController::threadFunc() {
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);//������̵������ msg
		DispatchMessage(&msg);//�ַ���Ϣ
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
			SetEvent(hEvent);//�����¼�
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
