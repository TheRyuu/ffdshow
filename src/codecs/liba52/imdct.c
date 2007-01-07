/*
 * imdct.c
 * Copyright (C) 2000-2002 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * The ifft algorithms in this file have been largely inspired by Dan
 * Bernstein's work, djbfft, available at http://cr.yp.to/djbfft.html
 *
 * This file is part of a52dec, a free ATSC A-52 stream decoder.
 * See http://liba52.sourceforge.net/ for updates.
 *
 * a52dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * a52dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * SSE optimizations from Michael Niedermayer (michaelni@gmx.at)
 * 3DNOW optimizations from Nick Kurshev <nickols_k@mail.ru>
 *   michael did port them from libac3 (untested, perhaps totally broken)
 * AltiVec optimizations from Romain Dolbeau (romain@dolbeau.org)
 */

#include "config.h"

#include <math.h>
#include <stdio.h>
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795029
#endif
#include <inttypes.h>

#include "a52.h"
#include "a52_internal.h"
#define FF_CPU_ONLY
#include "../../Tconfig.h"
#include "../../csimd.h"
#include "mangle.h"

using namespace csimd;

void (* a52_imdct_256) (sample_t *data, sample_t *delay, sample_t bias);
void (* a52_imdct_512) (sample_t *data, sample_t *delay, sample_t bias);

typedef struct complex_s {
    sample_t real;
    sample_t imag;
} complex_t;

static uint8_t fftorder[] = {
      0,128, 64,192, 32,160,224, 96, 16,144, 80,208,240,112, 48,176,
      8,136, 72,200, 40,168,232,104,248,120, 56,184, 24,152,216, 88,
      4,132, 68,196, 36,164,228,100, 20,148, 84,212,244,116, 52,180,
    252,124, 60,188, 28,156,220, 92, 12,140, 76,204,236,108, 44,172,
      2,130, 66,194, 34,162,226, 98, 18,146, 82,210,242,114, 50,178,
     10,138, 74,202, 42,170,234,106,250,122, 58,186, 26,154,218, 90,
    254,126, 62,190, 30,158,222, 94, 14,142, 78,206,238,110, 46,174,
      6,134, 70,198, 38,166,230,102,246,118, 54,182, 22,150,214, 86
};

/* Root values for IFFT */
static sample_t roots16[3];
static sample_t roots32[7];
static sample_t roots64[15];
static sample_t roots128[31];

#ifdef __GNUC__
  #define attribute_used __attribute__((used))
  #define attribute_aligned __attribute__((aligned(16)))
#else
  #define attribute_used 
  #define attribute_aligned __declspec(align(16))
#endif

/* 128 point bit-reverse LUT */
static const uint8_t attribute_aligned attribute_used bit_reverse_512[] = {
	0x00, 0x40, 0x20, 0x60, 0x10, 0x50, 0x30, 0x70, 
	0x08, 0x48, 0x28, 0x68, 0x18, 0x58, 0x38, 0x78, 
	0x04, 0x44, 0x24, 0x64, 0x14, 0x54, 0x34, 0x74, 
	0x0c, 0x4c, 0x2c, 0x6c, 0x1c, 0x5c, 0x3c, 0x7c, 
	0x02, 0x42, 0x22, 0x62, 0x12, 0x52, 0x32, 0x72, 
	0x0a, 0x4a, 0x2a, 0x6a, 0x1a, 0x5a, 0x3a, 0x7a, 
	0x06, 0x46, 0x26, 0x66, 0x16, 0x56, 0x36, 0x76, 
	0x0e, 0x4e, 0x2e, 0x6e, 0x1e, 0x5e, 0x3e, 0x7e, 
	0x01, 0x41, 0x21, 0x61, 0x11, 0x51, 0x31, 0x71, 
	0x09, 0x49, 0x29, 0x69, 0x19, 0x59, 0x39, 0x79, 
	0x05, 0x45, 0x25, 0x65, 0x15, 0x55, 0x35, 0x75, 
	0x0d, 0x4d, 0x2d, 0x6d, 0x1d, 0x5d, 0x3d, 0x7d, 
	0x03, 0x43, 0x23, 0x63, 0x13, 0x53, 0x33, 0x73, 
	0x0b, 0x4b, 0x2b, 0x6b, 0x1b, 0x5b, 0x3b, 0x7b, 
	0x07, 0x47, 0x27, 0x67, 0x17, 0x57, 0x37, 0x77, 
	0x0f, 0x4f, 0x2f, 0x6f, 0x1f, 0x5f, 0x3f, 0x7f};

// NOTE: SSE needs 16byte alignment or it will segfault 
// 
static float attribute_aligned sseSinCos1c[256];
static float attribute_aligned sseSinCos1d[256];
static const float attribute_used attribute_aligned ps111_1[4]={1,1,1,-1};
//static float attribute_aligned sseW0[4];
static float attribute_aligned sseW1[8];
static float attribute_aligned sseW2[16];
static float attribute_aligned sseW3[32];
static float attribute_aligned sseW4[64];
static float attribute_aligned sseW5[128];
static float attribute_aligned sseW6[256];
static float attribute_aligned *sseW[7]=
       {NULL /*sseW0*/,sseW1,sseW2,sseW3,sseW4,sseW5,sseW6};
