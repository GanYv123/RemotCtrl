#pragma once
class CEdoyunTool {
public:
	static void Dump(BYTE* pData, size_t nSize) {
		std::string strOut;
		for (size_t i = 0; i < nSize; i++) {
			char buf[8] = "";
			if (i > 0 && (i % 16 == 0)) strOut += "\n ";
			snprintf(buf, sizeof(buf), "%02X", pData[i] & 0xFF);
			strOut += buf;
		}
		strOut += "\n";
		OutputDebugStringA(strOut.c_str());
	}

	static void ShowError() {
		LPWSTR lpMessageBuf = NULL;
		DWORD dwError = GetLastError();

		// FormatMessage 将系统错误代码转换为可读的错误消息
		DWORD ret = FormatMessage(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwError,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&lpMessageBuf, 0, NULL);

		if (ret == 0) {
			// 如果 FormatMessage 失败
			DWORD formatError = GetLastError();
			TCHAR errMsg[256];
			_stprintf_s(errMsg, _T("FormatMessage failed with error code: %lu"), formatError);
			OutputDebugString(errMsg);
			MessageBox(NULL, errMsg, _T("程序发送错误"), MB_ICONERROR);
		}
		else {
			// 输出错误消息到调试窗口和 MessageBox
			OutputDebugString(lpMessageBuf);
			MessageBox(NULL, lpMessageBuf, _T("程序发送错误"), MB_ICONERROR);
		}

		// 释放分配的缓冲区内存
		if (lpMessageBuf) {
			LocalFree(lpMessageBuf);
		}
	}
	
	/// <summary>
	/// 判断是否管理员身份运行
	/// </summary>
	/// <returns></returns>
	static bool IsAdmin() {
		HANDLE hToken = NULL;
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
			ShowError();
			return false;
		}
		TOKEN_ELEVATION eve;
		DWORD len = 0;
		if (!GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len)) {
			ShowError();
			return false;
		}
		CloseHandle(hToken);
		if (len == sizeof(eve)) {
			return eve.TokenIsElevated;
		}
		printf("length of tokenInformation is %d\r\n", len);
		return false;
	}

	/// <summary>
	/// // 本地策略组 开启管理员用户 禁止空密码只能登陆本地控制台
	/// </summary>
	/// <returns></returns>
	static BOOL RunAsAdmin() {
		// 本地策略组 开启管理员用户 禁止空密码只能登陆本地控制台
		STARTUPINFO si = { sizeof(si) };  // 初始化并指定结构体大小
		PROCESS_INFORMATION pi = { 0 };
		TCHAR sPath[MAX_PATH] = _T("");

		// 检查模块文件名是否成功获取
		if (GetModuleFileName(NULL, sPath, MAX_PATH) == 0) {
			MessageBox(NULL, _T("获取模块文件名失败"), _T("程序错误"), MB_ICONERROR);
			return FALSE;
		}

		// 创建进程
		BOOL ret = CreateProcessWithLogonW(
			_T("Administrator"), NULL, NULL,
			LOGON_WITH_PROFILE, NULL, sPath,
			CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi
		);

		// 错误处理
		if (!ret) {
			DWORD dwError = GetLastError();  // 获取错误代码
			ShowError();  // 调用自定义的错误处理函数
			TCHAR errorMsg[256];
			_stprintf_s(errorMsg, _T("创建进程失败，错误代码: %lu"), dwError);
			MessageBox(NULL, errorMsg, _T("程序错误"), MB_ICONERROR);
			return FALSE;
		}

		// 等待进程完成
		WaitForSingleObject(pi.hProcess, INFINITE);

		// 关闭句柄以避免资源泄露
		if (pi.hProcess) {
			CloseHandle(pi.hProcess);
		}
		if (pi.hThread) {
			CloseHandle(pi.hThread);
		}
		return true;
	}

	/// <summary>
	/// 将文件放到系统的开启自启目录中
	/// </summary>
	/// <param name="strPath"></param>
	/// <returns></returns>
	static bool WriteStartupDir(const CString& strPath) {
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);
		return CopyFile(sPath, strPath, FALSE);
	}

	/// <summary>
	/// 修改注册表实现开机自启
	/// </summary>
	/// <param name="strPath"></param>
	/// <returns></returns>
	static bool WriteRegisterTable(const CString& strPath) {
		CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);

		// 复制文件到指定路径
		BOOL bCopySuccess = CopyFile(sPath, strPath, FALSE);
		if (bCopySuccess == FALSE) {
			MessageBox(NULL, _T("复制文件失败，是否权限不足?\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
			return false; // 不直接退出，允许后续错误处理
		}

		// 打开注册表键
		HKEY hKey = NULL;
		LONG lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_32KEY, &hKey);
		if (lResult != ERROR_SUCCESS) {
			MessageBox(NULL, _T("打开注册表失败! 是否权限不足?\r\n程序启动失败!"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
			return false; // 不直接退出
		}

		// 设置启动项
		lResult = RegSetValueEx(hKey, _T("RemoteCtrl"), 0, REG_EXPAND_SZ,
			(BYTE*)(LPCTSTR)strPath, (strPath.GetLength() + 1) * sizeof(TCHAR)); // 确保包括 null 终止符
		if (lResult != ERROR_SUCCESS) {
			MessageBox(NULL, _T("设置开机启动失败! 是否权限不足?\r\n程序启动失败!"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
			RegCloseKey(hKey);
			return false; // 不直接退出
		}

		// 关闭注册表键
		RegCloseKey(hKey);
		return true;
	}

	/// <summary>
	/// 用于MFC程序初始化
	/// </summary>
	/// <returns></returns>
	static bool Init() {
		HMODULE hModule = ::GetModuleHandle(nullptr);
		if (hModule == nullptr) {
			wprintf(L"错误: GetModuleHandle 失败\n");
			return false;
		}
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0)) {
			// TODO: 在此处为应用程序的行为编写代码。
			wprintf(L"错误: MFC 初始化失败\n");
			return false;
		}
		return true;
	}
};

