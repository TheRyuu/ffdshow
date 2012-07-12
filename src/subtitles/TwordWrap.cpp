/*
 * Copyright (c) 2003-2006 Milan Cutka
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
#include "TwordWrap.h"
#include <mbstring.h>

TwordWrap::TwordWrap(int Imode, const wchar_t *Istr, int *Ipwidths, int IsplitdxMax, bool IassCompatibleMode):
    mode(Imode),
    str(Istr),
    pwidths(Ipwidths),
    splitdxMax(IsplitdxMax),
    assCompatibleMode(IassCompatibleMode)
{
    if (splitdxMax < 1) {
        splitdxMax = 1;
    }
    if (mode == 0) {
        int linesPredicted = pwidths[str.size() - 1] / splitdxMax + 1;
        splitdxMin = pwidths[str.size() - 1] / linesPredicted;
        splitdxMin = std::min(splitdxMin, splitdxMax);
        int lc = smart();
        if (linesPredicted < lc) {
            splitdxMin = pwidths[str.size() - 1] / lc;
            splitdxMin = std::min(splitdxMin, splitdxMax);
            smart();
        }
    } else if (mode == 1) {
        splitdxMin = splitdxMax;
        smart();
    } else if (mode == 2) {
        splitdxMin = splitdxMax = INT_MAX;
        smart();
    } else if (mode == 3) {
        int linesPredicted = pwidths[str.size() - 1] / splitdxMax + 1;
        splitdxMin = pwidths[str.size() - 1] / linesPredicted;
        splitdxMin = std::min(splitdxMin, splitdxMax);
        int lc = smartReverse();
        if (linesPredicted < lc) {
            splitdxMin = pwidths[str.size() - 1] / lc;
            splitdxMin = std::min(splitdxMin, splitdxMax);
            smartReverse();
        }
    }
}

int TwordWrap::smart()
{
    int left = 0;
    int strlenp = (int)str.size();
    bool skippingSpace = false;
    int xx = 0;
    rightOfTheLines.clear();
    for (int x = 0; x < strlenp; x++) {
        if (skippingSpace || (pwidths[x] - left > splitdxMin && pwidths[x] - left <= splitdxMax)) { // ideal for line break.
            if (skippingSpace) {
                if (!iswspace(str.at(x))) {
                    skippingSpace = false;
                    left = x > 0 ? pwidths[x - 1] : 0;
                    xx = x;
                }
            } else {
                if (iswspace(str.at(x))) {
                    rightOfTheLines.push_back(x > 0 ? x - 1 : 0);
                    skippingSpace = true;
                }
            }
        }
        if (pwidths[x] - left > splitdxMax && !skippingSpace) { // Not ideal, but we have to break before current word.
            bool found = false;
            skippingSpace = false;
            for (; x > xx; x--) {
                if (pwidths[x] - left <= splitdxMax) {
                    if (iswspace(str.at(x))) {
                        rightOfTheLines.push_back(x > 0 ? x - 1 : 0);
                        left = pwidths[x];
                        xx = nextChar(x);
                        found = true;
                        break;
                    }
                }
            }
            // fail over : no space to cut. Japanese subtitles often come here.
            if (!found) {
                if (assCompatibleMode) {
                    for (; x < strlenp; x++) {
                        if (iswspace(str.at(x))) {
                            rightOfTheLines.push_back(x > 0 ? x - 1 : 0);
                            skippingSpace = true;
                            break;
                        }
                    }
                } else {
                    for (; x < strlenp; x++) {
                        if (pwidths[x] - left > splitdxMin) {
                            if (x > xx && pwidths[x] - left > splitdxMax) {
                                x = prevChar(x);
                            }
                            left = pwidths[x];
                            xx = nextChar(x);
                            rightOfTheLines.push_back(x);
                            break;
                        }
                    }
                }
            }
        }
    }
    rightOfTheLines.push_back(strlenp);
    return (int)rightOfTheLines.size();
}

int TwordWrap::smartReverse()
{
    int strlenp = (int)str.size();
    int right = pwidths[strlenp - 1];
    bool skippingSpace = false;
    int xx = strlenp - 1;
    rightOfTheLines.clear();
    rightOfTheLines.push_front(xx);
    for (int x = strlenp - 1; x >= 0; x--) {
        if (skippingSpace || (right - pwidthsLeft(x) > splitdxMin && right - pwidthsLeft(x) <= splitdxMax)) { // ideal for line break.
            if (skippingSpace) {
                if (!iswspace(str.at(x))) {
                    skippingSpace = false;
                    right = pwidths[x];
                    xx = x;
                    rightOfTheLines.push_front(xx);
                }
            } else {
                if (iswspace(str.at(x))) {
                    skippingSpace = true;
                }
            }
        }
        if (right - pwidthsLeft(x) > splitdxMax && !skippingSpace) { // Not ideal, but we have to break after current word.
            bool found = false;
            skippingSpace = false;
            for (; x < xx; x++) {
                if (right - pwidths[x] <= splitdxMax) {
                    if (iswspace(str.at(x))) {
                        xx = x > 0 ? x - 1 : 0;
                        rightOfTheLines.push_front(xx);
                        right = pwidths[xx];
                        found = true;
                        break;
                    }
                }
            }
            // fail over : no space to cut. Japanese subtitles often come here.
            if (!found) {
                if (assCompatibleMode) {
                    for (; x >= 0; x--) {
                        if (iswspace(str.at(x))) {
                            skippingSpace = true;
                            break;
                        }
                    }
                } else {
                    for (; x >= 0; x--) {
                        if (right - pwidths[x] > splitdxMin) {
                            if (x < xx && right - pwidthsLeft(x) > splitdxMax) {
                                x = nextChar(x);
                            }
                            xx = prevChar(x);
                            rightOfTheLines.push_front(xx);
                            right = pwidths[xx];
                            break;
                        }
                    }
                }
            }
        }
    }
    return (int)rightOfTheLines.size();
}

int TwordWrap::nextChar(int x)
{
    if (x + 1 >= (int)str.size()) {
        return x;
    } else {
        return x + 1;
    }
}

int TwordWrap::prevChar(int x)
{
    if (x == 0) {
        return x;
    } else {
        return x - 1;
    }
}

int TwordWrap::pwidthsLeft(int x)
{
    if (x == 0) {
        return 0;
    }
    return pwidths[prevChar(x)];
}

void TwordWrap::debugprint()
{
    DPRINTF(_l("lineCount: %d"), rightOfTheLines.size());
    for (int l = 1; l <= (int)rightOfTheLines.size(); l++) {
        text<char_t> dbgstr0(str.c_str() + (l >= 2 ? rightOfTheLines[l - 2] : 0));
        const char_t *dbgstr = (const char_t*)dbgstr0;
        if (l >= 2) {
            if (*dbgstr) {
                dbgstr++;
            }
            if (*dbgstr) {
                dbgstr++;
            }
        }
        DPRINTF(_l("%d:%d %s"), l, rightOfTheLines[l - 1], dbgstr);
    }
}

int TwordWrap::getRightOfTheLine(int n)
{
    if (n < (int)rightOfTheLines.size()) {
        return rightOfTheLines[n];
    } else {
        return (int)str.size();
    }
}

int TwordWrap::iswspace(wchar_t ch)
{
    if (ch == 0xa0) {
        return false;
    }
    // call CRT
    return ::iswspace(wint_t(ch));
}
