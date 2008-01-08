#ifndef _TIMGFILTER_H_
#define _TIMGFILTER_H_

#include "Tfilter.h"
#include "TffRect.h"
#include "TffPict.h"
#include "ffImgfmt.h"
#include "TimgFilters.h"

struct TfilterSettings;
struct TglobalSettingsDecVideo;
class Tconvert;
class TimgFilter :public Tfilter
{
private:
 void free(void);
 Tbuffer own1;
 Tbuffer own2;
 Tbuffer own3;
 Tconvert *convert1,*convert2;
 bool getCur(int csp,TffPict &pict,int full,const unsigned char **src[4]);
 bool getNext(int csp,TffPict &pict,int full,unsigned char **dst[4],const Trect *rect2=NULL);
 bool getNext(int csp,TffPict &pict,const Trect &clipRect,unsigned char **dst[4],const Trect *rect2=NULL);
 bool getCurNext(int csp,TffPict &pict,int full,int copy,unsigned char **dst[4],Tbuffer &buf);

 int pictHalf;
 int oldBrightness;
protected:
 Trect pictRect;
 unsigned int dx1[4],dy1[4],dx2[4],dy2[4];
 stride_t stride1[4],stride2[4];
 int csp1,csp2;
 comptrQ<IffdshowDecVideo> deciV;
 TimgFilters *parent;
 void init(const TffPict &pict,int full,int half);
 virtual void onSizeChange(void) {}
 void checkBorder(TffPict &pict);
 virtual bool is(const TffPictBase &pict,const TfilterSettingsVideo *cfg);
 virtual int getSupportedInputColorspaces(const TfilterSettingsVideo *cfg) const=0;
 virtual int getSupportedOutputColorspaces(const TfilterSettingsVideo *cfg) const {return getSupportedInputColorspaces(cfg);}
 bool getCur(int csp,TffPict &pict,int full,const unsigned char **src0,const unsigned char **src1,const unsigned char **src2,const unsigned char **src3)
  {
   const unsigned char **src[4]={src0,src1,src2,src3};
   return getCur(csp,pict,full,src);
  }
 bool getCur(int csp,TffPict &pict,int full,const unsigned char *src[4])
  {
   const unsigned char **srcP[4]={&src[0],&src[1],&src[2],&src[3]};
   return getCur(csp,pict,full,srcP);
  }
 bool getNext(int csp,TffPict &pict,int full,unsigned char **dst0,unsigned char **dst1,unsigned char **dst2,unsigned char **dst3,const Trect *rect2=NULL)
  {
   unsigned char **dst[4]={dst0,dst1,dst2,dst3};
   return getNext(csp,pict,full,dst,rect2);
  }
 bool getNext(int csp,TffPict &pict,int full,unsigned char *dst[4],const Trect *rect2=NULL)
  {
   unsigned char **dstP[4]={&dst[0],&dst[1],&dst[2],&dst[3]};
   return getNext(csp,pict,full,dstP,rect2);
  }
 bool getNext(int csp,TffPict &pict,const Trect &clipRect,unsigned char **dst0,unsigned char **dst1,unsigned char **dst2,unsigned char **dst3,const Trect *rect2=NULL)
  {
   unsigned char **dst[4]={dst0,dst1,dst2,dst3};
   return getNext(csp,pict,clipRect,dst,rect2);
  }
 bool getNext(int csp,TffPict &pict,const Trect &clipRect,unsigned char *dst[4],const Trect *rect2=NULL)
  {
   unsigned char **dstP[4]={&dst[0],&dst[1],&dst[2],&dst[3]};
   return getNext(csp,pict,clipRect,dstP,rect2);
  }
 enum
  {
   COPYMODE_NO  =0,
   COPYMODE_CLIP=1,
   COPYMODE_FULL=2,
   COPYMODE_DEF =3
  };
 bool getCurNext(int csp,TffPict &pict,int full,int copy,unsigned char **dst0,unsigned char **dst1,unsigned char **dst2,unsigned char **dst3)
  {
   unsigned char **dst[4]={dst0,dst1,dst2,dst3};
   return getCurNext(csp,pict,full,copy,dst,own2);
  }
 bool getCurNext(int csp,TffPict &pict,int full,int copy,unsigned char *dst[4])
  {
   unsigned char **dstP[4]={&dst[0],&dst[1],&dst[2],&dst[3]};
   return getCurNext(csp,pict,full,copy,dstP,own2);
  }
 bool getCurNext3(int csp,TffPict &pict,int full,int copy,unsigned char *dst[4])
  {
   unsigned char **dstP[4]={&dst[0],&dst[1],&dst[2],&dst[3]};
   return getCurNext(csp,pict,full,copy,dstP,own3);
  }
 bool screenToPict(CPoint &pt);
public:
 TimgFilter(IffdshowBase *Ideci,Tfilters *Iparent);
 virtual ~TimgFilter();
 virtual bool acceptRandomYV12andRGB32(void) {return false;} // Subtitle filter may change color space to RGB32 randomly.
 virtual bool getOutputFmt(TffPictBase &pict,const TfilterSettingsVideo *cfg);
 virtual HRESULT process(TfilterQueue::iterator it,TffPict &pict,const TfilterSettingsVideo *cfg0)=0;
 virtual HRESULT flush(TfilterQueue::iterator it,TffPict &pict,const TfilterSettingsVideo *cfg0);
};

#endif
