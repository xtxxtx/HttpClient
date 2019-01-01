// Main.cpp

#include <stdio.h>


#include "HttpClient.h"


int OnRead(const char* pBuf, int iLen, void* pHandle)
{

	printf("%s", pBuf);

	return 0;
}

int main(int argc, char* argv[])
{
	CHttpClient httpClient;

	char szUrl[] = "https://www.baidu.com";

	httpClient.Initialize(szUrl, OnRead, nullptr);

	httpClient.Request();

	return 0;
}