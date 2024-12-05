#include "pch.h"
#include "EdoyunServer.h"
#include "EdoyunTool.h"
#pragma warning(disable:4407)

template<EdoyunOperator op>
AcceptOverlapped<op>::AcceptOverlapped() {
	m_operator = EAccept;
	m_worker = ThreadWorker(this, (FUNCTYPE)&AcceptOverlapped<op>::AcceptWorker);
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024);
	m_server = NULL;
}
template<EdoyunOperator op>
int AcceptOverlapped<op>::AcceptWorker() {
	INT lLength = 0, rLength = 0;
	if (m_client->GetBufferSize() > 0) {
		sockaddr* pLocal = NULL, * premote = NULL;
		GetAcceptExSockaddrs(*m_client, 0,
			sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			(sockaddr**)&pLocal, &lLength,
			(sockaddr**)&premote, &rLength
		);
		memcpy(m_client->GetLocalAddr(), pLocal, sizeof(sockaddr_in));
		memcpy(m_client->GetRemoteAddr(), premote, sizeof(sockaddr_in));
		m_server->BindNewSocket(*m_client);
		int ret = WSARecv((SOCKET)*m_client, m_client->RecvWSABuffer(), 1,
			*m_client, &m_client->flags(), m_client->RecvOverlapped(), NULL);
		if (ret == SOCKET_ERROR && (WSAGetLastError() != WSA_IO_PENDING)) {
			TRACE("ret = %d\r\n", ret);
		}
		if (!m_server->NewAccept()) {
			return -2;
		}
	}
	return -1;
}

template<EdoyunOperator op>
inline RecvOverlapped<op>::RecvOverlapped() {
	m_operator = ERecv;
	m_worker = ThreadWorker(this, (FUNCTYPE)&RecvOverlapped<op>::RecvWorker);
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024 * 256);
}
template<EdoyunOperator op>
inline SendOverlapped<op>::SendOverlapped() {
	m_operator = op;
	m_worker = ThreadWorker(this, FUNCTYPE & SendOverlapped<op>::SendWorker);
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024 * 256);
}
template<EdoyunOperator op>
inline ErrorOverlapped<op>::ErrorOverlapped() {
	m_operator = op;
	m_worker = ThreadWorker(this, (FUNCTYPE)&ErrorOverlapped<op>::ErrorWorker);
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024);
}

EdoyunClient::EdoyunClient()
	:m_isbusy(false), m_flags(0),
	m_overlapped(new ACCEPTOVERLAPPED()),
	m_recv(new RECVOVERLAPPED()),
	m_send(new SENDOVERLAPPED()),
	m_vecSend(this, (SENDCALLBACK)&EdoyunClient::SendData) {
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	m_buffer.resize(1024);
	memset(&m_laddr, 0, sizeof(m_laddr));
	memset(&m_raddr, 0, sizeof(m_raddr));
}

void EdoyunClient::SetOverlapped(PCLIENT& ptr) {
	m_overlapped->m_client = ptr.get();
	m_recv->m_client = ptr.get();
	m_send->m_client = ptr.get();
}

EdoyunClient::operator LPOVERLAPPED() {
	return &m_overlapped->m_overlapped;
}

LPWSABUF EdoyunClient::RecvWSABuffer() {
	return &(m_recv->m_wsabuffer);
}

LPOVERLAPPED EdoyunClient::RecvOverlapped() {
	return &m_recv->m_overlapped;
}

LPWSABUF EdoyunClient::SendWSABuffer() {
	return &(m_send->m_wsabuffer);
}

LPOVERLAPPED EdoyunClient::SendOverlapped() {
	return &m_send->m_overlapped;
}

int EdoyunClient::Recv() {
	int ret = recv(m_sock, m_buffer.data() + m_used, m_buffer.size() - m_used, 0);
	if (ret <= 0)return -1;
	m_used += (size_t)ret;
	CEdoyunTool
		::Dump((BYTE*)m_buffer.data(), ret);
	return 0;
}

