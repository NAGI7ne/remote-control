#include "pch.h"
#include "Command.h"

CCommand::CCommand() : threadid(0)
{
	struct {
		int nCmd;
		CMDFUNC func;
	}data[] = {
		{1,&CCommand::MakeDriverInfo},
		{2,&CCommand::MakeDirectoryInfo},
		{3,&CCommand::RunFile},
		{4,&CCommand::DownloadFile},
		{5,&CCommand::MouseEvent},
		{6,&CCommand::SendScreen},
		{7,&CCommand::LockMachine},
		{8,&CCommand::UnlockMachine},
		{9,&CCommand::DeleteLocalFile},
		{39,&CCommand::TestConnect},
		{-1,NULL},
	};
	for (int i = 0; data[i].nCmd != -1; i++) {
		mMapFunction.insert(std::pair<int, CMDFUNC>(data[i].nCmd, data[i].func));
	}
}

int CCommand::ExcuteCommand(int nCmd, std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	std::map<int, CMDFUNC>::iterator it = mMapFunction.find(nCmd);
	if (it == mMapFunction.end()) {
		return - 1;
	}
	//进入函数指针指向的函数完成业务，将对应的包压入包队列中
	return (this->*it->second)(lstPacket, inPacket);
}
