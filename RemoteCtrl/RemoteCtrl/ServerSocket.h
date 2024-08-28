#pragma once

#include "pch.h"
#include "framework.h"

/* �������ݵ� ����֡ */
class CPacket {
public:
	/*����*/
	CPacket();
	CPacket(const CPacket& packet);
	CPacket(const BYTE* pData,size_t& nSize);
	~CPacket();
	CPacket& operator=(const CPacket& pack);

	/*�� �� ��*/
	WORD sHead;			//�̶�λFEFF
	DWORD nLength;		//�����ȣ��������� �� У�� ����
	WORD sCmd;			//��������
	std::string strData;//������
	WORD sSum;			//��У��
};

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