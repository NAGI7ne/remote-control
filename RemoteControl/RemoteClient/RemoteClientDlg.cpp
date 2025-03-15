
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

int CRemoteClientDlg::SendCommendPacket(int nCmd, bool bAutoClose, BYTE* pData, size_t length)
{
	UpdateData();
	CClientSocket* pClient = CClientSocket::getInstance();
	bool ret = pClient->InitSocket(mServAddr, atoi((LPCTSTR)mNport));
	if (ret == NULL) {
		AfxMessageBox(_T("网络初始化失败!"));
		return -1;
	}
	CPacket pack(nCmd, pData, length);
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
		strRet = strTmp + "//" + strRet;
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
	int nCmd = SendCommendPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	CClientSocket* pClient = CClientSocket::getInstance();
	while (pInfo->HasNext) {
		TRACE("file[%s] Isdir : %d", pInfo->szFileName, pInfo->IsDirectory);
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

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN1_TEST, &CRemoteClientDlg::OnBnClickedBtn1Test)
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE1_DIR, &CRemoteClientDlg::OnNMDblclkTree1Dir)
	ON_NOTIFY(NM_CLICK, IDC_TREE1_DIR, &CRemoteClientDlg::OnNMClickTree1Dir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST1_FILE, &CRemoteClientDlg::OnNMRClickList1File)
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
	SendCommendPacket(39);

}


void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	int ret = SendCommendPacket(1);
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
	CPoint ptMouse, ptList;
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
