// flash and other usb stuff
import {useEffect, useState} from 'react';
import {str_to_arr, ab_to_str} from '../helper/functions.js'
import {CHUNK_SZ, BLK_SZ, CHUNK_TYPE_RAW, CHUNK_TYPE_DONT_CARE, build_sparse_header, build_chunk_header} from '../helper/sparse.js'

const USBrequestParams = { filters: [{ vendorId: 8137, productId: 0x0152 }] };

const PACKET_SZ = 0x10000;
const DATA_SZ = CHUNK_SZ * BLK_SZ; // in bytes

// HELPER //
async function get_reply(USBDevice, success_str) {
    let result = await USBDevice.transferIn(1, 1048);

    if (success_str !== ab_to_str(result.data.buffer)) {
        throw new Error ("failed to send data:", ab_to_str(result.data.buffer))
    }
    console.log(result.data.buffer);
}

async function send_output(USBDevice, data) {
    await USBDevice.transferOut(1, data);
}

async function send_data(USBDevice, data, success_str) { // TODO: use functions above to simplify
    await USBDevice.transferOut(1, data);
    let result = await USBDevice.transferIn(1, 1048);

    if (success_str !== ab_to_str(result.data.buffer)) {
        throw new Error ("failed to send data:", ab_to_str(result.data.buffer))
    }
}

async function send_headers(USBDevice, raw_data_bytelength, i) {
    let sparse = build_sparse_header(raw_data_bytelength, i);
    let dont_care = build_chunk_header(CHUNK_TYPE_DONT_CARE, raw_data_bytelength, i);
    let raw = build_chunk_header(CHUNK_TYPE_RAW, raw_data_bytelength, i);

    let headers = new Uint8Array(52);
    headers.set(sparse, 0);
    headers.set(dont_care, 28);
    headers.set(raw, 40);

    await USBDevice.transferOut(1, headers);
}

///////////


export async function preboot (USBDevice) {
    await USBDevice.claimInterface(0);

    await send_data(USBDevice, str_to_arr("UCmd:setenv fastboot_dev mmc"), "OKAY");
    await send_data(USBDevice, str_to_arr("UCmd:setenv mmcdev ${emmc_dev}"), "OKAY");
    await send_data(USBDevice, str_to_arr("UCmd:mmc dev ${emmc_dev}"), "OKAY");

    console.log("preboot complete")
}

async function send_chunk_begin (USBDevice, chunk_len, i) {
    // rounds chunk len up to nearest block:
    let round_chunk_len = Math.ceil(chunk_len/BLK_SZ)*BLK_SZ; //TODO: redundant
    let hex_len = (round_chunk_len + 52).toString(16);

    await send_data(USBDevice, str_to_arr(`download:${hex_len}`), `DATA${hex_len}`);
    await send_headers(USBDevice, round_chunk_len, i);
}

async function flash_all (USBDevice) {
    await send_data(USBDevice, str_to_arr("flash:all"), "OKAY");
    console.log("flash all");
}

export async function process_chunk (USBDevice, chunk, i) {
    console.log("process_chunk");
    console.log(USBDevice, chunk, i);
    // pad chunk with zeros

    let pad_count = Math.ceil(chunk.length/BLK_SZ)*BLK_SZ - chunk.length

    if (pad_count) {
        let pad = new Uint8Array(pad_count);
        let new_chunk = new Uint8Array(chunk.length + pad_count);
        new_chunk.set(chunk, 0);
        new_chunk.set(pad, chunk.length);
        chunk = new_chunk;
        console.log("new chunk", chunk)
    }

    await send_chunk_begin(USBDevice, chunk.length, i);
    console.log("sent chunk begin")
    
    let offset = 0;
    let packet;
    while (offset < chunk.length) {
        packet = chunk.slice(offset, offset + PACKET_SZ);
        await send_output(USBDevice, packet);
        console.log("sent output")
        offset += PACKET_SZ;
    }
    console.log("finished loop")
    await get_reply(USBDevice, "OKAY");
    await flash_all(USBDevice);
}