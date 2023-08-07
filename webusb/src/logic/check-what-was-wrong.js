// no awaits
    // function process_chunk2 (chunk, i) {
    //     console.log("process chunk", chunk);

    //     let hex_len = (chunk.length + 52).toString(16); // 52 comes from headers
    //     USBDevice.transferOut(1, str_to_arr(`download:${hex_len}`)).then (result => {
    //         if (`DATA${hex_len}` !== ab_to_str(result.data.buffer)) {
    //             throw new Error ("failed to send data:", ab_to_str(result.data.buffer))
    //         }
    //     }).then(() => {
    //         let sparse = build_sparse_header(raw_data_bytelength, i);
    //         let dont_care = build_chunk_header(CHUNK_TYPE_DONT_CARE, raw_data_bytelength, i);
    //         let raw = build_chunk_header(CHUNK_TYPE_RAW, raw_data_bytelength, i);

    //         let headers = new Uint8Array(52);
    //         headers.set(sparse, 0);
    //         headers.set(dont_care, 28);
    //         headers.set(raw, 40);

    //         USBDevice.transferOut(1, headers);
    //     }).then (()=> {
    //         let offset=0;
    //         while (offset < chunk.length) {
    //             // let packet_len = Math.min(PACKET_SZ, DATA_SZ - offset);
    //             let packet_len = PACKET_SZ;
    //             USBDevice.transferOut(1, chunk.slice(offset, offset + packet_len));
    //             console.log("transferout", chunk.slice(offset, offset + packet_len));
    //             offset += packet_len;
    //             console.log("offset in transfer", offset)
    //         }

    //         // let result = await USBDevice.transferIn(1, 1048);

    //         USBDevice.transferIn(1, 1048).then(result => {
    //             console.log(result);
    //             if ("OKAY" !== ab_to_str(result.data.buffer)) {
    //                 throw new Error (`failed to send data on i: ${i}`, ab_to_str(result.data.buffer))
    //             }
    //         })
    //     }) .then (()=> {
    //         USBDevice.transferOut(1, str_to_arr("flash:all"))
    //         .then(result => {
    //             if ("OKAY" !== ab_to_str(result.data.buffer)) {
    //                 throw new Error ("failed to send data:", ab_to_str(result.data.buffer))
    //             }
    //         })
    //     })


        // let hex_len = (chunk.length + 52).toString(16); // 52 comes from headers
        // send_data(str_to_arr(`download:${hex_len}`), `DATA${hex_len}`);
        // send_headers(chunk.length, i);
        // console.log("sent headers")

        // let offset=0;
        // while (offset < chunk.length) {
        //     // let packet_len = Math.min(PACKET_SZ, DATA_SZ - offset);
        //     let packet_len = PACKET_SZ;
        //     USBDevice.transferOut(1, chunk.slice(offset, offset + packet_len));
        //     console.log("transferout", chunk.slice(offset, offset + packet_len));
        //     offset += packet_len;
        //     console.log("offset in transfer", offset)
        // }

        // // let result = await USBDevice.transferIn(1, 1048);

        // // USBDevice.transferIn(1, 1048).then(result => {
        // //     console.log(result);
        // //     if ("OKAY" !== ab_to_str(result.data.buffer)) {
        // //         throw new Error (`failed to send data on i: ${i}`, ab_to_str(result.data.buffer))
        // //     }
        // // })

        // USBDevice.transferIn(1, 1048);
        // flash_all();
        // console.log("FLASH SUCCESS")
    // }

    
