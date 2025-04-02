// CWatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"


// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DLG_WATCH, pParent)
{
	m_nObjWidth = -1;
	m_nObjHeight = -1;
}

CWatchDialog::~CWatchDialog()
{
}

void CWatchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, mPicture);
}


BEGIN_MESSAGE_MAP(CWatchDialog, CDialog)
	ON_WM_TIMER()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_STN_CLICKED(IDC_WATCH, &CWatchDialog::OnStnClickedWatch)
	ON_BN_CLICKED(IDC_BTN_LOCK, &CWatchDialog::OnBnClickedBtnLock)
	ON_BN_CLICKED(IDC_BTN_UNLOCK, &CWatchDialog::OnBnClickedBtnUnlock)
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


CPoint CWatchDialog::UserPoint2RemotePoint(CPoint& point, bool isScreen)
{
	CRect clientRect;
	if(isScreen) ScreenToClient(&point);  //全局坐标到客户区坐标
	TRACE("x = %d y = %d\r\n", point.x, point.y);
	mPicture.GetWindowRect(clientRect);
	return CPoint(point.x * m_nObjWidth / clientRect.Width(), point.y * m_nObjHeight / clientRect.Height());
}

BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	SetTimer(0, 30, NULL);   //设置了一个 ID 为 0 的定时器，定时器周期为 30 毫秒
							 //NULL 表示使用默认的消息回调方式
							 //即定时器消息将通过窗口消息 WM_TIMER 传递给该对话框的 OnTimer 成员函数。
							 //消息包含定时器的 ID:0
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CWatchDialog::OnTimer(UINT_PTR nIDEvent)   
{
	if (nIDEvent == 0) {  //收到ID为0的定时器的消息
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		if (pParent->isFull()) {
			CRect rect;
			mPicture.GetWindowRect(rect);
			//pParent->GetImage().BitBlt(mPicture.GetDC()->GetSafeHdc(), 0, 0, SRCCOPY);
			if (m_nObjWidth == -1) m_nObjWidth = pParent->GetImage().GetWidth();
			if (m_nObjHeight == -1) m_nObjHeight = pParent->GetImage().GetHeight();
			//将图像缩放
			pParent->GetImage().StretchBlt(mPicture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(),SRCCOPY);
			mPicture.InvalidateRect(NULL);  //将整个 mPicture 区域标记为“无效”，通知 Windows 系统需要重绘该控件
											//系统会在空闲时触发 WM_PAINT 消息，调用控件的重绘函数
											// 使刚刚绘制好的图像显示出来
			pParent->GetImage().Destroy();  //释放图像所占用的资源
			pParent->SetImageStatus();
		}
	}
	 
	CDialog::OnTimer(nIDEvent);  //调用基类的 OnTimer 可以确保消息得到完整的默认处理
}


void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if((m_nObjWidth != -1) && (m_nObjHeight != -1)){
		CPoint remote = UserPoint2RemotePoint(point);
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;  //左键
		event.nAction = 1;  //双击
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	}
	CDialog::OnLButtonDblClk(nFlags, point);
}


void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		TRACE("x = %d y = %d\r\n", point.x, point.y);
		CPoint remote = UserPoint2RemotePoint(point);
		TRACE("x = %d y = %d\r\n", remote.x, remote.y);
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;  //左键
		event.nAction = 2;  //按下
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);    
		// 要设置的值是 5，true 所以 或上 1
		// 5 表示为 101。左移 1 位后变成 1010，也就是十进制的 10
		//再与 0001 按位或，结果为 1011，等于 11。
		//更进一步见 RemoteClientDlg 的 CRemoteClientDlg::OnSendPacket
	}
	CDialog::OnLButtonDown(nFlags, point);
}


void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		CPoint remote = UserPoint2RemotePoint(point);
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;  //左键
		event.nAction = 3;  //弹起
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	}
	CDialog::OnLButtonUp(nFlags, point);
}


void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		CPoint remote = UserPoint2RemotePoint(point);
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;  //右键
		event.nAction = 1;  //双击
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	}
	CDialog::OnRButtonDblClk(nFlags, point);
}


void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		CPoint remote = UserPoint2RemotePoint(point);
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;  //右键
		event.nAction = 2;  //按下
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	}
	CDialog::OnRButtonDown(nFlags, point);
}


void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		CPoint remote = UserPoint2RemotePoint(point);
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;  //右键
		event.nAction = 3;  //弹起
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	}
	CDialog::OnRButtonUp(nFlags, point);
}


void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		//对话框的point是局部坐标
		CPoint remote = UserPoint2RemotePoint(point);
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 8;
		event.nAction = 0;  //移动
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	}
	CDialog::OnMouseMove(nFlags, point);
}


void CWatchDialog::OnStnClickedWatch()
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		CPoint point;
		GetCursorPos(&point);  //全局坐标
		CPoint remote = UserPoint2RemotePoint(point, true);
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;  //左键
		event.nAction = 0;  //单击
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
	}
}


void CWatchDialog::OnOK()
{
	// TODO: 在此添加专用代码和/或调用基类

	//CDialog::OnOK();   // 禁用窗口Enter操作
}


void CWatchDialog::OnBnClickedBtnLock()
{
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 7 << 1 | 1);
}


void CWatchDialog::OnBnClickedBtnUnlock()
{
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 8 << 1 | 1);
}
