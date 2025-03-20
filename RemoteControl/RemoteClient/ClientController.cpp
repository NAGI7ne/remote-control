#include "pch.h"
#include "ClientController.h"

std::map<UINT, CClientController::MSGFUNC> CClientController::mMapFunc;
CClientController* CClientController::mInstance = NULL;

CClientController* CClientController::getInstance()  //为什么在这里构造
{
	if (mInstance == NULL) {
		mInstance = new CClientController();
		struct {
			UINT nMsg;
			MSGFUNC func;
		}MsgFunc[] = {
			{WM_SEND_PACK, &CClientController::OnSendPack},
			{WM_SEND_DATA, &CClientController::OnSendData},
			{WM_SHOW_STAUTS, &CClientController::OnShowStatus},
			{WM_SHOW_WATCH, &CClientController::OnShowWatcher},
			{(UINT) - 1, NULL}
		};
		for (int i = 0; MsgFunc[i].func != NULL; i++) {
			mMapFunc.insert(std::pair<UINT, MSGFUNC>(MsgFunc[i].nMsg, MsgFunc[i].func));
		}
	}
	return mInstance;
}

int CClientController::InitContorller()
{
	mhThread = (HANDLE)_beginthreadex(NULL, 0,
		&CClientController::threadEntry,
		this, 0, &mnThreadID);
	mStatusDlg.Create(IDD_DLG_STATUS, &mRemoteDlg);
	return 0;
}

int CClientController::Invoke(CWnd* pMainWnd)
{
	pMainWnd = &mRemoteDlg;
	return mRemoteDlg.DoModal();
}

LRESULT CClientController::SendMessage(MSG msg)
{
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);  //TODO;
	if(hEvent == NULL) return -2;
	MSGINFO info(msg);
	//PostThreadMessage 只是把消息放入目标线程的消息队列，
	//本身不会复制或管理这些数据的内存。
	//如果直接将局部变量的地址或临时构造的数据传递过去
	//可能会遇到在消息处理时数据已经失效的问题。
	//使用事件在发送消息后阻塞
	// 就能保证消息的内容在整个跨线程通信过程中始终有效。
	PostThreadMessage(mnThreadID, WM_SEND_MESSAGE, (WPARAM)&info, (LPARAM)&hEvent);
	WaitForSingleObject(hEvent, -1); 
	return info.result;
}

void CClientController::threadFunc()
{
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
		if (msg.message == WM_SEND_MESSAGE) {
			MSGINFO* pmsg = (MSGINFO*)msg.wParam;
			HANDLE hEvent = (HANDLE)msg.lParam;
			std::map<UINT, MSGFUNC>::iterator it = mMapFunc.find(pmsg->msg.message);
			if (it != mMapFunc.end()) {
				pmsg->result = (this->*it->second)(pmsg->msg.message, pmsg->msg.wParam, pmsg->msg.lParam);
			}
			else {
				pmsg->result = -1;
			}
			SetEvent(hEvent);
		}
		else {
			std::map<UINT, MSGFUNC>::iterator it = mMapFunc.find(msg.message);
			if (it != mMapFunc.end()) {
				(this->*it->second)(msg.message, msg.wParam, msg.lParam);
			}
		}
	}
}

unsigned __stdcall CClientController::threadEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadFunc();
	_endthreadex(0);
	return 0;
}

LRESULT CClientController::OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return LRESULT();
}

LRESULT CClientController::OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return LRESULT();
}

LRESULT CClientController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return mStatusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{

	return mWatchDlg.DoModal();
}
