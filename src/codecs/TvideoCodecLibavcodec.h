#ifndef _TVIDEOCODECLIBAVCODEC_H_
#define _TVIDEOCODECLIBAVCODEC_H_

#include "TvideoCodec.h"
#include "ffmpeg/Tlibavcodec.h"
#include "ffmpeg/libavcodec/avcodec.h"

#define MAX_THREADS 8 // FIXME: This is defined in mpegvideo.h.

struct Textradata;
class TccDecoder;

struct TlibavcodecExt {
private:
    static int get_buffer(AVCodecContext *s, AVFrame *pic);
    int (*default_get_buffer)(AVCodecContext *s, AVFrame *pic);
    static void release_buffer(AVCodecContext *s, AVFrame *pic);
    void (*default_release_buffer)(AVCodecContext *s, AVFrame *pic);
    static int reget_buffer(AVCodecContext *s, AVFrame *pic);
    int (*default_reget_buffer)(AVCodecContext *s, AVFrame *pic);
    static void handle_user_data0(AVCodecContext *c, const uint8_t *buf, int buf_len);
public:
    virtual ~TlibavcodecExt() {}
    void connectTo(AVCodecContext *ctx, Tlibavcodec *libavcodec);
    virtual void onGetBuffer(AVFrame *pic) {}
    virtual void onRegetBuffer(AVFrame *pic) {}
    virtual void onReleaseBuffer(AVFrame *pic) {}
    virtual void handle_user_data(const uint8_t *buf, int buf_len) {}
};

class TvideoCodecLibavcodec : public TvideoCodecDec, public TvideoCodecEnc, public TlibavcodecExt
{
    friend class TDXVADecoderVC1;
    friend class TDXVADecoderH264;
protected:
    Tlibavcodec *libavcodec;
    void create(void);
    AVCodec *avcodec;
    mutable char_t codecName[100];
    AVCodecContext *avctx;
    uint32_t palette[AVPALETTE_COUNT];
    int palette_size;
    AVFrame *frame;
    FOURCC fcc;
    FILE *statsfile;
    int cfgcomode;
    int psnr;
    bool isAdaptive;
    int threadcount;
    bool dont_use_rtStop_from_upper_stream; // and reordering of timpestams is justified.
    bool theorart;
    bool codecinited, ownmatrices;
    REFERENCE_TIME rtStart, rtStop, avgTimePerFrame, segmentTimeStart;
    REFERENCE_TIME prior_in_rtStart, prior_in_rtStop;
    REFERENCE_TIME prior_out_rtStart, prior_out_rtStop;

    struct {
        REFERENCE_TIME rtStart, rtStop;
        unsigned int srcSize;
    } b[MAX_THREADS + 1];
    int inPosB;

    Textradata *extradata;
    bool sendextradata;
    unsigned int mb_width, mb_height, mb_count;
    static void line(unsigned char *dst, unsigned int _x0, unsigned int _y0, unsigned int _x1, unsigned int _y1, stride_t strideY);
    static void draw_arrow(uint8_t *buf, int sx, int sy, int ex, int ey, stride_t stride, int mulx, int muly, int dstdx, int dstdy);
    unsigned char *ffbuf;
    unsigned int ffbuflen;
    bool wasKey;
    virtual void handle_user_data(const uint8_t *buf, int buf_len);
    TccDecoder *ccDecoder;
    bool autoSkipingLoopFilter;
    enum AVDiscard initialSkipLoopFilter;
    int got_picture;
    bool firstSeek; // firstSeek means start of palyback.
    bool mpeg2_in_doubt;
    bool mpeg2_new_sequence;
    bool bReorderBFrame;
    REFERENCE_TIME getDuration();
    int isReallyMPEG2(const unsigned char *src, size_t srcLen);
protected:
    virtual LRESULT beginCompress(int cfgcomode, uint64_t csp, const Trect &r);
    virtual bool beginDecompress(TffPictBase &pict, FOURCC infcc, const CMediaType &mt, int sourceFlags);
    virtual HRESULT flushDec(void);
    AVCodecParserContext *parser;
public:
    TvideoCodecLibavcodec(IffdshowBase *Ideci, IdecVideoSink *IsinkD);
    TvideoCodecLibavcodec(IffdshowBase *Ideci, IencVideoSink *IsinkE);
    virtual ~TvideoCodecLibavcodec();
    virtual int getType(void) const {
        return IDFF_MOVIE_LAVC;
    }
    virtual const char_t* getName(void) const;
    virtual int caps(void) const {
        return CAPS::VIS_MV | CAPS::VIS_QUANTS;
    }

    virtual void end(void);

    virtual void getCompressColorspaces(Tcsps &csps, unsigned int outDx, unsigned int outDy);
    virtual bool supExtradata(void);
    virtual bool getExtradata(const void* *ptr, size_t *len);
    virtual HRESULT compress(const TffPict &pict, TencFrameParams &params);
    virtual HRESULT flushEnc(const TffPict &pict, TencFrameParams &params) {
        return compress(pict, params);
    }

    virtual HRESULT decompress(const unsigned char *src, size_t srcLen, IMediaSample *pIn);
    virtual void onGetBuffer(AVFrame *pic);
    virtual bool onSeek(REFERENCE_TIME segmentStart);
    virtual bool onDiscontinuity(void);
    virtual bool drawMV(unsigned char *dst, unsigned int dx, stride_t stride, unsigned int dy) const;
    virtual void getEncoderInfo(char_t *buf, size_t buflen) const;
    virtual const char* get_current_idct(void);
    virtual HRESULT BeginFlush();
    bool isReorderBFrame() {
        return bReorderBFrame;
    };
    virtual void reorderBFrames(REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop);

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
};

#endif
