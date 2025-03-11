#pragma once
#include "pch.h"
#include "framework.h"
//���õ������ģʽ
class CServerSocket    
{
public:
	//ȷ���ⲿ�ܹ�����
	static CServerSocket* getInstance() {  //��̬����û��thisָ�룬�޷�ֱ�ӷ��ʳ�Ա����
		if(!mInstance) mInstance = new CServerSocket;
		return mInstance;
	}
	bool InitSocket() {
		SOCKET mSock = socket(PF_INET, SOCK_STREAM, 0);
		if (!mSock) return false;
		sockaddr_in servAddr, clntAddr;
		memset(&servAddr, 0, sizeof(servAddr));
		servAddr.sin_addr.s_addr = INADDR_ANY; // ��������ֹһ��ip��ַ,���������У�ȷ���ͻ���������
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
	// ���ÿ�������
	CServerSocket& operator=(const CServerSocket &ss){}
	CServerSocket(const CServerSocket& ss) {
		mSock = ss.mSock;
		mClntSock = ss.mClntSock;
	}
	CServerSocket() {
		mClntSock = INVALID_SOCKET; //-1
		if (!InitSockEnv()) {
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ�����������������"), _T("��ʼ������!"), MB_OK | MB_ICONERROR);
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
		}//ָ���汾
		return TRUE;
	}
	static void releaseInstance() {  //TODO:����Ϊʲô�Ǿ�̬��
		if (mInstance) {
			CServerSocket* tmp = mInstance;
			mInstance = NULL;
			delete tmp;
		}
	}
	static CServerSocket* mInstance;
	//��ֹ��û�б�������ԭ��mInstanceû��delete����һ������ɾ��mInstance
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

