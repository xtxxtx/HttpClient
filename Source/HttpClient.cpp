// HttpClient.cpp

#ifdef _MSC_VER
#include <WS2tcpip.h>
#else
#endif

#include "HttpClient.h"


#ifdef _MSC_VER
#pragma comment(lib, "ws2_32")

static struct CSockInit {
	CSockInit()	{ WSADATA dat; WSAStartup(0x0202, &dat); }
	~CSockInit() { WSACleanup(); }
} SOCK_INIT;
#endif

static char* 
URL::StrCpy(char* pDst, const char* pSrc)
{
	while (*pDst++ = *pSrc++);
	return pDst;
}

int	
URL::Parser(const char* pszUrl, char* pszProt, char* pszAddr, uint16_t& iPort, char* pszPath) 
{
	if (pszUrl == NULL || pszUrl[0] == 0) {
		return -1;
	}

	char szTmp[256] = { 0 };
	int i = 0;
	for (int j = 0; pszUrl[i]; i++) {
		if (pszUrl[i] == ':') {
			szTmp[j] = 0;
			++i;
			if (pszUrl[i] == '/') {
				StrCpy(pszProt, szTmp);
				i += 2;
				for (j = 0; pszUrl[i]; i++) {
					if (pszUrl[i] == ':') {
						szTmp[j] = 0;
						++i;
						StrCpy(pszAddr, szTmp);
						iPort = 0;
						for (; pszUrl[i] >= '0'&&pszUrl[i] <= '9'; i++) {
							iPort = iPort * 10 + pszUrl[i] - '0';
						}

						if (pszUrl[i] == '/') {
							StrCpy(pszPath, pszUrl + i);
						} else if (pszUrl[i] == 0) {
							StrCpy(pszPath, "/");
						} else {
							return -1;
						}
						return 0;
					} else if (pszUrl[i] == '/') {
						szTmp[j] = 0;
						StrCpy(pszAddr, szTmp);
						StrCpy(pszPath, pszUrl + i);
						iPort = 80;
						return 0;
					} else {
						szTmp[j++] = pszUrl[i];
					}
				}
				StrCpy(pszAddr, szTmp);
				StrCpy(pszPath, "/");
				iPort = 80;
			} else {
				StrCpy(pszProt, "http");
				StrCpy(pszAddr, szTmp);

				iPort = 0;
				for (; pszUrl[i] >= '0'&&pszUrl[i] <= '9'; i++) {
					iPort = iPort * 10 + pszUrl[i] - '0';
				}

				if (pszUrl[i] == '/') {
					StrCpy(pszPath, pszUrl + i);
				} else if (pszUrl[i] == 0) {
					StrCpy(pszPath, "/");
				} else {
					return -1;
				}
			}
			return 0;
		} else if (pszUrl[i] == '/') {
			szTmp[j] = 0;
			StrCpy(pszProt, "http");
			StrCpy(pszAddr, szTmp);
			StrCpy(pszPath, pszUrl + i);
			iPort = 80;
			return 0;
		} else {
			szTmp[j++] = pszUrl[i];
		}
	}

	// not found : or /
	if (pszUrl[i] == 0) {
		StrCpy(pszProt, "http");
		StrCpy(pszAddr, szTmp);
		StrCpy(pszPath, "/");
		iPort = 80;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
CHttpClient::CHttpClient()
{
	m_iFd = 0;
	m_pHead = nullptr;
	m_pTail = nullptr;
}

CHttpClient::~CHttpClient()
{

}

int32_t
CHttpClient::Initialize(const char* pszUrl)
{


	return 0;
}

void
CHttpClient::AddElem(const char* pszName, const char* pszValue)
{
	Elem* pElem = new Elem(pszName, pszValue);

	if (m_pHead == nullptr) {
		m_pHead = pElem;
	}

	if (m_pTail != nullptr) {
		m_pTail->pNext = pElem;
	}
	m_pTail = pElem;
}

int
CHttpClient::Request(const char* pszUrl)
{
	const int BUF_SIZ = 8192;
	char szBuf[BUF_SIZ] = { 0 };

	char szProt[8] = { 0 };
	char szAddr[64] = { 0 };
	uint16_t iPort = 0;
	char szPath[256] = { 0 };
	URL::Parser(pszUrl, szProt, szAddr, iPort, szPath);

	int iOffset = 0;
	iOffset = sprintf_s(szBuf, "GET %s HTTP/1.1\r\n", szPath);
	iOffset += sprintf_s(szBuf + iOffset, BUF_SIZ-iOffset, "Host: %s\r\n", szAddr);
	for (Elem* pCurr = m_pHead; pCurr != nullptr; pCurr = pCurr->pNext) {
		iOffset += sprintf_s(szBuf + iOffset, BUF_SIZ - iOffset, "%s: %s\r\n", pCurr->szName, pCurr->szValue);
	}
	iOffset += sprintf_s(szBuf + iOffset, BUF_SIZ - iOffset, "\r\n");

	Cleanup(m_pHead);

	socket_t iFd = Connect(szAddr, iPort);
	if (iFd == -1) {
		return -1;
	}
	send(iFd, szBuf, iOffset, 0);

	int iResult = 0;
	struct timeval tv = { 0, 100000 };
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(iFd, &rfds);

	for (iOffset=0;;) {
		iResult = select(0, &rfds, nullptr, nullptr, &tv);
		if (iResult == 0) {
			continue;
		}
		if (iResult < 0) {
			break;
		}

		iResult = recv(iFd, szBuf+iOffset, BUF_SIZ-iOffset, 0);
		if (iResult < 1) {
			break;
		}
		iOffset += iResult;
		szBuf[iOffset] = 0;

		if (strstr(szBuf, "\r\n\r\n") == nullptr) {
			continue;
		}
	}


	closesocket(iFd);

	return 0;
}

socket_t
CHttpClient::Connect(const char* pszAddr, uint16_t iPort)
{
	socket_t iFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	char szPort[8] = { 0 };
	_itoa_s(iPort, szPort, 10);

	addrinfo* ais = NULL;
	if (getaddrinfo(pszAddr, szPort, NULL, &ais) || ais == NULL) {
		return -1;
	}

	for (addrinfo* tmp = ais; tmp != NULL; tmp = tmp->ai_next) {
		if (tmp->ai_family != AF_INET) {
			continue;
		}
		sockaddr_in* sainf = (sockaddr_in*)(tmp->ai_addr);

		if (connect(iFd, tmp->ai_addr, sizeof(struct sockaddr_in)) == 0) {
			int iValue = 0x10000;
			setsockopt(iFd, SOL_SOCKET, SO_RCVBUF, (const char*)&iValue, sizeof(iValue));
			setsockopt(iFd, SOL_SOCKET, SO_SNDBUF, (const char*)&iValue, sizeof(iValue));

			return iFd;
		}
	}

	return -1;
}

void
CHttpClient::Cleanup(Elem* pHead)
{
	for (Elem* pCurr = pHead; pCurr != nullptr;) {
		pHead = pHead->pNext;
		delete pCurr;
		pCurr = pHead;
	}	
}
