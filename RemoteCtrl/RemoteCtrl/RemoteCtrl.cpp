// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;

void Dump(BYTE* pData,size_t nSize) {
	std::string strOut;
	for (size_t i = 0; i < nSize; i++) {
		char buf[8] = "";
		if (i > 0 && (i % 16 == 0)) strOut += "\n ";
		snprintf(buf,sizeof(buf),"%02X",pData[i]&0xFF);
		strOut += buf;
	}
	strOut += "\n";
	OutputDebugStringA(strOut.c_str());
}

int MakeDriverInfo() {//1==>A 2==>B ...
	std::string result;
	for (size_t i = 1; i <= 26; i++) { //改变当前驱动
		if (_chdrive(i) == 0) {
			if (result.size()) {
				result += ',';
			}
			result += 'A' + i - 1;
		}
	}
	CPacket pack(1, (BYTE*)result.c_str(), result.size());
	Dump((BYTE*)pack.Data(), pack.size());
	//CServerSocket::getInstance()->Send(pack);
	return 0;
}

#include <io.h>
#include <list>

typedef struct file_info
{
	file_info() {
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;  //是否为失效文件
	BOOL IsDirectory;//是否为目录
	BOOL HasNext;	 //是否还有后续文件
	char szFileName[256];

}FILEINFO,*PFILEINFO;

int MakeDirectoryInfo() {
	std::string strPath;
	//std::list<FILEINFO> lstFileInfos;
	
	if (CServerSocket::getInstance()->getFilePath(strPath) == FALSE) {
		OutputDebugString(_T("当前命令不是获取文件列表，命令解析错误!"));
		return -1;
	}
	if (_chdir(strPath.c_str()) != 0) { //切换目录失败
		FILEINFO finfo;
		finfo.IsInvalid = TRUE;
		finfo.IsDirectory = TRUE;
		finfo.HasNext = FALSE;
		memcpy(finfo.szFileName, strPath.c_str(), strPath.size());
		//lstFileInfos.push_back(finfo);
		CPacket pack(2,(BYTE*)&finfo,sizeof(finfo));
		CServerSocket::getInstance()->Send(pack);
		OutputDebugString(_T("没有权限访问目录!"));
		return -2;
	}

	_finddata_t fdata;
	int hfind = 0;

	if ((hfind = _findfirst("*", &fdata)) == -1) {
		OutputDebugString(_T("没有找到任何文件!"));
		return -3;
	}

	do 
	{
		FILEINFO finfo;
		finfo.IsDirectory = ((fdata.attrib & _A_SUBDIR) != 0);
		memcpy(finfo.szFileName,fdata.name,strlen(fdata.name));
		//lstFileInfos.push_back(finfo);
		CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
		CServerSocket::getInstance()->Send(pack);

	} while (!_findnext(hfind,&fdata));

	FILEINFO finfo;
	finfo.HasNext = FALSE;
	CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
	CServerSocket::getInstance()->Send(pack);

	return 0;
}

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
			//CServerSocket* pserver = CServerSocket::getInstance();
			//int count = 0;
			//
			//if (pserver->initSocket() == FALSE) {
			//	MessageBox(NULL, _T("网络初始化异常，请检查网络状态!"), _T("网络初始化失败!"), MB_OK | MB_ICONERROR);
			//	exit(0);
			//}
			//
			//while (CServerSocket::getInstance() != nullptr) {
			//	if (pserver->AcceptClient() == FALSE) {
			//		if (count >= 3) {
			//			MessageBox(NULL, _T("多次接入用户失败，结束程序!"), _T("网络初始化失败!"), MB_OK | MB_ICONERROR);
			//			exit(0);
			//
			//		}
			//		MessageBox(NULL, _T("接入用户失败,无法正常接入用户自动重试!"), _T("网络初始化失败!"), MB_OK | MB_ICONERROR);
			//		count++;
			//	}
			//	/*接收不同的命令 处理不同的响应*/
			//	int ret = pserver->DealCommand();
			//	//TODO:
			//}

			int nCmd = 1;
			switch (nCmd) {
			case 1:
				MakeDriverInfo();//查看磁盘分区
				break;
			case 2:
				MakeDirectoryInfo();
			default:
				break;
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
