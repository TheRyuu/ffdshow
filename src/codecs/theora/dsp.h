/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2003                *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

  function:

 ********************************************************************/

#ifndef DSP_H
#define DSP_H

#include "xiph/theora/theora.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PB_INSTANCE;
typedef struct
{
  void   (*save_fpu)            (void);
  void   (*restore_fpu)         (void);

  void   (*sub8x8)  		(unsigned char *FiltPtr, unsigned char *ReconPtr,
	                   	 ogg_int16_t *DctInputPtr, ogg_uint32_t PixelsPerLine,
				 ogg_uint32_t ReconPixelsPerLine);

  void   (*sub8x8_128) 		(unsigned char *FiltPtr, ogg_int16_t *DctInputPtr,
			         ogg_uint32_t PixelsPerLine);

  void   (*sub8x8avg2) 		(unsigned char *FiltPtr, unsigned char *ReconPtr1,
		                 unsigned char *ReconPtr2, ogg_int16_t *DctInputPtr,
			         ogg_uint32_t PixelsPerLine,
			         ogg_uint32_t ReconPixelsPerLine);

  void   (*copy8x8)  		(unsigned char *src, unsigned char *dest,
		                 ogg_uint32_t stride);

  void   (*recon_intra8x8)  	(unsigned char *ReconPtr, ogg_int16_t *ChangePtr,
		                 ogg_uint32_t LineStep);

  void   (*recon_inter8x8)  	(unsigned char *ReconPtr, unsigned char *RefPtr,
		                 ogg_int16_t *ChangePtr, ogg_uint32_t LineStep);

  void   (*recon_inter8x8_half)	(unsigned char *ReconPtr, unsigned char *RefPtr1,
		  		 unsigned char *RefPtr2, ogg_int16_t *ChangePtr,
				 ogg_uint32_t LineStep);

  void   (*fdct_short)          (ogg_int16_t *InputData, ogg_int16_t *OutputData);

  ogg_uint32_t (*row_sad8)	(unsigned char *Src1, unsigned char *Src2);

  ogg_uint32_t (*col_sad8x8)	(unsigned char *Src1, unsigned char *Src2,
		  		 ogg_uint32_t stride);

  ogg_uint32_t (*sad8x8)	(unsigned char *ptr1, ogg_uint32_t stride1,
		        	 unsigned char *ptr2, ogg_uint32_t stride2);

  ogg_uint32_t (*sad8x8_thres)	(unsigned char *ptr1, ogg_uint32_t stride1,
		       		 unsigned char *ptr2, ogg_uint32_t stride2,
				 ogg_uint32_t thres);

  ogg_uint32_t (*sad8x8_xy2_thres)(unsigned char *SrcData, ogg_uint32_t SrcStride,
		                 unsigned char *RefDataPtr1,
			         unsigned char *RefDataPtr2, ogg_uint32_t RefStride,
				 ogg_uint32_t thres);

  ogg_uint32_t (*intra8x8_err)	(unsigned char *DataPtr, ogg_uint32_t Stride);

  ogg_uint32_t (*inter8x8_err)	(unsigned char *SrcData, ogg_uint32_t SrcStride,
		                 unsigned char *RefDataPtr, ogg_uint32_t RefStride);

  ogg_uint32_t (*inter8x8_err_xy2)(unsigned char *SrcData, ogg_uint32_t SrcStride,
		                 unsigned char *RefDataPtr1,
			         unsigned char *RefDataPtr2, ogg_uint32_t RefStride);

  void (*IDct1)( ogg_int16_t * InputData, ogg_int16_t *QuantMatrix,  ogg_int16_t * OutputData);
  void (*IDct10)( ogg_int16_t * InputData, ogg_int16_t *QuantMatrix,  ogg_int16_t * OutputData);
  void (*IDctSlow)( ogg_int16_t * InputData, ogg_int16_t *QuantMatrix,  ogg_int16_t * OutputData);

  void (*FilterHoriz)(unsigned char * PixelPtr, ogg_int32_t LineLength, ogg_int32_t *BoundingValuePtr);
  void (*FilterVert)(unsigned char * PixelPtr, 	ogg_int32_t LineLength, ogg_int32_t *BoundingValuePtr);
  void (*SetupBoundingValueArray)(struct PB_INSTANCE *pbi, ogg_int32_t FLimit);

  void (*DeblockLoopFilteredBand)(struct PB_INSTANCE *pbi, unsigned char *SrcPtr, unsigned char *DesPtr,ogg_uint32_t PlaneLineStep, ogg_uint32_t FragsAcross,ogg_uint32_t StartFrag,const ogg_uint32_t *QuantScale);
  void (*DeringBlockStrong)(const unsigned char *SrcPtr,unsigned char *DstPtr,const ogg_int32_t Pitch,ogg_uint32_t FragQIndex,const ogg_uint32_t *QuantScale);
  void (*DeringBlockWeak)( const unsigned char *SrcPtr,unsigned char *DstPtr,const ogg_int32_t Pitch,ogg_uint32_t FragQIndex,const ogg_uint32_t *QuantScale);
} DspFunctions;

extern DspFunctions dsp_funcs;

extern void dsp_recon_init (DspFunctions *funcs);

void dsp_init(DspFunctions *funcs);
void dsp_static_init(void);
void dsp_dct_init (DspFunctions *funcs);
void dsp_i386_mmx_fdct_init(DspFunctions *funcs);
void dsp_i386_mmx_init(DspFunctions *funcs);
void dsp_i386_mmxext_init(DspFunctions *funcs);
void dsp_i386_mmx_recon_init(DspFunctions *funcs);

