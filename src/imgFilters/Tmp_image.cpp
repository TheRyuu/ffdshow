/*
 * Copyright (c) 2006-2007 h.yamagata
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
#include "ffImgfmt.h"
#include "Tmp_image.h"

Tmp_image::Tmp_image(TffPict &pict, int full, const unsigned char *src[4])
{
 int dx,dy;
 if (full)
  {
   dx = pict.rectFull.dx;
   dy = pict.rectFull.dy;
  }
 else
  {
   dx = pict.rectClip.dx;
   dy = pict.rectClip.dy;
  }

 mpi = new_mp_image(dx, dy);
 mp_image_setfmt(mpi, csp_ffdshow2mplayer(pict.csp));
 for (unsigned int i = 0 ; i < pict.cspInfo.numPlanes ; i++)
  {
   mpi->planes[i] = (unsigned char *)src[i];
   mpi->stride[i] = pict.stride[i];
  }
 mpi->fields = MP_IMGFIELD_ORDERED | (MP_IMGFIELD_TOP_FIRST * !!(pict.fieldtype & FIELD_TYPE::INT_TFF));
 
}

Tmp_image::~Tmp_image()
{
 free_mp_image(mpi);
}