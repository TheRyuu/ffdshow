#ifndef _TLIBMPLAYER_H_
#define _TLIBMPLAYER_H_

#include "ffImgfmt.h"
#include "TpostprocSettings.h"

class Tdll;
struct Tconfig;
struct mp3lib_ctx;
struct Tlibmplayer
{
private:
 Tdll *dll;
 Tlibmplayer(const Tconfig *config);
 ~Tlibmplayer();
 friend class TffdshowBase;
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

 mp3lib_ctx* (*MP3_Init)(int mono);
 int (*MP3_DecodeFrame)(mp3lib_ctx* ctx,const unsigned char *Isrc,unsigned int Isrclen,unsigned char *hova,short single,unsigned int *srcUsed);
 void (*MP3_Done)(mp3lib_ctx *ctx);
};

#endif
