#pragma once

#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>
#include <list>
#include <map>
#include <mutex>

#define WM_SEND_PACK (WM_USER + 1)  //�������ݰ�
#define WM_SEND_PACK_ACK (WM_USER + 2) //���Ͱ�����Ӧ��

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
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {  //��װ��
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
	{    //���
		size_t i = 0;
		for (i; i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFF) {   //��ͷ���� 
				sHead = *(WORD*)(pData + i);
				i += 2;    //������ͷ
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize)   //+4���ݶ� +2�������� +2��У��
		{							//����Ƿ���յ����ݲ�ȫ
			nSize = 0;
			return;
		}
		nLength = *(DWORD*)(pData + i); i += 4; //DWORDΪ4���ֽڣ�ǿת��ȡ+iӳ����4���ֽڣ�
		//ȡֵ��ɺ����������ȶ�
		if (nLength + i > nSize) // +i �ܵ����������ĳ���
		{							//�����Ƿ���ȫ�յ�
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
			sum += BYTE(strData[j]) & 0xFF;  //תΪ�ֽ�
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
	const char* Data(std::string& strOut) const{    //������������
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
	WORD sHead; //��ͷ 0xFEFF
	DWORD nLength; //������ �����������У�飩
	WORD sCmd;  //��������
	std::string strData;   //������ 
	WORD sSum; //��У��(У������ݵĺ�)
};
#pragma pack(pop)

typedef struct fileInfo {
	fileInfo() {
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid; //�Ƿ���Ч
	BOOL IsDirectory; //�Ƿ�ΪĿ¼ 
	BOOL HasNext; //�Ƿ��к���
	char szFileName[256];  //�ļ���
}FILEINFO, * PFILEINFO;

enum {   //TODO
	CSM_AUTOCLOSE = 1, //CSM = Client Socket Mode �Զ��ر�
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
	WORD nAction;  //����� �ƶ��� ˫��
	WORD nButton;  //����� �Ҽ��� �м�
	POINT ptXY;   //����
}MOUSEEV, * PMOUSEEV;

std::string GetErrorInfo(int wsaErrCode);
//���õ������ģʽ
class CClientSocket
{
public:
	//ȷ���ⲿ�ܹ�����
	//��̬����û��thisָ�룬�޷�ֱ�ӷ��ʳ�Ա����
	static CClientSocket* getInstance() {
		if (!mInstance) mInstance = new CClientSocket();
		return mInstance;
	}
	bool InitSocket();
#define BUFFER_SIZE 2048000
	int DealCommand() {
		if (mSock == -1) return false;
		char* buffer = mBuffer.data();  //���̷߳���������ܻ���ֳ�ͻ
		//memset(buffer, 0, BUFFER_SIZE);

		// index��¼��ǰ���������ۼƵ���Ч���ݳ���
		//ȷ��δ��������ݱ����ڻ�������ǰ����Ϊ�������ݽ�������׼��
		static size_t index = 0;    
		while (1) {
			size_t len = recv(mSock, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0 && (index <= 0)) return -1;  //���û���յ����ݲ��һ�����û������
			TRACE("rev len = %d(0x%08X) index = %d(0x%08X)\r\n", len, len, index, index);
			index += len;
			len = index;
			TRACE("rev len = %d(0x%08X) index = %d(0x%08X)\r\n", len, len, index, index);
			mPacket = CPacket((BYTE*)buffer, len);
			//TRACE("�ͻ��˽���ɹ� cmd: %d\r\n", mPacket.sCmd);
			if (len > 0) {
				memcpy(buffer, buffer + len, index - len);  //������index-len���״�
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
	////�洢��Ӧ�����һϵ�а�
	//���հ���ȷ��������£�Ч�ʱ�vector��
	std::map<HANDLE, std::list<CPacket>&> mMapAck; 
	std::map<HANDLE, bool>mMapAutoClosed;
	int mnIP;
	int mnPort;
	std::vector<char> mBuffer;
	SOCKET mSock;
	CPacket mPacket;
	// ���ÿ�������
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
		}//ָ���汾
		return TRUE;
	}
	//����mInstance�Ǿ�̬������release��Ҫ����������������Ǿ�̬�ĺ���
	static void releaseInstance() {
		if (mInstance != NULL) {
			CClientSocket* tmp = mInstance;
			mInstance = NULL;
			//deleteʱ����CClientSocket����������
			delete tmp;
		}
	}
	bool Send(const char* pData, int nSize) {
		if (mSock == -1) return false;
		return send(mSock, pData, nSize, 0) > 0;
	}
	bool Send(const CPacket& pack);
	void SendPack(UINT nmsg, WPARAM wParam/*��������ֵ*/, LPARAM lParam/*�������ĳ���*/);

	//��main��ʼǰ��ʼ��ʵ��
	static CClientSocket* mInstance;
	//��ֹ��û�б�������ԭ��mInstanceû��delete����һ���������ɾ��mInstance
	//CHelper �����������䵱��һ������ߣ����ڶ�������ʱ����������
	class CHelper {
	public:
		CHelper() {
			CClientSocket::getInstance();
		}
		~CHelper() {
			CClientSocket::releaseInstance();
		}
	};
	//��staticʵ�ֳ�������ǰ�Զ����졢������ֹʱ�Զ�����
	static CHelper mHelper;
};