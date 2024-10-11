#pragma once
#include <WinSock2.h>
#include <memory>
#include <string>
#pragma warning(disable : 4996)
enum class ETYPE {
	ETypeTCP = 1,
	ETypeUDP
};

class ESockaddrIn {
public:
	ESockaddrIn() {
		memset(&m_addr, 0, sizeof(m_addr));
		m_port = -1;
	}
	ESockaddrIn(sockaddr_in addr) {
		memcpy(&m_addr, &addr, sizeof(m_addr));
		m_ip = inet_ntoa(m_addr.sin_addr);
		m_port = ntohs(m_addr.sin_port);
	}
	ESockaddrIn(UINT nIP, short nPort) {
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(nPort);
		m_addr.sin_addr.S_un.S_addr = htonl(nIP);
		m_ip = inet_ntoa(m_addr.sin_addr);
		m_port = nPort;
	}
	ESockaddrIn(std::string strIP, short nPort) {
		m_ip = strIP;
		m_port = nPort;
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(nPort);
		m_addr.sin_addr.S_un.S_addr = inet_addr(strIP.c_str());
	}
	ESockaddrIn(const ESockaddrIn& addr) {
		memcpy(&m_addr, &addr.m_addr, sizeof(m_addr));
		m_ip = addr.m_ip;
		m_port = addr.m_port;
	}
	ESockaddrIn& operator=(const ESockaddrIn& addr) {
		if (this != &addr) {
			memcpy(&m_addr, &addr.m_addr, sizeof(m_addr));
			m_ip = addr.m_ip;
			m_port = addr.m_port;
		}
		return *this;
	}
	// 转换为 sockaddr*
	inline operator sockaddr* () {
		return (sockaddr*)&m_addr;
	}

	// 转换为 const sockaddr*
	inline operator const sockaddr* () const {
		return (const sockaddr*)&m_addr;
	}
	inline operator void* ()const {
		return (void*)&m_addr;
	}
	inline void update() {
		m_ip = inet_ntoa(m_addr.sin_addr);
		m_port = ntohs(m_addr.sin_port);
	}
	inline std::string GetIP()const { return m_ip; }
	inline short GetPort()const { return m_port; }
	inline int Size()const { return sizeof(sockaddr_in); }
private:
	sockaddr_in m_addr;
	std::string m_ip;
	short m_port;
};

class EBuffer :public std::string {
public:
	EBuffer(const char* str) {
		resize(strlen(str));
		memcpy((void*)c_str(), str, size());
	}
	EBuffer(size_t size = 0):std::string(){
		if (size > 0){
			resize(size);
			memset(*this,0,this->size());
		}
	}
	~EBuffer() {
		std::string::~basic_string();
	}
	EBuffer(void* buffer, size_t size) :std::string() {
		resize(size);
		memcpy((void*)c_str(), buffer, size);
	}
	// 提供转换操作符
	operator char* () {
		return const_cast<char*>(c_str()); // 注意：c_str() 返回的是 const char*
	}
	// 为了安全，可以考虑提供 const char* 的转换
	operator const char* () const {
		return c_str(); // const 类型的转换
	}
	// 提供一个将 std::string 转换为 BYTE* 的操作符
	operator BYTE* () const {
		return (BYTE*)c_str();
	}
	// 提供一个将 std::string 转换为 void* 的操作符
	operator void* () const {
		return (void*)c_str();
	}
	void Update(void* buffer, size_t size) {
		resize(size);
		memcpy((void*)c_str(), buffer, size);
	}
};

class ESocket {
public:
	/// <summary>
	/// 
	/// </summary>
	/// <param name="nType"> 参见{enum class ETYPE} </param>
	/// <param name="nProtocol"> 默认0系统将根据套接字类型自动选择适当的协议 </param>
	ESocket(ETYPE nType = ETYPE::ETypeTCP, int nProtocol = 0) {
		m_socket = socket(PF_INET, static_cast<int>(nType), nProtocol);
		m_type = nType;
		m_protocol = nProtocol;
	}

	ESocket(const ESocket& sock) {
		m_socket = socket(PF_INET, static_cast<int>(sock.m_type), sock.m_protocol);
		m_type = sock.m_type;
		m_protocol = sock.m_protocol;
		m_addr = sock.m_addr;
	}
	ESocket& operator=(const ESocket& sock) {
		if (this != &sock) {
			m_socket = socket(PF_INET, static_cast<int>(sock.m_type), sock.m_protocol);
			m_type = sock.m_type;
			m_protocol = sock.m_protocol;
			m_addr = sock.m_addr;
		}
		return *this;
	}

	~ESocket() {
		Close();
	}
	inline operator SOCKET() const { return m_socket; }
	inline operator SOCKET() { return m_socket; }
	inline bool operator==(SOCKET sock) const { return m_socket == sock; }

	int listen(int backlog = 5) {
		if (m_type != ETYPE::ETypeTCP) return -1;
		return ::listen(m_socket,backlog);
	}
	
	int bind(const std::string& ip, short port) {
		m_addr = ESockaddrIn(ip, port);
		return ::bind(m_socket, m_addr, m_addr.Size());
	}

	int accept() {

	}

	int connect(const std::string& ip, short port) {

	}

	int send(const EBuffer& buffer) {
		return ::send(m_socket, buffer, buffer.size(), 0);
	}

	int recv(EBuffer& buffer) {
		return ::recv(m_socket, buffer, buffer.size(), 0);
	}

	int sendto(const EBuffer& buffer, const ESockaddrIn& to){
		return ::sendto(m_socket, buffer, buffer.size(), 0, to, to.Size());
	}

	int recvfrom(EBuffer& buffer, ESockaddrIn& from) {
		int len = from.Size();
		int ret = ::recvfrom(m_socket, buffer, buffer.size(), 0, from, &len);
		if (ret > 0) {
			from.update();
		}
		return ret;
	}
	void Close() {
		if(m_socket != INVALID_SOCKET)
		{
			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
		}
	}
	int recvfrom(){}

private:
	SOCKET m_socket;
	ETYPE m_type;
	int m_protocol;
	ESockaddrIn m_addr;

};

typedef std::shared_ptr<ESocket> ESOCKET;
