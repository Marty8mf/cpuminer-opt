//#if 0
#if defined(__VAES__) && defined(__AVX512F__) && defined(__AVX512VL__) && defined(__AVX512DQ__) && defined(__AVX512BW__)

#include "simd-utils.h"
#include "echo-hash-4way.h"

/*
static const unsigned int mul2ipt[] __attribute__ ((aligned (64))) =
{  
   0x728efc00, 0x6894e61a, 0x3fc3b14d, 0x25d9ab57,
   0xfd5ba600, 0x2a8c71d7, 0x1eb845e3, 0xc96f9234
};
*/
// do these need to be reversed?

#define mul2mask \
   _mm512_set4_epi32( 0, 0, 0, 0x00001b00 ) 
//   _mm512_set4_epi32( 0x00001b00, 0, 0, 0 )  

#define lsbmask    m512_const1_32( 0x01010101 ) 

#define ECHO_SUBBYTES( state, i, j ) \
	state[i][j] = _mm512_aesenc_epi128( state[i][j], k1 ); \
	state[i][j] = _mm512_aesenc_epi128( state[i][j], m512_zero ); \
	k1 = _mm512_add_epi32( k1, m512_one_128 );

#define ECHO_MIXBYTES( state1, state2, j, t1, t2, s2 ) do \
{ \
   const int j1 = ( (j)+1 ) & 3; \
   const int j2 = ( (j)+2 ) & 3; \
   const int j3 = ( (j)+3 ) & 3; \
   s2 = _mm512_add_epi8( state1[ 0 ] [j ], state1[ 0 ][ j ] ); \
	t1 = _mm512_srli_epi16( state1[ 0 ][ j ], 7 ); \
	t1 = _mm512_and_si512( t1, lsbmask );\
	t2 = _mm512_shuffle_epi8( mul2mask, t1 ); \
	s2 = _mm512_xor_si512( s2, t2 ); \
	state2[ 0 ] [j ] = s2; \
	state2[ 1 ] [j ] = state1[ 0 ][ j ]; \
	state2[ 2 ] [j ] = state1[ 0 ][ j ]; \
	state2[ 3 ] [j ] = _mm512_xor_si512( s2, state1[ 0 ][ j ] );\
	s2 = _mm512_add_epi8( state1[ 1 ][ j1 ], state1[ 1 ][ j1 ] ); \
	t1 = _mm512_srli_epi16( state1[ 1 ][ j1 ], 7 ); \
	t1 = _mm512_and_si512( t1, lsbmask ); \
	t2 = _mm512_shuffle_epi8( mul2mask, t1 ); \
	s2 = _mm512_xor_si512( s2, t2 );\
	state2[ 0 ][ j ] = _mm512_xor_si512( state2[ 0 ][ j ], \
                            _mm512_xor_si512( s2, state1[ 1 ][ j1 ] ) ); \
	state2[ 1 ][ j ] = _mm512_xor_si512( state2[ 1 ][ j ], s2 ); \
	state2[ 2 ][ j ] = _mm512_xor_si512( state2[ 2 ][ j ], state1[ 1 ][ j1 ] ); \
	state2[ 3 ][ j ] = _mm512_xor_si512( state2[ 3 ][ j ], state1[ 1 ][ j1 ] ); \
	s2 = _mm512_add_epi8( state1[ 2 ][ j2 ], state1[ 2 ][ j2 ] ); \
	t1 = _mm512_srli_epi16( state1[ 2 ][ j2 ], 7 ); \
	t1 = _mm512_and_si512( t1, lsbmask ); \
	t2 = _mm512_shuffle_epi8( mul2mask, t1 ); \
	s2 = _mm512_xor_si512( s2, t2 ); \
	state2[ 0 ][ j ] = _mm512_xor_si512( state2[ 0 ][ j ], state1[ 2 ][ j2 ] ); \
	state2[ 1 ][ j ] = _mm512_xor_si512( state2[ 1 ][ j ], \
                            _mm512_xor_si512( s2, state1[ 2 ][ j2 ] ) ); \
	state2[ 2 ][ j ] = _mm512_xor_si512( state2[ 2 ][ j ], s2 ); \
	state2[ 3 ][ j ] = _mm512_xor_si512( state2[ 3][ j ], state1[ 2 ][ j2 ] ); \
	s2 = _mm512_add_epi8( state1[ 3 ][ j3 ], state1[ 3 ][ j3 ] ); \
	t1 = _mm512_srli_epi16( state1[ 3 ][ j3 ], 7 ); \
	t1 = _mm512_and_si512( t1, lsbmask ); \
	t2 = _mm512_shuffle_epi8( mul2mask, t1 ); \
	s2 = _mm512_xor_si512( s2, t2 ); \
	state2[ 0 ][ j ] = _mm512_xor_si512( state2[ 0 ][ j ], state1[ 3 ][ j3 ] ); \
	state2[ 1 ][ j ] = _mm512_xor_si512( state2[ 1 ][ j ], state1[ 3 ][ j3 ] ); \
	state2[ 2 ][ j ] = _mm512_xor_si512( state2[ 2 ][ j ], \
                            _mm512_xor_si512( s2, state1[ 3 ][ j3] ) ); \
	state2[ 3 ][ j ] = _mm512_xor_si512( state2[ 3 ][ j ], s2 ); \
} while(0)

