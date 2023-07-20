window.ZstdCodec = require('zstd-codec').ZstdCodec;
require("zstd-codec/lib/module.js").run((binding) => {console.log("running", binding); window.binding = binding} )
