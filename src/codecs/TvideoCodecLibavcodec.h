#ifndef _TVIDEOCODECLIBAVCODEC_H_
#define _TVIDEOCODECLIBAVCODEC_H_

#include "TvideoCodec.h"
#include "ffmpeg/Tlibavcodec.h"
// Do not include avcodec.h in this file, ffmpeg and ffmpeg-mt may conflict.

#define MAX_THREADS 8 // FIXME: This is defined in mpegvideo.h.

struct Textradata;
class TccDecoder;

struct TlibavcodecExt
{
private:
 static int get_buffer(AVCodecContext *s, AVFrame *pic);
 int  (*default_get_buffer)(AVCodecContext *s, AVFrame *pic);
 static void release_buffer(AVCodecContext *s, AVFrame *pic);
 void (*default_release_buffer)(AVCodecContext *s, AVFrame *pic);
 static int reget_buffer(AVCodecContext *s, AVFrame *pic);
 int  (*default_reget_buffer)(AVCodecContext *s, AVFrame *pic);
 static void handle_user_data0(AVCodecContext *c,const uint8_t *buf,int buf_len);
public:
 virtual ~TlibavcodecExt() {}
 void connectTo(AVCodecContext *ctx,Tlibavcodec *libavcodec);
 virtual void onGetBuffer(AVFrame *pic) {}
 virtual void onRegetBuffer(AVFrame *pic) {}
 virtual void onReleaseBuffer(AVFrame *pic) {}
 virtual void handle_user_data(const uint8_t *buf,int buf_len) {}
};

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
 int grayscale;
 bool isAdaptive;
 int threadcount;
 bool dont_use_rtStop_from_upper_stream; // and reordering of timpestams is justified.
 bool theorart;
 bool codecinited,ownmatrices;
 REFERENCE_TIME rtStart,rtStop,avgTimePerFrame,segmentTimeStart;

 struct
  {
   REFERENCE_TIME rtStart,rtStop;
   unsigned int srcSize;
  } b[MAX_THREADS + 1];
 int inPosB;

 Textradata *extradata;bool sendextradata;
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
 bool h264_on_MPEG2_system; // H.264 on MPEG2 trasport/program stream must have AU delimiter and start code.
 bool isMPEG2system(void);
 int got_picture;
 bool firstSeek; // firstSeek means start of palyback.
 bool mpeg2_new_sequence;
protected:
 virtual LRESULT beginCompress(int cfgcomode,int csp,const Trect &r);
 virtual bool beginDecompress(TffPictBase &pict,FOURCC infcc,const CMediaType &mt,int sourceFlags);
 virtual HRESULT flushDec(void);
public:
 TvideoCodecLibavcodec(IffdshowBase *Ideci,IdecVideoSink *IsinkD);
 TvideoCodecLibavcodec(IffdshowBase *Ideci,IencVideoSink *IsinkE);
 virtual ~TvideoCodecLibavcodec();

#if COMPILE_AS_FFMPEG_MT
 virtual int getType(void) const {return IDFF_MOVIE_FFMPEG_MT;}
#else
 virtual int getType(void) const {return IDFF_MOVIE_LAVC;}
#endif
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

 class TcodedPictureBuffer
  {
  private:
   Tbuffer priorBuf,outBuf,tmpBuf; 
   int priorSize,outSampleSize,used_bytes;
   TvideoCodecLibavcodec* parent;
   REFERENCE_TIME prior_rtStart,prior_rtStop,out_rtStart,out_rtStop;

  public:
   TcodedPictureBuffer(TvideoCodecLibavcodec* Iparent);
   void init(void);
   int append(const uint8_t *buf, int buf_size);
   int send(int *got_picture_ptr);
   void onSeek(void);
  } codedPictureBuffer;

 class Th264RandomAccess
  {
  friend class TvideoCodecLibavcodec;
  private:
   TvideoCodecLibavcodec* parent;
   int recovery_mode;  // 0:OK, 1:searching 2: found, 3:waiting for frame_num decoded, 4:waiting for POC outputed
   int recovery_frame_cnt;
   int recovery_poc;
   int thread_delay;

  public:
   Th264RandomAccess(TvideoCodecLibavcodec* Iparent);
   int search(uint8_t* buf, int buf_size);
   void onSeek(void);
   void judgeUsability(int *got_picture_ptr);
  } h264RandomAccess;

 class TtelecineManager
  {
  private:
   TvideoCodecLibavcodec* parent;
   int segment_count;
   int pos_in_group;
   struct {
    int fieldtype;
    int repeat_pict;
    REFERENCE_TIME rtStart;
   } group[4]; // 4 frames make up a group of soft telecine.
   REFERENCE_TIME average_duration,group_rtStart;
   bool film;
   int cfg_softTelecine;
  public:
   TtelecineManager(TvideoCodecLibavcodec* Iparent);
   void get_timestamps(TffPict &pict);
   void get_fieldtype(TffPict &pict);
   void new_frame(int fieldtype, int top_field_first, int repeat_pict, const REFERENCE_TIME &rtStart, const REFERENCE_TIME &rtStop);
   void onSeek(void);
  } telecineManager;
};

#endif
