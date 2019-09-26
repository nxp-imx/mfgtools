#ifdef _WIN32
#include <ws2tcpip.h>
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "Winhttp.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#define INVALID_SOCKET -1
#include <unistd.h>
#endif

#include "http.h"
#include "libuuu.h"
#include "liberror.h"
#include <string.h>
#include <locale>
#include <codecvt>

using namespace std;

#ifdef _WIN32
/* Win32 implement*/

HttpStream::HttpStream()
{
	m_buff.empty();
	m_socket = 0;
	m_hConnect = 0;
	m_hSession = 0;
	m_hRequest = 0;
}

int HttpStream::HttpGetHeader(std::string host, std::string path, int port)
{

	m_hSession = WinHttpOpen(L"WinHTTP UUU/1.0",
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS, 0);

	if (!m_hSession)
	{
		set_last_err_string("fail WinHttpOpen");
		return -1;
	}

	wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;
	wstring whost = converter.from_bytes(host);

	if (m_hSession)
		m_hConnect = WinHttpConnect(m_hSession, whost.c_str(),
			port, 0);

	if (!m_hConnect)
	{
		set_last_err_string("Fail Connection");
		return -1;
	}

	wstring wpath = converter.from_bytes(path);

	m_hRequest = WinHttpOpenRequest(m_hConnect, L"GET", wpath.c_str(),
			NULL, WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES,
			port==443?WINHTTP_FLAG_SECURE:0);

	BOOL  bResults = FALSE;
	if (!m_hRequest)
	{
		set_last_err_string("Fail WinHttpOpenRequest");
		return -1;
	}

	bResults = WinHttpSendRequest(m_hRequest,
			WINHTTP_NO_ADDITIONAL_HEADERS, 0,
			WINHTTP_NO_REQUEST_DATA, 0,
			0, 0);

	if (!bResults)
	{
		set_last_err_string("Fail WinHttpSendRequest");
		return -1;
	}

	bResults = WinHttpReceiveResponse(m_hRequest, NULL);

	if (!bResults)
	{
		set_last_err_string("Fail WinHttpReceiveResponse");
		return -1;
	}

	DWORD status = 0;
	DWORD dwSize = sizeof(status);
	WinHttpQueryHeaders(m_hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
		WINHTTP_HEADER_NAME_BY_INDEX, &status,
		&dwSize, WINHTTP_NO_HEADER_INDEX);

	if (status != HTTP_STATUS_OK)
	{
		set_last_err_string("HTTP status is not okay");
		return -1;
	}
	return 0;
}

size_t HttpStream::HttpGetFileSize()
{
	DWORD dwSize = 0;
	BOOL  bResults = FALSE;
	wstring out;

	WinHttpQueryHeaders(m_hRequest, WINHTTP_QUERY_CONTENT_LENGTH,
		WINHTTP_HEADER_NAME_BY_INDEX, NULL,
		&dwSize, WINHTTP_NO_HEADER_INDEX);

	// Allocate memory for the buffer.
	if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		out.resize(dwSize / sizeof(WCHAR));

		// Now, use WinHttpQueryHeaders to retrieve the header.
		bResults = WinHttpQueryHeaders(m_hRequest,
			WINHTTP_QUERY_CONTENT_LENGTH,
			WINHTTP_HEADER_NAME_BY_INDEX,
			(LPVOID)out.c_str(), &dwSize,
			WINHTTP_NO_HEADER_INDEX);
	}
	return _wtoll(out.c_str());
}

int HttpStream::HttpDownload(char *buff, size_t sz)
{
	DWORD dwSize = 0;
	DWORD dwDownloaded = 0;
	while (sz)
	{
		if (!WinHttpQueryDataAvailable(m_hRequest, &dwSize))
		{
			set_last_err_string("WinHttpQueryDataAvailable");
			return -1;
		}

		if (dwSize > sz)
			dwSize = sz;

		if (!WinHttpReadData(m_hRequest, (LPVOID)buff,
			dwSize, &dwDownloaded))
		{
			set_last_err_string("Fail at WinHttpReadData");
			return -1;
		}
		buff += dwDownloaded;
		sz -= dwDownloaded;
	}
	return 0;
}

