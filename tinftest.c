#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define STREAM_BUFFER_BITS 9

#include "common.h"

struct datagroup
{
	const uint8_t * data;
	int place;
	int len;

	uint8_t * dataOut;
	int placeout;
	int lenout;
};

int produceprint( void * v, unsigned char c )
{
	putchar( c );
	return 0;
}

int producedata( void * v, unsigned char c )
{
	struct datagroup * dg = (struct datagroup*)v;
	if( dg->placeout >= dg->lenout ) return -5;
	dg->dataOut[dg->placeout++] = c;
	return 0;
}

int feeddata( void * v )
{
	struct datagroup * dg = (struct datagroup*)v;
	if( dg->place >= dg->len ) return -1;
	return dg->data[dg->place++];
}

struct filegroup
{
	FILE * fRead;
	FILE * fWrite;
};

int producefile( void * v, unsigned char c )
{
	struct filegroup * dg = (struct filegroup*)v;
	putc( c, dg->fWrite );
	return 0;
}

int feedfile( void * v )
{
	struct filegroup * dg = (struct filegroup*)v;
	uint8_t ret = 0;
	if( fread( &ret, 1, 1, dg->fRead ) == 1 )
		return ret;
	else
		return -1;
}


int main()
{
	uint8_t * srcdata = (uint8_t*)"Hello world, how are you doing today today?";
	uLongf srcLen = strlen( srcdata );
	uint8_t comped[2048] = { 0 };
	uLongf compedLen = sizeof( comped );

	int r = compress2window( comped, &compedLen, srcdata, srcLen, 9, STREAM_BUFFER_BITS );
	printf( "Comped: %d / %ld / %ld\n", r, compedLen, srcLen );

	int i;
	for( i = 0; i < compedLen; i++ )
	{
		printf( "%02x ", comped[i] );
	}
	printf( "\n" );

	struct datagroup dg;
	dg.data = comped; dg.len = compedLen; // Throw away gzip header + CRC
	dg.place = 0;

	r = tinf_stream_uncompress( feeddata, produceprint, &dg );

	printf( "\nR tinf_stream_uncompress: %d\n", r );
	if( r ) return r;


	FILE * fTest = fopen( "/usr/bin/gcc", "rb" );
	if( !fTest )
	{
		fprintf( stderr, "Error couldn't find test file (/usr/bin/gcc)\n" );
		return -5;
	}
	fseek( fTest, 0, SEEK_END );
	long fLen = ftell( fTest );
	fseek( fTest, 0, SEEK_SET );
	uint8_t * uncompressed_input = malloc( fLen );
	uint8_t * compressed_test = malloc( fLen );
	uint8_t * uncompressed_test = malloc( fLen );
	if( fread( uncompressed_input, 1, fLen, fTest ) != fLen )
	{
		fprintf( stderr, "Error reading file\n" );
		return -6;
	}
	fclose( fTest );

	srcLen = fLen;
	compedLen = fLen;
	r = compress2window( compressed_test, &compedLen, uncompressed_input, srcLen, 9, STREAM_BUFFER_BITS );
	printf( "Comped: %d / %ld / %ld (%.2f %%)\n", r, compedLen, srcLen, 100.*(float)compedLen/(float)srcLen );
	if( r ) return r;


	dg.data = compressed_test; dg.len = compedLen; // Throw away gzip header + CRC
	dg.place = 0;
	dg.dataOut = uncompressed_test; dg.lenout = fLen;
	dg.placeout = 0;

	r = tinf_stream_uncompress( feeddata, producedata, &dg );
	printf( "R tinf_stream_uncompress: %d\n", r );
	printf( "Output size: %d\n", dg.placeout );
	if( r ) return r;
	if( dg.placeout != fLen || fLen < 100 )
	{
		fprintf( stderr, "Decompressed wrong size\n" );
		return -44;
	}
	if( memcmp( uncompressed_input, uncompressed_test, fLen ) != 0 )
	{
		fprintf( stderr, "Error: Check failed\n" );
		return -55;
	}
	printf( "Check passed\n" );

	printf( "Context Decode Size (Bytes): %ld\n", sizeof( struct tinf_data ) );

/*
	struct filegroup fg;
	fg.fRead = fopen( "gcc.gz", "rb" );
	fg.fWrite = fopen( "gcc.check", "wb" );
	uint8_t header_waste[10];
	fread( header_waste, 1, 10, fg.fRead );
	r = tinf_stream_uncompress( feedfile, producefile, &fg );
	printf( "\nR: %d\n", r );
*/
	return r;
}