int EdoyunClient::Send(void* buffer, size_t nSize) {

	std::vector<char>data(nSize);
	memcpy(data.data(), buffer, nSize);
	if (m_vecSend.PushBack(data)) {
		return 0;
	}
	return -1;
}

int EdoyunClient::SendData(std::vector<char>& data) {
	if (m_vecSend.Size() > 0) {
		int ret = WSASend(m_sock, SendWSABuffer(), 1, &m_received, m_flags,
			&m_send->m_overlapped, NULL);
		if (ret != 0 && (WSAGetLastError() != WSA_IO_PENDING)) {
			CEdoyunTool::ShowError();
			return -1;
		}
	}
	return 0;
}

EdoyunServer::~EdoyunServer() {
	closesocket(m_sock);
	auto it = m_client.begin();
	for (; it != m_client.end(); it++) {
		it->second.reset();
	}
	m_client.clear();
	CloseHandle(m_hIOCP);
	m_pool.Stop();
	WSACleanup();
}

bool EdoyunServer::StartService() {
	CreateSocket();
	if (bind(m_sock, (sockaddr*)&m_addr, sizeof(m_addr)) == -1) {
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
	if (m_hIOCP == NULL) {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		m_hIOCP = INVALID_HANDLE_VALUE;
		return false;
	}
	CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0);
	m_pool.Invoke();
	m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&EdoyunServer::threadIocp));
	if (!NewAccept())return false;
	return true;
}

bool EdoyunServer::NewAccept() {
	PCLIENT pClient(new EdoyunClient());
	pClient->SetOverlapped(pClient);
	m_client.insert({ *pClient,pClient });
	if (AcceptEx(m_sock, *pClient, *pClient, 0,
		sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
		*pClient, *pClient) == FALSE) {
		TRACE("%d\r\n", WSAGetLastError());
		if (WSAGetLastError() != WSA_IO_PENDING) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			m_hIOCP = INVALID_HANDLE_VALUE;
			return false;
		}
	}
	return true;
}

void EdoyunServer::BindNewSocket(SOCKET s) {
	CreateIoCompletionPort((HANDLE)s, m_hIOCP, (ULONG_PTR)this, 0);
}

void EdoyunServer::CreateSocket() {
	WSADATA data;
	if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
		TRACE("初始化套接字失败\r\n");
		return;
	}
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	int opt = 1;
	setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
}

int EdoyunServer::threadIocp() {
	DWORD dwTransferred = 0;
	ULONG_PTR CompletionKet = 0;
	OVERLAPPED* lpOverlapped = NULL;
	if (GetQueuedCompletionStatus(m_hIOCP,
		&dwTransferred, &CompletionKet, &lpOverlapped, INFINITE)) {
		if (CompletionKet != 0) {
			EdoyunOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, EdoyunOverlapped, m_overlapped);
			pOverlapped->m_server = this;
			switch (pOverlapped->m_operator) {
			case EAccept:
			{
				ACCEPTOVERLAPPED* pOver = (ACCEPTOVERLAPPED*)pOverlapped;
				m_pool.DispatchWorker(pOver->m_worker);//这里地址要注意,不是void的话会传另一个父类的地址
			}
			break;
			case ERecv:
			{
				RECVOVERLAPPED* pOver = (RECVOVERLAPPED*)pOverlapped;
				m_pool.DispatchWorker(pOver->m_worker);
			}
			break;
			case ESend:
			{
				SENDOVERLAPPED* pOver = (SENDOVERLAPPED*)pOverlapped;
				m_pool.DispatchWorker(pOver->m_worker);
			}
			break;
			case EError:
			{
				ERROROVERLAPPED* pOver = (ERROROVERLAPPED*)pOverlapped;
				m_pool.DispatchWorker(pOver->m_worker);
			}
			break;
			default:
				break;
			}
		}
		else {
			return -1;
		}


	}
	return 0;
}