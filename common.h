#ifndef _TINFL_STREAM_COMMON_H
#define _TINFL_STREAM_COMMON_H

#include <zlib.h>
#include <assert.h>

#ifndef STREAM_BUFFER_BITS
#define STREAM_BUFFER_BITS 9
#endif

#define TINF_ADLER32 0
#define TINF_CRC32 0
#define TINF_ZLIB 0
#define TINF_GZIP 0
#define TINF_STREAM 1
#define TINF_BUFFER 1
#define TINF_ASSERT assert
#define TINF_STREAM_BUFFER_SIZE (1<<(STREAM_BUFFER_BITS))
#define TINFLATE_IMPLEMENTATION

#include "tinf_sf.h"


int compress2window(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen, int level, int w_bits);

int compress2window(Bytef *dest, uLongf *destLen, const Bytef *source,
                      uLong sourceLen, int level, int w_bits) {
    z_stream stream;
    int err;
    const uInt max = (uInt)-1;
    uLong left;

    left = *destLen;
    *destLen = 0;

    stream.zalloc = (alloc_func)0;
    stream.zfree = (free_func)0;
    stream.opaque = (voidpf)0;

    err =
		 deflateInit2(&stream, level, Z_DEFLATED, -w_bits, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
		//deflateInit(&stream, level);
    if (err != Z_OK) return err;

    stream.next_out = dest;
    stream.avail_out = 0;
    stream.next_in = (z_const Bytef *)source;
    stream.avail_in = 0;

    do {
        if (stream.avail_out == 0) {
            stream.avail_out = left > (uLong)max ? max : (uInt)left;
            left -= stream.avail_out;
        }
        if (stream.avail_in == 0) {
            stream.avail_in = sourceLen > (uLong)max ? max : (uInt)sourceLen;
            sourceLen -= stream.avail_in;
        }
        err = deflate(&stream, sourceLen ? Z_NO_FLUSH : Z_FINISH);
    } while (err == Z_OK);

    *destLen = stream.total_out;
    deflateEnd(&stream);
    return err == Z_STREAM_END ? Z_OK : err;
}
#endif