#define dsp_save_fpu(funcs) (funcs.save_fpu ())
#define dsp_static_save_fpu() dsp_save_fpu(dsp_funcs)

#define dsp_restore_fpu(funcs) (funcs.restore_fpu ())
#define dsp_static_restore_fpu() dsp_restore_fpu(dsp_funcs)

#define dsp_sub8x8(funcs,a1,a2,a3,a4,a5) (funcs.sub8x8 (a1,a2,a3,a4,a5))
#define dsp_static_sub8x8(a1,a2,a3,a4,a5) dsp_sub8x8(dsp_funcs,a1,a2,a3,a4,a5)

#define dsp_sub8x8_128(funcs,a1,a2,a3) (funcs.sub8x8_128 (a1,a2,a3))
#define dsp_static_sub8x8_128(a1,a2,a3) dsp_sub8x8_128(dsp_funcs,a1,a2,a3)

#define dsp_sub8x8avg2(funcs,a1,a2,a3,a4,a5,a6) (funcs.sub8x8avg2 (a1,a2,a3,a4,a5,a6))
#define dsp_static_sub8x8avg2(a1,a2,a3,a4,a5,a6) dsp_sub8x8avg2(dsp_funcs,a1,a2,a3,a4,a5,a6)

#define dsp_copy8x8(funcs,ptr1,ptr2,str1) (funcs.copy8x8 (ptr1,ptr2,str1))
#define dsp_static_copy8x8(ptr1,ptr2,str1) dsp_copy8x8(dsp_funcs,ptr1,ptr2,str1)

#define dsp_recon_intra8x8(funcs,ptr1,ptr2,str1) (funcs.recon_intra8x8 (ptr1,ptr2,str1))
#define dsp_static_recon_intra8x8(ptr1,ptr2,str1) dsp_recon_intra8x8(dsp_funcs,ptr1,ptr2,str1)

#define dsp_recon_inter8x8(funcs,ptr1,ptr2,ptr3,str1) \
	(funcs.recon_inter8x8 (ptr1,ptr2,ptr3,str1))
#define dsp_static_recon_inter8x8(ptr1,ptr2,ptr3,str1) \
	dsp_recon_inter8x8(dsp_funcs,ptr1,ptr2,ptr3,str1)

#define dsp_recon_inter8x8_half(funcs,ptr1,ptr2,ptr3,ptr4,str1) \
	(funcs.recon_inter8x8_half (ptr1,ptr2,ptr3,ptr4,str1))
#define dsp_static_recon_inter8x8_half(ptr1,ptr2,ptr3,ptr4,str1) \
	dsp_recon_inter8x8_half(dsp_funcs,ptr1,ptr2,ptr3,ptr4,str1)

#define dsp_fdct_short(funcs,in,out) (funcs.fdct_short (in,out))
#define dsp_static_fdct_short(in,out) dsp_fdct_short(dsp_funcs,in,out)

#define dsp_row_sad8(funcs,ptr1,ptr2) (funcs.row_sad8 (ptr1,ptr2))
#define dsp_static_row_sad8(ptr1,ptr2) dsp_row_sad8(dsp_funcs,ptr1,ptr2)

#define dsp_col_sad8x8(funcs,ptr1,ptr2,str1) (funcs.col_sad8x8 (ptr1,ptr2,str1))
#define dsp_static_col_sad8x8(ptr1,ptr2,str1) dsp_col_sad8x8(dsp_funcs,ptr1,ptr2,str1)

#define dsp_sad8x8(funcs,ptr1,str1,ptr2,str2) (funcs.sad8x8 (ptr1,str1,ptr2,str2))
#define dsp_static_sad8x8(ptr1,str1,ptr2,str2) dsp_sad8x8(dsp_funcs,ptr1,str1,ptr2,str2)

#define dsp_sad8x8_thres(funcs,ptr1,str1,ptr2,str2,t) (funcs.sad8x8_thres (ptr1,str1,ptr2,str2,t))
#define dsp_static_sad8x8_thres(ptr1,str1,ptr2,str2,t) dsp_sad8x8_thres(dsp_funcs,ptr1,str1,ptr2,str2,t)

#define dsp_sad8x8_xy2_thres(funcs,ptr1,str1,ptr2,ptr3,str2,t) \
	(funcs.sad8x8_xy2_thres (ptr1,str1,ptr2,ptr3,str2,t))
#define dsp_static_sad8x8_xy2_thres(ptr1,str1,ptr2,ptr3,str2,t) \
	dsp_sad8x8_xy2_thres(dsp_funcs,ptr1,str1,ptr2,ptr3,str2,t)

#define dsp_intra8x8_err(funcs,ptr1,str1) (funcs.intra8x8_err (ptr1,str1))
#define dsp_static_intra8x8_err(ptr1,str1) dsp_intra8x8_err(dsp_funcs,ptr1,str1)

#define dsp_inter8x8_err(funcs,ptr1,str1,ptr2,str2) \
	(funcs.inter8x8_err (ptr1,str1,ptr2,str2))
#define dsp_static_inter8x8_err(ptr1,str1,ptr2,str2) \
	dsp_inter8x8_err(dsp_funcs,ptr1,str1,ptr2,str2)

#define dsp_inter8x8_err_xy2(funcs,ptr1,str1,ptr2,ptr3,str2) \
	(funcs.inter8x8_err_xy2 (ptr1,str1,ptr2,ptr3,str2))
#define dsp_static_inter8x8_err_xy2(ptr1,str1,ptr2,ptr3,str2) \
	dsp_inter8x8_err_xy2(dsp_funcs,ptr1,str1,ptr2,ptr3,str2)


#ifdef __cplusplus
}
#endif

#endif /* DSP_H */
