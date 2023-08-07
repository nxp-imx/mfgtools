
import {useEffect, useState} from 'react';
import {str_to_arr, ab_to_str} from '../helper/functions.js'
import {CHUNK_SZ, BLK_SZ, CHUNK_TYPE_RAW, CHUNK_TYPE_DONT_CARE, build_sparse_header, build_chunk_header} from '../helper/sparse.js'

const USBrequestParams = { filters: [{ vendorId: 8137, productId: 0x0152 }] };

const PACKET_SZ = 0x10000;
const DATA_SZ = CHUNK_SZ * BLK_SZ; // in bytes

let cap = 2;

const useDecompress3 = (flashFile) => {
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

    /*
        breaking down this function:
        USBDevice.claimInterface is an async function that would run off to the side
        BUT if we await it, then we wait for it to finish
        only then can we send any data to the usb, because we have claimed an interface

        await USBDevice.claimInterface is the same as saying:
        USBDevice.claimInterface() returns a Promise, and then on the .then(),
        you can follow up with things
        ...
        otherwise, claimInterface is an async function that runs off to the side
    */

    async function preBoot () {
        console.log("start preBoot");
        await USBDevice.claimInterface(0);

        send_data(str_to_arr("UCmd:setenv fastboot_dev mmc"), "OKAY");
        send_data(str_to_arr("UCmd:setenv mmcdev ${emmc_dev}"), "OKAY");
        send_data(str_to_arr("UCmd:mmc dev ${emmc_dev}"), "OKAY");

        console.log("preboot complete")
    }

    async function send_headers(raw_data_bytelength, i) {
        console.log("start send_headers", i);
        let sparse = build_sparse_header(raw_data_bytelength, i);
        let dont_care = build_chunk_header(CHUNK_TYPE_DONT_CARE, raw_data_bytelength, i);
        let raw = build_chunk_header(CHUNK_TYPE_RAW, raw_data_bytelength, i);

        let headers = new Uint8Array(52);
        headers.set(sparse, 0);
        headers.set(dont_care, 28);
        headers.set(raw, 40);

        await USBDevice.transferOut(1, headers);
        console.log("sent headers", headers);
    }

    async function send_packet(raw_data, flag="", callback) {
        console.log("start send_packet", raw_data, flag);
        USBDevice.transferOut(1, raw_data);
        callback();
    }

    /*
    * data: string or arraybuffer
    * success_str: checks response of usb
    * Throws error if USB input does not match success_str
    */
    async function send_data(data, success_str) {
        console.log("start send_data", data);
        await USBDevice.transferOut(1, data);
        console.log("after transferOut", data);

        let result = await USBDevice.transferIn(1, 1048);

        if (success_str !== ab_to_str(result.data.buffer)) {
            console.log('err with', data);
            console.log('err message', result.data.buffer);
            throw new Error ("failed to send data:",ab_to_str(result.data.buffer))
        }
        else {
            console.log("after transferIn for", data);
        }
    }

    async function wait_reply(data, success_str) {
        let result = await USBDevice.transferIn(1, 1048);

        if (success_str !== ab_to_str(result.data.buffer)) {
            console.log('err with', data);
            console.log('err message', result.data.buffer);
            throw new Error ("failed to send data:",ab_to_str(result.data.buffer))
        }
        else {
            console.log("after transferIn for", data);
        }
    }

    async function flash_all () {
        await send_data(str_to_arr("flash:all"), "OKAY");
        console.log("sent flash");
    }

    function dostuff (chunk) {
        //last packet: receive "OKAY" after
        send_packet(chunk.slice(chunk.length-PACKET_SZ), `${chunk.length/PACKET_SZ - 1}, last`);
        wait_reply(chunk.slice(chunk.length-PACKET_SZ), "OKAY");

        //flash: receive "OKAY" after
        send_packet(str_to_arr("flash:all"));
        wait_reply(str_to_arr("flash:all"), "OKAY");
    }
    
    const process_chunk2 = (chunk, i) => {
        console.log("process chunk", chunk);

        let hex_len = (chunk.length + 52).toString(16); // 52 comes from headers

        // before any writes: receive `DATA${length}` after
        send_packet(str_to_arr(`download:${hex_len}`));
        wait_reply(str_to_arr(`download:${hex_len}`), `DATA${hex_len}`);

        send_headers(chunk.length, i);

        let offset=0;
        while (offset < (chunk.length - PACKET_SZ)) {
            // let packet_len = Math.min(PACKET_SZ, DATA_SZ - offset);
            let packet_len = PACKET_SZ;
            let packet = chunk.slice(offset, offset + packet_len);
           
            send_packet(packet, offset/PACKET_SZ);
        
            offset += packet_len;
        }

        dostuff(chunk);
        
    }

    //for reference:

    const process_chunk_doesnt = (data, curr_len, i) => {
        console.log(data, curr_len, i);

        let hex_len = (data.length + 52).toString(16); // 52 comes from headers
        send_data(str_to_arr(`download:${hex_len}`), `DATA${hex_len}`);
        send_headers(data.length, i);

        let last_packet_i = data.length/PACKET_SZ-1;
        for (let i=0; i<last_packet_i; i++) {
            let packet_len = PACKET_SZ;
            let packet = data.slice(i*PACKET_SZ, i*PACKET_SZ + packet_len);
            send_packet(packet);
        }
        let last_packet = data.slice((last_packet_i)*PACKET_SZ);
        send_data(last_packet, "OKAY")

        flash_all();
    }

    const process_chunk = (data, curr_len, i) => {
        console.log(data, curr_len, i);

        let hex_len = (data.length + 52).toString(16); // 52 comes from headers
        USBDevice.transferOut(1, str_to_arr(`download:${hex_len}`)).then(()=> {
            USBDevice.transferIn(1, 1048).then(result => {
                console.log(result);

                if (`DATA${hex_len}` !== ab_to_str(result.data.buffer)) {
                    console.log('err with', data);
                    console.log('err message', result.data.buffer);
                    throw new Error ("failed to send data:",ab_to_str(result.data.buffer))
                }
                else {
                    console.log("sent data", data);
                }
            })
        })
        
        send_headers(data.length, i);

        let last_packet_i = data.length/PACKET_SZ-1;
        for (let i=0; i<last_packet_i; i++) {
            let packet_len = PACKET_SZ;
            let packet = data.slice(i*PACKET_SZ, i*PACKET_SZ + packet_len);
            USBDevice.transferOut(1, packet);
        }
        let last_packet = data.slice((last_packet_i)*PACKET_SZ);

        USBDevice.transferOut(1, last_packet).then(()=> {
            USBDevice.transferIn(1, 1048).then(result => {
                console.log(result);

                if ('OKAY' !== ab_to_str(result.data.buffer)) {
                    console.log('err with', data);
                    console.log('err message', result.data.buffer);
                    throw new Error ("failed to send data:",ab_to_str(result.data.buffer))
                }
                else {
                    console.log("sent data", data);
                }
            })
        })
        
        USBDevice.transferOut(1, str_to_arr("flash:all")).then(()=> {
            USBDevice.transferIn(1, 1048).then(result => {
                console.log(result);

                if ('OKAY' !== ab_to_str(result.data.buffer)) {
                    console.log('err with', data);
                    console.log('err message', result.data.buffer);
                    throw new Error ("failed to send data:",ab_to_str(result.data.buffer))
                }
                else {
                    console.log("sent data", data);
                }
            })
        })
    }

    const decompressFileToTransform = async() => {
        console.log("decompressFileToTransform");
        console.log("USBDevice", USBDevice);

        preBoot();

        if (!flashFile) return;

        console.log("file", flashFile)
        const file = flashFile;
        let data = new Uint8Array(await file.arrayBuffer());
        let totalBytes = 0;
        console.log(data)

        if (!window.binding) {
            console.log("no binding");
            return;
        }

        let num_callbacks = 0;
        let num_chunks = 0;
        let curr_chunk = new Uint8Array(DATA_SZ);
        let curr_chunk_offset = 0;

        const stream = new binding.ZstdDecompressStreamBinding();

        const callback = (decompressed) => {
            if (num_chunks==cap) return;
            totalBytes += decompressed.length;
            while(curr_chunk_offset + decompressed.length >= DATA_SZ) {
                curr_chunk.set(decompressed.slice(0, DATA_SZ - curr_chunk_offset), curr_chunk_offset);

                // process_chunk(curr_chunk, DATA_SZ, num_chunks);
                process_chunk2(curr_chunk, num_chunks);
                
                curr_chunk_offset = 0;
                decompressed = decompressed.slice(DATA_SZ - curr_chunk_offset, decompressed.length);
                num_chunks++;
            }
            curr_chunk.set(decompressed, curr_chunk_offset);
            curr_chunk_offset += decompressed.length;
            num_callbacks++;
        }

        const end_callback = () => {
            console.log("end");
            console.log(totalBytes);

            console.log(curr_chunk, curr_chunk_offset) // different
            curr_chunk = curr_chunk.slice(0, curr_chunk_offset);
            console.log(curr_chunk);
        }

        console.log("start unzipping");

        if (!stream.begin()) {
            console.log("stream.begin() error");
            return null;
        }

        let i = 0;
        const size = 1024*1024*2; // 2MB of data at a time
        // while (size*i < data.length) {
        while (size*i < data.length) {
            let end = Math.min(size*(i+1), data.length);
            let slice = data.slice(size*i, end);

            console.log("loop running transform")
            if (!stream.transform(slice, callback)) {
                console.log(`stream.transform() error on slice ${size*i} to ${end}`);
                return null;
            }
            // console.log(`transform on ${size*i} to ${end}`);

            i++;
        }

        if (!stream.end(callback)) {
            console.log("stream.end() error");
            return null;
        }

        // // end_callback();

        // console.log("finishing unzipping");

        // console.log(num_callbacks);
        // console.log(num_chunks);
    }

    return [{
        requestUSBDevice,
        decompressFileToTransform
    }]
}

export default useDecompress3;
