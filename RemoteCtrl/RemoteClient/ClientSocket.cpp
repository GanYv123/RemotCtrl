#include "pch.h"
#include "ClientSocket.h"
#define BUFFER_SIZE 4096

CClientSocket::CClientSocket() {
	m_sock = INVALID_SOCKET;
	if (InitSockEnv() == FALSE) {
		MessageBox(NULL, _T("�޷���ʼ���׽��ֻ���,������������"), _T("��ʼ���׽��ִ���"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	m_buffer.resize(BUFFER_SIZE);
}

CClientSocket::CClientSocket(const CClientSocket& ss) {
	m_sock = ss.m_sock;
}

CClientSocket::~CClientSocket() {
	closesocket(m_sock);
	WSACleanup();
}


CClientSocket* CClientSocket::getInstance() {
	// TODO: �ڴ˴����� return ���
	if (m_instance == nullptr)
		m_instance = new CClientSocket;
	return m_instance;
}

std::string GetErrorInfo(int wsaErrCode) {
	std::string ret;
	// ���������Ƿ�Ϊ 0
	if (wsaErrCode == 0) {
		return "No error.";
	}
	LPVOID lpMsgBuf = NULL;
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,wsaErrCode,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,0,NULL);
	if (lpMsgBuf) {
		ret = (char*)lpMsgBuf;
		LocalFree(lpMsgBuf);
	}
	else {
		ret = "Unknown error.";
	}
	return ret;
}


BOOL CClientSocket::initSocket(const std::string strIPAddress) {
	if (m_sock != INVALID_SOCKET) CloseSocket();
	m_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (m_sock == -1) { return FALSE; }
	SOCKADDR_IN serv_adr;
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;

	#pragma warning(push)
	#pragma warning(suppress : 4996) //inet_addr
	serv_adr.sin_addr.S_un.S_addr = inet_addr(strIPAddress.c_str());
	#pragma warning(pop) // ���뿪�������ʱ���ָ�֮ǰ�ľ��漶��

	serv_adr.sin_port = htons(2323);
	if (serv_adr.sin_addr.s_addr == INADDR_NONE) {
		AfxMessageBox(_T("ָ����IP��ַ������!"));
		return FALSE;
	}

	int ret = connect(m_sock, (SOCKADDR*)&serv_adr, sizeof(serv_adr));
	if (ret == -1) {
		AfxMessageBox(_T("����ʧ��!"));
		TRACE("����ʧ��: %d %s\r\n",WSAGetLastError(),GetErrorInfo(WSAGetLastError()).c_str());
		return FALSE;
	}
	return TRUE;

}
/** �������� */
int CClientSocket::DealCommand() {
	if (m_sock == -1) return FALSE;

	char* buffer = m_buffer.data();
	memset(buffer, 0, BUFFER_SIZE);
	size_t index = 0;
	while (TRUE) {
		int len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);
		if (len <= 0) {
			return -1;
		}
		size_t size_len = static_cast<size_t>(len);
		index += size_len;
		size_len = index;

		m_packet = CPacket((BYTE*)buffer, size_len);
		if (len > 0) {
			memmove(buffer, buffer + size_len, BUFFER_SIZE - size_len);
			index -= size_len;
			return m_packet.sCmd;
		}
	}
	return -1;
}

BOOL CClientSocket::Send(const char* pData, int nSize) {
	if (m_sock == -1) return FALSE;
	return send(m_sock, pData, nSize, 0) > 0;
}

BOOL CClientSocket::Send(CPacket& pack) {
	if (m_sock == -1) return FALSE;
	return send(m_sock, pack.Data(), pack.size(), 0) > 0;
}

BOOL CClientSocket::getFilePath(std::string& strPath) {
	if ((m_packet.sCmd == 2)
		|| (m_packet.sCmd == 3)
		|| (m_packet.sCmd == 4)) {
		strPath = m_packet.strData;
		return TRUE;
	}
	return FALSE;
}

BOOL CClientSocket::getMouseEvent(MOUSEEV& mouse) {
	if (m_packet.sCmd == 5) {
		memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
		return TRUE;
	}
	return FALSE;
}

CPacket& CClientSocket::getPacket() { return m_packet; }

void CClientSocket::CloseSocket() { closesocket(m_sock); m_sock = INVALID_SOCKET;}

/**
 * @purpose:���绷����ʼ��
 */
BOOL CClientSocket::InitSockEnv() {
	//�׽��ֳ�ʼ��
	WSADATA data;
	if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
		return FALSE;
	}
	return TRUE;
}

