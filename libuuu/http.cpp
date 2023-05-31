/*
 * Copyright 2019, 2023 NXP.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of the NXP Semiconductor nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


#ifdef _WIN32
//	request += "Connection: Keep-Alive\n";
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
#include "libcomm.h"
#include <string.h>
#include <locale>
#include <codecvt>

uuu_askpasswd g_ask_passwd;
int uuu_set_askpasswd(uuu_askpasswd ask)
{
	g_ask_passwd = ask;
	return 0;
}

map<string, pair<string, string>> g_passwd_map;

#ifdef UUUSSL
#include <openssl/ssl.h>
#include <openssl/err.h>

static const char* base64_table =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/";

static string base64_encode(string str)
{
	string ret;
	for (size_t i = 0; i < str.length(); i += 3)
	{
		ret.push_back(base64_table[(str[i] >> 2) & 0x3f]);
		if (i + 1 < str.length())
		{
			int index;
			index = ((str[i] & 3) << 4)
				+ ((str[i + 1] >> 4) & 0xf);
			ret.push_back(base64_table[index]);
		}
		else
		{
			ret.push_back(base64_table[(str[i] & 0x3) << 4]);
			ret.push_back('=');
			ret.push_back('=');
			return ret;
		}

		if (i + 2 < str.length())
		{
			int index;
			index = (((str[i + 1]) & 0xF) << 2)
				+ (((str[i + 2]) >> 6) & 0x3);
			ret.push_back(base64_table[index]);
			index = str[i + 2] & 0x3f;
			ret.push_back(base64_table[index]);

		}
		else
		{
			ret.push_back(base64_table[((str[i + 1]) & 0xF) << 2]);
			ret.push_back('=');
			return ret;
		}
	}
	return ret;
}

class CUUUSSL
{
public:
	CUUUSSL()
	{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
		SSL_library_init();
		SSLeay_add_ssl_algorithms();
		SSL_load_error_strings();
#else
		OPENSSL_init_ssl(0, nullptr);
		SSLeay_add_ssl_algorithms();
#endif
	}
	~CUUUSSL()
	{
	}
};

static CUUUSSL g_uuussl;

#endif
using namespace std;

#ifdef _WIN32
/* Win32 implement*/

DWORD ChooseAuthScheme(DWORD dwSupportedSchemes)
{
	if (dwSupportedSchemes & WINHTTP_AUTH_SCHEME_NEGOTIATE)
		return WINHTTP_AUTH_SCHEME_NEGOTIATE;
	else if (dwSupportedSchemes & WINHTTP_AUTH_SCHEME_NTLM)
		return WINHTTP_AUTH_SCHEME_NTLM;
	else if (dwSupportedSchemes & WINHTTP_AUTH_SCHEME_PASSPORT)
		return WINHTTP_AUTH_SCHEME_PASSPORT;
	else if (dwSupportedSchemes & WINHTTP_AUTH_SCHEME_DIGEST)
		return WINHTTP_AUTH_SCHEME_DIGEST;
	else if (dwSupportedSchemes & WINHTTP_AUTH_SCHEME_BASIC)
		return WINHTTP_AUTH_SCHEME_BASIC;

	return 0;
}

HttpStream::HttpStream()
{
	m_buff.empty();
	m_hConnect = 0;
	m_hSession = 0;
	m_hRequest = 0;
}

