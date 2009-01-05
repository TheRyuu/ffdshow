/*
 * Copyright (c) 2005,2006 Milan Cutka
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "stdafx.h"
#include "TimgFilterOutput.h"
#include "Tconvert.h"
#include "TffPict.h"
#include "ToutputVideoSettings.h"
#include "IffdshowBase.h"
#include "Tlibavcodec.h"
#include "IffdshowDecVideo.h"
#include "ffdebug.h"

TimgFilterOutput::TimgFilterOutput(IffdshowBase *Ideci,Tfilters *Iparent):TimgFilter(Ideci,Iparent),
 convert(NULL),
 libavcodec(NULL),avctx(NULL),dv(false),frame(NULL),dvpict(NULL),
 firsttime(true),
 vramBenchmark(this)
{
}
TimgFilterOutput::~TimgFilterOutput()
{
 if (convert) delete convert;
 if (dv) libavcodec->avcodec_close(avctx);
 if (frame) libavcodec->av_free(frame);
 if (avctx) libavcodec->av_free(avctx);
 if (libavcodec) libavcodec->Release();
 if (dvpict) delete dvpict;
}

HRESULT TimgFilterOutput::process(const TffPict &pict,int dstcsp,unsigned char *dst[4],int dstStride[4],LONG &dstSize,const ToutputVideoSettings *cfg)
{
 if (firsttime)
  {
   firsttime=false;
   if (cfg->dv && deci->getLibavcodec(&libavcodec)==S_OK)
    if (AVCodec *codec=libavcodec->avcodec_find_encoder(CODEC_ID_DVVIDEO))
     {
      avctx=libavcodec->avcodec_alloc_context();
      avctx->width=pict.rectFull.dx;
      avctx->height=pict.rectFull.dy;
      avctx->pix_fmt=PIX_FMT_YUV420P;
      if (libavcodec->avcodec_open(avctx,codec)>=0)
       {
        dv=true;
        frame=libavcodec->avcodec_alloc_frame();
        dvcsp=csp_lavc2ffdshow(avctx->pix_fmt);
        dvpict=new TffPict;
        dvpict->alloc(pict.rectFull.dx,pict.rectFull.dy,dvcsp,dvpictbuf);
       }
     }
  }

 if (!convert || convert->dx!=pict.rectFull.dx || convert->dy!=pict.rectFull.dy || old_cspOptionsRgbInterlaceMode != cfg->cspOptionsRgbInterlaceMode || old_avisynthYV12_RGB != cfg->avisynthYV12_RGB)
  {
   old_avisynthYV12_RGB = cfg->avisynthYV12_RGB;
   old_cspOptionsRgbInterlaceMode = cfg->cspOptionsRgbInterlaceMode;
   if (convert) delete convert;
   vramBenchmark.init();
   convert=new Tconvert(deci,pict.rectFull.dx,pict.rectFull.dy);
  }
 stride_t cspstride[4];unsigned char *cspdst[4];
 if (!dv)
  {
   const TcspInfo *outcspInfo=csp_getInfo(dstcsp);
   for (int i=0;i<4;i++)
    {
     cspstride[i]=dstStride[0]>>outcspInfo->shiftX[i];
     if (i==0)
      {
       cspdst[i]=dst[i];
      }
     else
      {
       cspdst[i]=cspdst[i-1]+cspstride[i-1]*(pict.rectFull.dy>>outcspInfo->shiftY[i-1]);
      }
    }
  }
 const TffPict *dvp;
 if (!dv || pict.csp!=dvcsp)
  {
   if (convert->m_wasChange)
    vramBenchmark.onChange();
   int cspret=convert->convert(pict,
                               ((dv ? dvcsp : dstcsp) ^ (cfg->flip ? FF_CSP_FLAGS_VFLIP : 0)) | ((pict.fieldtype & FIELD_TYPE::MASK_INT) ? FF_CSP_FLAGS_INTERLACED : 0),
                               dv ? dvpict->data : cspdst,
                               dv ? dvpict->stride : cspstride,
                               vramBenchmark.get_vram_indirect());
   vramBenchmark.update();
   if (!dv) return cspret?S_OK:E_FAIL;
   dvp=dvpict;
  }
 else
  dvp=&pict;
 for (int i=0;i<4;i++)
  {
   frame->data[i]=dvp->data[i];
   frame->linesize[i]=(int)dvp->stride[i];
  }
 int ret=libavcodec->avcodec_encode_video(avctx,dst[0],dstSize,frame);
 if (ret<0)
  return E_FAIL;
 else
  {
   dstSize=ret;
   return S_FALSE;
  }
}

void TimgFilterOutput::TvramBenchmark::init(void)
{
 vram_indirect = false;
 frame_count = 0;
}

TimgFilterOutput::TvramBenchmark::TvramBenchmark(TimgFilterOutput *Iparent) : parent(Iparent)
{
 init();
}

bool TimgFilterOutput::TvramBenchmark::get_vram_indirect(void)
{
 if (frame_count < BENCHMARK_FRAMES * 2)
  {
   parent->deciV->get_CurrentTime(&t1);
   return !(frame_count & 1);
  }
 return vram_indirect;
}

void TimgFilterOutput::TvramBenchmark::update(void)
{
 if (frame_count < BENCHMARK_FRAMES * 2)
  {
   REFERENCE_TIME t_indirect=0,t_direct=0;
   unsigned int frame_count_half = frame_count >> 1;
   parent->deciV->get_CurrentTime(&t2);

   if (frame_count & 1)
    time_on_convert_direct[frame_count_half] = t2 - t1;
   else
    time_on_convert_indirect[frame_count_half] = t2 - t1;

   if (frame_count & 1)
    {
     for (unsigned int i = 0 ; i <= frame_count_half ; i++)
      {
       t_indirect += time_on_convert_indirect[i];
       t_direct   += time_on_convert_direct[i];
      }

     // Judge the benchmark result.
     // Indirect mode has penalty : 1ms per frame.
     // For the 1st pair, if indirect mode is 5x faster, indirect wins.
     // For the average upto 2nd pair, if indirect mode is 4x faster, indirect wins.
     // ...
     // For the average upto 5th pair, if indirect mode is 1x faster, indirect wins.
     if ((t_indirect + (frame_count_half + 1) * 10000 /* 1ms */) * (BENCHMARK_FRAMES - frame_count_half) < t_direct)
      {
       frame_count = BENCHMARK_FRAMES * 2; // stop updating, as result is known.
       vram_indirect = true;
      }
     else
      vram_indirect = false;
    }
   frame_count++;
   if (frame_count == BENCHMARK_FRAMES * 2)
    DPRINTF(_l("TimgFilterOutput::process V-RAM access is %s, t_indirect = %I64i, t_direct = %I64i"),vram_indirect ? _l("Indirect") : _l("Direct"),t_indirect,t_direct);
  }
}

HRESULT TimgFilterOutputConvert::process(TfilterQueue::iterator it,TffPict &pict,const TfilterSettingsVideo *cfg0)
{
 const ToutputVideoConvertSettings *cfg=(const ToutputVideoConvertSettings*)cfg0;
 init(pict,true,false);
 const unsigned char *src[4];
 getCur(cfg->csp,pict,true,src);
 return parent->deliverSample(++it,pict);
}
