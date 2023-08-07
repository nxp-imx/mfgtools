import { useEffect, useState } from "react"
import useDecompress4 from '../logic/useDecompress2'
import { DATA_SZ, BLK_SZ, PACKET_SZ } from "../helper/sparse";

const Decompress2 = () => {
    const [flashFile, setFlashFile] = useState();

    const [{
        requestUSBDevice,
        decompressFileToTransform
    }] = useDecompress4(flashFile);

    useEffect(()=> {
        console.log(ZstdCodec)
        console.log(window.binding);
    }, [])

    // async test
    // useEffect(()=> {
    //     function sleep(ms) {
    //         let p1 = new Promise(resolve => setTimeout(resolve, ms));
    //         let p2 = p1.then(() => {
    //             console.log('after first setTimeout')
    //             return (
    //                 new Promise(resolve => setTimeout(resolve, ms))
    //             )
    //         });
    //         return p2.then(()=> Promise.resolve("hello")); // returns a promise
    //       }
          
    //       // Usage:

          
    //       console.log("Start");
          
    //       (async () => {
    //         for (let i=0; i<3; i++) {
    //             let res = await sleep(2000); // Sleep for 2 seconds
    //             console.log(res);
    //         }
    //         console.log("End");
    //       })();        
    // }, [])

    return (
        <div>
            file to decompress:
            <input type="file" onChange={(e) => setFlashFile(e.target.files[0])}/>

            <button onClick={requestUSBDevice}>Choose USB Dev </button>
        </div>
    )
}

export default Decompress2
