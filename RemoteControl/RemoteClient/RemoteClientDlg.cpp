
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include "CWatchDialog.h"


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRemoteClientDlg 对话框



CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, mServAddr(0)
	, mNport(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS2_SERV, mServAddr);
	DDX_Text(pDX, IDC_EDIT1_PORT, mNport);
	DDX_Control(pDX, IDC_TREE1_DIR, mTree);
	DDX_Control(pDX, IDC_LIST1_FILE, mList);
}

int CRemoteClientDlg::SendCommandPacket(int nCmd, bool bAutoClose, BYTE* pData, size_t length)
{
	UpdateData();  //TODO
	CClientSocket* pClient = CClientSocket::getInstance();
	bool ret = pClient->InitSocket(mServAddr, atoi((LPCTSTR)mNport));
	if (ret == NULL) {
		AfxMessageBox(_T("网络初始化失败!"));
		return -1;
	}
	CPacket pack(nCmd, pData, length);
	//TRACE("send file: %s\r\n", (const char*)pData);
	int rst = pClient->Send(pack);
	TRACE("send rst: %d\r\n", rst);
	int cmd = pClient->DealCommand();  //发送包后进入收包状态
	TRACE("ack: %d\r\n", cmd);
	if(bAutoClose) pClient->CloseSocket();
	return cmd;
}

CString CRemoteClientDlg::GetPath(HTREEITEM hTree)
{
	CString strRet, strTmp;
	do {
		strTmp = mTree.GetItemText(hTree);
		strRet = strTmp + '\\' + strRet;
		hTree = mTree.GetParentItem(hTree);
	} while (hTree != NULL);
	return strRet;
}

void CRemoteClientDlg::DeleteTreeChildrenItem(HTREEITEM hTree)
{
	HTREEITEM hSub = NULL;
	do {
		hSub = mTree.GetChildItem(hTree);
		if (hSub != NULL) mTree.DeleteItem(hSub);
	} while (hSub != NULL);
}

void CRemoteClientDlg::LoadFileInfo()
{
	CPoint ptMouse;
	GetCursorPos(&ptMouse);
	mTree.ScreenToClient(&ptMouse);
	HTREEITEM hTreeSelected = mTree.HitTest(ptMouse, 0);
	if (hTreeSelected == NULL) return;
	if (mTree.GetChildItem(hTreeSelected) == NULL) return;
	DeleteTreeChildrenItem(hTreeSelected);
	mList.DeleteAllItems();
	CString strPath = GetPath(hTreeSelected);
	int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	CClientSocket* pClient = CClientSocket::getInstance();
	while (pInfo->HasNext) {
		TRACE("file[%s] Isdir : %d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (pInfo->IsDirectory) {    //处理"."和".."
			if ((CString)pInfo->szFileName == "." || (CString)pInfo->szFileName == "..") {
				int cmd = pClient->DealCommand();
				TRACE("ack : %d\r\n", cmd);
				if (cmd < 0) break;
				continue;
			}
			HTREEITEM hTemp = mTree.InsertItem(pInfo->szFileName, hTreeSelected, TVI_LAST);
			mTree.InsertItem("", hTemp, TVI_LAST);
		}
		else {
			mList.InsertItem(0, pInfo->szFileName);   //TODO:什么意思
		}
		
		int cmd = pClient->DealCommand();
		TRACE("ack : %d\r\n", cmd);
		if (cmd < 0) break;
	}
	pClient->CloseSocket();
}

void CRemoteClientDlg::LaodFileCurrent()
{
	HTREEITEM hTree = mTree.GetSelectedItem();
	CString strPath = GetPath(hTree);
	mList.DeleteAllItems();
	int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	CClientSocket* pClient = CClientSocket::getInstance();
	int cnt = 0;
	while (pInfo->HasNext) {
		TRACE("file[%s] Isdir : %d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (!pInfo->IsDirectory) {    //处理"."和".."
			mList.InsertItem(0, pInfo->szFileName);   //TODO:什么意思
		}
		int cmd = pClient->DealCommand();
		TRACE("ack : %d\r\n", cmd);
		if (cmd < 0) break;
	}
	pClient->CloseSocket();
	TRACE("Count = %d\r\n", cnt);
}

void CRemoteClientDlg::threadEntryForDownFile(void* arg)
{
	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;
	thiz->threadDownFile();
	_endthread();
}

