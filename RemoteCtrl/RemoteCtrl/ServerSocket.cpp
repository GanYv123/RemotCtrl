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

BOOL CServerSocket::initSocket(short port) {
	// TODO: 1.socket bind listen accept read/write close;
	if (m_serv_socket == -1) { return FALSE; }
	SOCKADDR_IN serv_adr;
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(port);
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

int CServerSocket::Run(SOCK_CALLBACK callback, void* arg, short port) {
	// 初始化服务器套接字，绑定到指定端口
	bool ret = initSocket(port);
	if(ret == false){
		return -1; // 初始化失败，返回错误码 -1
	}

	// 用于存储需要发送的响应数据包
	std::list<CPacket> lstPackets;

	// 设置回调函数及其参数，用于处理业务逻辑
	m_callback = callback; // 回调函数指针
	m_arg = arg;           // 回调函数的附加参数

	// 计数器，用于记录连续客户端连接失败次数
	int count = 0;

	// 主循环：处理客户端请求
	while(true){
		// 等待并接受客户端连接
		if(AcceptClient() == false){
			// 如果连接失败，累加失败次数
			if(count >= 3){
				return -2; // 如果失败次数达到 3 次，退出程序返回错误码 -2
			}
			count++;
			continue; // 跳过本次循环，继续等待连接
		}

		// 处理客户端命令并获取返回值
		int ret = DealCommand();

		// 如果有有效的返回值
		if(ret > 0){
			// 调用用户自定义回调函数（runCommand），传入必要参数
			m_callback(m_arg, ret, lstPackets, m_packet);

			// 遍历待发送的响应数据包列表
			while(!lstPackets.empty()){
				// 发送列表中的第一个数据包到客户端
				Send(lstPackets.front());
				// 删除已发送的数据包
				lstPackets.pop_front();
			}
		}

		// 关闭当前客户端连接
		closeClient();
	}

	return 0; // 正常结束，返回 0
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

