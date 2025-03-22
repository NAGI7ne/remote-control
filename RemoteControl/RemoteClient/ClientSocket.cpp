#include "pch.h"
#include "ClientSocket.h"

CClientSocket* CClientSocket::mInstance = NULL;
CClientSocket::CHelper CClientSocket::mHelper;
CClientSocket* pclient = CClientSocket::getInstance();

std::string GetErrorInfo(int wsaErrCode) {
	std::string ret;
	LPVOID lpMsgBuf = NULL;
	FormatMessage(
		//��ϵͳ�ṩ��Ԥ���������Ϣ��Դ�л�ȡ������Ϣ��
		//��ϵͳ�Զ�����һ���㹻���ɴ�����Ϣ�ı����ڴ滺������
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		//������Ϣ��Դ��ϵͳ�����Զ���ģ�顣
		NULL,
		//��Ҫ��ѯ����Ĵ��롣
		wsaErrCode,
		//����һ�������ԡ����Ի�����ϵͳ������Ĭ�ϵ������Է��ش����ַ�����
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);

	ret = (char*)lpMsgBuf;
	//ϵͳΪ������Ϣ�������ڴ棬����ʹ�� LocalFree ���ͷ�����ڴ棬�����ڴ�й©��
	LocalFree(lpMsgBuf);
	return ret;
}

//void Dump(BYTE* pData, size_t nSize)
//{
//	std::string strOut;
//	for (size_t i = 0; i < nSize; i++)
//	{
//		char buf[8] = "";
//		if (i > 0 && (i % 16 == 0))strOut += "\n";
//		snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
//		strOut += buf;
//	}
//	strOut += "\n";
//	OutputDebugStringA(strOut.c_str());
//}

bool CClientSocket::InitSocket()
{
	if (mSock != INVALID_SOCKET) CloseSocket();
	mSock = socket(PF_INET, SOCK_STREAM, 0);
	if (mSock == -1) return false;
	sockaddr_in servAddr;
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	TRACE("addr: %08x nIP: %08x\r\n", inet_addr("127.0.0.1"), mnIP);
	servAddr.sin_port = htons(mnPort);
	servAddr.sin_addr.s_addr = htonl(mnIP);
	if (servAddr.sin_addr.s_addr == INADDR_NONE) {
		AfxMessageBox(_T("ָ��IP������!"));
		return false;
	}
	int ret = connect(mSock, (sockaddr*)&servAddr, sizeof(servAddr));
	if (ret == -1) {
		AfxMessageBox(_T("����ʧ��!"));
		TRACE("����ʧ��: %d %s\r\n", WSAGetLastError(), GetErrorInfo(WSAGetLastError()).c_str());
		return false;
	}
	return true;
}

bool CClientSocket::SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClosed)
{
	if (mSock == INVALID_SOCKET && mhThread ==INVALID_HANDLE_VALUE) {
		//if (InitSocket() == false) return false;
		mhThread = (HANDLE)_beginthread(&CClientSocket::threadEntry, 0, this);
		TRACE("thread start\r\n");
	}
	mLock.lock();
	auto pr = mMapAck.insert(std::pair<HANDLE, std::list<CPacket>&>
		(pack.hEvent, lstPacks));
	mMapAutoClosed.insert(std::pair<HANDLE, bool>(pack.hEvent, isAutoClosed));
	mlstSend.push_back(pack);  //TODO:���̰߳�ȫ
	mLock.unlock();
	TRACE("cmd: %d event: %08X threadID: %d\r\n", pack.sCmd, pack.hEvent, GetCurrentThreadId());
	WaitForSingleObject(pack.hEvent, INFINITE);
	TRACE("cmd: %d event: %08X threadID: %d\r\n", pack.sCmd, pack.hEvent, GetCurrentThreadId());
	std::map<HANDLE, std::list<CPacket>&>::iterator it;
	it = mMapAck.find(pack.hEvent);
	if (it != mMapAck.end()) {
		mLock.lock();
		mMapAck.erase(it);
		mLock.unlock();
		return true;
	}
	TRACE("socket init done!\r\n");
	return false;
}

void CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc();
}

void CClientSocket::threadFunc()
{
	std::string strBuffer;
	strBuffer.resize(BUFFER_SIZE);
	char* pBuffer = (char*)strBuffer.c_str();
	int index = 0;
	InitSocket();
	while (mSock != INVALID_SOCKET) {
		if (mlstSend.size() > 0) {
			TRACE("lstSend size: %d\r\n", mlstSend.size());
			mLock.lock();
			CPacket& head = mlstSend.front();
			mLock.unlock();
			if (Send(head) == false) {
				TRACE("����ʧ��!\r\n");
				continue;
			}
			std::map<HANDLE, std::list<CPacket>&>::iterator it;
			it = mMapAck.find(head.hEvent);
			if (it != mMapAck.end()) {
				std::map<HANDLE, bool>::iterator it0 = mMapAutoClosed.find(head.hEvent);
				do {
					int length = recv(mSock, pBuffer + index, BUFFER_SIZE - index, 0);
					TRACE("recv: %d index: %d\r\n", length, index);
					if (length > 0 || (index > 0)) {   //�������˳��Ҳ���ܲ���Ӱ��
						index += length;
						size_t size = (size_t)index;
						CPacket pack((BYTE*)pBuffer, size);
						if (size > 0) {
							pack.hEvent = head.hEvent;
							it->second.push_back(pack);
							memmove(pBuffer, pBuffer + size, index - size);
							index -= size;
							TRACE("SetEvent: %d %d\r\n", pack.sCmd, it0->second);
							if (it0->second) {
								SetEvent(head.hEvent);
								break;
							}
						}
					}
					else if (length <= 0 && index <= 0) {
						CloseSocket();
						SetEvent(head.hEvent); //�ȵ��������ر�����֮����֪ͨ�¼����
						mMapAutoClosed.erase(it0);
						TRACE("SetEvent: %d %d\r\n", head.sCmd, it0->second);
						break;
					}
				} while (it0->second == false);
			}
			mLock.lock();
			mlstSend.pop_front();
			mLock.unlock();
			if (InitSocket() == false) {
				InitSocket();
			}
		}
		Sleep(1);
	}
	CloseSocket();
}

bool CClientSocket::Send(const CPacket& pack)
{
	TRACE("mSock = %d\r\n", mSock);
	if (mSock == -1) return false;
	std::string strOut;
	pack.Data(strOut);
	return send(mSock, strOut.c_str(), strOut.size(), 0) > 0;
}
