import {useEffect, useState} from 'react';
import ProgressBar from './ProgressBar';
import useHIDBoot from '../logic/useHIDBoot';
import useUSBFlash from '../logic/useUSBFlash'
import imgSrc from '../images/imx8qxp_mek_bootmode.png'

import "./Combined.css"

const Combined = ({bootFile, flashFile}) => {
    // const [imgUrl, setImgUrl] = useState();

    const [{ requestHIDDevice, HIDdevice, bootProgress, bootTotal }] = useHIDBoot(bootFile);
    const [{ requestUSBDevice, USBDevice, flashProgress, flashTotal}] = useUSBFlash(flashFile);

    // useEffect(() => {
    //     const dothis = async()=>{
    //         const response = await fetch("mode");
    //         const blob = await response.blob();
    //         setImgUrl(URL.createObjectURL(blob));
    //     }
    //     dothis();
    // }, [])

    return(
        <div>
            <ul className="Popup-list">
                <li>
                    <span>1. Switch Boot Mode to "SERIAL"</span>
                    <div className="u-flex u-center">
                        <div className="image-container">
                        {imgSrc? <img className="boot-image" src={imgSrc}/>: ""}
                        </div>
                    </div>
                </li>
                <li>
                    <div className="u-flex">
                        <div className="checkbox-container">
                            {bootProgress&&(bootProgress===bootTotal)?<p className="checkbox">&#x2714;</p>:""}
                        </div>
                        <div>
                            <span>2. </span>
                            <button className="Popup-button" onClick={requestHIDDevice}> Pair HIDdevice </button>
                            <span>{bootProgress? `${bootProgress} bytes out of ${bootTotal} downloaded`: ""}</span>
                            <div className="Popup-empty">
                                {bootProgress&&(bootProgress===bootTotal)?`connected: ${HIDdevice.productName}`: (bootProgress?`Pairing`:"")}
                            </div>
                        </div>
                    </div>
                </li>
                <li >
                    <div className="u-flex">
                        <div className="checkbox-container">
                            {USBDevice?<p className="checkbox">&#x2714;</p>:""}
                        </div>
                        <div>
                            <span>3. </span>
                            <button onClick={requestUSBDevice}>Pair USBDevice </button>
                            <div className="Popup-empty"> {USBDevice? `connected: ${USBDevice.productName}`: ""} </div>
                        </div>
                    </div>
                </li>
            </ul>
                <ProgressBar
                    soFar = {flashProgress}
                    total = {flashTotal}
                />
        </div>
    )
}

export default Combined