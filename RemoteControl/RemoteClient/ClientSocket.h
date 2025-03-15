#pragma once

#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>

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
	CPacket(const BYTE* pData, size_t& nSize) {    //解包
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
	size_t Size()  {
		return nLength + 6;
	}
	const char* Data() {    //返回完整数据
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
	std::string strOut; //整个包的数据
};
#pragma pack(pop)

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
		if (!mInstance) mInstance = new CClientSocket;
		return mInstance;
	}
	bool InitSocket(int nIP, int nPort) {
		if (mSock != INVALID_SOCKET) CloseSocket();
		mSock = socket(PF_INET, SOCK_STREAM, 0);
		if (mSock == -1) return false;
		sockaddr_in servAddr;
		memset(&servAddr, 0, sizeof(servAddr));
		servAddr.sin_family = AF_INET;
		TRACE("addr: %08x nIP: %08x\r\n", inet_addr("127.0.0.1"), nIP);
		servAddr.sin_port = htons(nPort);
		servAddr.sin_addr.s_addr = htonl(nIP);
		if (servAddr.sin_addr.s_addr == INADDR_NONE) {
			AfxMessageBox(_T("指定IP不存在!"));
			return false;
		}
		int ret = connect(mSock, (sockaddr*)&servAddr, sizeof(servAddr));
		if (ret == -1) {
			AfxMessageBox(_T("连接失败!"));
			TRACE("连接失败: %d %s\r\n", WSAGetLastError(), GetErrorInfo(WSAGetLastError()).c_str());
			return false;
		}
		return true;
	}
#define BUFFER_SIZE 4096
	int DealCommand() {
		if (mSock == -1) return false;
		char* buffer = mBuffer.data();
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (1) {
			size_t len = recv(mSock, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0) return -1;
			TRACE("client rev : %d\r\n", len);
			index += len;
			len = index;
			mPacket = CPacket((BYTE*)buffer, len);
			TRACE("客户端解包成功 cmd: %d\r\n", mPacket.sCmd);
			if (len > 0) {
				memcpy(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				return mPacket.sCmd;
			}
		}
		return -1;
	}
	bool Send(const char* pData, int nSize) {
		if (mSock == -1) return false;
		return send(mSock, pData, nSize, 0) > 0;
	}
	bool Send(CPacket& pack) {
		TRACE("mSock = %d\r\n", mSock);
		if (mSock == -1) return false;
		return send(mSock, pack.Data(), pack.Size(), 0) > 0;
	}
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
private:
	std::vector<char> mBuffer;
	SOCKET mSock;
	CPacket mPacket;
	// 禁用拷贝构造
	CClientSocket& operator=(const CClientSocket& ss) {}
	CClientSocket(const CClientSocket& ss) {
		mSock = ss.mSock;
	}
	CClientSocket() {
		if (!InitSockEnv()) {
			MessageBox(NULL, _T("无法初始化套接字环境，请检查网络设置"), _T("初始化错误!"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		mBuffer.resize(BUFFER_SIZE);
	}
	~CClientSocket() {
		closesocket(mSock);
		WSACleanup();
	}
	BOOL InitSockEnv() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
			return FALSE;
		}//指定版本
		return TRUE;
	}
	//这里mInstance是静态变量，release需要操作这个变量所以是静态的函数
	static void releaseInstance() {
		if (mInstance) {
			CClientSocket* tmp = mInstance;
			mInstance = NULL;
			//delete时调用CClientSocket的析构函数
			delete tmp;
		}
	}
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