/*****************************************************************************
 * rdo.c: h264 encoder library (rate-distortion optimization)
 *****************************************************************************
 * Copyright (C) 2005 x264 project
 *
 * Authors: Loren Merritt <lorenm@u.washington.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/

/* duplicate all the writer functions, just calculating bit cost
 * instead of writing the bitstream.
 * TODO: use these for fast 1st pass too. */

#define RDO_SKIP_BS

static uint8_t cabac_prefix_transition[15][128];
static uint16_t cabac_prefix_size[15][128];

/* CAVLC: produces exactly the same bit count as a normal encode */
/* this probably still leaves some unnecessary computations */
#define bs_write1(s,v)     ((s)->i_bits_encoded += 1)
#define bs_write(s,n,v)    ((s)->i_bits_encoded += (n))
#define bs_write_ue(s,v)   ((s)->i_bits_encoded += bs_size_ue(v))
#define bs_write_se(s,v)   ((s)->i_bits_encoded += bs_size_se(v))
#define bs_write_te(s,v,l) ((s)->i_bits_encoded += bs_size_te(v,l))
#define x264_macroblock_write_cavlc  x264_macroblock_size_cavlc
#include "cavlc.c"

/* CABAC: not exactly the same. x264_cabac_size_decision() keeps track of
 * fractional bits, but only finite precision. */
#undef  x264_cabac_encode_decision
#define x264_cabac_encode_decision(c,x,v) x264_cabac_size_decision(c,x,v)
#define x264_cabac_encode_terminal(c)     x264_cabac_size_decision(c,276,0)
#define x264_cabac_encode_bypass(c,v)     ((c)->f8_bits_encoded += 256)
#define x264_cabac_encode_ue_bypass(c,e,v) ((c)->f8_bits_encoded += (bs_size_ue(v+(1<<e)-1)-e)<<8)
#define x264_cabac_encode_flush(h,c)
#define x264_macroblock_write_cabac  x264_macroblock_size_cabac
#include "cabac.c"

#define COPY_CABAC h->mc.memcpy_aligned( &cabac_tmp.f8_bits_encoded, &h->cabac.f8_bits_encoded, \
        sizeof(x264_cabac_t) - offsetof(x264_cabac_t,f8_bits_encoded) )
    
static int ssd_mb( x264_t *h )
{
    return h->pixf.ssd[PIXEL_16x16]( h->mb.pic.p_fenc[0], FENC_STRIDE,
                                     h->mb.pic.p_fdec[0], FDEC_STRIDE )
         + h->pixf.ssd[PIXEL_8x8](   h->mb.pic.p_fenc[1], FENC_STRIDE,
                                     h->mb.pic.p_fdec[1], FDEC_STRIDE )
         + h->pixf.ssd[PIXEL_8x8](   h->mb.pic.p_fenc[2], FENC_STRIDE,
                                     h->mb.pic.p_fdec[2], FDEC_STRIDE );
}

static int ssd_plane( x264_t *h, int size, int p, int x, int y )
{
    return h->pixf.ssd[size]( h->mb.pic.p_fenc[p] + x+y*FENC_STRIDE, FENC_STRIDE,
                              h->mb.pic.p_fdec[p] + x+y*FDEC_STRIDE, FDEC_STRIDE );
}

static int x264_rd_cost_mb( x264_t *h, int i_lambda2 )
{
    int b_transform_bak = h->mb.b_transform_8x8;
    int i_ssd;
    int i_bits;

    x264_macroblock_encode( h );

    i_ssd = ssd_mb( h );

    if( IS_SKIP( h->mb.i_type ) )
    {
        i_bits = (1 * i_lambda2 + 128) >> 8;
    }
    else if( h->param.b_cabac )
    {
        x264_cabac_t cabac_tmp;
        COPY_CABAC;
        x264_macroblock_size_cabac( h, &cabac_tmp );
        i_bits = ( (uint64_t)cabac_tmp.f8_bits_encoded * i_lambda2 + 32768 ) >> 16;
    }
    else
    {
        bs_t bs_tmp = h->out.bs;
        bs_tmp.i_bits_encoded = 0;
        x264_macroblock_size_cavlc( h, &bs_tmp );
        i_bits = ( bs_tmp.i_bits_encoded * i_lambda2 + 128 ) >> 8;
    }

    h->mb.b_transform_8x8 = b_transform_bak;

    return i_ssd + i_bits;
}

