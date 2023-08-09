
import {useEffect, useState} from 'react';
import {str_to_arr, ab_to_str} from '../helper/functions.js'
import {CHUNK_SZ, BLK_SZ, CHUNK_TYPE_RAW, CHUNK_TYPE_DONT_CARE, build_sparse_header, build_chunk_header} from '../helper/sparse.js'

const USBrequestParams = { filters: [{ vendorId: 8137, productId: 0x0152 }] };

const PACKET_SZ = 0x10000;
const DATA_SZ = CHUNK_SZ * BLK_SZ; // in bytes

let promises = [];
let promise1 = Promise.resolve(0)

let cap = 2;

const size = 1024*1024;

let counter = 0;

const useDecompress2 = (flashFile) => {
    const [USBDevice, setUSBDevice] = useState();
    const [flashProgress, setFlashProgress] = useState();
    const [flashTotal, setFlashTotal] = useState();
    const [chunk, setChunk] = useState();

    const [data, setData] = useState();
    const [i, setI] = useState(0);
    const [stream, setStream] = useState();

    const [currOffset, setCurrOffset] = useState(0);
    const [prevOffset, setPrevOffset] = useState(0);
    const [currPos, setCurrPos] = useState(0);

    useEffect (()=> {
        // if (USBDevice) {
        //     USBDevice.open().then(()=>{
        //         console.log(`Opened USB Device: ${USBDevice.productName}`);
        //     }, (err)=> {
        //         console.log(err)
        //     })
        //     .then(()=> {
        //         if (flashFile)
        //             doDecompress();
        //     })
        // }

        if (flashFile) doDecompress();

    }, [USBDevice])

    const requestUSBDevice = () => { // first thing called with button
        // navigator.usb
        // .requestDevice(USBrequestParams)
        // .then((device) => {
        //     setUSBDevice(device);
        // })
        // .catch((e) => {
        //     console.error(`There is no device. ${e}`);
        // });

        setUSBDevice(1);
    }   


    // async test
    async function dothingsinorder (a) {
        console.log("start of", a);

        function sleep(i, ms){
            let pq = new Promise((resolve, reject) => {
                setTimeout(()=> {
                    resolve("hi", i);
                }, ms)
            })

            return pq;
        }
          // Usage:

        for (let i=0; i<3; i++) {
            await sleep(i, 2000); // Sleep for 2 seconds
            console.log("end of", i)
        }

        console.log("end of ", a);
    }
    
    const cb = async(decompressed) => {
        console.log(counter, decompressed);
        counter ++;

        if (counter < 3) {
            stream.read(cb);
        }
    }

    useEffect(()=> {
        (async()=> {
            if (!stream) return;

            if (!stream.begin()) {
                console.log("stream.begin() error");
                return null;
            }

            // get first chunk
            let data = new Uint8Array(await flashFile.arrayBuffer());
            let chunk = data.slice(0, size);

            if (!stream.load(chunk)) {
                console.log("failed");
                return null;
            }

            stream.read(cb);

        })();

    }, [stream])

    const doDecompress = async(e) => {
        //get data
        

        if (!window.binding) {
            console.log("no binding");
            return;
        }

        // const stream = new binding.ZstdDecompressStreamBinding();
        // setStream(stream);
        const s = new binding.ZstdDecompressReadBinding();
        setStream(s);  
    }
    
    const doDecompress1 = async(e) => {
        const file = flashFile;
        let data = new Uint8Array(await file.arrayBuffer());

        // setup stream
        if (!window.binding) {
            console.log("no binding");
            return;
        }

        const stream = new binding.ZstdDecompressStreamBinding();
        if (!stream.begin()) {
            console.log("stream.begin() error");
            return null;
        }

        // setup decompress loop
        let prev_chunk_offset = 0;
        let curr_chunk_offset = 0;
        let curr_pos = 0;

        let i = 0;
        const size = 1024*1024;

        const callback = async(decompressed) => {
            console.log(decompressed);
        }

        while (size*i < size) {
            // for every chunk
            let chunk = data.slice(size*i, Math.min(size*(i+1), data.length));
            console.log(chunk);

            while (curr_chunk_offset < size) {
                // keep running transform
                let pos = await stream.transform(chunk, curr_chunk_offset, curr_pos, callback);

                if (pos == 0) {
                    //  means that when we 
                    prev_chunk_offset = curr_chunk_offset;
                    continue;
                }

                curr_chunk_offset = prev_chunk_offset + pos;
                curr_pos = 0 + pos;
                console.log("offset in chunk,", curr_chunk_offset);
                console.log("offset in src_bytes_", curr_pos);
                await dothingsinorder(pos); // still doesn't work how I want it to
            }

            //reset chunk offsets
            curr_chunk_offset = 0;
            prev_chunk_offset = 0;
            i++
        } // this should look like 131072 bytes, pos 1, pos 2, pos 3

       


        // const stream = new binding.ZstdDecompressStreamBinding();
        // let pos = await stream.transform(chunk, 0, 0, callback);

        // when you get -2 from transform -> need to feed a new chunk to transform
        // when you get 0 from transform -> src_bytes have already finished, need to set pos to 0
    }

 
    const decompressFileToTransform2 = async(e) => {
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

        const callback = (decompressed) => {
            console.log(decompressed);
        }

        if (!stream.begin()) {
            console.log("stream.begin() error");
            return null;
        }

        console.log("start unzipping");

        let i = 0;
        const size = 1024*1024*2;
      
        let end = Math.min(size*(i+1), data.length);
        let chunk = data.slice(size*i, end);

        let prev_chunk_offset = 0;
        let curr_chunk_offset = 0;

        let curr_pos = 0;

        let pos = await stream.transform(chunk, 0, 0, callback);
        await dothingsinorder("first");
        curr_chunk_offset = prev_chunk_offset + pos;
        curr_pos = 0 + pos;

        console.log(curr_chunk_offset, curr_pos);

        pos = await stream.transform(chunk, curr_chunk_offset, curr_pos, callback);
        
        console.log(pos)
        if (pos == 0) {
            // means you need to reset for a new src_bytes_
            prev_chunk_offset = curr_chunk_offset; // because pos resets every src_bytes_ 
            
            pos = await stream.transform(chunk, curr_chunk_offset, 0, callback);
            await dothingsinorder("second");
            curr_pos = 0 + pos;
            curr_chunk_offset = prev_chunk_offset + pos;
            console.log(curr_pos, curr_chunk_offset);

            pos = stream.transform(chunk, curr_chunk_offset, curr_pos, callback);
            await dothingsinorder("third");
        }

        // if (!stream.end(callback)) {
        //     console.log("stream.end() error");
        //     return null;
        // }
    }

    return [{
        requestUSBDevice,
        doDecompress
    }]
}

export default useDecompress2;
