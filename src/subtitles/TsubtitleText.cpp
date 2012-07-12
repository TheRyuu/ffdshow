/*
 * Copyright (c) 2003-2006 Milan Cutka
 *               2007-2011 h.yamagata
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
#include "TsubtitleText.h"
#include "TrenderedTextSubtitleWord.h"
#include "Tsubreader.h"
#include "TwordWrap.h"
#include "IffdshowBase.h"
#include "IffdshowDecVideo.h"
#include "TtoGdiFont.h"
#include "CPolygon.h"

//================================= TsubtitleLine ==================================
size_t TsubtitleLine::strlen(void) const
{
    size_t len = 0;
    for (Tbase::const_iterator w = this->begin(); w != this->end(); w++) {
        len +=::strlen(*w);
    }
    return len;
}

// This method duplicate the words with karaoke settings
void TsubtitleLine::applyWords(const TsubtitleFormat::Twords &words, int subFormat)
{
    // We already have one word that contains full text of the line.
    // It is going to be removed at the end of this function if the line have more than two words.

    // If there are only one word, then just change the props and return.
    if (words.size() == 1) {
        this->front().props = words.front().props;
        return;
    }

    bool karaokeNewWord = false;
    const wchar_t *lineString = this->front().getString();
    foreach(const TsubtitleFormat::Tword & w, words) {
        karaokeNewWord |= w.props.karaokeNewWord;

        // avoid adding empty words
        if (w.i1 == w.i2) {
            continue;
        }
        TsubtitleWord word(lineString + w.i1, w.i2 - w.i1);
        word.props = w.props;
        word.props.karaokeNewWord = karaokeNewWord;

        /* With SRT subtitles the secondary color is (most of the time) not set
            so we have to revert the primary (which is the overlay color of karaoke) with the secondary color */
        if ((subFormat == Tsubreader::SUB_SUBVIEWER || subFormat == Tsubreader::SUB_SUBVIEWER2)
                && word.props.karaokeMode != TSubtitleProps::KARAOKE_NONE) {
            word.props.SecondaryColour = w.props.color;
            word.props.color = w.props.SecondaryColour;
        }

        this->push_back(word);
        karaokeNewWord = false;
    }
    if (!this->empty()) {
        this->erase(this->begin());
    }
}

void TsubtitleLine::format(TsubtitleFormat &format, int sfmt, TsubtitleText &parent)
{
    TsubtitleFormat::Twords words;
    if (sfmt == Tsubreader::SUB_SSA || sfmt == Tsubreader::SUB_SUBVIEWER || sfmt == Tsubreader::SUB_SUBVIEWER2) {
        props = format.processSSA(words, *this, sfmt, parent);
        applyWords(words, sfmt);
    } else {
        props = format.processHTML(words, *this);
        applyWords(words, sfmt);
    }
}

bool TsubtitleLine::checkTrailingSpaceRight(const_iterator w) const
{
    while (++w != end()) {
        ffstring wstr(w->getString());
        if (wstr.size() && wstr.find_first_not_of(L" ") != ffstring::npos) {
            return false;
        }
    }
    return true;
}

//================================= TsubtitleText ==================================

// Copy constructor. mutex cannot be copied.
TsubtitleText::TsubtitleText(const TsubtitleText &src):
    Tsubtitle(src)
{
    insert(end(), src.begin(), src.end());
    lines = src.lines;
    subformat = src.subformat;
    defProps = src.defProps;
    old_prefs = src.old_prefs;
    rendering_ready = src.rendering_ready;
}

void TsubtitleText::fixFade(const TSubtitleProps &lineProps)
{
    // Some style over-rides belongs to line, rather than words (thanks, STaRGaZeR).
    // For such properties, lineProps has been used to store parameters.
    // Here we copy some members of lineProps to props.
    foreach(TsubtitleLine & line, *this) {
        foreach(TsubtitleWord & word, line) {
            word.props.isFad  = lineProps.isFad;
            word.props.fadeA1 = lineProps.fadeA1;
            word.props.fadeA2 = lineProps.fadeA2;
            word.props.fadeA3 = lineProps.fadeA3;
            word.props.fadeT1 = lineProps.fadeT1;
            word.props.fadeT2 = lineProps.fadeT2;
            word.props.fadeT3 = lineProps.fadeT3;
            word.props.fadeT4 = lineProps.fadeT4;
            word.props.isMove = lineProps.isMove;
            word.props.pos    = lineProps.pos;
            word.props.pos2   = lineProps.pos2;
            word.props.alignment    = lineProps.alignment;
            word.props.isAlignment = lineProps.isAlignment;
            word.props.wrapStyle = lineProps.wrapStyle;
            word.props.clip = lineProps.clip;
            word.props.isClip = lineProps.isClip;
            word.props.isOrg  = lineProps.isOrg;
            word.props.org    = lineProps.org;
        }
    }
}

