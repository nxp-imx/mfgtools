/*
* Copyright 2023 NXP.
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

import {useEffect, useState} from 'react';
import {str_to_arr, ab_to_str} from '../helper/functions.js'
import {CHUNK_SZ, BLK_SZ, CHUNK_TYPE_RAW, CHUNK_TYPE_DONT_CARE, build_sparse_header, build_chunk_header} from '../helper/sparse.js'

const USBrequestParams = { filters: [{ vendorId: 8137, productId: 0x0152 }] };

const useUSBFlash = (flashFile) => {
    const [USBDevice, setUSBDevice] = useState();
    const [flashProgress, setFlashProgress] = useState();
    const [flashTotal, setFlashTotal] = useState();

    useEffect (()=> {
        const downloadFlash = async() => {
            if (!USBDevice) return;
            try {
                await USBDevice.open();
                console.log(`Opened USB Device: ${USBDevice.productName}`);
            }
            catch (err) {
                console.log(err);
            }
            doFlash();
        }
        downloadFlash();
    }, [USBDevice])

    const requestUSBDevice = async() => {
        try {
            const device = await navigator.usb.requestDevice(USBrequestParams);
            setUSBDevice(device);
        }
        catch (err) {
            console.error(`There is no device. ${err}`);
        }
    }

    async function preBoot () {
        await USBDevice.claimInterface(0);

        await send_data(str_to_arr("UCmd:setenv fastboot_dev mmc"), "OKAY");
        await send_data(str_to_arr("UCmd:setenv mmcdev ${emmc_dev}"), "OKAY");
        await send_data(str_to_arr("UCmd:mmc dev ${emmc_dev}"), "OKAY");

        console.log("preboot complete")
    }

    const doFlash = async () => {
        // downloads and splits image into chunks to be processed
        const PACKET_SZ = 0x10000;
        const DATA_SZ = CHUNK_SZ * BLK_SZ; // in bytes
        if (flashFile===null) {return;}

        await preBoot();
        setFlashTotal(flashFile.size);
        setFlashProgress(0);

        let offset = 0;
        let chunk_end = 0;
        let i = 0;

        let chunk_len = 0;
        let packet_len = 0;

        let reader = new FileReader();
        reader.onerror = (evt) => {
            console.log(evt.target.error);
        }

        reader.onload = async (evt) => {
            let raw_data = evt.target.result;
            await USBDevice.transferOut(1, raw_data);
            setFlashProgress(offset);

            if (offset < chunk_end) {
                readNextPacket();
            }
            else { // reached end of a chunk

                // pad with zeros
                if (packet_len !== PACKET_SZ) {
                    let fill_len = (Math.ceil(packet_len/BLK_SZ)*BLK_SZ - packet_len)
                    let fill = new Uint8Array(fill_len);
                    await USBDevice.transferOut(1, fill);
                }

                let result = await USBDevice.transferIn(1, 1048);
                if ("OKAY" !== await ab_to_str(result.data.buffer)) {
                    throw new Error ("failed to send data:", ab_to_str(result.data.buffer))
                }

                await flash_all();
                readNextChunk();
            }
        };

        function readNextPacket() {
            packet_len = Math.min(PACKET_SZ, flashFile.size-offset);
            let slice = flashFile.slice(offset, offset + packet_len);
            reader.readAsArrayBuffer(slice);
            offset += packet_len;
        }

        async function readNextChunk() {
            if (offset !== flashFile.size) {
                chunk_len = Math.min(DATA_SZ, flashFile.size-offset);
                chunk_end = Math.min(offset + DATA_SZ, flashFile.size);

                // rounds chunk len up to nearest block:
                let write_chunk_len = Math.ceil(chunk_len/BLK_SZ)*BLK_SZ;
                let hex_len = (write_chunk_len + 52).toString(16);
                await send_data(str_to_arr(`download:${hex_len}`), `DATA${hex_len}`);
                await send_headers(write_chunk_len, i);

                readNextPacket();
                i++;
            }
        }

        readNextChunk();
    }

    async function send_headers(raw_data_bytelength, i) {
        let sparse = build_sparse_header(raw_data_bytelength, i);
        let dont_care = build_chunk_header(CHUNK_TYPE_DONT_CARE, raw_data_bytelength, i);
        let raw = build_chunk_header(CHUNK_TYPE_RAW, raw_data_bytelength, i);

        let headers = new Uint8Array(52);
        headers.set(sparse, 0);
        headers.set(dont_care, 28);
        headers.set(raw, 40);

        await USBDevice.transferOut(1, headers);
    }

    /*
    * data: string or arraybuffer
    * success_str: checks response of usb
    * Throws error if USB input does not match success_str
    */
    async function send_data(data, success_str) {
        await USBDevice.transferOut(1, data);
        let result = await USBDevice.transferIn(1, 1048);

        if (success_str !== ab_to_str(result.data.buffer)) {
            throw new Error ("failed to send data:", ab_to_str(result.data.buffer))
        }
    }

    async function flash_all () {
        await send_data(str_to_arr("flash:all"), "OKAY");
    }

    return [{
        requestUSBDevice,
        USBDevice,
        flashProgress,
        flashTotal
    }]
}

export default useUSBFlash;
