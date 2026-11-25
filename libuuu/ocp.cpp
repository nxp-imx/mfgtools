/*
 * Copyright 2026 NXP.
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

/*
 * OCP (Open Compute Project) USB Device Recovery Interface.
 * Based on OCP Recovery Specification v1.1, December 2025.
 *
 * Wire protocol (section 8.5.1):
 *   Each OCP register access = one USB EP0 control transfer.
 *
 *   Write register (OUT):
 *     bmRequestType = 0x21  (class | interface | host-to-device)
 *     bRequest      = 0     (OCP Recovery Transfer, fixed)
 *     wValue        = OCP command code (OCP_CMD_*)
 *     wIndex        = interface number
 *     wLength       = register data size
 *     data          = register value bytes
 *
 *   Read register (IN):
 *     bmRequestType = 0xA1  (class | interface | device-to-host)
 *     bRequest      = 0     (OCP Recovery Transfer, fixed)
 *     wValue        = OCP command code (OCP_CMD_*)
 *     wIndex        = interface number
 *     wLength       = register data size
 *
 *   Firmware image upload sequence (section 6.4, INDIRECT FIFO):
 *     1. Write INDIRECT_FIFO_CTRL (cmd 0x2D) with image_size + OCP_FIFO_RESET.
 *     2. Write INDIRECT_FIFO_DATA (cmd 0x2F) repeatedly until image is sent.
 *        Each write carries up to max_transfer_size bytes.
 *     3. Poll INDIRECT_FIFO_STATUS (cmd 0x2E) until status != IN_PROGRESS.
 */

#include "ocp.h"
#include "buffer.h"
#include "libcomm.h"
#include "liberror.h"
#include "libuuu.h"

extern "C" {
#include "libusb.h"
}

#include <string.h>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>

using namespace std;

/* ------------------------------------------------------------------
 * OCPBase::WriteCmd
 *
 * Issues an OUT control transfer to write an OCP command.
 * bRequest = 0, wValue = cmd, wIndex = interface number, wLength = len.
 * ------------------------------------------------------------------ */

int OCPBase::WriteCmd(uint8_t cmd, const void *data, uint16_t len)
{
	libusb_device_handle *h = (libusb_device_handle *)m_devhandle;

	int ret = libusb_control_transfer(
		h,
		OCP_CTRL_OUT,
		0,      /* bRequest = 0 (OCP Recovery Transfer) */
		cmd,    /* wValue  = OCP command code */
		0,      /* wIndex  = interface number */
		(uint8_t *)(data ? const_cast<void *>(data) : nullptr),
		len,
		OCP_CTRL_TIMEOUT
	);
	if (ret < 0)
	{
		string_ex err;
		err.format("OCP WriteCmd cmd=0x%02X: %s (%d)", cmd,
				   libusb_error_name(ret), ret);
		set_last_err_string(err);
		return -1;
	}
	return 0;
}

/* ------------------------------------------------------------------
 * OCPBase::ReadCmd
 *
 * Issues an IN control transfer to read an OCP command.
 * bRequest = 0, wValue = cmd, wIndex = interface number, wLength = len.
 * ------------------------------------------------------------------ */

int OCPBase::ReadCmd(uint8_t cmd, void *data, uint16_t len)
{
	libusb_device_handle *h = (libusb_device_handle *)m_devhandle;

	int ret = libusb_control_transfer(
		h,
		OCP_CTRL_IN,
		0,      /* bRequest = 0 (OCP Recovery Transfer) */
		cmd,    /* wValue  = OCP command code */
		0,      /* wIndex  = interface number */
		(uint8_t *)data,
		len,
		OCP_CTRL_TIMEOUT
	);
	if (ret < (int)len)
	{
		string_ex err;
		err.format("OCP ReadCmd cmd=0x%02X: short/error (%d, expected %u)",
				   cmd, ret, (unsigned)len);
		set_last_err_string(err);
		return -1;
	}
	return 0;
}

/* ------------------------------------------------------------------
 * High-level register helpers
 * ------------------------------------------------------------------ */

int OCPBase::GetProtCap(OCPProtCap *out)
{
	OCPProtCap cap;
	memset(&cap, 0, sizeof(cap));
	if (ReadCmd(OCP_CMD_PROT_CAP, &cap, sizeof(cap)))
		return -1;
	if (out)
		*out = cap;
	return 0;
}