static float attribute_aligned sseWindow[512];

/* Twiddle factor LUT */
static complex_t attribute_aligned w_1[1];
static complex_t attribute_aligned w_2[2];
static complex_t attribute_aligned w_4[4];
static complex_t attribute_aligned w_8[8];
static complex_t attribute_aligned w_16[16];
static complex_t attribute_aligned w_32[32];
static complex_t attribute_aligned w_64[64];
static complex_t attribute_aligned * w[7] = {w_1, w_2, w_4, w_8, w_16, w_32, w_64};

/* Twiddle factors for IMDCT */
static sample_t attribute_aligned xcos1[128];
static sample_t attribute_aligned xsin1[128];
static sample_t attribute_aligned xcos2[64];
static sample_t attribute_aligned xsin2[64];

/* Windowing function for Modified DCT - Thank you acroread */
static const sample_t imdct_window[] = {
	0.00014, 0.00024, 0.00037, 0.00051, 0.00067, 0.00086, 0.00107, 0.00130,
	0.00157, 0.00187, 0.00220, 0.00256, 0.00297, 0.00341, 0.00390, 0.00443,
	0.00501, 0.00564, 0.00632, 0.00706, 0.00785, 0.00871, 0.00962, 0.01061,
	0.01166, 0.01279, 0.01399, 0.01526, 0.01662, 0.01806, 0.01959, 0.02121,
	0.02292, 0.02472, 0.02662, 0.02863, 0.03073, 0.03294, 0.03527, 0.03770,
	0.04025, 0.04292, 0.04571, 0.04862, 0.05165, 0.05481, 0.05810, 0.06153,
	0.06508, 0.06878, 0.07261, 0.07658, 0.08069, 0.08495, 0.08935, 0.09389,
	0.09859, 0.10343, 0.10842, 0.11356, 0.11885, 0.12429, 0.12988, 0.13563,
	0.14152, 0.14757, 0.15376, 0.16011, 0.16661, 0.17325, 0.18005, 0.18699,
	0.19407, 0.20130, 0.20867, 0.21618, 0.22382, 0.23161, 0.23952, 0.24757,
	0.25574, 0.26404, 0.27246, 0.28100, 0.28965, 0.29841, 0.30729, 0.31626,
	0.32533, 0.33450, 0.34376, 0.35311, 0.36253, 0.37204, 0.38161, 0.39126,
	0.40096, 0.41072, 0.42054, 0.43040, 0.44030, 0.45023, 0.46020, 0.47019,
	0.48020, 0.49022, 0.50025, 0.51028, 0.52031, 0.53033, 0.54033, 0.55031,
	0.56026, 0.57019, 0.58007, 0.58991, 0.59970, 0.60944, 0.61912, 0.62873,
	0.63827, 0.64774, 0.65713, 0.66643, 0.67564, 0.68476, 0.69377, 0.70269,
	0.71150, 0.72019, 0.72877, 0.73723, 0.74557, 0.75378, 0.76186, 0.76981,
	0.77762, 0.78530, 0.79283, 0.80022, 0.80747, 0.81457, 0.82151, 0.82831,
	0.83496, 0.84145, 0.84779, 0.85398, 0.86001, 0.86588, 0.87160, 0.87716,
	0.88257, 0.88782, 0.89291, 0.89785, 0.90264, 0.90728, 0.91176, 0.91610,
	0.92028, 0.92432, 0.92822, 0.93197, 0.93558, 0.93906, 0.94240, 0.94560,
	0.94867, 0.95162, 0.95444, 0.95713, 0.95971, 0.96217, 0.96451, 0.96674,
	0.96887, 0.97089, 0.97281, 0.97463, 0.97635, 0.97799, 0.97953, 0.98099,
	0.98236, 0.98366, 0.98488, 0.98602, 0.98710, 0.98811, 0.98905, 0.98994,
	0.99076, 0.99153, 0.99225, 0.99291, 0.99353, 0.99411, 0.99464, 0.99513,
	0.99558, 0.99600, 0.99639, 0.99674, 0.99706, 0.99736, 0.99763, 0.99788,
	0.99811, 0.99831, 0.99850, 0.99867, 0.99882, 0.99895, 0.99908, 0.99919,
	0.99929, 0.99938, 0.99946, 0.99953, 0.99959, 0.99965, 0.99969, 0.99974,
	0.99978, 0.99981, 0.99984, 0.99986, 0.99988, 0.99990, 0.99992, 0.99993,
	0.99994, 0.99995, 0.99996, 0.99997, 0.99998, 0.99998, 0.99998, 0.99999,
	0.99999, 0.99999, 0.99999, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000,
	1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000 };

/* Twiddle factors for IMDCT */
static complex_t pre1[128];
static complex_t post1[64];
static complex_t pre2[64];
static complex_t post2[32];

static sample_t a52_imdct_window[256];