int x264_rd_cost_part( x264_t *h, int i_lambda2, int i8, int i_pixel )
{
    int i_ssd, i_bits;

    if( i_pixel == PIXEL_16x16 )
    {
        int type_bak = h->mb.i_type;
        int i_cost = x264_rd_cost_mb( h, i_lambda2 );
        h->mb.i_type = type_bak;
        return i_cost;
    }

    x264_macroblock_encode_p8x8( h, i8 );
    if( i_pixel == PIXEL_16x8 )
        x264_macroblock_encode_p8x8( h, i8+1 );
    if( i_pixel == PIXEL_8x16 )
        x264_macroblock_encode_p8x8( h, i8+2 );

    i_ssd = ssd_plane( h, i_pixel,   0, (i8&1)*8, (i8>>1)*8 )
          + ssd_plane( h, i_pixel+3, 1, (i8&1)*4, (i8>>1)*4 )
          + ssd_plane( h, i_pixel+3, 2, (i8&1)*4, (i8>>1)*4 );

    if( h->param.b_cabac )
    {
        x264_cabac_t cabac_tmp;
        COPY_CABAC;
        x264_partition_size_cabac( h, &cabac_tmp, i8, i_pixel );
        i_bits = ( (uint64_t)cabac_tmp.f8_bits_encoded * i_lambda2 + 32768 ) >> 16;
    }
    else
    {
        i_bits = ( x264_partition_size_cavlc( h, i8, i_pixel ) * i_lambda2 + 128 ) >> 8;
    }

    return i_ssd + i_bits;
}

int x264_rd_cost_i8x8( x264_t *h, int i_lambda2, int i8, int i_mode )
{
    int i_ssd, i_bits;

    x264_mb_encode_i8x8( h, i8, h->mb.i_qp );
    i_ssd = ssd_plane( h, PIXEL_8x8, 0, (i8&1)*8, (i8>>1)*8 );

    if( h->param.b_cabac )
    {
        x264_cabac_t cabac_tmp;
        COPY_CABAC;
        x264_partition_i8x8_size_cabac( h, &cabac_tmp, i8, i_mode );
        i_bits = ( (uint64_t)cabac_tmp.f8_bits_encoded * i_lambda2 + 32768 ) >> 16;
    }
    else
    {
        i_bits = ( x264_partition_i8x8_size_cavlc( h, i8, i_mode ) * i_lambda2 + 128 ) >> 8;
    }

    return i_ssd + i_bits;
}

int x264_rd_cost_i4x4( x264_t *h, int i_lambda2, int i4, int i_mode )
{
    int i_ssd, i_bits;

    x264_mb_encode_i4x4( h, i4, h->mb.i_qp );
    i_ssd = ssd_plane( h, PIXEL_4x4, 0, block_idx_x[i4]*4, block_idx_y[i4]*4 );

    if( h->param.b_cabac )
    {
        x264_cabac_t cabac_tmp;
        COPY_CABAC;
        x264_partition_i4x4_size_cabac( h, &cabac_tmp, i4, i_mode );
        i_bits = ( (uint64_t)cabac_tmp.f8_bits_encoded * i_lambda2 + 32768 ) >> 16;
    }
    else
    {
        i_bits = ( x264_partition_i4x4_size_cavlc( h, i4, i_mode ) * i_lambda2 + 128 ) >> 8;
    }

    return i_ssd + i_bits;
}

int x264_rd_cost_i8x8_chroma( x264_t *h, int i_lambda2, int i_mode, int b_dct )
{
    int i_ssd, i_bits;

    if( b_dct )
        x264_mb_encode_8x8_chroma( h, 0, h->mb.i_chroma_qp );
    i_ssd = ssd_plane( h, PIXEL_8x8, 1, 0, 0 ) +
            ssd_plane( h, PIXEL_8x8, 2, 0, 0 );

    h->mb.i_chroma_pred_mode = i_mode;

    if( h->param.b_cabac )
    {
        x264_cabac_t cabac_tmp;
        COPY_CABAC;
        x264_i8x8_chroma_size_cabac( h, &cabac_tmp );
        i_bits = ( (uint64_t)cabac_tmp.f8_bits_encoded * i_lambda2 + 32768 ) >> 16;
    }
    else
    {
        i_bits = ( x264_i8x8_chroma_size_cavlc( h ) * i_lambda2 + 128 ) >> 8;
    }

    return i_ssd + i_bits;
}
/****************************************************************************
 * Trellis RD quantization
 ****************************************************************************/

