
// RemoteClientDlg.h: 头文件
//

#pragma once
#include "ClientSocket.h"

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
	// 1查看磁盘分区  2查看指定目录文件
	// 3打开文件      4下载文件
	// 返回值：命令， -1错误
	int SendCommendPacket(int nCmd, bool bAutoClose = true, BYTE* pData = NULL, size_t length = 0);
	CString GetPath(HTREEITEM hTree);
	void DeleteTreeChildrenItem(HTREEITEM hTree);
	void LoadFileInfo();
// 实现
protected:
	HICON m_hIcon;

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
};
