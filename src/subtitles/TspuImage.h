#ifndef _TSPUIMAGE_H_
#define _TSPUIMAGE_H_

#include "Tfont.h"
#include "postproc/swscale.h"
#include "Crect.h"

struct TspuPlane
{
private:
 size_t allocated;
public:
 unsigned char *c,*r;
 stride_t stride;
 TspuPlane():c(NULL),r(NULL),allocated(0) {}
 ~TspuPlane()
  {
   if (c) aligned_free(c);
   if (r) aligned_free(r);
  }
 void alloc(const CSize &sz, int div, int csp);
 void setZero();
};

struct Tlibmplayer;
struct TspuImage : TrenderedSubtitleWordBase
{
protected:
 int csp;
 TspuPlane plane[3];
 CRect rect[3];
 class Tscaler
  {
  protected:
   int csp;
  public:
   int srcdx,srcdy,dstdx,dstdy;
   static Tscaler *create(const TprintPrefs &prefs,int srcdx,int srcdy,int dstdx,int dstdy);
   Tscaler(const TprintPrefs &prefs,int Isrcdx,int Isrcdy,int Idstdx,int Idstdy):srcdx(Isrcdx),srcdy(Isrcdy),dstdx(Idstdx),dstdy(Idstdy),csp(prefs.csp & FF_CSPS_MASK) {}
   virtual ~Tscaler() {}
   virtual void scale(const unsigned char *srci,const unsigned char *srca,stride_t srcStride,unsigned char *dsti,unsigned char *dsta,stride_t dstStride)=0;
  };
 class TscalerPoint :public Tscaler
  {
  public:
   TscalerPoint(const TprintPrefs &prefs,int srcdx,int srcdy,int dstdx,int dstdy);
   virtual void scale(const unsigned char *srci,const unsigned char *srca,stride_t srcStride,unsigned char *dsti,unsigned char *dsta,stride_t dstStride);
  };
 class TscalerApprox :public Tscaler
  {
  private:
   unsigned int scalex,scaley;
  public:
   TscalerApprox(const TprintPrefs &prefs,int srcdx,int srcdy,int dstdx,int dstdy);
   virtual void scale(const unsigned char *srci,const unsigned char *srca,stride_t srcStride,unsigned char *dsti,unsigned char *dsta,stride_t dstStride);
  };
 class TscalerFull :public Tscaler
  {
  public:
   TscalerFull(const TprintPrefs &prefs,int srcdx,int srcdy,int dstdx,int dstdy);
   virtual void scale(const unsigned char *srci,const unsigned char *srca,stride_t srcStride,unsigned char *dsti,unsigned char *dsta,stride_t dstStride);
  };
 class TscalerBilin :public Tscaler
  {
  private:
   struct scale_pixel
    {
     unsigned position;
     unsigned left_up;
     unsigned right_down;
    } *table_x,*table_y;
   static void scale_table(unsigned int start_src,unsigned int start_tar,unsigned int end_src,unsigned int end_tar,scale_pixel *table);
   static int canon_alpha(int alpha)
    {
     return alpha?256-alpha:0;
    }
  public:
   TscalerBilin(const TprintPrefs &prefs,int srcdx,int srcdy,int dstdx,int dstdy);
   virtual ~TscalerBilin();
   virtual void scale(const unsigned char *srci,const unsigned char *srca,stride_t srcStride,unsigned char *dsti,unsigned char *dsta,stride_t dstStride);
  };
 class TscalerSw :public Tscaler
  {
  private:
   SwsFilter filter;
   SwsContext *ctx;
   Tlibmplayer *libmplayer;
   TscalerApprox approx;
  public:
   TscalerSw(const TprintPrefs &prefs,int srcdx,int srcdy,int dstdx,int dstdy);
   virtual ~TscalerSw();
   virtual void scale(const unsigned char *srci,const unsigned char *srca,stride_t srcStride,unsigned char *dsti,unsigned char *dsta,stride_t dstStride);
  };
public:
 TspuImage(const TspuPlane src[3],const CRect &rcclip,const CRect &rectReal,const CRect &rectOrig,const TprintPrefs &prefs);
 virtual void ownprint(
    const TprintPrefs &prefs,
    unsigned char **dst,
    const stride_t *stride)=0;
};

template<class _mm> struct TspuImageSimd : public TspuImage
{
public:
 TspuImageSimd(const TspuPlane src[3],const CRect &rcclip,const CRect &rectReal,const CRect &rectOrig,const TprintPrefs &prefs):TspuImage(src,rcclip,rectReal,rectOrig,prefs) {}
 virtual void ownprint(
    const TprintPrefs &prefs,
    unsigned char **dst,
    const stride_t *stride);
 virtual void print(int startx, int starty, unsigned int dx[3],int dy[3],unsigned char *dstLn[3],const stride_t stride[3],const unsigned char *bmp[3],const unsigned char *msk[3],REFERENCE_TIME rtStart=REFTIME_INVALID) const;
};

#endif
