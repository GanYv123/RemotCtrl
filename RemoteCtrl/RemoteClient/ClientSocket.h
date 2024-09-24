#pragma once

#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>
#include <map>
#include <list>
#include <mutex>

/* �������ݵ� ����֡ */
#pragma pack(push)
#pragma pack(1)
class CPacket {
public:
	/*����*/
	CPacket();
	CPacket(const CPacket& packet);
	CPacket(const BYTE* pData, size_t& nSize);
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize,HANDLE hEvent);

	~CPacket();
	CPacket& operator=(const CPacket& pack);

	int size();
	const char* Data(std::string& strOut)const;

	/*�� �� ��*/
	WORD sHead;			//�̶�λFEFF
	DWORD nLength;		//�����ȣ��������� �� У�� ����
	WORD sCmd;			//��������
	std::string strData;//������
	WORD sSum;			//��У��
	HANDLE hEvent;		
};
#pragma pack(pop)

typedef struct file_info {
	file_info() {
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;  //�Ƿ�ΪʧЧ�ļ�
	BOOL IsDirectory;//�Ƿ�ΪĿ¼
	BOOL HasNext;	 //�Ƿ��к����ļ�
	char szFileName[256];

}FILEINFO, * PFILEINFO;

typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}

	WORD nAction;	//��� �ƶ� ˫��
	WORD nButton;	//����Ҽ��м�
	POINT ptXY;		//����
}MOUSEEV, * PMOUSEEV;


class CClientSocket {
public:
	static CClientSocket* getInstance();
	BOOL initSocket();
	void Dump(BYTE* pData, size_t nSize);
	int DealCommand();
	/// <summary>
	/// �������ݰ��������ݰ����뷢�Ͷ��У�
	/// �����յ������ݴ��� ������
	/// </summary>
	/// <param name="pack">Ҫ���͵İ�</param>
	/// <param name="lstPacks">Ӧ���</param>
	/// <returns>�Ƿ��ͳɹ�</returns>
	BOOL SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks,BOOL isAutoClosed = TRUE);
	BOOL getFilePath(std::string& strPath);
	BOOL getMouseEvent(MOUSEEV& mouse);
	CPacket& getPacket();
	void CloseSocket();
	void UpdateAddress(int nIP,int nPort) {
		if ((m_nIP != nIP) || (m_nPort != nPort)) {
			m_nIP = nIP;
			m_nPort = nPort;
		}
	}
private:
	HANDLE m_hThread;
	BOOL m_bAutoClose;
	std::mutex m_lock;
	std::list<CPacket> m_lstSend;
	std::map<HANDLE, std::list<CPacket>&> m_mapAck; 
	std::map<HANDLE, BOOL> m_mapAutoClosed;
	int m_nIP;//��ַ
	int m_nPort;//�˿�
	SOCKET m_sock;
	CPacket m_packet;
	std::vector<char> m_buffer;
	//
	CClientSocket();
	CClientSocket(const CClientSocket&);
	~CClientSocket();
	/// <summary>
	/// ��������ж��̷߳���ͬ��
	/// </summary>
	/// <param name="arg">����thisָ�����threadFunc�����߳�</param>
	static void threadEntry(void* arg);
	//��Ϊ�����ӣ�ֻ��ʼ��һ��
	void threadFunc();
	BOOL InitSockEnv();
	CClientSocket& operator=(const CClientSocket&ss) {}
	BOOL Send(const char* pData, int nSize);
	BOOL Send(const CPacket& pack);
	static CClientSocket* m_instance;
	static void releaseInstance();
	class CHelper {
	public:
		CHelper();
		~CHelper();
	};
	static CHelper m_helper;
};

