
import {useEffect, useState} from 'react';
import {str_to_arr, ab_to_str} from '../helper/functions.js'
import {CHUNK_SZ, BLK_SZ, CHUNK_TYPE_RAW, CHUNK_TYPE_DONT_CARE, build_sparse_header, build_chunk_header} from '../helper/sparse.js'

const USBrequestParams = { filters: [{ vendorId: 8137, productId: 0x0152 }] };

const PACKET_SZ = 0x10000;
const DATA_SZ = CHUNK_SZ * BLK_SZ; // in bytes

let cap = 2;

const decompress_sz = 1024*64;
let totalBytes = 0;

let buffer = new Uint8Array();

const useDecompress4 = (flashFile) => {
    const [USBDevice, setUSBDevice] = useState();
    const [flashProgress, setFlashProgress] = useState();
    const [flashTotal, setFlashTotal] = useState();
    const [stream, setStream] = useState();
    const [decompress_i, setDecompress_i] = useState(0);

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
        setTimeout(()=> { // takes a while for this to load?
            const s = new binding.ZstdDecompressStreamBinding();
            setStream(s);
        }, 500)
    }, [])

    
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
    async function send_data(data, success_str) {
        await USBDevice.transferOut(1, data);
        let result = await USBDevice.transferIn(1, 1048);

        if (success_str !== ab_to_str(result.data.buffer)) {
            console.log('err message', result.data.buffer);
            throw new Error ("failed to send data:", ab_to_str(result.data.buffer))
        }
        else {
            console.log("transfer in reply after sending", data);
        }
    }

    async function preBoot () {
        await USBDevice.claimInterface(0);

        await send_data(str_to_arr("UCmd:setenv fastboot_dev mmc"), "OKAY");
        await send_data(str_to_arr("UCmd:setenv mmcdev ${emmc_dev}"), "OKAY");
        await send_data(str_to_arr("UCmd:mmc dev ${emmc_dev}"), "OKAY");

        console.log("preboot complete")
    }

    /*
    * data: string or arraybuffer
    * success_str: checks response of usb
    * Throws error if USB input does not match success_str
    */
    
    /// new stuff

    const appendBuffer = (new_buffer) => {
        console.log("length", buffer.length + new_buffer.length);
        let temp = new Uint8Array(buffer.length + new_buffer.length);
        temp.set(buffer, 0);
        temp.set(new_buffer, buffer.length);
        // console.log("temp, buffer so far", temp);
        buffer = temp;
    }

    const callback = (decompressed) => {
        // console.log("callback")

        // if (decompressed.length + buffer.length < DATA_SZ) {
        //     appendBuffer(decompressed);
        // }

        // let padCount = DATA_SZ - buffer.length;
        // appendBuffer(decompressed.slice(0, padCount));
        // console.log(buffer); // do something

        // setBuffer(decompressed.slice(padCount));
        // while (true) {
        //     console.log(buffer.slice(0, DATA_SZ));
        //     let temp = buffer.slice(DATA_SZ);

        //     if (temp.length<DATA_SZ) {
        //         setBuffer(temp);
        //         break;
        //     }
        // }

        // appendBuffer(decompressed);
        // let temp = new Uint8Array(buffer.length + decompressed.length);
        // temp.set(buffer, 0);
        // temp.set(decompressed, buffer.length);
        // buffer = temp;

        // console.log(buffer);
        // totalBytes += decompressed.length;
        // console.log(totalBytes);

        totalBytes+= decompressed.length;
    }


    const decompressFileToTransform = async() => {
        console.log("binding", stream);
        await preBoot(); // problem was you didn't reset the board
        console.log("after preboot");

        if (!flashFile) return;

        const file = flashFile;
        let data = new Uint8Array(await file.arrayBuffer());

        if (!window.binding) {
            console.log("no binding");
            return;
        }

        // let num_callbacks = 0;
        // let num_chunks = 0;
        // let curr_chunk = new Uint8Array(DATA_SZ);
        // let curr_chunk_offset = 0;

        

        // const callback = (decompressed) => {
        //     if (num_chunks==cap) return;
        //     totalBytes += decompressed.length;
        //     while(curr_chunk_offset + decompressed.length >= DATA_SZ) {
        //         curr_chunk.set(decompressed.slice(0, DATA_SZ - curr_chunk_offset), curr_chunk_offset);

        //         // process_chunk(curr_chunk, DATA_SZ, num_chunks);
        //         process_chunk2(curr_chunk, num_chunks);
                
        //         curr_chunk_offset = 0;
        //         decompressed = decompressed.slice(DATA_SZ - curr_chunk_offset, decompressed.length);
        //         num_chunks++;
        //     }
        //     curr_chunk.set(decompressed, curr_chunk_offset);
        //     curr_chunk_offset += decompressed.length;
        //     num_callbacks++;
        // }

        // console.log("start unzipping");

        if (!stream.begin()) {
            console.log("stream.begin() error");
            return null;
        }

        let offset = 0;
        while (offset < data.length) {
            let slice = data.slice(offset, offset + decompress_sz);
            let res = await stream.transform(slice, callback);

            if (!res) {console.log("stream transform fail"); return null;}

            // console.log("buffer after transform", buffer);
            // buffer = new Uint8Array();
            console.log("bytes after transform", totalBytes);
            totalBytes = 0;
            offset += decompress_sz;
        }

        if (!stream.end(callback)) {
            console.log("stream.end() error");
            return null;
        }

        // let i = 0;
        // const size = 1024*1024*2; // 2MB of data at a time
        // // while (size*i < data.length) {
        // while (size*i < data.length) {
        //     let end = Math.min(size*(i+1), data.length);
        //     let slice = data.slice(size*i, end);

        //     console.log("loop running transform")
        //     if (!stream.transform(slice, callback)) {
        //         console.log(`stream.transform() error on slice ${size*i} to ${end}`);
        //         return null;
        //     }
        //     // console.log(`transform on ${size*i} to ${end}`);

        //     i++;
        // }

        // if (!stream.end(callback)) {
        //     console.log("stream.end() error");
        //     return null;
        // }
    }

    return [{
        requestUSBDevice,
        decompressFileToTransform
    }]
}

export default useDecompress4;
