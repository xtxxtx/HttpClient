// HttpClient.cpp

#ifdef _MSC_VER
#include <WS2tcpip.h>
#else
#endif

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "HttpClient.h"


#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "openssl/libcrypto.lib")
#pragma comment(lib, "openssl/libssl.lib")

static struct CSockInit {
	CSockInit()	{ WSADATA dat; WSAStartup(0x0202, &dat); }
	~CSockInit() { WSACleanup(); }
} SOCK_INIT;
#endif

//////////////////////////////////////////////////////////////////////////
char* 
URL::StrCpy(char* pDst, const char* pSrc)
{
	if (pDst == nullptr || pSrc == nullptr) {
		return nullptr;
	}

	char* pTmp = pDst;
	while (*pTmp++ = *pSrc++){}

	return pDst;
}

char*
URL::StrDup(const char* pBuf)
{
	if (pBuf == nullptr) {
		return nullptr;
	}

	size_t len = 0;
	char* pTmp = (char*)pBuf;
	while (*pTmp++) len++;

	char* pRet = (char*)malloc(len + 1);
	pTmp = pRet;
	while (*pTmp++ = *pBuf++){}

	return pRet;
}

int	
URL::Parser(const char* pszUrl, char* pszProt, char* pszAddr, uint16_t& iPort, const char*& pszPath) 
{
	if (pszUrl == NULL || pszUrl[0] == 0) {
		return -1;
	}

	static const char* PATH_DAFAULT = "/";

	bool bS = false;
	char szTmp[256] = { 0 };
	int i = 0;
	for (int j = 0; pszUrl[i]; i++) {
		if (pszUrl[i] == ':') {
			szTmp[j] = 0;
			if (pszUrl[++i] == '/') {
				bS = (pszUrl[i-2] == 's' || pszUrl[i-2] == 'S');
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
							pszPath = pszUrl + i;
						} else if (pszUrl[i] == 0) {
							pszPath = PATH_DAFAULT;
						} else {
							return -1;
						}
						return 0;
					} else if (pszUrl[i] == '/') {
						szTmp[j] = 0;
						StrCpy(pszAddr, szTmp);
						pszPath = pszUrl + i;
						iPort = bS ? 443 : 80;
						return 0;
					} else {
						szTmp[j++] = pszUrl[i];
					}
				}
				StrCpy(pszAddr, szTmp);
				pszPath = PATH_DAFAULT;
				
				iPort = bS ? 443 : 80;
			} else {
				StrCpy(pszProt, "http");
				StrCpy(pszAddr, szTmp);

				iPort = 0;
				for (; pszUrl[i] >= '0'&&pszUrl[i] <= '9'; i++) {
					iPort = iPort * 10 + pszUrl[i] - '0';
				}

				if (pszUrl[i] == '/') {
					pszPath = pszUrl + i;
				} else if (pszUrl[i] == 0) {
					pszPath = PATH_DAFAULT;
				} else {
					return -1;
				}
			}
			return 0;
		} else if (pszUrl[i] == '/') {
			szTmp[j] = 0;
			StrCpy(pszProt, "http");
			StrCpy(pszAddr, szTmp);
			pszPath = pszUrl + i;
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
		pszPath = PATH_DAFAULT;
		iPort = 80;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
class CSocket
{
public:
	CSocket() {
		fd = -1;
		ctx = nullptr;
		ssl = nullptr;

		fWrite = nullptr;
		fRead = nullptr;
	}
	~CSocket() {
		if (ssl) {
			SSL_shutdown(ssl);
			SSL_free(ssl);
		}
		xclose(fd);

		if (ctx) {
			SSL_CTX_free(ctx);
		}
	}

	int Initialize(const char*pszProt, const char* pszAddr, uint16_t iPort) {
		fd = Connect(pszAddr, iPort);
		if (fd == -1) {
			return -1;
		}

		if (_stricmp(pszProt, "HTTPS") == 0) {
			ctx = SSL_CTX_new(TLS_client_method());
			SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
			ssl = SSL_new(ctx);
			SSL_set_fd(ssl, (int)fd);
			if (SSL_connect(ssl) == -1) {
				printf("SSL_connect() failed.\n");
				return -1;
			}

			fWrite = Write1;
			fRead = Read1;
		} else {
			fWrite = Write0;
			fRead = Read0;
		}

		return 0;
	}

private:
	socket_t Connect(const char* pszAddr, uint16_t iPort) {
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

	static int Read0(CSocket& ctx, char* pBuf, int iLen) { return recv(ctx.fd, pBuf, iLen, 0); }
	static int Write0(CSocket& ctx, const char* pBuf, int iLen) { return send(ctx.fd, pBuf, iLen, 0); }

	static int Read1(CSocket& ctx, char* pBuf, int iLen) { return SSL_read(ctx.ssl, pBuf, iLen); }
	static int Write1(CSocket& ctx, const char* pBuf, int iLen) { return SSL_write(ctx.ssl, pBuf, iLen); }

public:
	socket_t	fd;

	int(*fWrite)(CSocket&, const char*, int);
	int(*fRead)(CSocket&, char*, int);

private:
	SSL_CTX*	ctx;
	SSL*		ssl;

};

//////////////////////////////////////////////////////////////////////////
CHttpClient::CElement::CElement()
{
	m_pHead = nullptr;
	m_pTail = nullptr;
}

CHttpClient::CElement::~CElement()
{
	Cleanup();
}

void
CHttpClient::CElement::AddElem(const char* pszName, const char* pszValue)
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

const char*
CHttpClient::CElement::GetElem(const char* pszKey)
{
	Elem* pTemp = m_pHead;
	for (; pTemp; pTemp=pTemp->pNext) {
		if (strcmp(pTemp->pName, pszKey) == 0) {
			return pTemp->pValue;
		}
	}
	return nullptr;
}

int
CHttpClient::CElement::Build(char* pszBuf, int iSize)
{
	int iOffset = 0;	
	for (Elem* pCurr = m_pHead; pCurr != nullptr; pCurr = pCurr->pNext) {
		iOffset += sprintf_s(pszBuf + iOffset, iSize - iOffset, "%s: %s\r\n", pCurr->pName, pCurr->pValue);
	}
	iOffset += sprintf_s(pszBuf + iOffset, iSize - iOffset, "\r\n");

	return iOffset;
}

void
CHttpClient::CElement::Cleanup()
{
	Elem* pHead = m_pHead;
	for (Elem* pCurr = m_pHead; pCurr != nullptr;) {
		pHead = pHead->pNext;
		delete pCurr;
		pCurr = pHead;
	}
}

//////////////////////////////////////////////////////////////////////////
CHttpClient::CHttpClient()
{
	m_cbRead = nullptr;
	m_pHandle = nullptr;

	m_pszUrl = nullptr;
	m_pszPath = nullptr;
}

CHttpClient::~CHttpClient()
{
	if (m_pszUrl) {
		free(m_pszUrl);
		m_pszUrl = nullptr;
	}
}

int32_t
CHttpClient::Initialize(const char* pszUrl, CBRead cbRead, void* pHandle)
{
	if (m_pszUrl) {
		free(m_pszUrl);
	}

	m_pszUrl = URL::StrDup(pszUrl);
	m_cbRead = cbRead;
	m_pHandle = pHandle;

	URL::Parser(m_pszUrl, m_szProt, m_szAddr, m_iPort, m_pszPath);

	m_elemReq.Cleanup();
	m_elemReq.AddElem("Accept", "*/*");
	m_elemReq.AddElem("Connection", "Keep-Alive");
	m_elemReq.AddElem("Host", m_szAddr);
	m_elemReq.AddElem("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/64.0.3282.140 Safari/537.36 Edge/17.17134");

	return 0;
}

int
CHttpClient::Request()
{
	const int BUF_SIZ = 8192;
	char szBuf[BUF_SIZ] = { 0 };

	m_elemRsp.Cleanup();

	int iOffset = 0;
	iOffset = sprintf_s(szBuf, "GET %s HTTP/1.1\r\n", m_pszPath);
	iOffset += m_elemReq.Build(szBuf + iOffset, BUF_SIZ - iOffset);

	CSocket sock;
	if (sock.Initialize(m_szProt, m_szAddr, m_iPort) == -1) {
		return -1;
	}

	sock.fWrite(sock, szBuf, iOffset);

	char* pBody = nullptr;
	int iResult = 0;
	int iLength = 0;
	struct timeval tv = { 0, 100000 };
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(sock.fd, &rfds);

	for (iOffset=0; ;) {
		iResult = select(0, &rfds, nullptr, nullptr, &tv);
		if (iResult == 0) {
			continue;
		}
		if (iResult < 0) {
			break;
		}

		iResult = sock.fRead(sock, szBuf + iOffset, BUF_SIZ - iOffset);
		if (iResult < 1) {
			break;
		}
		iOffset += iResult;

		if (pBody == nullptr) {
			pBody = strstr(szBuf, "\r\n\r\n");
			if (pBody == nullptr) {
				continue;
			}
			pBody[2] = 0;
			pBody[3] = 0;
			pBody = pBody + 4;
			printf("%s", szBuf);
		
			int iHdrLen = int(pBody - szBuf);
			ParserHeader(szBuf, iHdrLen);

			iOffset -= iHdrLen;
			memmove_s(szBuf, BUF_SIZ, pBody, iOffset);

			const char* pLength = m_elemRsp.GetElem("Content-Length");
			if (pLength != nullptr) {
				iLength = atoi(pLength);
			}
		}

		szBuf[iOffset] = 0;
		if (m_cbRead) {
			m_cbRead(szBuf, iOffset, m_pHandle);
		}

		if (iLength>0 && iOffset >= iLength) {
			break;
		}
		iOffset = 0;
	}

	

	return 0;
}

int
CHttpClient::ParserHeader(const char* pBuf, int iLen)
{
	const char* pTmp = pBuf;

	char szProt[32] = { 0 };
	int iCode = 0;
	char szDesc[256] = { 0 };
	int i = 0;
	for (int j=0; pTmp[i]&&i<32; i++) {
		if (pTmp[i] == ' ') {
			szProt[j] = 0;
			i++;
			while (pTmp[i] == ' ') { i++; }

			for (j = 0; pTmp[i]; i++) {
				if (pTmp[i] == ' ') {
					i++;
					while (pTmp[i] == ' ') { i++; }
					for (j=0; pTmp[i]!='\r'&&pTmp[i]!=0; j++,i++) {
						szDesc[j] = pTmp[i]; 
					}
					szDesc[j] = 0;
					if (pTmp[i] == '\r' && pTmp[i + 1] == '\n') {
						i += 2;
						return ParserElement(pTmp + i, iLen - i);
					}
					return -1;
				} else {
					iCode = iCode * 10 + (pTmp[i] - '0');
				}
			}
		} else {
			szProt[j++] = pTmp[i];
		}		
	}

	return -1;
}

int
CHttpClient::ParserElement(const char* pBuf, int iLen)
{
	const char* pKey = nullptr;
	const char* pVal = nullptr;
	char* pTmp = (char*)pBuf;
	pKey = pBuf;
	for (int i = 0; pTmp[i]; i++) {
		if (pTmp[i] == ':') {
			int k = i;
			pTmp[i++] = 0;
			while (pTmp[i] == ' ') i++;
			for (pVal = pTmp + i; pTmp[i]; i++) {
				if (pTmp[i] == '\r') {
					int v = i;
					pTmp[i] = 0;
					m_elemRsp.AddElem(pKey, pVal);
					pTmp[i++] = '\r';
					if (pTmp[i] == '\n') {
						i++;
						pKey = pTmp + i;
						break;
					}
				}
			}
			pTmp[k] = ':';
		} else if (pTmp[i] == '\r') {
			return -1;
		}
	}

	return 0;
}

const char*
CHttpClient::GetLine(const char* pBuf, int iLen, const char*& pNext)
{
	const char* pTmp = pBuf;
	return 0;
}
