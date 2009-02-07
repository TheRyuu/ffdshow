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
#include "ffYCbCr_RGB_MatrixCoefficients.h"

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
                                const double cspOptionsRGB_BlackLevel)
{
 if (cspOptionsIturBt == ffYCbCr_RGB_coeff_ITUR_BT601)
  {
   Kr = 0.299;
   Kg = 0.587;
   Kb = 0.114;
  }
 else if (cspOptionsIturBt == ffYCbCr_RGB_coeff_SMPTE240M)
  {
   Kr = 0.2122;
   Kg = 0.7013;
   Kb = 0.0865;
  }
 else
  {
   Kr = 0.2125;
   Kg = 0.7154;
   Kb = 0.0721;
  }

 double in_y_range   = cspOptionsWhiteCutoff - cspOptionsBlackCutoff;
 chr_range = 128 - cspOptionsChromaCutoff;

 double cspOptionsRGBrange = cspOptionsRGB_WhiteLevel - cspOptionsRGB_BlackLevel;
 y_mul =cspOptionsRGBrange / in_y_range;
 vr_mul=(cspOptionsRGBrange / chr_range) * (1.0 - Kr);
 ug_mul=(cspOptionsRGBrange / chr_range) * (1.0 - Kb) * Kb / Kg;
 vg_mul=(cspOptionsRGBrange / chr_range) * (1.0 - Kr) * Kr / Kg;   
 ub_mul=(cspOptionsRGBrange / chr_range) * (1.0 - Kb);
 int sub = std::min((int)cspOptionsRGB_BlackLevel, cspOptionsBlackCutoff);
 Ysub = cspOptionsBlackCutoff - sub;
 RGB_add = (int)cspOptionsRGB_BlackLevel - sub;
 RGB_add += (RGB_add << 8) + (RGB_add << 16);
}
