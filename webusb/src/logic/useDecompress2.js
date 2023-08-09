

import {useEffect, useState} from 'react';
import { unstable_renderSubtreeIntoContainer } from 'react-dom';
import {str_to_arr, ab_to_str} from '../helper/functions.js'
import {CHUNK_SZ, BLK_SZ, CHUNK_TYPE_RAW, CHUNK_TYPE_DONT_CARE, build_sparse_header, build_chunk_header} from '../helper/sparse.js'

const USBrequestParams = { filters: [{ vendorId: 8137, productId: 0x0152 }] };

const PACKET_SZ = 0x10000;
const DATA_SZ = CHUNK_SZ * BLK_SZ; // in bytes

const size = 1024*1024;

const useDecompress2 = (flashFile) => {
    const [USBDevice, setUSBDevice] = useState();
    const [flashProgress, setFlashProgress] = useState();
    const [flashTotal, setFlashTotal] = useState();
    const [chunkOffset, setChunkOffset] = useState(-1);
    const [stream, setStream] = useState();
    const [data, setData] = useState();
    const [i, setI] = useState(0);
    const [chunk, setChunk] = useState();

    useEffect(()=> {
        if (!flashFile) return;

        (async() => {
            let d = new Uint8Array(await flashFile.arrayBuffer());
            setData(d);
        })()
        

    }, [flashFile])

    useEffect (()=> {

        if (flashFile) decompressFileToTransform();

    }, [USBDevice])

    useEffect(()=> {
        // called when another thing is processed from src_bytes

        if (chunkOffset < 0) return;

        console.log(chunkOffset);
    }, [chunkOffset])

    const requestUSBDevice = () => { // first thing called with button
        setUSBDevice(1);
    }

    const another = () => {
        let p = new Promise((resolve, reject) => {
            setTimeout(()=> resolve(3), 3000);
        })
        return p;
    }
    const callbackold = async() => {
        // console.log(1);
        // let p1 = new Promise((resolve, reject) => {
        //     setTimeout(()=> {
        //         resolve(2);
        //     }, 3000)
        // })
        // let res = await p1;
        // console.log(res);
        // res = await another();
        // console.log(res);
        // read(callback)
    }

    const readold = (cb) => {
        // console.log("read");
        // cb();
    }

    const decompressFileToTransformold = async(e) => {
        // read(callback);
    }

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
            }, 3000)
        })
    }

    const p3 = () => {
        return new Promise((res, rej) => {
            setTimeout(()=>{
                res(3);
            }, 3000)
        })
    }

    const callback = async(decompressed) => {
        let res = await p1();
        console.log(res);
        // res = await p2();
        // console.log(res);
        // res = await p3();
        // console.log(res);

        console.log(decompressed);
        if (!stream.read(callback)){
            // load in a new chunk
            console.log("NEED NEW CHUNK NOM")

            //test this next:
            // doLoad();
        }
    }

    useEffect(()=> {
        // after updating i, know it's ready to load a chunk
        if (!chunk) return;

        if (!stream.load(chunk)) {
            console.log("stream.load() error");
            return null;
        }
        stream.read(callback);
    }, [i])

    useEffect(()=> {
        if (!chunk) return;

        let new_i = i+1;
        setI(new_i);
    }, [chunk])

    const doLoad = async() => {
        // if size*i < data.length? needed?
        let chunk = data.slice(size*i, Math.min(size*(i+1), data.length));
        setChunk(chunk);
    }

    useEffect(() => {
        if (!stream) return;

        (async() => {
            // let data = new Uint8Array(await flashFile.arrayBuffer());
            // let chunk = data.slice(0, size);
            // i++;
            if (!stream.begin()) {
                console.log("stream.begin() error");
                return null;
            }
            // if (!stream.load(chunk)) {
            //     console.log("stream.load() error");
            //     return null;
            // }

            // stream.read(callback);

            doLoad();
        })()
    }, [stream])

    const decompressFileToTransform = async(e) => {
        console.log("data", data);
        // read(callback);

        if (!window.binding) {
            console.log("no binding");
            return;
        }

        const s = new binding.ZstdDecompressReadBinding();
        setStream(s);
    }

    return [{
        requestUSBDevice,
    }]
}

export default useDecompress2;