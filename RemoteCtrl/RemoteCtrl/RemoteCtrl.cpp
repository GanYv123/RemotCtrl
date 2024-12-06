// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "Command.h"
#include "ServerSocket.h"
#include "EdoyunTool.h"
#include <conio.h>
#include "CEdoyunQueue.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include "EdoyunServer.h"

#define INVOKE_PATH _T("C:\\Users\\op\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe")
//#define INVOKE_PATH _T("C:\\Windows\\SysWOW64\\RemoteCtrl.exe")

// 唯一的应用程序对象
CWinApp theApp;
using namespace std;

bool ChooseAutoInvoke(const CString& strPath) {
	TCHAR wcsSystem[MAX_PATH] = _T("");

	if (PathFileExists(strPath)) {
		return true;
	}
	CString strInfo = _T("该程序只允许用于合法的用途!\n");
	strInfo += _T("继续运行该程序，将使得该机器处于被监控状态\n");
	strInfo += _T("按下\"取消\"按钮 结束操作 退出程序！\n");
	strInfo += _T("按下\"是\"按钮 会将程序复制到机器上，并随系统启动而自动运行!\n");
	strInfo += _T("按下\"否\"按钮 将只会运行一次!\n");
	int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
	if (ret == IDYES) {
		//WriteRegisterTable(strPath);
		if (!CEdoyunTool::WriteStartupDir(strPath)) {
			MessageBox(NULL, _T("复制文件失败，是否权限不足?\r\n"), _T("error!"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
	}
	else if (ret == IDCANCEL) {
		return false;
	}
	return true;
}


void iocp() {
	EdoyunServer server;
	server.StartService();
	getchar();
}

void udp_server();
void udp_client(bool ishost = true);

void initsock() {
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
}

void clearsock() {
	WSACleanup();
}

int main(int argc, char* argv[]) {
	if (!CEdoyunTool::Init()) return 1;
	initsock();
	if (argc == 1) {
		char wstrDir[MAX_PATH];
		GetCurrentDirectoryA(MAX_PATH, wstrDir);
		STARTUPINFOA si;
		PROCESS_INFORMATION pi;
		memset(&si, 0, sizeof(si));
		memset(&pi, 0, sizeof(pi));
		string strCmd = argv[0];
		strCmd += " 1";
		BOOL bRet = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, 0, NULL, wstrDir, &si, &pi);
		if (bRet) {
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
			TRACE("进程ID：%d\r\n", pi.dwProcessId);
			TRACE("线程ID：%d\r\n", pi.dwThreadId);

			strCmd += " 2";
			bRet = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, 0, NULL, wstrDir, &si, &pi);
			if (bRet) {
				CloseHandle(pi.hThread);
				CloseHandle(pi.hProcess);
				TRACE("进程ID：%d\r\n", pi.dwProcessId);
				TRACE("线程ID：%d\r\n", pi.dwThreadId);
				udp_server();//服务器
			}
		}
	}
	else if (argc == 2) {//主客户端
		udp_client();
	}
	else {//从客户端
		udp_client(false);
	}
	clearsock();
	//iocp();
	//*//
	if (CEdoyunTool::IsAdmin()) {
		if (!CEdoyunTool::Init()) return 1;
		if (ChooseAutoInvoke(INVOKE_PATH)) {
			CCommand cmd;
			int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);
			switch (ret) {
			case -1:
				MessageBox(NULL, _T("网络初始化异常，请检查网络状态!"), _T("网络初始化失败!"), MB_OK | MB_ICONERROR);
				break;
			case -2:
				MessageBox(NULL, _T("多次接入用户失败，结束程序!"), _T("网络初始化失败!"), MB_OK | MB_ICONERROR);
				break;
			}
		}
	}
	else {
		if (!CEdoyunTool::RunAsAdmin()) {
			CEdoyunTool::ShowError();
			return 1;
		}
	}
	//*/
	return 0;
}
#include "ESocket.h"
#include "ENetWork.h"

