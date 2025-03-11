#include "pch.h"
#include "ServerSocket.h"

CServerSocket* CServerSocket::mInstance = NULL;
CServerSocket::CHelper CServerSocket::mHelper;
CServerSocket* pserver = CServerSocket::getInstance();