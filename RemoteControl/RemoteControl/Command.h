#pragma once
#include <map>
#include <atlimage.h>
#include "RemoteTool.h"
#include "ServerSocket.h"
#include <direct.h>
#include <io.h>
#include <stdio.h>
#include <list>
#include "LockInfoDialog.h"
#include "Resource.h"

class CCommand
{
public:    //TODO��Ϊʲô��Init����������Ļ��Ͳ��ù�����
	//static void Init();
	//static int ExcuteCommand(int nCmd);
	CCommand();
    ~CCommand() {}
	int ExcuteCommand(int nCmd);
protected:
	typedef int(CCommand::* CMDFUNC)(); //��Ա����ָ��  //TODO:
	std::map<int, CMDFUNC> mMapFunction; // ������ŵ����ܵ�ӳ��
	                                    //����ʹ ExcuteCommand ��ü��
    CLockInfoDialog dlg;  //��Ҫ��ε��ã���ȫ�ֱ���
    unsigned threadid;
protected:
    static unsigned __stdcall threadLockDlg(void* arg) {
        CCommand* thiz = (CCommand*)arg;
        thiz->threadLockDlgMain();
        _endthreadex(0);
        return 0;
    }
    void threadLockDlgMain() {
        TRACE("%s (%d) : %d\r\n", __FUNCTION__, __LINE__, GetCurrentThreadId());
        dlg.Create(IDD_DIALOG1_INFO, NULL);
        dlg.ShowWindow(SW_SHOW);
        //dlg.CenterWindow();
        CRect rect;
        //rect.left = 0;
        //rect.top = 0;
        //rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
        //rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);  //��ȡ������Ļ
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

        dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);  //�����ö������ò����ƶ������ܸı��С
        ShowCursor(false);   //ȡ�������ʾ
        ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);  //����������

        dlg.GetWindowRect(rect);   //��ȡ����
        /*rect.left = 0;
        rect.top = 0;
        rect.right = 1;
        rect.bottom = 1;*/
        rect.right = rect.left + 1;
        rect.bottom = rect.top + 1;
        TRACE("right = %d, left = %d\r\n", rect.right, rect.left);
        ClipCursor(rect);   //�����������һ�����ص�
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);  //���������Ϣת��Ϊ�ַ���Ϣ
            DispatchMessage(&msg);  //��Ϣ�ַ������ڹ��̽��д���
            if (msg.message == WM_KEYDOWN) {
                TRACE("msg:%08X wparam:%08x lparam:%08x\r\n", msg.message, msg.wParam, msg.lParam);
                if (msg.wParam == 0x1B)  break;  //��ESC�˳�
            }
        }
        ClipCursor(NULL);
        ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);  //��ʾ������
        ShowCursor(true);
        dlg.DestroyWindow();
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
        CRemoteTool::Dump((BYTE*)pack.Data(), pack.Size());
        CServerSocket::getInstance()->Send(pack);
        return 0;
    }

    int MakeDirectoryInfo() {
        std::string strPath;
        //std::list<FILEINFO> lstFileInfos;
        if (CServerSocket::getInstance()->GetFilePath(strPath) == false) {
            OutputDebugString(_T("���ǻ�ȡ�ļ�����,�����������"));
            return -1;
        }
        if (_chdir(strPath.c_str()) != 0) {
            FILEINFO finfo;
            finfo.HasNext = FALSE;
            CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
            CServerSocket::getInstance()->Send(pack);
            OutputDebugString(_T("�޷����ļ�������Ȩ��"));
            return -2;
        }
        _finddata_t fdata;
        intptr_t hfind = 0;
        if ((hfind = _findfirst("*", &fdata)) == -1) {
            OutputDebugString(_T("û���ҵ��ļ���"));
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
            //Sleep(5);  //ȷ���������������������յ�
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
        //��·��Ϊ strPath ���ļ�,����������״̬��ʾӦ�ó���
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
            switch (mouse.nButton) {
            case 0:   //���
                nFlags = 0x01;
                break;
            case 1:  //�Ҽ�
                nFlags = 0x02;
                break;
            case 2:  //�м�
                nFlags = 0x04;
                break;
            case 4:  //û�а���
                nFlags = 0x08;
                break;
            }
            if (nFlags != 8) SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
            switch (mouse.nAction) {
            case 0:  //����
                nFlags |= 0x10;
                break;
            case 1:  //˫��
                nFlags |= 0x20;
                break;
            case 2:  //����
                nFlags |= 0x40;
                break;
            case 3:  //�ſ�
                nFlags |= 0x80;
                break;
            default:
                break;
            }
            TRACE("mouse event : %08X x %d y %d\r\n", nFlags, mouse.ptXY.x, mouse.ptXY.y);
            switch (nFlags) {
            case 0x21:  //���˫��
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            case 0x11:  //�������
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x41:  //�������
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x81:  //����ſ�
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x22:  //�Ҽ�˫��
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            case 0x12:  //�Ҽ�����
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x42:  //�Ҽ�����
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x82:  //�Ҽ��ſ�
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x24:  //�м�˫��
                mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            case 0x14:  //�м�����
                mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x44:  //�м�����
                mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x84:  //�м��ſ�
                mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x08:  //����ƶ�
                mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
                break;
            }
            CPacket pack(4, NULL, 0);
            CServerSocket::getInstance()->Send(pack);
        }
        else {
            OutputDebugString(_T("��ȡ����������ʧ�ܣ�"));
            return -1;
        }

        return 0;
    }

    int SendScreen() {
        CImage screen;
        HDC hScreen = ::GetDC(NULL);    //��ȡ��Ļ������
        int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL); //��ȡ��Ļÿ�����ص�λ������ɫ��ȣ�
        int nWidth = GetDeviceCaps(hScreen, HORZRES);
        int nHeight = GetDeviceCaps(hScreen, VERTRES);
        screen.Create(nWidth, nHeight, nBitPerPixel); //Ϊ screen ����������Ļ�ߴ��ɫ��ƥ���ͼ�񻺳�����
        BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY); //����Ļ�ϵ����ݸ��Ƶ� screen ��Ӧ���豸��������
        ReleaseDC(NULL, hScreen);
        //��Ҫ����Ļ����ʹ��������ʽ�����ڴ�
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);//����һ�����ƶ���ȫ���ڴ�飨�����
        if (hMem == NULL) return -1;
        IStream* pStream = NULL;
        HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream); //�������ȫ���ڴ��װ��һ�������󣨴˺�����Ҫ���ƶ���ȫ���ڴ�飩
        if (ret == S_OK) {
            screen.Save(pStream, Gdiplus::ImageFormatJPEG); //��ͼ�񱣴浽����
            LARGE_INTEGER bg{ 0 };
            pStream->Seek(bg, STREAM_SEEK_SET, NULL);//Saveд��������ڲ�ָ��ͨ����λ�����ݵ�ĩβ, ��Ҫ��ͷ��ʼ
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

    int LockMachine() {
        //���������߳�����
        if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE)) {
            _beginthreadex(NULL, 0, &CCommand::threadLockDlg, this, 0, &threadid),
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
        TRACE("����������pack : %d\r\n", ret);
        return 0;
    }

    int TestConnect() {
        CPacket pack(39, NULL, 0);
        bool ret = CServerSocket::getInstance()->Send(pack);
        TRACE("����������pack : %d\r\n", ret);
        return 0;
    }
};

