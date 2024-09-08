#pragma once

#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>

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
	const char* Data();

	/*包 变 量*/
	WORD sHead;			//固定位FEFF
	DWORD nLength;		//包长度：控制命令 到 校验 结束
	WORD sCmd;			//控制命令
	std::string strData;//包数据
	WORD sSum;			//和校验
	std::string strOut; //整包数据
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
	BOOL initSocket(int nIP, int nPort);
	void Dump(BYTE* pData, size_t nSize);
	int DealCommand();
	BOOL Send(const char* pData, int nSize);
	BOOL Send(CPacket& pack);
	BOOL getFilePath(std::string& strPath);
	BOOL getMouseEvent(MOUSEEV& mouse);
	CPacket& getPacket();
	void CloseSocket();
private:
	SOCKET m_sock;
	CPacket m_packet;
	std::vector<char> m_buffer;
	//
	CClientSocket();
	CClientSocket(const CClientSocket&);
	~CClientSocket();
	BOOL InitSockEnv();
	CClientSocket& operator=(const CClientSocket&) {}
	static CClientSocket* m_instance;
	static void releaseInstance();
	class CHelper {
	public:
		CHelper();
		~CHelper();
	};
	static CHelper m_helper;
};

