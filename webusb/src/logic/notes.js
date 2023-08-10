<<<<<<< HEAD
let buffer = new Uint8Array();

let decompress_sz = 1024*64;

const callback = (decompressed) => {
    let temp = new Uint8Array(buffer.length + decompressed.length);
    temp.set(buffer, 0);
    temp.set(decompressed, buffer.length);
    buffer = temp;
}

const decompress = async(data) => {

    if (!stream.begin()) {
        console.log("stream.begin() error");
        return null;
    }

    let offset = 0;
    while (offset < data.length) {
        let slice = data.slice(offset, offset + decompress_sz);
        await stream.transform(slice, callback);

        // do something with buffer after one round of transform
        doSomething(buffer);
        buffer = new Uint8Array();

        offset += decompress_sz;
    }

    if (!stream.end(callback)) {
        console.log("stream.end() error");
        return null;
    }
}

/**
hi ricky, sorry to bother you again, but I was wondering what you would do if you ended up with a buffer that is too big.

what we are trying to do right now is save a buffer after one round of stream.transform, then process the buffer.

the reason is because each stream.transform calls callback multiple times. then, we can't do any reading/writing to the usb within callback, because
the stream.transform function currently doesn't wait on each callback to finish before executing the next callback (we must send output and read input
in a certain order). So what we are trying right now is saving all the decompressed data from one call of stream.transform, then so all the reading/writing we need from that decompressed data.

the problem right now is that the last transform results in an
arraybuffer that is too large (700 MB), because the last part that is decompressed is a lot of zeroes, which are compressed compactly. 

I'm wondering if you have suggestions on how to deal with this?
 */


function sleep(ms) {
    let p1 = new Promise(resolve => setTimeout(resolve, ms));
    let p2 = p1.then(() => {
        console.log('after first setTimeout')
        return (
            new Promise(resolve => setTimeout(resolve, ms))
        )
    });
    return p2.then(()=> new Promise.resolve("hello")); // returns a promise
  }
  
   
  
  // Usage:
  console.log("Start");
  
  (async () => {
    for (let i=0; i<3; i++) {
        let res = await sleep(2000); // Sleep for 2 seconds
        console.log(res);
    }
    console.log("End");
  })();

=======
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
>>>>>>> 8ab6186ba1f75467c69e0196e4293e4c0bde5a9c
