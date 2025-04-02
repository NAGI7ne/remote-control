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
		//OpenProcessToken 用来打开当前进程的访问令牌
		//GetCurrentProcess() 返回当前进程的句柄
		//参数 TOKEN_QUERY 请求查询权限
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
		{
			ShowError();
			return false;
		}
		TOKEN_ELEVATION eve;
		DWORD len = 0;
		//来获取当前进程访问令牌的“提升”（elevation）信息，判断该令牌是否具有管理员权限
		//TokenElevation：一个枚举值，表示请求的是提升信息。
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
		//在windows本地策略组 开启Administrator账户  禁止空密码只能登录本地控制台
		/*HANDLE hToken = NULL;
		* //LogonUser 用来验证指定用户的登录信息并创建一个访问令牌
		* //L"Administrator"：指定要登录的账户名称，这里为内置 Administrator 账户
		* //NULL:表示域名，设置为 NULL 通常表示登录本地计算机
		* //NULL:代表密码
		* //LOGON32_LOGON_BATCH：指定登录类型为批处理（Batch Logon），
		* //    适用于后台服务或批处理作业，不需要交互式用户环境。
		* //LOGON32_PROVIDER_DEFAULT：指定使用默认的登录验证所需的提供程序。
		* //登录成功后，会返回一个可用于后续 API 操作的访问令牌句柄。
		BOOL ret = LogonUser(L"Administrator", NULL, NULL, LOGON32_LOGON_BATCH,
			LOGON32_PROVIDER_DEFAULT, &hToken);
		if (!ret) {
			ShowError();
			MessageBox(NULL, _T("登录错误"), _T("程序错误"), 0);
			return false;
		}*/
		STARTUPINFO si = { 0 };
		PROCESS_INFORMATION pi = { 0 };
		TCHAR sPath[MAX_PATH] = _T("");
		//GetModuleFileName 获取当前运行程序的完整路径，并存入 sPath 中。
		GetModuleFileName(NULL, sPath, MAX_PATH);

		//BOOL ret = CreateProcessWithTokenW(hToken, LOGON_WITH_PROFILE, NULL, sPath, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
		
		
		//_T("Administrator") 这表示要使用内置的 Administrator 账户启动新进程
		//设置为 NULL 一般用于指定域和密码。这里都为 NULL，
		// 表示不指定域，并且需要根据本地策略允许无密码登录
		//LOGON_WITH_PROFILE 指定登录时加载用户配置文件，即新进程运行时会有管理员的配置环境
		//NULL，表示新进程的命令行无需额外前缀
		//sPath传入当前程序的完整路径,新进程将运行与当前程序相同的可执行文件
		//CREATE_UNICODE_ENVIRONMENT 指定创建 Unicode 环境变量
		//用于传递启动信息（si）和接收创建出来的新进程信息（pi）
		BOOL ret = CreateProcessWithLogonW(_T("Administrator"), NULL, NULL, LOGON_WITH_PROFILE, NULL, sPath, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
		if (!ret) {
			ShowError();//TODO:去除调试信息
			MessageBox(NULL, sPath, _T("创建进程失败"), 0);//TODO:去除调试信息
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
		//strerror(errno);//标准C语音库
		
		
		// FORMAT_MESSAGE_FROM_SYSTEM 表明错误信息来自于系统定义的错误信息.
		// FORMAT_MESSAGE_ALLOCATE_BUFFER 表示让系统来分配所需的内存，
		// 返回的消息字符串会放在由系统分配的缓冲区内，缓冲区地址赋值给 lpMessageBuf。
		// NULL 表示不使用特定的消息来源，直接从系统消息库中查找错误信息。
		// GetLastError() 获取当前线程中最后一次发生错误的错误码，作为查找错误信息的依据。
		// MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)指定使用中性语言和默认子语言来读取错误消息，
		// 这样能保证在多语言环境下获得合适的语言信息。
		// 使用 FORMAT_MESSAGE_ALLOCATE_BUFFER 时，FormatMessage 会在此位置分配内存并把地址传回。
		// 0 表示缓冲区的最小大小, 于使用了 ALLOCATE_BUFFER 标志，系统会自动分配足够的空间，所以这里填 0
		// NULL : 表示不需要额外参数（例如插入字符串）去格式化错误信息。
		FormatMessage(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			NULL, GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&lpMessageBuf, 0, NULL);
		OutputDebugString(lpMessageBuf);
		MessageBox(NULL, lpMessageBuf, _T("发生错误"), 0);
		LocalFree(lpMessageBuf);
	}
	/**
	* 改bug的思路
	* 0 观察现象
	* 1 先确定范围
	* 2 分析错误的可能性
	* 3 调试或者打日志，排查错误
	* 4 处理错误
	* 5 验证/长时间验证/多次验证/多条件的验证
	**/
	static BOOL WriteStartupDir(const CString& strPath)
	{//通过修改开机启动文件夹来实现开机启动
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);
		return CopyFile(sPath, strPath, FALSE);
		//fopen CFile system(copy) CopyFile OpenFile 
	}
	//开机启动的时候，程序的权限是跟随启动用户的
	//如果两者权限不一致，则会导致程序启动失败
	// （此时需要排除权限和环境变量的影响）
	//  属性->链接器->清单文件->UAC执行级别 （改变管理级别）
	//  属性->高级->MFC使用 ( 使用静态库 )
	//开机启动对环境变量有影响，如果依赖dll（动态库），则可能启动失败
	//解决方法：
	//【复制这些dll到system32下面或者sysWOW64下面,】
	//system32下面，多是64位程序 syswow64下面多是32位程序
	//【使用静态库，而非动态库】
	static bool WriteRegisterTable(const CString& strPath)
	{//通过修改注册表来实现开机启动
		CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);
		BOOL ret = CopyFile(sPath, strPath, FALSE);
		//fopen CFile system(copy) CopyFile OpenFile 
		if (ret == FALSE) {
			MessageBox(NULL, _T("复制文件失败，是否权限不足？\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		HKEY hKey = NULL;
		// HKEY_LOCAL_MACHINE 指定了要打开的根注册表项，即本地计算机的注册表树
	   // strSubKey 在 HKEY_LOCAL_MACHINE 下要打开的子项路径
	   //0 保留参数，必须填写为 0
	   // KEY_ALL_ACCESS 表示请求对该注册表项拥有所有可能的访问权限，包括读取、写入以及修改权限。
	   // KEY_WOW64_64KEY 强制在 64 位 Windows 系统上以 64 位视图打开注册表项。
	   // 即使程序可能以 32 位模式运行，这个标志也能确保访问到系统实际使用的 64 位注册表区。
	   // &hKey 于接收打开后的注册表项句柄
		ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);
		if (ret != ERROR_SUCCESS) {
			RegCloseKey(hKey);
			MessageBox(NULL, _T("设置自动开机启动失败！是否权限不足？\r\n程序启动失败！"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		//strPath.GetLength() 返回的是字符串中字符的数量
	   //RegSetValueEx 要求提供的数据大小以字节数为单位
	   //需要乘以每个字符所占的字节数，也就是 sizeof(TCHAR)
	   //如果是 ANSI 编译模式，则 TCHAR 其实就是 char，此时 sizeof(TCHAR) 为 1。
	   //如果是 Unicode 编译模式，则 TCHAR 是 wchar_t，通常 sizeof(TCHAR) 为 2
	   //    REG_EXPAND_SZ    表示数据类型是一个可扩展字符串
	   // 可扩展字符串允许在字符串内容中包含环境变量（例如 %SYSTEMROOT%），
	   // 这些环境变量在读取时会被系统展开成实际的值。
		ret = RegSetValueEx(hKey, _T("RemoteCtrl"), 0, REG_EXPAND_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength() * sizeof(TCHAR));
		if (ret != ERROR_SUCCESS) {
			RegCloseKey(hKey);
			MessageBox(NULL, _T("设置自动开机启动失败！是否权限不足？\r\n程序启动失败！"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		RegCloseKey(hKey);
		return true;
	}

	static bool Init()
	{//用于带mfc命令行项目初始化（通用）
		HMODULE hModule = ::GetModuleHandle(nullptr);
		if (hModule == nullptr) {
			wprintf(L"错误: GetModuleHandle 失败\n");
			return false;
		}
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
		{
			// TODO: 在此处为应用程序的行为编写代码。
			wprintf(L"错误: MFC 初始化失败\n");
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

