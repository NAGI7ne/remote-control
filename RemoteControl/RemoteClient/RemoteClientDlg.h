
// RemoteClientDlg.h: 头文件
//

#pragma once
#include "ClientSocket.h"
#include "StatusDlg.h"

#define WM_SEND_PACKET (WM_USER+1) //TODO:  (发送数据包消息①)

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
	// 1:查看磁盘分区    2:查看指定目录文件
	// 3:打开文件        4:下载文件  
	// 5:鼠标操作        6:发送屏幕内容
	// 7:锁机            8:解锁
	// 9:删除文件        39:测试连接
	// 返回值：命令， -1错误
	int SendCommandPacket(int nCmd, bool bAutoClose = true, BYTE* pData = NULL, size_t length = 0);
	CString GetPath(HTREEITEM hTree);
	void DeleteTreeChildrenItem(HTREEITEM hTree);
	void LoadFileInfo();
	void LaodFileCurrent();
	static void threadEntryForDownFile(void* arg);   //TODO:这里为什么是static
	void threadDownFile();
	static void threadEntryForWatchData(void* arg);  //静态函数不能用this指针
	void threadWatchFile();

private:
	CImage mImage;  //作图像缓存
	bool mImageIsFull;  //缓存是否有数据

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
	afx_msg void OnNMRClickList1File(NMHDR* pNMHDR, LRESULT* pResult);  //TODO：参数含义
	afx_msg void OnDownloadFile();
	afx_msg void OnDeleteFile();
	afx_msg void OnRunFile();
	afx_msg LRESULT OnSendPacket(WPARAM wParam, LPARAM lParam);  //TODO参数含义    (定义自定义消息响应函数②)
};
