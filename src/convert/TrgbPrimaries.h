#ifndef _TRGBPRIMARIES_H_
#define _TRGBPRIMARIES_H_

#include "interfaces.h"
#include "simd_common.h"
#include "TYCbCr2RGB_coeffs.h"
#include "libswscale\swscale.h"

struct ToutputVideoSettings;
typedef struct {
    int64_t cybgr_64;
    int64_t fpix_mul;
    int fraction;
    int y1y2_mult;
    int sub_32;
} Tmmx_ConvertRGBtoYUY2matrix;

class TrgbPrimaries
{
private:
    short avisynthMmxMatrixBuf[88 + 8];
    int32_t swscaleTable[7];
    Tmmx_ConvertRGBtoYUY2matrix mmx_ConvertRGBtoYUY2matrix;

    void reset(void);
    IffdshowDecVideo* deciV;
    bool wasJpeg, wasFraps;
    struct Th264Primaries {
        double green_x, green_y, blue_x, blue_y, red_X, red_y;
        double white_x, white_y;
    };
protected:
    int cspOptionsIturBt;
    int cspOptionsBlackCutoff, cspOptionsWhiteCutoff, cspOptionsChromaCutoff;
    double cspOptionsRGB_BlackLevel, cspOptionsRGB_WhiteLevel;
public:
    TrgbPrimaries(IffdshowBase *deci);
    TrgbPrimaries();
    /**
     * UpdateSettings
     * @return a value that has to be added to RGB
     */
    int UpdateSettings(enum AVColorRange video_full_range_flag, enum AVColorSpace YCbCr_RGB_matrix_coefficients);
    void writeToXvidYCbCr2RgbMatrix(short *asmData);
    void writeToXvidRgb2YCbCrMatrix(short *asmData);
    const unsigned char* getAvisynthYCbCr2RgbMatrix(int &rgb_add);
    const Tmmx_ConvertRGBtoYUY2matrix* getAvisynthRgb2YuvMatrix(void);
    const void initXvid(int rgb_add);
    const int32_t* toSwscaleTable(void);
    void setJpeg(bool isjpeg, int rgb_add = 0);
    int get_sws_cs() const {
        switch (cspOptionsIturBt) {
            case ITUR_BT709:
                return SWS_CS_ITU709;
            case SMPTE240M:
                return SWS_CS_SMPTE240M;
            case ITUR_BT601:
            default:
                return SWS_CS_ITU601;
        }
    }
    bool isYCbCrFullRange() const {
        return (cspOptionsBlackCutoff <= 1 && cspOptionsWhiteCutoff >= 254);
    }
    bool isRGB_FullRange() const {
        return (cspOptionsRGB_BlackLevel <= 1 && cspOptionsRGB_WhiteLevel >= 254);
    }
    enum {
        ITUR_BT601    = ffYCbCr_RGB_coeff_ITUR_BT601,
        ITUR_BT709    = ffYCbCr_RGB_coeff_ITUR_BT709,
        SMPTE240M     = ffYCbCr_RGB_coeff_SMPTE240M,
        ITUR_BT_AUTO  = 3,
        ITUR_BT_MAX   = 4
    };
    enum {
        RecYCbCr = 0,
        PcYCbCr = 1,
        CutomYCbCr = 2,
        AutoYCbCr = 3
    };
    enum {
        TvRGB = 0,
        PcRGB = 1,
        Invalid_RGB_range = 2
    };
};

#endif
