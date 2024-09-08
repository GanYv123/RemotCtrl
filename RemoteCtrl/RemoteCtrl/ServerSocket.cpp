#include "pch.h"
#include "ServerSocket.h"

CServerSocket::CServerSocket() {
	m_client = INVALID_SOCKET;
	if (InitSockEnv() == FALSE) {
		MessageBox(NULL, _T("无法初始化套接字环境,请检查网络设置"), _T("初始化套接字错误"), MB_OK | MB_ICONERROR);
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
	// TODO: 在此处插入 return 语句
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
	//绑定监听
	//TODO:校验
	if (bind(m_serv_socket, (SOCKADDR*)&serv_adr, sizeof(serv_adr)) == -1) {
		return FALSE;
	}

	if (listen(m_serv_socket, 1) == -1) {
		return FALSE;
	}
	return TRUE;

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
 * 处理命令
 */
#define BUFFER_SIZE 4096
int CServerSocket::DealCommand() {
	if (m_client == -1) return FALSE;

	char* buffer = new char[BUFFER_SIZE];
	if (buffer == NULL) {
		TRACE("内存不足!\r\n");
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
	TRACE("Send m_sock = %d\r\n",pack.sCmd);
	if (m_client == -1) return FALSE;
	return send(m_client, pack.Data(), pack.size(), 0) > 0;
}

BOOL CServerSocket::getFilePath(std::string& strPath) {
	if ((m_packet.sCmd == 2)
		||(m_packet.sCmd == 3)
		||(m_packet.sCmd == 4)
		||(m_packet.sCmd == 9)) 
	{
		strPath = m_packet.strData;
		return TRUE;
	}
	return FALSE;
}

BOOL CServerSocket::getMouseEvent(MOUSEEV& mouse) {
	if (m_packet.sCmd == 5) {
		memcpy(&mouse,m_packet.strData.c_str(),sizeof(MOUSEEV));
		return TRUE;
	}
	return FALSE;
}

CPacket& CServerSocket::getPacket() {return m_packet;}

void CServerSocket::closeClient() { closesocket(m_client); m_client = INVALID_SOCKET;};


/**
 * @purpose:网络环境初始化
 */
BOOL CServerSocket::InitSockEnv() {
	//套接字初始化
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

/**
 * 构造函数
 * 初始化包成员变量
 */
CPacket::CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}
//复制构造函数
CPacket::CPacket(const CPacket& pack) {
	sHead = pack.sHead;
	nLength = pack.nLength;
	sCmd = pack.sCmd;
	strData = pack.strData;
	sSum = pack.sSum;
}

/**
 * @argument:
 * nSize:数据的字节大小
 */
CPacket::CPacket(const BYTE* pData, size_t& nSize) :sHead(0), nLength(0), sCmd(0), sSum(0) 
{
	size_t i = 0;
	for (; i < nSize; i++) {
		if (*(WORD*)(pData + i) == 0xFEFF) { //校验包头
			sHead = *(WORD*)(pData + i);
			i += 2;//一个WORD 2byte
			break;
		}
	}
	if (i + 8 > nSize) { //解析不成功 length + cmd +sum
		nSize = 0;
		return;/*大小不完整的包*/
	}

	nLength = *(DWORD*)(pData + i);
	i += 4;
	/*- - -*/
	//此处 i = length 之前的字节数
	if (nLength + i > nSize) { //包没完全接收到，有残缺
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
	if (sum == sSum) {//解析成功
		nSize = i;
		return;
		//FFFE 0900 0000 0100 432C442C46 2501
	}
	nSize = 0;
	
}

CPacket::CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
	sHead = 0xFEFF;
	nLength = nSize + 4;//数据长度 = cmd + 校验
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
	memcpy(pData,strData.c_str(),strData.size());
	pData += strData.size();
	*(WORD*)pData = sSum;
	
	return strOut.c_str();
}
