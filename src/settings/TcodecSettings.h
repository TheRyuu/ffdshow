#ifndef _TCODECSETTINGS_H_
#define _TCODECSETTINGS_H_

#include "ffcodecs.h"
#include "ffImgfmt.h"
#include "rational.h"
#include "Toptions.h"

#pragma warning(push)
#pragma warning(disable:4201)

struct DVprofile;
struct TcoSettings : public Toptions {
private:
    typedef int TcoSettings::* TintVal;
    void onIncspChange(int, int);
public:
    TcoSettings(TintStrColl *Icoll = NULL);

    TintStrColl *options;
    void saveReg(void), loadReg(void);

    int mode;
    int bitrate1000;
    int desiredSize;

    int codecId;
    FOURCC fourcc;

    int interlacing, interlacing_tff;
    int dropFrames;

    int forceIncsp, incspFourcc;
    TcspInfos incsps;
    void fillIncsps(void);
    int isProc;
    int flip;

    int storeAVI;
    int storeExt;
    char_t storeExtFlnm[MAX_PATH];
    int muxer;

    int q_i_min, q_i_max, q_p_min, q_p_max, q_b_min, q_b_max, q_mb_min, q_mb_max;
    bool isQuantControlActive(void) const {
        return mode != ENC_MODE::VBR_QUANT;
    }
    int i_quant_factor, i_quant_offset;
    int quant, qual;
    int qns;
    int isIntraQuantBias, intraQuantBias;
    int isInterQuantBias, interQuantBias;
    int dct_algo;

    static std::pair<int, int> getMinMaxQuant(int codecId) {
        return std::make_pair(1, 31);
    }
    std::pair<int, int> getMinMaxQuant(void) const {
        return getMinMaxQuant(codecId);
    }
    static const int minQuant = 1, maxQuant = 51; //global

    int limitq(int q) const {
        return limit(q, getMinMaxQuant().first, getMinMaxQuant().second);
    }

    //one pass with libavcodec
    int ff1_vratetol;
    int ff1_vqcomp;
    int ff1_vqblur1, ff1_vqblur2;
    int ff1_vqdiff;
    int ff1_rc_squish, ff1_rc_max_rate1000, ff1_rc_min_rate1000, ff1_rc_buffer_size;

    int is_lavc_nr, lavc_nr;
    int huffyuv_csp;
    int huffyuv_pred;
    int huffyuv_ctx;
    static const char_t *huffYUVcsps[], *huffYUVpreds[];

    int ljpeg_csp;

    int ffv1_coder;
    int ffv1_context;
    int ffv1_key_interval;
    int ffv1_csp;
    static const TcspFcc ffv1csps[];
    static const char_t *ffv1coders[];
    static const char_t *ffv1contexts[];

    int dv_profile;
    std::vector<const DVprofile*> getDVprofile(unsigned int dx, unsigned int dy) const;
    const DVprofile* getDVprofile(unsigned int dx, unsigned int dy, PixelFormat lavc_pix_fmt) const;

    FOURCC raw_fourcc;

    int gray;
};

#pragma warning(pop)

#endif
