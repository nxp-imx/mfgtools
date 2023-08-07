
import {useEffect, useState} from 'react';
import {str_to_arr, ab_to_str} from '../helper/functions.js'
import {CHUNK_SZ, BLK_SZ, CHUNK_TYPE_RAW, CHUNK_TYPE_DONT_CARE, build_sparse_header, build_chunk_header} from '../helper/sparse.js'

const USBrequestParams = { filters: [{ vendorId: 8137, productId: 0x0152 }] };

const PACKET_SZ = 0x10000;
const DATA_SZ = CHUNK_SZ * BLK_SZ; // in bytes

const useDecompress2 = (flashFile) => {
    const [USBDevice, setUSBDevice] = useState();
    const [flashProgress, setFlashProgress] = useState();
    const [flashTotal, setFlashTotal] = useState();
    const [chunkOffset, setChunkOffset] = useState(-1);

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

    useEffect(()=> {
        // called when another thing is processed from src_bytes

        if (chunkOffset < 0) return;

        console.log(chunkOffset);
    }, [chunkOffset])

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
        // const transform = new TransformStream();
        // const writer = transform.writable.getWriter();

        const callback = (decompressed) => {
            console.log(decompressed);
            // totalBytes += decompressed.length;
            // writer.write(decompressed);
        }

        if (!stream.begin()) {
            console.log("stream.begin() error");
            return null;
        }

        console.log("start unzipping");

        let i = 0;
        const size = 1024*1024;
        
        let end = Math.min(size*(i+1), data.length);
        let chunk = data.slice(size*i, end);

        let prev_offset = 0;
        let curr_offset = 0;
        let printpos = 0;
        let pos = stream.transform(chunk, 0, 0, callback);
        console.log({pos});
        printpos = printpos + pos;
        curr_offset = prev_offset + pos;

        if (curr_offset == chunk.length) {
            //read another chunk
        }
        else {
            // still in the middle of reading a chunk
            console.log("pos", pos, "curr_offset", curr_offset, "printpos", printpos);
            let pos = stream.transform(chunk, 131075, 131075, callback);
            console.log(pos);

            if (pos==0) {
                // it's time to move onto a new src_bytes
                prev_offset = curr_offset;

                pos = stream.transform(chunk, 131075, 0, callback);
                console.log(pos);
                curr_offset = prev_offset + pos;
                console.log(curr_offset);
                    
                pos = stream.transform(chunk, curr_offset, pos, callback);
            }

        }


        // if (!stream.end(callback)) {
        //     console.log("stream.end() error");
        //     return null;
        // }

        // console.log("finishing unzipping");

        // const readable = transform.readable;
        // doFlash(readable, PACKET_SZ, totalBytes);
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

export default useDecompress2;