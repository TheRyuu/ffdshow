#ifndef _TLIBMPLAYER_H_
#define _TLIBMPLAYER_H_

#include "ffImgfmt.h"
#include "libaf/reorder_ch.h"
#include "TpostprocSettings.h"
#include "yadif/vf_yadif.h"

class Tdll;
struct Tconfig;
struct SwsContext;
struct SwsFilter;
struct SwsVector;
struct mp3lib_ctx;
struct SwsParams;
struct Tlibmplayer
{
private:
 Tdll *dll;
 Tlibmplayer(const Tconfig *config);
 ~Tlibmplayer();
 friend class TffdshowBase;
 friend class TffColorspaceConvert;
 int refcount;
public:
 void AddRef(void)
  {
   refcount++;
  }
 void Release(void)
  {
   if (--refcount<0)
    delete this;
  }
 static const char_t *dllname;

 void (*init_mplayer)(int mmx,int mmx2,int _3dnow,int _3dnowExt,int sse,int sse2,int ssse3);

 void (*reorder_channel_nch) (void *buf, int src_layout,int dest_layout,int chnum,int samples,int samplesize);
 void (*reorder_channel_copy_nch)(void *src,int src_layout,void *dest,int dest_layout,int chnum,int samples,int samplesize);

 //int (*GetCPUCount)(void);

 mp3lib_ctx* (*MP3_Init)(int mono);
 int (*MP3_DecodeFrame)(mp3lib_ctx* ctx,const unsigned char *Isrc,unsigned int Isrclen,unsigned char *hova,short single,unsigned int *srcUsed);
 void (*MP3_Done)(mp3lib_ctx *ctx);


 void (*yadif_init)(YadifContext *yadctx);
 void (*yadif_uninit)(YadifContext *yadctx);
 void (*yadif_filter)(YadifContext *yadctx, uint8_t *dst[3], stride_t dst_stride[3], int width, int height, int parity, int tff);

};

#endif
