﻿// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
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

int main(int argc, char* argv[]) {
	if (!CEdoyunTool::Init()) return 1;

	if (argc == 1) {
		char wstrDir[MAX_PATH];
		GetCurrentDirectoryA(MAX_PATH, wstrDir);
		STARTUPINFOA si;
		PROCESS_INFORMATION pi;
		memset(&si, 0, sizeof(si));
		memset(&pi, 0, sizeof(pi));
		string strCmd = argv[0];
		strCmd += " 1";
		BOOL bRet = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, wstrDir, &si, &pi);
		if (bRet) {
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
			TRACE("进程ID：%d\r\n", pi.dwProcessId);
			TRACE("线程ID：%d\r\n", pi.dwThreadId);

			strCmd += " 2";
			bRet = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, wstrDir, &si, &pi);
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

	//iocp();
	/*//
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

void udp_server() {
	
	printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
	getchar();
}

void udp_client(bool ishost) {
	if(ishost)
	{	
		printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
	}
	else 
	{
		printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
	}
}