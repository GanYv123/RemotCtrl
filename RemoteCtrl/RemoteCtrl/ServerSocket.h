#pragma once

#include "pch.h"
#include "framework.h"

/* 用于数据的 包、帧 */
#pragma pack(push)
#pragma pack(1)
class CPacket {
public:
	/*函数*/
	CPacket();
	CPacket(const CPacket& packet);
	CPacket(const BYTE* pData,size_t& nSize);
	CPacket(WORD nCmd,const BYTE* pData,size_t nSize);

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

typedef struct MouseEvent 
{
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}

	WORD nAction;	//点击 移动 双击
	WORD nButton;	//左键右键中键
	POINT ptXY;		//坐标
}MOUSEEV,*PMOUSEEV;


class CServerSocket {
public:
	static CServerSocket* getInstance();
	BOOL initSocket();
	BOOL AcceptClient();
	int DealCommand();
	BOOL Send(const char* pData, int nSize);
	BOOL Send(CPacket& pack);
	BOOL getFilePath(std::string& strPath);
	BOOL getMouseEvent(MOUSEEV& mouse);
private:
	SOCKET m_serv_socket;
	SOCKET m_client;
	CPacket m_packet;
	//
	CServerSocket();
	CServerSocket(const CServerSocket&);
	~CServerSocket();
	BOOL InitSockEnv();
	CServerSocket& operator=(const CServerSocket&) {}
	static CServerSocket* m_instance;
	static void releaseInstance();
	class CHelper {
	public:
		CHelper();
		~CHelper();
	};
	static CHelper m_helper;
};