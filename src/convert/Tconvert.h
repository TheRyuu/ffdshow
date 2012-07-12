#ifndef _TCONVERT_H_
#define _TCONVERT_H_

#include "interfaces.h"
#include "ffImgfmt.h"
#include "IffColorspaceConvert.h"
#include "TrgbPrimaries.h"
#include "libavcodec/avcodec.h"
#include "Tconfig.h"
#include "TPerformanceCounter.h"

//#define AVISYNTH_BITBLT //use avisynth bitblt (memcpy) function to just copy frame when no colorspace conversion is needed
#define XVID_BITBLT //use xvid's YV12 -> YV12 copy function - seems to be fastest

struct Tswscale;
struct Tconfig;
struct Tlibavcodec;
struct TcspInfo;
struct Tpalette;
struct TffPict;
struct ToutputVideoSettings;
class TffdshowConverters;

class Tconvert : public TrgbPrimaries
{
private:
    void init(Tlibavcodec *Ilibavcodec, bool IavisynthYV12_RGB, unsigned int Idx, unsigned int Idy, int rgbInterlaceMode, bool dithering, bool isMPEG1);
    bool m_highQualityRGB, m_dithering;
    Tlibavcodec *libavcodec;
    Tswscale *swscale;
    bool initsws;
    bool m_isMPEG1;
    uint64_t oldincsp, oldoutcsp;
    uint64_t incsp1, outcsp1;
    const TcspInfo *incspInfo, *outcspInfo;
    int rgbInterlaceMode;
    TffdshowConverters *ffdshow_converters;

    enum {
        MODE_none,
        MODE_avisynth_yv12_to_yuy2,
        MODE_xvidImage_output,
        MODE_avisynth_yuy2_to_yv12,
        MODE_mmx_ConvertRGB32toYUY2,
        MODE_mmx_ConvertRGB24toYUY2,
        MODE_mmx_ConvertYUY2toRGB32,
        MODE_mmx_ConvertYUY2toRGB24,
        MODE_mmx_ConvertUYVYtoRGB32,
        MODE_mmx_ConvertUYVYtoRGB24,
        MODE_CLJR,
        MODE_xvidImage_input,
        MODE_swscale,
        MODE_avisynth_bitblt,
        MODE_ffdshow_converters,
        MODE_fast_copy,
        MODE_ffdshow_converters2,
        MODE_MODE_palette8torgb,
        MODE_fallback
    } mode;
    static const char_t* getModeName(int mode);

    void (*avisynth_yv12_to_yuy2)(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, int src_rowsize, stride_t src_pitch, stride_t src_pitch_uv,
                                  BYTE* dst, stride_t dst_pitch,
                                  int height);
    void (*avisynth_yuy2_to_yv12)(const BYTE* src, int src_rowsize, stride_t src_pitch,
                                  BYTE* dstY, BYTE* dstU, BYTE* dstV, stride_t dst_pitch, stride_t dst_pitchUV,
                                  int height);
    void (*palette8torgb)(const uint8_t *src, uint8_t *dst, long num_pixels, const uint8_t *palette);
    uint64_t tmpcsp;
    unsigned char *tmp[3];
    stride_t tmpStride[3];
    Tconvert *tmpConvert1, *tmpConvert2;
    unsigned int rowsize;
    void freeTmpConvert(void);
    TPerformanceCounter timer;

public:
    bool m_wasChange;
    LONG m_dstSize;
    Tconvert(IffdshowBase *deci, unsigned int Idx, unsigned int Idy, LONG dstSize = 0);
    Tconvert(Tlibavcodec *Ilibavcodec, bool highQualityRGB, unsigned int Idx, unsigned int Idy, const TrgbPrimaries &IrgbPrimaries, int rgbInterlaceMode, bool dithering, bool isMPEG1);
    ~Tconvert();
    unsigned int dx, dy, outdy;
    int convert(uint64_t incsp,
                const uint8_t*const src[],
                const stride_t srcStride[],
                uint64_t outcsp,
                uint8_t* dst[],
                stride_t dstStride[],
                const Tpalette *srcpal,
                enum AVColorRange &video_full_range_flag,
                enum AVColorSpace YCbCr_RGB_matrix_coefficients = AVCOL_SPC_UNSPECIFIED,
                bool vram_indirect = false);
    int convert(TffPict &pict, uint64_t outcsp, uint8_t* dst[], stride_t dstStride[], bool vram_indirect = false);
    static void copyPlane(BYTE *dstp, stride_t dst_pitch, const BYTE *srcp, stride_t src_pitch, int row_size, int height, bool flip = false);
};

class TffColorspaceConvert : public CUnknown,
    public IffColorspaceConvert
{
private:
    Tconfig *config;
    Tlibavcodec *libavcodec;
    Tconvert *c;
public:
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);
    DECLARE_IUNKNOWN

    TffColorspaceConvert(LPUNKNOWN punk, HRESULT *phr);
    virtual ~TffColorspaceConvert();

    STDMETHODIMP allocPicture(uint64_t csp, unsigned int dx, unsigned int dy, uint8_t *data[], stride_t stride[]);
    STDMETHODIMP freePicture(uint8_t *data[]);
    STDMETHODIMP convert(unsigned int dx, unsigned int dy, uint64_t incsp, uint8_t *src[], const stride_t srcStride[], uint64_t outcsp, uint8_t *dst[], stride_t dstStride[]);
    STDMETHODIMP convertPalette(unsigned int dx, unsigned int dy, uint64_t incsp, uint8_t *src[], const stride_t srcStride[], uint64_t outcsp, uint8_t *dst[], stride_t dstStride[], const unsigned char *pal, unsigned int numcolors);
};

#endif
