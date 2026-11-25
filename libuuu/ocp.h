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
 * OCP (Open Compute Project) USB Device Recovery Interface support.
 * Based on OCP Recovery Specification v1.1, December 2025.
 * https://www.opencompute.org/documents/ocp-recovery-document-1p1-final-pdf
 *
 * Protocol overview (section 8.5):
 *   - Each OCP register access maps to one USB EP0 control transfer.
 *   - bmRequestType type field = 01b (USB Class), NOT vendor.
 *   - bRequest = 0 (OCP Recovery Transfer, fixed).
 *   - wValue   = OCP register address (see OCP_CMD_* defines below).
 *   - Writes: OUT control transfer, wLength = register data size.
 *   - Reads:  IN control transfer,  wLength = register data size.
 *   - Firmware image transfer uses INDIRECT_FIFO registers (section 6.4).
 */

#pragma once

#include "cmd.h"
#include <cstdint>
#include <string>
#include <vector>

/*
 * OCP Recovery register addresses (bRequest values).
 * Source: OCP Recovery Specification v1.1, Table 2 (Command Map).
 */
#define OCP_CMD_PROT_CAP             0x22  /* Protocol capabilities (RO) */
#define OCP_CMD_DEVICE_ID            0x23  /* Device identification (RO) */
#define OCP_CMD_DEVICE_STATUS        0x24  /* Device status (RO) */
#define OCP_CMD_DEVICE_RESET         0x25  /* Device reset control (WO) */
#define OCP_CMD_RECOVERY_CTRL        0x26  /* Recovery control (WO) */
#define OCP_CMD_RECOVERY_STATUS      0x27  /* Recovery status (RO) */
#define OCP_CMD_HW_STATUS            0x28  /* Hardware status (RO) */
#define OCP_CMD_INDIRECT_CTRL        0x29  /* Indirect memory access control (RW) */
#define OCP_CMD_INDIRECT_STATUS      0x2A  /* Indirect memory access status (RO) */
#define OCP_CMD_INDIRECT_DATA        0x2B  /* Indirect memory data (RW) */
#define OCP_CMD_VENDOR               0x2C  /* Vendor-specific (RW) */
#define OCP_CMD_INDIRECT_FIFO_CTRL   0x2D  /* FIFO transfer control (RW) */
#define OCP_CMD_INDIRECT_FIFO_STATUS 0x2E  /* FIFO transfer status (RO) */
#define OCP_CMD_INDIRECT_FIFO_DATA   0x2F  /* FIFO data port - stream writes here (WO) */

/*
 * bmRequestType for OCP EP0 control transfers (section 8.5.1):
 *   Bit 7   : direction  - 0 = OUT (host to device), 1 = IN (device to host)
 *   Bit 6:5 : type       - 01b = USB Class
 *   Bit 4:0 : recipient  - 00001b = Interface
 *
 *   OUT: 0b_0_01_00001 = 0x21
 *   IN:  0b_1_01_00001 = 0xA1
 */
#define OCP_CTRL_OUT \
	(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE)
#define OCP_CTRL_IN  \
	(LIBUSB_ENDPOINT_IN  | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE)

/* USB control transfer timeout (ms) */
#define OCP_CTRL_TIMEOUT        5000

/*
 * PROT_CAP agent_caps bit definitions (bytes 10-11, spec section 9.3).
 */
#define OCP_CAP_DEVICE_ID       (1u <<  0)  /* BIT 0: DEVICE_ID supported */
#define OCP_CAP_FORCED_RECOVERY (1u <<  1)  /* BIT 1: Forced recovery from RESET */
#define OCP_CAP_MGMT_RESET      (1u <<  2)  /* BIT 2: Management reset */
#define OCP_CAP_DEVICE_RESET    (1u <<  3)  /* BIT 3: Device reset */
#define OCP_CAP_DEVICE_STATUS   (1u <<  4)  /* BIT 4: DEVICE_STATUS supported */
#define OCP_CAP_INDIRECT_MEM    (1u <<  5)  /* BIT 5: Indirect memory access (INDIRECT_CTRL) */
#define OCP_CAP_LOCAL_CIMAGE    (1u <<  6)  /* BIT 6: Local C-image support */
#define OCP_CAP_PUSH_CIMAGE     (1u <<  7)  /* BIT 7: Push C-image support */
#define OCP_CAP_IFACE_ISOLATION (1u <<  8)  /* BIT 8: Interface isolation */
#define OCP_CAP_HW_STATUS       (1u <<  9)  /* BIT 9: Hardware status */
#define OCP_CAP_VENDOR_CMD      (1u << 10)  /* BIT 10: Vendor command */
#define OCP_CAP_FLASHLESS_BOOT  (1u << 11)  /* BIT 11: Flashless boot */
#define OCP_CAP_FIFO_CMS        (1u << 12)  /* BIT 12: FIFO CMS (INDIRECT_FIFO_CTRL) */

