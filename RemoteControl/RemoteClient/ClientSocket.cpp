#include "pch.h"
#include "ClientSocket.h"

CClientSocket* CClientSocket::mInstance = NULL;
CClientSocket::CHelper CClientSocket::mHelper;
CClientSocket* pclient = CClientSocket::getInstance();