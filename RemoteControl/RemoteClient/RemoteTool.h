#pragma once
#include <Windows.h>
#include <atlimage.h>
#include <string>

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
    static int Bytr2Image(CImage& image, const std::string& strBuffer) {
		BYTE* pData = (BYTE*)strBuffer.c_str();
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
		if (hMem == NULL) {
			TRACE("内存不足!");
			Sleep(1);
			return -1;
		}
		IStream* pStream = NULL;
		HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
		if (hRet == S_OK) {
			ULONG length = 0;
			pStream->Write(pData, strBuffer.size(), &length);
			LARGE_INTEGER bg{ 0 };
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);
			if ((HBITMAP)image != NULL) image.Destroy();
			image.Load(pStream);
		}
		return hRet;
    }

	static std::string GetErrorInfo(int wsaErrCode) {
		std::string ret;
		LPVOID lpMsgBuf = NULL;
		FormatMessage(
			//从系统提供的预定义错误消息资源中获取错误信息。
			//让系统自动分配一个足够容纳错误信息文本的内存缓冲区。
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			//错误信息来源于系统而非自定义模块。
			NULL,
			//需要查询错误的代码。
			wsaErrCode,
			//构造一个“中性”语言环境，系统将基于默认的子语言返回错误字符串。
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf, 0, NULL);

		ret = (char*)lpMsgBuf;
		//系统为错误消息分配了内存，所以使用 LocalFree 来释放这块内存，避免内存泄漏。
		LocalFree(lpMsgBuf);
		return ret;
	}
};

