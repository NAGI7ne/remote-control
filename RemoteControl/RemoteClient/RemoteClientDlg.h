
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
	int SendCommendPacket(int nCmd, BYTE* pData = NULL, size_t length = 0) {
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
		int cmd = pClient->DealCommand();
		TRACE("ack: %d\r\n", cmd);
		pClient->CloseSocket();
		return cmd;
	}
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
};
