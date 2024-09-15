// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Command.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 唯一的应用程序对象
CWinApp theApp;
using namespace std;

int main() {
	int nRetCode = 0;

	HMODULE hModule = ::GetModuleHandle(nullptr);

	if (hModule != nullptr) {
		// 初始化 MFC 并在失败时显示错误
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0)) {
			// TODO: 在此处为应用程序的行为编写代码。
			wprintf(L"错误: MFC 初始化失败\n");
			nRetCode = 1;
		}
		else {
			CCommand cmd;
			CServerSocket* pserver = CServerSocket::getInstance();
			int count = 0;
			if (pserver->initSocket() == FALSE) {
				MessageBox(NULL, _T("网络初始化异常，请检查网络状态!"), _T("网络初始化失败!"), MB_OK | MB_ICONERROR);
				exit(0);
			}
			
			while (CServerSocket::getInstance() != nullptr) {
				if (pserver->AcceptClient() == FALSE) {
					if (count >= 3) {
						MessageBox(NULL, _T("多次接入用户失败，结束程序!"), _T("网络初始化失败!"), MB_OK | MB_ICONERROR);
						exit(0);
					}
					MessageBox(NULL, _T("接入用户失败,无法正常接入用户自动重试!"), _T("网络初始化失败!"), MB_OK | MB_ICONERROR);
					count++;
				}
				/*接收不同的命令 处理不同的响应*/
				int ret = pserver->DealCommand();
				TRACE("DealCommand ret = %d\r\n",ret);
				if (ret > 0) {
					ret = cmd.ExcuteCommand(ret);
					if (ret != 0) {
						TRACE("执行命令失败!%d ret = %d\r\n",pserver->getPacket().sCmd,ret);
					}
					pserver->closeClient();
					TRACE("Command has done!\r\n");
				}

			}

		}
	}
	else {
		// TODO: 更改错误代码以符合需要
		wprintf(L"错误: GetModuleHandle 失败\n");
		nRetCode = 1;
	}

	return nRetCode;
}