#define ECHO_ROUND_UNROLL2 \
	ECHO_SUBBYTES(_state, 0, 0);\
   ECHO_SUBBYTES(_state, 1, 0);\
	ECHO_SUBBYTES(_state, 2, 0);\
	ECHO_SUBBYTES(_state, 3, 0);\
	ECHO_SUBBYTES(_state, 0, 1);\
	ECHO_SUBBYTES(_state, 1, 1);\
	ECHO_SUBBYTES(_state, 2, 1);\
	ECHO_SUBBYTES(_state, 3, 1);\
	ECHO_SUBBYTES(_state, 0, 2);\
	ECHO_SUBBYTES(_state, 1, 2);\
	ECHO_SUBBYTES(_state, 2, 2);\
	ECHO_SUBBYTES(_state, 3, 2);\
	ECHO_SUBBYTES(_state, 0, 3);\
	ECHO_SUBBYTES(_state, 1, 3);\
	ECHO_SUBBYTES(_state, 2, 3);\
	ECHO_SUBBYTES(_state, 3, 3);\
	ECHO_MIXBYTES(_state, _state2, 0, t1, t2, s2);\
	ECHO_MIXBYTES(_state, _state2, 1, t1, t2, s2);\
	ECHO_MIXBYTES(_state, _state2, 2, t1, t2, s2);\
	ECHO_MIXBYTES(_state, _state2, 3, t1, t2, s2);\
	ECHO_SUBBYTES(_state2, 0, 0);\
	ECHO_SUBBYTES(_state2, 1, 0);\
	ECHO_SUBBYTES(_state2, 2, 0);\
	ECHO_SUBBYTES(_state2, 3, 0);\
	ECHO_SUBBYTES(_state2, 0, 1);\
	ECHO_SUBBYTES(_state2, 1, 1);\
	ECHO_SUBBYTES(_state2, 2, 1);\
	ECHO_SUBBYTES(_state2, 3, 1);\
	ECHO_SUBBYTES(_state2, 0, 2);\
	ECHO_SUBBYTES(_state2, 1, 2);\
	ECHO_SUBBYTES(_state2, 2, 2);\
	ECHO_SUBBYTES(_state2, 3, 2);\
	ECHO_SUBBYTES(_state2, 0, 3);\
	ECHO_SUBBYTES(_state2, 1, 3);\
	ECHO_SUBBYTES(_state2, 2, 3);\
	ECHO_SUBBYTES(_state2, 3, 3);\
	ECHO_MIXBYTES(_state2, _state, 0, t1, t2, s2);\
	ECHO_MIXBYTES(_state2, _state, 1, t1, t2, s2);\
	ECHO_MIXBYTES(_state2, _state, 2, t1, t2, s2);\
	ECHO_MIXBYTES(_state2, _state, 3, t1, t2, s2)

#define SAVESTATE(dst, src)\
	dst[0][0] = src[0][0];\
	dst[0][1] = src[0][1];\
	dst[0][2] = src[0][2];\
	dst[0][3] = src[0][3];\
	dst[1][0] = src[1][0];\
	dst[1][1] = src[1][1];\
	dst[1][2] = src[1][2];\
	dst[1][3] = src[1][3];\
	dst[2][0] = src[2][0];\
	dst[2][1] = src[2][1];\
	dst[2][2] = src[2][2];\
	dst[2][3] = src[2][3];\
	dst[3][0] = src[3][0];\
	dst[3][1] = src[3][1];\
	dst[3][2] = src[3][2];\
	dst[3][3] = src[3][3]

