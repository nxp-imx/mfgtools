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
#ifndef __libuuu___
#define __libuuu___

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
#define EXT extern "C"
#else
#define EXT
#endif

/**
 * Get Last error string
 * @return last error string
*/
EXT const char * uuu_get_last_err_string();

/**
* Get Last error code
* @return last error code
*/
EXT int uuu_get_last_err();

EXT const char * uuu_get_version_string();

/**
 * 1.0.1
 * bit[31:24].bit[23:12].bit[11:0]
 */

EXT int uuu_get_version();



struct uuu_notify
{
	enum NOTIFY_TYPE
	{
		NOTIFY_CMD_TOTAL,
		NOTIFY_CMD_START,	/* str is command name*/
		NOTIFY_CMD_END,	    /* status show command finish status. 0 is success. Other failure.*/
		NOTIFY_CMD_INDEX,   /*Current running command index*/

		NOTIFY_CMD_INFO,	/* Status info string */

		NOTIFY_PHASE_TOTAL,
		NOTIFY_PHASE_INDEX, /*Current running phase*/

		NOTIFY_TRANS_SIZE,  /*Total size*/
		NOTIFY_TRANS_POS,   /*Current finished transfer pos*/

		NOTIFY_WAIT_FOR,
		NOTIFY_DEV_ATTACH,

		NOTIFY_DECOMPRESS_START,
		NOTIFY_DECOMPRESS_SIZE,
		NOTIFY_DECOMPRESS_POS, 

		NOTIFY_DOWNLOAD_START,
		NOTIFY_DOWNLOAD_END,
		NOTIFY_THREAD_EXIT,

		NOTIFY_DONE,
	};

	NOTIFY_TYPE type;
	uint64_t id;
	uint64_t timestamp;
	union
	{
		int status;
		size_t index;
		size_t total;
		char *str;
	};
};

typedef int (*uuu_notify_fun)(struct uuu_notify, void *data);

int uuu_register_notify_callback(uuu_notify_fun f, void *data);
int uuu_unregister_notify_callback(uuu_notify_fun f);

typedef int(*uuu_show_cfg)(const char *pro, const char *chip, const char *comp, uint16_t vid, uint16_t pid, uint16_t bcdlow, uint16_t bcdhigh, void *p);
int uuu_for_each_cfg(uuu_show_cfg fn, void *p);

typedef int(*uuu_ls_file)(const char *path, void *p);
int uuu_for_each_ls_file(uuu_ls_file fn, const char *path, void *p);

typedef int(*uuu_ls_usb_devices)(const char *path, const char *chip, const char *pro,  uint16_t vid, uint16_t pid, uint16_t bcd, const char *serial_no, void *p);
int uuu_for_each_devices(uuu_ls_usb_devices fn, void *p);

int uuu_run_cmd(const char * cmd, int dry);
int uuu_run_cmd_script(const char *script, int dry);

int uuu_auto_detect_file(const char * filename);
int uuu_wait_uuu_finish(int deamon, int dry);
int uuu_add_usbpath_filter(const char *path);
int uuu_add_usbserial_no_filter(const char *serial_no);

/*Set timeout wait for known devices appeared*/
int uuu_set_wait_timeout(int timeout_in_seconds);
/*Set timeout wait for next devices appeared, e.g. FB -> FBK*/
int uuu_set_wait_next_timeout(int timeout_in_seconds);
/*Set usb device polling period */
void uuu_set_poll_period(int period_in_milliseconds);
/*
 * bit 0:15 for libusb
 * bit 16:31 for uuu
 */
void uuu_set_debug_level(uint32_t mask);

/*
 * 0 disable small memory mode, buffer all data, it is used for multi-board program.
 */
void uuu_set_small_mem(uint32_t val);

#define MAX_USER_LEN 128
typedef int (*uuu_askpasswd)(char* prompt, char user[MAX_USER_LEN], char passwd[MAX_USER_LEN]);
int uuu_set_askpasswd(uuu_askpasswd ask);

enum class bmap_mode {
	Default,
	Force,
	Ignore
};

/*Get .bmap handling mode*/
static inline bmap_mode uuu_get_bmap_mode() {
	extern bmap_mode g_bmap_mode;
	return g_bmap_mode;
}

static inline int uuu_force_bmap() {
	return uuu_get_bmap_mode() == bmap_mode::Force;
}

static inline int uuu_ignore_bmap() {
	return uuu_get_bmap_mode() == bmap_mode::Ignore;
}

#endif
