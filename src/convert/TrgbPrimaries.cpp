/*
 * Copyright (c) 2007-2009 h.yamagata
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "stdafx.h"
#include "IffdshowBase.h"
#include "IffdshowDecVideo.h"
#include "ffdshow_constants.h"
#include "TrgbPrimaries.h"
#include "ToutputVideoSettings.h"
#include "image.h"
#include "TffRect.h"
#include "libavcodec/avcodec.h"
#include "ffdshow_mediaguids.h"

//===================================== TrgbPrimaries ====================================
TrgbPrimaries::TrgbPrimaries():
    deciV(NULL)
{
    reset();
}

TrgbPrimaries::TrgbPrimaries(IffdshowBase *deci):
    deciV(NULL)
{
    deciV = comptrQ<IffdshowDecVideo>(deci);
    if (deciV) {
        UpdateSettings(AVCOL_RANGE_UNSPECIFIED, AVCOL_SPC_UNSPECIFIED);
    } else {
        reset();
    }
}

// return a value that has to be added to RGB
int TrgbPrimaries::UpdateSettings(enum AVColorRange video_full_range_flag, enum AVColorSpace YCbCr_RGB_matrix_coefficients)
{
    if (deciV) {
        const ToutputVideoSettings *outcfg = deciV->getToutputVideoSettings(); // This pointer may change during playback.
        cspOptionsIturBt = (ffYCbCr_RGB_MatrixCoefficientsType)outcfg->cspOptionsIturBt;
        if (cspOptionsIturBt == ITUR_BT_AUTO) {
            if (YCbCr_RGB_matrix_coefficients == AVCOL_SPC_UNSPECIFIED) {
                const Trect *decodedRect = deciV->getDecodedPictdimensions();
                if (decodedRect) {
                    if (decodedRect->dx > 1024 || decodedRect->dy >= 600) {
                        cspOptionsIturBt = ITUR_BT709;
                    } else {
                        cspOptionsIturBt = ITUR_BT601;
                    }
                } else {
                    cspOptionsIturBt = ITUR_BT601;
                }
            } else if (YCbCr_RGB_matrix_coefficients == AVCOL_SPC_BT709) {
                cspOptionsIturBt = ITUR_BT709;
            } else if (YCbCr_RGB_matrix_coefficients == AVCOL_SPC_SMPTE240M) {
                cspOptionsIturBt = SMPTE240M;
            } else {
                cspOptionsIturBt = ITUR_BT601;
            }
        }
        cspOptionsBlackCutoff = outcfg->get_cspOptionsBlackCutoff(video_full_range_flag);
        cspOptionsWhiteCutoff = outcfg->get_cspOptionsWhiteCutoff(video_full_range_flag);
        cspOptionsChromaCutoff = outcfg->get_cspOptionsChromaCutoff(video_full_range_flag);
        cspOptionsRGB_BlackLevel = outcfg->cspOptionsOutputLevelsMode == PcRGB ? 0 : 16;
        cspOptionsRGB_WhiteLevel = outcfg->cspOptionsOutputLevelsMode == PcRGB ? 255 : 235;

        return (int)cspOptionsRGB_BlackLevel - std::min((int)cspOptionsRGB_BlackLevel, cspOptionsBlackCutoff);
    }
    wasJpeg = false;
    wasFraps = false;
    return PcRGB;
}

void TrgbPrimaries::reset(void)
{
    cspOptionsIturBt = ITUR_BT601;
    cspOptionsBlackCutoff = 16;
    cspOptionsWhiteCutoff = 235;
    cspOptionsChromaCutoff = 16;
    wasJpeg = false;
    wasFraps = false;
}

void TrgbPrimaries::setJpeg(bool isjpeg, int rgb_add)
{
    // force BT.601 PC-YUV for MJPEG.
    if (deciV) {
        const ToutputVideoSettings *outcfg = deciV->getToutputVideoSettings(); // This pointer may change during playback.
        int cspOptionsInputLevelsMode = (int)outcfg->cspOptionsInputLevelsMode;
        if (isjpeg) {
            if ((int)outcfg->cspOptionsIturBt == ITUR_BT_AUTO) { // it's possible that cspOptionsIturBt have already been updated in UpdateSettings method, so we're checking the value selected by the user.
                cspOptionsIturBt = ITUR_BT601;
            }
            if (cspOptionsInputLevelsMode != TrgbPrimaries::CutomYCbCr) {
                cspOptionsBlackCutoff = 0;
                cspOptionsWhiteCutoff = 255;
            }
            cspOptionsChromaCutoff = 1;
            if (!wasJpeg) {
                initXvid(rgb_add);
            }
            wasJpeg = true;
        } else if (deciV->getMovieFOURCC() == FOURCC_FPS1) {
            if ((int)outcfg->cspOptionsIturBt == ITUR_BT_AUTO) { // it's possible that cspOptionsIturBt have already been updated in UpdateSettings method, so we're checking the value selected by the user.
                cspOptionsIturBt = ITUR_BT709; // sRGB
            }
            if (cspOptionsInputLevelsMode != TrgbPrimaries::CutomYCbCr) {
                cspOptionsBlackCutoff = 0;
                cspOptionsWhiteCutoff = 255;
            }
            cspOptionsChromaCutoff = 1;
            if (!wasFraps) {
                initXvid(rgb_add);
            }
            wasFraps = true;
        }
    }
}

void TrgbPrimaries::writeToXvidRgb2YCbCrMatrix(short *asmData)
{
#define toRgb2YCbCrDataI                                                       \
 double Kr,Kg,Kb;                                                              \
 if (cspOptionsIturBt == ITUR_BT601)                                           \
  {                                                                            \
   Kr = 0.299;                                                                 \
   Kg = 0.587;                                                                 \
   Kb = 0.114;                                                                 \
  }                                                                            \
 else if (cspOptionsIturBt == SMPTE240M)                                       \
  {                                                                            \
   Kr = 0.2122;                                                                \
   Kg = 0.7013;                                                                \
   Kb = 0.0865;                                                                \
  }                                                                            \
 else                                                                          \
  {                                                                            \
   Kr = 0.2125;                                                                \
   Kg = 0.7154;                                                                \
   Kb = 0.0721;                                                                \
  }                                                                            \
                                                                               \
 int whiteCutOff = cspOptionsWhiteCutoff >= 255 ? 255 : cspOptionsWhiteCutoff; \
 int blackCutOff = cspOptionsBlackCutoff <= 0 ? 1 : cspOptionsBlackCutoff;     \
 double y_range   = whiteCutOff - blackCutOff;                                 \
 double chr_range = 128 - cspOptionsChromaCutoff;

    toRgb2YCbCrDataI
    asmData[ 0] = asmData[ 4] = short((Kb * y_range / 255.0) * 256.0 + 0.5);
    asmData[ 1] = asmData[ 5] = short((Kg * y_range / 255.0) * 256.0 + 0.5);
    asmData[ 2] = asmData[ 6] = short((Kr * y_range / 255.0) * 256.0 + 0.5);

    asmData[ 8] = asmData[12] =
                      asmData[18] = asmData[22] = short((chr_range / 255.0) * 256.0 + 0.5);
    asmData[ 9] = asmData[13] = short(-((Kg * chr_range / 255.0 / (1 - Kb)) * 256.0 + 0.5));
    asmData[10] = asmData[14] = short(-((Kr * chr_range / 255.0 / (1 - Kb)) * 256.0 + 0.5));

    asmData[16] = asmData[20] = short(-((Kb * chr_range / 255.0 / (1 - Kr)) * 256.0 + 0.5));
    asmData[17] = asmData[21] = short(-((Kg * chr_range / 255.0 / (1 - Kr)) * 256.0 + 0.5));

    asmData[24] = short(blackCutOff);

    asmData[ 3] = asmData[ 7] =
                      asmData[11] = asmData[15] =
                                        asmData[19] = asmData[23] =
                                                asmData[25] = 0;
}

void TrgbPrimaries::writeToXvidYCbCr2RgbMatrix(short *asmData)
{
    TYCbCr2RGB_coeffs coeffs((ffYCbCr_RGB_MatrixCoefficientsType)cspOptionsIturBt, cspOptionsWhiteCutoff, cspOptionsBlackCutoff, cspOptionsChromaCutoff, cspOptionsRGB_WhiteLevel, cspOptionsRGB_BlackLevel);

    short Y_MUL = short(coeffs.y_mul  * 64 + 0.4);
    short UG_MUL = short(coeffs.ug_mul * 64 + 0.5);
    short VG_MUL = short(coeffs.vg_mul * 64 + 0.5);
    short UB_MUL = short(coeffs.ub_mul * 64 + 0.5);
    short VR_MUL = short(coeffs.vr_mul * 64 + 0.5);
    for (int i = 0 ; i < 8 ; i++) {
        asmData[i]   = (short)coeffs.Ysub;          // YSUB
        asmData[i + 8] = 128;                // U_SUB
        asmData[i + 16] = 128;               // V_SUB
        asmData[i + 24] = Y_MUL;
        asmData[i + 32] = UG_MUL;
        asmData[i + 40] = VG_MUL;
        asmData[i + 48] = UB_MUL;
        asmData[i + 56] = VR_MUL;
    }
    int *asmData_RGB_ADD = (int *)(&asmData[72]);
    asmData_RGB_ADD[0] = asmData_RGB_ADD[1] = asmData_RGB_ADD[2] = asmData_RGB_ADD[3] = coeffs.RGB_add3;
}

const void TrgbPrimaries::initXvid(int rgb_add)
{
    TYCbCr2RGB_coeffs coeffs((ffYCbCr_RGB_MatrixCoefficientsType)cspOptionsIturBt, cspOptionsWhiteCutoff, cspOptionsBlackCutoff, cspOptionsChromaCutoff, cspOptionsRGB_WhiteLevel, cspOptionsRGB_BlackLevel);
    xvid_colorspace_init(coeffs);
}

const Tmmx_ConvertRGBtoYUY2matrix* TrgbPrimaries::getAvisynthRgb2YuvMatrix(void)
{
    toRgb2YCbCrDataI
    mmx_ConvertRGBtoYUY2matrix.cybgr_64 = (int64_t(Kr * y_range / 255.0 * 32768.0) << 32) + (int64_t(Kg * y_range / 255.0 * 32768.0) << 16) + int64_t(Kb * y_range / 255.0 * 32768.0);
    mmx_ConvertRGBtoYUY2matrix.fpix_mul = (int64_t(112.0 / ((1 - Kr) * 255.0) * 32768.0 + 0.5) << 32) + int64_t(112.0 / ((1 - Kb) * 255.0) * 32768.0 + 0.5);
    mmx_ConvertRGBtoYUY2matrix.fraction = int((cspOptionsBlackCutoff + 0.5) * 32768.0);
    mmx_ConvertRGBtoYUY2matrix.sub_32 = -cspOptionsBlackCutoff * 2;
    mmx_ConvertRGBtoYUY2matrix.y1y2_mult = int(255.0 / y_range * 16384.0);
    return &mmx_ConvertRGBtoYUY2matrix;
}

const unsigned char* TrgbPrimaries::getAvisynthYCbCr2RgbMatrix(int &rgb_add)
{
    static const int64_t avisynthMmxMatrixConstants[10] = {
        0x0080008000800080LL, 0x0080008000800080LL,
        0x00FF00FF00FF00FFLL, 0x00FF00FF00FF00FFLL,
        0x0000200000002000LL, 0x0000200000002000LL,
        0xFF000000FF000000LL, 0xFF000000FF000000LL,
        0xFF00FF00FF00FF00LL, 0xFF00FF00FF00FF00LL
    };

    TYCbCr2RGB_coeffs coeffs((ffYCbCr_RGB_MatrixCoefficientsType)cspOptionsIturBt, cspOptionsWhiteCutoff, cspOptionsBlackCutoff, cspOptionsChromaCutoff, cspOptionsRGB_WhiteLevel, cspOptionsRGB_BlackLevel);
    // Avisynth YUY2->RGB
    short *avisynthMmxMatrix = (short*)getAlignedPtr(avisynthMmxMatrixBuf);

    int cy = short(coeffs.y_mul * 16384 + 0.5);
    short crv = short(coeffs.vr_mul * 8192 + 0.5);
    short cgu = short(-coeffs.ug_mul * 8192 - 0.5);
    short cgv = short(-coeffs.vg_mul * 8192 - 0.5);
    short cbu = short(coeffs.ub_mul * 8192 + 0.5);
    memcpy(&avisynthMmxMatrix[8], avisynthMmxMatrixConstants, 80); // common part
    int *avisynthMmxMatrixInt = (int*)avisynthMmxMatrix;
    avisynthMmxMatrix[0] = avisynthMmxMatrix[1] =
                               avisynthMmxMatrix[2] = avisynthMmxMatrix[3] = // This is wrong for mmx ([2] and [3] should be 0). Fortunately, these bytes are ignored.
                                           short(coeffs.Ysub);
    avisynthMmxMatrixInt[2] = avisynthMmxMatrixInt[3] = 0;
    avisynthMmxMatrixInt[24] = avisynthMmxMatrixInt[25] = avisynthMmxMatrixInt[26] = avisynthMmxMatrixInt[27] = coeffs.RGB_add3;
    avisynthMmxMatrixInt[28] = avisynthMmxMatrixInt[29] = avisynthMmxMatrixInt[30] = avisynthMmxMatrixInt[31] = cy;
    avisynthMmxMatrixInt[32] = avisynthMmxMatrixInt[33] = avisynthMmxMatrixInt[34] = avisynthMmxMatrixInt[35] = crv << 16;
    avisynthMmxMatrix[72] = avisynthMmxMatrix[74] = avisynthMmxMatrix[76] = avisynthMmxMatrix[78] = cgu;
    avisynthMmxMatrix[73] = avisynthMmxMatrix[75] = avisynthMmxMatrix[77] = avisynthMmxMatrix[79] = cgv;
    avisynthMmxMatrixInt[40] = avisynthMmxMatrixInt[41] = avisynthMmxMatrixInt[42] = avisynthMmxMatrixInt[43] = cbu;
    rgb_add = coeffs.RGB_add1;
    return (const unsigned char*)avisynthMmxMatrix; // none 0 value indiates that adding ofs_rgb_add to RGB is necessary.
}

const int32_t* TrgbPrimaries::toSwscaleTable(void)
{
    TYCbCr2RGB_coeffs coeffs((ffYCbCr_RGB_MatrixCoefficientsType)cspOptionsIturBt, cspOptionsWhiteCutoff, cspOptionsBlackCutoff, cspOptionsChromaCutoff, cspOptionsRGB_WhiteLevel, cspOptionsRGB_BlackLevel);

    swscaleTable[0] = int32_t(coeffs.vr_mul * 65536 + 0.5);
    swscaleTable[1] = int32_t(coeffs.ub_mul * 65536 + 0.5);
    swscaleTable[2] = int32_t(coeffs.ug_mul * 65536 + 0.5);
    swscaleTable[3] = int32_t(coeffs.vg_mul * 65536 + 0.5);
    swscaleTable[4] = int32_t(coeffs.y_mul  * 65536 + 0.5);
    swscaleTable[5] = int32_t(coeffs.Ysub * 65536);
    swscaleTable[6] = coeffs.RGB_add1;
    return swscaleTable;
}
