﻿// RemoteControl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteControl.h"
#include "ServerSocket.h"
#include <direct.h>
#include <atlimage.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;

void Dump(BYTE* pData, size_t nSize) {
    string strOut;
    for (size_t i = 0; i < nSize; i++) {
        char buf[8]{};
        if (i > 0 && (i % 16 == 0)) strOut += "\n";
        snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF); 
        strOut += buf;
    }
    strOut += "\n";
    OutputDebugStringA(strOut.c_str());  
}

int MakeDriverInfo() {
    std::string result;
    for (int i = 1; i <= 26; i++) {
        if (_chdrive(i) == 0) {
            if (result.size() > 0)
                result += ',';
            result += 'A' + i - 1;
        }
    }
    CPacket pack(1, (BYTE*)result.c_str(), result.size());
    Dump((BYTE*)pack.Data(), pack.Size());
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

#include <io.h>
#include <stdio.h>
#include <list>

int MakeDirectoryInfo() {
    std::string strPath;
    //std::list<FILEINFO> lstFileInfos;
    if (CServerSocket::getInstance()->GetFilePath(strPath) == false) {
        OutputDebugString(_T("不是获取文件命令,命令解析错误！"));
        return -1;
    }
    if (_chdir(strPath.c_str()) != 0) {
        FILEINFO finfo;
        finfo.HasNext = FALSE;
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
        OutputDebugString(_T("无法打开文件！请检查权限"));
        return -2;
    }
    _finddata_t fdata;
    intptr_t hfind = 0;
    if ((hfind = _findfirst("*", &fdata)) == -1) {
        OutputDebugString(_T("没有找到文件！"));
        FILEINFO finfo;
        finfo.HasNext = FALSE;
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
        return -3;
    }
    int count = 0;
    do {
        FILEINFO finfo;
        finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
        memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
        TRACE("FileName :%s\r\n", finfo.szFileName);
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        //Dump((BYTE*)pack.Data(), pack.Size());
        //Sleep(5);  //确保数据能正常被服务器收到
        CServerSocket::getInstance()->Send(pack);
        count++;
    } while (!_findnext(hfind, &fdata));
    TRACE("server: count = %d\r\n", count);
    FILEINFO finfo;
    finfo.HasNext = FALSE;
    CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int RunFile() {
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    //打开路径为 strPath 的文件,以正常窗口状态显示应用程序
    ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    CPacket pack(3, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int DownloadFile() {
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    //TRACE("file: %s", strPath.c_str());
    long long data = 0;
    FILE* pFile = NULL;
    errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
    if (err != 0) {
        CPacket pack(4, (BYTE*)&data, 8);
        CServerSocket::getInstance()->Send(pack);
        return -1;
    }
    if (pFile != NULL) {
        fseek(pFile, 0, SEEK_END);
        data = _ftelli64(pFile);
        CPacket head(4, (BYTE*)&data, 8);
        CServerSocket::getInstance()->Send(head);
        fseek(pFile, 0, SEEK_SET);
        char buffer[1024]{};
        size_t rlen = 0;
        do {
            rlen = fread(buffer, 1, 1024, pFile);
            TRACE("server send file : %d\r\n", rlen);
            CPacket pack(4, (BYTE*)buffer, rlen);
            //Sleep(5);
            CServerSocket::getInstance()->Send(pack);
        } while (rlen >= 1024);
        fclose(pFile);
    }
    CPacket pack(4, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int MouseEvent() {
    MOUSEEV mouse;
    if (CServerSocket::getInstance()->GetMouseEvent(mouse)) {
        DWORD nFlags = 0;
        switch (mouse.nButton){
        case 0:   //左键
            nFlags = 0x01;
            break;
        case 1:  //右键
            nFlags = 0x02;
            break;
        case 2:  //中键
            nFlags = 0x04;
            break;
        case 4:  //没有按键
            nFlags = 0x08;
            break;
        }
        if (nFlags != 8) SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
        switch (mouse.nAction) {
        case 0:  //单击
            nFlags |= 0x10;
            break;
        case 1:  //双击
            nFlags |= 0x20;
            break;
        case 2:  //按下
            nFlags |= 0x40;
            break;
        case 3:  //放开
            nFlags |= 0x80;
            break;
        default:
            break;
        }
        TRACE("mouse event : %08X x %d y %d\r\n", nFlags, mouse.ptXY.x, mouse.ptXY.y);
        switch (nFlags) {
        case 0x21:  //左键双击
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x11:  //左键单击
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x41:  //左键按下
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x81:  //左键放开
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x22:  //右键双击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x12:  //右键单击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;  
        case 0x42:  //右键按下
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x82:  //右键放开
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x24:  //中键双击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x14:  //中键单击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x44:  //中键按下
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x84:  //中键放开
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x08:  //鼠标移动
            mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
            break;
        }
        CPacket pack(4, NULL, 0);
        CServerSocket::getInstance()->Send(pack);
    }
    else {
        OutputDebugString(_T("获取鼠标操作参数失败！"));
        return -1;
    }

    return 0;
}

int SendScreen() {
    CImage screen;
    HDC hScreen = ::GetDC(NULL);    //获取屏幕上下文
    int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL); //获取屏幕每个像素的位数（颜色深度）
    int nWidth = GetDeviceCaps(hScreen, HORZRES);
    int nHeight = GetDeviceCaps(hScreen, VERTRES);
    screen.Create(nWidth, nHeight, nBitPerPixel); //为 screen 分配了与屏幕尺寸和色深匹配的图像缓冲区。
    BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight,hScreen, 0, 0, SRCCOPY); //将屏幕上的内容复制到 screen 对应的设备上下文中
    ReleaseDC(NULL, hScreen);
    //需要把屏幕数据使用流的形式存入内存
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);//分配一个可移动的全局内存块（句柄）
    if (hMem == NULL) return -1;
    IStream* pStream = NULL;
    HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream); //将分配的全局内存包装成一个流对象（此函数需要可移动的全局内存块）
    if (ret == S_OK) {
        screen.Save(pStream, Gdiplus::ImageFormatJPEG); //将图像保存到流中
        LARGE_INTEGER bg{ 0 };
        pStream->Seek(bg, STREAM_SEEK_SET, NULL);//Save写完后流的内部指针通常会位于数据的末尾, 需要从头开始
        BYTE* pData = (BYTE*)GlobalLock(hMem);
        SIZE_T nSize = GlobalSize(hMem);
        CPacket pack(6, pData, nSize);
        CServerSocket::getInstance()->Send(pack);
        GlobalUnlock(hMem);
    }
    /*screen.Save(_T("test.jpg"), Gdiplus::ImageFormatJPEG);
    DWORD tick = GetTickCount64();
    TRACE("jpg %d\r\n", GetTickCount64() - tick);*/
    pStream->Release();
    GlobalFree(hMem);
    screen.ReleaseDC();
    return 0;
}

#include "LockInfoDialog.h"
CLockInfoDialog dlg;  //需要多次调用，用全局变量
unsigned threadid = 0;

unsigned __stdcall threadLockDlg(void* arg) {
    TRACE("%s (%d) : %d\r\n", __FUNCTION__, __LINE__, GetCurrentThreadId());
    dlg.Create(IDD_DIALOG1_INFO, NULL);
    dlg.ShowWindow(SW_SHOW);
    //dlg.CenterWindow();
    CRect rect;
    //rect.left = 0;
    //rect.top = 0;
    //rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
    //rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);  //获取整个屏幕
    //rect.bottom = LONG(rect.bottom * 1.10);
    //TRACE("right = %d bottom = %d\r\n", rect.right, rect.bottom);
    //dlg.MoveWindow(rect);
    //CWnd* pText = dlg.GetDlgItem(IDC_STATIC);
    //if (pText) {
    //    CRect rtText;
    //    pText->GetWindowRect(rtText);
    //    int nWidth = rtText.Width();
    //    int x = (rect.right - nWidth) / 2;
    //    int nHeight = rtText.Height();
    //    int y = (rect.bottom - nHeight) / 2;
    //    pText->MoveWindow(x, y, rtText.Width(), rtText.Height());
    //}

    dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);  //窗口置顶，设置不能移动，不能改变大小
    ShowCursor(false);   //取消鼠标显示
    ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);  //隐藏任务栏

    dlg.GetWindowRect(rect);   //获取窗口
    /*rect.left = 0;
    rect.top = 0;
    rect.right = 1;
    rect.bottom = 1;*/
    rect.right = rect.left + 1;
    rect.bottom = rect.top + 1;
    TRACE("right = %d, left = %d\r\n", rect.right, rect.left);
    ClipCursor(rect);   //将鼠标限制在一个像素点
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);  //虚拟键码消息转换为字符消息
        DispatchMessage(&msg);  //消息分发给窗口过程进行处理
        if (msg.message == WM_KEYDOWN) {
            TRACE("msg:%08X wparam:%08x lparam:%08x\r\n", msg.message, msg.wParam, msg.lParam); 
            if (msg.wParam == 0x1B)  break;  //按ESC退出
        }
    }
    ClipCursor(NULL); 
    ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);  //显示任务栏
    ShowCursor(true);
    dlg.DestroyWindow();
    _endthreadex(0);
    return 0;
}