// blockcount always 1
void echo_4way_compress( echo_4way_context *ctx, const __m512i *pmsg,
               unsigned int uBlockCount )
{
  unsigned int r, b, i, j;
  __m512i t1, t2, s2, k1;
  __m512i _state[4][4], _state2[4][4], _statebackup[4][4]; 

  _state[ 0 ][ 0 ] = ctx->state[ 0 ][ 0 ];
  _state[ 0 ][ 1 ] = ctx->state[ 0 ][ 1 ];
  _state[ 0 ][ 2 ] = ctx->state[ 0 ][ 2 ];
  _state[ 0 ][ 3 ] = ctx->state[ 0 ][ 3 ];
  _state[ 1 ][ 0 ] = ctx->state[ 1 ][ 0 ];
  _state[ 1 ][ 1 ] = ctx->state[ 1 ][ 1 ];
  _state[ 1 ][ 2 ] = ctx->state[ 1 ][ 2 ];
  _state[ 1 ][ 3 ] = ctx->state[ 1 ][ 3 ];
  _state[ 2 ][ 0 ] = ctx->state[ 2 ][ 0 ];
  _state[ 2 ][ 1 ] = ctx->state[ 2 ][ 1 ];
  _state[ 2 ][ 2 ] = ctx->state[ 2 ][ 2 ];
  _state[ 2 ][ 3 ] = ctx->state[ 2 ][ 3 ];
  _state[ 3 ][ 0 ] = ctx->state[ 3 ][ 0 ];
  _state[ 3 ][ 1 ] = ctx->state[ 3 ][ 1 ];
  _state[ 3 ][ 2 ] = ctx->state[ 3 ][ 2 ];
  _state[ 3 ][ 3 ] = ctx->state[ 3 ][ 3 ];

  for ( b = 0; b < uBlockCount; b++ )
  {
    ctx->k = _mm512_add_epi64( ctx->k, ctx->const1536 );

    for( j = ctx->uHashSize / 256; j < 4; j++ )
    {
      for ( i = 0; i < 4; i++ )
	   {
        _state[ i ][ j ] = _mm512_load_si512( 
                     pmsg + 4 * (j - (ctx->uHashSize / 256)) + i );
	   }
	 }
    
    // save state
	 SAVESTATE( _statebackup, _state );

	 k1 = ctx->k;

	 for ( r = 0; r < ctx->uRounds / 2; r++ )
	 {
		ECHO_ROUND_UNROLL2;
	 }
		
	 if ( ctx->uHashSize == 256 )
	 {
	   for ( i = 0; i < 4; i++ )
	   {
		   _state[ i ][ 0 ] = _mm512_xor_si512( _state[ i ][ 0 ],
                                              _state[ i ][ 1 ] );
		   _state[ i ][ 0 ] = _mm512_xor_si512( _state[ i ][ 0 ],
                                              _state[ i ][ 2 ] );
		   _state[ i ][ 0 ] = _mm512_xor_si512( _state[ i ][ 0 ],
                                              _state[ i ][ 3 ] );
		   _state[ i ][ 0 ] = _mm512_xor_si512( _state[ i ][ 0 ],
                                              _statebackup[ i ][ 0 ] );
		   _state[ i ][ 0 ] = _mm512_xor_si512( _state[ i ][ 0 ],
                                              _statebackup[ i ][ 1 ] );
		   _state[ i ][ 0 ] = _mm512_xor_si512( _state[ i ][ 0 ],
                                              _statebackup[ i ][ 2 ] ) ;
		   _state[ i ][ 0 ] = _mm512_xor_si512( _state[ i ][ 0 ],
                                              _statebackup[ i ][ 3 ] );
	   }
	 }
	 else
	 {
	   for ( i = 0; i < 4; i++ )
	   {
		   _state[ i ][ 0 ] = _mm512_xor_si512( _state[ i ][ 0 ],
                                              _state[ i ][ 2 ] );
		   _state[ i ][ 1 ] = _mm512_xor_si512( _state[ i ][ 1 ],
                                              _state[ i ][ 3 ] );
		   _state[ i ][ 0 ] = _mm512_xor_si512( _state[ i ][ 0 ],
                                              _statebackup[ i ][ 0 ] );
		   _state[ i ][ 0 ] = _mm512_xor_si512( _state[ i ] [0 ],
                                              _statebackup[ i ][ 2 ] );
		   _state[ i ][ 1 ] = _mm512_xor_si512( _state[ i ][ 1 ],
                                              _statebackup[ i ][ 1 ] );
		   _state[ i ][ 1 ] = _mm512_xor_si512( _state[ i ][ 1 ],
                                              _statebackup[ i ][ 3 ] );
      }
	 }
    pmsg += ctx->uBlockLength;
  }
  SAVESTATE(ctx->state, _state);

}

