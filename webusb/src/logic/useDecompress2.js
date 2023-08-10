

import {useEffect, useRef, useState} from 'react';
import {str_to_arr, ab_to_str} from '../helper/functions.js'
import {CHUNK_SZ, BLK_SZ, CHUNK_TYPE_RAW, CHUNK_TYPE_DONT_CARE, build_sparse_header, build_chunk_header} from '../helper/sparse.js'

const USBrequestParams = { filters: [{ vendorId: 8137, productId: 0x0152 }] };

const PACKET_SZ = 0x10000;
const DATA_SZ = CHUNK_SZ * BLK_SZ; // in bytes

const size = 1024*1024;

const test_data_sz = 262144;

import ChunkProcessor from './flashMachine.js';
import {preboot, process_chunk} from './useFlash.js'

const useDecompress2 = (flashFile) => {
    const chunk_processor = useRef();
    const total = useRef(0);
    const stream = useRef();
    const data = useRef();
    const i = useRef(0);
    const chunk_i = useRef(0);


    const [USBDevice, setUSBDevice] = useState();
    const [flashProgress, setFlashProgress] = useState();
    const [flashTotal, setFlashTotal] = useState();

    useEffect(()=> {
        console.log("first")
        // change back
        if (!flashFile) return;

        (async() => {
            data.current = new Uint8Array(await flashFile.arrayBuffer());
        })()
    }, [flashFile])

    // things to test: do you have to await on 
    const processChunk = async(chunk) => {
        console.log('processing...');
        let res = await p2();
        console.log(res);
        res = await p3();
        console.log(res);
        await p3();

        await process_chunk(USBDevice, chunk);
        // chunk_i.current += 1; TODO: CHANGE THIS FOR ME

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
        // change back
        // setUSBDevice(1);
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

        // await chunk_processor.current.push_data(decompressed);
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
        if (size*i.current >= data.current.length) {
            console.log("at the end")
            console.log(total.current);
            if(!stream.current.end(callback)) {
                console.log("stream.end() fail");
                return;
            }
            return;
        }

        let chunk = data.current.slice(size*i.current, Math.min(size*(i.current+1), data.current.length));
        i.current += 1;

        if (!stream.current.load(chunk)) {
            console.log("stream.load() error");
            return null;
        }
        stream.current.read(callback);
    }

    const decompressFileToTransform = async(e) => {
        console.log("decompressFileToTransform", data.current);

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
    }]
}

export default useDecompress2;
