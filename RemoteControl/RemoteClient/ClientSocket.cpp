#include "pch.h"
#include "ClientSocket.h"

CClientSocket* CClientSocket::mInstance = NULL;
CClientSocket::CHelper CClientSocket::mHelper;
CClientSocket* pclient = CClientSocket::getInstance();

std::string GetErrorInfo(int wsaErrCode) {
	std::string ret;
	LPVOID lpMsgBuf = NULL;
	FormatMessage(
		//从系统提供的预定义错误消息资源中获取错误信息。
		//让系统自动分配一个足够容纳错误信息文本的内存缓冲区。
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		//错误信息来源于系统而非自定义模块。
		NULL,
		//需要查询错误的代码。
		wsaErrCode,
		//构造一个“中性”语言环境，系统将基于默认的子语言返回错误字符串。
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);

	ret = (char*)lpMsgBuf;
	//系统为错误消息分配了内存，所以使用 LocalFree 来释放这块内存，避免内存泄漏。
	LocalFree(lpMsgBuf);
	return ret;
}

void Dump(BYTE* pData, size_t nSize)
{
	std::string strOut;
	for (size_t i = 0; i < nSize; i++)
	{
		char buf[8] = "";
		if (i > 0 && (i % 16 == 0))strOut += "\n";
		snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
		strOut += buf;
	}
	strOut += "\n";
	OutputDebugStringA(strOut.c_str());
}

CClientSocket::CClientSocket(const CClientSocket& ss) {
	mhThread = INVALID_HANDLE_VALUE;
	mbAutoClose = ss.mbAutoClose;
	mSock = ss.mSock;
	mnIP = ss.mnIP;
	mnPort = ss.mnPort;
	std::map<UINT, CClientSocket::MSGFUNC>::const_iterator it = ss.mMapFunc.begin();
	for (; it != ss.mMapFunc.end(); it++) {
		mMapFunc.insert(std::pair<UINT, MSGFUNC>(it->first, it->second));
	}
}

CClientSocket::CClientSocket() : mnIP(INADDR_ANY), mnPort(0),
mSock(INVALID_SOCKET), mbAutoClose(true), mhThread(INVALID_HANDLE_VALUE) {
	if (InitSockEnv() == FALSE) {
		MessageBox(NULL, _T("无法初始化套接字环境，请检查网络设置"), _T("初始化错误!"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	mEventInvoke = CreateEvent(NULL, TRUE, FALSE, NULL);
	mhThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry, this, 0, &mhThreadID);
	if (WaitForSingleObject(mEventInvoke, 100) == WAIT_TIMEOUT) {
		TRACE("线程启动失败!\r\n");
	}
	CloseHandle(mEventInvoke);
	mBuffer.resize(BUFFER_SIZE);
	memset(mBuffer.data(), 0, BUFFER_SIZE);
	struct {
		UINT message;
		MSGFUNC func;
	}funcs[] = {
		{WM_SEND_PACK, &CClientSocket::SendPack},
		{0, NULL},
	};
	for (int i = 0; funcs[i].message != 0; i++) {
		if (mMapFunc.insert(std::pair<UINT, MSGFUNC>(funcs[i].message, funcs[i].func)).second == false) {
			TRACE("插入消息失败， 消息值：%d 函数值: %08X 序号: %d\r\n", funcs[i].message, funcs[i].func, i);
		}
	}
}

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

bool CClientSocket::SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed, WPARAM wParam) {
	UINT nMode = isAutoClosed ? CSM_AUTOCLOSE : 0;
	std::string strOut;
	pack.Data(strOut);
	PACKET_DATA* pData = new PACKET_DATA(strOut.c_str(), strOut.size(), nMode, wParam);
	bool ret = PostThreadMessage(mhThreadID, WM_SEND_PACK, (WPARAM)pData, (LPARAM)hWnd);
	if (ret == false) {
		delete pData;
	}
	return ret;
}

//bool CClientSocket::SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClosed)
//{
//	if (mSock == INVALID_SOCKET && mhThread ==INVALID_HANDLE_VALUE) {
//		//if (InitSocket() == false) return false;
//		mhThread = (HANDLE)_beginthread(&CClientSocket::threadEntry, 0, this);
//		TRACE("thread start\r\n");
//	}
//	mLock.lock();
//	auto pr = mMapAck.insert(std::pair<HANDLE, std::list<CPacket>&>
//		(pack.hEvent, lstPacks));
//	mMapAutoClosed.insert(std::pair<HANDLE, bool>(pack.hEvent, isAutoClosed));
//	mlstSend.push_back(pack);  //TODO:非线程安全
//	mLock.unlock();
//	TRACE("cmd: %d event: %08X threadID: %d\r\n", pack.sCmd, pack.hEvent, GetCurrentThreadId());
//	WaitForSingleObject(pack.hEvent, INFINITE);
//	TRACE("cmd: %d event: %08X threadID: %d\r\n", pack.sCmd, pack.hEvent, GetCurrentThreadId());
//	std::map<HANDLE, std::list<CPacket>&>::iterator it;
//	it = mMapAck.find(pack.hEvent);
//	if (it != mMapAck.end()) {
//		mLock.lock();
//		mMapAck.erase(it);
//		mLock.unlock();
//		return true;
//	}
//	TRACE("socket init done!\r\n");
//	return false;
//}

unsigned CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc2();
	_endthreadex(0);
	return 0;
}

