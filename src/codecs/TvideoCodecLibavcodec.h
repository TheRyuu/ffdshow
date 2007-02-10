#ifndef _TVIDEOCODECLIBAVCODEC_H_
#define _TVIDEOCODECLIBAVCODEC_H_

#include "TvideoCodec.h"
#include "Tlibavcodec.h"

struct Textradata;
class TccDecoder;
class TvideoCodecLibavcodec :public TvideoCodecDec,public TvideoCodecEnc,public TlibavcodecExt
{
private:
 void create(void);
 Tlibavcodec *libavcodec;
 AVCodec *avcodec;mutable char_t codecName[100];
 AVCodecContext *avctx;
 AVPaletteControl pal;
 AVFrame *frame;
 FOURCC fcc;
 FILE *statsfile;
 int cfgcomode;
 int psnr;
 bool isAdaptive;
 int threadcount;
 int neroavc,theorart;
 bool codecinited,ownmatrices;
 REFERENCE_TIME rtStart,rtStop,avgTimePerFrame,segmentTimeStart;
 struct
  {
   REFERENCE_TIME rtStart,rtStop;
   unsigned int srcSize;
  } b[2];int posB;
 Textradata *extradata;bool sendextradata;
 Rational containerSar;
 TffPict oldpict;
 unsigned int mb_width,mb_height,mb_count;
 static void line(unsigned char *dst,unsigned int _x0,unsigned int _y0,unsigned int _x1,unsigned int _y1,stride_t strideY);
 static void draw_arrow(uint8_t *buf, int sx, int sy, int ex, int ey, stride_t stride,int mulx,int muly,int dstdx,int dstdy);
 unsigned char *ffbuf;unsigned int ffbuflen;
 bool wasKey;
 virtual void handle_user_data(const uint8_t *buf,int buf_len);
 TccDecoder *ccDecoder;
 bool autoSkipingLoopFilter;
 enum AVDiscard initialSkipLoopFilter;
protected: 
 virtual LRESULT beginCompress(int cfgcomode,int csp,const Trect &r);
 virtual bool beginDecompress(TffPictBase &pict,FOURCC infcc,const CMediaType &mt,int sourceFlags);
 virtual HRESULT flushDec(void);
public:
 TvideoCodecLibavcodec(IffdshowBase *Ideci,IdecVideoSink *IsinkD);
 TvideoCodecLibavcodec(IffdshowBase *Ideci,IencVideoSink *IsinkE);
 virtual ~TvideoCodecLibavcodec();

 virtual int getType(void) const {return IDFF_MOVIE_LAVC;}
 virtual const char_t* getName(void) const;
 virtual int caps(void) const {return CAPS::VIS_MV|CAPS::VIS_QUANTS;}

 virtual void end(void);

 virtual void getCompressColorspaces(Tcsps &csps,unsigned int outDx,unsigned int outDy);
 virtual bool supExtradata(void);
 virtual bool getExtradata(const void* *ptr,size_t *len);
 virtual HRESULT compress(const TffPict &pict,TencFrameParams &params);
 virtual HRESULT flushEnc(const TffPict &pict,TencFrameParams &params) {return compress(pict,params);}

 virtual HRESULT decompress(const unsigned char *src,size_t srcLen,IMediaSample *pIn);
 virtual void onGetBuffer(AVFrame *pic);
 virtual bool onSeek(REFERENCE_TIME segmentStart);
 virtual bool onDiscontinuity(void);
 virtual bool drawMV(unsigned char *dst,unsigned int dx,stride_t stride,unsigned int dy) const;
 virtual void getEncoderInfo(char_t *buf,size_t buflen) const;
 virtual const char* get_current_idct(void);
};

#endif
