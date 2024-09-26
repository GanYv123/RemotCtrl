#pragma once
#include "ClientSocket.h"
#include "WatchDialog.h"
#include "StatusDlg.h"
#include "resource.h"
#include "RemoteClientDlg.h"
#include <map>
#include "EdoyunTool.h"

#define WM_SHOW_STATUS (WM_USER+3)	//展示状态
#define WM_SHOW_WATCH (WM_USER+4)	//远程监控
#define WM_SEND_MESSAGE (WM_USER+0x1000)//自定义消息处理

class CClientController {
public:
	//获取全局唯一对象
	static CClientController* getInstance();
	//初始化操作
	int InitController();
	//启动
	int Invoke(CWnd*& pMainWnd);
	//发送消息
	LRESULT SendMessage(MSG msg);
	//更新网络服务器的地址
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
	/// 1.查看磁盘分区
	///	2.查看指定目录文件
	///	3.打开文件
	///	4.下载文件
	///	9.删除文件
	///	5.鼠标操作
	///	6.发送屏幕内容
	///	7.锁机
	///	8.解锁
	/// </summary>
	/// <param name="hWnd">数据包收到后要应答的窗口</param>
	/// <param name="nCmd">命令号</param>
	/// <param name="bAutoClose">自动关闭套接字</param>
	/// <param name="pData">数据</param>
	/// <param name="nLength">数据长度</param>
	/// <returns>是否发送成功</returns>
	BOOL SendCommandPacket(HWND hWnd, int nCmd, bool bAutoClose = true,
		BYTE* pData = NULL, size_t nLength = 0, WPARAM wParam = 0);

	int GetImage(CImage& image) {
		CClientSocket* pClient = CClientSocket::getInstance();
		return CEdoyunTool::Byte2Image(image, pClient->getPacket().strData);
	}
	void DownloadEnd();
	int DownFile(CString strPath);
	void StartWatchScreen();
protected:
	void threadWatchScreen();
	static void threadWatchScreen(void* arg);
	void threadDownloadFile();
	static void threadDownloadEntry(void* arg);
	CClientController() :
		m_statusDlg(&m_remoteDlg),
		m_watchDlg(&m_remoteDlg) 
	{
		m_isClosed = TRUE;
		m_hThreadWatch = INVALID_HANDLE_VALUE;
		m_hThread = INVALID_HANDLE_VALUE;
		m_hThreadDownload = INVALID_HANDLE_VALUE;
		m_nThreadId = -1;
	}
	~CClientController() {
		WaitForSingleObject(m_hThread, 100);
	}
	/// <summary>
	/// 消息处理循环函数
	/// </summary>
	void threadFunc();

	static unsigned int WINAPI threadEntry(void* arg);
	static void releaseInstance() {
		if (m_instance != nullptr) {
			delete m_instance;
			m_instance = nullptr;
		}
	}
	LRESULT OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);
	//LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowWatch(UINT nMsg, WPARAM wParam, LPARAM lParam);

private:
	typedef struct MsgInfo {
		MSG msg; //发送MSG
		LRESULT result; //命令执行的返回值

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

	//定义函数指针
	typedef LRESULT(CClientController::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);

	static std::map<UINT, MSGFUNC> m_mapFunc;
	CWatchDialog m_watchDlg;
	CRemoteClientDlg m_remoteDlg;
	CStatusDlg m_statusDlg;
	HANDLE m_hThread;
	HANDLE m_hThreadDownload;
	HANDLE m_hThreadWatch;
	BOOL m_isClosed;//监视是否关闭
	//下载文件的远程路径
	CString m_strRemote;	
	//下载文件的本地保存路径
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

