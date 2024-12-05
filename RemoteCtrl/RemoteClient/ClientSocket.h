#pragma once

#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>
#include <map>
#include <list>
#include <mutex>

#define  WM_SEND_PACK (WM_USER+1) //发送包数据
#define  WM_SEND_PACK_ACK (WM_USER+2) //发送包数据应答

/* 用于数据的 包、帧 */
#pragma pack(push)
#pragma pack(1)
class CPacket {
public:
	/*函数*/
	CPacket();
	CPacket(const CPacket& packet);
	CPacket(const BYTE* pData, size_t& nSize);
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize);

	~CPacket();
	CPacket& operator=(const CPacket& pack);

	int size();
	const char* Data(std::string& strOut)const;

	/*包 变 量*/
	WORD sHead;			//固定位FEFF
	DWORD nLength;		//包长度：控制命令 到 校验 结束
	WORD sCmd;			//控制命令
	std::string strData;//包数据
	WORD sSum;			//和校验
};
#pragma pack(pop)

typedef struct file_info {
	file_info() {
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;  //是否为失效文件
	BOOL IsDirectory;//是否为目录
	BOOL HasNext;	 //是否还有后续文件
	char szFileName[256];

}FILEINFO, * PFILEINFO;

enum{
	CSM_AUTOCLOSE = 1,//CSM = client socket mode 自动关闭模式
	
};

typedef struct PacketData {
	std::string strData;
	UINT nMode;
	WPARAM wParam;

	PacketData(const char* pData, size_t nLen, UINT mode, WPARAM nParam = 0) {
		strData.resize(nLen);
		memcpy((char*)strData.c_str(), pData, nLen);
		nMode = mode;
		wParam = nParam;
	}
	PacketData(const PacketData& data) {
		strData = data.strData;
		nMode = data.nMode;
		wParam = data.wParam;
	}
	PacketData& operator=(const PacketData& data) {
		if (this != &data) {
			strData = data.strData;
			nMode = data.nMode;
			wParam = data.wParam;
		}
		return *this;
	}
}PACKET_DATA;

typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}

	WORD nAction;	//点击 移动 双击
	WORD nButton;	//左键右键中键
	POINT ptXY;		//坐标
}MOUSEEV, * PMOUSEEV;


class CClientSocket {
public:
	static CClientSocket* getInstance();
	BOOL initSocket();
	void Dump(BYTE* pData, size_t nSize);
	int DealCommand();
	BOOL SendPacket(HWND hWnd, const CPacket& pack, BOOL isAutoClosed = TRUE, WPARAM wParam = 0);
	BOOL getFilePath(std::string& strPath);
	BOOL getMouseEvent(MOUSEEV& mouse);
	CPacket& getPacket();
	void CloseSocket();
	void UpdateAddress(int nIP, int nPort) {
		if ((m_nIP != nIP) || (m_nPort != nPort)) {
			m_nIP = nIP;
			m_nPort = nPort;
		}
	}
private:
	HANDLE m_eventInvoke;//启动事件
	UINT m_nThreadID;
	typedef void(CClientSocket::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	std::map<UINT, MSGFUNC> m_mapFunc;
	HANDLE m_hThread;
	BOOL m_bAutoClose;
	std::mutex m_lock;
	std::list<CPacket> m_lstSend;
	std::map<HANDLE, std::list<CPacket>&> m_mapAck;
	std::map<HANDLE, BOOL> m_mapAutoClosed;
	int m_nIP;//地址
	int m_nPort;//端口
	SOCKET m_sock;
	CPacket m_packet;
	std::vector<char> m_buffer;
	//
	CClientSocket();
	CClientSocket(const CClientSocket& ss);
	~CClientSocket();
	/// <summary>
	/// 解决网络中多线程发送同步
	/// </summary>
	/// <param name="arg">传入this指针调用threadFunc启动线程</param>
	static unsigned WINAPI threadEntry(void* arg);
	//改为长连接，只初始化一次
	//void threadFunc();
	void threadFunc2();
	BOOL InitSockEnv();
	CClientSocket& operator=(const CClientSocket& ss) {}
	BOOL Send(const char* pData, int nSize);
	BOOL Send(const CPacket& pack);
	/// <summary>
	/// 
	/// </summary>
	/// <param name="nMsg"></param>
	/// <param name="wParam">缓冲区的值</param>
	/// <param name="lParam">缓冲区的长度</param>
	void SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);
	static CClientSocket* m_instance;
	static void releaseInstance();
	class CHelper {
	public:
		CHelper();
		~CHelper();
	};
	static CHelper m_helper;
};

