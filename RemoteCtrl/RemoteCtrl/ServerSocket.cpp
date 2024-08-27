#include "pch.h"
#include "ServerSocket.h"

CServerSocket::CServerSocket() {
	m_client = INVALID_SOCKET;
	if (InitSockEnv() == FALSE) {
		MessageBox(NULL, _T("�޷���ʼ���׽��ֻ���,������������"), _T("��ʼ���׽��ִ���"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	m_serv_socket = socket(PF_INET, SOCK_STREAM, 0);
}

CServerSocket::CServerSocket(const CServerSocket& ss) {
	m_serv_socket = ss.m_serv_socket;
	m_client = ss.m_client;
}

CServerSocket::~CServerSocket() {
	closesocket(m_serv_socket);
	WSACleanup();
}


CServerSocket* CServerSocket::getInstance() {
	// TODO: �ڴ˴����� return ���
	if (m_instance == nullptr)
		m_instance = new CServerSocket;
	return m_instance;
}

BOOL CServerSocket::initSocket() {
	// TODO: 1.socket bind listen accept read/write close;
	if (m_serv_socket == -1) { return FALSE; }
	SOCKADDR_IN serv_adr;
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(2323);
	//�󶨼���
	//TODO:У��
	if (bind(m_serv_socket, (SOCKADDR*)&serv_adr, sizeof(serv_adr)) == -1) {
		return FALSE;
	}

	if (listen(m_serv_socket, 1) == -1) {
		return FALSE;
	}
	return TRUE;

}

BOOL CServerSocket::AcceptClient() {
	SOCKADDR_IN cli_adr;
	int cli_adr_sz;
	m_client = accept(m_serv_socket, (SOCKADDR*)&cli_adr, &cli_adr_sz);
	if (m_client == -1) {
		return FALSE;
	}
	return TRUE;
}
int CServerSocket::DealCommand() {
	if (m_client == -1) return FALSE;
	char buffer[1024] = "";
	while (TRUE) {
		int len = recv(m_client, buffer, sizeof(buffer), 0);
		if (len <= 0) {
			return -1;
		}
		//TODO:��������
	}
}

BOOL CServerSocket::Send(const char* pData, int nSize) {
	return send(m_client, pData, nSize, 0) > 0;
}


/**
 * @purpose:���绷����ʼ��
 */
BOOL CServerSocket::InitSockEnv() {
	//�׽��ֳ�ʼ��
	WSADATA data;
	if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
		return FALSE;
	}
	return TRUE;
}

void CServerSocket::releaseInstance() {
	if (m_instance != nullptr) {
		CServerSocket* tmp = m_instance;
		m_instance = nullptr;
		delete tmp;
	}
}

CServerSocket* CServerSocket::m_instance = nullptr;

CServerSocket::CHelper CServerSocket::m_helper;

CServerSocket* pserver = CServerSocket::getInstance();

CServerSocket::CHelper::CHelper() {
	CServerSocket::getInstance();
}

CServerSocket::CHelper::~CHelper() {
	CServerSocket::releaseInstance();
}
