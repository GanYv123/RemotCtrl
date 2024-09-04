#include "pch.h"
#include "ClientSocket.h"
#define BUFFER_SIZE 4096

CClientSocket::CClientSocket() {
	m_sock = INVALID_SOCKET;
	if (InitSockEnv() == FALSE) {
		MessageBox(NULL, _T("无法初始化套接字环境,请检查网络设置"), _T("初始化套接字错误"), MB_OK | MB_ICONERROR);
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
	// TODO: 在此处插入 return 语句
	if (m_instance == nullptr)
		m_instance = new CClientSocket;
	return m_instance;
}

std::string GetErrorInfo(int wsaErrCode) {
	std::string ret;
	// 检查错误码是否为 0
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
	#pragma warning(pop) // 当离开这个区域时，恢复之前的警告级别

	serv_adr.sin_port = htons(2323);
	if (serv_adr.sin_addr.s_addr == INADDR_NONE) {
		AfxMessageBox(_T("指定的IP地址不存在!"));
		return FALSE;
	}

	int ret = connect(m_sock, (SOCKADDR*)&serv_adr, sizeof(serv_adr));
	if (ret == -1) {
		AfxMessageBox(_T("连接失败!"));
		TRACE("连接失败: %d %s\r\n",WSAGetLastError(),GetErrorInfo(WSAGetLastError()).c_str());
		return FALSE;
	}
	return TRUE;

}
/** 处理命令 */
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
 * @purpose:网络环境初始化
 */
BOOL CClientSocket::InitSockEnv() {
	//套接字初始化
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
CPacket::CPacket(const BYTE* pData, size_t& nSize) :sHead(0), nLength(0), sCmd(0), sSum(0) {
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
	memcpy(pData, strData.c_str(), strData.size());
	pData += strData.size();
	*(WORD*)pData = sSum;

	return strOut.c_str();
}
