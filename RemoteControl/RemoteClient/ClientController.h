#pragma once
#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include "resource.h"
#include <map>

#define WM_SEND_PACK (WM_USER + 1)  //发送数据包
#define WM_SEND_DATA (WM_USER + 2)  //发送数据
#define WM_SHOW_STAUTS (WM_USER + 3 ) // 展示状态
#define WM_SHOW_WATCH (WM_USER + 4) // 远程控制
#define WM_SEND_MESSAGE (WM_USER + 0X1000) //自定义消息处理

class CClientController
{
public:
	//获取全局唯一对象
	static CClientController* getInstance();
	//初始化操作
	int InitContorller();
	//启动
	int Invoke(CWnd* pMainWnd);
	//发送消息
	LRESULT SendMessage(MSG msg);
protected:
	CClientController() : 
		mStatusDlg(&mRemoteDlg),
		mWatchDlg(&mRemoteDlg)
	{
		mhThread = INVALID_HANDLE_VALUE;
		mnThreadID = -1;
	}
	~CClientController() {
		WaitForSingleObject(mhThread, 100);
	}
	void threadFunc();
	static unsigned __stdcall threadEntry(void* arg);
	static void releaseInstance() {
		if (mInstance != NULL) {
			delete mInstance;
			mInstance = NULL;
		}
	}
	LRESULT OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam);
private:
	typedef struct MsgInfo {   
		MSG msg;
		LRESULT result;
		MsgInfo(MSG m) {
			result = 0;
			memcpy(&msg, &m, sizeof(MSG));
		}
		MsgInfo(const MsgInfo& m) {
			result = m.result;
			memcpy(&msg, &m.msg, sizeof(MSG));
		}
		MsgInfo& operator=(const MsgInfo& m) {
			if (this != &m) {
				result = m.result;
				memcpy(&msg, &m.msg, sizeof(MSG));
			}
			return *this;
		}
	}MSGINFO;
	typedef LRESULT(CClientController::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	static std::map<UINT, MSGFUNC> mMapFunc;  //头文件声明，还需要在cpp实现
	CWatchDialog mWatchDlg;
	CRemoteClientDlg mRemoteDlg;
	CStatusDlg mStatusDlg;
	HANDLE mhThread;
	unsigned mnThreadID;
	static CClientController* mInstance;  //头文件声明，还需要在cpp实现
	class CHelper {
	public:
		CHelper() {
			CClientController::getInstance();
		}
		~CHelper() {
			CClientController::releaseInstance();
		}
	};
	static CHelper mHelper;
};

