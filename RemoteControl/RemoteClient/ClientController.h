#pragma once
#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include "resource.h"
#include "RemoteTool.h"
#include <map>

//#define WM_SEND_PACK (WM_USER + 1)  //�������ݰ�
//#define WM_SEND_DATA (WM_USER + 2)  //��������
#define WM_SHOW_STAUTS (WM_USER + 3 ) // չʾ״̬
#define WM_SHOW_WATCH (WM_USER + 4) // Զ�̿���
#define WM_SEND_MESSAGE (WM_USER + 0X1000) //�Զ�����Ϣ����

class CClientController
{
public:
	//��ȡȫ��Ψһ����
	static CClientController* getInstance();
	//��ʼ������
	int InitContorller();
	//����
	int Invoke(CWnd* pMainWnd);
	//������Ϣ
	LRESULT SendMessage(MSG msg);
	//���������������ַ
	void UpdataAddress(int nIP, int nPort) {
		CClientSocket::getInstance()->UpdataAddress(nIP, nPort);
	}
	int DealCommand() {
		return CClientSocket::getInstance()->DealCommand();
	}
	void CloseSocket() {
		CClientSocket::getInstance()->CloseSocket();
	}
	// 1:�鿴���̷���    2:�鿴ָ��Ŀ¼�ļ�
	// 3:���ļ�        4:�����ļ�  
	// 5:������        6:������Ļ����
	// 7:����            8:����
	// 9:ɾ���ļ�        39:��������
	// ����ֵ����� -1����
	int SendCommandPacket(int nCmd,
		bool bAutoClose = true,
		BYTE* pData = NULL,
		size_t length = 0,
		std::list<CPacket>* plstPacks = NULL);
	
	int GetImage(CImage& image) {
		CClientSocket* pClient = CClientSocket::getInstance();
		return CRemoteTool::Bytr2Image(image, pClient->GetPacket().strData);
	}

	int DownFile(CString strPath);

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
	//LRESULT OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);
	//LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
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
	static std::map<UINT, MSGFUNC> mMapFunc;  //ͷ�ļ�����������Ҫ��cppʵ��
	CWatchDialog mWatchDlg;
	CRemoteClientDlg mRemoteDlg;
	CStatusDlg mStatusDlg;
	HANDLE mhThread;
	HANDLE mhThreadDownload;
	HANDLE mhThreadWatch;
	bool mIsClosed;
	// �����ļ���Զ��·��
	CString mstrRemote;
	//�����ļ��ı��ر���·��
	CString mstrLocal;
	unsigned mnThreadID;
	static CClientController* mInstance;  //ͷ�ļ�����������Ҫ��cppʵ��
	class CHelper {
	public:
		CHelper() {
			//CClientController::getInstance();  // Ҫ��main����ִ����֮���ٳ�ʼ��
		}
		~CHelper() {
			CClientController::releaseInstance();
		}
	};
	static CHelper mHelper;
};