#define TRELLIS_SCORE_MAX ((uint64_t)1<<50)
#define CABAC_SIZE_BITS 8
#define SSD_WEIGHT_BITS 5
#define LAMBDA_BITS 4

/* precalculate the cost of coding abs_level_m1 */
void x264_rdo_init( )
{
    int i_prefix;
    int i_ctx;
    for( i_prefix = 0; i_prefix < 15; i_prefix++ )
    {
        for( i_ctx = 0; i_ctx < 128; i_ctx++ )
        {
            int f8_bits = 0;
            uint8_t ctx = i_ctx;
            int i;

            for( i = 1; i < i_prefix; i++ )
                f8_bits += x264_cabac_size_decision2( &ctx, 1 );
            if( i_prefix > 0 && i_prefix < 14 )
                f8_bits += x264_cabac_size_decision2( &ctx, 0 );
            f8_bits += 1 << CABAC_SIZE_BITS; //sign

            cabac_prefix_size[i_prefix][i_ctx] = f8_bits;
            cabac_prefix_transition[i_prefix][i_ctx] = ctx;
        }
    }
}

// should the intra and inter lambdas be different?
// I'm just matching the behaviour of deadzone quant.
static const int lambda2_tab[2][52] = {
    // inter lambda = .85 * .85 * 2**(qp/3. + 10 - LAMBDA_BITS)
    {    46,      58,      73,      92,     117,     147, 
        185,     233,     294,     370,     466,     587, 
        740,     932,    1174,    1480,    1864,    2349, 
       2959,    3728,    4697,    5918,    7457,    9395, 
      11837,   14914,   18790,   23674,   29828,   37581, 
      47349,   59656,   75163,   94699,  119313,  150326, 
     189399,  238627,  300652,  378798,  477255,  601304, 
     757596,  954511, 1202608, 1515192, 1909022, 2405217, 
    3030384, 3818045, 4810435, 6060769 },
    // intra lambda = .65 * .65 * 2**(qp/3. + 10 - LAMBDA_BITS)
    {    27,      34,      43,      54,      68,      86, 
        108,     136,     172,     216,     273,     343, 
        433,     545,     687,     865,    1090,    1374, 
       1731,    2180,    2747,    3461,    4361,    5494, 
       6922,    8721,   10988,   13844,   17442,   21976, 
      27688,   34885,   43953,   55377,   69771,   87906, 
     110755,  139543,  175813,  221511,  279087,  351627, 
     443023,  558174,  703255,  886046, 1116348, 1406511, 
    1772093, 2232697, 2813022, 3544186 }
};

typedef struct {
    uint64_t score;
    int level_idx; // index into level_tree[]
    uint8_t cabac_state[10]; //just the contexts relevant to coding abs_level_m1
} trellis_node_t;

// TODO:
// support chroma and i16x16 DC
// save cabac state between blocks?
// use trellis' RD score instead of x264_mb_decimate_score?
// code 8x8 sig/last flags forwards with deadzone and save the contexts at
//   each position?
// change weights when using CQMs?

// possible optimizations:
// make scores fit in 32bit
// save quantized coefs during rd, to avoid a duplicate trellis in the final encode
// if trellissing all MBRD modes, finish SSD calculation so we can skip all of
//   the normal dequant/idct/ssd/cabac

// the unquant_mf here is not the same as dequant_mf:
// in normal operation (dct->quant->dequant->idct) the dct and idct are not
// normalized. quant/dequant absorb those scaling factors.
// in this function, we just do (quant->unquant) and want the output to be
// comparable to the input. so unquant is the direct inverse of quant,
// and uses the dct scaling factors, not the idct ones.