int HttpStream::HttpGetHeader(std::string host, std::string path, int port, bool ishttps)
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
			nullptr, WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES,
			ishttps ?WINHTTP_FLAG_SECURE:0);

	BOOL  bResults = FALSE;
	if (!m_hRequest)
	{
		set_last_err_string("Fail WinHttpOpenRequest");
		return -1;
	}

	DWORD dwProxyAuthScheme = 0;
	int retry = 3;

	pair<string, string> up = g_passwd_map[host];

	while (1)
	{
		DWORD status = 0;
		DWORD dwSize = sizeof(status);

		bResults = WinHttpSendRequest(m_hRequest,
			WINHTTP_NO_ADDITIONAL_HEADERS, 0,
			WINHTTP_NO_REQUEST_DATA, 0,
			0, 0);

		// End the request.
		if (bResults)
			bResults = WinHttpReceiveResponse(m_hRequest, NULL);

		// Resend the request in case of
		// ERROR_WINHTTP_RESEND_REQUEST error.
		if (!bResults && GetLastError() == ERROR_WINHTTP_RESEND_REQUEST)
			continue;

		// Check the status code.
		if (bResults)
			bResults = WinHttpQueryHeaders(m_hRequest,
				WINHTTP_QUERY_STATUS_CODE |
				WINHTTP_QUERY_FLAG_NUMBER,
				NULL,
				&status,
				&dwSize,
				NULL);

		if (bResults)
		{
			DWORD dwSupportedSchemes;
			DWORD dwFirstScheme;
			DWORD dwSelectedScheme;
			DWORD dwTarget;

			switch (status)
			{
			case HTTP_STATUS_OK: //200
				g_passwd_map[host] = up;
				return 0;

			case HTTP_STATUS_DENIED: //401
				// The server requires authentication.
				if(g_passwd_map[host].first.empty())
				{
					char user[MAX_USER_LEN];
					char passwd[MAX_USER_LEN];
					if (g_ask_passwd((char*)host.c_str(), user, passwd))
						return -1;

					up.first = user;
					up.second = passwd;
				}
				// Obtain the supported and preferred schemes.
				bResults = WinHttpQueryAuthSchemes(m_hRequest,
					&dwSupportedSchemes,
					&dwFirstScheme,
					&dwTarget);

				// Set the credentials before resending the request.
				if (bResults)
				{
					dwSelectedScheme = ChooseAuthScheme(dwSupportedSchemes);

					std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;

					if (dwSelectedScheme == 0)
					{
						set_last_err_string("unsupported http auth");
						return -1;
					}
					else
						bResults = WinHttpSetCredentials(m_hRequest,
							dwTarget,
							dwSelectedScheme,
							convert.from_bytes(up.first).c_str(),
							convert.from_bytes(up.second).c_str(),
							NULL);
				}

				retry--;
				if (!retry)
					return -1;

				break;

			case HTTP_STATUS_PROXY_AUTH_REQ:
				set_last_err_string("unsupported proxy auth");
				return -1;

			default:
				g_passwd_map[host] = up;
				// The status code does not indicate success.
				string_ex str;
				str.format("Error. Status code %d returned.\n", status);
				set_last_err_string(str);
				return -1;
			}
		}
	}

	return -1;
}

