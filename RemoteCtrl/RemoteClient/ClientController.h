#pragma once
#include "ClientSocket.h"
#include "WatchDialog.h"
#include "StatusDlg.h"
#include "resource.h"
#include "RemoteClientDlg.h"
#include <map>
#include "EdoyunTool.h"

#define WM_SHOW_STATUS (WM_USER+3)	//չʾ״̬
#define WM_SHOW_WATCH (WM_USER+4)	//Զ�̼��
#define WM_SEND_MESSAGE (WM_USER+0x1000)//�Զ�����Ϣ����

class CClientController {
public:
	//��ȡȫ��Ψһ����
	static CClientController* getInstance();
	//��ʼ������
	int InitController();
	//����
	int Invoke(CWnd*& pMainWnd);
	//��������������ĵ�ַ
	void UpdateAddress(int nIP, int nPort) {
		CClientSocket::getInstance()->UpdateAddress(nIP, nPort);
	}
	int DealCommand() {
		return CClientSocket::getInstance()->DealCommand();
	}
	void CloseSocket() {
		CClientSocket::getInstance()->CloseSocket();
	}
	/// <summary>
	/// 1.�鿴���̷���
	///	2.�鿴ָ��Ŀ¼�ļ�
	///	3.���ļ�
	///	4.�����ļ�
	///	9.ɾ���ļ�
	///	5.������
	///	6.������Ļ����
	///	7.����
	///	8.����
	/// </summary>
	/// <param name="hWnd">���ݰ��յ���ҪӦ��Ĵ���</param>
	/// <param name="nCmd">�����</param>
	/// <param name="bAutoClose">�Զ��ر��׽���</param>
	/// <param name="pData">����</param>
	/// <param name="nLength">���ݳ���</param>
	/// <returns>�Ƿ��ͳɹ�</returns>
	static BOOL SendCommandPacket(HWND hWnd, int nCmd, bool bAutoClose = true,
	                              BYTE* pData = NULL, size_t nLength = 0, WPARAM wParam = 0);

	static int GetImage(CImage& image) {
		CClientSocket* pClient = CClientSocket::getInstance();
		return CEdoyunTool::Byte2Image(image, pClient->getPacket().strData);
	}
	void DownloadEnd();
	int DownFile(CString strPath);
	void StartWatchScreen();
protected:
	void threadWatchScreen();
	static void threadWatchScreen(void* arg);
	CClientController() :
		m_watchDlg(&m_remoteDlg),
		m_statusDlg(&m_remoteDlg) 
	{
		m_isClosed = TRUE;
		m_hThreadWatch = INVALID_HANDLE_VALUE;
		m_hThread = INVALID_HANDLE_VALUE;
		m_nThreadId = -1;
	}
	~CClientController() {
		WaitForSingleObject(m_hThread, 100);
	}
	/// <summary>
	/// ��Ϣ����ѭ������
	/// </summary>
	void threadFunc();

	static unsigned int WINAPI threadEntry(void* arg);
	static void releaseInstance() {
		if (m_instance != nullptr) {
			delete m_instance;
			m_instance = nullptr;
		}
	}
	//LRESULT OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);
	//LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowWatch(UINT nMsg, WPARAM wParam, LPARAM lParam);

private:
	typedef struct MsgInfo {
		MSG msg; //����MSG
		LRESULT result; //����ִ�еķ���ֵ

		MsgInfo(MSG m) {
			result = 0;
			memcpy(&msg, &m, sizeof(MSG));
		}

		MsgInfo(const MsgInfo& m) {
			result = m.result;
			memcpy(&msg, &m.msg, sizeof(MSG));
		}

		MsgInfo& operator=(const MsgInfo& m) {
			if (this != &m) {
				result = m.result;
				memcpy(&msg, &m.msg, sizeof(MSG));
			}
			return *this;
		}

	}MSGINFO;

	//���庯��ָ��
	typedef LRESULT(CClientController::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);

	static std::map<UINT, MSGFUNC> m_mapFunc;
	CWatchDialog m_watchDlg;//��Ϣ�����ڶԻ���ر�֮�󣬿��ܵ����ڴ�й¶
	CRemoteClientDlg m_remoteDlg;
	CStatusDlg m_statusDlg;
	HANDLE m_hThread;
	HANDLE m_hThreadWatch;
	BOOL m_isClosed;//�����Ƿ�ر�
	//�����ļ���Զ��·��
	CString m_strRemote;	
	//�����ļ��ı��ر���·��
	CString m_strLocal;		
	UINT m_nThreadId;
	static CClientController* m_instance;

	class CHelper {
	public:
		CHelper() {
			//CClientController::getInstance();
		}
		~CHelper() {
			CClientController::releaseInstance();
		}
	};
	static CHelper m_helper;
};

