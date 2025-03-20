
// RemoteClientDlg.h: 头文件
//

#pragma once
#include "ClientSocket.h"
#include "StatusDlg.h"

#define WM_SEND_PACKET (WM_USER+1) //(发送数据包消息①)
								   //通过发送和接收消息进行内部通信
								   // 从 WM_USER 开始，Windows 预留了一段消息编号（通常定义为 0x0400）
								   // 给开发者用来定义自己的自定义消息，避免和系统的标准消息冲突
								   
// CRemoteClientDlg 对话框
class CRemoteClientDlg : public CDialogEx
{
// 构造
public:
	CRemoteClientDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REMOTECLIENT_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

private:
	
	CString GetPath(HTREEITEM hTree);
	void DeleteTreeChildrenItem(HTREEITEM hTree);
	void LoadFileInfo();
	void LaodFileCurrent();
	//static void threadEntryForDownFile(void* arg);   //TODO:这里为什么是static
											   //线程入口函数必须是一个普通函数
											   //非静态成员函数，它会默认带有一个隐藏的 this 指针
											   //使成员函数无法直接作为线程入口函数使用，因为不符合预期的函数原型
											   //还可以限定该函数的作用域,避免不必要的外部访问
	//void threadDownFile();
	//static void threadEntryForWatchData(void* arg);  //静态函数不能用this指针
	//void threadWatchData();

private:
	CImage mImage;  //作图像缓存
	bool mImageIsFull;  //缓存是否有数据
	bool mIsThreadClosed;  //监视线程是否关闭
public:
	bool isFull() const { return mImageIsFull; }
	CImage& GetImage() { return mImage; }
	void SetImageStatus(bool isFull = false) { mImageIsFull = isFull; }

// 实现
protected:
	HICON m_hIcon;
	CStatusDlg mDlgStatus;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtn1Test();
	DWORD mServAddr;
	CString mNport;
	afx_msg void OnBnClickedBtnFileinfo();
	CTreeCtrl mTree;
	afx_msg void OnNMDblclkTree1Dir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMClickTree1Dir(NMHDR* pNMHDR, LRESULT* pResult);
	// 显示文件
	CListCtrl mList;
	afx_msg void OnNMRClickList1File(NMHDR* pNMHDR, LRESULT* pResult);  
	afx_msg void OnDownloadFile();
	afx_msg void OnDeleteFile();
	afx_msg void OnRunFile();
	afx_msg LRESULT OnSendPacket(WPARAM wParam, LPARAM lParam);  //定义自定义消息响应函数②
	afx_msg void OnBnClickedBtnStartWatch();
	afx_msg void OnIpnFieldchangedIpaddress2Serv(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEnChangeEdit1Port();
};
