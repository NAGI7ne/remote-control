// LockInfoDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteControl.h"
#include "afxdialogex.h"
#include "LockInfoDialog.h"


// CLockInfoDialog 对话框

IMPLEMENT_DYNAMIC(CLockInfoDialog, CDialogEx)

CLockInfoDialog::CLockInfoDialog(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG1_INFO, pParent)
{

}

CLockInfoDialog::~CLockInfoDialog()
{
}

void CLockInfoDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CLockInfoDialog, CDialogEx)
END_MESSAGE_MAP()


// CLockInfoDialog 消息处理程序