static void (* ifft128) (complex_t * buf);
static void (* ifft64) (complex_t * buf);

static inline void ifft2 (complex_t * buf)
{
    sample_t r, i;

    r = buf[0].real;
    i = buf[0].imag;
    buf[0].real += buf[1].real;
    buf[0].imag += buf[1].imag;
    buf[1].real = r - buf[1].real;
    buf[1].imag = i - buf[1].imag;
}

static inline void ifft4 (complex_t * buf)
{
    sample_t tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;

    tmp1 = buf[0].real + buf[1].real;
    tmp2 = buf[3].real + buf[2].real;
    tmp3 = buf[0].imag + buf[1].imag;
    tmp4 = buf[2].imag + buf[3].imag;
    tmp5 = buf[0].real - buf[1].real;
    tmp6 = buf[0].imag - buf[1].imag;
    tmp7 = buf[2].imag - buf[3].imag;
    tmp8 = buf[3].real - buf[2].real;

    buf[0].real = tmp1 + tmp2;
    buf[0].imag = tmp3 + tmp4;
    buf[2].real = tmp1 - tmp2;
    buf[2].imag = tmp3 - tmp4;
    buf[1].real = tmp5 + tmp7;
    buf[1].imag = tmp6 + tmp8;
    buf[3].real = tmp5 - tmp7;
    buf[3].imag = tmp6 - tmp8;
}

/* the basic split-radix ifft butterfly */

#define BUTTERFLY(a0,a1,a2,a3,wr,wi) do {	\
    tmp5 = a2.real * wr + a2.imag * wi;		\
    tmp6 = a2.imag * wr - a2.real * wi;		\
    tmp7 = a3.real * wr - a3.imag * wi;		\
    tmp8 = a3.imag * wr + a3.real * wi;		\
    tmp1 = tmp5 + tmp7;				\
    tmp2 = tmp6 + tmp8;				\
    tmp3 = tmp6 - tmp8;				\
    tmp4 = tmp7 - tmp5;				\
    a2.real = a0.real - tmp1;			\
    a2.imag = a0.imag - tmp2;			\
    a3.real = a1.real - tmp3;			\
    a3.imag = a1.imag - tmp4;			\
    a0.real += tmp1;				\
    a0.imag += tmp2;				\
    a1.real += tmp3;				\
    a1.imag += tmp4;				\
} while (0)

/* split-radix ifft butterfly, specialized for wr=1 wi=0 */

#define BUTTERFLY_ZERO(a0,a1,a2,a3) do {	\
    tmp1 = a2.real + a3.real;			\
    tmp2 = a2.imag + a3.imag;			\
    tmp3 = a2.imag - a3.imag;			\
    tmp4 = a3.real - a2.real;			\
    a2.real = a0.real - tmp1;			\
    a2.imag = a0.imag - tmp2;			\
    a3.real = a1.real - tmp3;			\
    a3.imag = a1.imag - tmp4;			\
    a0.real += tmp1;				\
    a0.imag += tmp2;				\
    a1.real += tmp3;				\
    a1.imag += tmp4;				\
} while (0)

/* split-radix ifft butterfly, specialized for wr=wi */

#define BUTTERFLY_HALF(a0,a1,a2,a3,w) do {	\
    tmp5 = (a2.real + a2.imag) * w;		\
    tmp6 = (a2.imag - a2.real) * w;		\
    tmp7 = (a3.real - a3.imag) * w;		\
    tmp8 = (a3.imag + a3.real) * w;		\
    tmp1 = tmp5 + tmp7;				\
    tmp2 = tmp6 + tmp8;				\
    tmp3 = tmp6 - tmp8;				\
    tmp4 = tmp7 - tmp5;				\
    a2.real = a0.real - tmp1;			\
    a2.imag = a0.imag - tmp2;			\
    a3.real = a1.real - tmp3;			\
    a3.imag = a1.imag - tmp4;			\
    a0.real += tmp1;				\
    a0.imag += tmp2;				\
    a1.real += tmp3;				\
    a1.imag += tmp4;				\
} while (0)

static inline void ifft8 (complex_t * buf)
{
    sample_t tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;

    ifft4 (buf);
    ifft2 (buf + 4);
    ifft2 (buf + 6);
    BUTTERFLY_ZERO (buf[0], buf[2], buf[4], buf[6]);
    BUTTERFLY_HALF (buf[1], buf[3], buf[5], buf[7], roots16[1]);
}

static void ifft_pass (complex_t * buf, sample_t * weight, int n)
{
    complex_t * buf1;
    complex_t * buf2;
    complex_t * buf3;
    sample_t tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;
    int i;

    buf++;
    buf1 = buf + n;
    buf2 = buf + 2 * n;
    buf3 = buf + 3 * n;

    BUTTERFLY_ZERO (buf[-1], buf1[-1], buf2[-1], buf3[-1]);

    i = n - 1;

    do {
	BUTTERFLY (buf[0], buf1[0], buf2[0], buf3[0], weight[n], weight[2*i]);
	buf++;
	buf1++;
	buf2++;
	buf3++;
	weight++;
    } while (--i);
}