int LockMachine() {
    //锁机放入线程运行
    if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE)) {
        _beginthreadex(NULL, 0, threadLockDlg, NULL, 0, &threadid),
            TRACE("thread = %d\r\n", threadid);
    }
    CPacket pack(7, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int UnlockMachine() {
    PostThreadMessage(threadid, WM_KEYDOWN, 0x1B, 0);
    CPacket pack(8, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int DeleteLocalFile() {
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    TCHAR sPath[MAX_PATH] = _T("");
    MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), 
        strPath.size(), sPath, sizeof(sPath) / sizeof(TCHAR)); 
    DeleteFileA(strPath.c_str());
    CPacket pack(9, NULL, 0);
    bool ret = CServerSocket::getInstance()->Send(pack);
    TRACE("服务器发送pack : %d\r\n", ret);
    return 0;
}

int TestConnect() {
    CPacket pack(39, NULL, 0);
    bool ret = CServerSocket::getInstance()->Send(pack);
    TRACE("服务器发送pack : %d\r\n", ret);
    return 0;
}

int ExcuteCommand(int nCmd) {
    int ret = 0;
    switch (nCmd) {
    case 1:  //查看磁盘分区
        ret = MakeDriverInfo();
        break;
    case 2:  //查看指定目录下的文件
        ret = MakeDirectoryInfo();
        break;
    case 3:  //打开文件
        ret = RunFile();
        break;
    case 4:  //下载文件
        ret = DownloadFile();
        break;
    case 5: //鼠标操作
        ret = MouseEvent();
        break;
    case 6:  //发送屏幕内容(截图)
        ret = SendScreen();
        break;
    case 7:  //锁机
        ret = LockMachine();
        break;
    case 8:  //解锁
        ret = UnlockMachine();
        break;
    case 9:  //删除文件
        ret = DeleteLocalFile();
        break;
    case 39: //连接测试
        ret = TestConnect();
        break;
    }
    /*Sleep(3000);
    UnlockMachine();
    while ((dlg.m_hWnd != NULL) && (dlg.m_hWnd != INVALID_HANDLE_VALUE)) Sleep(1000);
    TRACE("m_hWnd = %08X\r\n", dlg.m_hWnd);*/
    return ret;
}

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
                    ret = ExcuteCommand(ret);
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