/*
 * DEVICE_STATUS flags (section 5.4).
 */
#define OCP_DEV_STATUS_HEALTHY      0x00
#define OCP_DEV_STATUS_RECOVERY_REQ 0x01  /* Device requesting recovery */
#define OCP_DEV_STATUS_ERROR        0x02  /* Device in error state */

/*
 * DEVICE_RESET reset type codes (section 5.5, wValue field).
 */
#define OCP_RESET_WARM  0x0000  /* Warm reset */
#define OCP_RESET_COLD  0x0001  /* Cold reset */
#define OCP_RESET_BOOT  0x0002  /* Reset and boot to recovery image */

/*
 * RECOVERY_CTRL image type codes (section 5.7).
 */
#define OCP_RECOVERY_IMAGE_UPDATE   0x0001  /* Push a new firmware image */

/*
 * RECOVERY_STATUS status codes (section 5.8).
 */
#define OCP_RECOVERY_STATUS_IDLE        0x00
#define OCP_RECOVERY_STATUS_IN_PROGRESS 0x01
#define OCP_RECOVERY_STATUS_SUCCESS     0x02
#define OCP_RECOVERY_STATUS_ERROR       0x03

/*
 * INDIRECT_FIFO_CTRL layout (section 6.4).
 * Written before streaming INDIRECT_FIFO_DATA to configure the transfer.
 */
#define OCP_FIFO_RESET  0x0001  /* Reset the FIFO before a new transfer */

/* Maximum bytes per INDIRECT_FIFO_DATA write (256 KB, adjust per PROT_CAP) */
#define OCP_MAX_TRANSFER_SIZE   0x40000

#pragma pack(1)

/*
 * OCP_CMD_PROT_CAP response layout (15 bytes, RO).
 * Source: OCP Recovery Spec v1.1, section 9.3 (cmd=34, size=15).
 *
 *  Byte  0-7  : Magic string "OCP RECV" (ASCII, not null-terminated)
 *  Byte  8    : Major version (0x01)
 *  Byte  9    : Minor version (0x01)
 *  Byte 10-11 : Agent capability flags (OCP_PROT_CAP_*)
 *  Byte 12    : CMS count (number of Component Memory Space regions)
 *  Byte 13    : Max response time (2^n microseconds)
 *  Byte 14    : Heartbeat period (2^n microseconds, 0 = not supported)
 */
struct OCPProtCap {
	uint8_t  magic[8];      /* "OCP RECV" */
	uint8_t  ver_major;     /* Major version */
	uint8_t  ver_minor;     /* Minor version */
	uint16_t agent_caps;    /* Capability flags (OCP_PROT_CAP_*) */
	uint8_t  cms_count;     /* Number of CMS regions */
	uint8_t  max_resp_time; /* Max response time: 2^n us */
	uint8_t  heartbeat;     /* Heartbeat period: 2^n us, 0=not supported */
};

/*
 * OCP_CMD_DEVICE_ID register layout (variable, 24-255 bytes, RO).
 * Source: OCP Recovery Spec v1.1, section 9.4.
 *
 *  Byte  0-1  : Vendor ID
 *  Byte  2-3  : Device ID
 *  Byte  4-7  : Subsystem Vendor ID
 *  Byte  8-11 : Subsystem Device ID
 *  Byte 12-15 : Revision
 *  Byte 16-19 : FW Version
 *  Byte 20    : Description length (N, may be 0)
 *  Byte 21..  : Description string (N bytes, ASCII, not null-terminated)
 *  Padding to reach a multiple of 4 bytes, minimum total = 24 bytes.
 */
#define OCP_DEVICE_ID_MIN_LEN  24
#define OCP_DEVICE_ID_MAX_LEN  255

struct OCPDeviceIdHdr {
	uint16_t vendor_id;          /* Vendor ID */
	uint16_t device_id;          /* Device ID */
	uint32_t subsystem_vendor;   /* Subsystem vendor ID */
	uint32_t subsystem_device;   /* Subsystem device ID */
	uint32_t revision;           /* Hardware revision */
	uint32_t fw_version;         /* Firmware version */
	uint8_t  desc_len;           /* Length of following description string */
	/* variable-length description follows */
};

