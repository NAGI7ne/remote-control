
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
#include "ClientController.h"


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
	int nCmd = CClientController::getInstance()->SendCommandPacket(2, false, 
		(BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	while (pInfo->HasNext) {
		TRACE("file[%s] Isdir : %d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (pInfo->IsDirectory) {    //处理"."和".."
			if ((CString)pInfo->szFileName == "." || (CString)pInfo->szFileName == "..") {
				int cmd = CClientController::getInstance()->DealCommand();
				TRACE("ack : %d\r\n", cmd);
				if (cmd < 0) break;
				continue;
			}
			HTREEITEM hTemp = mTree.InsertItem(pInfo->szFileName, hTreeSelected, TVI_LAST);
			mTree.InsertItem("", hTemp, TVI_LAST);
		}
		else {
			mList.InsertItem(0, pInfo->szFileName);  //0：列表的第 0 行（第一行）插入
		}
		
		int cmd = CClientController::getInstance()->DealCommand();
		TRACE("ack : %d\r\n", cmd);
		if (cmd < 0) break;
	}
	CClientController::getInstance()->CloseSocket();
}

void CRemoteClientDlg::LaodFileCurrent()
{
	HTREEITEM hTree = mTree.GetSelectedItem();
	CString strPath = GetPath(hTree);
	mList.DeleteAllItems();
	int nCmd = CClientController::getInstance()->SendCommandPacket(2, false, 
		(BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	int cnt = 0;
	while (pInfo->HasNext) {
		TRACE("file[%s] Isdir : %d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (!pInfo->IsDirectory) {    //处理"."和".."
			mList.InsertItem(0, pInfo->szFileName);  
		}
		int cmd = CClientController::getInstance()->DealCommand();
		TRACE("ack : %d\r\n", cmd);
		if (cmd < 0) break;
	}
	CClientController::getInstance()->CloseSocket();
	TRACE("Count = %d\r\n", cnt);
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
	//ON_MESSAGE(WM_SEND_PACKET, &CRemoteClientDlg::OnSendPacket)   //注册消息 ③
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_IPADDRESS2_SERV, &CRemoteClientDlg::OnIpnFieldchangedIpaddress2Serv)
	ON_EN_CHANGE(IDC_EDIT1_PORT, &CRemoteClientDlg::OnEnChangeEdit1Port)
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

	
	UpdateData();  //从对话框上的控件（例如编辑框、复选框等）中提取当前的内容，并把这些数据写入关联的成员变量中。
	mServAddr = 0xC0A8A084;  //192.168.160.132
	//mServAddr = 0x7F000001;  //127.0.0.1
	mNport = _T("9339");
	CClientController* pController = CClientController::getInstance();
	pController->UpdataAddress(mServAddr, atoi((LPCTSTR)mNport));
	UpdateData(FALSE);   //FALSE 时,UpdateData 则会把成员变量的值更新到对话框控件上（通常用于在对话框初始化时显示默认值）。
	mDlgStatus.Create(IDD_DLG_STATUS, this);  
	mDlgStatus.ShowWindow(SW_HIDE);
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
	CClientController::getInstance()->SendCommandPacket(39);
}


void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	int ret = CClientController::getInstance()->SendCommandPacket(1);
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
	GetCursorPos(&ptMouse); //鼠标当前在屏幕上的坐标
	ptList = ptMouse;
	mList.ScreenToClient(&ptList);  //将屏幕坐标转换为相对于列表控件（mList）客户端区域的坐标
	int ListSelected = mList.HitTest(ptList);  //判断点击的位置是否落在某个列表项内
	if (ListSelected < 0) return;
	CMenu menu;
	menu.LoadMenu(IDR_MENU1_RCLICK);
	CMenu* pPupup = menu.GetSubMenu(0);    //取得菜单中的第一个子菜单
	if (pPupup != NULL) {
		pPupup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);  //指定菜单项的对齐方式以及用右键来做菜单的选中操作。

	}
}


void CRemoteClientDlg::OnDownloadFile()
{
	int nListSelected = mList.GetSelectionMark();
	CString strFile = mList.GetItemText(nListSelected, 0);
	HTREEITEM hSelected = mTree.GetSelectedItem();
	strFile = GetPath(hSelected) + strFile;
	int ret = CClientController::getInstance()->DownFile(strFile);
	if (ret != 0) {
		MessageBox(_T("下载失败！"));
		TRACE("下载失败 ret = %d\r\n", ret);
	}
}


void CRemoteClientDlg::OnDeleteFile()
{
	HTREEITEM hSelected = mTree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = mList.GetSelectionMark();
	CString strFile = mList.GetItemText(nSelected, 0);
	strFile = strPath + strFile;
	int ret = CClientController::getInstance()->SendCommandPacket(9, true,
		(BYTE*)(LPCTSTR)strFile, strFile.GetLength());
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
	int ret = CClientController::getInstance()->SendCommandPacket(3, true, 
		(BYTE*)(LPCTSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox("打开文件失败");
	}
}

void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	CClientController::getInstance()->StartWatchScreen();			
}


void CRemoteClientDlg::OnIpnFieldchangedIpaddress2Serv(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	UpdateData();  //在对话框的控件和类的成员变量之间进行数据同步
	CClientController* pController = CClientController::getInstance();
	pController->UpdataAddress(mServAddr, atoi((LPCTSTR)mNport));
}


void CRemoteClientDlg::OnEnChangeEdit1Port()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData();  //在对话框的控件和类的成员变量之间进行数据同步
	CClientController* pController = CClientController::getInstance();
	pController->UpdataAddress(mServAddr, atoi((LPCTSTR)mNport));
}
