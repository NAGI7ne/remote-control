#pragma once
#include "pch.h"
#include "framework.h"

#pragma pack(push) 
#pragma pack(1)
class CPacket {
public:
	CPacket() : sHead(0), nLength(0), sCmd(0), sSum(0){}
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
	CPacket(const BYTE* pData, size_t& nSize) {    //���
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
	size_t Size() {
		return nLength + 6;
	}
	const char* Data(){    //������������
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)pData = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum; 
		return strOut.c_str();
	}
	~CPacket(){}
public:
	WORD sHead; //��ͷ 0xFEFF
	DWORD nLength; //������ �����������У�飩
	WORD sCmd;  //��������
	std::string strData;   //������ 
	WORD sSum; //��У��(У������ݵĺ�)
	std::string strOut; //������������
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

typedef struct MouseEvent {
	MouseEvent(){
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;  //����� �ƶ��� ˫��
	WORD nButton;  //����� �Ҽ��� �м�
	POINT ptXY;   //����
}MOUSEEV, *PMOUSEEV;

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
	bool InitSocket() {
		//mSock = socket(PF_INET, SOCK_STREAM, 0);
		if (mSock == -1) return false;
		sockaddr_in servAddr;
		memset(&servAddr, 0, sizeof(servAddr));
		// ���������ܲ�ֹһ��ip��ַ,���������У�ȷ���ͻ���������
		servAddr.sin_addr.s_addr = INADDR_ANY; 
		servAddr.sin_family = AF_INET;
		servAddr.sin_port = htons(9339);
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
	bool GetFilePath(std::string &strPath)const {
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
	void CloseClient() {
		closesocket(mClntSock);
		mClntSock = INVALID_SOCKET;
	}
private:
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