/* Full max-size buffer for a DEVICE_ID read */
struct OCPDeviceId {
	uint8_t  raw[OCP_DEVICE_ID_MAX_LEN]; /* raw response bytes */
	uint16_t len;                         /* actual bytes returned by device */

	/* Typed accessors into raw[] */
	const OCPDeviceIdHdr *hdr() const
	{
		return reinterpret_cast<const OCPDeviceIdHdr *>(raw);
	}
};

/*
 * OCP_CMD_DEVICE_STATUS register layout (4 bytes, RO).
 * Section 5.4.
 */
struct OCPDeviceStatus {
	uint8_t  status;            /* OCP_DEV_STATUS_* */
	uint8_t  reserved[3];
};

/*
 * OCP_CMD_DEVICE_RESET register layout (2 bytes, WO).
 * Section 5.5.  Written with wLength=2.
 */
struct OCPDeviceReset {
	uint16_t reset_type;        /* OCP_RESET_* */
};

/*
 * OCP_CMD_RECOVERY_CTRL register layout (4 bytes, WO).
 * Section 5.7.  Written to initiate recovery.
 */
struct OCPRecoveryCtrl {
	uint16_t image_type;        /* OCP_RECOVERY_IMAGE_UPDATE */
	uint16_t reserved;
};

/*
 * OCP_CMD_RECOVERY_STATUS register layout (4 bytes, RO).
 * Section 5.8.
 */
struct OCPRecoveryStatus {
	uint8_t  status;            /* OCP_RECOVERY_STATUS_* */
	uint8_t  reserved[3];
};

/*
 * OCP_CMD_HW_STATUS register layout (variable, 7-255 bytes, RO).
 * Source: OCP Recovery Spec v1.1, section 9.9.
 *
 *  Byte 0   : hw_state  - device hardware state code
 *  Byte 1   : err_valid - number of valid error registers (0-3)
 *  Byte 2-3 : err[0]    - error register 0 (if err_valid >= 1)
 *  Byte 4-5 : err[1]    - error register 1 (if err_valid >= 2)
 *  Byte 6-7 : err[2]    - error register 2 (if err_valid >= 3)
 *  ...      : vendor-specific bytes may follow
 *  Minimum total = 7 bytes (hw_state + err_valid + err[0..2] partial).
 */
#define OCP_HW_STATUS_MIN_LEN  7
#define OCP_HW_STATUS_MAX_LEN  255

struct OCPHwStatusHdr {
	uint8_t  hw_state;   /* Hardware state code */
	uint8_t  err_valid;  /* Number of valid error registers (0-3) */
	uint16_t err[3];     /* Error registers (only err_valid entries are valid) */
	/* vendor-specific bytes may follow */
};

/* Full max-size buffer for a HW_STATUS read */
struct OCPHwStatus {
	uint8_t  raw[OCP_HW_STATUS_MAX_LEN]; /* raw response bytes */
	uint16_t len;                          /* actual bytes returned by device */

	const OCPHwStatusHdr *hdr() const
	{
		return reinterpret_cast<const OCPHwStatusHdr *>(raw);
	}
};

/*
 * OCP_CMD_INDIRECT_FIFO_CTRL register layout (8 bytes, RW).
 * Section 6.4.  Written before streaming image data.
 */
struct OCPFifoCtrl {
	uint32_t image_size;        /* Total image size in bytes */
	uint16_t ctrl_flags;        /* OCP_FIFO_RESET etc. */
	uint16_t reserved;
};

#pragma pack()

/*
 * OCP_CMD_INDIRECT_FIFO_STATUS register layout (20 bytes, RO).
 * Source: OCP Recovery Spec v1.1, section 9.14.
 *
 *  Byte  0    : flags       - bit 1 = FIFO full
 *  Byte  1-3  : reserved
 *  Byte  4-7  : write_index - next write position (in dwords)
 *  Byte  8-11 : read_index  - next read position  (in dwords)
 *  Byte 12-15 : size_dwords - total FIFO capacity  (in dwords)
 *  Byte 16-19 : max_transfer_dwords - max dwords per INDIRECT_FIFO_DATA write
 *
 * All fields are 4-byte aligned; no packing needed.
 */
#define OCP_FIFO_STATUS_LEN  20
#define OCP_FIFO_FLAG_FULL   (1u << 1)  /* FIFO full flag in flags byte */