void TsubtitleText::format(TsubtitleFormat &format)
{
    int sfmt = subformat & Tsubreader::SUB_FORMATMASK;
    foreach(TsubtitleLine & line, *this)
    line.format(format, sfmt, *this);

    for (Tbase::iterator l = this->begin(); l != this->end(); l++) {
        format.processMicroDVD(*this, l);
    }

    if ((sfmt == Tsubreader::SUB_MPL2) || (sfmt == Tsubreader::SUB_VPLAYER))
        foreach(TsubtitleLine & line, *this)
        format.processMPL2(line);
}

void TsubtitleText::print(
    REFERENCE_TIME time,
    bool wasseek,
    Tfont &f,
    bool forceChange,
    TprintPrefs &prefs,
    unsigned char **dst,
    const stride_t *stride)
{
    f.prepareC(this, prefs, false);
}

size_t TsubtitleText::prepareGlyph(TprintPrefs prefs, Tfont &font, bool forceChange)
{
    TrenderedPolygons renderedPolygons;
    size_t used_memory = 0;
    if (!rendering_ready || forceChange || old_prefs != prefs) {
        //if (!prefs.isOSD) DPRINTF(_l("TsubtitleText::prepareGlyph %I64i"),start);
        old_prefs = prefs;

        unsigned int dx = prefs.dx;
        unsigned int dy = prefs.dy;
        unsigned int gdi_font_scale = prefs.fontSettings.gdi_font_scale;

#if 0  // What is this code for? removed by h.yamagata
        if (prefs.sizeDx && prefs.sizeDy) {
            dx = prefs.sizeDx;
            dy = prefs.sizeDy;
        }
#endif

        IffdshowBase *deci = font.deci;

        lines.clear();
        if (!font.fontManager) {
            comptrQ<IffdshowDecVideo>(deci)->getFontManager(&font.fontManager);
        }
        TfontManager *fontManager = font.fontManager;
        bool nosplit = !prefs.fontSettings.split && !prefs.OSDitemSplit;
        double splitdx0 = nosplit ? 0 : ((int)dx - prefs.textMarginLR < 1 ? 1 : dx - prefs.textMarginLR) * gdi_font_scale;

        int *pwidths = NULL;
        Tbuffer width;

        foreach(const TsubtitleLine & l, *this) {
            int charCount = 0;
            ffstring allStr;
            Tbuffer tempwidth;
            double left = 0.0, nextleft = 0.0;
            int wordWrapMode = -1;
            double splitdxMax = splitdx0;
            if (l.empty()) {
                TtoGdiFont gf(l.props, font.hdc, prefs, dx, dy, fontManager);
                // empty lines have half height.
                TrenderedSubtitleLine *line = new TrenderedSubtitleLine(l.props, prefs, gf.getHeight() / 2.0);
                lines.push_back(line);
            }
            foreach(const TsubtitleWord & w, l) {
                TtoGdiFont gf(w.props, font.hdc, prefs, dx, dy, fontManager);
                int spacing = w.props.get_spacing(prefs);
                const wchar_t *p = w;
                double xscale = w.props.get_xscale(
                                    prefs.fontSettings.xscale / 100.0,
                                    prefs.sar,
                                    prefs.fontSettings.fontSettingsOverride);

                wordWrapMode = w.props.wrapStyle;
                int polygon = w.props.polygon; // ASS Drawing mode.

                splitdxMax = get_splitdx_for_new_line(w, dx, prefs, deci);
                allStr += p;
                pwidths = (int*)width.resize((allStr.size() + 1) * sizeof(int));
                left = nextleft;
                int nfit; // dummy
                SIZE sz;
                size_t strlenp = strlen(p);

                int *ptempwidths = NULL;
                TrenderedTextSubtitleWord *rw = NULL;
                if (polygon == 0) {
                    ptempwidths = (int*)tempwidth.alloc((strlenp + 1) * sizeof(int) * 2); // *2 to work around Layer For Unicode on Windows 9x.
                    if (spacing == 0) {
                        GetTextExtentExPointW(font.hdc, p, (int)strlenp, INT_MAX, &nfit, ptempwidths, &sz);
                    } else {
                        int sum = 0;
                        for (unsigned int i = 0; i < strlenp; i++) {
                            int char_width;
                            GetTextExtentExPointW(font.hdc, p + i, 1, INT_MAX, &nfit, &char_width, &sz);
                            sum += char_width;
                            ptempwidths[i] = sum;
                        }
                    }
                } else {
                    // polygon
                    // We need dxChar here, earlier than normal words.
                    // Store it in renderedPolygons and reuse later.
                    rw = newWord(p, strlenp, prefs, &w, gf.lf, font, true, true);
                    renderedPolygons.push_back(rw);
                }

                for (size_t x = 0; x < strlenp; x++) {
                    nextleft = left;
                    // ASS Drawing mode commands should not be wrapped.
                    if (ptempwidths) {
                        nextleft += (double)ptempwidths[x] * xscale;
                    }
                    if (rw) {
                        nextleft += rw->dxChar * 64;
                    }
                    pwidths[charCount] = int(nextleft);
                    charCount++;
                }
            }
            if (allStr.empty()) {
                continue;
            }
            if (wordWrapMode == -1) {
                // non SSA/ASS/ASS2
                if (nosplit) {
                    wordWrapMode = 2; // no word wrapping
                } else {
                    deci->getParam(IDFF_subWordWrap, &wordWrapMode);
                    if (wordWrapMode >= 2) {
                        wordWrapMode++; // wordWrapMode = 3; lower line gets wider
                    }
                }
            }
            TwordWrap wordWrap(wordWrapMode, allStr.c_str(), pwidths, (int)splitdxMax, l.props.isSSA());
            //wordWrap.debugprint();

            TrenderedSubtitleLine *line = NULL;
            int cx = 0, cy = 0;

            bool firstLine = true;

            for (TsubtitleLine::const_iterator w0 = l.begin(); w0 != l.end(); w0++) {
                TsubtitleWord w(*w0);
                TtoGdiFont gf(w.props, font.hdc, prefs, dx, dy, fontManager);
                if (!line) {
                    line = new TrenderedSubtitleLine(l.props, prefs);
                }

                const wchar_t *p = w;

                do {
                    int strlenp = (int)strlen(p);
                    // If line goes out of screen, wraps it except if no wrap defined
                    int z1 = cx + strlenp - 1;
                    int z2 = wordWrap.getRightOfTheLine(cy);
                    if (z1 <= z2) {
                        // OK, the word will be stored in the line.
                        bool trimRight = (l.checkTrailingSpaceRight(w0)) || z1 == z2;
                        TrenderedTextSubtitleWord *rw = getRenderedWord(p, strlenp, prefs, &w, gf.lf, font, line->empty(), trimRight, renderedPolygons);
                        if (rw) {
                            line->push_back(rw);
                        }
                        cx += strlenp;
                        break;
                    } else {
                        int n = wordWrap.getRightOfTheLine(cy) - cx + 1;
                        if (n <= 0) {
                            // We may have to split a word into some lines. (And don't split polygon)
                            cy++;
                            n = wordWrap.getRightOfTheLine(cy) - cx + 1;
                            if (!line->empty()) {
                                lines.push_back(line);
                                line = new TrenderedSubtitleLine(l.props, prefs);
                            }
                            if (cy >= wordWrap.getLineCount()) {
                                break;
                            }
                        }

                        TrenderedTextSubtitleWord *rw = getRenderedWord(p, n, prefs, &w, gf.lf, font, line->empty(), true, renderedPolygons);
                        w.props.karaokeNewWord = false;
                        w.props.karaokeStart = w.props.karaokeFillStart = w.props.karaokeFillEnd;
                        if (rw) {
                            line->push_back(rw);
                        }
                        if (!line->empty()) {
                            lines.push_back(line);
                            line = new TrenderedSubtitleLine(l.props, prefs);
                        }
                        if (w.props.polygon) {
                            break;
                        }
                        p += wordWrap.getRightOfTheLine(cy) - cx + 1;
                        cx = wordWrap.getRightOfTheLine(cy) + 1;
                        cy++;
                    }
                } while (cy < wordWrap.getLineCount());
            }
            if (line) {
                if (!line->empty()) {
                    lines.push_back(line);
                } else {
                    delete line;
                }
            }
        }
        // Get the bitmap of the glyph
        // Call TrenderedSubtitleLine::print with dsl NULL to prepare bitmaps of the text.
        // No real rendering here.
        // Because we need the axis of ratations (\fr) to get the bitmaps of the text, we use TrenderedSubtitleLine::print.
        // Note that the detection of collision is impossible here, because not all lines that should be displayed at the
        // same time are not included in TrenderedSubtitleLines.
        lines.print(prefs, NULL, NULL);
        used_memory = getRenderedMemorySize();
    }
    renderedPolygons.clear();
    rendering_ready = true;
    return used_memory;
}

