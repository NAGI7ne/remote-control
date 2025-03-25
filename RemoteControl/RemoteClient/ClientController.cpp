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

LRESULT CClientController::SendMessage(MSG msg)
{
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);  //TODO;
	if(hEvent == NULL) return -2;
	MSGINFO info(msg);
	//PostThreadMessage ֻ�ǰ���Ϣ����Ŀ���̵߳���Ϣ���У�
	//�����Ḵ�ƻ������Щ���ݵ��ڴ档
	//���ֱ�ӽ��ֲ������ĵ�ַ����ʱ��������ݴ��ݹ�ȥ
	//���ܻ���������Ϣ����ʱ�����Ѿ�ʧЧ�����⡣
	//ʹ���¼��ڷ�����Ϣ������
	// ���ܱ�֤��Ϣ���������������߳�ͨ�Ź�����ʼ����Ч��
	PostThreadMessage(mnThreadID, WM_SEND_MESSAGE, (WPARAM)&info, (LPARAM)&hEvent);
	WaitForSingleObject(hEvent, INFINITE); 
	CloseHandle(hEvent);
	return info.result;
}

bool CClientController::SendCommandPacket(HWND hWnd, int nCmd, bool bAutoClose, BYTE* pData, 
		size_t length, WPARAM wParam)
{
	TRACE("%s start %lld \r\n", __FUNCTION__, GetTickCount64());
	CClientSocket* pClient = CClientSocket::getInstance();
	return pClient->SendPacket(hWnd, CPacket(nCmd, pData, length), bAutoClose, wParam);
}

void CClientController::DownloadEnd()
{
	mStatusDlg.ShowWindow(SW_HIDE);
	mRemoteDlg.EndWaitCursor();
	mRemoteDlg.MessageBox(_T("�������!"), _T("���"));
}

int CClientController::DownFile(CString strPath)
{
	//����һ���������ļ����Ի���
		//����һ������Ϊ FALSE ��ʾ�����ļ�����Ϊ TRUE ��Ϊ���ļ�����
		//NULL����ָ��ȱʡ����չ��
		//OFN_HIDEREADONLY ����ֻ����ѡ�� OFN_OVERWRITEPROMPT ����ļ��Ѵ���
		// ����ʾ�û��Ƿ񸲸�
	CFileDialog dlg(FALSE, NULL, strPath,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, &mRemoteDlg);
	if (dlg.DoModal() == IDOK) {
		mstrRemote = strPath;
		mstrLocal = dlg.GetPathName();
		//������д��ģʽ�򿪣��򴴽����ļ� 
		FILE* pFile = fopen(mstrLocal, "wb+");
		if (pFile == NULL) {
			AfxMessageBox("�ļ��޷�����!");
			return -1;
		}
		SendCommandPacket(mRemoteDlg, 4, false, (BYTE*)(LPCTSTR)mstrRemote, mstrRemote.GetLength(), (WPARAM)pFile);
		/*mhThreadDownload = (HANDLE)_beginthread(&CClientController::threadDownloadEntry, 0, this);
		if (WaitForSingleObject(mhThreadDownload, 0) != WAIT_TIMEOUT) {
			return -1;
		}*/
		mRemoteDlg.BeginWaitCursor();  //�������Ϊ�ȴ�״̬ 
		mStatusDlg.mDlgStatusInfo.SetWindowText(_T("������..."));
		mStatusDlg.ShowWindow(SW_SHOW);
		mStatusDlg.CenterWindow(&mRemoteDlg);
		mStatusDlg.SetActiveWindow(); //  ������ǰ̨
	}
	return 0;
}

void CClientController::StartWatchScreen()
{
	mIsClosed = false;
	//mWatchDlg.SetParent(&mRemoteDlg);
	//_beginthread ���ص����߳� ID �����������ں˾��
	// ���ʹ�� _beginthreadex
	mhThreadWatch = (HANDLE)_beginthread(&CClientController::threadWatchScreenEntry, 0, this);
	mWatchDlg.DoModal();  //�����Ի����ģ̬ѭ����ģ̬�Ի����������ǰ�̵߳�����������ֱ���û��رնԻ���
	mIsClosed = true;
	WaitForSingleObject(mhThreadWatch, 500);    //�ȴ���̨�߳������ 500 �����ڽ������У���������ǰ�߳� ֱ����̨�߳��˳���ʱ
	//�̺߳����ڲ���Ȼ������ _endthread()���̵߳�ʵ����ֹ����Ҳ�����ڲ���ϵͳ�ĵ��ȡ�
   //����Waitȷ���߳����㹻ʱ��������������߳���Դй©
}

void CClientController::threadWatchScreen()
{
	Sleep(50);   //ȷ������dlg.DoModal();��������
	while (!mIsClosed) {
		if (mWatchDlg.isFull() == false) {   //�������ݵ�����
			std::list<CPacket> lstPacks;
			int ret = SendCommandPacket(mWatchDlg.GetSafeHwnd(), true, NULL, 0);
			//�����Ϣ��Ӧ������ ���Ʒ���Ƶ��
			if (ret == 6) {
				if (CRemoteTool::Bytr2Image(mWatchDlg.GetImage(), lstPacks.front().strData) == 0) {
					mWatchDlg.SetImageStatus(true);
					TRACE("�ɹ�����ͼƬ! ���: %08X\r\n", (HBITMAP)mWatchDlg.GetImage());
					TRACE("��У��: %04X", lstPacks.front().sSum);
				}
				else {
					TRACE("��ȡͼƬʧ��! ret = %d\r\n", ret);
				}
			}
			else {
				TRACE("��ȡͼƬʧ��! ret = %d\r\n", ret);
			}
		}
		else Sleep(1);
	}
	TRACE("thread end %d\r\n", mIsClosed);
}

void CClientController::threadWatchScreenEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadWatchScreen();
	_endthread();
}

void CClientController::threadDownloadFile()
{
	//������д��ģʽ�򿪣��򴴽����ļ� 
	FILE* pFile = fopen(mstrLocal, "wb+");    
	if (pFile == NULL) {
		AfxMessageBox("�ļ��޷�����!");
		mStatusDlg.ShowWindow(SW_HIDE);
		mRemoteDlg.EndWaitCursor();
		return;
	}
	CClientSocket* pClient = CClientSocket::getInstance();
	do {
		int ret = SendCommandPacket(mRemoteDlg, 4, false,(BYTE*)(LPCTSTR)mstrRemote, mstrRemote.GetLength(), (WPARAM)pFile);
		long long nLength = *(long long*)pClient->GetPacket().strData.c_str();
		TRACE("client rev fileLength: %d\r\n", nLength);
		if (nLength == 0) {
			AfxMessageBox("�޷���ȡ�ļ�!");
			break;
		}
		long long nCnt = 0;
		while (nCnt < nLength) {
			ret = pClient->DealCommand();
			if (ret < 0) {
				AfxMessageBox("����ʧ��!");
				TRACE("����ʧ�� ret = %d\r\n", ret);
				break;
			}
			fwrite(pClient->GetPacket().strData.c_str(), 1, pClient->GetPacket().strData.size(), pFile);
			nCnt += pClient->GetPacket().strData.size();
		}
	} while (false);
	fclose(pFile);
	pClient->CloseSocket();
	mStatusDlg.ShowWindow(SW_HIDE);
	mRemoteDlg.EndWaitCursor();
	mRemoteDlg.MessageBox(_T("�������!"), _T("���"));
}

void CClientController::threadDownloadEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadDownloadFile();
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
			SetEvent(hEvent);   //TODO:
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