void CRemoteClientDlg::threadDownFile()
{
	int nListSelected = mList.GetSelectionMark();
	CString strFile = mList.GetItemText(nListSelected, 0);
	CFileDialog dlg(FALSE, NULL, strFile,  //TODO:
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, this);
	if (dlg.DoModal() == IDOK) {
		FILE* pFile = fopen(dlg.GetPathName(), "wb+");    //TODO:
		if (pFile == NULL) {
			AfxMessageBox("文件无法创建!");
			mDlgStatus.ShowWindow(SW_HIDE);
			EndWaitCursor();
			return;
		}
		HTREEITEM hSelected = mTree.GetSelectedItem();
		strFile = GetPath(hSelected) + strFile;
		TRACE("strFile : %s\r\n", LPCSTR(strFile));
		CClientSocket* pClient = CClientSocket::getInstance();
		do {
			//int ret = SendCommandPacket(4, false, (BYTE*)(LPCTSTR)strFile, strFile.GetLength());
			int ret = SendMessage(WM_SEND_PACKET, 4 << 1 | 0, (LPARAM)(LPCTSTR)strFile); //TODO:
			if (ret < 0) {
				AfxMessageBox("执行下载命令失败!");
				TRACE("下载失败 ret = %d\r\n", ret);
				break;
			}
			long long nLength = *(long long*)pClient->GetPacket().strData.c_str();
			TRACE("client rev fileLength: %d\r\n", nLength);
			if (nLength == 0) {
				AfxMessageBox("无法读取文件!");
				break;
			}
			long long nCnt = 0;
			while (nCnt < nLength) {
				ret = pClient->DealCommand();
				if (ret < 0) {
					AfxMessageBox("传输失败!");
					TRACE("传输失败 ret = %d\r\n", ret);
					break;
				}
				fwrite(pClient->GetPacket().strData.c_str(), 1, pClient->GetPacket().strData.size(), pFile);
				nCnt += pClient->GetPacket().strData.size();
			}
		} while (0);
		fclose(pFile);
		pClient->CloseSocket();
	}
	mDlgStatus.ShowWindow(SW_HIDE);
	EndWaitCursor();
	MessageBox(_T("下载完成!"), _T("完成"));
}

void CRemoteClientDlg::threadEntryForWatchData(void* arg)
{
	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;
	thiz->threadWatchFile();
	_endthread();
}

void CRemoteClientDlg::threadWatchFile()
{
	Sleep(50);   //确保晚于dlg.DoModal();窗口启动
	CClientSocket* pClient = NULL;
	do {
		pClient = CClientSocket::getInstance();    //确保能拿到实体
	} while (pClient == NULL);
	ULONGLONG tick = GetTickCount64();
	for (;;) {
		if (mImageIsFull == false) {   //更新数据到缓存
			int ret = SendMessage(WM_SEND_PACKET, 6 << 1 | 1); //TODO:
			if (ret == 6) {
				BYTE* pData = (BYTE*)pClient->GetPacket().strData.c_str();
				HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
				if (hMem == NULL) {
					TRACE("内存不足!");
					Sleep(1);
					continue;
				}
				IStream* pStream = NULL;
				HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
				if (hRet == S_OK) {
					ULONG length = 0;
					pStream->Write(pData, pClient->GetPacket().Size(), &length);
					LARGE_INTEGER bg{ 0 };   // TODO:
					pStream->Seek(bg, STREAM_SEEK_SET, NULL);
					mImage.Load(pStream);
					mImageIsFull = true;
				}
			}
			else {
				Sleep(1);     //确保如果发送失败不会占用太多cpu资源
			}
		}
		else Sleep(1);
	}
}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN1_TEST, &CRemoteClientDlg::OnBnClickedBtn1Test)
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE1_DIR, &CRemoteClientDlg::OnNMDblclkTree1Dir)
	ON_NOTIFY(NM_CLICK, IDC_TREE1_DIR, &CRemoteClientDlg::OnNMClickTree1Dir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST1_FILE, &CRemoteClientDlg::OnNMRClickList1File)
	ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)
	ON_COMMAND(ID_DELETE_FILE, &CRemoteClientDlg::OnDeleteFile)
	ON_COMMAND(ID_RUN_FILE, &CRemoteClientDlg::OnRunFile)
	ON_MESSAGE(WM_SEND_PACKET, &CRemoteClientDlg::OnSendPacket)   //注册消息 ③
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
END_MESSAGE_MAP()


