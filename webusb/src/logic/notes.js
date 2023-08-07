async function callback() {

    await send_data();

    callback();
}

function caller () {
    stream.transform(chunk_offset, slice, callback);
}

///

function transform (chunk, callback){
    // fill up a src_bytes worth from chunk
    decompress(src_bytes);
}

function decompress(src_bytes) {
    // loop over src_bytes

    while (condition){
        let decompressed = zstd_decompress(src_bytes);
        callback(decompressed)
    }

    src_bytes.clear();
}