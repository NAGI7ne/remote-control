#pragma once
#include "pch.h"
#include "framework.h"
//采用单例设计模式
class CServerSocket    
{
public:
	//确保外部能够访问
	static CServerSocket* getInstance() {  //静态函数没有this指针，无法直接访问成员变量
		if(!mInstance) mInstance = new CServerSocket;
		return mInstance;
	}
	bool InitSocket() {
		SOCKET mSock = socket(PF_INET, SOCK_STREAM, 0);
		if (!mSock) return false;
		sockaddr_in servAddr, clntAddr;
		memset(&servAddr, 0, sizeof(servAddr));
		servAddr.sin_addr.s_addr = INADDR_ANY; // 服务器不止一个ip地址,所监听所有，确保客户端能连上
		servAddr.sin_family = AF_INET;
		servAddr.sin_port = htons(9339);
		if (!(bind(mSock, (sockaddr*)&servAddr, sizeof(servAddr)))) return false;
		if (!listen(mSock, 1)) return false;
		return true;
	}
	bool AcceptClient() {
		sockaddr_in clntAddr;
		int clntAddrSize = sizeof(clntAddr);
		mClntSock = accept(mSock, (sockaddr*)&clntAddr, &clntAddrSize);
		if (!mClntSock) return false;
	}
	int DealCommand() {
		if (!mClntSock) return false;
		char buffer[1024]{};
		while (1) {
			int len = recv(mSock, buffer, sizeof(buffer), 0);
			if (len <= 0) return -1;
		}
	}
	bool Send(const char* pData, int nSize) {
		if (!mClntSock) return false;
		return send(mClntSock, pData, nSize, 0) > 0;
	}
private:
	SOCKET mSock, mClntSock;
	// 禁用拷贝构造
	CServerSocket& operator=(const CServerSocket &ss){}
	CServerSocket(const CServerSocket& ss) {
		mSock = ss.mSock;
		mClntSock = ss.mClntSock;
	}
	CServerSocket() {
		mClntSock = INVALID_SOCKET; //-1
		if (!InitSockEnv()) {
			MessageBox(NULL, _T("无法初始化套接字环境，请检查网络设置"), _T("初始化错误!"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		mSock = socket(PF_INET, SOCK_STREAM, 0);
	}
	~CServerSocket() {
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
	static void releaseInstance() {  //TODO:这里为什么是静态的
		if (mInstance) {
			CServerSocket* tmp = mInstance;
			mInstance = NULL;
			delete tmp;
		}
	}
	static CServerSocket* mInstance;
	//防止类没有被析构，原因mInstance没有delete，加一个类来删除mInstance
	class CHelper {
	public:
		CHelper() {
			CServerSocket::getInstance();
		}
		~CHelper() {
			CServerSocket::releaseInstance();
		}
	};
	static CHelper mHelper;
};

