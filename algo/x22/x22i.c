#include "algo/blake/sph_blake.h"
#include "algo/bmw/sph_bmw.h"
#if defined(__AES__)
  #include "algo/echo/aes_ni/hash_api.h"
  #include "algo/groestl/aes_ni/hash-groestl.h"
#else
  #include "algo/groestl/sph_groestl.h"
  #include "algo/echo/sph_echo.h"
#endif
#include "algo/skein/sph_skein.h"
#include "algo/jh/sph_jh.h"
#include "algo/keccak/sph_keccak.h"
#include "algo/luffa/luffa_for_sse2.h"
#include "algo/cubehash/cubehash_sse2.h"
#include "algo/shavite/sph_shavite.h"
#include "algo/simd/nist.h"
#include "algo/hamsi/sph_hamsi.h"
#include "algo/fugue/sph_fugue.h"
#include "algo/shabal/sph_shabal.h"
#include "algo/whirlpool/sph_whirlpool.h"
#include <openssl/sha.h>
#include "algo/haval/sph-haval.h"
#include "algo/tiger/sph_tiger.h"
#include "algo/lyra2/lyra2.h"
#include "algo/gost/sph_gost.h"
#include "algo/swifftx/swifftx.h"
#include "x22i-gate.h"

union _x22i_context_overlay
{
        sph_blake512_context blake;
        sph_bmw512_context bmw;
#if defined(__AES__)
        hashState_groestl       groestl;
        hashState_echo          echo;
#else
        sph_groestl512_context  groestl;
        sph_echo512_context     echo;
#endif
        sph_jh512_context       jh;
        sph_keccak512_context   keccak;
        sph_skein512_context    skein;
        hashState_luffa         luffa;
        cubehashParam           cube;
        sph_shavite512_context  shavite;
        hashState_sd            simd;
        sph_hamsi512_context    hamsi;
        sph_fugue512_context    fugue;
        sph_shabal512_context   shabal;
        sph_whirlpool_context   whirlpool;
        SHA512_CTX              sha512;
        sph_haval256_5_context  haval;
        sph_tiger_context       tiger;
        sph_gost512_context     gost;
        SHA256_CTX              sha256;
};
typedef union _x22i_context_overlay x22i_context_overlay;

