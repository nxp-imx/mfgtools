import React, { useState, useEffect } from 'react';
import Combined from "./components/Combined";
import Decompress from "./components/Decompress";
import usePopup from "./logic/usePopup";

import NewApp from "./components/NewApp";

import './App.css'

const App = () => {
    const [bootFile, setBootFile] = useState();
    const [flashFile, setFlashFile] = useState();
    const [{ showPopup, closePopup, show, error }] = usePopup({bootFile, flashFile});

    return (
    <div className="App">
        <div className="u-flex u-column">
            <div>
                <Decompress />
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
            
            {/* for testing purpose, just let you input files without waiting */}
            {/* <div className="popup-loc ">
                <div className="popup-container" style={show?{}:{display:'none'}}>
                    <Combined bootFile={bootFile} flashFile={flashFile}/>
                    <button onClick={()=>closePopup()}>close</button>
                </div>
            </div> */}

            <Combined bootFile={bootFile} flashFile={flashFile}/>

            <span>{error}</span>
        </div>
        <NewApp />
    </div>
    );
}

export default App;
