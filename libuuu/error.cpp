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

#include "liberror.h"
#include "libuuu.h"
#include "libusb.h"

#include <mutex>
#include <atomic>

using namespace std;

static mutex g_last_error_str_mutex;
static string g_last_error_str;
static atomic<int> g_last_err_id;

static uint32_t g_debug_level;

int get_libusb_debug_level() noexcept
{
	return g_debug_level & 0xFFFF;
}

void uuu_set_debug_level(uint32_t mask)
{
	g_debug_level = mask;

#if LIBUSB_API_VERSION > 0x01000106 && !defined(FORCE_OLDLIBUSB)
		libusb_set_option(nullptr, LIBUSB_OPTION_LOG_LEVEL, get_libusb_debug_level());
#else
		libusb_set_debug(nullptr, get_libusb_debug_level());
#endif
}

const char * uuu_get_last_err_string()
{
	lock_guard<mutex> l(g_last_error_str_mutex);
	return g_last_error_str.c_str();
}

void set_last_err_string(const string &str)
{
	lock_guard<mutex> l(g_last_error_str_mutex);
	g_last_error_str = str;
}

int uuu_get_last_err()
{
	return g_last_err_id.load();
}

void set_last_err_id(int id)
{
	g_last_err_id = id;
}
