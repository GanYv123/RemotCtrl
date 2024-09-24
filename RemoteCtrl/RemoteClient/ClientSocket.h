#pragma once

#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>
#include <map>
#include <list>
#include <mutex>

/* 用于数据的 包、帧 */
#pragma pack(push)
#pragma pack(1)
class CPacket {
public:
	/*函数*/
	CPacket();
	CPacket(const CPacket& packet);
	CPacket(const BYTE* pData, size_t& nSize);
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize,HANDLE hEvent);

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
	HANDLE hEvent;		
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
	/// <summary>
	/// 发送数据包（将数据包加入发送队列）
	/// 将接收到的数据存入 参数二
	/// </summary>
	/// <param name="pack">要发送的包</param>
	/// <param name="lstPacks">应答包</param>
	/// <returns>是否发送成功</returns>
	BOOL SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks,BOOL isAutoClosed = TRUE);
	BOOL getFilePath(std::string& strPath);
	BOOL getMouseEvent(MOUSEEV& mouse);
	CPacket& getPacket();
	void CloseSocket();
	void UpdateAddress(int nIP,int nPort) {
		if ((m_nIP != nIP) || (m_nPort != nPort)) {
			m_nIP = nIP;
			m_nPort = nPort;
		}
	}
private:
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
	CClientSocket(const CClientSocket&);
	~CClientSocket();
	/// <summary>
	/// 解决网络中多线程发送同步
	/// </summary>
	/// <param name="arg">传入this指针调用threadFunc启动线程</param>
	static void threadEntry(void* arg);
	//改为长连接，只初始化一次
	void threadFunc();
	BOOL InitSockEnv();
	CClientSocket& operator=(const CClientSocket&ss) {}
	BOOL Send(const char* pData, int nSize);
	BOOL Send(const CPacket& pack);
	static CClientSocket* m_instance;
	static void releaseInstance();
	class CHelper {
	public:
		CHelper();
		~CHelper();
	};
	static CHelper m_helper;
};