static void ifft16 (complex_t * buf)
{
    ifft8 (buf);
    ifft4 (buf + 8);
    ifft4 (buf + 12);
    ifft_pass (buf, roots16 - 4, 4);
}

static void ifft32 (complex_t * buf)
{
    ifft16 (buf);
    ifft8 (buf + 16);
    ifft8 (buf + 24);
    ifft_pass (buf, roots32 - 8, 8);
}


static void ifft64_c (complex_t * buf)
{
    ifft32 (buf);
    ifft16 (buf + 32);
    ifft16 (buf + 48);
    ifft_pass (buf, roots64 - 16, 16);
}


static void ifft128_c (complex_t * buf)
{
    ifft32 (buf);
    ifft16 (buf + 32);
    ifft16 (buf + 48);
    ifft_pass (buf, roots64 - 16, 16);

    ifft32 (buf + 64);
    ifft32 (buf + 96);
    ifft_pass (buf, roots128 - 32, 32);
}

static void a52_imdct_512_C (sample_t * data, sample_t * delay, sample_t bias)
{
    int i, k;
    sample_t t_r, t_i, a_r, a_i, b_r, b_i, w_1, w_2;
    const sample_t * window = a52_imdct_window;
    complex_t buf[128];
	
    for (i = 0; i < 128; i++) {
	k = fftorder[i];
	t_r = pre1[i].real;
	t_i = pre1[i].imag;

	buf[i].real = t_i * data[255-k] + t_r * data[k];
	buf[i].imag = t_r * data[255-k] - t_i * data[k];
    }

    ifft128 (buf);

    /* Post IFFT complex multiply plus IFFT complex conjugate*/
    /* Window and convert to real valued signal */
    for (i = 0; i < 64; i++) {
	/* y[n] = z[n] * (xcos1[n] + j * xsin1[n]) ; */
	t_r = post1[i].real;
	t_i = post1[i].imag;

	a_r = t_r * buf[i].real     + t_i * buf[i].imag;
	a_i = t_i * buf[i].real     - t_r * buf[i].imag;
	b_r = t_i * buf[127-i].real + t_r * buf[127-i].imag;
	b_i = t_r * buf[127-i].real - t_i * buf[127-i].imag;

	w_1 = window[2*i];
	w_2 = window[255-2*i];
	data[2*i]     = delay[2*i] * w_2 - a_r * w_1 + bias;
	data[255-2*i] = delay[2*i] * w_1 + a_r * w_2 + bias;
	delay[2*i] = a_i;

	w_1 = window[2*i+1];
	w_2 = window[254-2*i];
	data[2*i+1]   = delay[2*i+1] * w_2 + b_r * w_1 + bias;
	data[254-2*i] = delay[2*i+1] * w_1 - b_r * w_2 + bias;
	delay[2*i+1] = b_i;
    }
}

