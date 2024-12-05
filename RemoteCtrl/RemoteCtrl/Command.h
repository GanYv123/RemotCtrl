#pragma once
#include "resource.h"
#include <map>
#include <atlimage.h>
#include <direct.h>
#include "EdoyunTool.h"
#include "Packet.h"
#include <io.h>
#include <list>
#include "LockInfoDialog.h"

class CCommand {
public:
	CCommand();
	~CCommand() {}
	int ExcuteCommand(int nCmd, std::list<CPacket>& lstPacket, CPacket& inPacket);

	static void RunCommand(void* arg, int status, std::list<CPacket>& lstPacket, CPacket& inPacket) {
		CCommand* thiz = (CCommand*)arg;
		int ret = thiz->ExcuteCommand(status, lstPacket, inPacket);
		if (status > 0) {
			if (ret != 0) {
				TRACE("执行命令失败,%d ret = %d\r\n", status, ret);
			}
		}
		else {
			MessageBox(NULL, _T("无法正常接入用户，自动重试"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
			exit(0);
		}
	}
protected:
	typedef int (CCommand::* CMDFUNC)(std::list<CPacket>& lstPacket, CPacket& inPacket);//成员函数指针
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

	/**
	 * 锁机 函数
	 */
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

	int MakeDriverInfo(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		// 初始化用于存储结果的字符串
		std::string result;
		// 循环遍历字母驱动器 A 到 Z，对应 ASCII 值 65 到 90
		for(int i = 1; i <= 26; i++){ // 改变当前驱动器
			// 使用 _chdrive 函数尝试切换到驱动器 i
			// _chdrive(i) 返回值为 0 表示驱动器有效
			if(_chdrive(i) == 0){
				// 如果驱动器有效，将对应字母追加到结果字符串
				result += 'A' + i - 1; // 例如，i = 3 时，'A' + 2 = 'C'
				// 如果结果字符串已非空（至少包含一个驱动器）
				// 则添加逗号作为分隔符
				if(result.size()){
					result += ','; // 示例结果格式：C,D,F,
				}
			}
		}
		// 将生成的驱动器信息字符串存储到 CPacket 中
		// 参数说明：
		// 1 - 包的命令号
		// (BYTE*)result.c_str() - 驱动器信息数据的指针
		// result.size() - 数据的长度
		lstPacket.push_back(CPacket(1, (BYTE*)result.c_str(), result.size()));
		// 返回值 0 表示操作成功
		return 0;
	}


	int MakeDirectoryInfo(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		//std::list<FILEINFO> lstFileInfos;
		std::string strPath = inPacket.strData;
		//TRACE("MakeDirectoryInfo()->strPath:%s\n", strPath.c_str());
		if (_chdir(strPath.c_str()) != 0) { //切换目录失败
			FILEINFO finfo;
			finfo.HasNext = FALSE;
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
			OutputDebugString(_T("没有权限访问目录!\r\n"));
			return -2;
		}

		_finddata_t fdata;
		int hfind = 0;

		if ((hfind = _findfirst("*", &fdata)) == -1) {
			OutputDebugString(_T("没有找到任何文件!"));
			FILEINFO finfo;
			finfo.HasNext = FALSE;
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));

			return -3;
		}

		int SEND_FILE_CNT{ 0 };
		do {
			FILEINFO finfo;
			finfo.IsDirectory = ((fdata.attrib & _A_SUBDIR) != 0);
			memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
			TRACE("%s\r\n", finfo.szFileName);
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
			SEND_FILE_CNT++;

		} while (!_findnext(hfind, &fdata));

		TRACE("SEND_FILE_CNT = %d\r\n\n", SEND_FILE_CNT);

		FILEINFO finfo;
		finfo.HasNext = FALSE;
		lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));

		return 0;
	}

	int RunFile(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		std::string strPath = inPacket.strData;
		ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
		lstPacket.push_back(CPacket(3, NULL, 0));
		return 0;
	}

	int DownloadFile(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		std::string strPath = inPacket.strData;
		FILE* pFile = nullptr;
		errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
		long long data = 0;

		if (err != 0) {
			lstPacket.push_back(CPacket(4, (BYTE*)&data, sizeof(long long)));
			return -1;
		}

		if (pFile != nullptr) {
			/*获取文件大小*/
			fseek(pFile, 0, SEEK_END);
			data = _ftelli64(pFile);
			lstPacket.push_back(CPacket(4, (BYTE*)&data, sizeof(long long)));
			fseek(pFile, 0, SEEK_SET);
			char buffer[1024] = "";
			size_t rlen = 0;
			do {
				rlen = fread(buffer, 1, 1024, pFile);
				lstPacket.push_back(CPacket(4, (BYTE*)buffer, rlen));
			} while (rlen >= 1024);
			fclose(pFile);
		}
		else {
			lstPacket.push_back(CPacket(4, NULL, 0));
		}
		return 0;
	}

	int MouseEvent(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		MOUSEEV mouse;
		memcpy(&mouse, inPacket.strData.c_str(), sizeof(MOUSEEV));
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
		lstPacket.push_back(CPacket(5, NULL, 0));
		return 0;
	}

	int SendScreen(std::list<CPacket>& lstPacket, CPacket& inPacket) {
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
			lstPacket.push_back(CPacket(6, (BYTE*)pData, nSize));

			GlobalUnlock(hMem);
		}
		pStream->Release();
		GlobalFree(hMem);
		screen.ReleaseDC();
		return 0;
	}

	int LockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE)) {
			//_beginthread(threadLockDlg, 0, NULL);
			_beginthreadex(NULL, 0, &CCommand::threadLockDlg, this, 0, &threadId);
			TRACE("threadId = %d\r\n", threadId);
		}
		lstPacket.push_back(CPacket(7, NULL, 0));
		return 0;
	}

	int UnlockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		//dlg.SendMessage(WM_KEYDOWN, 0x41, 0X01E0001);
		//::SendMessage(dlg.m_hWnd,WM_KEYDOWN,0X41, 0X01E0001);//Windows消息泵只能发同一线程
		::PostThreadMessage(threadId, WM_KEYDOWN, 0x41, 0);
		lstPacket.push_back(CPacket(8, NULL, 0));
		return 0;
	}

	int TestConnect(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		lstPacket.push_back(CPacket(22233, NULL, 0));
		return 0;
	}

	int deleteLocalFile(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		std::string strPath = inPacket.strData;
		TCHAR sPath[MAX_PATH] = _T("");
		//mbstowcs(sPath,strPath.c_str(),strPath.size());
		MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), strPath.size(), sPath, sizeof(sPath) / sizeof(TCHAR));
		BOOL bRet = DeleteFileA(strPath.c_str());
		lstPacket.push_back(CPacket(9, NULL, 0));
		return 0;
	}
};

