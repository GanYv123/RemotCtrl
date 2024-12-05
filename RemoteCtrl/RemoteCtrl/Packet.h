#pragma once
#include "pch.h"
#include "framework.h"
/* �������ݵ� ����֡ */
#pragma pack(push)
#pragma pack(1)
class CPacket {
public:

	/**
	 * ���캯��
	 * ��ʼ������Ա����
	 */
	CPacket() : sHead(0), nLength(0), sCmd(0), sSum(0) {}
	//���ƹ��캯��
	CPacket(const CPacket& pack) {
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}

	/**
	 * @argument:
	 * nSize:���ݵ��ֽڴ�С
	 */
	CPacket(const BYTE* pData, size_t& nSize) :sHead(0), nLength(0), sCmd(0), sSum(0) {
		size_t i = 0;
		for (; i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFF) { //У���ͷ
				sHead = *(WORD*)(pData + i);
				i += 2;//һ��WORD 2byte
				break;
			}
		}
		if (i + 8 > nSize) { //�������ɹ� length + cmd +sum
			nSize = 0;
			return;/*��С�������İ�*/
		}

		nLength = *(DWORD*)(pData + i);
		i += 4;
		/*- - -*/
		//�˴� i = length ֮ǰ���ֽ���
		if (nLength + i > nSize) { //��û��ȫ���յ����в�ȱ
			nSize = 0;
			return;
		}

		sCmd = *(WORD*)(pData + i);
		i += 2;
		/*- - -*/
		if (nLength > 4) {
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			i += nLength - 4;
		}
		/*- - -*/
		sSum = *(WORD*)(pData + i);
		i += 2;
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++) {
			sum += (BYTE(strData[j]) & 0xFF);
		}
		if (sum == sSum) {//�����ɹ�
			nSize = i;
			return;
			//FFFE 0900 0000 0100 432C442C46 2501
		}
		nSize = 0;

	}

	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
		sHead = 0xFEFF;
		nLength = nSize + 4;//���ݳ��� = cmd + У��
		sCmd = nCmd;
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else {
			strData.clear();
		}
		sSum = 0;
		for (size_t j = 0; j < strData.size(); j++) {
			sSum += (BYTE(strData[j]) & 0xFF);
		}
	}

	~CPacket() {
	}

	CPacket& operator=(const CPacket& pack) {
		if (this == &pack) return *this;
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
		return *this;
	}

	int size() {
		return nLength + 6;
	}
	const char* Data() {
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead;
		pData += 2;
		*(DWORD*)pData = nLength;
		pData += 4;
		*(WORD*)pData = sCmd;
		pData += 2;
		memcpy(pData, strData.c_str(), strData.size());
		pData += strData.size();
		*(WORD*)pData = sSum;

		return strOut.c_str();
	}

	/*�� �� ��*/
	WORD sHead;			//�̶�λFEFF
	DWORD nLength;		//�����ȣ��������� �� У�� ����
	WORD sCmd;			//��������
	std::string strData;//������
	WORD sSum;			//��У��
	std::string strOut; //��������
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
