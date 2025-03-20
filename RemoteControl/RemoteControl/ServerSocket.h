#pragma once
#include "pch.h"
#include "framework.h"
#include <list>
#include "Packet.h"

typedef void (*SOCKET_CALLBACK)(void* , int, std::list<CPacket>&, CPacket&);

//���õ������ģʽ
class CServerSocket    
{
public:
	//ȷ���ⲿ�ܹ�����
	//��̬����û��thisָ�룬�޷�ֱ�ӷ��ʳ�Ա����
	static CServerSocket* getInstance() { 
		if(!mInstance) mInstance = new CServerSocket;
		return mInstance;
	}

	int Run(SOCKET_CALLBACK callback, void* arg, short port = 9339) {
		bool ret = InitSocket(port);
		if (ret == false) return -1;
		std::list<CPacket> lstPacket;
		mCallback = callback;  //ʹ mCallback ָ�� CCommand::RunCommand
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
				mCallback(mArg, ret, lstPacket, mPacket); //���ò����� () �Ѳ������ݸ� mCallback ��ָ��ĺ���
														  //������ִ��mCallbackָ��ĺ�����
				while (lstPacket.size() > 0) {
					Send(lstPacket.front());   //�Ӷ�����ȡ������ɷ��ͣ�����
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
		// ���������ܲ�ֹһ��ip��ַ,���������У�ȷ���ͻ���������
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
	//�������ݣ���ȡ����
	//�������ݰ�������ȡ�����
	int DealCommand() {
		if (mClntSock == -1) return -1;
		char* buffer = new char[BUFFER_SIZE];
		if (buffer == NULL) {
			TRACE("�ڴ治��! \r\n");
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
			TRACE("����������ɹ� cmd: %d\r\n", mPacket.sCmd);
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
	//����mInstance�Ǿ�̬������release��Ҫ����������������Ǿ�̬�ĺ���
	static void releaseInstance() { 
		if (mInstance) {
			CServerSocket* tmp = mInstance;
			mInstance = NULL;
			//deleteʱ����CServerSocket����������
			delete tmp;
		}
	}
	//��main��ʼǰ��ʼ��ʵ��
	static CServerSocket* mInstance;
	//��ֹ��û�б�������ԭ��mInstanceû��delete����һ���������ɾ��mInstance
	//CHelper �����������䵱��һ������ߣ����ڶ�������ʱ����������
	class CHelper {
	public:
		CHelper() {
			CServerSocket::getInstance();
		}
		~CHelper() {
			CServerSocket::releaseInstance();
		}
	};
	//��staticʵ�ֳ�������ǰ�Զ����졢������ֹʱ�Զ�����
	static CHelper mHelper;
};