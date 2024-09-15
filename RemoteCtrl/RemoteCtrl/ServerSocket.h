#pragma once

#include "pch.h"
#include "framework.h"

/* �������ݵ� ����֡ */
#pragma pack(push)
#pragma pack(1)

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


class CPacket {
public:
	/*����*/
	CPacket();
	CPacket(const CPacket& packet);
	CPacket(const BYTE* pData,size_t& nSize);
	CPacket(WORD nCmd,const BYTE* pData,size_t nSize);

	~CPacket();
	CPacket& operator=(const CPacket& pack);

	int size();
	const char* Data();

	/*�� �� ��*/
	WORD sHead;			//�̶�λFEFF
	DWORD nLength;		//�����ȣ��������� �� У�� ����
	WORD sCmd;			//��������
	std::string strData;//������
	WORD sSum;			//��У��
	std::string strOut; //��������
};
#pragma pack(pop)

typedef struct MouseEvent 
{
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}

	WORD nAction;	//��� �ƶ� ˫��
	WORD nButton;	//����Ҽ��м�
	POINT ptXY;		//����
}MOUSEEV,*PMOUSEEV;


class CServerSocket {
public:
	static CServerSocket* getInstance();
	BOOL initSocket();
	BOOL AcceptClient();
	int DealCommand();
	BOOL Send(const char* pData, int nSize);
	BOOL Send(CPacket& pack);
	BOOL getFilePath(std::string& strPath);
	BOOL getMouseEvent(MOUSEEV& mouse);
	CPacket& getPacket();
	void closeClient();
private:
	SOCKET m_serv_socket;
	SOCKET m_client;
	CPacket m_packet;
	//
	CServerSocket();
	CServerSocket(const CServerSocket& ss);
	~CServerSocket();
	BOOL InitSockEnv();
	CServerSocket& operator=(const CServerSocket& ss) {}
	static CServerSocket* m_instance;
	static void releaseInstance();
	class CHelper {
	public:
		CHelper();
		~CHelper();
	};
	static CHelper m_helper;
};