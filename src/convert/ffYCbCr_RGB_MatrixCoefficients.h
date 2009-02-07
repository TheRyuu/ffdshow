#ifndef _FFYCBCR_RGB_MATRIXCOEFFICIENTS_H_
#define _FFYCBCR_RGB_MATRIXCOEFFICIENTS_H_

typedef enum
{
 ffYCbCr_RGB_coeff_ITUR_BT601    = 0,
 ffYCbCr_RGB_coeff_ITUR_BT709    = 1,
 ffYCbCr_RGB_coeff_SMPTE240M     = 2,
} ffYCbCr_RGB_MatrixCoefficientsType;

void YCbCr2RGBdata_common_inint(double &Kr,
                                double &Kg,
                                double &Kb,
                                double &chr_range,
                                double &y_mul,
                                double &vr_mul,
                                double &ug_mul,
                                double &vg_mul,
                                double &ub_mul,
                                int &Ysub,
                                int &RGB_add,
                                const ffYCbCr_RGB_MatrixCoefficientsType cspOptionsIturBt,
                                const int cspOptionsWhiteCutoff,
                                const int cspOptionsBlackCutoff,
                                const int cspOptionsChromaCutoff,
                                const double cspOptionsRGB_WhiteLevel,
                                const double cspOptionsRGB_BlackLevel);

#endif // _FFYCBCR_RGB_MATRIXCOEFFICIENTS_H_
