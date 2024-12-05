#include "pch.h"
#include "ClientSocket.h"
#define BUFFER_SIZE 4096000

CClientSocket::CClientSocket() :
	m_nIP(INADDR_ANY), m_nPort(0), m_sock(INVALID_SOCKET),
	m_bAutoClose(TRUE), m_hThread(INVALID_HANDLE_VALUE) {
	if (InitSockEnv() == FALSE) {
		MessageBox(NULL, _T("无法初始化套接字环境,请检查网络设置"), _T("初始化套接字错误"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	m_eventInvoke = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry, this, 0, &m_nThreadID);
	if (WaitForSingleObject(m_eventInvoke, 100) == WAIT_TIMEOUT) {
		TRACE("网络消息处理 线程启动失败!\r\n");
	}
	CloseHandle(m_eventInvoke);
	m_buffer.resize(BUFFER_SIZE);
	memset(m_buffer.data(), 0, BUFFER_SIZE);
	struct {
		UINT message;
		MSGFUNC func;
	}funcs[] = {
		{WM_SEND_PACK,&CClientSocket::SendPack},
		{0,NULL}
	};
	for (int i = 0; funcs[i].message != 0; i++) {
		if (m_mapFunc.insert(std::make_pair(funcs[i].message, funcs[i].func)).second == false) {
			TRACE("插入失败,消息值:%d  函数值:%08x 序号:%d\r\n", funcs[i].message, funcs[i].func, i);
		}
	}
}

CClientSocket::CClientSocket(const CClientSocket& ss) {
	m_hThread = ss.m_hThread;
	m_bAutoClose = ss.m_bAutoClose;
	m_sock = ss.m_sock;
	m_nIP = ss.m_nIP;
	m_nPort = ss.m_nPort;
	std::map<UINT, CClientSocket::MSGFUNC>::const_iterator it = ss.m_mapFunc.begin();
	for (; it != ss.m_mapFunc.end(); it++) {
		m_mapFunc.insert(std::make_pair(it->first, it->second));
	}
}

CClientSocket::~CClientSocket() {
	closesocket(m_sock);
	m_sock = INVALID_SOCKET;
	WSACleanup();
}

unsigned CClientSocket::threadEntry(void* arg) {
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc2();
	_endthreadex(0);
	return 0;
}

void CClientSocket::threadFunc2() {
	SetEvent(m_eventInvoke);
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		TRACE("Get Message:%08X \r\n", msg.message);
		if (m_mapFunc.find(msg.message) != m_mapFunc.end()) {
			(this->*m_mapFunc[msg.message])(msg.message, msg.wParam, msg.lParam);
		}
	}
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
		NULL, wsaErrCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	if (lpMsgBuf) {
		ret = (char*)lpMsgBuf;
		LocalFree(lpMsgBuf);
	}
	else {
		ret = "Unknown error.";
	}
	return ret;
}


BOOL CClientSocket::initSocket() {
	if (m_sock != INVALID_SOCKET) CloseSocket();
	m_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (m_sock == -1) { return FALSE; }
	SOCKADDR_IN serv_adr;
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.S_un.S_addr = htonl(m_nIP);
	serv_adr.sin_port = htons(m_nPort);

	if (serv_adr.sin_addr.s_addr == INADDR_NONE) {
		AfxMessageBox(_T("指定的IP地址不存在!"));
		return FALSE;
	}

	int ret = connect(m_sock, (SOCKADDR*)&serv_adr, sizeof(serv_adr));
	if (ret == -1) {
		AfxMessageBox(_T("连接失败!"));
		TRACE("连接失败: %d %s\r\n", WSAGetLastError(), GetErrorInfo(WSAGetLastError()).c_str());
		return FALSE;
	}
	TRACE("socket init done!\r\n");
	return TRUE;

}

void CClientSocket::Dump(BYTE* pData, size_t nSize) {
	std::string strOut;
	for (size_t i = 0; i < nSize; i++) {
		char buf[8] = "";
		if (i > 0 && (i % 16 == 0)) strOut += "\n ";
		snprintf(buf, sizeof(buf), "%02X", pData[i] & 0xFF);
		strOut += buf;
	}
	strOut += "\n";
	OutputDebugStringA(strOut.c_str());
}

/**
 * 处理命令函数
 * 功能：接收数据包并解析命令，通过套接字接收数据，解析为 CPacket 对象，返回解析出的命令类型。
 * 注意：存在多线程冲突和粘包/半包问题的处理。
 */
int CClientSocket::DealCommand() {
	if(m_sock == -1) return FALSE; // 如果套接字无效，直接返回 FALSE（异常状态）。

	char* buffer = m_buffer.data(); // 数据接收缓冲区，注意：直接使用成员变量可能导致多线程冲突。
	static size_t index = 0;        // 静态变量，用于记录当前缓冲区中的数据长度，多线程环境下可能引发问题。

	while(TRUE){
		// 接收数据：
		// recv 函数从 m_sock 套接字接收数据，写入 buffer 的空闲区域（从 buffer + index 开始）。
		size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);

		// 如果接收失败并且缓冲区中无残留数据，认为连接关闭或发生错误。
		if(((int)len <= 0) && ((int)index <= 0)){
			return -1; // 返回错误状态，表示接收失败。
		}

		// 累加接收到的数据长度到 index，表示缓冲区中有效数据的总长度。
		index += len;

		// 将缓冲区中的数据尝试解析为 CPacket 对象。
		len = index; // 假设整个缓冲区中都有可能构成完整的数据包。
		m_packet = CPacket((BYTE*)buffer, len); // 调用 CPacket 的构造函数解析数据包。

		if(len > 0){ // 如果成功解析出一个完整的数据包：
			// 处理粘包/半包问题：
			// 将剩余未处理的数据移动到缓冲区的开头，确保下一次解析能正确进行。
			memmove(buffer, buffer + len, index - len); // 移动剩余数据到缓冲区开头。
			index -= len; // 更新缓冲区中未处理数据的长度。

			// 返回当前数据包的命令字段（m_packet.sCmd），表示解析成功。
			return m_packet.sCmd;
		}
	}

	return -1; // 默认返回 -1，表示未能成功接收或解析数据。
}


