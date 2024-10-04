#pragma once
#pragma warning(disable : 4996)
#pragma warning(disable : 4407)

#include "EdoyunThread.h"
#include <map>
#include "CEdoyunQueue.h"
#include <MSWSock.h>

enum EdoyunOperator {
	ENone,
	EAccept,
	ERecv,
	ESend,
	EError
};

class EdoyunServer;
class EdoyunClient;
typedef std::shared_ptr<EdoyunClient> PCLIENT;

class EOverlapped {
public:
	OVERLAPPED m_overlapped;
	DWORD m_operator;
	std::vector<char> m_buffer;
	ThreadWorker m_worker;
	EdoyunServer* m_server;
	EOverlapped():  m_server() {
		m_operator = 0;
		memset(&m_overlapped, 0, sizeof(OVERLAPPED));
		memset(&m_buffer, 0, sizeof(m_buffer));
	}

};

// EAccept
template <EdoyunOperator op>
class AcceptOverlapped :public EOverlapped, ThreadFuncBase {
public:
	AcceptOverlapped();
	int AcceptWorker();
	PCLIENT m_client;
private:

};
typedef AcceptOverlapped<EAccept> ACCEPTOVERLAPPED;

// ERecv
template <EdoyunOperator>
class RecvOverlapped :public EOverlapped, ThreadFuncBase {
public:
	RecvOverlapped() :
		m_operator(EAccept),
		m_worker(this, &RecvOverlapped::RecvWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024 * 256);
	}
	int RecvWorker() {
		// TODO
	}
private:

};
typedef RecvOverlapped<ERecv> RECVOVERLAPPED;

// ESend
template <EdoyunOperator>
class SendOverlapped :public EOverlapped, ThreadFuncBase {
public:
	SendOverlapped() :
		m_operator(ESend),
		m_worker(this, &SendOverlapped::SendWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024 * 256);
	}
	int SendWorker() {
		// TODO
	}
private:

};
typedef SendOverlapped<ESend> SENDOVERLAPPED;


// EAccept
template <EdoyunOperator>
class ErrorOverlapped :public EOverlapped, ThreadFuncBase {
public:
	ErrorOverlapped() :
		m_operator(EAccept),
		m_worker(this, &ErrorOverlapped::ErrorWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024);
	}
	int ErrorWorker() {
		// TODO
	}
private:

};
typedef ErrorOverlapped<EError> ERROROVERLAPPED;
// ============================================================================
class EdoyunClient {
public:
	EdoyunClient();

	~EdoyunClient() {
		closesocket(m_sock);
	}

	void SetOverlapped(PCLIENT& ptr) {
		m_overlapped.m_client = ptr;
	}

	operator SOCKET() {
		return m_sock;
	}
	operator PVOID() {
		return &m_buffer[0];
	}
	operator LPOVERLAPPED() {
		return &m_overlapped.m_overlapped;
	}
	operator LPDWORD() {
		return &m_received;
	}

	sockaddr_in* GetLocalAddr() { return &m_laddr; }
	sockaddr_in* GetRemoteAddr() { return &m_raddr; }

private:
	SOCKET m_sock;
	DWORD m_received;
	ACCEPTOVERLAPPED m_overlapped;
	std::vector<char> m_buffer;
	sockaddr_in m_laddr;
	sockaddr_in m_raddr;
	bool isBusy;
};
// ============================================================================
class EdoyunServer : public ThreadFuncBase {
public:
	EdoyunServer(const std::string& ip = "0.0.0.0", const short& port = 2233) : m_pool(10) {
		m_hIOCP = INVALID_HANDLE_VALUE;
		m_sock = INVALID_SOCKET;
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(port);
		m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
	}
	bool StartServer() {
		CreateSocket();
		if (bind(m_sock, (sockaddr*)&m_addr, sizeof(m_addr)) == false) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			return false;
		}
		if (listen(m_sock, 3) == -1) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			return false;
		}
		m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
		if (m_hIOCP == NULL || m_hIOCP == INVALID_HANDLE_VALUE) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			m_hIOCP = INVALID_HANDLE_VALUE;
			return false;
		}
		CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0);
		m_pool.Invoke();
		m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&EdoyunServer::threadIocp));
		return NewAccept();
	}
	~EdoyunServer() {}
public:
	void CreateSocket() {
		m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		int opt = 1;
		setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
	}

	bool NewAccept() {
		PCLIENT pClient(new EdoyunClient());
		pClient->SetOverlapped(pClient);
		m_client.insert(std::pair <SOCKET, PCLIENT>(*pClient, pClient));

		if (!AcceptEx(m_sock, *pClient, *pClient, 0,
			sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, *pClient, *pClient)) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			m_hIOCP = INVALID_HANDLE_VALUE;
			return false;
		}
		return true;
	}

	int threadIocp() {
		DWORD transfer = 0;
		ULONG_PTR compKey = 0;
		OVERLAPPED* lpOver = NULL;
		if (GetQueuedCompletionStatus(m_hIOCP, &transfer, &compKey, &lpOver, INFINITE)) {
			EOverlapped* pO = CONTAINING_RECORD(lpOver, EOverlapped, m_overlapped);
			if (transfer > 0 && compKey != 0) {
				EOverlapped* pO = CONTAINING_RECORD(lpOver, EOverlapped, m_overlapped);
				switch (pO->m_operator) {
				case EAccept:
				{
					ACCEPTOVERLAPPED* pOver = (ACCEPTOVERLAPPED*)pO;
					m_pool.DispatchWorker(pOver->m_worker);
					break;
				}
				case ERecv:
				{
					RECVOVERLAPPED* pOver = (RECVOVERLAPPED*)pO;
					m_pool.DispatchWorker(pOver->m_worker);
					break;
				}
				case ESend:
				{
					SENDOVERLAPPED* pOver = (SENDOVERLAPPED*)pO;
					m_pool.DispatchWorker(pOver->m_worker);
					break;
				}
				case EError:
				{
					ERROROVERLAPPED* pOver = (ERROROVERLAPPED*)pO;
					m_pool.DispatchWorker(pOver->m_worker);
					break;
				}
				default:
					break;
				}
			}
			else {
				return -1;
			}

		}
		return -2;
	}
public:
	EdoyunThreadPool m_pool;
	HANDLE m_hIOCP;
	SOCKET m_sock;
	std::map<SOCKET, PCLIENT> m_client;
	CEdoyunQueue<EdoyunClient> m_lstClient;
	sockaddr_in m_addr;
};
