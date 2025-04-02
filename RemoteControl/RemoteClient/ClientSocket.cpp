#include "pch.h"
#include "ClientSocket.h"

CClientSocket* CClientSocket::mInstance = NULL;
CClientSocket::CHelper CClientSocket::mHelper;
CClientSocket* pclient = CClientSocket::getInstance();

std::string GetErrorInfo(int wsaErrCode) {
	std::string ret;
	LPVOID lpMsgBuf = NULL;
	FormatMessage(
		//��ϵͳ�ṩ��Ԥ���������Ϣ��Դ�л�ȡ������Ϣ��
		//��ϵͳ�Զ�����һ���㹻���ɴ�����Ϣ�ı����ڴ滺������
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		//������Ϣ��Դ��ϵͳ�����Զ���ģ�顣
		NULL,
		//��Ҫ��ѯ����Ĵ��롣
		wsaErrCode,
		//����һ�������ԡ����Ի�����ϵͳ������Ĭ�ϵ������Է��ش����ַ�����
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);

	ret = (char*)lpMsgBuf;
	//ϵͳΪ������Ϣ�������ڴ棬����ʹ�� LocalFree ���ͷ�����ڴ棬�����ڴ�й©��
	LocalFree(lpMsgBuf);
	return ret;
}

void Dump(BYTE* pData, size_t nSize)
{
	std::string strOut;
	for (size_t i = 0; i < nSize; i++)
	{
		char buf[8] = "";
		if (i > 0 && (i % 16 == 0))strOut += "\n";
		snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
		strOut += buf;
	}
	strOut += "\n";
	OutputDebugStringA(strOut.c_str());
}