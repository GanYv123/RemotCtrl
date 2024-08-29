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
	CPacket(DWORD nCmd,const BYTE* pData,size_t nSize);

	~CPacket();
	CPacket& operator=(const CPacket& pack);

	int size();
	const char* Data();

	/*�� �� ��*/
	WORD sHead;			//�̶�λFEFF
	DWORD nLength;		//�����ȣ��������� �� У�� ����
	WORD sCmd;			//��������
	std::string strData;//������
	WORD sSum;			//��У��
	std::string strOut; //��������
};


class CServerSocket {
public:
	static CServerSocket* getInstance();
	BOOL initSocket();
	BOOL AcceptClient();
	int DealCommand();
	BOOL Send(const char* pData, int nSize);
	BOOL Send(CPacket& pack);
	BOOL getFilePath(std::string& strPath);
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