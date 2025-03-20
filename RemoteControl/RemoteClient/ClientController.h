#pragma once
#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include "resource.h"
#include "RemoteTool.h"
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
	//更新网络服务器地址
	void UpdataAddress(int nIP, int nPort) {
		CClientSocket::getInstance()->UpdataAddress(nIP, nPort);
	}
	int DealCommand() {
		return CClientSocket::getInstance()->DealCommand();
	}
	void CloseSocket() {
		CClientSocket::getInstance()->CloseSocket();
	}
	bool SendPacket(const CPacket& pack) {
		CClientSocket* pClient = CClientSocket::getInstance();
		if (pClient->InitSocket() == false) return false;
		pClient->Send(pack);
	}
	// 1:查看磁盘分区    2:查看指定目录文件
	// 3:打开文件        4:下载文件  
	// 5:鼠标操作        6:发送屏幕内容
	// 7:锁机            8:解锁
	// 9:删除文件        39:测试连接
	// 返回值：命令， -1错误
	int SendCommandPacket(int nCmd,
		bool bAutoClose = true ,
		BYTE* pData = NULL, 
		size_t length = 0) 
	{
		CClientSocket* pClient = CClientSocket::getInstance();
		if (pClient->InitSocket() == false) return false;
		pClient->Send(CPacket(nCmd, pData, length));
		int cmd = DealCommand();  //发送包后进入收包状态
		TRACE("ack: %d\r\n", cmd);
		if (bAutoClose) CloseSocket();
		return cmd;
	}
	
	int GetImage(CImage& image) {
		CClientSocket* pClient = CClientSocket::getInstance();
		return CRemoteTool::Bytr2Image(image, pClient->GetPacket().strData);
	}

	int DownFile(CString strPath) {
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
			mhThreadDownload = (HANDLE)_beginthread(&CClientController::threadDownloadEntry, 0, this);
			if (WaitForSingleObject(mhThreadDownload, 0) != WAIT_TIMEOUT) {
				return -1;
			}
			mRemoteDlg.BeginWaitCursor();  //设置鼠标为等待状态 
			mStatusDlg.mDlgStatusInfo.SetWindowText(_T("下载中..."));
			mStatusDlg.ShowWindow(SW_SHOW);
			mStatusDlg.CenterWindow(&mRemoteDlg);
			mStatusDlg.SetActiveWindow(); //  激活至前台
		}
		return 0;
	}

	void StartWatchScreen();
protected:
	void threadWatchScreen();
	static void threadWatchScreenEntry(void* arg);
	void threadDownloadFile();
	static void threadDownloadEntry(void* arg);
	CClientController() : 
		mStatusDlg(&mRemoteDlg),
		mWatchDlg(&mRemoteDlg)
	{
		mhThread = INVALID_HANDLE_VALUE;
		mhThreadDownload = INVALID_HANDLE_VALUE;
		mhThreadWatch = INVALID_HANDLE_VALUE;
		mnThreadID = -1;
		mIsClosed = true;
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
	HANDLE mhThreadDownload;
	HANDLE mhThreadWatch;
	bool mIsClosed;
	// 下载文件的远程路径
	CString mstrRemote;
	//下载文件的本地保存路径
	CString mstrLocal;
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

