import React, { useState } from 'react';
import Combined from "./components/Combined";
import usePopup from "./logic/usePopup";

import './App.css'

const App = () => {
    const [bootFile, setBootFile] = useState();
    const [flashFile, setFlashFile] = useState();
    const [{ showPopup, closePopup, show, error }] = usePopup({bootFile, flashFile});

    return (
    <div className="App">
        <div className="u-flex u-column">
    	    <div> <b> Demo for webuuu, Only Support 8QXP/8QM board, NO SPL version </b>  </div>
	    <div> This is early development version. Any issue or idea: report to <a href="https://github.com/nxp-imx/mfgtools/issues"> https://github.com/nxp-imx/mfgtools/issues </a> </div>
            <div> <p> </p> </div>
	    <div> <b> Before use this tools, please add below udev rule at linux machine </b> </div>
	    <div> KERNEL=="hidraw*", ATTRS{'{idVendor}'}=="1fc9", MODE="0666"</div>
	    <div> SUBSYSTEM=="usb", ATTRS{'{idVendor}'}=="1fc9", ATTRS{'{idProduct}'}=="0152", MODE="0664", TAG+="uaccess" </div>
	    <div> <p> </p> </div>
            <div className="u-row">
                <span className="link-container">bootloader: </span>
                <span className="file-name">{bootFile? bootFile.name:""}</span>
                <label className="custom-file-upload">
                    <input className="input" type="file" onChange={(e)=>setBootFile(e.target.files[0])}/>
                    Choose File
                </label>
            </div>

            <div className="u-row">
                <span className="link-container">wic image: </span>
                <span className="file-name">{flashFile? flashFile.name:""}</span>
                <label className="custom-file-upload">
                    <input className="input" type="file" onChange={(e)=>setFlashFile(e.target.files[0])}/>
                    Choose File
                </label>
            </div>

            <div className="popup-button">
                <button onClick = {showPopup}>USB_down</button>
            </div>

            <div className="popup-loc ">
                <div className="popup-container" style={show?{}:{display:'none'}}>
                    <Combined bootFile={bootFile} flashFile={flashFile}/>
                    <button onClick={()=>closePopup()}>close</button>
                </div>
            </div>
            <span>{error}</span>
        </div>

    </div>
    );
}

export default App;