int OCPBase::GetDeviceId(OCPDeviceId *out)
{
	libusb_device_handle *h = (libusb_device_handle *)m_devhandle;
	OCPDeviceId id;
	memset(&id, 0, sizeof(id));

	/*
	 * DEVICE_ID is variable-length (24-255 bytes).  Request the maximum
	 * and let the device return as many bytes as it has.  A short read is
	 * acceptable; negative return means an actual error.
	 */
	int ret = libusb_control_transfer(
		h,
		OCP_CTRL_IN,
		0,                   /* bRequest = 0 (OCP Recovery Transfer) */
		OCP_CMD_DEVICE_ID,   /* wValue   = OCP command code */
		0,                   /* wIndex   = interface number */
		id.raw,
		OCP_DEVICE_ID_MAX_LEN,
		OCP_CTRL_TIMEOUT
	);
	if (ret < OCP_DEVICE_ID_MIN_LEN)
	{
		string_ex err;
		err.format("OCP DEVICE_ID: short/error (%d, need >= %d)",
				   ret, OCP_DEVICE_ID_MIN_LEN);
		set_last_err_string(err);
		return -1;
	}
	id.len = (uint16_t)ret;
	if (out)
		*out = id;
	return 0;
}

int OCPBase::GetDeviceStatus(OCPDeviceStatus *out)
{
	OCPDeviceStatus st;
	memset(&st, 0, sizeof(st));
	if (ReadCmd(OCP_CMD_DEVICE_STATUS, &st, sizeof(st)))
		return -1;
	if (out)
		*out = st;
	return 0;
}

int OCPBase::GetHwStatus(OCPHwStatus *out)
{
	libusb_device_handle *h = (libusb_device_handle *)m_devhandle;
	OCPHwStatus hw;
	memset(&hw, 0, sizeof(hw));

	/*
	 * HW_STATUS is variable-length (7-255 bytes).  Request the maximum
	 * and accept any response >= OCP_HW_STATUS_MIN_LEN.
	 */
	int ret = libusb_control_transfer(
		h,
		OCP_CTRL_IN,
		0,                   /* bRequest = 0 (OCP Recovery Transfer) */
		OCP_CMD_HW_STATUS,   /* wValue   = OCP command code */
		0,                   /* wIndex   = interface number */
		hw.raw,
		OCP_HW_STATUS_MAX_LEN,
		OCP_CTRL_TIMEOUT
	);
	if (ret < OCP_HW_STATUS_MIN_LEN)
	{
		string_ex err;
		err.format("OCP HW_STATUS: short/error (%d, need >= %d)",
				   ret, OCP_HW_STATUS_MIN_LEN);
		set_last_err_string(err);
		return -1;
	}
	hw.len = (uint16_t)ret;
	if (out)
		*out = hw;
	return 0;
}

int OCPBase::GetRecoveryStatus(OCPRecoveryStatus *out)
{
	OCPRecoveryStatus rs;
	memset(&rs, 0, sizeof(rs));
	if (ReadCmd(OCP_CMD_RECOVERY_STATUS, &rs, sizeof(rs)))
		return -1;
	if (out)
		*out = rs;
	return 0;
}

int OCPBase::DeviceReset(uint16_t reset_type)
{
	OCPDeviceReset dr;
	dr.reset_type = reset_type;
	return WriteCmd(OCP_CMD_DEVICE_RESET, &dr, sizeof(dr));
}

/*
 * StartRecovery -- write RECOVERY_CTRL to tell the device a new image
 * is about to arrive, then write INDIRECT_FIFO_CTRL to set image_size
 * and reset the FIFO.
 */
int OCPBase::StartRecovery(uint32_t image_size)
{
	OCPRecoveryCtrl rc;
	rc.image_type = OCP_RECOVERY_IMAGE_UPDATE;
	rc.reserved   = 0;
	if (WriteCmd(OCP_CMD_RECOVERY_CTRL, &rc, sizeof(rc)))
		return -1;

	OCPFifoCtrl fc;
	fc.image_size  = image_size;
	fc.ctrl_flags  = OCP_FIFO_RESET;
	fc.reserved    = 0;
	return WriteCmd(OCP_CMD_INDIRECT_FIFO_CTRL, &fc, sizeof(fc));
}

/*
 * FifoWrite -- stream firmware bytes to INDIRECT_FIFO_DATA via EP0.
 * The OCP spec (section 8.5) uses only EP0 control transfers; there is
 * no bulk endpoint.  Data is sent in up to 4096-byte OUT control transfers.
 */
