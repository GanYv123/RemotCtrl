#pragma once
#include "resource.h"
#include <map>
#include <atlimage.h>
#include <direct.h>
#include "EdoyunTool.h"
#include "ServerSocket.h"
#include <io.h>
#include <list>
#include "LockInfoDialog.h"

class CCommand {
public:
	CCommand();
	~CCommand() {}
	int ExcuteCommand(int nCmd);

protected:
	typedef int (CCommand::* CMDFUNC)();//成员函数指针
	std::map<int, CMDFUNC> m_mapFunction;//命令号到功能的映射
	CLockInfoDialog dlg;
	unsigned threadId;
protected:
	static unsigned int WINAPI threadLockDlg(void* arg) {
		CCommand* thiz = (CCommand*)arg;
		thiz->threadLockDlgMain();
		_endthreadex(0);
		return 0;
	}
	
	void threadLockDlgMain() {
		TRACE("%s(%d) %d\r\n", __FUNCTION__, __LINE__, GetCurrentThreadId());
		dlg.Create(IDD_DIALOG_INFO, NULL);
		CRect rect;
		//rect.left = 0; rect.top = 0; rect.right = 1920; rect.bottom = 1020;
		rect.left = 0; rect.top = 0;
		//自适应设备显示器分辨率
		rect.right = GetSystemMetrics(SM_CXSCREEN); rect.bottom = GetSystemMetrics(SM_CYSCREEN);
		TRACE("right = %04d  bottom = %04d", rect.right, rect.bottom);

		dlg.ShowWindow(SW_SHOWNA);
		dlg.MoveWindow(rect);
		//置顶窗口
		dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
		ShowCursor(FALSE);
		//隐藏任务栏
		::ShowWindow(::FindWindow(_T("Shell_Traywnd"), NULL), SW_HIDE);
		/*限制鼠标活动范围*/
		dlg.GetWindowRect(rect);
		rect.left = 0;
		rect.right = 1;
		rect.bottom = 1;
		rect.top = 0;
		ClipCursor(rect);

		/**用一个消息循环来屏蔽其他键盘按键*/
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_KEYDOWN) {
				TRACE("msg:%08X wparam:%08X lparam:%08X\r\n", msg.message, msg.wParam, msg.lParam);

				if (msg.wParam == 0x41)//按下esc退出
					break;
			}
		}
		//恢复鼠标
		ShowCursor(TRUE);
		//恢复任务栏
		::ShowWindow(::FindWindow(_T("Shell_Traywnd"), NULL), SW_SHOW);
		dlg.DestroyWindow();
	}

	int MakeDriverInfo(){//1==>A 2==>B ...
		std::string result;
		for (int i = 1; i <= 26; i++) { //改变当前驱动
			if (_chdrive(i) == 0) {
				result += 'A' + i - 1; //C,D,F,
				if (result.size()) {
					result += ',';
				}
			}
		}
		CPacket pack(1, (BYTE*)result.c_str(), result.size());
		CEdoyunTool::Dump((BYTE*)pack.Data(), pack.size());
		CServerSocket::getInstance()->Send(pack);
		return 0;
	}


	int MakeDirectoryInfo() {
		std::string strPath;
		//std::list<FILEINFO> lstFileInfos;

		if (CServerSocket::getInstance()->getFilePath(strPath) == FALSE) {
			OutputDebugString(_T("当前命令不是获取文件列表，命令解析错误!\n"));
			return -1;
		}
		TRACE("MakeDirectoryInfo()->strPath:%s\n", strPath.c_str());
		if (_chdir(strPath.c_str()) != 0) { //切换目录失败
			FILEINFO finfo;
			finfo.HasNext = FALSE;
			CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
			CServerSocket::getInstance()->Send(pack);
			OutputDebugString(_T("没有权限访问目录!\r\n"));
			return -2;
		}

		_finddata_t fdata;
		int hfind = 0;

		if ((hfind = _findfirst("*", &fdata)) == -1) {
			OutputDebugString(_T("没有找到任何文件!"));
			FILEINFO finfo;
			finfo.HasNext = FALSE;
			CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
			CServerSocket::getInstance()->Send(pack);
			return -3;
		}

		int SEND_FILE_CNT{ 0 };
		do {
			FILEINFO finfo;
			finfo.IsDirectory = ((fdata.attrib & _A_SUBDIR) != 0);
			memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
			TRACE("%s\r\n", finfo.szFileName);
			CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
			CServerSocket::getInstance()->Send(pack);
			SEND_FILE_CNT++;

		} while (!_findnext(hfind, &fdata));

		TRACE("SEND_FILE_CNT = %d\r\n\n", SEND_FILE_CNT);

		FILEINFO finfo;
		finfo.HasNext = FALSE;
		CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
		CServerSocket::getInstance()->Send(pack);

		return 0;
	}

	int RunFile() {
		std::string strPath;
		CServerSocket::getInstance()->getFilePath(strPath);
		ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
		CPacket pack(3, NULL, 0);
		CServerSocket::getInstance()->Send(pack);
		return 0;
	}

	int DownloadFile() {
		std::string strPath;
		CServerSocket::getInstance()->getFilePath(strPath);
		FILE* pFile = nullptr;
		errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
		long long data = 0;

		if (err != 0) {
			CPacket pack(4, (BYTE*)&data, sizeof(long long));
			CServerSocket::getInstance()->Send(pack);
			return -1;
		}

		if (pFile != nullptr) {
			/*获取文件大小*/
			fseek(pFile, 0, SEEK_END);
			data = _ftelli64(pFile);
			CPacket head(4, (BYTE*)&data, sizeof(long long));
			CServerSocket::getInstance()->Send(head);
			fseek(pFile, 0, SEEK_SET);

			char buffer[1024] = "";
			size_t rlen = 0;
			do {
				rlen = fread(buffer, 1, 1024, pFile);
				CPacket pack(4, (BYTE*)buffer, rlen);
				CServerSocket::getInstance()->Send(pack);
			} while (rlen >= 1024);
			fclose(pFile);
		}

		CPacket pack(4, NULL, 0);
		CServerSocket::getInstance()->Send(pack);
		return 0;
	}

	int MouseEvent() {
		MOUSEEV mouse;
		if (CServerSocket::getInstance()->getMouseEvent(mouse)) {

			DWORD nFlags = 0;
			//if(nFlags)
			switch (mouse.nButton) {
			case 0:	//左键
				nFlags = 1;
				break;
			case 1:	//右键
				nFlags = 2;
				break;
			case 2:	//中
				nFlags = 4;
				break;
			case 4:	//没有按键
				nFlags = 8;
				break;
			default:
				break;
			}

			if (nFlags != 8)SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);

			switch (mouse.nAction) {
			case 0:	//单击
				nFlags |= 0x10;
				break;
			case 1:	//双击
				nFlags |= 0x20;
				break;
			case 2:	//按下
				nFlags |= 0x40;
				break;
			case 3:	//放开
				nFlags |= 0x80;
				break;
			default:
				break;
			}
			switch (nFlags) {
				/*------------------  左 键 操 作  ------------------------*/

			case 0x21:	//左键双击
				mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
				//GetMessageExtraInfo() 获取额外信息
				mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
				//无 break 往下执行一次单击
			case 0x11:	//左键单击
				mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
				break;

			case 0x41:	//左键按下
				mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x81:	//左键放开
				mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
				break;

				/*------------------  右 键 操 作  ------------------------*/
			case 0x22:	//右键双击
				mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			case 0x12:	//右键单击
				mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
				break;

			case 0x42:	//右键按下
				mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
				break;

			case 0x82:	//右键放开
				mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
				break;
				/*------------------  中 键 操 作  ------------------------*/


			case 0x24:	//中键双击
				mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());

			case 0x14:	//中键单击
				mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
				break;

			case 0x44:	//中键按下
				mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
				break;

			case 0x84:	//中键放开
				mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
				break;

				/*------------------  鼠 标 移 动  ------------------------*/
			case 0x08:
				mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
				break;

			default:
				break;
			}
			CPacket pack(4, NULL, 0);
			CServerSocket::getInstance()->Send(pack);
		}
		else {
			OutputDebugString(_T("获取鼠标操作参数失败！"));
			return -1;
		}

		return 0;
	}

	int SendScreen() {
		CImage screen;
		HDC hScreen = ::GetDC(NULL);//获取屏幕句柄
		int nBitPerpixel = GetDeviceCaps(hScreen, BITSPIXEL);//24 rgb888
		int nWidth = GetDeviceCaps(hScreen, HORZRES);//宽度
		int nHeight = GetDeviceCaps(hScreen, VERTRES);//高
		/*GetDeviceCaps(hScreen, HORZRES) 和 GetDeviceCaps(hScreen, VERTRES):
			分别获取屏幕的水平和垂直分辨率（宽度和高度，以像素为单位）。
		*/
		screen.Create(nWidth, nHeight, nBitPerpixel);//2560 1440
		BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);
		ReleaseDC(NULL, hScreen);
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
		if (hMem == NULL) return -1;

		IStream* pStream = NULL;
		HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
		if (ret == S_OK) {
			screen.Save(pStream, Gdiplus::ImageFormatPNG);
			LARGE_INTEGER bg = { 0 };
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);
			PBYTE pData = (PBYTE)GlobalLock(hMem);
			size_t nSize = GlobalSize(hMem); //2.5k -> 457494
			CPacket pack(6, (BYTE*)pData, nSize);
			CServerSocket::getInstance()->Send(pack);
			GlobalUnlock(hMem);
		}
		pStream->Release();
		GlobalFree(hMem);
		screen.ReleaseDC();
		return 0;
	}

	int LockMachine() {
		if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE)) {
			//_beginthread(threadLockDlg, 0, NULL);
			_beginthreadex(NULL, 0, &CCommand::threadLockDlg, this, 0, &threadId);
			TRACE("threadId = %d\r\n", threadId);
		}

		CPacket pack(7, NULL, 0);
		CServerSocket::getInstance()->Send(pack);

		return 0;
	}

	int UnlockMachine() {
		//dlg.SendMessage(WM_KEYDOWN, 0x41, 0X01E0001);
		//::SendMessage(dlg.m_hWnd,WM_KEYDOWN,0X41, 0X01E0001);//Windows消息泵只能发同一线程
		::PostThreadMessage(threadId, WM_KEYDOWN, 0x41, 0);
		CPacket pack(8, NULL, 0);
		CServerSocket::getInstance()->Send(pack);
		return 0;
	}

	int TestConnect() {
		CPacket pack(22233, NULL, 0);
		CServerSocket::getInstance()->Send(pack);
		return 0;
	}

	int deleteLocalFile() {
		std::string strPath;
		CServerSocket::getInstance()->getFilePath(strPath);
		TCHAR sPath[MAX_PATH] = _T("");
		//mbstowcs(sPath,strPath.c_str(),strPath.size());
		MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), strPath.size(), sPath, sizeof(sPath) / sizeof(TCHAR));
		BOOL bRet = DeleteFileA(strPath.c_str());
		CPacket pack(9, NULL, 0);
		BOOL ret = CServerSocket::getInstance()->Send(pack);
		TRACE("DeleteFile ret = %d\r\n", ret);
		return 0;
	}
};

