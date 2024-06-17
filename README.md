# rtgz and tinf_sf.h

tinfl (deflate decompressor) geared for very embedded environments, with single-file-header with streamable data support + tool (`rtgz`) for working with raw deflate/inflate blobs, like gzip/zlib, but without a file header.  Useful for working with compressed inflate/deflate blobs.

Usage:

```sh
echo "woot woot mc doot" | ./rtgz -c -w 9 -l 9 -v | ./rtgz -d
Compression: 18 / 16 (88.89%) (w_bits = 9)
woot woot mc doot
```

And for `tinf_sf.h`, there is `demo.c` that demonstrates the streaming API.

This also targets a reasonably small target, and it is setup to allow streaming without a large buffer.

```sh
$ make && ./demo
gcc -o demo demo.c -lz -g -O2
hello world
R: 0
Context size: 1784
```

```c
#include <stdio.h>
#include <assert.h>

#define STREAM_BUFFER_BITS 9
#define TINF_ADLER32 0
#define TINF_CRC32 0
#define TINF_ZLIB 0
#define TINF_GZIP 0
#define TINF_STREAM 1
#define TINF_BUFFER 0
#define TINF_ASSERT(x)
#define TINF_STREAM_BUFFER_SIZE (1<<(STREAM_BUFFER_BITS))
#define TINFLATE_IMPLEMENTATION

#include "tinf_sf.h"

struct dataingroup
{
	const uint8_t * data;
	int place, len;
};

int produceprint( void * v, unsigned char c )
{
	putchar( c );
	return 0;
}

int feeddata( void * v )
{
	struct dataingroup * dg = (struct dataingroup*)v;
	if( dg->place >= dg->len ) return -1;
	return dg->data[dg->place++];
}

int main()
{
	// Raw data, not with gzip or zlib headers.
	uint8_t compressed[] = {
		0xcb, 0x48, 0xcd, 0xc9, 0xc9, 0x57, 0x28, 0xcf,
		0x2f, 0xca, 0x49, 0xe1, 0x02, 0x00 };
	
	struct dataingroup dg;

	dg.data = compressed; dg.len = sizeof(compressed);
	dg.place = 0;

	int r = tinf_stream_uncompress( feeddata, produceprint, &dg );
	printf( "R: %d\n", r );
	printf( "Context size: %ld\n", sizeof( struct tinf_data ) );
	return r;
}
```
