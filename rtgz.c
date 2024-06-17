/*
 * rtgz.c - raw tiny deflate/inflate.
 *
 * Copyright (c) 2024 <>< Charles Lohr
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 *   1. The origin of this software must not be misrepresented; you must
 *      not claim that you wrote the original software. If you use this
2 *      software in a product, an acknowledgment in the product
 *      documentation would be appreciated but is not required.
 *
 *   2. Altered source versions must be plainly marked as such, and must
 *      not be misrepresented as being the original software.
 *
 *   3. This notice may not be removed or altered from any source
 *      distribution.
 */

// This file is based on zpipe.c which is a great example.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define STREAM_BUFFER_BITS 15

#include "common.h"
#include <zlib.h>

// From zpipe
#define CHUNK 16384

struct filegroup
{
	FILE * fRead;
	FILE * fWrite;
};

static int producefile( void * v, unsigned char c )
{
	struct filegroup * dg = (struct filegroup*)v;
	putc( c, dg->fWrite );
	return 0;
}

static int feedfile( void * v )
{
	struct filegroup * dg = (struct filegroup*)v;
	uint8_t ret = 0;
	if( fread( &ret, 1, 1, dg->fRead ) == 1 )
	{
		return ret;
	}
	else
		return -1;
}

int main( int argc, char ** argv )
{
	char * infile = 0;
	char * outfile = 0;
	int operation = 0;
	int windowsize = 9;
	opterr = 0;
	int compresslevel = 6;
	int verbose = 0;
	int c;
	while( ( c = getopt (argc, argv, "o:i:cdhw:l:v") ) != -1 )
	{
		switch( c )
		{
		case 'i':
			infile = optarg;
			break;
		case 'o':
			outfile = optarg;
			break;
		case 'w':
			windowsize = atoi( optarg );
			break;
		case 'l':
			compresslevel = atoi( optarg );
			break;
		case 'v':
			verbose = 1;
			break;
		case 'c':
		case 'd':
			if( operation > 0 )
			{
				fprintf( stderr, "Error: can't compress and decompress\n" );
				return -5;
			}
			operation = c - 'c' + 1;
			break;
		default:
			fprintf( stderr, "Error: Usage: rtgz [-o out file] [-i infile] -c/-d [-w windowsize bits (9-15)] [-l compress level] [-v]\n" );
			fprintf( stderr, "  compresses / decompreses raw deflate data (gzip/zlib) without a header and with limited window size\n" );
			return -5;
		}
	}

	if( operation == 0 )
	{
		fprintf( stderr, "Error: Need to have either -c or -d flag for compress/decompress\n" );
		return -5;
	}

	if( windowsize > 15 || windowsize < 9 )
	{
		fprintf( stderr, "Error: Invalid window size %d\n", windowsize );
		return -6;
	}

	struct filegroup fg;
	fg.fRead = infile ? fopen( infile, "rb" ) : stdin;
	fg.fWrite = outfile ? fopen( outfile, "wb" ) : stdout;
	unsigned char in[CHUNK];
	unsigned char out[CHUNK];

	if( !fg.fRead )
	{
		fprintf( stderr, "Error: can't open in file %s\n", infile );
		return -7;
	}

	if( !fg.fWrite )
	{
		fprintf( stderr, "Error: can't open out file %s\n", outfile );
		return -8;
	}

	int bytesin = 0;
	int bytesout = 0;

	if( operation == 1 )
	{
		int ret, flush;
		unsigned have;
		z_stream stream = { 0 };

		// Compress( deflate )
		ret = deflateInit2( &stream, compresslevel, Z_DEFLATED, -windowsize, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY );
		if (ret != Z_OK)
		{
			fprintf( stderr, "Error: deflateInit2() = %d\n", ret );
			return ret;
		}

		do
		{
			stream.avail_in = fread( in, 1, CHUNK, fg.fRead );
			if ( ferror( fg.fRead ) )
			{
				fprintf( stderr, "Error: read failure on in file\n" );
				(void)deflateEnd( &stream );
				return -13;
			}
			bytesin += stream.avail_in;
			flush = feof( fg.fRead ) ? Z_FINISH : Z_NO_FLUSH;
			stream.next_in = in;

			/* run deflate() on input until output buffer not full, finish
			compression if all of source has been read in */
			do {
				stream.avail_out = CHUNK;
				stream.next_out = out;
				ret = deflate(&stream, flush);    /* no bad return value */
				if( ret == Z_STREAM_ERROR )
				{
					fprintf( stderr, "Error: Error compressing block\n" );
					return -12;
				}

				have = CHUNK - stream.avail_out;
				bytesout += have;
				if ( fwrite( out, 1, have, fg.fWrite ) != have || ferror( fg.fWrite ) )
				{
					fprintf( stderr, "Error: Error writing compressed data\n" );
					return -12;
				}
			} while (stream.avail_out == 0);
			if( stream.avail_in != 0 )
			{
				fprintf( stderr, "Error: Stream ended prematurely\n" );
			}
		} while (flush != Z_FINISH);

		if( ret != Z_STREAM_END )
		{
			fprintf( stderr, "stream not ended\n" );
			return -6;
		}
		(void)deflateEnd(&stream);
		if( verbose )
		{
			fprintf( stderr, "Compression: %d / %d (%.2f%%) (w_bits = %d)\n", bytesin, bytesout, 100.0 * bytesout / bytesin, windowsize );
		}
	}
	else if( operation == 2 )
	{
		z_stream strm = { 0 };

		int ret = inflateInit2( &strm, -windowsize ); //const char *version, int stream_size);

		int have = 0;

		if (ret != Z_OK)
		{
			fprintf( stderr, "Error: inflateInit() fault %d\n", ret );
			return ret;
		}

		do {
			strm.avail_in = fread(in, 1, CHUNK, fg.fRead);
			if (ferror(fg.fRead)) {
				(void)inflateEnd(&strm);
				fprintf( stderr, "Error: reading from file %s\n", infile );
				return Z_ERRNO;
			}

			bytesin += strm.avail_in;
			if (strm.avail_in == 0)
				break;
			strm.next_in = in;

			/* run inflate() on input until output buffer not full */
			do {
				strm.avail_out = CHUNK;
				strm.next_out = out;
				ret = inflate(&strm, Z_NO_FLUSH);
				switch (ret) {
					case Z_NEED_DICT:
						ret = Z_DATA_ERROR;     /* and fall through */
					case Z_DATA_ERROR:
					case Z_MEM_ERROR:
						(void)inflateEnd(&strm);
						fprintf( stderr, "Error: zlib error: %d\n", ret );
						return ret;
					default:
						break;
				}
				have = CHUNK - strm.avail_out;
				bytesout += have;
				if (fwrite(out, 1, have, fg.fWrite) != have || ferror(fg.fWrite)) {
					(void)inflateEnd(&strm);
					fprintf( stderr, "Error: error writing to file %s\n", outfile );
					return Z_ERRNO;
				}
			} while (strm.avail_out == 0);

			/* done when inflate() says it's done */
		} while (ret != Z_STREAM_END);

		/* clean up and return */
		(void)inflateEnd(&strm);
		if( ret != Z_STREAM_END )
		{
			fprintf( stderr, "Error: inflateEnd error\n" );
			return -44;
		}
		if( verbose )
		{
			fprintf( stderr, "Decompression: %d -> %d (Was %.2f%%) (w_bits: %d)\n", bytesin, bytesout, 100.0 * bytesin / bytesout, windowsize );
		}
	}

	

	fclose( fg.fWrite );
	fclose( fg.fRead );
	return 0;
}