void
imdct_do_512_sse(sample_t data[],sample_t delay[], sample_t bias)
{
    int m;
    int two_m;
    int two_m_plus_one;

    sample_t *data_ptr;
    sample_t *delay_ptr;
    
    complex_t attribute_aligned buf[128];
    
    /* 512 IMDCT with source and dest data in 'data' */
    /* see the c version (dct_do_512()), its allmost identical, just in C */ 

    /* Pre IFFT complex multiply plus IFFT cmplx conjugate */
    /* Bit reversed shuffling */
    {
		int esi=0;
		const uint8_t *eax=bit_reverse_512;
		__m128 xmm0,xmm1,xmm2;
		for (int edi=1008;edi>=0;esi+=16,eax+=2,edi-=16) {
		    movlps ((uint8_t*)data+ esi, xmm0);		 // XXXI
		    movhps (8+(uint8_t*)data+ edi, xmm0);		 // RXXI
		    movlps (8+(uint8_t*)data+ esi, xmm1);		 // XXXi
		    movhps ((uint8_t*)data+ edi, xmm1);		 // rXXi
		    xmm0=_mm_shuffle_ps(xmm0,xmm1,0x33); //shufps (0x33, xmm1, xmm0);		 // irIR
		    movaps ((uint8_t*)sseSinCos1c+esi, xmm2);
		    mulps (xmm0, xmm2);
		    xmm0=_mm_shuffle_ps(xmm0,xmm0,0xB1); //shufps $0xB1, xmm0, xmm0		 // riRI
		    mulps ((uint8_t*)sseSinCos1d+esi, xmm0);
		    subps (xmm0, xmm2);			
		    //movzbl (eax), edx			
		    //movzbl 1(eax), ebp			
		    movlps (xmm2, (uint8_t*)buf+ eax[0]*8);
		    movhps (xmm2, (uint8_t*)buf+ eax[1]*8);		
		}   
	}


    /* FFT Merge */
/* unoptimized variant
    for (m=1; m < 7; m++) {
	if(m)
	    two_m = (1 << m);
	else
	    two_m = 1;

	two_m_plus_one = (1 << (m+1));

	for(i = 0; i < 128; i += two_m_plus_one) {
	    for(k = 0; k < two_m; k++) {
		p = k + i;
		q = p + two_m;
		tmp_a_r = buf[p].real;
		tmp_a_i = buf[p].imag;
		tmp_b_r = buf[q].real * w[m][k].real - buf[q].imag * w[m][k].imag;
		tmp_b_i = buf[q].imag * w[m][k].real + buf[q].real * w[m][k].imag;
		buf[p].real = tmp_a_r + tmp_b_r;
		buf[p].imag =  tmp_a_i + tmp_b_i;
		buf[q].real = tmp_a_r - tmp_b_r;
		buf[q].imag =  tmp_a_i - tmp_b_i;
	    }
	}
    }
*/
    
    /* 1. iteration */
	// Note w[0][0]={1,0}
	{
	        __m128 xmm0,xmm1,xmm2;
		xorps (xmm1, xmm1);	
		xorps (xmm2, xmm2);	
		for (unsigned char *esi=(unsigned char*)buf;(complex_t*)esi<buf+128;esi+=16){
		    movlps (esi, xmm0);	 //buf[p]
		    movlps (8+esi, xmm1); //buf[q]
		    movhps (esi, xmm0);	 //buf[p]
		    movhps (8+esi, xmm2); //buf[q]
		    addps (xmm1, xmm0);	
		    subps (xmm2, xmm0);	
		    movaps (xmm0, esi);	
		}    
        }
    /* 2. iteration */
	// Note w[1]={{1,0}, {0,-1}}
	{
	        __m128 xmm7,xmm0,xmm1,xmm2;
		movaps (ps111_1, xmm7); // 1,1,1,-1
		for (unsigned char *esi=(unsigned char*)buf;(complex_t*)esi<buf+128;esi+=32){
		    movaps (16+esi, xmm2);	 //r2,i2,r3,i3
		    xmm2=_mm_shuffle_ps(xmm2,xmm2,0xB4);	 //r2,i2,i3,r3
		    mulps (xmm7, xmm2);		 //r2,i2,i3,-r3
		    movaps (esi, xmm0);		 //r0,i0,r1,i1
		    movaps (esi, xmm1);		 //r0,i0,r1,i1
		    addps (xmm2, xmm0);		
		    subps (xmm2, xmm1);		
		    movaps (xmm0, esi);		
		    movaps (xmm1, 16+esi);	
		}    
	};

    /* 3. iteration */
/*
 Note sseW2+0={1,1,sqrt(2),sqrt(2))
 Note sseW2+16={0,0,sqrt(2),-sqrt(2))
 Note sseW2+32={0,0,-sqrt(2),-sqrt(2))
 Note sseW2+48={1,-1,sqrt(2),-sqrt(2))
*/
	{
	        __m128 xmm6,xmm7,xmm5,xmm2,xmm3,xmm4,xmm0,xmm1;
		movaps (48+(uint8_t*)sseW2, xmm6); 
		movaps (16+(uint8_t*)sseW2, xmm7);
		xorps (xmm5, xmm5);		
		xorps (xmm2, xmm2);		
		for (unsigned char *esi=(unsigned char*)buf;(complex_t*)esi<buf+128;esi+=64){
		    movaps (32+esi, xmm2);	 //r4,i4,r5,i5
		    movaps (48+esi, xmm3);	 //r6,i6,r7,i7
		    movaps ((uint8_t*)sseW2, xmm4);	 //r4,i4,r5,i5
		    movaps (32+(uint8_t*)sseW2, xmm5); //r6,i6,r7,i7
		    mulps (xmm2, xmm4);		
		    mulps (xmm3, xmm5);		
		    xmm2=_mm_shuffle_ps(xmm2,xmm2,0xB1);	 //i4,r4,i5,r5
		    xmm3=_mm_shuffle_ps(xmm3,xmm3,0xB1);	 //i6,r6,i7,r7
		    mulps (xmm6, xmm3);		
		    mulps (xmm7, xmm2);		
		    movaps (esi, xmm0);		 //r0,i0,r1,i1
		    movaps (16+esi, xmm1);	 //r2,i2,r3,i3
		    addps (xmm4, xmm2);		
		    addps (xmm5, xmm3);		
		    movaps (xmm2, xmm4);		
		    movaps (xmm3, xmm5);		
		    addps (xmm0, xmm2);		
		    addps (xmm1, xmm3);		
		    subps (xmm4, xmm0);		
		    subps (xmm5, xmm1);		
		    movaps (xmm2, esi);		 
		    movaps (xmm3, 16+esi);	 
		    movaps (xmm0, 32+esi);	 
		    movaps (xmm1, 48+esi);	 
		}    
	}

    /* 4-7. iterations */
    for (m=3; m < 7; m++) {
	two_m = (1 << m);
	two_m_plus_one = two_m<<1;
	{
	        __m128 xmm1,xmm2,xmm0;
		for (unsigned char *esi=(unsigned char*)buf;(complex_t*)esi<buf+128;esi+=two_m_plus_one<<3){
		    int edi=0;			 // k
		    for (unsigned char *edx=esi+ (two_m<<3);edi< (two_m<<3);edi+=16){
		        movaps (edx+ edi, xmm1);		
		        movaps ((uint8_t*)sseW[m]+ edi* 2, xmm2);		
		        mulps (xmm1, xmm2);			
		        xmm1=_mm_shuffle_ps(xmm1,xmm1,0xB1);
		        mulps (16+(uint8_t*)sseW[m]+ edi* 2, xmm1);		
		        movaps (esi+ edi, xmm0);		
		        addps (xmm2, xmm1);			
		        movaps( xmm1, xmm2);			
		        addps (xmm0, xmm1);			
		        subps (xmm2, xmm0);			
		        movaps (xmm1, esi+ edi);		
		        movaps (xmm0, edx+ edi);		
		    }    
		}
	}
    }

    /* Post IFFT complex multiply  plus IFFT complex conjugate*/
	{
		__m128 xmm0,xmm1;
		for (int esi=-1024;esi!=0;esi+=16){
		    movaps ((uint8_t*)(buf+128)+ esi, xmm0);		
		    movaps ((uint8_t*)(buf+128)+ esi, xmm1);		
		    xmm0=_mm_shuffle_ps(xmm0,xmm0,0xB1);
		    mulps (1024+(uint8_t*)sseSinCos1c+esi, xmm1);
		    mulps (1024+(uint8_t*)sseSinCos1d+esi, xmm0);
		    addps (xmm1, xmm0);			
		    movaps (xmm0, (uint8_t*)(buf+128)+ esi);
		}
	}

	
    data_ptr = data;
    delay_ptr = delay;
    //window_ptr = imdct_window;

    /* Window and convert to real valued signal */
	{
		__m128 xmm0,xmm1,xmm2;
		movss (bias, xmm2);			  // bias
		xmm2=_mm_shuffle_ps(xmm2,xmm2,0x00);		  // bias, bias, ...
		for (int esi=0,edi=0;esi<512;esi+=16,edi-=16){
		    movlps ((uint8_t*)(buf+64)+ esi, xmm0);		 // ? ? A ?
		    movlps (8+(uint8_t*)(buf+64)+ esi, xmm1);		 // ? ? C ?
		    movhps (-16+(uint8_t*)(buf+64)+ edi, xmm1);		 // ? D C ?
		    movhps (-8+(uint8_t*)(buf+64)+ edi, xmm0);		 // ? B A ?
		    xmm0=_mm_shuffle_ps(xmm0,xmm1,0x99);// shufps $0x99, xmm1, xmm0		 // D C B A
		    mulps ((uint8_t*)sseWindow+esi, xmm0);
		    addps ((uint8_t*)delay_ptr+ esi, xmm0);		
		    addps (xmm2, xmm0);			
		    movaps (xmm0, (uint8_t*)data_ptr+ esi);
		}    
	}
	data_ptr+=128;
	delay_ptr+=128;
//	window_ptr+=128;
	
	{
		__m128 xmm0,xmm1,xmm2;
		movss (bias, xmm2);			  // bias
		xmm2=_mm_shuffle_ps(xmm2,xmm2,0x00);		  // bias, bias, ...
		for (int edi=1024,esi=0;esi<512;esi+=16,edi-=16){
		    movlps ((uint8_t*)buf+ esi, xmm0);		 // ? ? ? A
		    movlps (8+(uint8_t*)buf+ esi, xmm1);		 // ? ? ? C
		    movhps (-16+(uint8_t*)buf+ edi, xmm1);		 // D ? ? C
		    movhps (-8+(uint8_t*)buf+ edi, xmm0);		 // B ? ? A
		    xmm0=_mm_shuffle_ps(xmm0,xmm1,0xCC);		 // D C B A
		    mulps (512+(uint8_t*)sseWindow+esi, xmm0);
		    addps ((uint8_t*)delay_ptr+ esi, xmm0);		
		    addps (xmm2, xmm0);			
		    movaps (xmm0, (uint8_t*)data_ptr+ esi);		
		}    
	}
	data_ptr+=128;
//	window_ptr+=128;

    /* The trailing edge of the window goes into the delay line */
    delay_ptr = delay;

	{
		__m128 xmm0,xmm1;
		for (int edi=0,esi=0;esi<512;esi+=16,edi-=16){
		    movlps ((uint8_t*)(buf+64)+ esi, xmm0);		 // ? ? ? A
		    movlps (8+(uint8_t*)(buf+64)+ esi, xmm1);		 // ? ? ? C
		    movhps (-16+(uint8_t*)(buf+64)+ edi, xmm1);		 // D ? ? C 
		    movhps (-8+(uint8_t*)(buf+64)+ edi, xmm0);		 // B ? ? A 
		    xmm0=_mm_shuffle_ps(xmm0,xmm1,0xcc);//shufps $0xCC, xmm1, xmm0		 // D C B A
		    mulps (1024+(uint8_t*)sseWindow+esi, xmm0);
		    movaps (xmm0, (uint8_t*)delay_ptr+ esi);
		}    
	}
	delay_ptr+=128;
//	window_ptr-=128;
	
	{
		__m128 xmm0,xmm1;
		for (int edi=1024,esi=0;esi<512;esi+=16,edi-=16){
		    movlps ((uint8_t*)buf+ esi, xmm0);		 // ? ? A ?
		    movlps (8+(uint8_t*)buf+ esi, xmm1);		 // ? ? C ?
		    movhps (-16+(uint8_t*)buf+ edi, xmm1);		 // ? D C ? 
		    movhps (-8+(uint8_t*)buf+ edi, xmm0);		 // ? B A ? 
		    xmm0=_mm_shuffle_ps(xmm0,xmm1,0x99);// shufps $0x99, xmm1, xmm0		 // D C B A
		    mulps (1536+(uint8_t*)sseWindow+esi, xmm0);
		    movaps (xmm0, (uint8_t*)delay_ptr+ esi);
		}    
	}
}