int OCPBase::FifoWrite(const void *buf, uint32_t len)
{
	const uint8_t *p   = (const uint8_t *)buf;
	uint32_t       rem = len;

	/* Send up to 4096 bytes per OUT control transfer */
	const uint16_t CHUNK = 4096;
	while (rem > 0)
	{
		uint16_t chunk = (rem > CHUNK) ? CHUNK : (uint16_t)rem;
		if (WriteCmd(OCP_CMD_INDIRECT_FIFO_DATA, p, chunk))
			return -1;
		p   += chunk;
		rem -= chunk;
	}
	return 0;
}

/* ------------------------------------------------------------------
 * Helper: extract the raw libusb_device_handle from a CmdCtx.
 * ------------------------------------------------------------------ */

static void *get_devhandle(CmdCtx *ctx)
{
	return ctx->m_dev.handle;
}

/* ------------------------------------------------------------------
 * OCPQueryCmd -- print protocol version and capabilities
 * ------------------------------------------------------------------ */

OCPQueryCmd::OCPQueryCmd(const char *p) : CmdBase(p)
{
	insert_param_info("QUERY", nullptr, Param::Type::e_null);
}

int OCPQueryCmd::run(CmdCtx *ctx)
{
	void *h = get_devhandle(ctx);
	if (!h) { set_last_err_string("OCP: invalid device handle"); return -1; }

	OCPBase ocp(h);

	OCPProtCap cap;
	if (ocp.GetProtCap(&cap))
		return -1;

	OCPDeviceId id;
	if (ocp.GetDeviceId(&id))
		return -1;

	const OCPDeviceIdHdr *idh = id.hdr();
	/* extract description string if present */
	char desc[256] = "";
	if (id.len > (uint16_t)sizeof(OCPDeviceIdHdr) && idh->desc_len > 0)
	{
		uint16_t dlen = idh->desc_len;
		if (dlen > (uint16_t)(id.len - sizeof(OCPDeviceIdHdr)))
			dlen = (uint16_t)(id.len - sizeof(OCPDeviceIdHdr));
		memcpy(desc, id.raw + sizeof(OCPDeviceIdHdr), dlen);
		desc[dlen] = '\0';
	}

	uuu_notify nt;
	nt.type = uuu_notify::NOTIFY_CMD_INFO;
	string_ex info;
	info.format("OCP magic=%.8s  v%d.%d  caps=0x%04X  cms=%u  max_resp=2^%u us  heartbeat=2^%u us\n"
				"  vendor=0x%04X device=0x%04X rev=0x%08X fw=0x%08X desc=%s\n",
				(char *)cap.magic,
				cap.ver_major, cap.ver_minor,
				cap.agent_caps,
				cap.cms_count,
				cap.max_resp_time,
				cap.heartbeat,
				idh->vendor_id, idh->device_id,
				idh->revision, idh->fw_version,
				desc);
	nt.str = (char *)info.c_str();
	call_notify(nt);

	return 0;
}

/* ------------------------------------------------------------------
 * OCPStatusCmd -- print device and hardware status
 * ------------------------------------------------------------------ */

OCPStatusCmd::OCPStatusCmd(const char *p) : CmdBase(p)
{
	insert_param_info("STATUS", nullptr, Param::Type::e_null);
}

int OCPStatusCmd::run(CmdCtx *ctx)
{
	void *h = get_devhandle(ctx);
	if (!h) { set_last_err_string("OCP: invalid device handle"); return -1; }

	OCPBase ocp(h);

	OCPDeviceStatus ds;
	if (ocp.GetDeviceStatus(&ds))
		return -1;

	OCPHwStatus hw;
	if (ocp.GetHwStatus(&hw))
		return -1;

	OCPRecoveryStatus rs;
	if (ocp.GetRecoveryStatus(&rs))
		return -1;

	const char *dev_str = "unknown";
	switch (ds.status) {
	case OCP_DEV_STATUS_HEALTHY:      dev_str = "healthy";            break;
	case OCP_DEV_STATUS_RECOVERY_REQ: dev_str = "recovery_requested"; break;
	case OCP_DEV_STATUS_ERROR:        dev_str = "error";              break;
	}

	const char *rec_str = "unknown";
	switch (rs.status) {
	case OCP_RECOVERY_STATUS_IDLE:        rec_str = "idle";        break;
	case OCP_RECOVERY_STATUS_IN_PROGRESS: rec_str = "in_progress"; break;
	case OCP_RECOVERY_STATUS_SUCCESS:     rec_str = "success";     break;
	case OCP_RECOVERY_STATUS_ERROR:       rec_str = "error";       break;
	}

	const OCPHwStatusHdr *hwh = hw.hdr();
	/* print up to err_valid error registers */
	char errbuf[64] = "none";
	uint8_t nvalid = hwh->err_valid > 3 ? 3 : hwh->err_valid;
	if (nvalid > 0)
	{
		int off = 0;
		for (uint8_t i = 0; i < nvalid; i++)
			off += snprintf(errbuf + off, sizeof(errbuf) - off,
					"0x%04X ", hwh->err[i]);
	}

	uuu_notify nt;
	nt.type = uuu_notify::NOTIFY_CMD_INFO;
	string_ex info;
	info.format("OCP device_status=0x%02X (%s)  hw_state=0x%02X  errors=%s  recovery=%s\n",
				ds.status, dev_str, hwh->hw_state, errbuf, rec_str);
	nt.str = (char *)info.c_str();
	call_notify(nt);

	return 0;
}

