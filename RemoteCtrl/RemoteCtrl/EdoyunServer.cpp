#include "pch.h"
#include "EdoyunServer.h"

template <EdoyunOperator op>
AcceptOverlapped<op>::AcceptOverlapped() {
	m_operator = EAccept;
	m_worker = ThreadWorker(this, (FUNCTYPE)&AcceptOverlapped<op>::AcceptWorker);
	m_server = NULL;
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024);
}

template <EdoyunOperator op>
int AcceptOverlapped<op>::AcceptWorker() {
	INT lLength = 0, rLength = 0;
	if (*(LPDWORD)*m_client.get() > 0) {
		GetAcceptExSockaddrs(*m_client, 0,
			sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			(sockaddr**)m_client->GetLocalAddr(), &lLength, //本地地址
			(sockaddr**)m_client->GetRemoteAddr(), &rLength //远程地址
		); 
		//RECVOVERLAPPED
		int ret = WSARecv((SOCKET)*m_client, m_client->RecvWSABuffer(), 1, 
			*m_client, &m_client->flags(), *m_client, NULL);
		if (ret == SOCKET_ERROR && (WSAGetLastError() != WSA_IO_PENDING)) {
			//TODO:报错
		}
		if (!m_server->NewAccept()) {
			return -2;
		}
	}
	return -1;
}


EdoyunClient::EdoyunClient() :
	isBusy(false), m_received(0), m_flags(0),m_used(0),
	m_overlapped(new ACCEPTOVERLAPPED()),
	m_recv(new RECVOVERLAPPED),
	m_send(new SENDOVERLAPPED)
{
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	m_buffer.resize(1024);
	memset(&m_laddr, 0, sizeof(m_laddr));
	memset(&m_raddr, 0, sizeof(m_raddr));
}

LPWSABUF EdoyunClient::RecvWSABuffer() {
	return &m_recv->m_wsaBuffer;
}

LPWSABUF EdoyunClient::SendWSABuffer() {
	return &m_send->m_wsaBuffer;
}

bool EdoyunServer::StartServer() {
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

int EdoyunServer::threadIocp() {
	DWORD transfer = 0;
	ULONG_PTR compKey = 0;
	OVERLAPPED* lpOver = NULL;
	if (GetQueuedCompletionStatus(m_hIOCP, &transfer, &compKey, &lpOver, INFINITE)) {
		EdoyunOverlapped* pO = CONTAINING_RECORD(lpOver, EdoyunOverlapped, m_overlapped);
		if (transfer > 0 && compKey != 0) {
			EdoyunOverlapped* pO = CONTAINING_RECORD(lpOver, EdoyunOverlapped, m_overlapped);
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


template<EdoyunOperator op>
inline SendOverlapped<op>::SendOverlapped() {
	m_operator = op;
	m_worker = ThreadWorker(this, (FUNCTYPE)&SendOverlapped<op>::SendWorker);
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024 * 256);
}

template<EdoyunOperator op>
inline RecvOverlapped<op>::RecvOverlapped() {
	m_operator = op;
	m_worker = ThreadWorker(this, (FUNCTYPE)&RecvOverlapped<op>::RecvWorker);
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024 * 256);
}