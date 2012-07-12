#ifndef _TVIDEOCODECXVID4_H_
#define _TVIDEOCODECXVID4_H_

#include "TvideoCodec.h"

class Tdll;
struct Textradata;
class TvideoCodecXviD4 : public TvideoCodecDec
{
private:
    void create(void);
    Tdll *dll;
public:
    TvideoCodecXviD4(IffdshowBase *Ideci, IdecVideoSink *IsinkD);
    virtual ~TvideoCodecXviD4();
    int (*xvid_global)(void *handle, int opt, void *param1, void *param2);
    int (*xvid_decore)(void *handle, int opt, void *param1, void *param2);
    int (*xvid_plugin_single)(void *handle, int opt, void *param1, void *param2);
    int (*xvid_plugin_lumimasking)(void *handle, int opt, void *param1, void *param2);
    static const char_t *dllname;
private:
    void *enchandle, *dechandle;
    int psnr;
    TffPict pict;
    Tbuffer pictbuf;
    static int me_hq(int rd3), me_(int me3);
    Textradata *extradata;
    REFERENCE_TIME rtStart, rtStop;
protected:
    virtual bool beginDecompress(TffPictBase &pict, FOURCC infcc, const CMediaType &mt, int sourceFlags);
    virtual HRESULT flushDec(void);
public:
    virtual int getType(void) const {
        return IDFF_MOVIE_XVID4;
    }
    virtual int caps(void) const {
        return CAPS::VIS_QUANTS;
    }

    virtual HRESULT decompress(const unsigned char *src, size_t srcLen, IMediaSample *pIn);
};

#endif
