// HttpClient.h

#ifndef HTTPCLIENT_H_
#define	HTTPCLIENT_H_

#include <stdint.h>
#include <map>

#ifdef _MSC_VER
#if defined(_WIN64)
typedef unsigned __int64	socket_t;
#else
typedef unsigned int		socket_t;
#endif

#define	sclose				closesocket

#else
typedef int					socket_t;

#define sclose				close

#endif

namespace URL {
	char*	StrCpy(char* pDst, const char* pSrc);

	char*	StrDup(const char* pBuf);

	int		Parser(const char* pszUrl, char* pszProt, char* pszAddr, uint16_t& iPort, const char*& pszPath);
}

typedef int(*CBRead)(const char*, int, void*);

enum MENTHOD {
	HTTP_GET,
	HTTP_POST
};

//////////////////////////////////////////////////////////////////////////
class CHttpClient
{
	typedef struct tagElem {
		char		szName[64];
		char		szValue[512];
		tagElem*	pNext;

		tagElem() {
			szName[0] = 0;
			szValue[0] = 0;
			pNext = nullptr;
		}
		tagElem(const char* pszName, const char* pszValue) {
			URL::StrCpy(szName, pszName);
			URL::StrCpy(szValue, pszValue);
			pNext = nullptr;
		}
	} Elem;

public:
	CHttpClient();
	~CHttpClient();

	int32_t			Initialize(const char* pszUrl, CBRead cbRead, void* pHandle);

	void			AddElem(const char* pszKey, const char* pszVal);

	int				Request();

	void			Close();

private:
	socket_t		Connect(const char* pszAddr, uint16_t iPort);

	void			Cleanup(Elem* pHead);

private:
	socket_t		m_iFd;

	CBRead			m_cbRead;
	void*			m_pHandle;

	char*			m_pszUrl;
	char			m_szProt[8];
	char			m_szAddr[256];
	uint16_t		m_iPort;
	const char*		m_pszPath;

	Elem*			m_pHead;
	Elem*			m_pTail;
};

#endif // !HTTPCLIENT_H_