/* ------------------------------------------------------------------
 * OCPBootCmd -- upload firmware image via INDIRECT FIFO
 * ------------------------------------------------------------------ */

OCPBootCmd::OCPBootCmd(const char *p) : CmdBase(p)
{
	insert_param_info("BOOT", nullptr,       Param::Type::e_null);
	insert_param_info("-f",   &m_filename,   Param::Type::e_string_filename);
}

int OCPBootCmd::run(CmdCtx *ctx)
{
	if (m_filename.empty())
	{
		set_last_err_string("OCP WRITE: filename required (-f <file>)");
		return -1;
	}

	shared_ptr<FileBuffer> fb = get_file_buffer(m_filename);
	if (!fb)
		return -1;

	shared_ptr<DataBuffer> db = fb->request_data(0, SIZE_MAX);
	if (!db)
		return -1;

	void *h = get_devhandle(ctx);
	if (!h) { set_last_err_string("OCP: invalid device handle"); return -1; }

	OCPBase ocp(h);

	size_t total = db->size();

	/* Write RECOVERY_CTRL + INDIRECT_FIFO_CTRL to initiate the transfer */
	if (ocp.StartRecovery(static_cast<uint32_t>(total)))
		return -1;

	/* Wall-clock timeout for FIFO space polling: 5 seconds per chunk */
	const auto FIFO_POLL_TIMEOUT = std::chrono::seconds(5);

	size_t offset = 0;
	while (offset < total)
	{
		/* Poll FIFO status to determine how many bytes are safe to send */
		OCPFifoStatus fs;
		auto deadline = std::chrono::steady_clock::now() + FIFO_POLL_TIMEOUT;
		for (;;)
		{
			if (ocp.GetFifoStatus(&fs))
				return -1;
			if (fs.available_dwords() > 0)
				break;
			if (std::chrono::steady_clock::now() >= deadline)
			{
				set_last_err_string("OCP BOOT: FIFO full, timeout waiting for space");
				return -1;
			}
		}

		/* Limit chunk to available FIFO space and device max-transfer size */
		size_t avail = (size_t)fs.available_dwords() * 4;
		size_t max_t = (size_t)fs.max_transfer_dwords * 4;
		if (max_t == 0)
			max_t = 4096; /* fallback if device reports 0 */

		size_t chunk = total - offset;
		if (chunk > avail)   chunk = avail;
		if (chunk > max_t)   chunk = max_t;

		uuu_notify nt;
		nt.type  = uuu_notify::NOTIFY_TRANS_POS;
		nt.total = total;
		nt.index = offset;
		call_notify(nt);

		if (ocp.FifoWrite(db->data() + offset, static_cast<uint32_t>(chunk)))
			return -1;

		offset += chunk;
	}

	/* Final progress notification */
	uuu_notify nt;
	nt.type  = uuu_notify::NOTIFY_TRANS_POS;
	nt.total = total;
	nt.index = total;
	call_notify(nt);

	return 0;
}

/* ------------------------------------------------------------------
 * OCPRecoveryCmd -- write RECOVERY_CTRL without a data image
 * (used to trigger recovery when the device already has a staged image)
 * ------------------------------------------------------------------ */

OCPRecoveryCmd::OCPRecoveryCmd(const char *p) : CmdBase(p)
{
	insert_param_info("RECOVERY", nullptr, Param::Type::e_null);
}

