/*
 * Copyright (c) 2003-2006 Milan Cutka
 *               2007-2011 h.yamagata
 * subtitles fixing code from SubRip by MJQ (subrip@divx.pl)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "stdafx.h"
#include "TSSAstyles.h"
#include "TsubtitleProps.h"
#include "TsubreaderMplayer.h"

void nmTextSubtitles::strToInt(const ffstring &str, int *i)
{
    if (!str.empty()) {
        if (str.compare(0, 4, L" yes", 4) == 0) {
            *i = 1;
            return;
        } else if (str.compare(0, 3, L" no", 3) == 0) {
            *i = 0;
            return;
        }
        wchar_t *end;
        int val = strtol(str.c_str(), &end, 10);
        if (*end == '\0' && val >= 0) {
            *i = val;
        }
    }
}

void nmTextSubtitles::strToIntMargin(const ffstring &str, int *i)
{
    if (!str.empty() /*str.size()==4 && str.compare(_L("0000"))!=0*/) {
        wchar_t *end;
        int val = strtol(str.c_str(), &end, 10);
        if (*end == '\0' && val > 0) {
            *i = val;
        }
    }
}

void nmTextSubtitles::strToDouble(const ffstring &str, double *d)
{
    if (!str.empty()) {
        wchar_t *end;
        double val = strtod(str.c_str(), &end);
        if (*end == '\0') {
            *d = val;
        }
    }
}
