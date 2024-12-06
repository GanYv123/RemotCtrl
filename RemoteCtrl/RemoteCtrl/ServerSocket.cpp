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

BOOL CServerSocket::initSocket(short port) {
	// TODO: 1.socket bind listen accept read/write close;
	if (m_serv_socket == -1) { return FALSE; }
	SOCKADDR_IN serv_adr;
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(port);
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

int CServerSocket::Run(SOCK_CALLBACK callback, void* arg, short port) {
	// ��ʼ���������׽��֣��󶨵�ָ���˿�
	bool ret = initSocket(port);
	if(ret == false){
		return -1; // ��ʼ��ʧ�ܣ����ش����� -1
	}

	// ���ڴ洢��Ҫ���͵���Ӧ���ݰ�
	std::list<CPacket> lstPackets;

	// ���ûص�����������������ڴ���ҵ���߼�
	m_callback = callback; // �ص�����ָ��
	m_arg = arg;           // �ص������ĸ��Ӳ���

	// �����������ڼ�¼�����ͻ�������ʧ�ܴ���
	int count = 0;

	// ��ѭ��������ͻ�������
	while(true){
		// �ȴ������ܿͻ�������
		if(AcceptClient() == false){
			// �������ʧ�ܣ��ۼ�ʧ�ܴ���
			if(count >= 3){
				return -2; // ���ʧ�ܴ����ﵽ 3 �Σ��˳����򷵻ش����� -2
			}
			count++;
			continue; // ��������ѭ���������ȴ�����
		}

		// ����ͻ��������ȡ����ֵ
		int ret = DealCommand();

		// �������Ч�ķ���ֵ
		if(ret > 0){
			// �����û��Զ���ص�������runCommand���������Ҫ����
			m_callback(m_arg, ret, lstPackets, m_packet);

			// ���������͵���Ӧ���ݰ��б�
			while(!lstPackets.empty()){
				// �����б��еĵ�һ�����ݰ����ͻ���
				Send(lstPackets.front());
				// ɾ���ѷ��͵����ݰ�
				lstPackets.pop_front();
			}
		}

		// �رյ�ǰ�ͻ�������
		closeClient();
	}

	return 0; // �������������� 0
}


BOOL CServerSocket::AcceptClient() {
	TRACE("enter AcceptClient()\r\n");
	SOCKADDR_IN cli_adr;
	int cli_adr_sz = sizeof(cli_adr);
	m_client = accept(m_serv_socket, (SOCKADDR*)&cli_adr, &cli_adr_sz);
	TRACE("m_client = %d\r\n",m_client);
	if (m_client == -1) {
		return FALSE;
	}
	return TRUE;
}

/**
 * ��������
 */
#define BUFFER_SIZE 4096
int CServerSocket::DealCommand() {
	if (m_client == -1) return FALSE;

	char* buffer = new char[BUFFER_SIZE];
	if (buffer == NULL) {
		TRACE("�ڴ治��!\r\n");
		return -2;
	}
	memset(buffer, 0, BUFFER_SIZE);
	size_t index = 0;
	while (TRUE) {
		int len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0);
		if (len <= 0) {
			delete[] buffer;
			return -1;
		}
		size_t size_len = static_cast<size_t>(len);
		index += size_len;
		size_len = index;

		m_packet = CPacket((BYTE*)buffer, size_len);
		if (len > 0) {
			memmove(buffer, buffer + size_len, BUFFER_SIZE - size_len);
			index -= size_len;
			delete[] buffer;
			return m_packet.sCmd;
		}
	}
	delete[] buffer;
	return -1;
}

BOOL CServerSocket::Send(const char* pData, int nSize) {
	if (m_client == -1) return FALSE;
	return send(m_client, pData, nSize, 0) > 0;
}

BOOL CServerSocket::Send(CPacket& pack) {
	//TRACE("Send m_sock = %d\r\n",pack.sCmd);
	if (m_client == -1) return FALSE;
	return send(m_client, pack.Data(), pack.size(), 0) > 0;
}

void CServerSocket::closeClient() { 
	if (m_client != INVALID_SOCKET) {
		closesocket(m_client);
		m_client = INVALID_SOCKET;
	}
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