//void CClientSocket::threadFunc()
//{
//	std::string strBuffer;
//	strBuffer.resize(BUFFER_SIZE);
//	char* pBuffer = (char*)strBuffer.c_str();
//	int index = 0;
//	InitSocket();
//	while (mSock != INVALID_SOCKET) {
//		if (mlstSend.size() > 0) {
//			TRACE("lstSend size: %d\r\n", mlstSend.size());
//			mLock.lock();
//			CPacket& head = mlstSend.front();
//			mLock.unlock();
//			if (Send(head) == false) {
//				TRACE("发送失败!\r\n");
//				continue;
//			}
//			std::map<HANDLE, std::list<CPacket>&>::iterator it;
//			it = mMapAck.find(head.hEvent);
//			if (it != mMapAck.end()) {
//				std::map<HANDLE, bool>::iterator it0 = mMapAutoClosed.find(head.hEvent);
//				do {
//					int length = recv(mSock, pBuffer + index, BUFFER_SIZE - index, 0);
//					TRACE("recv: %d index: %d\r\n", length, index);
//					if (length > 0 || (index > 0)) {   //运算符的顺序也可能产生影响
//						index += length;
//						size_t size = (size_t)index;
//						CPacket pack((BYTE*)pBuffer, size);
//						if (size > 0) {
//							pack.hEvent = head.hEvent;
//							it->second.push_back(pack);
//							memmove(pBuffer, pBuffer + size, index - size);
//							index -= size;
//							TRACE("SetEvent: %d %d\r\n", pack.sCmd, it0->second);
//							if (it0->second) {
//								SetEvent(head.hEvent);
//								break;
//							}
//						}
//					}
//					else if (length <= 0 && index <= 0) {
//						CloseSocket();
//						SetEvent(head.hEvent); //等到服务器关闭命令之后，再通知事件完成
//						if (it0 != mMapAutoClosed.end()) {
//							TRACE("SetEvent: %d %d\r\n", head.sCmd, it0->second);
//						}
//						else {
//							TRACE("异常情况\r\n");
//						}
//						break;
//					}
//				} while (it0->second == false);
//			}
//			mLock.lock();
//			mlstSend.pop_front();
//			mMapAutoClosed.erase(head.hEvent);
//			mLock.unlock();
//			if (InitSocket() == false) {
//				InitSocket();
//			}
//		}
//		Sleep(1);
//	}
//	CloseSocket();
//}

void CClientSocket::threadFunc2()
{
	SetEvent(mEventInvoke);
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		TRACE("Get Message :%08X \r\n", msg.message);
		if (mMapFunc.find(msg.message) != mMapFunc.end()) {
			(this->*mMapFunc[msg.message])(msg.message, msg.wParam, msg.lParam);
		}
	}
}

bool CClientSocket::Send(const CPacket& pack)
{
	TRACE("mSock = %d\r\n", mSock);
	if (mSock == -1) return false;
	std::string strOut;
	pack.Data(strOut);
	return send(mSock, strOut.c_str(), strOut.size(), 0) > 0;
}

void CClientSocket::SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	// 如果函数发送的是临时变量
	// 在函数执行完成之后会被释放，有潜在的风险
	// 所以在这里构造局部变量一个用于储存发送值的临时变量
	// 并且将wParam释放，以防内存泄露的风险
	PACKET_DATA data = *(PACKET_DATA*)wParam;
	delete (PACKET_DATA*)wParam;
	HWND hWnd = (HWND)lParam;
	size_t nTemp = data.strData.size();
	CPacket current((BYTE*)data.strData.c_str(), nTemp);
	if (InitSocket() == true) {
		int ret = send(mSock, (char*)data.strData.c_str(), (int)data.strData.size(), 0);
		if (ret > 0) {
			size_t index = 0;
			std::string strBuffer;
			strBuffer.resize(BUFFER_SIZE);
			char* pBuffer = (char*)strBuffer.c_str();
			while (mSock != INVALID_SOCKET) {
				int length = recv(mSock, pBuffer + index, BUFFER_SIZE - index, 0);
				if (length > 0 || index > 0) {
					index += (size_t)length;
					size_t nLen = index;
					CPacket pack((BYTE*)pBuffer, nLen);  //发包完成后会返回发包用的nLen
					if(nLen > 0){
						TRACE("ack pack %d to hWnd %08X %d %d\r\n", pack.sCmd, hWnd, index, nLen);
						TRACE("%04X\r\n", *(WORD*)pBuffer + nLen);
						::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(pack), data.wParam);  //TODO:
						if (data.nMode & CSM_AUTOCLOSE) {  //TODO:
							CloseSocket();
							return;
						}
						index -= nLen;
						memmove(pBuffer, pBuffer + nLen, index);
					}
				}
				else {
					TRACE("recv failed length %d index %d cmd %d\r\n", length, index, current.sCmd);
					CloseSocket();
					::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(current.sCmd, NULL, 0), 1);
				}
			}
		}
		else {
			CloseSocket();
			::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -1);
		}
	}
	else {
		::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -2); 
	}
}
