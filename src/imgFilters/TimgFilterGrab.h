#ifndef _TIMGFILTERGRAB_H_
#define _TIMGFILTERGRAB_H_

#include "TimgFilter.h"
#include "ffcodecs.h"
#include "Tlibavcodec.h"
#include "IimgFilterGrab.h"
#include "TffPict.h"
#include "ffmpeg/libavcodec/avcodec.h"

class Tdll;
struct Tconfig;
struct Tlibavcodec;
struct AVCodecContext;
struct AVFrame;
class TimgFilterGrab : public TimgFilter, public IimgFilterGrab
    _DECLARE_FILTER(TimgFilterGrab, TimgFilter)
    private:
        struct TimgExport
    {
public:
        bool ok, inited;
        TimgExport(void): ok(false), inited(false) {}
        virtual ~TimgExport() {}
        virtual void init(unsigned int dx, unsigned int dy) = 0;
        virtual uint64_t requiredCSP() = 0;
        virtual int compress(const unsigned char *src[4], stride_t stride[4], unsigned char *dst, unsigned int dstlen, int qual) = 0;
        virtual void done(void) {
            inited = false;
        }
    };
struct TimgExportLibavcodec : public TimgExport {
private:
    Tlibavcodec *dll;
    AVCodecContext *avctx;
    AVFrame *picture;
    AVCodecID codecId;
    bool avctxinited;
protected:
    virtual int setQual(int qual) = 0;
public:
    TimgExportLibavcodec(const Tconfig *config, IffdshowBase *deci, AVCodecID IcodecId);
    virtual ~TimgExportLibavcodec();
    virtual void init(unsigned int dx, unsigned int dy);
    virtual int compress(const unsigned char *src[4], stride_t stride[4], unsigned char *dst, unsigned int dstlen, int qual);
    virtual void done(void);
};
struct TimgExportJPEG : public TimgExportLibavcodec {
protected:
    virtual int setQual(int qual) {
        return int(FF_QP2LAMBDA * (30.0f * (100 - qual) / 100 + 1));
    }
public:
    TimgExportJPEG(const Tconfig *config, IffdshowBase *deci): TimgExportLibavcodec(config, deci, AV_CODEC_ID_MJPEG) {}
    virtual uint64_t requiredCSP() {
        return FF_CSP_420P | FF_CSP_FLAGS_YUV_JPEG;
    }
};
struct TimgExportPNG : public TimgExportLibavcodec {
    virtual int setQual(int qual) {
        /* FFmpeg uses lossless PNG compression, we therefore won't use the
         * quality parameter. Instead we use a fixed compression level that
         * has a good balance between output file size and compression performance.
         */
        return 3;
    }
public:
    // We are interesting in writing RGB24 (as required by PNG), so we have to select FF_CSP_BGR24.
    // see the comment above the FF_CSP_ enum definition.
    TimgExportPNG(const Tconfig *config, IffdshowBase *deci): TimgExportLibavcodec(config, deci, AV_CODEC_ID_PNG) {}
    virtual uint64_t requiredCSP() {
        return FF_CSP_BGR24;
    }
};
struct TimgExportBMP : public TimgExport {
private:
    BITMAPFILEHEADER bfh;
    BITMAPCOREHEADER bch;
public:
    TimgExportBMP(void);
    virtual void init(unsigned int dx, unsigned int dy);
    // We are interesting in writing BGR24 (as required for BMP), so we have to select FF_CSP_RGB24.
    // see the comment above the FF_CSP_ enum definition.
    virtual uint64_t requiredCSP() {
        return FF_CSP_RGB24 | FF_CSP_FLAGS_VFLIP;
    }
    virtual int compress(const unsigned char *src[4], stride_t stride[4], unsigned char *dst, unsigned int dstlen, int qual);
    virtual int compressRGB32(const unsigned char *src[4], stride_t stride[4], unsigned char *dst, unsigned int dstlen, int qual);
};
TimgExport *exp[3];
unsigned char *dstbuf;
unsigned int dstbuflen;
volatile LONG now;
TffPict temp;
Tbuffer buffer;
protected:
virtual bool is(const TffPictBase &pict, const TfilterSettingsVideo *cfg);
virtual uint64_t getSupportedInputColorspaces(const TfilterSettingsVideo *cfg) const;
virtual void onSizeChange(void);
public:
TimgFilterGrab(IffdshowBase *Ideci, Tfilters *Iparent);
virtual ~TimgFilterGrab();
virtual void done(void);
virtual HRESULT process(TfilterQueue::iterator it, TffPict &pict, const TfilterSettingsVideo *cfg);
static HRESULT grabRGB32ToBMP(const unsigned char *src[4], stride_t stride[4], int dx, int dy, char_t *filename);

virtual HRESULT queryInterface(const IID &iid, void **ptr) const;
STDMETHODIMP grabNow(void);
virtual bool acceptRandomYV12andRGB32(void)
{
    return true;
}
};

#endif
