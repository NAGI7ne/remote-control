#pragma once
class CRemoteTool
{
public:
    static void Dump(BYTE* pData, size_t nSize) {
        std::string strOut;
        for (size_t i = 0; i < nSize; i++) {
            char buf[8]{};
            if (i > 0 && (i % 16 == 0)) strOut += "\n";
            snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
            strOut += buf;
        }
        strOut += "\n";
        OutputDebugStringA(strOut.c_str());
    }

	static bool IsAdmin() {
		HANDLE hToken = NULL;
		//OpenProcessToken �����򿪵�ǰ���̵ķ�������
		//GetCurrentProcess() ���ص�ǰ���̵ľ��
		//���� TOKEN_QUERY �����ѯȨ��
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
		{
			ShowError();
			return false;
		}
		TOKEN_ELEVATION eve;
		DWORD len = 0;
		//����ȡ��ǰ���̷������Ƶġ���������elevation����Ϣ���жϸ������Ƿ���й���ԱȨ��
		//TokenElevation��һ��ö��ֵ����ʾ�������������Ϣ��
		if (GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len) == FALSE) {
			ShowError();
			return false;
		}
		CloseHandle(hToken);
		if (len == sizeof(eve)) {
			return eve.TokenIsElevated;
		}
		printf("length of tokeninformation is %d\r\n", len);
		return false;
	}

	static bool RunAsAdmin()
	{
		//��windows���ز����� ����Administrator�˻�  ��ֹ������ֻ�ܵ�¼���ؿ���̨
		/*HANDLE hToken = NULL;
		* //LogonUser ������ָ֤���û��ĵ�¼��Ϣ������һ����������
		* //L"Administrator"��ָ��Ҫ��¼���˻����ƣ�����Ϊ���� Administrator �˻�
		* //NULL:��ʾ����������Ϊ NULL ͨ����ʾ��¼���ؼ����
		* //NULL:��������
		* //LOGON32_LOGON_BATCH��ָ����¼����Ϊ������Batch Logon����
		* //    �����ں�̨�������������ҵ������Ҫ����ʽ�û�������
		* //LOGON32_PROVIDER_DEFAULT��ָ��ʹ��Ĭ�ϵĵ�¼��֤������ṩ����
		* //��¼�ɹ��󣬻᷵��һ�������ں��� API �����ķ������ƾ����
		BOOL ret = LogonUser(L"Administrator", NULL, NULL, LOGON32_LOGON_BATCH,
			LOGON32_PROVIDER_DEFAULT, &hToken);
		if (!ret) {
			ShowError();
			MessageBox(NULL, _T("��¼����"), _T("�������"), 0);
			return false;
		}*/
		STARTUPINFO si = { 0 };
		PROCESS_INFORMATION pi = { 0 };
		TCHAR sPath[MAX_PATH] = _T("");
		//GetModuleFileName ��ȡ��ǰ���г��������·���������� sPath �С�
		GetModuleFileName(NULL, sPath, MAX_PATH);

		//BOOL ret = CreateProcessWithTokenW(hToken, LOGON_WITH_PROFILE, NULL, sPath, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
		
		
		//_T("Administrator") ���ʾҪʹ�����õ� Administrator �˻������½���
		//����Ϊ NULL һ������ָ��������롣���ﶼΪ NULL��
		// ��ʾ��ָ���򣬲�����Ҫ���ݱ��ز��������������¼
		//LOGON_WITH_PROFILE ָ����¼ʱ�����û������ļ������½�������ʱ���й���Ա�����û���
		//NULL����ʾ�½��̵��������������ǰ׺
		//sPath���뵱ǰ���������·��,�½��̽������뵱ǰ������ͬ�Ŀ�ִ���ļ�
		//CREATE_UNICODE_ENVIRONMENT ָ������ Unicode ��������
		//���ڴ���������Ϣ��si���ͽ��մ����������½�����Ϣ��pi��
		BOOL ret = CreateProcessWithLogonW(_T("Administrator"), NULL, NULL, LOGON_WITH_PROFILE, NULL, sPath, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
		if (!ret) {
			ShowError();//TODO:ȥ��������Ϣ
			MessageBox(NULL, sPath, _T("��������ʧ��"), 0);//TODO:ȥ��������Ϣ
			return false;
		}
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return true;
	}

	static void ShowError()
	{
		LPWSTR lpMessageBuf = NULL;
		//strerror(errno);//��׼C������
		
		
		// FORMAT_MESSAGE_FROM_SYSTEM ����������Ϣ������ϵͳ����Ĵ�����Ϣ.
		// FORMAT_MESSAGE_ALLOCATE_BUFFER ��ʾ��ϵͳ������������ڴ棬
		// ���ص���Ϣ�ַ����������ϵͳ����Ļ������ڣ���������ַ��ֵ�� lpMessageBuf��
		// NULL ��ʾ��ʹ���ض�����Ϣ��Դ��ֱ�Ӵ�ϵͳ��Ϣ���в��Ҵ�����Ϣ��
		// GetLastError() ��ȡ��ǰ�߳������һ�η�������Ĵ����룬��Ϊ���Ҵ�����Ϣ�����ݡ�
		// MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)ָ��ʹ���������Ժ�Ĭ������������ȡ������Ϣ��
		// �����ܱ�֤�ڶ����Ի����»�ú��ʵ�������Ϣ��
		// ʹ�� FORMAT_MESSAGE_ALLOCATE_BUFFER ʱ��FormatMessage ���ڴ�λ�÷����ڴ沢�ѵ�ַ���ء�
		// 0 ��ʾ����������С��С, ��ʹ���� ALLOCATE_BUFFER ��־��ϵͳ���Զ������㹻�Ŀռ䣬���������� 0
		// NULL : ��ʾ����Ҫ�����������������ַ�����ȥ��ʽ��������Ϣ��
		FormatMessage(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			NULL, GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&lpMessageBuf, 0, NULL);
		OutputDebugString(lpMessageBuf);
		MessageBox(NULL, lpMessageBuf, _T("��������"), 0);
		LocalFree(lpMessageBuf);
	}
	/**
	* ��bug��˼·
	* 0 �۲�����
	* 1 ��ȷ����Χ
	* 2 ��������Ŀ�����
	* 3 ���Ի��ߴ���־���Ų����
	* 4 �������
	* 5 ��֤/��ʱ����֤/�����֤/����������֤
	**/
	static BOOL WriteStartupDir(const CString& strPath)
	{//ͨ���޸Ŀ��������ļ�����ʵ�ֿ�������
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);
		return CopyFile(sPath, strPath, FALSE);
		//fopen CFile system(copy) CopyFile OpenFile 
	}
	//����������ʱ�򣬳����Ȩ���Ǹ��������û���
	//�������Ȩ�޲�һ�£���ᵼ�³�������ʧ��
	// ����ʱ��Ҫ�ų�Ȩ�޺ͻ���������Ӱ�죩
	//  ����->������->�嵥�ļ�->UACִ�м��� ���ı������
	//  ����->�߼�->MFCʹ�� ( ʹ�þ�̬�� )
	//���������Ի���������Ӱ�죬�������dll����̬�⣩�����������ʧ��
	//���������
	//��������Щdll��system32�������sysWOW64����,��
	//system32���棬����64λ���� syswow64�������32λ����
	//��ʹ�þ�̬�⣬���Ƕ�̬�⡿
	static bool WriteRegisterTable(const CString& strPath)
	{//ͨ���޸�ע�����ʵ�ֿ�������
		CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);
		BOOL ret = CopyFile(sPath, strPath, FALSE);
		//fopen CFile system(copy) CopyFile OpenFile 
		if (ret == FALSE) {
			MessageBox(NULL, _T("�����ļ�ʧ�ܣ��Ƿ�Ȩ�޲��㣿\r\n"), _T("����"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		HKEY hKey = NULL;
		// HKEY_LOCAL_MACHINE ָ����Ҫ�򿪵ĸ�ע���������ؼ������ע�����
	   // strSubKey �� HKEY_LOCAL_MACHINE ��Ҫ�򿪵�����·��
	   //0 ����������������дΪ 0
	   // KEY_ALL_ACCESS ��ʾ����Ը�ע�����ӵ�����п��ܵķ���Ȩ�ޣ�������ȡ��д���Լ��޸�Ȩ�ޡ�
	   // KEY_WOW64_64KEY ǿ���� 64 λ Windows ϵͳ���� 64 λ��ͼ��ע����
	   // ��ʹ��������� 32 λģʽ���У������־Ҳ��ȷ�����ʵ�ϵͳʵ��ʹ�õ� 64 λע�������
	   // &hKey �ڽ��մ򿪺��ע�������
		ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);
		if (ret != ERROR_SUCCESS) {
			RegCloseKey(hKey);
			MessageBox(NULL, _T("�����Զ���������ʧ�ܣ��Ƿ�Ȩ�޲��㣿\r\n��������ʧ�ܣ�"), _T("����"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		//strPath.GetLength() ���ص����ַ������ַ�������
	   //RegSetValueEx Ҫ���ṩ�����ݴ�С���ֽ���Ϊ��λ
	   //��Ҫ����ÿ���ַ���ռ���ֽ�����Ҳ���� sizeof(TCHAR)
	   //����� ANSI ����ģʽ���� TCHAR ��ʵ���� char����ʱ sizeof(TCHAR) Ϊ 1��
	   //����� Unicode ����ģʽ���� TCHAR �� wchar_t��ͨ�� sizeof(TCHAR) Ϊ 2
	   //    REG_EXPAND_SZ    ��ʾ����������һ������չ�ַ���
	   // ����չ�ַ����������ַ��������а����������������� %SYSTEMROOT%����
	   // ��Щ���������ڶ�ȡʱ�ᱻϵͳչ����ʵ�ʵ�ֵ��
		ret = RegSetValueEx(hKey, _T("RemoteCtrl"), 0, REG_EXPAND_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength() * sizeof(TCHAR));
		if (ret != ERROR_SUCCESS) {
			RegCloseKey(hKey);
			MessageBox(NULL, _T("�����Զ���������ʧ�ܣ��Ƿ�Ȩ�޲��㣿\r\n��������ʧ�ܣ�"), _T("����"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		RegCloseKey(hKey);
		return true;
	}

	static bool Init()
	{//���ڴ�mfc��������Ŀ��ʼ����ͨ�ã�
		HMODULE hModule = ::GetModuleHandle(nullptr);
		if (hModule == nullptr) {
			wprintf(L"����: GetModuleHandle ʧ��\n");
			return false;
		}
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
		{
			// TODO: �ڴ˴�ΪӦ�ó������Ϊ��д���롣
			wprintf(L"����: MFC ��ʼ��ʧ��\n");
			return false;
		}
		return true;
	}

	static std::string GetErrInfo(int wsaErrCode)
	{
		std::string ret;
		LPVOID lpMsgBuf = NULL;
		FormatMessage(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			NULL,
			wsaErrCode,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf, 0, NULL);
		ret = (char*)lpMsgBuf;
		LocalFree(lpMsgBuf);
		return ret;
	}
};

