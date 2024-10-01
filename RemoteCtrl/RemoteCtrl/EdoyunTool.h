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

		// FormatMessage ��ϵͳ�������ת��Ϊ�ɶ��Ĵ�����Ϣ
		DWORD ret = FormatMessage(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwError,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&lpMessageBuf, 0, NULL);

		if (ret == 0) {
			// ��� FormatMessage ʧ��
			DWORD formatError = GetLastError();
			TCHAR errMsg[256];
			_stprintf_s(errMsg, _T("FormatMessage failed with error code: %lu"), formatError);
			OutputDebugString(errMsg);
			MessageBox(NULL, errMsg, _T("�����ʹ���"), MB_ICONERROR);
		}
		else {
			// ���������Ϣ�����Դ��ں� MessageBox
			OutputDebugString(lpMessageBuf);
			MessageBox(NULL, lpMessageBuf, _T("�����ʹ���"), MB_ICONERROR);
		}

		// �ͷŷ���Ļ������ڴ�
		if (lpMessageBuf) {
			LocalFree(lpMessageBuf);
		}
	}
	
	/// <summary>
	/// �ж��Ƿ����Ա�������
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
	/// // ���ز����� ��������Ա�û� ��ֹ������ֻ�ܵ�½���ؿ���̨
	/// </summary>
	/// <returns></returns>
	static BOOL RunAsAdmin() {
		// ���ز����� ��������Ա�û� ��ֹ������ֻ�ܵ�½���ؿ���̨
		STARTUPINFO si = { sizeof(si) };  // ��ʼ����ָ���ṹ���С
		PROCESS_INFORMATION pi = { 0 };
		TCHAR sPath[MAX_PATH] = _T("");

		// ���ģ���ļ����Ƿ�ɹ���ȡ
		if (GetModuleFileName(NULL, sPath, MAX_PATH) == 0) {
			MessageBox(NULL, _T("��ȡģ���ļ���ʧ��"), _T("�������"), MB_ICONERROR);
			return FALSE;
		}

		// ��������
		BOOL ret = CreateProcessWithLogonW(
			_T("Administrator"), NULL, NULL,
			LOGON_WITH_PROFILE, NULL, sPath,
			CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi
		);

		// ������
		if (!ret) {
			DWORD dwError = GetLastError();  // ��ȡ�������
			ShowError();  // �����Զ���Ĵ�������
			TCHAR errorMsg[256];
			_stprintf_s(errorMsg, _T("��������ʧ�ܣ��������: %lu"), dwError);
			MessageBox(NULL, errorMsg, _T("�������"), MB_ICONERROR);
			return FALSE;
		}

		// �ȴ��������
		WaitForSingleObject(pi.hProcess, INFINITE);

		// �رվ���Ա�����Դй¶
		if (pi.hProcess) {
			CloseHandle(pi.hProcess);
		}
		if (pi.hThread) {
			CloseHandle(pi.hThread);
		}
		return true;
	}

	/// <summary>
	/// ���ļ��ŵ�ϵͳ�Ŀ�������Ŀ¼��
	/// </summary>
	/// <param name="strPath"></param>
	/// <returns></returns>
	static bool WriteStartupDir(const CString& strPath) {
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);
		return CopyFile(sPath, strPath, FALSE);
	}

	/// <summary>
	/// �޸�ע���ʵ�ֿ�������
	/// </summary>
	/// <param name="strPath"></param>
	/// <returns></returns>
	static bool WriteRegisterTable(const CString& strPath) {
		CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);

		// �����ļ���ָ��·��
		BOOL bCopySuccess = CopyFile(sPath, strPath, FALSE);
		if (bCopySuccess == FALSE) {
			MessageBox(NULL, _T("�����ļ�ʧ�ܣ��Ƿ�Ȩ�޲���?\r\n"), _T("����"), MB_ICONERROR | MB_TOPMOST);
			return false; // ��ֱ���˳����������������
		}

		// ��ע����
		HKEY hKey = NULL;
		LONG lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_32KEY, &hKey);
		if (lResult != ERROR_SUCCESS) {
			MessageBox(NULL, _T("��ע���ʧ��! �Ƿ�Ȩ�޲���?\r\n��������ʧ��!"), _T("����"), MB_ICONERROR | MB_TOPMOST);
			return false; // ��ֱ���˳�
		}

		// ����������
		lResult = RegSetValueEx(hKey, _T("RemoteCtrl"), 0, REG_EXPAND_SZ,
			(BYTE*)(LPCTSTR)strPath, (strPath.GetLength() + 1) * sizeof(TCHAR)); // ȷ������ null ��ֹ��
		if (lResult != ERROR_SUCCESS) {
			MessageBox(NULL, _T("���ÿ�������ʧ��! �Ƿ�Ȩ�޲���?\r\n��������ʧ��!"), _T("����"), MB_ICONERROR | MB_TOPMOST);
			RegCloseKey(hKey);
			return false; // ��ֱ���˳�
		}

		// �ر�ע����
		RegCloseKey(hKey);
		return true;
	}

	/// <summary>
	/// ����MFC�����ʼ��
	/// </summary>
	/// <returns></returns>
	static bool Init() {
		HMODULE hModule = ::GetModuleHandle(nullptr);
		if (hModule == nullptr) {
			wprintf(L"����: GetModuleHandle ʧ��\n");
			return false;
		}
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0)) {
			// TODO: �ڴ˴�ΪӦ�ó������Ϊ��д���롣
			wprintf(L"����: MFC ��ʼ��ʧ��\n");
			return false;
		}
		return true;
	}
};

