#include <string>
#include <vector>
#include <map>

class HttpStream
{
	std::vector<uint8_t> m_buff;
	UINT_PTR			m_socket;
	std::map<std::string, std::string> m_response;
	size_t			m_data_start;

	int parser_response(std::string rep);
public:
	HttpStream();
	int HttpGetHeader(std::string host, std::string path);
	size_t HttpGetFileSize();
	uint64_t HttpGetModifyTime();
	int HttpDownload(char *buff, size_t sz);
	~HttpStream();
};