int OCPRecoveryCmd::run(CmdCtx *ctx)
{
	void *h = get_devhandle(ctx);
	if (!h) { set_last_err_string("OCP: invalid device handle"); return -1; }

	OCPBase ocp(h);
	OCPRecoveryCtrl rc;
	rc.image_type = OCP_RECOVERY_IMAGE_UPDATE;
	rc.reserved   = 0;
	return ocp.WriteCmd(OCP_CMD_RECOVERY_CTRL, &rc, sizeof(rc));
}

/* ------------------------------------------------------------------
 * OCPResetCmd -- write DEVICE_RESET
 * ------------------------------------------------------------------ */

OCPResetCmd::OCPResetCmd(const char *p) : CmdBase(p)
{
	insert_param_info("RESET", nullptr, Param::Type::e_null);
	insert_param_info("-cold", &m_cold, Param::Type::e_bool);
	insert_param_info("-boot", &m_boot, Param::Type::e_bool);
}

int OCPResetCmd::run(CmdCtx *ctx)
{
	void *h = get_devhandle(ctx);
	if (!h) { set_last_err_string("OCP: invalid device handle"); return -1; }

	uint16_t reset_type = OCP_RESET_WARM;
	if (m_boot)
		reset_type = OCP_RESET_BOOT;
	else if (m_cold)
		reset_type = OCP_RESET_COLD;

	OCPBase ocp(h);
	return ocp.DeviceReset(reset_type);
}

int OCPBase::GetFifoStatus(OCPFifoStatus *out)
{
	OCPFifoStatus fs;
	memset(&fs, 0, sizeof(fs));
	if (ReadCmd(OCP_CMD_INDIRECT_FIFO_STATUS, &fs, OCP_FIFO_STATUS_LEN))
		return -1;
	if (out)
		*out = fs;
	return 0;
}

/* ------------------------------------------------------------------
 * OCPFifoStatusCmd -- print INDIRECT_FIFO_STATUS
 * ------------------------------------------------------------------ */

OCPFifoStatusCmd::OCPFifoStatusCmd(const char *p) : CmdBase(p)
{
	insert_param_info("FIFO", nullptr, Param::Type::e_null);
}

int OCPFifoStatusCmd::run(CmdCtx *ctx)
{
	void *h = get_devhandle(ctx);
	if (!h) { set_last_err_string("OCP: invalid device handle"); return -1; }

	OCPBase ocp(h);
	OCPFifoStatus fs;
	if (ocp.GetFifoStatus(&fs))
		return -1;

	uuu_notify nt;
	nt.type = uuu_notify::NOTIFY_CMD_INFO;
	string_ex info;
	info.format(
		"OCP FIFO_STATUS: full=%d  write_idx=%u  read_idx=%u"
		"  size=%u dwords  max_transfer=%u dwords  available=%u dwords\n",
		(int)fs.full(),
		fs.write_index, fs.read_index,
		fs.size_dwords, fs.max_transfer_dwords,
		fs.available_dwords());
	nt.str = (char *)info.c_str();
	call_notify(nt);

	return 0;
}

/* ------------------------------------------------------------------
 * OCPLogCmd -- read VENDOR register (0x2C) as a log buffer
 * ------------------------------------------------------------------ */

OCPLogCmd::OCPLogCmd(const char *p) : CmdBase(p)
{
	insert_param_info("LOG", nullptr, Param::Type::e_null);
}

int OCPLogCmd::run(CmdCtx *ctx)
{
	void *h = get_devhandle(ctx);
	if (!h) { set_last_err_string("OCP: invalid device handle"); return -1; }

	/* Read up to 4096 bytes from the VENDOR register as a log string */
	const uint16_t LOG_LEN = 4096;
	vector<uint8_t> buf(LOG_LEN + 1, 0);

	OCPBase ocp(h);
	/* A short read is acceptable here -- use ReadCmd directly */
	libusb_device_handle *dev = (libusb_device_handle *)h;
	int ret = libusb_control_transfer(
		dev,
		OCP_CTRL_IN,
		0,               /* bRequest = 0 (OCP Recovery Transfer) */
		OCP_CMD_VENDOR,  /* wValue   = OCP command code */
		0,               /* wIndex   = interface number */
		buf.data(),
		LOG_LEN,
		OCP_CTRL_TIMEOUT
	);
	if (ret < 0)
	{
		string_ex err;
		err.format("OCP LOG: %s (%d)", libusb_error_name(ret), ret);
		set_last_err_string(err);
		return -1;
	}
	buf[ret] = '\0';

	uuu_notify nt;
	nt.type = uuu_notify::NOTIFY_CMD_INFO;
	nt.str  = (char *)buf.data();
	call_notify(nt);

	return 0;
}