struct OCPFifoStatus {
	uint8_t  flags;              /* bit 1: FIFO full */
	uint8_t  reserved[3];
	uint32_t write_index;        /* next write position (dwords) */
	uint32_t read_index;         /* next read position  (dwords) */
	uint32_t size_dwords;        /* total FIFO depth    (dwords) */
	uint32_t max_transfer_dwords;/* max dwords per INDIRECT_FIFO_DATA write */

	bool full() const { return !!(flags & OCP_FIFO_FLAG_FULL); }

	/* Returns the number of dwords available for writing. */
	uint32_t available_dwords() const
	{
		if (full() || size_dwords == 0)
			return 0;
		uint32_t used = (write_index >= read_index)
			? write_index - read_index
			: size_dwords - (read_index - write_index);
		return (size_dwords > used) ? size_dwords - used : 0;
	}
};

/*
 * OCPBase -- low-level OCP register access helper.
 *
 * All communication uses USB EP0 control transfers only (OCP spec section 8.5).
 * WriteCmd() issues an OUT control transfer to write a command.
 * ReadCmd()  issues an IN  control transfer to read  a command.
 * FifoWrite() streams firmware bytes via repeated INDIRECT_FIFO_DATA OUT transfers.
 *
 * The caller supplies:
 *   devhandle -- raw libusb_device_handle* (cast to void*)
 */
class OCPBase
{
public:
	OCPBase(void *devhandle) : m_devhandle{devhandle} {}

	/* Write a command via OUT control transfer */
	int WriteCmd(uint8_t cmd, const void *data, uint16_t len);

	/* Read a command via IN control transfer */
	int ReadCmd(uint8_t cmd, void *data, uint16_t len);

	/* High-level helpers */
	int GetProtCap(OCPProtCap *out);
	int GetDeviceId(OCPDeviceId *out);
	int GetDeviceStatus(OCPDeviceStatus *out);
	int GetHwStatus(OCPHwStatus *out);
	int GetRecoveryStatus(OCPRecoveryStatus *out);
	int GetFifoStatus(OCPFifoStatus *out);
	int DeviceReset(uint16_t reset_type = OCP_RESET_WARM);
	int StartRecovery(uint32_t image_size);
	int FifoWrite(const void *buf, uint32_t len);

private:
	void *m_devhandle = nullptr;
};

/* ------------------------------------------------------------------ */
/* Command objects                                                      */
/* ------------------------------------------------------------------ */

/*
 * OCP:QUERY -- print device protocol version and capabilities
 * Usage: OCP: QUERY
 */
class OCPQueryCmd : public CmdBase
{
public:
	OCPQueryCmd(const char *p);
	int run(CmdCtx *ctx) override;
};

/*
 * OCP:STATUS -- print hardware and device status
 * Usage: OCP: STATUS
 */
class OCPStatusCmd : public CmdBase
{
public:
	OCPStatusCmd(const char *p);
	int run(CmdCtx *ctx) override;
};

/*
 * OCP:BOOT -- flash a firmware image to the device via INDIRECT FIFO
 * Usage: OCP: BOOT -f <filename>
 */
class OCPBootCmd : public CmdBase
{
public:
	OCPBootCmd(const char *p);
	int run(CmdCtx *ctx) override;

private:
	std::string m_filename;
};

/*
 * OCP:RECOVERY -- trigger device recovery mode
 * Usage: OCP: RECOVERY
 */
class OCPRecoveryCmd : public CmdBase
{
public:
	OCPRecoveryCmd(const char *p);
	int run(CmdCtx *ctx) override;
};

/*
 * OCP:RESET -- reset the device
 * Usage: OCP: RESET [-cold] [-boot]
 */
class OCPResetCmd : public CmdBase
{
public:
	OCPResetCmd(const char *p);
	int run(CmdCtx *ctx) override;

private:
	bool m_cold = false;
	bool m_boot = false;
};

/*
 * OCP:FIFO -- read and print INDIRECT_FIFO_STATUS (cmd 0x2E)
 * Usage: OCP: FIFO
 */
class OCPFifoStatusCmd : public CmdBase
{
public:
	OCPFifoStatusCmd(const char *p);
	int run(CmdCtx *ctx) override;
};

/*
 * OCP:LOG -- retrieve and print the device log (vendor-specific, cmd 0x2C)
 * Usage: OCP: LOG
 */
class OCPLogCmd : public CmdBase
{
public:
	OCPLogCmd(const char *p);
	int run(CmdCtx *ctx) override;
};
