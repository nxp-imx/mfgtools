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
#include <string>
#include "sdps.h"
#include "hidreport.h"
#include "liberror.h"
#include "libcomm.h"
#include "buffer.h"
#include "rominfo.h"

int SDPSCmd::run(CmdCtx *pro)
{
	ROM_INFO * rom;
	rom = search_rom_info(pro->m_config_item);
	if (rom == NULL)
	{
		string_ex err;
		err.format("%s:%d can't get rom info", __FUNCTION__, __LINE__);
		set_last_err_string(err);
		return -1;
	}

	HIDTrans dev;
	if (rom->flags & ROM_INFO_HID_EP1)
		dev.set_hid_out_ep(1);

	if(dev.open(pro->m_dev))
		return -1;

	shared_ptr<FileBuffer> p = get_file_buffer(m_filename);
	if (!p)
		return -1;

	HIDReport report(&dev);
	report.m_skip_notify = false;

	if (rom->flags & ROM_INFO_HID_PACK_SIZE_1020)
		report.set_out_package_size(1020);

	return report.write(p->data(), p->size(),  2);
}