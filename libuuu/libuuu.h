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

#ifdef __cplusplus
#define EXT extern "C"
#else
#define EXT
#endif

/**
 * Get Last error string
 * @return last error string
*/
EXT const char * get_last_err_string();

/**
* Get Last error code
* @return last error code
*/
EXT int get_last_err();

EXT const char * get_version_string();

/**
 * 1.0.1
 * bit[31:24].bit[23:16].bit[15:0]
 */

EXT int get_version();

enum NOTIFY_TYPE
{
	NOTIFY_CMD_START,	/* str is command name*/
	NOTIFY_CMD_END,	/* status show command finish status. 0 is success. Other failure.*/
	NOTIFY_PHASE_INDEX,/*Current running phase*/
	NOTIFY_CMD_INDEX,  /*Current running command index*/
	NOTIFY_TRANS_SIZE,  /*Total size*/
	NOTIFY_TRANS_POS,   /*Current finished transfer pos*/
};

struct notify
{
	NOTIFY_TYPE type;
	union
	{
		int status;
		int index;
		int total;
		char *str;
	};
};

typedef int (*notify_fun)(struct notify, void *data);

int register_notify_callback(notify_fun f, void *data);
int unregister_notify_callback(notify_fun f);

int run_cmd(const char * cmd);

#endif