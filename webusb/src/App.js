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
        <NewApp />
    </div>
    );
}

export default App;
