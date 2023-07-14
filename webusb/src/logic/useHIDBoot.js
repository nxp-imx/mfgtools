/*
* Copyright 2023 NXP.
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* Redistributions of source code must retain the above copyright notice, this
* list of conditions and the following disclaimer.
*
* Redistributions in binary form must reproduce the above copyright notice, this
* list of conditions and the following disclaimer in the documentation and/or
* other materials provided with the distribution.
*
* Neither the name of the NXP Semiconductor nor the names of its
* contributors may be used to endorse or promote products derived from this
* software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*/

import {useEffect, useState} from 'react';

const HIDrequestParams = {
    filters: [{ vendorId: 8137, productId: 303 }]
};
const outputReportId = 0x02;

const useHIDBoot = (bootFile) => {
    const [HIDdevice, setHIDdevice] = useState();
    const [bootProgress, setBootProgress] = useState(0);
    const [bootTotal, setBootTotal] = useState(0);

    useEffect(()=> {
        navigator.hid.addEventListener("connect", handleConnectedDevice);
        navigator.hid.addEventListener("disconnect", handleDisconnectedDevice);
    }, [])

    useEffect(() => {
        const downloadBoot = async() => {
            if (!HIDdevice) return;
            await HIDdevice.open();
            console.log("Opened device: " + HIDdevice.productName);

            HIDdevice.addEventListener("inputreport", handleInputReport);
            doUBoot();
        }
        downloadBoot();
    }, [HIDdevice])

    const handleConnectedDevice = (e) => {
        console.log("Device connected: " + e.device.productName);
    }

    const handleDisconnectedDevice = (e) => {
        console.log("Device disconnected: " + e.device.productName);
    }

    const handleInputReport = (e) => {
        console.log(e.device.productName + ": got input report " + e.reportId);
        console.log(new Uint8Array(e.data.buffer));
    }

    const requestHIDDevice = async() => {
        const devices = await navigator.hid.requestDevice(HIDrequestParams);

        if (devices.length === 0) return;
        setHIDdevice(devices[0]);
    }

    const doUBoot = async() => {
        console.log("trying to boot...");

        if (!bootFile) {console.log("choose a file"); return 0;}
        const imageByteLength = bootFile.size;
        console.log(bootFile) // 1313792
        setBootTotal(imageByteLength);
        setBootProgress(0);

        console.log(await bootFile.arrayBuffer())

        for (let i=0; 1024*i<imageByteLength; i++) {
            let len;
            if (1024*i+len> imageByteLength)
                len = imageByteLength-1024*i;
            else
                len = 1024;
            const packet = await bootFile.slice(1024*i, 1024*i+len).arrayBuffer();
            await HIDdevice.sendReport(outputReportId, packet)
            setBootProgress((1024*i + len));
            console.log(1024*i + len)
        }
    }

    return [{
        requestHIDDevice: requestHIDDevice,
        HIDdevice: HIDdevice,
        bootProgress,
        bootTotal,
    }]
}

export default useHIDBoot;
