

import { useEffect, useRef, useState } from 'react';
import { CHUNK_SZ, BLK_SZ } from '../helper/sparse.js'
import ChunkProcessor from './ChunkProcessor.js';
import { preboot, process_chunk } from './useFlash.js'

const USBrequestParams = { filters: [{ vendorId: 8137, productId: 0x0152 }] };
const DATA_SZ = CHUNK_SZ * BLK_SZ; // in bytes
const size = 1024*1024; // for decompressing

const useDecompress2 = (flashFile) => {
    const chunk_processor = useRef();
    const total = useRef(0);
    const stream = useRef();
    const data = useRef();
    const i = useRef(0); // for indexing through flashfile
    const chunk_i = useRef(0); // for building chunk headers

    // const flashProgress = useRef();
    // const flashTotal = useRef();

    const [USBDevice, setUSBDevice] = useState();
    const [flashProgress, setFlashProgress] = useState(0);
    const [flashTotal, setFlashTotal] = useState();

    useEffect(()=> {
        console.log("first")
        // change back
        if (!flashFile) return;

        // sets data from flashFile
        (async() => {
            data.current = new Uint8Array(await flashFile.arrayBuffer());
        })()
    }, [flashFile])

    // things to test: do you have to await on 
    const processChunk = async(chunk) => {
        console.log('processing...');

        await process_chunk(USBDevice, chunk, chunk_i.current);
        chunk_i.current ++;

        console.log("COMPLETED ONE CHUNK");
        console.log(chunk);
    }

    useEffect (()=> {
        if (!USBDevice) return;

        if (!data.current) {
            console.log("no data");
            return;
        }

        USBDevice.open().then(()=>{
            console.log(USBDevice);
            console.log(`Opened USB Device: ${USBDevice.productName}`);
        }, (err)=> {
            console.log(err)
        })
        .then(()=> {
            // ready to decompress the file
        
            let cp = new ChunkProcessor(DATA_SZ, processChunk);
            chunk_processor.current = cp;
            
            decompressFileToTransform();
        })
        
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

    // ///// FLASH PROCESSES /////

    const p1 = () => {
        return new Promise((res, rej) => {
            setTimeout(()=>{
                res(1);
            }, 0)
        })
    }

    const p2 = () => {
        return new Promise((res, rej) => {
            setTimeout(()=>{
                res(2);
            }, 1000)
        })
    }

    const p3 = () => {
        return new Promise((res, rej) => {
            setTimeout(()=>{
                res(3);
            }, 1000)
        })
    }

    // ///// FLASH PROCESSES //////

    const callback = async(decompressed) => {
        // execute async stuff
        // let res = await p1(); // TODO: why do we have to have this for this to work?
        // console.log(res);

        // res = await p2();
        // console.log(res);
        // res = await p3();
        // console.log(res);

        console.log(decompressed);
        total.current += decompressed.length;

        

        await chunk_processor.current.push_data(decompressed);

        // uncomment below this
        if (!stream.current.read(callback)){
            // load in a new chunk
            console.log("NEED NEW CHUNK NOM")

            //test this next:
            doLoad();
        }
    }

    // // need to load a chunk of data into zstd-read stream
    const doLoad = () => {
        // reached the end of processing data
        setFlashProgress(Math.min(i.current*size, data.current.length));
        
        if (size*i.current >= data.current.length) {
            console.log("at the end")
            console.log(total.current);
            if(!stream.current.end(callback)) {
                console.log("stream.end() fail");
                return;
            }
            chunk_processor.current.flush(processChunk);
            return;
        }
        let chunk = data.current.slice(size*i.current, size*(i.current+1));
        
        i.current ++;

        if (!stream.current.load(chunk)) {
            console.log("stream.load() error");
            return null;
        }
        stream.current.read(callback);
    }

    const decompressFileToTransform = async(e) => {
        console.log("decompressFileToTransform", data.current);

        setFlashTotal(data.current.length);
        // flashTotal.current = data.current.length;

        await preboot(USBDevice);

        if (!window.binding) {
            console.log("no binding");
            return;
        }

        stream.current = new binding.ZstdDecompressReadBinding();
        
        if (!stream.current.begin()) {
            console.log("stream.begin() error");
            return null;
        }

        doLoad();
    }

    return [{
        requestUSBDevice,
        USBDevice,
        flashProgress,
        flashTotal,
    }]
}

export default useDecompress2;
