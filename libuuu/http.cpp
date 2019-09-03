#ifdef _WIN32
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "Ws2_32.lib")
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

using namespace std;

HttpStream::HttpStream()
{
	m_buff.empty();
#ifdef _WIN32
	WSADATA wsaData;
	int result;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData))
		set_last_err_string("WSA setup error");
#endif
}

int HttpStream::HttpGetHeader(std::string host, std::string path)
{
	int ret;
	addrinfo *pAddrInfo;

	if (getaddrinfo(host.c_str(), "80", 0, &pAddrInfo))
	{
		set_last_err_string("get network address error");
		return -1;
	}

	m_socket = socket(pAddrInfo->ai_family, pAddrInfo->ai_socktype, pAddrInfo->ai_protocol);

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
		size_t ret = 0;
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
#ifdef _WIN32
	closesocket(m_socket);
	WSACleanup();
#else
	close(m_socket);
#endif
}
