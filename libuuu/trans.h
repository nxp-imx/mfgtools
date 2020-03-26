/*
* Copyright 2018 NXP.
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
#pragma once

#include <atomic>
#include <string>
#include <vector>

using namespace std;

class TransBase
{
public:
	string m_path;
	void * m_devhandle = nullptr;
	virtual ~TransBase();
	virtual int open(void *) { return 0; }
	virtual int close() { return 0; }
	virtual int write(void *buff, size_t size) = 0;
	virtual int read(void *buff, size_t size, size_t *return_size) = 0;
	int write(vector<uint8_t> & buff) { return write(buff.data(), buff.size()); }
	int read(vector<uint8_t> &buff)
	{
		size_t size;
		const auto ret = read(buff.data(), buff.size(), &size);
		if (ret)
			return ret;
		buff.resize(size);
		return ret;
	}
};

class EPInfo
{
public:
	constexpr EPInfo() = default;
	constexpr EPInfo(int a, int size) : addr{a}, package_size{size} {}
	int addr = 0;
	int package_size = 64;
};

class USBTrans : public TransBase
{
public:
	vector<EPInfo> m_EPs;
	int open(void *p) override;
	int close() override;
};
class HIDTrans : public USBTrans
{
	int m_set_report = 9;
	int m_outEP = 0;
public:
	int m_read_timeout = 1000;
	void set_hid_out_ep(int ep) noexcept { m_outEP = ep; }

	int open(void *p) override
	{
		if (USBTrans::open(p))
			return -1;

		for (const auto &ep : m_EPs)
		{
			if (ep.addr > 0 && ((ep.addr & 0x80) == 0))
				m_outEP = ep.addr;
		}

		return 0;
	}
	~HIDTrans() override { if (m_devhandle) close();  m_devhandle = nullptr; }
	int write(void *buff, size_t size) override;
	int read(void *buff, size_t size, size_t *return_size) override;
};

class BulkTrans : public USBTrans
{
public:
	EPInfo m_ep_in;
	EPInfo m_ep_out;
	size_t m_MaxTransPreRequest = 0x100000;
	int m_b_send_zero = 0;
	uint64_t m_timeout = 2000;

	int open(void *p) override;

	~BulkTrans() override { if (m_devhandle) close();  m_devhandle = nullptr; }
	int write(void *buff, size_t size) override;
	int read(void *buff, size_t size, size_t *return_size) override;
};

int polling_usb(std::atomic<int>& bexit);
