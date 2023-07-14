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

/* UTILITY */

/*
* hex: Hex number to convert
* size: Int number of bytes
* Returns: an UInt8Array; 2 digits in hex = 1 byte
*/
function hex_to_ab(hex, sz) {
    let buffer = new Array(sz).fill(0)

    for (let i=0; i<sz&&hex; i++){
        buffer[i] = hex & 0xff;
        hex = hex >> 8;
    }
    return buffer;
}

/*
* str: String
* Returns: a Uint8Array
*/
function str_to_arr(str) {
    const encoder = new TextEncoder();
    return encoder.encode(str);
}

/*
* ab: ArrayBuffer
* Returns a string decoding message in array buffer
*/
function ab_to_str(ab) {
    const decoder = new TextDecoder();
    return decoder.decode(ab);
}

/*
* obj: Object with key (property) value (hex value) pairs
* obj_sz: Object with key (property) value (size of prop, in bytes) pairs
* sz: size of complete array buffer, in bytes
* Returns Uint8Array with padded values
*/
function obj_to_arr(obj, obj_sz, sz) {
    let ab = new Uint8Array(sz);
    let offset = 0;
    for (const prop in obj) {
        let prop_arr = hex_to_ab(obj[prop], obj_sz[prop]);
        ab.set(prop_arr, offset);
        offset += obj_sz[prop];
    }
    return ab;
}

export {hex_to_ab, str_to_arr, ab_to_str, obj_to_arr};