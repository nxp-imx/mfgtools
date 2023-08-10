import { useEffect, useState } from "react"
// import useDecompress from '../logic/useDecompress'
import useDecompress from '../logic/useDecompress3'
import { DATA_SZ, BLK_SZ, PACKET_SZ } from "../helper/sparse";

const DecompressAlt = () => {
    const [flashFile, setFlashFile] = useState();
    const [{
        requestUSBDevice
    }] = useDecompress(flashFile);

    useEffect(()=> {
        console.log(ZstdCodec)
        console.log(window.binding);
    }, [])

    return (
        <div>
            file to decompress (3):
            <input type="file" onChange={(e) => setFlashFile(e.target.files[0])}/>

            <button onClick={requestUSBDevice}>Pair USBDevice </button>
        </div>
    )
}

export default DecompressAlt
