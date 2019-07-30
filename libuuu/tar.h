#include <stdint.h>
#include <map>
#include <string>
#include <vector>
#include "zlib.h"
#include <memory>
#include "buffer.h"
#include "liberror.h"
#include "libuuu.h"
#include <stdio.h>
#include <string.h>

using namespace std;

#define TAR_BLOCK_SIZE 512

#pragma pack(1)
struct Tar_header
{
	uint8_t name[100];
	uint8_t mode[8];
	uint8_t owner_id[8];
	uint8_t group_id[8];
	uint8_t size[12];
	uint8_t modi_time[12];
	uint8_t checksum[8];
	uint8_t type[1];
	uint8_t linkname[100];
	uint8_t ustar[6];
	uint8_t version[2];
	uint8_t uname[32];
	uint8_t gname[32];
	uint8_t major_num[8];
	uint8_t minor_num[8];
	uint8_t prefix[155];
};
#pragma pack()

class Tar_file_Info
{
public:
	string filename;
	uint64_t offset;
	uint64_t size;

};


class Tar
{
	string m_tarfilename;

public:
	map<string, Tar_file_Info> m_filemap;
	int Open(string filename);
	bool check_file_exist(string filename);
	int get_file_buff(string filename, shared_ptr<FileBuffer> p );
};