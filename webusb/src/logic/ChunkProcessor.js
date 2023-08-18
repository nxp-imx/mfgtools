class ChunkProcessor 
{
    #chunk_size
    #buffer
    #on_chunk

    /**
     * This callback is displayed as part of the Requester class.
     * @callback dataCallback
     * @param {buffer} Uint8Array
     */

    /**
     * @param {number} chunk_size 
     * @param {dataCallback} on_chunk
     */
    constructor(chunk_size, on_chunk) 
    {
        this.#buffer = new Uint8Array();
        this.#chunk_size = chunk_size;
        this.#on_chunk = on_chunk;
    }

    /**
     * @param {Uint8Array} new_buffer 
     */
    #append_buffer(new_buffer) {
        const temp = new Uint8Array(this.#buffer.length + new_buffer.length);
        temp.set(this.#buffer, 0);
        temp.set(new_buffer, this.#buffer.length);
        this.#buffer = temp;
    }

    /**
     * Adds new data to the buffer, and consumes chunks
     * @param {Uint8Array} new_buffer 
     */
    async push_data(new_buffer) {
        if(this.#buffer.length + new_buffer.length < this.#chunk_size) {
            this.#append_buffer(new_buffer);
            return;
        }

        // process old data
        const chunkPadCount = this.#chunk_size - this.#buffer.length;
        this.#append_buffer(new_buffer.slice(0, chunkPadCount));
        await this.#on_chunk(this.#buffer); // await is still necessary, even though js says it's not

        // remove the data already processed
        new_buffer = new_buffer.slice(chunkPadCount);
        while(true) {
            const curr_slice = new_buffer.slice(0, this.#chunk_size); // dont care about OOB, js handles it for us
            if(curr_slice.length === this.#chunk_size) {
                await this.#on_chunk(curr_slice); // await is still necessary, even though js says it's not
                new_buffer = new_buffer.slice(this.#chunk_size);
            } else {
                this.#buffer = curr_slice;
                break;
            }
        }
    }

    /**
     * flushes anything currently being buffered
     * @param {dataCallback=} callback 
     */
    flush(callback) {
        if(!callback) {
            callback = this.#on_chunk;
        }

        callback(this.#buffer);
        this.#buffer = new Uint8Array();
    }

    /**
     * processes an entire array, including remaining data
     * @param {Uint8Array} new_buffer 
     * @param {dataCallback=} callback 
     */
    process_entire_arr(new_buffer, callback) {
        this.push_data(new_buffer);
        this.flush(callback);
    }
}

export default ChunkProcessor