static void a52_imdct_256_C(sample_t * data, sample_t * delay, sample_t bias)
{
    int i, k;
    sample_t t_r, t_i, a_r, a_i, b_r, b_i, c_r, c_i, d_r, d_i, w_1, w_2;
    const sample_t * window = a52_imdct_window;
    complex_t buf1[64], buf2[64];

    /* Pre IFFT complex multiply plus IFFT cmplx conjugate */
    for (i = 0; i < 64; i++) {
	k = fftorder[i];
	t_r = pre2[i].real;
	t_i = pre2[i].imag;

	buf1[i].real = t_i * data[254-k] + t_r * data[k];
	buf1[i].imag = t_r * data[254-k] - t_i * data[k];

	buf2[i].real = t_i * data[255-k] + t_r * data[k+1];
	buf2[i].imag = t_r * data[255-k] - t_i * data[k+1];
    }

    ifft64 (buf1);
    ifft64 (buf2);

    /* Post IFFT complex multiply */
    /* Window and convert to real valued signal */
    for (i = 0; i < 32; i++) {
	/* y1[n] = z1[n] * (xcos2[n] + j * xs in2[n]) ; */ 
	t_r = post2[i].real;
	t_i = post2[i].imag;

	a_r = t_r * buf1[i].real    + t_i * buf1[i].imag;
	a_i = t_i * buf1[i].real    - t_r * buf1[i].imag;
	b_r = t_i * buf1[63-i].real + t_r * buf1[63-i].imag;
	b_i = t_r * buf1[63-i].real - t_i * buf1[63-i].imag;

	c_r = t_r * buf2[i].real    + t_i * buf2[i].imag;
	c_i = t_i * buf2[i].real    - t_r * buf2[i].imag;
	d_r = t_i * buf2[63-i].real + t_r * buf2[63-i].imag;
	d_i = t_r * buf2[63-i].real - t_i * buf2[63-i].imag;

	w_1 = window[2*i];
	w_2 = window[255-2*i];
	data[2*i]     = delay[2*i] * w_2 - a_r * w_1 + bias;
	data[255-2*i] = delay[2*i] * w_1 + a_r * w_2 + bias;
	delay[2*i] = c_i;

	w_1 = window[128+2*i];
	w_2 = window[127-2*i];
	data[128+2*i] = delay[127-2*i] * w_2 + a_i * w_1 + bias;
	data[127-2*i] = delay[127-2*i] * w_1 - a_i * w_2 + bias;
	delay[127-2*i] = c_r;

	w_1 = window[2*i+1];
	w_2 = window[254-2*i];
	data[2*i+1]   = delay[2*i+1] * w_2 - b_i * w_1 + bias;
	data[254-2*i] = delay[2*i+1] * w_1 + b_i * w_2 + bias;
	delay[2*i+1] = d_r;

	w_1 = window[129+2*i];
	w_2 = window[126-2*i];
	data[129+2*i] = delay[126-2*i] * w_2 + b_r * w_1 + bias;
	data[126-2*i] = delay[126-2*i] * w_1 - b_r * w_2 + bias;
	delay[126-2*i] = d_i;
    }
}

