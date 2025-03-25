#pragma once

#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>
#include <list>
#include <map>
#include <mutex>

#define WM_SEND_PACK (WM_USER + 1)  //发送数据包
#define WM_SEND_PACK_ACK (WM_USER + 2) //发送包数据应答

#pragma pack(push) 
#pragma pack(1)
class CPacket {
public:
	CPacket() : sHead(0), nLength(0), sCmd(0), sSum(0) {}
	CPacket(const CPacket& pack) {
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {  //封装包
		sHead = 0xFEFF;
		nLength = nSize + 2 + 2;
		sCmd = nCmd;
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else {
			strData.clear();
		}
		sSum = 0;
		for (size_t j = 0; j < nSize; j++) {
			sSum += pData[j] & 0xFF;
		}
	}
	CPacket(const BYTE* pData, size_t& nSize)
	{    //解包
		size_t i = 0;
		for (i; i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFF) {   //包头存在 
				sHead = *(WORD*)(pData + i);
				i += 2;    //跳过包头
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize)   //+4数据段 +2控制命令 +2和校验
		{							//检查是否接收的数据不全
			nSize = 0;
			return;
		}
		nLength = *(DWORD*)(pData + i); i += 4; //DWORD为4个字节，强转后取+i映射后的4个字节，
		//取值完成后再跳过长度段
		if (nLength + i > nSize) // +i 总的是完整包的长度
		{							//检查包是否完全收到
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i); i += 2;
		if (nLength > 4) {
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), (pData + i), nLength - 4);
			i += nLength - 4;
		}
		sSum = *(WORD*)(pData + i); i += 2;
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++) {
			sum += BYTE(strData[j]) & 0xFF;  //转为字节
		}
		if (sum == sSum) {
			nSize = i;
			return;
		}
		nSize = 0;
		return;
	}
	CPacket& operator=(const CPacket& pack) {
		if (this != &pack) {
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		return *this;
	}
	int Size()  {
		return nLength + 6;
	}
	const char* Data(std::string& strOut) const{    //返回完整数据
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)pData = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}
	~CPacket() {}
public:
	WORD sHead; //包头 0xFEFF
	DWORD nLength; //包长度 （控制命令到和校验）
	WORD sCmd;  //控制命令
	std::string strData;   //包数据 
	WORD sSum; //和校验(校验包数据的和)
};
#pragma pack(pop)

typedef struct fileInfo {
	fileInfo() {
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid; //是否无效
	BOOL IsDirectory; //是否为目录 
	BOOL HasNext; //是否还有后续
	char szFileName[256];  //文件名
}FILEINFO, * PFILEINFO;

enum {   //TODO
	CSM_AUTOCLOSE = 1, //CSM = Client Socket Mode 自动关闭
};

typedef struct PacketData {
	std::string strData;
	UINT nMode;
	PacketData(const char* pData, size_t nLen, UINT mode) {
		strData.resize(nLen);
		memcpy((char*)strData.c_str(), pData, nLen);
		nMode = mode;
	}
	PacketData(const PacketData& data) {
		strData = data.strData;
		nMode = data.nMode;
	}
	PacketData& operator=(const PacketData& data) {
		if (this != &data) {
			strData = data.strData;
			nMode = data.nMode;
		}
		return *this;
	}
}PACKET_DATA;

typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;  //点击， 移动， 双击
	WORD nButton;  //左键， 右键， 中键
	POINT ptXY;   //坐标
}MOUSEEV, * PMOUSEEV;

