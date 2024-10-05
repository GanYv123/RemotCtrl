#include "pch.h"
#include "CFunction.h"
CFunction* CFunction::m_instance = nullptr;
CFunction::Helper CFunction::helper;

CFunction::CFunction() :threadid(0) {
	struct {
		int nCmd;
		CMDFUNC func;
	}data[] = {
		{1,&CFunction::MakeDriverInfo},
		{2,&CFunction::MakeDirectoryInfo},
		{3,&CFunction::Runfile},
		{4,&CFunction::DownloadFile},
		{5,&CFunction::MouseEvent},
		{6,&CFunction::SendScreen},
		{7,&CFunction::LockMachine},
		{8,&CFunction::UnloakMachine},
		{9,&CFunction::DeleteLocalFile},
		{1981,&CFunction::TestConnect},
		{-1,NULL}
	};
	for (int i = 0; data[i].nCmd != -1; i++) {
		m_mapFunction.insert({ data[i].nCmd ,data[i].func });
	}
}

int CFunction::ExcuteCommand(int nCmd, std::list<CPacket>& listPacket, CPacket& inPacket) {
	std::map<int, CMDFUNC>::iterator it = m_mapFunction.find(nCmd);
	if (it == m_mapFunction.end()) {
		return -1;
	}
	return (this->*it->second)(listPacket, inPacket);
	//return (this->*m_mapFunction[nCmd])();
}