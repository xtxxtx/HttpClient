// HttpClient.h

#ifndef HTTPCLIENT_H_
#define	HTTPCLIENT_H_

#include <stdint.h>

#include <string>
#include <map>

#ifdef _MSC_VER
#if defined(_WIN64)
typedef unsigned __int64	socket_t;
#else
typedef unsigned int		socket_t;
#endif

#define	xclose				closesocket

#else
typedef int					socket_t;

#define xclose				close

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
		char*		pName;
		char*		pValue;
		tagElem*	pNext;

		tagElem() {
			pName = 0;
			pValue = 0;
			pNext = nullptr;
		}
		tagElem(const char* name, const char* value) {
			pName = URL::StrDup(name);
			pValue = URL::StrDup(value);
			pNext = nullptr;
		}
		~tagElem() {
			if (pName) free(pName);
			if (pValue) free(pValue);
			pNext = nullptr;
		}
	} Elem;

	class CElement
	{
	public:
		CElement();
		~CElement();

		void			AddElem(const char* pszKey, const char* pszVal);

		int				Build(char* pszBuf, int iSize);

		const char*		GetElem(const char* pszKey);

		void			Cleanup();

	private:
		Elem*	m_pHead;
		Elem*	m_pTail;
	};

	typedef std::map<std::string, std::string>	MAP_STRING;

	class CResponse
	{
	public:
		CResponse();
		~CResponse();

		MAP_STRING		m_mapString;
	};

public:
	CHttpClient();
	~CHttpClient();

	int32_t			Initialize(const char* pszUrl, CBRead cbRead, void* pHandle);

	int				Request();

	void			Close();

private:
	socket_t		Connect(const char* pszAddr, uint16_t iPort);

	int				ParserHeader(const char* pBuf, int iLen);

	int				ParserElement(const char* pBuf, int iLen);

	const char*		GetLine(const char* pBuf, int iLen, const char*& pNext);

private:
	socket_t		m_iFd;

	CBRead			m_cbRead;
	void*			m_pHandle;

	char*			m_pszUrl;
	char			m_szProt[8];
	char			m_szAddr[256];
	uint16_t		m_iPort;
	const char*		m_pszPath;

	CElement		m_elemReq;
	CElement		m_elemRsp;
};

#endif // !HTTPCLIENT_H_
