﻿// CWatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "CWatchDialog.h"
#include "ClientController.h"


// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DLG_WATCH, pParent)
{
	mImageIsFull = false;
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
	ON_MESSAGE(WM_SEND_PACK_ACK, &CWatchDialog::OnSendPackAck)
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


CPoint CWatchDialog::UserPoint2RemotePoint(CPoint& point, bool isScreen)
{
	CRect clientRect;
	if(!isScreen)ClientToScreen(&point);   //转换为全局坐标（屏幕左上角）
	mPicture.ScreenToClient(&point);  //转换为相对picture的坐标（客户区域左上角）
	TRACE("x = %d y = %d\r\n", point.x, point.y);
	mPicture.GetWindowRect(clientRect);
	return CPoint(point.x * m_nObjWidth / clientRect.Width(), point.y * m_nObjHeight / clientRect.Height());
}

BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	mImageIsFull = false;
	//SetTimer(0, 30, NULL);   //设置了一个 ID 为 0 的定时器，定时器周期为 30 毫秒
							 //NULL 表示使用默认的消息回调方式
							 //即定时器消息将通过窗口消息 WM_TIMER 传递给该对话框的 OnTimer 成员函数。
							 //消息包含定时器的 ID:0
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CWatchDialog::OnTimer(UINT_PTR nIDEvent)   
{
	//if (nIDEvent == 0) {  //收到ID为0的定时器的消息
	//	CClientController* pParent = CClientController::getInstance();
	//	if (mImageIsFull) {
	//		CRect rect;
	//		mPicture.GetWindowRect(rect);
	//		m_nObjWidth = mImage.GetWidth();
	//		m_nObjHeight = mImage.GetHeight();
	//		//将图像缩放
	//		mImage.StretchBlt(mPicture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(),SRCCOPY);
	//		mPicture.InvalidateRect(NULL);  //将整个 mPicture 区域标记为“无效”，通知 Windows 系统需要重绘该控件
	//										//系统会在空闲时触发 WM_PAINT 消息，调用控件的重绘函数
	//										// 使刚刚绘制好的图像显示出来
	//		TRACE("更新图片完成%d %d %08X\r\n", m_nObjWidth, m_nObjHeight, (HBITMAP)mImage);
	//		mImage.Destroy();  //释放图像所占用的资源
	//		//image.SetImageStatus();
	//		mImageIsFull = false;
	//	}
	//}
	 
	CDialog::OnTimer(nIDEvent);  //调用基类的 OnTimer 可以确保消息得到完整的默认处理
}


LRESULT CWatchDialog::OnSendPackAck(WPARAM wParam, LPARAM lParam)
{
	if (lParam == -1 || (lParam == -2)) {
	//错误处理
	}
	else if (lParam == 1) {
		//对方关闭套接字
	}
	else {
		CPacket* pPacket = (CPacket*)wParam;
		if (pPacket != NULL) {
			CPacket head = *(CPacket*)wParam;
			delete (CPacket*)wParam;
			switch (head.sCmd) {
			case 6:
			{
				CRemoteTool::Bytr2Image(mImage, head.strData);
				CRect rect;
				mPicture.GetWindowRect(rect);
				m_nObjWidth = mImage.GetWidth();
				m_nObjHeight = mImage.GetHeight();
				//将图像缩放
				mImage.StretchBlt(mPicture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
				mPicture.InvalidateRect(NULL);  //将整个 mPicture 区域标记为“无效”，通知 Windows 系统需要重绘该控件
				//系统会在空闲时触发 WM_PAINT 消息，调用控件的重绘函数
				// 使刚刚绘制好的图像显示出来
				TRACE("更新图片完成%d %d %08X\r\n", m_nObjWidth, m_nObjHeight, (HBITMAP)mImage);
				mImage.Destroy();  //释放图像所占用的资源
				//image.SetImageStatus();
				break;
			}
			case 5:
				TRACE("远程端应答鼠标\r\n");
				break;
			case 7:
			case 8:
			default:
				break;
			}
		}
	}
	return 0;
}

void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if((m_nObjWidth != -1) && (m_nObjHeight != -1)){
		CPoint remote = UserPoint2RemotePoint(point);
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;  //左键
		event.nAction = 2;  //双击
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
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
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));


			/*CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
			pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);    */
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
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
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
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
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
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
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
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
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
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
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
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}
}


void CWatchDialog::OnOK()
{
	// TODO: 在此添加专用代码和/或调用基类

	//CDialog::OnOK();   // 禁用窗口Enter操作
}


void CWatchDialog::OnBnClickedBtnLock()
{
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 7);
}


void CWatchDialog::OnBnClickedBtnUnlock()
{
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 8);
}
