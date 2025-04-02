#pragma once
#include "pch.h"
#include "framework.h"
#include <list>
#include "Packet.h"

typedef void (*SOCKET_CALLBACK)(void* , int, std::list<CPacket>&, CPacket&);

//采用单例设计模式
class CServerSocket    
{
public:
	//确保外部能够访问
	//静态函数没有this指针，无法直接访问成员变量
	static CServerSocket* getInstance() { 
		if(!mInstance) mInstance = new CServerSocket;
		return mInstance;
	}

	int Run(SOCKET_CALLBACK callback, void* arg, short port = 9339) {
		bool ret = InitSocket(port);
		if (ret == false) return -1;
		std::list<CPacket> lstPacket;
		mCallback = callback;  //使 mCallback 指向 CCommand::RunCommand
		mArg = arg;
		int cnt = 0;
		while (true) {
			if (AcceptClient() == false) {
				if (cnt >= 3) {
					return -2;
				}
				cnt++;
			}
			int ret = DealCommand();  
			if (ret > 0) {
				mCallback(mArg, ret, lstPacket, mPacket); //调用操作符 () 把参数传递给 mCallback 所指向的函数
														  //并立即执行mCallback指向的函数。
				while (lstPacket.size() > 0) {
					Send(lstPacket.front());   //从队列中取包，完成发送，弹出
					lstPacket.pop_front();
				}
			}
			CloseClient();
		}
		return 0;
	}

protected:
	bool InitSocket(short port) {
		//mSock = socket(PF_INET, SOCK_STREAM, 0);
		if (mSock == -1) return false;
		sockaddr_in servAddr;
		memset(&servAddr, 0, sizeof(servAddr));
		// 服务器可能不止一个ip地址,所监听所有，确保客户端能连上
		servAddr.sin_addr.s_addr = INADDR_ANY;
		servAddr.sin_family = AF_INET;
		servAddr.sin_port = htons(port);
		if (bind(mSock, (sockaddr*)&servAddr, sizeof(servAddr)) == -1) return false;
		if (listen(mSock, 1) == -1) return false;
		return true;
	}

	bool AcceptClient() {
		TRACE("enter AcceptClient\r\n");
		sockaddr_in clntAddr;
		int clntAddrSize = sizeof(clntAddr);
		mClntSock = accept(mSock, (sockaddr*)&clntAddr, &clntAddrSize);
		TRACE("mClnt = %d\r\n", mClntSock);
		if (mClntSock == -1) return false;
		return true;
	}
#define BUFFER_SIZE 4096
	//接收数据，提取数据
	//构造数据包，并提取命令号
	int DealCommand() {
		if (mClntSock == -1) return -1;
		char* buffer = new char[BUFFER_SIZE];
		if (buffer == NULL) {
			TRACE("内存不足! \r\n");
			return - 2;
		}
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (1) {
			size_t len = recv(mClntSock, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0) { delete[] buffer; return -1; }
			TRACE("server rev : %d\r\n", len);
			index += len;
			len = index;
			mPacket = CPacket((BYTE*)buffer, len);
			TRACE("服务器解包成功 cmd: %d\r\n", mPacket.sCmd);
			if (len > 0) {
				memcpy(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				delete[] buffer;
				return mPacket.sCmd;
			}
		}
		delete[] buffer;
		return -1;
	}
	bool Send(const char* pData, int nSize) {
		if (mClntSock == -1) return false;
		return send(mClntSock, pData, nSize, 0) > 0;
	}
	bool Send(CPacket& pack) {
		if (mClntSock == -1) return false;
		return send(mClntSock, pack.Data(), pack.Size(), 0) > 0;
	}
	/*bool GetMouseEvent(MOUSEEV& mouse) {
		if (mPacket.sCmd == 5) {
			memcpy(&mouse, mPacket.strData.c_str(), mPacket.strData.size());
			return true;
		}
		return false;
	}*/
	void CloseClient() {
		if (mClntSock != INVALID_SOCKET) {
			closesocket(mClntSock);
			mClntSock = INVALID_SOCKET;
		}
	}
private:
	SOCKET_CALLBACK mCallback;
	void* mArg;
	SOCKET mSock, mClntSock;
	CPacket mPacket;
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
	//这里mInstance是静态变量，release需要操作这个变量所以是静态的函数
	static void releaseInstance() { 
		if (mInstance) {
			CServerSocket* tmp = mInstance;
			mInstance = NULL;
			//delete时调用CServerSocket的析构函数
			delete tmp;
		}
	}
	//在main开始前初始化实例
	static CServerSocket* mInstance;
	//防止类没有被析构，原因mInstance没有delete，加一个类来间接删除mInstance
	//CHelper 的析构函数充当了一个监控者，它在对象销毁时主动清理单例
	class CHelper {
	public:
		CHelper() {
			CServerSocket::getInstance();
		}
		~CHelper() {
			CServerSocket::releaseInstance();
		}
	};
	//加static实现程序启动前自动构造、程序终止时自动析构
	static CHelper mHelper;
};