void x22i_hash( void *output, const void *input )
{
   unsigned char hash[64 * 4] __attribute__((aligned(64))) = {0};
   unsigned char hash2[65]    __attribute__((aligned(64))) = {0};
   x22i_context_overlay ctx;

	sph_blake512_init(&ctx.blake);
	sph_blake512(&ctx.blake, input, 80);
	sph_blake512_close(&ctx.blake, hash);

	sph_bmw512_init(&ctx.bmw);
	sph_bmw512(&ctx.bmw, (const void*) hash, 64);
	sph_bmw512_close(&ctx.bmw, hash);

#if defined(__AES__)
   init_groestl( &ctx.groestl, 64 );
   update_and_final_groestl( &ctx.groestl, (char*)hash,
                                  (const char*)hash, 512 );
#else
   sph_groestl512_init( &ctx.groestl );
   sph_groestl512( &ctx.groestl, hash, 64 );
   sph_groestl512_close( &ctx.groestl, hash );
#endif
   
	sph_skein512_init(&ctx.skein);
	sph_skein512(&ctx.skein, (const void*) hash, 64);
	sph_skein512_close(&ctx.skein, hash);

	sph_jh512_init(&ctx.jh);
	sph_jh512(&ctx.jh, (const void*) hash, 64);
	sph_jh512_close(&ctx.jh, hash);

	sph_keccak512_init(&ctx.keccak);
	sph_keccak512(&ctx.keccak, (const void*) hash, 64);
	sph_keccak512_close(&ctx.keccak, hash);

   init_luffa( &ctx.luffa, 512 );
   update_and_final_luffa( &ctx.luffa, (BitSequence*)hash,
                                (const BitSequence*)hash, 64 );

   cubehashInit( &ctx.cube, 512, 16, 32 );
   cubehashUpdateDigest( &ctx.cube, (byte*) hash,
                              (const byte*)hash, 64 );

	sph_shavite512_init(&ctx.shavite);
	sph_shavite512(&ctx.shavite, (const void*) hash, 64);
	sph_shavite512_close(&ctx.shavite, hash);

   init_sd( &ctx.simd, 512 );
   update_final_sd( &ctx.simd, (BitSequence*)hash,
                         (const BitSequence*)hash, 512 );

#if defined(__AES__)
   init_echo( &ctx.echo, 512 );
   update_final_echo ( &ctx.echo, (BitSequence*)hash,
                            (const BitSequence*)hash, 512 );
#else
   sph_echo512_init( &ctx.echo );
   sph_echo512( &ctx.echo, hash, 64 );
   sph_echo512_close( &ctx.echo, hash );
#endif

	sph_hamsi512_init(&ctx.hamsi);
	sph_hamsi512(&ctx.hamsi, (const void*) hash, 64);
	sph_hamsi512_close(&ctx.hamsi, hash);

	sph_fugue512_init(&ctx.fugue);
	sph_fugue512(&ctx.fugue, (const void*) hash, 64);
	sph_fugue512_close(&ctx.fugue, hash);

	sph_shabal512_init(&ctx.shabal);
	sph_shabal512(&ctx.shabal, (const void*) hash, 64);
	sph_shabal512_close(&ctx.shabal, &hash[64]);

	sph_whirlpool_init(&ctx.whirlpool);
	sph_whirlpool (&ctx.whirlpool, (const void*) &hash[64], 64);
	sph_whirlpool_close(&ctx.whirlpool, &hash[128]);

   SHA512_Init( &ctx.sha512 );
   SHA512_Update(  &ctx.sha512, (const void*) &hash[128], 64);
   SHA512_Final( (void*) &hash[192], &ctx.sha512 );

	ComputeSingleSWIFFTX((unsigned char*)hash, (unsigned char*)hash2);

	memset(hash, 0, 64);
	sph_haval256_5_init(&ctx.haval);
	sph_haval256_5(&ctx.haval,(const void*) hash2, 64);
	sph_haval256_5_close(&ctx.haval,hash);

	memset(hash2, 0, 64);
	sph_tiger_init(&ctx.tiger);
	sph_tiger (&ctx.tiger, (const void*) hash, 64);
	sph_tiger_close(&ctx.tiger, (void*) hash2);

	memset(hash, 0, 64);
	LYRA2RE((void*) hash, 32, (const void*) hash2, 32, (const void*) hash2, 32, 1, 4, 4);

	sph_gost512_init(&ctx.gost);
	sph_gost512 (&ctx.gost, (const void*) hash, 64);
	sph_gost512_close(&ctx.gost, (void*) hash);

   SHA256_Init( &ctx.sha256 );
   SHA256_Update(  &ctx.sha256, (const void*) hash, 64 );
   SHA256_Final( (unsigned char*) hash, &ctx.sha256 );

	memcpy(output, hash, 32);
}

int scanhash_x22i( struct work* work, uint32_t max_nonce,
                   uint64_t *hashes_done, struct thr_info *mythr )
{
   uint32_t endiandata[20] __attribute__((aligned(64)));
   uint32_t hash[8] __attribute__((aligned(64)));
	uint32_t *pdata = work->data;
	uint32_t *ptarget = work->target;
	const uint32_t first_nonce = pdata[19];
   const uint32_t Htarg = ptarget[7];
   uint32_t n = first_nonce;
   const int thr_id = mythr->id;

	if (opt_benchmark)
		((uint32_t*)ptarget)[7] = 0x08ff;

	for (int k=0; k < 20; k++)
		be32enc(&endiandata[k], pdata[k]);

   InitializeSWIFFTX();

   do
   {
       pdata[19] = ++n;
       be32enc( &endiandata[19], n );

       x22i_hash( hash, endiandata );

       if ( hash[7] < Htarg )
       if ( fulltest( hash, ptarget ) && !opt_benchmark )
           submit_solution( work, hash, mythr );
    } while ( n < max_nonce && !work_restart[thr_id].restart );

	 *hashes_done = pdata[19] - first_nonce;
	 return 0;
}