void CClientSocket::releaseInstance() {
	if (m_instance != nullptr) {
		CClientSocket* tmp = m_instance;
		m_instance = nullptr;
		delete tmp;
	}
}

CClientSocket* CClientSocket::m_instance = nullptr;

CClientSocket::CHelper CClientSocket::m_helper;

CClientSocket* pserver = CClientSocket::getInstance();

CClientSocket::CHelper::CHelper() {
	CClientSocket::getInstance();
}

CClientSocket::CHelper::~CHelper() {
	CClientSocket::releaseInstance();
}

/**
 * ���캯��
 * ��ʼ������Ա����
 */
CPacket::CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}
//���ƹ��캯��
CPacket::CPacket(const CPacket& pack) {
	sHead = pack.sHead;
	nLength = pack.nLength;
	sCmd = pack.sCmd;
	strData = pack.strData;
	sSum = pack.sSum;
}

/**
 * @argument:
 * nSize:���ݵ��ֽڴ�С
 */
CPacket::CPacket(const BYTE* pData, size_t& nSize) :sHead(0), nLength(0), sCmd(0), sSum(0) {
	size_t i = 0;
	for (; i < nSize; i++) {
		if (*(WORD*)(pData + i) == 0xFEFF) { //У���ͷ
			sHead = *(WORD*)(pData + i);
			i += 2;//һ��WORD 2byte
			break;
		}
	}
	if (i + 8 > nSize) { //�������ɹ� length + cmd +sum
		nSize = 0;
		return;/*��С�������İ�*/
	}

	nLength = *(DWORD*)(pData + i);
	i += 4;
	/*- - -*/
	//�˴� i = length ֮ǰ���ֽ���
	if (nLength + i > nSize) { //��û��ȫ���յ����в�ȱ
		nSize = 0;
		return;
	}

	sCmd = *(WORD*)(pData + i);
	i += 2;
	/*- - -*/
	if (nLength > 4) {
		strData.resize(nLength - 2 - 2);
		memcpy((void*)strData.c_str(), pData + i, nLength - 4);
		i += nLength - 4;
	}
	/*- - -*/
	sSum = *(WORD*)(pData + i);
	i += 2;
	WORD sum = 0;
	for (size_t j = 0; j < strData.size(); j++) {
		sum += (BYTE(strData[j]) & 0xFF);
	}
	if (sum == sSum) {//�����ɹ�
		nSize = i;
		return;
		//FFFE 0900 0000 0100 432C442C46 2501
	}
	nSize = 0;
}

CPacket::CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
	sHead = 0xFEFF;
	nLength = nSize + 4;//���ݳ��� = cmd + У��
	sCmd = nCmd;
	if (nSize > 0) {
		strData.resize(nSize);
		memcpy((void*)strData.c_str(), pData, nSize);
	}
	else {
		strData.clear();
	}
	sSum = 0;
	for (size_t j = 0; j < strData.size(); j++) {
		sSum += (BYTE(strData[j]) & 0xFF);
	}
}

CPacket::~CPacket() {
}

CPacket& CPacket::operator=(const CPacket& pack) {
	if (this == &pack) return *this;
	sHead = pack.sHead;
	nLength = pack.nLength;
	sCmd = pack.sCmd;
	strData = pack.strData;
	sSum = pack.sSum;
	return *this;
}

int CPacket::size() {
	return nLength + 6;
}

const char* CPacket::Data() {
	strOut.resize(nLength + 6);
	BYTE* pData = (BYTE*)strOut.c_str();
	*(WORD*)pData = sHead;
	pData += 2;
	*(DWORD*)pData = nLength;
	pData += 4;
	*(WORD*)pData = sCmd;
	pData += 2;
	memcpy(pData, strData.c_str(), strData.size());
	pData += strData.size();
	*(WORD*)pData = sSum;

	return strOut.c_str();
}