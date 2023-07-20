
import {useEffect, useState} from 'react';
import {str_to_arr, ab_to_str} from '../helper/functions.js'
import {CHUNK_SZ, BLK_SZ, CHUNK_TYPE_RAW, CHUNK_TYPE_DONT_CARE, build_sparse_header, build_chunk_header} from '../helper/sparse.js'

const USBrequestParams = { filters: [{ vendorId: 8137, productId: 0x0152 }] };

const PACKET_SZ = 0x10000;
const DATA_SZ = CHUNK_SZ * BLK_SZ; // in bytes

const useDecompress = (flashFile) => {
    const [USBDevice, setUSBDevice] = useState();
    const [flashProgress, setFlashProgress] = useState();
    const [flashTotal, setFlashTotal] = useState();

    useEffect (()=> {
        if (USBDevice) {
            USBDevice.open().then(()=>{
                console.log(`Opened USB Device: ${USBDevice.productName}`);
            }, (err)=> {
                console.log(err)
            })
            .then(()=> {
                if (flashFile)
                    decompressFileToTransform();
            })
        }

    }, [USBDevice])

    const requestUSBDevice = () => { // first thing called with button
        navigator.usb
        .requestDevice(USBrequestParams)
        .then((device) => {
            setUSBDevice(device);
        })
        .catch((e) => {
            console.error(`There is no device. ${e}`);
        });
    }

    async function preBoot () {
        await USBDevice.claimInterface(0);

        await send_data(await str_to_arr("UCmd:setenv fastboot_dev mmc"), "OKAY");
        await send_data(await str_to_arr("UCmd:setenv mmcdev ${emmc_dev}"), "OKAY");
        await send_data(await str_to_arr("UCmd:mmc dev ${emmc_dev}"), "OKAY");

        console.log("preboot complete")
    }

    const processChunk = async(data, curr_len, i) => {
        console.log(data, curr_len, i);

        // ignore, fill with zeroes
        if (curr_len != data.length) {
            let fill_len = (Math.ceil(curr_len/BLK_SZ)*BLK_SZ - curr_len);
            // let fill = new Uint8Array(fill_len);
            data.set(curr_len, new Uint8Array(fill_len));
            data = data.slice(0, curr_len+fill_len);
        }

        let hex_len = (data.length + 52).toString(16); // 52 comes from headers
        await send_data(await str_to_arr(`download:${hex_len}`), `DATA${hex_len}`);
        await send_headers(data.length, i);

        let offset=0;
        while (offset < data.length) {
            let packet_len = Math.min(PACKET_SZ, DATA_SZ - offset);
            await USBDevice.transferOut(1, data.slice(offset, offset + packet_len));
            console.log("transferout", data.slice(offset, offset + packet_len));
            offset += packet_len;
        }

        let result = await USBDevice.transferIn(1, 1048);
        if ("OKAY" !== await ab_to_str(result.data.buffer)) {
            throw new Error ("failed to send data:", await ab_to_str(result.data.buffer))
        }

        await flash_all();
        console.log("FLASH SUCCESS")
    }

    async function send_packet(raw_data) {
        await USBDevice.transferOut(1, raw_data);
        console.log("transferout", raw_data);
    }

    async function send_chunk_headers (chunk_len, i) {
        let hex_len = (chunk_len + 52).toString(16); // 52 comes from headers
        await send_data(await str_to_arr(`download:${hex_len}`), `DATA${hex_len}`);
        await send_headers(chunk_len, i);
    }

    async function send_headers(raw_data_bytelength, i) {
        let sparse = await build_sparse_header(raw_data_bytelength, i);
        let dont_care = await build_chunk_header(CHUNK_TYPE_DONT_CARE, raw_data_bytelength, i);
        let raw = await build_chunk_header(CHUNK_TYPE_RAW, raw_data_bytelength, i);

        let headers = new Uint8Array(52);
        headers.set(sparse, 0);
        headers.set(dont_care, 28);
        headers.set(raw, 40);

        await USBDevice.transferOut(1, headers);
    }

    async function send_flash () {
        let result = await USBDevice.transferIn(1, 1048);
        if ("OKAY" !== await ab_to_str(result.data.buffer)) {
            throw new Error ("failed to send data:", await ab_to_str(result.data.buffer))
        }

        await flash_all();
    }

    /*
    * data: string or arraybuffer
    * success_str: checks response of usb
    * Throws error if USB input does not match success_str
    */
    async function send_data(data, success_str) {
        await USBDevice.transferOut(1, data);
        let result = await USBDevice.transferIn(1, 1048);

        if (success_str !== await ab_to_str(result.data.buffer)) {
            throw new Error ("failed to send data:",await ab_to_str(result.data.buffer))
        }
    }

    async function flash_all () {
        await send_data(await str_to_arr("flash:all"), "OKAY");
        console.log("flash");
    }

    const decompressFileToTransform = async(e) => {
        console.log(USBDevice);

        if (!flashFile) return;

        const file = flashFile;
        let buff = await file.arrayBuffer()
        let data = new Uint8Array(buff);
        let totalBytes = 0;

        console.log(data)

        if (!window.binding) {
            console.log("no binding");
            return;
        }

        const stream = new binding.ZstdDecompressStreamBinding();
        const transform = new TransformStream();
        const writer = transform.writable.getWriter();

        const callback = (decompressed) => {
            totalBytes += decompressed.length;
            writer.write(decompressed);
        }

        if (!stream.begin()) {
            console.log("stream.begin() error");
            return null;
        }

        console.log("start unzipping");

        let i = 0;
        const size = 1024*1024*2;
        while (size*i < data.length) {
            let end = Math.min(size*(i+1), data.length);
            let slice = data.slice(size*i, end);

            if (!stream.transform(slice, callback)) {
                console.log(`stream.transform() error on slice ${size*i} to ${end}`);
                return null;
            }

            i++;
        }

        if (!stream.end(callback)) {
            console.log("stream.end() error");
            return null;
        }

        console.log("finishing unzipping");

        const readable = transform.readable;
        doFlash(readable, PACKET_SZ, totalBytes);
    }

    const doFlash = async(readable, size, totalSize) => {
        await preBoot();

        const reader = readable.getReader();

        let offset = 0;
        let sofar = new Uint8Array(size);
        let i=0;
        let totalBytes = 0;

        let i_last = Math.floor(totalSize/DATA_SZ);
        let last_len = totalSize - Math.floor(totalSize/DATA_SZ)*DATA_SZ;

        console.log(i_last, last_len);

        let isFirst = true;
        let bytesChunk = 0;

        reader.read().then(async function processText ({ done, value }) {
            // Result objects contain two properties:
            // done  - true if the stream has already given you all its data.
            // value - some data. Always undefined when done is true.

            totalBytes += value.length;
            console.log(totalBytes);

            // Send chunk headers
            if (isFirst) {
                let send_len;
                if (i==i_last) {
                    send_len = Math.ceil(last_len/BLK_SZ)*BLK_SZ
                }
                else {
                    send_len = DATA_SZ;
                }
                await send_chunk_headers(send_len, i);
                console.log("Sent headers", send_len, i)

                isFirst = false;
            }

            while (offset + value.length >= size) {
                sofar.set(value.slice(0, size-offset), offset); // Whole packet is ready to send
                bytesChunk += size-offset;
                await send_packet(sofar);

                console.log("bytes", bytesChunk, i);

                if (bytesChunk == DATA_SZ) {
                    await send_flash();
                    bytesChunk = 0;
                    i++;
                    isFirst = true;
                }
                value = value.slice(size-offset, value.length);
                offset = 0;
            }

            sofar.set(value, offset);
            offset += value.length;
            bytesChunk += value.length

            if (i==i_last && bytesChunk==last_len) {
                console.log("reached last send", i, bytesChunk);

                // Pad last packet with zeros
                let fill_len = Math.ceil(last_len/BLK_SZ)*BLK_SZ - last_len;
                sofar.set(new Uint8Array(fill_len), offset);
                sofar = sofar.slice(0, offset+fill_len);

                console.log(sofar);

                await send_packet(sofar);
                await send_flash();
            }

            if (done) {
                console.log("stream done")
                return;
            }
            // Read some more, and call this function again
            return reader.read().then(processText);
          });
    }

    return [{
        requestUSBDevice,
        USBDevice,
        flashProgress,
        flashTotal,
        preBoot,
        processChunk,
        send_chunk_headers,
        send_packet,
        send_flash,
        flash_all,
    }]
}

export default useDecompress;