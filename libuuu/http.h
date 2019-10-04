#include <string>
#include <vector>
#include <map>

class HttpStream
{
	std::vector<uint8_t> m_buff;
	uintptr_t	m_socket;
	std::map<std::string, std::string> m_response;
	size_t			m_data_start;

#ifdef _WIN32
	void far * m_hSession;
	void far * m_hConnect;
	void far * m_hRequest;
#endif

	void * m_ssl;
	int parser_response(std::string rep);
public:
	HttpStream();
	int HttpGetHeader(std::string host, std::string path, int port = 80);
	size_t HttpGetFileSize();
	uint64_t HttpGetModifyTime();
	int HttpDownload(char *buff, size_t sz);
	~HttpStream();
	int RecvPacket(char *buff, size_t sz);
	int SendPacket(char *buff, size_t sz);
};
