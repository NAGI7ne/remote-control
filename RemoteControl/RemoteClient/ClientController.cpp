#include "pch.h"
#include "ClientSocket.h"
#include "ClientController.h"

std::map<UINT, CClientController::MSGFUNC> CClientController::mMapFunc;
CClientController* CClientController::mInstance = NULL;
CClientController::CHelper CClientController::mHelper;

CClientController* CClientController::getInstance()
{
	if (mInstance == NULL) {
		mInstance = new CClientController();
		struct {
			UINT nMsg;
			MSGFUNC func;
		}MsgFunc[] = {
			//{WM_SEND_PACK, &CClientController::OnSendPack},
			//{WM_SEND_DATA, &CClientController::OnSendData},
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

//LRESULT CClientController::SendMessage(MSG msg)
//{
//	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);  //TODO;
//	if(hEvent == NULL) return -2;
//	MSGINFO info(msg);
//	//PostThreadMessage 只是把消息放入目标线程的消息队列，
//	//本身不会复制或管理这些数据的内存。
//	//如果直接将局部变量的地址或临时构造的数据传递过去
//	//可能会遇到在消息处理时数据已经失效的问题。
//	//使用事件在发送消息后阻塞
//	// 就能保证消息的内容在整个跨线程通信过程中始终有效。
//	PostThreadMessage(mnThreadID, WM_SEND_MESSAGE, (WPARAM)&info, (LPARAM)&hEvent);
//	WaitForSingleObject(hEvent, INFINITE); 
//	CloseHandle(hEvent);
//	return info.result;
//}

bool CClientController::SendCommandPacket(HWND hWnd, int nCmd, bool bAutoClose, BYTE* pData, 
		size_t nLength, WPARAM wParam)
{
	TRACE("cmd:%d %s\r\n", nCmd, __FUNCTION__);
	CClientSocket* pClient = CClientSocket::getInstance();
	bool ret =  pClient->SendPacket(hWnd, CPacket(nCmd, pData, nLength), bAutoClose, wParam);
	//if (!ret) {  // 如果涉及的命令需要创建线程，那么在创建线程时，线程不会立刻执行
	//			 // 所以之后的PostThreadMessage可能会失败
	//			 // 所以需要等待线程启动再重新发送
	//			 // 可以用事件解决
	//	Sleep(30);
	//	ret = pClient->SendPacket(hWnd, CPacket(nCmd, pData, length), bAutoClose, wParam);
	//}
	return ret;
}

void CClientController::DownloadEnd()
{
	mStatusDlg.ShowWindow(SW_HIDE);
	mRemoteDlg.EndWaitCursor();
	mRemoteDlg.MessageBox(_T("下载完成!"), _T("完成"));
}

int CClientController::DownFile(CString strPath)
{
	//创建一个“保存文件”对话框
		//（第一个参数为 FALSE 表示保存文件，若为 TRUE 则为打开文件）。
		//NULL：不指定缺省的扩展名
		//OFN_HIDEREADONLY 隐藏只读复选框 OFN_OVERWRITEPROMPT 如果文件已存在
		// 则提示用户是否覆盖
	CFileDialog dlg(FALSE, NULL, strPath,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, &mRemoteDlg);
	if (dlg.DoModal() == IDOK) {
		mstrRemote = strPath;
		mstrLocal = dlg.GetPathName();
		//二进制写入模式打开（或创建）文件 
		FILE* pFile = fopen(mstrLocal, "wb+");
		if (pFile == NULL) {
			AfxMessageBox("文件无法创建!");
			return -1;
		}
		SendCommandPacket(mRemoteDlg, 4, false, 
			(BYTE*)(LPCTSTR)mstrRemote, 
			mstrRemote.GetLength(), 
			(WPARAM)pFile
		);
		/*mhThreadDownload = (HANDLE)_beginthread(&CClientController::threadDownloadEntry, 0, this);
		if (WaitForSingleObject(mhThreadDownload, 0) != WAIT_TIMEOUT) {
			return -1;
		}*/
		mRemoteDlg.BeginWaitCursor();  //设置鼠标为等待状态 
		mStatusDlg.mDlgStatusInfo.SetWindowText(_T("下载中..."));
		mStatusDlg.ShowWindow(SW_SHOW);
		mStatusDlg.CenterWindow(&mRemoteDlg);
		mStatusDlg.SetActiveWindow(); //  激活至前台
	}
	return 0;
}

void CClientController::StartWatchScreen()
{
	mIsClosed = false;
	//mWatchDlg.SetParent(&mRemoteDlg);
	//_beginthread 返回的是线程 ID 而非真正的内核句柄
	// 最好使用 _beginthreadex
	mhThreadWatch = (HANDLE)_beginthread(&CClientController::threadWatchScreenEntry, 0, this);
	mWatchDlg.DoModal();  //启动对话框的模态循环，模态对话框会阻塞当前线程的其他操作，直到用户关闭对话框。
	mIsClosed = true;
	WaitForSingleObject(mhThreadWatch, 500);    //等待后台线程在最多 500 毫秒内结束运行，会阻塞当前线程 直到后台线程退出或超时
	//线程函数内部虽然调用了 _endthread()，线程的实际终止过程也依赖于操作系统的调度。
   //所以Wait确保线程有足够时间进行清理，避免线程资源泄漏
}

void CClientController::threadWatchScreen()
{
	Sleep(50);   //确保晚于dlg.DoModal();窗口启动
	ULONGLONG nTick = GetTickCount64();
	while (!mIsClosed) {
		if (mWatchDlg.isFull() == false) {   //更新数据到缓存
			if (GetTickCount64() - nTick < 200) {
				Sleep(200 - DWORD(GetTickCount64() - nTick));
			}
			nTick = GetTickCount64();
			int ret = SendCommandPacket(mWatchDlg.GetSafeHwnd(), 6, true, NULL, 0);
			if (ret == 1) {
				//TRACE("成功设置图片!\r\n");
			}
			else {
				TRACE("获取图片失败! ret = %d\r\n", ret);
			}
		}
		Sleep(1);
	}
	TRACE("thread end %d\r\n", mIsClosed);
}

void CClientController::threadWatchScreenEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadWatchScreen();
	_endthread();
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


LRESULT CClientController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return mStatusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{

	return mWatchDlg.DoModal();
}
