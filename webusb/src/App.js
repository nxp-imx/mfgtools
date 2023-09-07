import React, { useState } from 'react';
import Combined from "./components/Combined";
import usePopup from "./logic/usePopup";
import UartTerminal from './terminal';
import 'xterm/css/xterm.css';
import './App.css'

const App = () => {
	const [bootFile, setBootFile] = useState();
	const [flashFile, setFlashFile] = useState();
	const [term, setTerm] = useState();
	const [port, setPort] = useState();
	const [{ showPopup, closePopup, show, error }] = usePopup({ bootFile, flashFile });
	const pipetoterm = async (port) => {
		await port.open({ baudRate: 115200 });
		const reader = port.readable.getReader();
		const readfun = (port, reader) => {
			reader.read().then(({ value, done }) => {
				if (!done) {
					console.log(value);
					term.write(value);
					return readfun(port, reader);
				} else {
					console.log("End of stream");
				}
			})
		};
		readfun(port, reader);
	}

	return (
		<div className="App">
			<div className="split-layout">
				<div className="left-column">
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
							<span className="file-name">{bootFile ? bootFile.name : ""}</span>
							<label className="custom-file-upload">
								<input className="input" type="file" onChange={(e) => setBootFile(e.target.files[0])} />
								Choose File
							</label>
						</div>

						<div className="u-row">
							<span className="link-container">wic image: </span>
							<span className="file-name">{flashFile ? flashFile.name : ""}</span>
							<label className="custom-file-upload">
								<input className="input" type="file" onChange={(e) => setFlashFile(e.target.files[0])} />
								Choose File
							</label>
						</div>

						<div className="popup-button">
							<button onClick={showPopup}>USB_down</button>
						</div>

						<div className="popup-loc ">
							<div className="popup-container" style={show ? {} : { display: 'none' }}>
								<Combined bootFile={bootFile} flashFile={flashFile} />
								<button onClick={() => closePopup()}>close</button>
							</div>
						</div>
						<span>{error}</span>
					</div>
				</div>
				<div className="right-column">
					<div>
						<button id="uart" onClick={() => navigator.serial.requestPort().then((e) => {
							document.getElementById('uart').textContent = "paired";
							term.write('<Connected>');
							window.uartport = e;
							pipetoterm(e);
						})
						} > Choose Uart Port </button>
					</div>
					<div>
						<UartTerminal onTerminalReady={(term) => {
							setTerm(term);
							term.onData((data) => {
								console.log(data);
								const encoder = new TextEncoder();
								const writer = window.uartport.writable.getWriter();
								writer.write(encoder.encode(data));
								writer.releaseLock();
							});
						}} />
					</div>
				</div>
			</div>
		</div>
	);
}

export default App;
