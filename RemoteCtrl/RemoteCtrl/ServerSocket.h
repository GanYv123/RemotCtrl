#pragma once

#include "pch.h"
#include <list>
#include "Packet.h"
#include "framework.h"

typedef void(*SOCK_CALLBACK)(void* arg,int status,std::list<CPacket>& lstPacket,CPacket& inPacket);

class CServerSocket {
public:
	static CServerSocket* getInstance();
	int Run(SOCK_CALLBACK callback, void* arg, short port = 2233);

protected:
	BOOL initSocket(short port);
	BOOL AcceptClient();
	int DealCommand();
	BOOL Send(const char* pData, int nSize);
	BOOL Send(CPacket& pack);
	BOOL getFilePath(std::string& strPath);
	BOOL getMouseEvent(MOUSEEV& mouse);
	CPacket& getPacket();
	void closeClient();
private:
	SOCK_CALLBACK m_callback;
	void* m_arg;
	SOCKET m_serv_socket;
	SOCKET m_client;
	CPacket m_packet;
	//
	CServerSocket();
	CServerSocket(const CServerSocket& ss);
	~CServerSocket();
	BOOL InitSockEnv();
	CServerSocket& operator=(const CServerSocket& ss) {}
	static CServerSocket* m_instance;
	static void releaseInstance();
	class CHelper {
	public:
		CHelper();
		~CHelper();
	};
	static CHelper m_helper;
};