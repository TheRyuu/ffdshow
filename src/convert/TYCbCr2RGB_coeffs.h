#ifndef _FFYCBCR_RGB_MATRIXCOEFFICIENTS_H_
#define _FFYCBCR_RGB_MATRIXCOEFFICIENTS_H_

typedef enum {
    ffYCbCr_RGB_coeff_ITUR_BT601    = 0,
    ffYCbCr_RGB_coeff_ITUR_BT709    = 1,
    ffYCbCr_RGB_coeff_SMPTE240M     = 2,
} ffYCbCr_RGB_MatrixCoefficientsType;

struct TYCbCr2RGB_coeffs {
    double Kr;
    double Kg;
    double Kb;
    double chr_range;
    double y_mul;
    double vr_mul;
    double ug_mul;
    double vg_mul;
    double ub_mul;
    int Ysub;
    int RGB_add1;
    int RGB_add3;

    TYCbCr2RGB_coeffs(ffYCbCr_RGB_MatrixCoefficientsType cspOptionsIturBt,
                      int cspOptionsWhiteCutoff,
                      int cspOptionsBlackCutoff,
                      int cspOptionsChromaCutoff,
                      double cspOptionsRGB_WhiteLevel,
                      double cspOptionsRGB_BlackLevel) {
        if (cspOptionsIturBt == ffYCbCr_RGB_coeff_ITUR_BT601) {
            Kr = 0.299;
            Kg = 0.587;
            Kb = 0.114;
        } else if (cspOptionsIturBt == ffYCbCr_RGB_coeff_SMPTE240M) {
            Kr = 0.2122;
            Kg = 0.7013;
            Kb = 0.0865;
        } else {
            Kr = 0.2125;
            Kg = 0.7154;
            Kb = 0.0721;
        }

        double in_y_range   = cspOptionsWhiteCutoff - cspOptionsBlackCutoff;
        chr_range = 128 - cspOptionsChromaCutoff;

        double cspOptionsRGBrange = cspOptionsRGB_WhiteLevel - cspOptionsRGB_BlackLevel;
        y_mul = cspOptionsRGBrange / in_y_range;
        vr_mul = (cspOptionsRGBrange / chr_range) * (1.0 - Kr);
        ug_mul = (cspOptionsRGBrange / chr_range) * (1.0 - Kb) * Kb / Kg;
        vg_mul = (cspOptionsRGBrange / chr_range) * (1.0 - Kr) * Kr / Kg;
        ub_mul = (cspOptionsRGBrange / chr_range) * (1.0 - Kb);
        int sub = std::min((int)cspOptionsRGB_BlackLevel, cspOptionsBlackCutoff);
        Ysub = cspOptionsBlackCutoff - sub;
        RGB_add1 = (int)cspOptionsRGB_BlackLevel - sub;
        RGB_add3 = (RGB_add1 << 8) + (RGB_add1 << 16) + RGB_add1;
    }
};

#endif // _FFYCBCR_RGB_MATRIXCOEFFICIENTS_H_
