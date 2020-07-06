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

#include "libuuu.h"

#include <chrono>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>

using namespace std;

static map<uuu_notify_fun, void*> g_notification_map;
static mutex g_mutex_notify;

using namespace std::chrono;
static const time_point<steady_clock> g_now = steady_clock::now();

int uuu_register_notify_callback(uuu_notify_fun f, void *data)
{
	std::lock_guard<mutex> lock(g_mutex_notify);

	return g_notification_map.emplace(f, data).second ? 0 : 1;
}

int uuu_unregister_notify_callback(uuu_notify_fun f)
{
	std::lock_guard<mutex> lock(g_mutex_notify);

	return g_notification_map.erase(f) > 0 ? 0 : 1;
}

void call_notify(struct uuu_notify nf)
{
	//Change RW lock later;
	std::lock_guard<mutex> lock(g_mutex_notify);

	nf.id = std::hash<std::thread::id>{}(std::this_thread::get_id());
	nf.timestamp = static_cast<uint64_t>(
		duration_cast<milliseconds>(steady_clock::now() - g_now).count());

	for (const auto &item : g_notification_map)
	{
		try {
			item.first(nf, item.second);
		} catch (const std::exception& e) {
			std::cerr << "notify exception: " << e.what() << std::endl;
		}
	}
}