std::string GetErrorInfo(int wsaErrCode);
//采用单例设计模式
class CClientSocket
{
public:
	//确保外部能够访问
	//静态函数没有this指针，无法直接访问成员变量
	static CClientSocket* getInstance() {
		if (!mInstance) mInstance = new CClientSocket();
		return mInstance;
	}
	bool InitSocket();
#define BUFFER_SIZE 2048000
	int DealCommand() {
		if (mSock == -1) return false;
		char* buffer = mBuffer.data();  //多线程发送命令可能会出现冲突
		//memset(buffer, 0, BUFFER_SIZE);

		// index记录当前缓冲区中累计的有效数据长度
		//确保未处理的数据保留在缓冲区的前部，为后续数据接收做好准备
		static size_t index = 0;    
		while (1) {
			size_t len = recv(mSock, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0 && (index <= 0)) return -1;  //如果没接收到数据并且缓冲区没有数据
			TRACE("rev len = %d(0x%08X) index = %d(0x%08X)\r\n", len, len, index, index);
			index += len;
			len = index;
			TRACE("rev len = %d(0x%08X) index = %d(0x%08X)\r\n", len, len, index, index);
			mPacket = CPacket((BYTE*)buffer, len);
			//TRACE("客户端解包成功 cmd: %d\r\n", mPacket.sCmd);
			if (len > 0) {
				memcpy(buffer, buffer + len, index - len);  //这里是index-len，易错
				index -= len;
				return mPacket.sCmd;
			}
		}
		return -1;
	}
	//bool SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClosed = true);
	bool SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed = true);

	bool GetFilePath(std::string& strPath)const {
		if (mPacket.sCmd >= 2 && mPacket.sCmd <= 4) {
			strPath = mPacket.strData;
			return true;
		}
		return false;
	}
	bool GetMouseEvent(MOUSEEV& mouse) {
		if (mPacket.sCmd == 5) {
			memcpy(&mouse, mPacket.strData.c_str(), mPacket.strData.size());
			return true;
		}
		return false;
	}
	CPacket& GetPacket() {
		return mPacket;
	}
	void CloseSocket() {
		closesocket(mSock);
		mSock = INVALID_SOCKET;
	}
	void UpdataAddress(int nIP, int nPort) {
		if ((mnIP != nIP) || (mnPort != nPort)) {
			mnIP = nIP;
			mnPort = nPort;
		}
	}
private:
	UINT mhThreadID;
	typedef void(CClientSocket::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	std::map<UINT, MSGFUNC> mMapFunc;
	HANDLE mhThread;
	bool mbAutoClose;
	std::mutex mLock;
	std::list<CPacket> mlstSend;
	////存储对应命令的一系列包
	//在收包不确定的情况下，效率比vector高
	std::map<HANDLE, std::list<CPacket>&> mMapAck; 
	std::map<HANDLE, bool>mMapAutoClosed;
	int mnIP;
	int mnPort;
	std::vector<char> mBuffer;
	SOCKET mSock;
	CPacket mPacket;
	// 禁用拷贝构造
	CClientSocket& operator=(const CClientSocket& ss) {}
	CClientSocket(const CClientSocket& ss);
	CClientSocket();
	~CClientSocket() {
		closesocket(mSock);
		mSock = INVALID_SOCKET;
		WSACleanup();
	}
	static unsigned __stdcall threadEntry(void* arg);
	//void threadFunc();
	void threadFunc2();
	BOOL InitSockEnv() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
			return FALSE;
		}//指定版本
		return TRUE;
	}
	//这里mInstance是静态变量，release需要操作这个变量所以是静态的函数
	static void releaseInstance() {
		if (mInstance != NULL) {
			CClientSocket* tmp = mInstance;
			mInstance = NULL;
			//delete时调用CClientSocket的析构函数
			delete tmp;
		}
	}
	bool Send(const char* pData, int nSize) {
		if (mSock == -1) return false;
		return send(mSock, pData, nSize, 0) > 0;
	}
	bool Send(const CPacket& pack);
	void SendPack(UINT nmsg, WPARAM wParam/*缓冲区的值*/, LPARAM lParam/*缓冲区的长度*/);

	//在main开始前初始化实例
	static CClientSocket* mInstance;
	//防止类没有被析构，原因mInstance没有delete，加一个类来间接删除mInstance
	//CHelper 的析构函数充当了一个监控者，它在对象销毁时主动清理单例
	class CHelper {
	public:
		CHelper() {
			CClientSocket::getInstance();
		}
		~CHelper() {
			CClientSocket::releaseInstance();
		}
	};
	//加static实现程序启动前自动构造、程序终止时自动析构
	static CHelper mHelper;
};