size_t HttpStream::HttpGetFileSize()
{
	DWORD dwSize = 0;
	BOOL  bResults = FALSE;
	wstring out;

	WinHttpQueryHeaders(m_hRequest, WINHTTP_QUERY_CONTENT_LENGTH,
		WINHTTP_HEADER_NAME_BY_INDEX, nullptr,
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

int HttpStream::SendPacket(char *buff, size_t sz)
{
#ifdef UUUSSL
	if(m_ssl)
		return SSL_write((SSL*)m_ssl, buff, sz);
#endif
	return send(m_socket, buff, sz, 0);
}


int HttpStream::RecvPacket(char *buff, size_t sz)
{
#ifdef UUUSSL
	if(m_ssl)
		return SSL_read((SSL*)m_ssl, buff, sz);
#endif
	return recv(m_socket, buff, sz, 0);
}

class CAutoAddrInfo
{
addrinfo *m_p;
public:
	CAutoAddrInfo(addrinfo *pAddrInfo)
	{
		m_p = pAddrInfo;
	}
	~CAutoAddrInfo()
	{
		freeaddrinfo(m_p);
	}
};
#include <iostream>
int HttpStream::HttpGetHeader(std::string host, std::string path, int port, bool ishttps)
{
	int ret;
	addrinfo *pAddrInfo;
	char s_port[10];
	snprintf(s_port, 10, "%d", port);

	if (getaddrinfo(host.c_str(), s_port, 0, &pAddrInfo))
	{
		set_last_err_string("get network address error");
		return -1;
	}

	CAutoAddrInfo A(pAddrInfo);

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

	if(ishttps)
	{
#ifdef UUUSSL

		const SSL_METHOD* meth =
#if (OPENSSL_VERSION_NUMBER < 0x10100000L)
		TLSv1_2_client_method();
#else
		TLS_client_method();
#endif
		if(!meth)
		{
			set_last_err_string("Failure at TLSv1_2_client_method\n");
			return -1;
		}
		SSL_CTX *ctx = SSL_CTX_new (meth);
		if(!ctx)
		{
			set_last_err_string("Error create ssl ctx\n");
			return -1;
		}
		m_ssl = SSL_new (ctx);
		if(!m_ssl)
		{
			set_last_err_string("Error create SSL\n");
			return -1;
		}
		SSL_set_fd((SSL*)m_ssl, m_socket);
		if( SSL_connect((SSL*)m_ssl) <= 0)
		{
			set_last_err_string("error build ssl connection");
			return -1;
		}
#else
		set_last_err_string("Can't support https");
		return -1;
#endif
        }

	int retry = 3;
	pair<string, string> up = g_passwd_map[host];
	while(retry--)
	{
		string userpd = up.first + ":" + up.second;
		string httppath = path;

		if(ishttps)
			httppath = "https://" + host + path;

		string request = "GET " + httppath + " HTTP/1.1\r\n";
		request += "Host: " + host + "\r\n";

		if (!up.first.empty())
			request += "Authorization: Basic " + base64_encode(userpd) + "\r\n";

		request += "User-Agent:uuu\r\nAccept: */*\r\n";
		request += "\r\n";

		ret = SendPacket((char*)request.c_str(), request.size());
		if ((size_t)(ret) != request.size())
		{
			set_last_err_string("http send error");
			return -1;
		}

		m_buff.resize(1024);
		ret = RecvPacket((char*)m_buff.data(), m_buff.size());
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
			set_last_err_string("Can't find terminate");
			return -1;
		}

		m_data_start = i + 4;

		string str;
		str.resize(i + 2);
		memcpy((void*)str.c_str(), m_buff.data(), i + 2);

		int ret = parser_response(str);
		if (ret == ERR_ACCESS_DENIED)
		{
			if(g_passwd_map[host].first.empty())
			{
				char user[MAX_USER_LEN];
				char passwd[MAX_USER_LEN];
				if (g_ask_passwd((char*)host.c_str(), user, passwd))
					return -1;

				up.first = user;
				up.second = passwd;
			}
			continue;
		}
		else if(ret == 0)
		{
			g_passwd_map[host] = up;
			return 0;
		}
		g_passwd_map[host] = up;
	}

	return -1;
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
	if (str == "HTTP/1.1 401 Unauthorized")
		return ERR_ACCESS_DENIED;

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

	size_t trim_transferred = 0;

	if (left)
	{

		trim_transferred = sz;
		if (trim_transferred > left)
			trim_transferred = left;

		memcpy(buff, m_buff.data() + m_data_start, trim_transferred);
		m_data_start += trim_transferred;
	}

	if (trim_transferred < sz)
	{
		int ret = 0;
		sz -= trim_transferred;
		buff += trim_transferred;
		while (sz && ((ret = RecvPacket(buff, sz)) > 0))
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
#ifdef UUUSSL
	if(m_ssl)
	{
		SSL_CTX_free(SSL_get_SSL_CTX((SSL*)m_ssl));
		SSL_free((SSL*)m_ssl);
	}
#endif
}

#endif
