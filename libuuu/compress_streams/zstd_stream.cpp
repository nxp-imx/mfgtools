#include "zstd_stream.h"

#include "../buffer.h"

size_t ZstdStream::decompress_size(const std::string& backfile)
{
	std::shared_ptr<FileBuffer> inp = get_file_buffer(backfile, true);
	if (inp == nullptr)
	{
		return 0;
	}
	size_t sz = ZSTD_DStreamInSize();
	std::shared_ptr<DataBuffer> pb = inp->request_data(0, sz);
	if (!pb)
		return 0;
	size_t decompressed_sz = ZSTD_getFrameContentSize(pb->data(), sz);

	return decompressed_sz;
}