HttpStream::~HttpStream()
{
	if (m_hRequest)
		WinHttpCloseHandle(m_hRequest);
	if (m_hConnect)
		WinHttpCloseHandle(m_hConnect);
	if (m_hSession)
		WinHttpCloseHandle(m_hSession);
}

#else

HttpStream::HttpStream()
{
	m_buff.empty();
}

int HttpStream::HttpGetHeader(std::string host, std::string path, int port)
{
	int ret;
	addrinfo *pAddrInfo;

	if (getaddrinfo(host.c_str(), "80", 0, &pAddrInfo))
	{
		set_last_err_string("get network address error");
		return -1;
	}

	m_socket = socket(pAddrInfo->ai_family, pAddrInfo->ai_socktype, pAddrInfo->ai_protocol);

	struct timeval tv;
	tv.tv_sec = 10;
	tv.tv_usec = 0;

	setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

	if (m_socket == INVALID_SOCKET)
	{
		set_last_err_string("Can't get sock");
		return -1;
	}

	if (connect(m_socket, pAddrInfo->ai_addr, pAddrInfo->ai_addrlen))
	{
		set_last_err_string("connect error");
		return -1;
	}

	string request = "GET " + path + " HTTP/1/1\nHost: " + host + "\n\n";

	ret = send(m_socket, request.c_str(), request.size(), 0);

	if (ret != request.size())
	{
		set_last_err_string("http send error");
		return -1;
	}

	m_buff.resize(1024);

	ret = recv(m_socket, (char*)m_buff.data(), m_buff.size(), 0);

	if (ret < 0)
	{
		set_last_err_string("http recv Error");
		return -1;
	}

	int i;
	for (i = 0; i < 1024 - 4; i++)
	{
		if (m_buff[i] == 0xd &&
			m_buff[i + 1] == 0xa &&
			m_buff[i + 2] == 0xd &&
			m_buff[i + 3] == 0xa)
		{
			break;
		}
	}

	if (i >= 1024 - 4)
	{
		set_last_err_string("Can't find termaniate");
		return -1;
	}

	m_data_start = i + 4;

	string str;
	str.resize(i + 2);
	memcpy((void*)str.c_str(), m_buff.data(), i + 2);

	if (parser_response(str))
		return -1;

	return 0;
}

size_t HttpStream::HttpGetFileSize()
{
	return atoll(m_response["Content-Length"].c_str());
}

int HttpStream::parser_response(string rep)
{
	size_t pos = rep.find("\r\n");
	if (pos == string::npos)
	{
		set_last_err_string("Can't find \r\n");
		return -1;
	}

	string str = rep.substr(0, pos);
	if (str != "HTTP/1.1 200 OK")
	{
		set_last_err_string(str);
		return -1;
	}

	m_response.clear();

	while (pos != string::npos)
	{
		pos += 2;
		size_t split = rep.find(':', pos);
		if (split == string::npos)
			break;
		string key = rep.substr(pos, split - pos);
		pos = rep.find("\r\n", pos);
		string value = rep.substr(split + 1, pos - split - 1);
		m_response[key] = value;
	}

	return 0;
}

int HttpStream::HttpDownload(char *buff, size_t sz)
{
	size_t left = 0;
	if (m_data_start < m_buff.size())
		left = m_buff.size() - m_data_start;
	
	size_t trim_transfered = 0;

	if (left)
	{
		
		trim_transfered = sz;
		if (trim_transfered > left)
			trim_transfered = left;

		memcpy(buff, m_buff.data() + m_data_start, trim_transfered);
		m_data_start += trim_transfered;
	}

	if (trim_transfered < sz)
	{
		int ret = 0;
		sz -= trim_transfered;
		buff += trim_transfered;
		while ( (ret = recv(m_socket, buff, sz, 0)) > 0)
		{
			buff += ret;
			sz -= ret;
		}

		if (ret < 0)
		{
			set_last_err_string("recv error");
			return -1;
		}
	}

	return 0;
}

HttpStream::~HttpStream()
{
	close(m_socket);
}

#endif
