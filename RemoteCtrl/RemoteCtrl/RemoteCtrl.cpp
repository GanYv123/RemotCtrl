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
//C:\Users\op\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup

enum {
	IocpListEmpty,
	IocpListPush,
	IocpListPop
};

typedef struct iocpParam {
	int nOperator;//操作
	std::string strData;//数据
	_beginthread_proc_type cbFunc;//回调
	iocpParam(int op, const char* sData, _beginthread_proc_type cb = NULL) {
		nOperator = op;
		strData = sData;
		cbFunc = cb;
	}
	iocpParam() {
		nOperator = -1;
	}
} IOCP_PARAM;

void threadmain(HANDLE hIOCP) {
	std::list<std::string> lstString;
	DWORD dwTransferred = 0;
	ULONG_PTR completionKey = NULL;
	OVERLAPPED* pOverlapped = NULL;
	int count = 0, count0 = 0;
	while (GetQueuedCompletionStatus(hIOCP, &dwTransferred, &completionKey, &pOverlapped, INFINITE)) {
		if ((dwTransferred == 0) || (completionKey == NULL)) {
			printf("thread is prepare to exit;");
			break;
		}
		IOCP_PARAM* pParam = (IOCP_PARAM*)completionKey;
		if (pParam->nOperator == IocpListPush) {
			lstString.push_back(pParam->strData);
			count++;
		}
		else if (pParam->nOperator == IocpListPop) {
			std::string* pStr = NULL;
			if (lstString.size() > 0) {
				pStr = new std::string(lstString.front());
				lstString.pop_front();
			}
			if (pParam->cbFunc) {
				pParam->cbFunc(pStr);
			}
			count0++;
		}
		else if (pParam->nOperator == IocpListEmpty) {
			lstString.clear();
		}
		delete pParam;
		printf("thread count0:%d  count:%d\r\n", count0, count);
	}
}

void threadQueueEntry(HANDLE hIOCP) {
	threadmain(hIOCP);
	_endthread();
}

void func(void* arg) {
	std::string* pstr = (std::string*)arg;
	if (pstr != NULL) {
		printf("pop from list:%s\r\n", pstr->c_str());
		delete pstr;
	}
	else {
		printf("list is not data\r\n");
	}
}

int main() {
	if (!CEdoyunTool::Init()) return 1;
	printf("press any key to exit...\r\n");
	ULONGLONG tick0 = GetTickCount64();
	ULONGLONG tick = GetTickCount64();

	CEdoyunQueue<std::string> lstStrings;
	while (_kbhit() == 0) {
		if (GetTickCount64() - tick0 > 1300) {
			lstStrings.PushBack("hello world");
			tick0 = GetTickCount64();
		}

		if (GetTickCount64() - tick > 2000) {
			std::string str;
			lstStrings.PopFront(str);
			printf("pop from queue:%s\r\n", str.c_str());
			tick = GetTickCount64();
		}
		Sleep(1);
	}

	printf("exit done!size %d\r\n",lstStrings.Size());
	lstStrings.Clear();
	printf("clear exit done!size %d\r\n", lstStrings.Size());
	exit(0);

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