int RecvFromCB(void* arg, const EBuffer& buffer, ESockaddrIn& addr) {
	EServer* server = (EServer*)arg;
	return server->Sendto(addr, buffer);
}
int SendToCB(void* arg, const ESockaddrIn& addr, int ret) {
	EServer* server = (EServer*)arg;
	printf("sendto done!%p\r\n", server);
	return 0;
}

void udp_server() {
	std::list<ESockaddrIn> lst_clients;
	EServerParameter param(
		"127.0.0.1",
		20000,
		ETYPE::ETypeUDP,
		NULL,
		NULL,
		NULL,
		RecvFromCB,
		SendToCB
	);
	EServer server{param};
	server.Invoke(&server);
	printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
	getchar();
	return;
}

void udp_client(bool ishost) {
	Sleep(1000);
	sockaddr_in server, client;
	int len = sizeof(client);
	memset(&server, 0, sizeof(sockaddr_in));
	memset(&client, 0, sizeof(sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(20000);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");

	SOCKET sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) {
		printf("%s(%d):%s error!\r\n", __FILE__, __LINE__, __FUNCTION__);
		return;
	}

	if (ishost) { // 主客户端
		EBuffer msg{ "hello world!\n" };
		int ret = sendto(sock, msg.c_str(), msg.size(), 0, (PSOCKADDR)&server, sizeof(server));
		printf("[主客户端] %s(%d):%s ret=%d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);

		if (ret > 0) {
			msg.resize(1024);
			memset((char*)msg.c_str(), 0, msg.size());
			ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (PSOCKADDR)&client, &len);
			printf("[主客户端] %s(%d):%s error!:(%d) ret=%d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
			if (ret > 0) {
				printf("[主客户端] %s(%d):%s ip:%08X port:%d\r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr, ntohs(client.sin_port));
				printf("[主客户端] %s(%d):%s msg size=%d\r\n", __FILE__, __LINE__, __FUNCTION__, msg.size());
			}
			//第二次接收来自client2的通信
			ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (PSOCKADDR)&client, &len);
			printf("[主客户端] %s(%d):%s error!:(%d) ret=%d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
			if (ret > 0) {
				printf("[recv从->主] %s(%d):%s client_ip:%08X client_port:%d\r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr, ntohs(client.sin_port));
				printf("[recv从->主] %s(%d):%s msg = %s\r\n", __FILE__, __LINE__, __FUNCTION__, msg.c_str());
			}
		}
	}
	else { // 从客户端
		Sleep(1000);
		EBuffer msg{ "hello world!\n" };
		int ret = sendto(sock, msg.c_str(), msg.size(), 0, (PSOCKADDR)&server, sizeof(server));
		printf("[从客户端] %s(%d):%s ret=%d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);

		if (ret > 0) {
			msg.resize(1024);
			memset((char*)msg.c_str(), 0, msg.size());
			ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (PSOCKADDR)&client, &len);
			printf("[从客户端] %s(%d):%s error!:(%d) ret=%d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);

			if (ret > 0) {
				sockaddr_in addr;
				memcpy(&addr, msg.c_str(), sizeof(addr));
				sockaddr_in* pAddr = (sockaddr_in*)&addr;
				printf("[从客户端] %s(%d):%s ip:%08X port:%d\r\n", __FILE__, __LINE__, __FUNCTION__, pAddr->sin_addr.s_addr, ntohs(pAddr->sin_port));
				printf("[从客户端] %s(%d):%s msg size=%d\r\n", __FILE__, __LINE__, __FUNCTION__, msg.size());

				msg = "hello, i am client2\r\n";
				ret = sendto(sock, (char*)msg.c_str(), msg.size(), 0, (PSOCKADDR)pAddr, sizeof(sockaddr_in));
				printf("[send从->主] %s(%d):%s ip:%08X port:%d\r\n", __FILE__, __LINE__, __FUNCTION__, pAddr->sin_addr.s_addr, ntohs(pAddr->sin_port));
				printf("[send从->主] %s(%d):%s error!:(%d) ret=%d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
				printf("[send从->主] %s(%d):%s error!:(%d) msg=%s\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), msg.c_str());
			}
		}
	}
	closesocket(sock);
}