BOOL CClientSocket::Send(const char* pData, int nSize) {
	if (m_sock == -1) return FALSE;
	return send(m_sock, pData, nSize, 0) > 0;
}

BOOL CClientSocket::Send(const CPacket& pack) {
	if (m_sock == -1)
		return FALSE;
	std::string strOut;
	pack.Data(strOut);
	return send(m_sock, strOut.c_str(), strOut.size(), 0) > 0;
}

/**
 * 发送数据包并接收服务器的响应
 *
 * @param nMsg  消息类型（此处未使用）
 * @param wParam 包含发送数据的指针（PACKET_DATA 结构体）
 * @param lParam 接收响应的窗口句柄 (HWND)
 */
void CClientSocket::SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam) {
	// 1. 解析数据结构
	PACKET_DATA data = *(PACKET_DATA*)wParam; // 提取数据
	delete (PACKET_DATA*)wParam; // 释放动态分配的内存

	size_t nTemp = data.strData.size(); // 数据长度
	HWND hWnd = (HWND)lParam;           // 接收消息的窗口句柄
	CPacket current((BYTE*)data.strData.c_str(), nTemp); // 封装成数据包对象

	// 2. 初始化套接字
	if(initSocket() == TRUE){
		// 3. 发送数据包
		int ret = send(m_sock, (char*)data.strData.c_str(), (int)data.strData.size(), 0);
		if(ret > 0){ // 数据发送成功
			size_t index = 0; // 当前接收缓冲区的有效数据索引
			std::string strBuffer; // 缓冲区
			strBuffer.resize(BUFFER_SIZE);
			char* pBuffer = (char*)strBuffer.c_str();

			// 4. 循环接收数据包
			while(m_sock != INVALID_SOCKET){ // 套接字有效时继续接收
				int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0); // 接收数据
				if((length > 0) || (index > 0)){ // 收到数据或存在未处理的残留数据
					index += (size_t)length; // 更新有效数据索引
					size_t nLen = index;    // 数据长度
					CPacket pack((BYTE*)pBuffer, nLen); // 尝试解析数据包

					if(nLen > 0){ // 数据包解析成功
						// 打印调试信息
						TRACE("ack pack %d to hWnd %08X i:%d len:%d\r\n", pack.sCmd, hWnd, index, nLen);
						TRACE("%04X\r\n", *(WORD*)(pBuffer + nLen));

						// 发送解析成功的消息给目标窗口
						::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(pack), data.wParam);

						// 根据模式自动关闭套接字
						if(data.nMode & CSM_AUTOCLOSE){
							CloseSocket();
							return;
						}

						// 处理剩余未解析的数据
						index -= nLen;
						memmove(pBuffer, pBuffer + nLen, index); // 移动剩余数据到缓冲区起始位置
					}
				} else{ // 未收到数据或网络异常
					TRACE("recv failed length %d index %d\r\n");
					CloseSocket();

					// 通知接收窗口异常情况
					::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(current.sCmd, NULL, 0), 1);
				}
			}
		} else{ // 数据发送失败
			CloseSocket();
			::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -1); // 通知窗口网络错误
		}
	} else{
		// 套接字初始化失败，通知窗口错误
		::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -2);
	}
}



BOOL CClientSocket::SendPacket(HWND hWnd, const CPacket& pack, BOOL isAutoClosed, WPARAM wParam) {
	UINT nMode = isAutoClosed ? CSM_AUTOCLOSE : 0;
	std::string strOut;
	pack.Data(strOut);
	PACKET_DATA* pData = new PACKET_DATA(strOut.c_str(), strOut.size(), nMode, wParam);
	BOOL ret = PostThreadMessage(m_nThreadID, WM_SEND_PACK, (WPARAM)pData, (LPARAM)hWnd);
	if (ret == FALSE) delete pData;
	return ret;
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

void CClientSocket::CloseSocket() { closesocket(m_sock); m_sock = INVALID_SOCKET; }

/** @purpose:网络环境初始化*/
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
CPacket::CPacket(const BYTE* pData, size_t& nSize) :
	sHead(0), nLength(0), sCmd(0), sSum(0) {
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
		//TRACE("%s\r\n",strData.c_str()+12);
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
	nLength = unsigned(nSize + 4);//数据长度 = cmd + 校验
	sCmd = nCmd;
	if (nSize > 0) {
		strData.resize(nSize);
		memcpy(&strData[0], pData, nSize);
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

const char* CPacket::Data(std::string& strOut) const {
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
