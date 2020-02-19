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

#define ROM_INFO_HID					   0x1
#define ROM_INFO_HID_MX23				   0x2
#define ROM_INFO_HID_MX50				   0x4
#define ROM_INFO_HID_MX6				   0x8
#define ROM_INFO_HID_SKIP_DCD			  0x10
#define ROM_INFO_HID_MX8_MULTI_IMAGE	  0x20
#define ROM_INFO_HID_MX8_STREAM			  0x40
#define ROM_INFO_HID_UID_STRING			  0x80
#define ROM_INFO_HID_NO_CMD				 0x400
#define ROM_INFO_SPL_JUMP				 0x800
#define ROM_INFO_HID_EP1				0x1000
#define ROM_INFO_HID_PACK_SIZE_1020		0x2000
#define ROM_INFO_HID_SDP_NO_MAX_PER_TRANS	0x4000
#define ROM_INFO_AUTO_SCAN_UBOOT_POS		0x8000

#include <stdint.h>
#include <stddef.h>

struct ROM_INFO
{
	const char * m_name;
	uint32_t    free_addr;
	uint32_t	flags;
};

ROM_INFO * search_rom_info(const char *s);
ROM_INFO * search_rom_info(ConfigItem *item);

size_t GetContainerActualSize(shared_ptr<FileBuffer> p, size_t offset);
size_t GetFlashHeaderSize(shared_ptr<FileBuffer> p, size_t offset = 0);