// CRemoteClientDlg 消息处理程序

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	//从对话框上的控件（例如编辑框、复选框等）中提取当前的内容，并把这些数据写入关联的成员变量中。
	UpdateData();  
	mServAddr = 0x7F000001;
	mNport = _T("9339");
	//FALSE 时,UpdateData 则会把成员变量的值更新到对话框控件上（通常用于在对话框初始化时显示默认值）。
	UpdateData(FALSE);
	mDlgStatus.Create(IDD_DLG_STATUS, this);   //TODO：这里this是什么
	mDlgStatus.ShowWindow(SW_HIDE);
	mImageIsFull = false;
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CRemoteClientDlg::OnBnClickedBtn1Test()
{
	SendCommandPacket(39);

}


void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	int ret = SendCommandPacket(1);
	if (ret == -1) {
		AfxMessageBox(_T("命令处理失败"));
		return;
	}
	CClientSocket* pClient = CClientSocket::getInstance();
	std::string drivers = pClient->GetPacket().strData;
	std::string dr;
	mTree.DeleteAllItems();
	HTREEITEM hTemp{};
	for (int i = 0; i < drivers.size(); i++) {
		if (drivers[i] == ',') {
			dr += ':';
			hTemp = mTree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);  //加入到根目录，追加形式
			mTree.InsertItem("", hTemp, TVI_LAST);
			dr.clear();
			continue;
		}
		dr += drivers[i];
	} 
	dr += ':';
	hTemp = mTree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);  //加入到根目录，追加形式
	mTree.InsertItem("", hTemp, TVI_LAST);
	dr.clear();

}

void CRemoteClientDlg::OnNMDblclkTree1Dir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMClickTree1Dir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMRClickList1File(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	CPoint ptMouse, ptList;      //TODO:函数作用了解
	GetCursorPos(&ptMouse);
	ptList = ptMouse;
	mList.ScreenToClient(&ptList);
	int ListSelected = mList.HitTest(ptList);
	if (ListSelected < 0) return;
	CMenu menu;
	menu.LoadMenu(IDR_MENU1_RCLICK);
	CMenu* pPupup = menu.GetSubMenu(0);    
	if (pPupup != NULL) {
		pPupup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);
	}
}


void CRemoteClientDlg::OnDownloadFile()
{
	//添加线程函数
	_beginthread(CRemoteClientDlg::threadEntryForDownFile, 0, this);
	BeginWaitCursor();  //设置鼠标为等待状态
	mDlgStatus.mDlgStatusInfo.SetWindowText(_T("下载中..."));
	mDlgStatus.ShowWindow(SW_SHOW);
	mDlgStatus.CenterWindow(this);
	mDlgStatus.SetActiveWindow(); //  激活至前台
	//Sleep(50); //保证线程能正确开启
}


void CRemoteClientDlg::OnDeleteFile()
{
	HTREEITEM hSelected = mTree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = mList.GetSelectionMark();
	CString strFile = mList.GetItemText(nSelected, 0);
	strFile = strPath + strFile;
	int ret = SendCommandPacket(9, true, (BYTE*)(LPCTSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox("删除文件失败");
	}
	LaodFileCurrent();
}


void CRemoteClientDlg::OnRunFile()
{
	HTREEITEM hSelected = mTree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = mList.GetSelectionMark();
	CString strFile = mList.GetItemText(nSelected, 0);
	strFile = strPath + strFile;
	int ret = SendCommandPacket(3, true, (BYTE*)(LPCTSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox("打开文件失败");
	}
}

LRESULT CRemoteClientDlg::OnSendPacket(WPARAM wParam, LPARAM lParam)   //实现消息函数④
{
	int ret = 0;
	int cmd = wParam >> 1;
	switch (cmd)
	{
	case 4:
		{
			CString strFile = (LPCTSTR)lParam;
			int ret = SendCommandPacket(cmd, wParam & 1, (BYTE*)(LPCTSTR)strFile, strFile.GetLength());  //TODO
		}
		break;
	case 6:
		{
			ret = SendCommandPacket(cmd, wParam & 1);
		}
		break;
	default:
		ret = -1;
		//break;
	}
	
	return  ret;
}


void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	CWatchDialog dlg(this);
	_beginthread(CRemoteClientDlg::threadEntryForWatchData, 0, this);
	dlg.DoModal();  //TODO:
}