static double besselI0 (double x)
{
    double bessel = 1;
    int i = 100;

    do
	bessel = bessel * x / (i * i) + 1;
    while (--i);
    return bessel;
}

void a52_imdct_init (uint32_t mm_accel)
{
    int i, k;
    double sum;
    sample_t local_imdct_window[256];

    /* compute imdct window - kaiser-bessel derived window, alpha = 5.0 */
    sum = 0;
    for (i = 0; i < 256; i++) {
	sum += besselI0 (i * (256 - i) * (5 * M_PI / 256) * (5 * M_PI / 256));
	local_imdct_window[i] = sum;
    }
    sum++;

    for (i = 0; i < 256; i++)
	a52_imdct_window[i] = (sample_t) (sqrt (local_imdct_window[i] / sum));

    for (i = 0; i < 3; i++)
	roots16[i] = cos ((M_PI / 8) * (i + 1));

    for (i = 0; i < 7; i++)
	roots32[i] = cos ((M_PI / 16) * (i + 1));

    for (i = 0; i < 15; i++)
	roots64[i] = cos ((M_PI / 32) * (i + 1));

    for (i = 0; i < 31; i++)
	roots128[i] = cos ((M_PI / 64) * (i + 1));

    for (i = 0; i < 64; i++) {
	k = fftorder[i] / 2 + 64;
	pre1[i].real = cos ((M_PI / 256) * (k - 0.25));
	pre1[i].imag = sin ((M_PI / 256) * (k - 0.25));
    }

    for (i = 64; i < 128; i++) {
	k = fftorder[i] / 2 + 64;
	pre1[i].real = -cos ((M_PI / 256) * (k - 0.25));
	pre1[i].imag = -sin ((M_PI / 256) * (k - 0.25));
    }

    for (i = 0; i < 64; i++) {
	post1[i].real = cos ((M_PI / 256) * (i + 0.5));
	post1[i].imag = sin ((M_PI / 256) * (i + 0.5));
    }

    for (i = 0; i < 64; i++) {
	k = fftorder[i] / 4;
	pre2[i].real = cos ((M_PI / 128) * (k - 0.25));
	pre2[i].imag = sin ((M_PI / 128) * (k - 0.25));
    }

    for (i = 0; i < 32; i++) {
	post2[i].real = cos ((M_PI / 128) * (i + 0.5));
	post2[i].imag = sin ((M_PI / 128) * (i + 0.5));
    }

    {
	int i, j, k;
	
/* Twiddle factors to turn IFFT into IMDCT */
	for (i = 0; i < 128; i++) {
	    xcos1[i] = -cos ((M_PI / 2048) * (8 * i + 1));
	    xsin1[i] = -sin ((M_PI / 2048) * (8 * i + 1));
	}
	for (i = 0; i < 128; i++) {
	    sseSinCos1c[2*i+0]= xcos1[i];
	    sseSinCos1c[2*i+1]= -xcos1[i];
	    sseSinCos1d[2*i+0]= xsin1[i];
	    sseSinCos1d[2*i+1]= xsin1[i];	
	}

	/* More twiddle factors to turn IFFT into IMDCT */
	for (i = 0; i < 64; i++) {
	    xcos2[i] = -cos ((M_PI / 1024) * (8 * i + 1));
	    xsin2[i] = -sin ((M_PI / 1024) * (8 * i + 1));
	}

	for (i = 0; i < 7; i++) {
	    j = 1 << i;
	    for (k = 0; k < j; k++) {
		w[i][k].real = cos (-M_PI * k / j);
		w[i][k].imag = sin (-M_PI * k / j);
	    }
	}
	for (i = 1; i < 7; i++) {
	    j = 1 << i;
	    for (k = 0; k < j; k+=2) {
	    
	    	sseW[i][4*k + 0] = w[i][k+0].real;
	    	sseW[i][4*k + 1] = w[i][k+0].real;
	    	sseW[i][4*k + 2] = w[i][k+1].real;
	    	sseW[i][4*k + 3] = w[i][k+1].real;

	    	sseW[i][4*k + 4] = -w[i][k+0].imag;
	    	sseW[i][4*k + 5] = w[i][k+0].imag;
	    	sseW[i][4*k + 6] = -w[i][k+1].imag;
	    	sseW[i][4*k + 7] = w[i][k+1].imag;	    
	    	
	//we multiply more or less uninitialized numbers so we need to use exactly 0.0
		if(k==0)
		{
//			sseW[i][4*k + 0]= sseW[i][4*k + 1]= 1.0;
			sseW[i][4*k + 4]= sseW[i][4*k + 5]= 0.0;
		}
		
		if(2*k == j)
		{
			sseW[i][4*k + 0]= sseW[i][4*k + 1]= 0.0;
//			sseW[i][4*k + 4]= -(sseW[i][4*k + 5]= -1.0);
		}
	    }
	}

	for(i=0; i<128; i++)
	{
		sseWindow[2*i+0]= -imdct_window[2*i+0];
		sseWindow[2*i+1]=  imdct_window[2*i+1];	
	}
	
	for(i=0; i<64; i++)
	{
		sseWindow[256 + 2*i+0]= -imdct_window[254 - 2*i+1];
		sseWindow[256 + 2*i+1]=  imdct_window[254 - 2*i+0];
		sseWindow[384 + 2*i+0]=  imdct_window[126 - 2*i+1];
		sseWindow[384 + 2*i+1]= -imdct_window[126 - 2*i+0];
	}
    }


    {
	//fprintf (stderr, "No accelerated IMDCT transform found\n");
	ifft128 = ifft128_c;
	ifft64 = ifft64_c;
    }
#ifndef __GNUC__
    if(mm_accel & FF_CPU_SSE)
        a52_imdct_512 = imdct_do_512_sse;
    else
#endif    
        a52_imdct_512 = a52_imdct_512_C;

/*
    else
    if(mm_accel & FF_CPU_3DNOWEXT)
    {
      a52_imdct_512 = imdct_do_512_3dnowex;
    }
    else
    if(mm_accel & FF_CPU_3DNOW)
    {
      a52_imdct_512 = imdct_do_512_3dnow;
    }
*/
    a52_imdct_256 = a52_imdct_256_C;
}