static inline void quant_trellis_cabac( x264_t *h, int16_t *dct,
                                 const uint16_t *quant_mf, const int *unquant_mf,
                                 const int *coef_weight, const uint8_t *zigzag,
                                 int i_ctxBlockCat, int i_lambda2, int b_ac, int i_coefs )
{
    int abs_coefs[64], signs[64];
    trellis_node_t nodes[2][8];
    trellis_node_t *nodes_cur = nodes[0];
    trellis_node_t *nodes_prev = nodes[1];
    trellis_node_t *bnode;
    uint8_t cabac_state_sig[64];
    uint8_t cabac_state_last[64];
    const int b_interlaced = h->mb.b_interlaced;
    const int f = 1 << 15; // no deadzone
    int i_last_nnz;
    int i, j;

    // (# of coefs) * (# of ctx) * (# of levels tried) = 1024
    // we don't need to keep all of those: (# of coefs) * (# of ctx) would be enough,
    // but it takes more time to remove dead states than you gain in reduced memory.
    struct {
        uint16_t abs_level;
        uint16_t next;
    } level_tree[64*8*2];
    int i_levels_used = 1;

    /* init coefs */
    for( i = i_coefs-1; i >= b_ac; i-- )
        if( (unsigned)(dct[zigzag[i]] * quant_mf[zigzag[i]] + f-1) >= 2*f )
            break;

    if( i < b_ac )
    {
        memset( dct, 0, i_coefs * sizeof(*dct) );
        return;
    }

    i_last_nnz = i;

    for( ; i >= b_ac; i-- )
    {
        int coef = dct[zigzag[i]];
        abs_coefs[i] = abs(coef);
        signs[i] = coef < 0 ? -1 : 1;
    }

    /* init trellis */
    for( i = 1; i < 8; i++ )
        nodes_cur[i].score = TRELLIS_SCORE_MAX;
    nodes_cur[0].score = 0;
    nodes_cur[0].level_idx = 0;
    level_tree[0].abs_level = 0;
    level_tree[0].next = 0;

    // coefs are processed in reverse order, because that's how the abs value is coded.
    // last_coef and significant_coef flags are normally coded in forward order, but
    // we have to reverse them to match the levels.
    // in 4x4 blocks, last_coef and significant_coef use a separate context for each
    // position, so the order doesn't matter, and we don't even have to update their contexts.
    // in 8x8 blocks, some positions share contexts, so we'll just have to hope that
    // cabac isn't too sensitive.

    if( i_coefs == 64 )
    {
        const uint8_t *ctx_sig  = &h->cabac.state[ significant_coeff_flag_offset[b_interlaced][i_ctxBlockCat] ];
        const uint8_t *ctx_last = &h->cabac.state[ last_coeff_flag_offset[b_interlaced][i_ctxBlockCat] ];
        for( i = 0; i < 63; i++ )
        {
            cabac_state_sig[i]  = ctx_sig[ significant_coeff_flag_offset_8x8[b_interlaced][i] ];
            cabac_state_last[i] = ctx_last[ last_coeff_flag_offset_8x8[i] ];
        }
    }
    else
    {
        memcpy( cabac_state_sig,  &h->cabac.state[ significant_coeff_flag_offset[b_interlaced][i_ctxBlockCat] ], 15 );
        memcpy( cabac_state_last, &h->cabac.state[ last_coeff_flag_offset[b_interlaced][i_ctxBlockCat] ], 15 );
    }
    memcpy( nodes_cur[0].cabac_state, &h->cabac.state[ coeff_abs_level_m1_offset[i_ctxBlockCat] ], 10 );

    for( i = i_last_nnz; i >= b_ac; i-- )
    {
        int i_coef = abs_coefs[i];
        int q = ( f + i_coef * quant_mf[zigzag[i]] ) >> 16;
        int abs_level;
        int cost_sig[2], cost_last[2];
        trellis_node_t n;

        // skip 0s: this doesn't affect the output, but saves some unnecessary computation.
        if( q == 0 )
        {
            // no need to calculate ssd of 0s: it's the same in all nodes.
            // no need to modify level_tree for ctx=0: it starts with an infinite loop of 0s.
            const uint32_t cost_sig0 = x264_cabac_size_decision_noup( &cabac_state_sig[i], 0 )
                                     * (uint64_t)i_lambda2 >> ( CABAC_SIZE_BITS - LAMBDA_BITS );
            for( j = 1; j < 8; j++ )
            {
                if( nodes_cur[j].score != TRELLIS_SCORE_MAX )
                {
#define SET_LEVEL(n,l) \
                    level_tree[i_levels_used].abs_level = l; \
                    level_tree[i_levels_used].next = n.level_idx; \
                    n.level_idx = i_levels_used; \
                    i_levels_used++;

                    SET_LEVEL( nodes_cur[j], 0 );
                    nodes_cur[j].score += cost_sig0;
                }
            }
            continue;
        }

        XCHG( trellis_node_t*, nodes_cur, nodes_prev );

        for( j = 0; j < 8; j++ )
            nodes_cur[j].score = TRELLIS_SCORE_MAX;

        if( i < i_coefs-1 )
        {
            cost_sig[0] = x264_cabac_size_decision_noup( &cabac_state_sig[i], 0 );
            cost_sig[1] = x264_cabac_size_decision_noup( &cabac_state_sig[i], 1 );
            cost_last[0] = x264_cabac_size_decision_noup( &cabac_state_last[i], 0 );
            cost_last[1] = x264_cabac_size_decision_noup( &cabac_state_last[i], 1 );
        }
        else
        {
            cost_sig[0] = cost_sig[1] = 0;
            cost_last[0] = cost_last[1] = 0;
        }

        // there are a few cases where increasing the coeff magnitude helps,
        // but it's only around .003 dB, and skipping them ~doubles the speed of trellis.
        // could also try q-2: that sometimes helps, but also sometimes decimates blocks
        // that are better left coded, especially at QP > 40.
        for( abs_level = q; abs_level >= q-1; abs_level-- )
        {
            int d = i_coef - ((unquant_mf[zigzag[i]] * abs_level + 128) >> 8);
            uint64_t ssd = (int64_t)d*d * coef_weight[i];

            for( j = 0; j < 8; j++ )
            {
                int node_ctx = j;
                if( nodes_prev[j].score == TRELLIS_SCORE_MAX )
                    continue;
                n = nodes_prev[j];

                /* code the proposed level, and count how much entropy it would take */
                if( abs_level || node_ctx )
                {
                    unsigned f8_bits = cost_sig[ abs_level != 0 ];
                    if( abs_level )
                    {
                        const int i_prefix = X264_MIN( abs_level - 1, 14 );
                        f8_bits += cost_last[ node_ctx == 0 ];
                        f8_bits += x264_cabac_size_decision2( &n.cabac_state[coeff_abs_level1_ctx[node_ctx]], i_prefix > 0 );
                        if( i_prefix > 0 )
                        {
                            uint8_t *ctx = &n.cabac_state[coeff_abs_levelgt1_ctx[node_ctx]];
                            f8_bits += cabac_prefix_size[i_prefix][*ctx];
                            *ctx = cabac_prefix_transition[i_prefix][*ctx];
                            if( abs_level >= 15 )
                                f8_bits += bs_size_ue( abs_level - 15 ) << CABAC_SIZE_BITS;
                            node_ctx = coeff_abs_level_transition[1][node_ctx];
                        }
                        else
                        {
                            f8_bits += 1 << CABAC_SIZE_BITS;
                            node_ctx = coeff_abs_level_transition[0][node_ctx];
                        }
                    }
                    n.score += (uint64_t)f8_bits * i_lambda2 >> ( CABAC_SIZE_BITS - LAMBDA_BITS );
                }

                n.score += ssd;

                /* save the node if it's better than any existing node with the same cabac ctx */
                if( n.score < nodes_cur[node_ctx].score )
                {
                    SET_LEVEL( n, abs_level );
                    nodes_cur[node_ctx] = n;
                }
            }
        }
    }

    /* output levels from the best path through the trellis */
    bnode = &nodes_cur[0];
    for( j = 1; j < 8; j++ )
        if( nodes_cur[j].score < bnode->score )
            bnode = &nodes_cur[j];

    j = bnode->level_idx;
    for( i = b_ac; i < i_coefs; i++ )
    {
        dct[zigzag[i]] = level_tree[j].abs_level * signs[i];
        j = level_tree[j].next;
    }
}


void x264_quant_4x4_trellis( x264_t *h, int16_t dct[4][4], int i_quant_cat,
                             int i_qp, int i_ctxBlockCat, int b_intra )
{
    int b_ac = (i_ctxBlockCat == DCT_LUMA_AC);
    quant_trellis_cabac( h, (int16_t*)dct,
        h->quant4_mf[i_quant_cat][i_qp], h->unquant4_mf[i_quant_cat][i_qp],
        x264_dct4_weight2_zigzag[h->mb.b_interlaced],
        x264_zigzag_scan4[h->mb.b_interlaced],
        i_ctxBlockCat, lambda2_tab[b_intra][i_qp], b_ac, 16 );
}


void x264_quant_8x8_trellis( x264_t *h, int16_t dct[8][8], int i_quant_cat,
                             int i_qp, int b_intra )
{
    quant_trellis_cabac( h, (int16_t*)dct,
        h->quant8_mf[i_quant_cat][i_qp], h->unquant8_mf[i_quant_cat][i_qp],
        x264_dct8_weight2_zigzag[h->mb.b_interlaced],
        x264_zigzag_scan8[h->mb.b_interlaced],
        DCT_LUMA_8x8, lambda2_tab[b_intra][i_qp], 0, 64 );
}

