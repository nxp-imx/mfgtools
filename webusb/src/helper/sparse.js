/*
* Copyright (C) 2010 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

import {obj_to_arr} from './functions';

export const SPARSE_HEADER_MAGIC = 0xed26ff3a
export const CHUNK_TYPE_RAW = 0xCAC1
export const CHUNK_TYPE_FILL = 0xCAC2
export const CHUNK_TYPE_DONT_CARE = 0xCAC3
export const CHUNK_TYPE_CRC32 = 0xCAC4

export const BLK_SZ = 0x1000; // bytes per block
export const CHUNK_SZ = 0x1000; // blocks per full chunk

const CHUNK_HEADER_SZ = 12;
const SPARSE_HEADER_SZ = 28;

export const sparse_header =  { // in bytes
    magic : 4,           /* 0xed26ff3a */
    major_version : 2,       /* (0x1) - reject images with higher major versions */
    minor_version : 2,       /* (0x0) - allow images with higher minor versions */
    file_hdr_sz : 2,         /* 28 bytes for first revision of the file format */
    chunk_hdr_sz : 2,        /* 12 bytes for first revision of the file format */
    blk_sz : 4,              /* block size in bytes, must be a multiple of 4 (4096) */
    total_blks : 4,          /* total blocks in the non-sparse output image */ // CHANGE
    total_chunks : 4,        /* total chunks in the sparse input image */ // CHANGE
    image_checksum : 4,      /* CRC32 checksum of the original data, counting "don't care" */ // CHANGE
}

export const chunk_header = { // in bytes
    chunk_type : 2,          /* 0xCAC1 -> raw; 0xCAC2 -> fill; 0xCAC3 -> don't care */
    reserved1 : 2,
    chunk_sz : 4,           /* in blocks in output image */
    total_sz : 4,            /* in bytes of chunk input file including chunk header and data */
}

export const PACKET_SZ = 0x10000;
export const DATA_SZ = CHUNK_SZ * BLK_SZ; // in bytes

export function build_sparse_header(raw_data_bytelength, i) {
    let sparse_format = {
        magic : SPARSE_HEADER_MAGIC,           /* 0xed26ff3a */
        major_version : 0x1,       /* (0x1) - reject images with higher major versions */
        minor_version : 0x0,       /* (0x0) - allow images with higher minor versions */
        file_hdr_sz : 0x1c,         /* 28 bytes for first revision of the file format */
        chunk_hdr_sz : 0xc,        /* 12 bytes for first revision of the file format */
        blk_sz : 0x1000,              /* block size in bytes, must be a multiple of 4 (4096) */
        total_blks : 0x0,          /* total blocks in the non-sparse output image */ // CHANGE
        total_chunks : 0x2,        /* total chunks in the sparse input image */ //don't care and raw_data
        image_checksum : 0x0,      /* CRC32 checksum of the original data, counting "don't care" */
    }

    //first chunk is don't care: we don't care about the first 4096*i blocks
    //second chunk is raw_data: there are 4096 blocks in a 16mb chunk
    sparse_format.total_blks = CHUNK_SZ * i + raw_data_bytelength/BLK_SZ;

    return obj_to_arr(sparse_format, sparse_header, SPARSE_HEADER_SZ)
}

export function build_chunk_header (chunk_type, raw_data_bytelength, i) {
    let  chunk_format = {
        chunk_type : chunk_type,          /* 0xCAC1 -> raw; 0xCAC2 -> fill; 0xCAC3 -> don't care */
        reserved1 : 1,
        chunk_sz : 0,           /* in blocks in output image */
        total_sz : 0,            /* in bytes of chunk input file including chunk header and data */
    }

    if (chunk_type === CHUNK_TYPE_DONT_CARE) {
        chunk_format.chunk_sz = CHUNK_SZ * i; // don't care
        chunk_format.total_sz = CHUNK_SZ*BLK_SZ * i + CHUNK_HEADER_SZ; // bytes raw_data + header
    }
    else if (chunk_type === CHUNK_TYPE_RAW) { // raw_data
        chunk_format.chunk_sz = Math.ceil(raw_data_bytelength/BLK_SZ);
        chunk_format.total_sz = raw_data_bytelength + CHUNK_HEADER_SZ;

        console.log(chunk_format)
    }

    return obj_to_arr(chunk_format, chunk_header, CHUNK_HEADER_SZ);
}