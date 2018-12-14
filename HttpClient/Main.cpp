// Main.cpp

#include <stdio.h>


#include "HttpClient.h"



int main(int argc, char* argv[])
{
	CHttpClient httpClient;

	char szUrl[] = "http://www.baidu.com";

	httpClient.Request(szUrl);

	return 0;
}