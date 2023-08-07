/*

GOAL: 

process a chunk's worth of data and send it through USB, one chunk after another :
chunk1
chunk2
chunk3...

when a chunk's worth of data has been decompressed, 
do 'process_chunk' on it
before decompressing more


PROBLEM: 

making 'callback' an async function, like
```
const callback = async(decompressed) => { 
    ...
    await process_chunk(curr_chunk);
}

```
still batches all the 'process_chunk' events until the end

*/


const process_chunk = async(chunk, i) => {
    // call functions on chunk that do asynchronous stuff to chunk
    // 'transferData' is an async function
    await transferData(chunk)
}

const decompressFileToTransform = async(file) => {
    let data = new Uint8Array(await file.arrayBuffer());


    /* CALLBACK FUNCTION USED BY ZSTD-CODEC */

    let curr_chunk = new Uint8Array(DATA_SZ);
    let curr_chunk_offset = 0;

    const callback = (decompressed) => { // 'decompressed' is an Uint8Array, usually 131072 bytes
        while(curr_chunk_offset + decompressed.length >= DATA_SZ) { // 'DATA_SZ' is num of bytes in a chunk
            curr_chunk.set(decompressed.slice(0, DATA_SZ - curr_chunk_offset), curr_chunk_offset);
            
            process_chunk(curr_chunk);

            curr_chunk_offset = 0;
            decompressed = decompressed.slice(DATA_SZ - curr_chunk_offset, decompressed.length);
        }

        curr_chunk.set(decompressed, curr_chunk_offset);
        curr_chunk_offset += decompressed.length;
    }


    /* 
        ZSTD-CODEC: https://github.com/yoshihitoh/zstd-codec

        sends decompressed data to 'callback' function
    */
   
    const stream = new binding.ZstdDecompressStreamBinding();

    if (!stream.begin()) {
        console.log("stream.begin() error");
        return null;
    }

    console.log("start unzipping");

    let i = 0;
    const size = 1024*1024*2; // 2MB of data at a time
    while (size*i < data.length) {
        let end = Math.min(size*(i+1), data.length);
        let slice = data.slice(size*i, end);

        if (!stream.transform(slice, callback)) {
            console.log(`stream.transform() error on slice ${size*i} to ${end}`);
            return null;
        }

        i++;
    }

    if (!stream.end(callback)) {
        console.log("stream.end() error");
        return null;
    }

    console.log("finishing unzipping");
}