TrenderedTextSubtitleWord* TsubtitleText::getRenderedWord(
    const wchar_t *s,
    size_t slen,
    const TprintPrefs &prefs,
    const TsubtitleWord *w,
    const LOGFONT &lf,
    const Tfont &font,
    bool trimLeftSpaces,
    bool trimRightSpaces,
    TrenderedPolygons &renderedPolygons)
{
    if (w->props.polygon) {
        if (renderedPolygons.empty()) {
            DPRINTF(L"renderedPolygons is empty. This shouldn't happen.");
            ASSERT(0);
            return NULL;
        }
        TrenderedTextSubtitleWord *rw = renderedPolygons.front();
        renderedPolygons.pop_front();
        return rw;
    }
    return newWord(s, slen, prefs, w, lf, font, trimLeftSpaces, trimRightSpaces);
}

TrenderedTextSubtitleWord* TsubtitleText::newWord(
    const wchar_t *s,
    size_t slen,
    TprintPrefs prefs,
    const TsubtitleWord *w,
    const LOGFONT &lf,
    const Tfont &font,
    bool trimLeftSpaces,
    bool trimRightSpaces)
{
    if (!*s) { return NULL; }

    TfontSettings &fontSettings = prefs.fontSettings;
    ffstring s1(s, slen);

    if (trimLeftSpaces) {
        while (s1.size() && TwordWrap::iswspace(s1.at(0))) {
            s1.erase(0, 1);
        }
    }

    if (trimRightSpaces) {
        while (s1.size() && TwordWrap::iswspace(s1.at(s1.size() - 1))) {
            s1.erase(s1.size() - 1, 1);
        }
    }

    if (s1.empty()) {
        return NULL;
    }

    TrenderedTextSubtitleWord *rw = NULL;
    if (w->props.polygon) {
        rw = new CPolygon(TSubtitleMixedProps(w->props, prefs), s);
    } else {
        rw = new TrenderedTextSubtitleWord(
            font.hdc,
            s1.c_str(),
            slen,
            prefs,
            lf,
            w->props);
    }
    return rw;
}

size_t TsubtitleText::dropRenderedLines(void)
{
    boost::unique_lock<boost::mutex> lock(mutex_lines);
    size_t released_memory = getRenderedMemorySize();
    lines.clear(); // clear pointers and delete objects.
    old_prefs.csp = 0;
    rendering_ready = false;
    return released_memory;
}

void TsubtitleTexts::print(
    REFERENCE_TIME time,
    bool wasseek,
    Tfont &f,
    bool forceChange,
    TprintPrefs &prefs,
    unsigned char **dst,
    const stride_t *stride)
{
    f.reset();
    foreach(TsubtitleText * subtitleText, *this) {
        boost::unique_lock<boost::mutex> lock(*subtitleText->get_lock_ptr(), boost::try_to_lock_t());
        if (!lock.owns_lock()) {
            // hustle glyphThread
            TthreadPriority pr(comptrQ<IffdshowDecVideo>(prefs.deci)->getGlyphThreadHandle(),
                               THREAD_PRIORITY_ABOVE_NORMAL,
                               THREAD_PRIORITY_BELOW_NORMAL);
            lock.lock();
        }
        subtitleText->print(time, wasseek, f, forceChange, prefs, dst, stride);
    }
    f.print(prefs, dst, stride);
}
