import { useEffect, useState } from "react"
import useDecompress2 from '../logic/useDecompress2'

const Decompress = () => {
    const [flashFile, setFlashFile] = useState();
    const [{
        requestUSBDevice
    }] = useDecompress2(flashFile);

    useEffect(()=> {
        console.log(ZstdCodec)
        console.log(window.binding);
    }, [])

    return (
        <div>
            file to decompress:
            <input type="file" onChange={(e) => setFlashFile(e.target.files[0])}/>

            <button onClick={requestUSBDevice}>Pair USBDevice </button>
        </div>
    )
}

export default Decompress
