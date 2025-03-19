// RemoteControl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteControl.h"
#include "ServerSocket.h"
#include "Command.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 唯一的应用程序对象

CWinApp theApp;

using namespace std;


int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
            CCommand cmd;
            //实例在main前已被初始化，这里申请一个指针来操作这个实例
            CServerSocket* pserver = CServerSocket::getInstance();  
            int cnt = 0;
            if (!pserver->InitSocket()) {
                MessageBox(NULL, _T("网络初始化异常，请检查网络设置"), _T("网络初始化失败!"), MB_OK | MB_ICONERROR);
                exit(0);
            }
            //CServerSocket::getInstance() 返回实例的指针
            while (CServerSocket::getInstance() != NULL) {  
                if (!pserver->AcceptClient()) {
                    if (cnt >= 3) {
                        MessageBox(NULL, _T("超时!"), _T("网络初始化失败!"), MB_OK | MB_ICONERROR);
                        exit(0);
                    }
                    MessageBox(NULL, _T("无法接入用户，自动重试"), _T("网络初始化失败!"), MB_OK | MB_ICONERROR);
                    cnt++;
                }
                TRACE("AcceptClient return ture\r\n");
                int ret = pserver->DealCommand();
                TRACE("DealCommand ret: %d\r\n", ret);
                if (ret > 0) {
                    ret = cmd.ExcuteCommand(ret);
                    if (ret != 0) {
                        TRACE("执行命令失败：%d ret = %d\r\n", pserver->GetPacket().sCmd, ret);
                    }
                    pserver->CloseClient();
                    TRACE("Command has done!\r\n");
                }
            }
        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}