int echo_4way_init( echo_4way_context *ctx, int nHashSize )
{
	int i, j;

   ctx->k = m512_zero; 
	ctx->processed_bits = 0;
	ctx->uBufferBytes = 0;

	switch( nHashSize )
	{
		case 256:
			ctx->uHashSize = 256;
			ctx->uBlockLength = 192;
			ctx->uRounds = 8;
			ctx->hashsize = _mm512_set4_epi32( 0, 0, 0, 0x100 );
			ctx->const1536 = _mm512_set4_epi32( 0, 0, 0, 0x600 );
			break;

		case 512:
			ctx->uHashSize = 512;
			ctx->uBlockLength = 128;
			ctx->uRounds = 10;
			ctx->hashsize = _mm512_set4_epi32( 0, 0, 0, 0x200 );
			ctx->const1536 = _mm512_set4_epi32( 0, 0, 0, 0x400);
			break;

		default:
			return 1;
	}

	for( i = 0; i < 4; i++ )
		for( j = 0; j < nHashSize / 256; j++ )
			ctx->state[ i ][ j ] = ctx->hashsize;

	for( i = 0; i < 4; i++ )
		for( j = nHashSize / 256; j < 4; j++ )
			ctx->state[ i ][ j ] = m512_zero;

	return 0;
}

int echo_4way_update_close( echo_4way_context *state, void *hashval,
                              const void *data, int databitlen )
{
// bytelen is either 32 (maybe), 64 or 80 or 128!
// all are less than full block.

   int vlen = databitlen / 128;  // * 4 lanes / 128 bits per lane
   const int vblen = state->uBlockLength / 16; //  16 bytes per lane
   __m512i remainingbits;

   if ( databitlen == 1024 )
   {
      echo_4way_compress( state, data, 1 );
      state->processed_bits = 1024;
      remainingbits = m512_const2_64( 0, -1024 );
      vlen = 0;
   }
   else
   {
      vlen = databitlen / 128;  // * 4 lanes / 128 bits per lane
      memcpy_512( state->buffer, data, vlen );
      state->processed_bits += (unsigned int)( databitlen );
      remainingbits = _mm512_set4_epi32( 0, 0, 0, databitlen );

   }

   state->buffer[ vlen ] = _mm512_set4_epi32( 0, 0, 0, 0x80 );
   memset_zero_512( state->buffer + vlen + 1, vblen - vlen - 2 );
   state->buffer[ vblen-2 ] =
                _mm512_set4_epi32( (uint32_t)state->uHashSize << 16, 0, 0, 0 );
   state->buffer[ vblen-1 ] =
                   _mm512_set4_epi64( 0, state->processed_bits,
                                      0, state->processed_bits );  

   state->k = _mm512_add_epi64( state->k, remainingbits );
   state->k = _mm512_sub_epi64( state->k, state->const1536 );

   echo_4way_compress( state, state->buffer, 1 );

   _mm512_store_si512( (__m512i*)hashval + 0, state->state[ 0 ][ 0] );
   _mm512_store_si512( (__m512i*)hashval + 1, state->state[ 1 ][ 0] );

   if ( state->uHashSize == 512 )
   {
      _mm512_store_si512( (__m512i*)hashval + 2, state->state[ 2 ][ 0 ] );
      _mm512_store_si512( (__m512i*)hashval + 3, state->state[ 3 ][ 0 ] );
   }
   return 0;
}

#endif
