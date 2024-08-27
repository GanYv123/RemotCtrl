#pragma once

#include "pch.h"
#include "framework.h"

class CServerSocket {
public:
	static CServerSocket* getInstance();
	BOOL initSocket();
	BOOL AcceptClient();
	int DealCommand();
	BOOL Send(const char* pData, int nSize);
private:
	SOCKET m_serv_socket;
	SOCKET m_client;
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