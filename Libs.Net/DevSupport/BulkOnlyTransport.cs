/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

using DevSupport.Api;

namespace DevSupport.DeviceManager.BulkOnlyTransportProtocol
{
    //------------------------------------------------------------------------------
    // Command Block Wrapper (CBW)
    //------------------------------------------------------------------------------
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public class CBW
    {
        public UInt32 Signature;        // Signature: 0x43425355, or "USBC" (little endian) for the BOT CBW
        public UInt32 Tag;              // Tag: to be returned in the csw
        public UInt32 XferLength;       // XferLength: number of bytes to transfer
        public CbwFlags Flags;          // Flags:
                                        //   Bit 7: direction - device shall ignore this bit if the
                                        //     XferLength field is zero, otherwise:
                                        //     0 = data-out from the host to the device,
                                        //     1 = data-in from the device to the host.
                                        //   Bits 6..0: reserved - shall be zero.
        public Byte Lun;          // Bits 0-3 : Lun; Bits 4-7 : Reserved and shall be zero.
        public Byte CdbLength;    // Bits 0-4 : CdbLength - Valid length of the CDB. Must be 1 thru 16(decimal); Bits 5-7 : Reserved and shall be zero.
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
        public Byte[] Cdb;        // Cdb: the command descriptor block

        // Signature value for _ST_HID_CBW
        const UInt32 CBW_SIGNATURE = 0x43425355; // "USBC" (little endian)

        // Flags values for _ST_HID_CBW
        [Flags]
        public enum CbwFlags : byte
        {
            HostToDevice = 0x00, // Data-Out : from Host to Device
            DeviceToHost = 0x80  // Data-In : from Device to Host
        }

        public CBW(ScsiApi api)
        {
            Signature = CBW_SIGNATURE; // "USBC" (little endian)
            Tag = api.CbwTag;
            XferLength = Convert.ToUInt32(api.TransferSize);
            Flags = api.Direction == DevSupport.Api.Api.CommandDirection.ReadWithData ? CbwFlags.DeviceToHost : CbwFlags.HostToDevice;
            Lun = 0;
            CdbLength = Convert.ToByte(api.Cdb.Length);
            Cdb = new Byte[16];
            api.Cdb.CopyTo(Cdb, 0);
        }

        public Byte[] ToByteArray()
        {
            List<Byte> bytes = new List<Byte>(Marshal.SizeOf(this));
            bytes.AddRange(BitConverter.GetBytes(Signature));
            bytes.AddRange(BitConverter.GetBytes(Tag));
            bytes.AddRange(BitConverter.GetBytes(XferLength));
            bytes.Add((Byte)Flags);
            bytes.Add(Lun);
            bytes.Add(CdbLength);
            bytes.AddRange(Cdb);
            return bytes.ToArray();
        }
    };

    //------------------------------------------------------------------------------
    // Command Status Wrapper (CSW)
    //------------------------------------------------------------------------------
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct CSW
    {
        public readonly UInt32 Signature;       // Signature: 0x53425355 "USBS" (little endian)
        public readonly UInt32 Tag;             // Tag: matches the value from the CBW
        public readonly UInt32 Residue;         // Residue: number of bytes not transferred
        public readonly CommandStatus Status;   // Status:
                                                //   00h command passed ("good status")
                                                //   01h command failed
                                                //   02h phase error
                                                //   03h to FFh reserved

        // Signature value for CSW
        const UInt32 CSW_SIGNATURE = 0x53544C42; // "USBS" (little endian)
        // Status values for CSW
        public enum CommandStatus : byte { Passed = 0x00, Failed = 0x01, PhaseError = 0x02 }

        public CSW(Byte[] returnedBytes)
        {
            Array.Reverse(returnedBytes, 0, 4);
            Signature = BitConverter.ToUInt32(returnedBytes, 0);

            Array.Reverse(returnedBytes, 4, 4);
            Tag = BitConverter.ToUInt32(returnedBytes, 4);

            Array.Reverse(returnedBytes, 8, 4);
            Residue = BitConverter.ToUInt32(returnedBytes, 8);

            Status = (CommandStatus)returnedBytes[12];
        }

        public Byte[] ToByteArray()
        {
            List<Byte> bytes = new List<Byte>(Marshal.SizeOf(this));
            bytes.AddRange(BitConverter.GetBytes(Signature));
            bytes.AddRange(BitConverter.GetBytes(Tag));
            bytes.AddRange(BitConverter.GetBytes(Residue));
            bytes.Add((Byte)Status);
            return bytes.ToArray();
        }
    }

    public class BulkOnlyTransport
    {
        public Int32 SendCommand(ScsiApi api)
        {
            return 0;
        }

        public Int32 ResetRecovery()
        {
            return 0;
        }
    }

}
