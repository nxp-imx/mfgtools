import React, { useState } from 'react';
import Combined from "./components/Combined";
import Decompress2 from "./components/Decompress";
import usePopup from "./logic/usePopup";

import './App.css'

const App = () => {
    const [bootFile, setBootFile] = useState();
    const [flashFile, setFlashFile] = useState();
    const [{ showPopup, closePopup, show, error }] = usePopup({bootFile, flashFile});

    return (
    <div className="App">
        <div className="u-flex u-column">
            <div>
                <Decompress2 />
            </div>
            <div className="u-row">
                <span className="link-container">bootloader [optional]: </span>
                <span className="file-name">{bootFile? bootFile.name:""}</span>
                <label className="custom-file-upload">
                    <input className="input" type="file" onChange={(e)=>setBootFile(e.target.files[0])}/>
                    Choose File
                </label>
            </div>

            <div className="u-row">
                <span className="link-container">flash: </span>
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
