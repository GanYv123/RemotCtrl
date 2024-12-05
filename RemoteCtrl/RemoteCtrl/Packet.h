#pragma once
#include "pch.h"
#include "framework.h"
/* 用于数据的 包、帧 */
#pragma pack(push)
#pragma pack(1)
class CPacket {
public:

	/**
	 * 构造函数
	 * 初始化包成员变量
	 */
	CPacket() : sHead(0), nLength(0), sCmd(0), sSum(0) {}
	//复制构造函数
	CPacket(const CPacket& pack) {
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}

	/**
	 * @argument:
	 * nSize:数据的字节大小
	 */
	CPacket(const BYTE* pData, size_t& nSize) :sHead(0), nLength(0), sCmd(0), sSum(0) {
		size_t i = 0;
		for (; i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFF) { //校验包头
				sHead = *(WORD*)(pData + i);
				i += 2;//一个WORD 2byte
				break;
			}
		}
		if (i + 8 > nSize) { //解析不成功 length + cmd +sum
			nSize = 0;
			return;/*大小不完整的包*/
		}

		nLength = *(DWORD*)(pData + i);
		i += 4;
		/*- - -*/
		//此处 i = length 之前的字节数
		if (nLength + i > nSize) { //包没完全接收到，有残缺
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
		if (sum == sSum) {//解析成功
			nSize = i;
			return;
			//FFFE 0900 0000 0100 432C442C46 2501
		}
		nSize = 0;

	}

	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
		sHead = 0xFEFF;
		nLength = nSize + 4;//数据长度 = cmd + 校验
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

	/*包 变 量*/
	WORD sHead;			//固定位FEFF
	DWORD nLength;		//包长度：控制命令 到 校验 结束
	WORD sCmd;			//控制命令
	std::string strData;//包数据
	WORD sSum;			//和校验
	std::string strOut; //整包数据
};
#pragma pack(pop)



typedef struct file_info {
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

}FILEINFO, * PFILEINFO;


typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}

	WORD nAction;	//点击 移动 双击
	WORD nButton;	//左键右键中键
	POINT ptXY;		//坐标
}MOUSEEV, * PMOUSEEV;
