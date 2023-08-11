import { useState, useRef } from 'react';
import ProgressBar from './ProgressBar';
import useHIDBoot from '../logic/useHIDBoot';
import useDecompress2 from '../logic/useDecompress2';
import imgSrc from '../images/imx8qxp_mek_bootmode.png';
import "./NewApp.css";
import "./Combined.css";

const NewPopup = () => {
    const [bootFile, setBootFile] = useState();
    const [flashFile, setFlashFile] = useState();
    const [{ requestHIDDevice, HIDdevice, bootProgress, bootTotal }] = useHIDBoot(bootFile);
    const [{ requestUSBDevice, USBDevice, flashProgress, flashTotal}] = useDecompress2(flashFile);

    return (
        <div className="popup-container">
            <ul className="Popup-list">
                <li>
                    <div className="u-flex">
                        <div className="checkbox-container">
                        </div>
                        <div>
                            <div className="h4 instruction-title ">
                                <span>1. Switch Boot Mode to "SERIAL"</span>
                            </div>
                            <div className="u-flex u-center">
                                <div className="image-container">
                                {imgSrc? <img className="boot-image" src={imgSrc} alt="boot mode switches"/>: ""}
                                </div>
                            </div>
                        </div>
                    </div>
                </li>
                <li>
                    <div className="u-flex">
                        <div className="checkbox-container">
                            {bootProgress&&(bootProgress===bootTotal)?<p className="checkbox">&#x2714;</p>:""}
                        </div>
                        <div className="u-column instruction-container">
                            <div className="h4 instruction-title">
                                <span>2. [optional] Upload bootloader</span>
                            </div>
                            <div className="instruction-elt">
                                <label className="file-input-container">
                                    choose file
                                    <input className="file-input" type="file" onChange={(e) => setBootFile(e.target.files[0])}/>
                                </label>
                                <span>{bootFile?(bootFile.name.length>10 ? `${bootFile.name.slice(0,10)}...`:bootFile.name):""}</span>
                            </div>
                            <div className="instruction-elt">
                                <button className="button-secondary" onClick={requestHIDDevice}> Pair HIDdevice </button>
                            </div>
                            <div>
                                {/* <span>{bootProgress? `${bootProgress} bytes out of ${bootTotal} downloaded`: ""}</span> */}
                                <div className="Popup-empty">
                                    {bootProgress&&(bootProgress===bootTotal)?
                                    `connected: ${HIDdevice.productName}`
                                    : (bootProgress? `${bootProgress} bytes out of ${bootTotal} downloaded`: "")}
                                </div>
                            </div>
                        </div>
                    </div>
                </li>
                <li >
                    <div className="u-flex ">
                        <div className="checkbox-container ">
                            {USBDevice?<p className="checkbox">&#x2714;</p>:""}
                        </div>
                        <div className="u-column instruction-container">
                            <div className="h4 instruction-title ">
                                <span>3. Upload image file</span>
                            </div>
                            <div className="instruction-elt">
                                <label className="file-input-container">
                                    choose file
                                    <input className="file-input" type="file" onChange={(e) => {setFlashFile(e.target.files[0])}}/>
                                </label>
                                <span>{flashFile?(flashFile.name.length>10 ? `${flashFile.name.slice(0,10)}...`:flashFile.name):""}</span>
                            </div>
                            <div className="instruction-elt">
                                <button className="button-secondary" onClick={requestUSBDevice}>Pair USBDevice </button>
                            </div>
                            <div className="Popup-empty">
                                {flashProgress&&(flashProgress===flashTotal)?`connected: ${USBDevice.productName}`: (flashProgress?`Pairing`:"")}
                            </div>
                            <ProgressBar
                                soFar = {flashProgress}
                                total = {flashTotal}
                            />
                        </div>
                    </div>
                    
                </li>
            </ul>
            
        </div>
    )
}

const NewApp = () => {
    const [show, setShow] = useState(false);

    return(
        <div className="new-app">
            <div className="u-flex u-column list-files-container">
                <div className="u-flex file-option-container">
                    <div className="u-flex u-align-center file-name-container">
                        <div className="h3">
                                imx-image-core-imx8qxpc0mek-20230725120337.rootfs.wic
                        </div>
                    </div>
                    <div className="u-flex u-space-between file-action-container">
                        <button class="button">Download</button>
                        <button class="button-secondary" onClick={()=>{setShow(!show)}}>USB Download</button>
                    </div>
                </div>
            </div>
            <div style={show?{}:{display:'none'}}>
                <NewPopup />
            </div>           
        </div>
    )
}

export default NewApp