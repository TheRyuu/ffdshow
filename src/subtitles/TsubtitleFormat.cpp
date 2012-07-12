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
#include "TsubtitleFormat.h"
#include "Tsubreader.h"
#include "TsubtitlesSettings.h"
#include "Tconfig.h"
#include "ThtmlColors.h"

//============================ TsubtitleFormat =============================
ffstring TsubtitleFormat::getAttribute(const wchar_t *start, const wchar_t *end, const wchar_t *attrname)
{
    if (const wchar_t *attr = strnistr(start, end - start + 1, attrname))
        if (const wchar_t *eq = strnchr(attr, end - attr + 1, '=')) {
            eq++;
            bool in = false;
            for (int i = 0; i < end - eq + 1; i++)
                if (eq[i] == '"') {
                    in = !in;
                } else if (!in && (eq[i] == ' ' || eq[i] == '>' || eq[i] == '\0')) {
                    return stringreplace(ffstring(eq, i).Trim(), L"\"", L"", rfReplaceAll);
                }
        }
    return ffstring();
}

void TsubtitleFormat::processHTMLTags(Twords &words, const wchar_t* &l, const wchar_t* &l1, const wchar_t* &l2)
{
    if (_strnicmp(l2, L"<i>", 3) == 0) {
        words.add(l, l1, l2, props, 3);
        props.italic = true;
    } else if (_strnicmp(l2, L"</i>", 4) == 0) {
        words.add(l, l1, l2, props, 4);
        props.italic = false;
    } else if (_strnicmp(l2, L"<u>", 3) == 0) {
        words.add(l, l1, l2, props, 3);
        props.underline = true;
    } else if (_strnicmp(l2, L"</u>", 4) == 0) {
        words.add(l, l1, l2, props, 4);
        props.underline = false;
    } else if (_strnicmp(l2, L"<b>", 3) == 0) {
        words.add(l, l1, l2, props, 3);
        props.bold = true;
    } else if (_strnicmp(l2, L"</b>", 4) == 0) {
        words.add(l, l1, l2, props, 4);
        props.bold = false;
    } else if (_strnicmp(l2, L"<font ", 6) == 0) {
        const wchar_t *end = strchr(l2, '>');
        if (end) {
            const wchar_t *start = l2 + 6;
            ffstring face = getAttribute(start, end, L"face");
            ffstring color = getAttribute(start, end, L"color").ConvertToLowerCase();
            ffstring size = getAttribute(start, end, L"size");
            words.add(l, l1, l2, props, end - l2 + 1);
            if (!face.empty()) {
                ff_strncpy(props.fontname, (const char_t *)text<char_t>(face.c_str()), countof(props.fontname));
            }
            if (!color.empty() && ((color[0] == '#' && swscanf(color.c_str() + 1, L"%x", &props.color)) || (htmlcolors->getColor(color, &props.color, 0xffffff), true))) {
                std::swap(((uint8_t*)&props.color)[0], ((uint8_t*)&props.color)[2]);
                props.isColor = true;
            }
            if (!size.empty()) {
                wchar_t *ll;
                int s = strtol(size.c_str(), &ll, 10);
                if (*ll == '\0') {
                    props.size = s;
                }
            }
        }
    } else if (_strnicmp(l2, L"</font>", 7) == 0) {
        words.add(l, l1, l2, props, 7);
        props.isColor = false;
        props.size = 0;
        props.fontname[0] = '\0';
    }
    // Hacks for badly written HTML tags go here. Comment everything to only parse good HTML tags
    else if (_strnicmp(l2, L"< i>", 4) == 0) {
        words.add(l, l1, l2, props, 4);
        props.italic = true;
    } else if (_strnicmp(l2, L"< /i>", 5) == 0) {
        words.add(l, l1, l2, props, 5);
        props.italic = false;
    } else if (_strnicmp(l2, L"< u>", 4) == 0) {
        words.add(l, l1, l2, props, 4);
        props.underline = true;
    } else if (_strnicmp(l2, L"< /u>", 5) == 0) {
        words.add(l, l1, l2, props, 5);
        props.underline = false;
    } else if (_strnicmp(l2, L"< b>", 4) == 0) {
        words.add(l, l1, l2, props, 4);
        props.bold = true;
    } else if (_strnicmp(l2, L"< /b>", 5) == 0) {
        words.add(l, l1, l2, props, 5);
        props.bold = false;
    }
    // End of hacks
    else {
        l2++;
    }
}

const TSubtitleProps& TsubtitleFormat::processHTML(Twords &words, const TsubtitleLine &line)
{
    if (line.empty()) {
        return lineProps;
    }
    const wchar_t *l = line[0];
    const wchar_t *l1 = l, *l2 = l;
    while (*l2) {
        processHTMLTags(words, l, l1, l2);
    }

    words.add(l, l1, l2, props, 0);
    return lineProps;
}

int TsubtitleFormat::Tssa::parse_parentheses(TparenthesesContents &contents, ffstring arg)
{
    // (a1,a2, ... ,an) is expected.
    size_t first_paren = arg.find('(');
    if (first_paren == ffstring::npos) {
        return -1;
    }
    size_t second_paren = arg.find(')', first_paren + 1);
    if (second_paren == ffstring::npos) {
        return -1;
    }
    arg.erase(0, first_paren + 1);
    arg.erase(second_paren - first_paren - 1);
    strings input_strings;
    strtok(arg.c_str(), L",", input_strings);
    foreach(ffstring & s, input_strings)
    contents.push_back(TparenthesesContent(s));
    return (int)(second_paren + 1);
}

int TsubtitleFormat::Tssa::TstoreParams::writeProps(const TparenthesesContents &contents, TSubtitleProps *props)
{
    int count = 0;
    iterator store_i = begin();
    TparenthesesContents::const_iterator contents_i = contents.begin();
    for (; store_i != end() ; store_i++) {
        int64_t val;
        double doubleval;
        if (store_i->offset && store_i->isInteger) {
            if (contents_i != contents.end()
                    && contents_i->ok) {
                if (contents_i->doubleval < store_i->min) {
                    val = (int64_t)store_i->min;
                } else if (contents_i->doubleval > store_i->max) {
                    val = (int64_t)store_i->max;
                } else {
                    val = (int64_t)contents_i->doubleval;
                    count++;
                }
            } else {
                val = (int64_t)store_i->default_value;
            }
            void *dst = (uint8_t *)props + store_i->offset;
            memcpy(dst, &val, store_i->size);
        }
        if (store_i->offset && !store_i->isInteger) {
            if (contents_i != contents.end()
                    && contents_i->ok) {
                if (contents_i->doubleval < store_i->min) {
                    doubleval = store_i->min;
                } else if (contents_i->doubleval > store_i->max) {
                    doubleval = store_i->max;
                } else {
                    doubleval = contents_i->doubleval;
                    count++;
                }
            } else {
                doubleval = store_i->default_value;
            }
            void *dst = (uint8_t *)props + store_i->offset;
            memcpy(dst, &doubleval, store_i->size);
        }
        if (contents_i != contents.end()) {
            contents_i++;
        }
    }
    return count;
}

void TsubtitleFormat::Tssa::fontName(ffstring &arg)
{
    if (arg.size()) {
        memset(props.fontname, 0, sizeof(props.fontname));
        text<char_t>(arg.c_str(), (int)arg.size(), (char_t*)props.fontname, countof(props.fontname));
    } else {
        ff_strncpy(props.fontname, defprops.fontname, countof(props.fontname));
    }
}

template<int TSubtitleProps::*offset, int min, int max> void TsubtitleFormat::Tssa::intProp(ffstring &arg)
{
    if (props.transform.isTransform) {
        // Don't do anything until transform is complete
    } else {
        int enc;
        if (arg2int(arg, min, max, enc)) {
            props.*offset = enc;
        } else {
            props.*offset = defprops.*offset;
        }
    }
}

template<int min, int max> void TsubtitleFormat::Tssa::intPropAn(ffstring &arg)
{
    int enc;
    if (lineProps.isAlignment == false && arg2int(arg, min, max, enc)) {
        lineProps.alignment = TSubtitleProps::alignASS2SSA(enc);
        lineProps.isAlignment = true;
    }
}

template<int min, int max> void TsubtitleFormat::Tssa::intPropA(ffstring &arg)
{
    int enc;
    if (lineProps.isAlignment == false && arg2int(arg, min, max, enc)) {
        lineProps.alignment = enc;
        lineProps.isAlignment = true;
    }
}

template<int min, int max> void TsubtitleFormat::Tssa::intPropQ(ffstring &arg)
{
    int enc;
    if (arg2int(arg, min, max, enc)) {
        lineProps.wrapStyle = enc;
    }
}

template<double TSubtitleProps::*offset, int min, int max> void TsubtitleFormat::Tssa::doubleProp(ffstring &arg)
{
    if (props.transform.isTransform) {
        // Don't do anything until transform is complete
    } else {
        const wchar_t* buf = arg.c_str();
        wchar_t *bufend;
        double enc = strtod(buf, &bufend);
        if (buf != bufend && *bufend == '\0' && isIn(enc, (double)min, (double)max)) {
            props.*offset = enc;
        } else {
            props.*offset = defprops.*offset;
        }
    }
}

template<double TSubtitleProps::*offset, int min, int max> void TsubtitleFormat::Tssa::doublePropDiv100(ffstring &arg)
{
    if (props.transform.isTransform) {
        // Don't do anything until transform is complete
    } else {
        const wchar_t* buf = arg.c_str();
        wchar_t *bufend;
        double enc = strtod(buf, &bufend);
        if (buf != bufend && *bufend == '\0' && isIn(enc, (double)min, (double)max)) {
            props.*offset = enc / 100.0;
        } else {
            props.*offset = defprops.*offset;
        }
    }
}

void TsubtitleFormat::Tssa::clip(ffstring &arg)
{
    // (x1,y1,x2,y2,[t1[,t2]]) is expected.
    TstoreParams store;
    store.push_back(TstoreParam(offsetof(TSubtitleProps, clip.left),  0, INT_MAX,  defprops.clip.left,  sizeof(lineProps.clip.left),  true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, clip.top),  0, INT_MAX,  defprops.clip.top,  sizeof(lineProps.clip.top),  true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, clip.right), 0, INT_MAX,  defprops.clip.right, sizeof(lineProps.clip.right), true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, clip.bottom), 0, INT_MAX,  defprops.clip.bottom, sizeof(lineProps.clip.bottom), true));

    TparenthesesContents contents;
    parse_parentheses(contents, arg);
    if (store.writeProps(contents, &lineProps) >= 4) {
        lineProps.isClip = true;
    }
}

void TsubtitleFormat::Tssa::pos(ffstring &arg)
{
    if (lineProps.isMove) {
        return;
    }
    // (x1,y1) is expected.
    TstoreParams store;
    store.push_back(TstoreParam(offsetof(TSubtitleProps, pos.x), 0, INT_MAX, defprops.pos.x, sizeof(lineProps.pos.x), true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, pos.y), 0, INT_MAX, defprops.pos.y, sizeof(lineProps.pos.y), true));

    TparenthesesContents contents;
    parse_parentheses(contents, arg);
    if (store.writeProps(contents, &lineProps) == 2) {
        lineProps.pos2 = lineProps.pos;
        lineProps.isMove = true;
    }
}

void TsubtitleFormat::Tssa::move(ffstring &arg)
{
    if (lineProps.isMove) {
        return;
    }
    // (x1,y1,x2,y2,[t1[,t2]]) is expected.
    TstoreParams store;
    store.push_back(TstoreParam(offsetof(TSubtitleProps, pos.x),  0, INT_MAX,  defprops.pos.x,  sizeof(lineProps.pos.x),  true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, pos.y),  0, INT_MAX,  defprops.pos.y,  sizeof(lineProps.pos.y),  true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, pos2.x), 0, INT_MAX,  defprops.pos2.x, sizeof(lineProps.pos2.x), true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, pos2.y), 0, INT_MAX,  defprops.pos2.y, sizeof(lineProps.pos2.y), true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, moveT1), 0, UINT_MAX, 0,               sizeof(lineProps.moveT1), true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, moveT2), 0, UINT_MAX, 0,               sizeof(lineProps.moveT2), true));

    TparenthesesContents contents;
    parse_parentheses(contents, arg);
    if (store.writeProps(contents, &lineProps) >= 4) {
        lineProps.isMove = true;
    }
}

void TsubtitleFormat::Tssa::org(ffstring &arg)
{
    if (lineProps.isOrg) {
        return;
    }
    // (x1,y1) is expected.
    TstoreParams store;
    store.push_back(TstoreParam(offsetof(TSubtitleProps, org.x), 0, INT_MAX, defprops.org.x, sizeof(lineProps.org.x), true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, org.y), 0, INT_MAX, defprops.org.x, sizeof(lineProps.org.y), true));

    TparenthesesContents contents;
    parse_parentheses(contents, arg);
    if (store.writeProps(contents, &lineProps) == 2) {
        lineProps.isOrg = true;
    }
}

void TsubtitleFormat::Tssa::transform(ffstring &arg)
{
    // ([t1,t2,][accel,]<style modifiers>) is expected.
    TparenthesesContents contents;
    parse_parentheses(contents, arg);
    TstoreParams store;
    if (contents.size() != 2) {
        store.push_back(TstoreParam(offsetof(TSubtitleProps, transformT1),     0, UINT_MAX, 0, sizeof(props.transformT1),     true));
        store.push_back(TstoreParam(offsetof(TSubtitleProps, transformT2),     0, UINT_MAX, 0, sizeof(props.transformT2),     true));
        store.push_back(TstoreParam(offsetof(TSubtitleProps, transform.accel), 0, DBL_MAX,  1, sizeof(props.transform.accel), false));
    } else {
        props.transformT1 = props.transformT2 = 0;
        store.push_back(TstoreParam(offsetof(TSubtitleProps, transform.accel), 0, DBL_MAX,  1, sizeof(props.transform.accel), false));
    }

    store.writeProps(contents, &props);
    if (contents.size() > 0) {
        props.transform.isTransform = true;
        const wchar_t *s = L"{", *e = L"}", *temp = L"";
        wchar_t tokensT[500] = {0};
        unsigned int i;
        for (i = 0; i < contents.size(); i++) {
            if (!contents[i].ok) {
                temp = contents[i].str.c_str();
                wcscat(tokensT, s);
                wcscat(tokensT, (wchar_t *)temp);
                wcscat(tokensT, e);
            }
        }
        const wchar_t *tokens = tokensT, *tokens1 = tokens, *tokens2 = tokens, *end = strchr(tokens + 1, '}');
        Tssa::processTokens(tokens, tokens1, tokens2, end);
    }
}

void TsubtitleFormat::Tssa::karaoke_fixProperties()
{
    /* SRT subtitles do not define (most of the time) any secondary color
        So we have to put the secondary color into the primary color and vice versa for the karaoke word
        that will be added later */
    if ((sfmt == Tsubreader::SUB_SUBVIEWER || sfmt == Tsubreader::SUB_SUBVIEWER2)
            && props.karaokeMode != TSubtitleProps::KARAOKE_NONE && props.SecondaryColour == 0xffffff) {
        props.SecondaryColour = DEFAULT_SECONDARY_COLOR;
        props.isColor = true;
    }
}

void TsubtitleFormat::Tssa::karaoke_kf(ffstring &arg)
{
    intProp<&TSubtitleProps::tmpFadT1, 0, INT_MAX>(arg);
    REFERENCE_TIME t = (REFERENCE_TIME)props.tmpFadT1 * 100000;
    props.karaokeDuration = t;
    props.karaokeStart = lineProps.karaokeStart;
    lineProps.karaokeStart += t;
    props.karaokeFillStart = props.karaokeStart;
    props.karaokeFillEnd = props.karaokeStart + t;

    props.karaokeMode = TSubtitleProps::KARAOKE_kf;
    props.karaokeNewWord = true;
    karaoke_fixProperties();
}
void TsubtitleFormat::Tssa::karaoke_ko(ffstring &arg)
{
    intProp<&TSubtitleProps::tmpFadT1, 0, INT_MAX>(arg);
    REFERENCE_TIME t = (REFERENCE_TIME)props.tmpFadT1 * 100000;
    props.karaokeDuration = t;
    props.karaokeStart = lineProps.karaokeStart;
    lineProps.karaokeStart += t;
    props.karaokeFillStart = props.karaokeFillEnd = props.karaokeStart + t;

    props.karaokeMode = TSubtitleProps::KARAOKE_ko;
    props.karaokeNewWord = true;
    karaoke_fixProperties();
}
void TsubtitleFormat::Tssa::karaoke_k(ffstring &arg)
{
    intProp<&TSubtitleProps::tmpFadT1, 0, INT_MAX>(arg);
    REFERENCE_TIME t = (REFERENCE_TIME)props.tmpFadT1 * 100000;
    props.karaokeDuration = t;
    props.karaokeStart = lineProps.karaokeStart;
    lineProps.karaokeStart += t;
    props.karaokeFillStart = props.karaokeFillEnd = props.karaokeStart + t;

    props.karaokeMode = TSubtitleProps::KARAOKE_k;
    props.karaokeNewWord = true;
    karaoke_fixProperties();
}

void TsubtitleFormat::Tssa::fad(ffstring &arg)
{
    if (lineProps.isFad) {
        return;
    }

    TstoreParams store;
    store.push_back(TstoreParam(offsetof(TSubtitleProps, fadeT3), 0, LLONG_MAX / 10000, (double)defprops.fadeT3, sizeof(lineProps.fadeT3), true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, fadeT4), 0, LLONG_MAX / 10000, (double)defprops.fadeT4, sizeof(lineProps.fadeT4), true));

    TparenthesesContents contents;
    parse_parentheses(contents, arg);
    if (store.writeProps(contents, &lineProps) == 2) {
        lineProps.isFad  = true;
        lineProps.fadeA1 = 0;
        lineProps.fadeA2 = 255;
        lineProps.fadeA3 = 0;
        lineProps.fadeT1 = lineProps.tStart;
        lineProps.fadeT2 = lineProps.fadeT1 + (lineProps.fadeT3 * 10000);
        lineProps.fadeT3 = lineProps.tStop - (lineProps.fadeT4 * 10000);
        lineProps.fadeT4 = lineProps.tStop;
    }
}

void TsubtitleFormat::Tssa::fade(ffstring &arg)
{
    // \fade(<a1>, <a2>, <a3>, <t1>, <t2>, <t3>, <t4>)

    if (lineProps.isFad) {
        return;
    }

    TparenthesesContents contents;
    parse_parentheses(contents, arg);

    if (contents.size() == 2) {
        // Workaround for \fade(<t1>, <t2>), found in some scripts,
        // meaning the same as \fad(<t1>, <t2>)
        Tssa::fad(arg);
        return;
    }

    TstoreParams store;
    store.push_back(TstoreParam(offsetof(TSubtitleProps, fadeA1), 0, 255,             0,    sizeof(lineProps.fadeA1), true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, fadeA2), 0, 255,             255,  sizeof(lineProps.fadeA2), true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, fadeA3), 0, 255,             0,    sizeof(lineProps.fadeA3), true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, fadeT1), 0, LLONG_MAX / 10000, 0,    sizeof(lineProps.fadeT1), true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, fadeT2), 0, LLONG_MAX / 10000, 1000, sizeof(lineProps.fadeT2), true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, fadeT3), 0, INT_MAX, 2000, sizeof(lineProps.fadeT3), true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, fadeT4), 0, INT_MAX, 3000, sizeof(lineProps.fadeT4), true));

    if (store.writeProps(contents, &lineProps) == 7) {
        lineProps.fadeA1 = 256 - lineProps.fadeA1;
        lineProps.fadeA2 = 256 - lineProps.fadeA2;
        lineProps.fadeA3 = 256 - lineProps.fadeA3;

        lineProps.fadeT1 = lineProps.tStart + lineProps.fadeT1 * 10000;
        lineProps.fadeT2 = lineProps.tStart + lineProps.fadeT2 * 10000;
        lineProps.fadeT3 = lineProps.tStart + lineProps.fadeT3 * 10000;
        lineProps.fadeT4 = lineProps.tStart + lineProps.fadeT4 * 10000;
        lineProps.isFad = true;
    }
}

template<int TSubtitleProps::*offset1, int TSubtitleProps::*offset2, int min, int max> bool TsubtitleFormat::Tssa::intProp2(ffstring &arg)
{
    // (x,y) is expected.
    TstoreParams store;
    store.push_back(TstoreParam((int64_t TSubtitleProps::*)offset1, min, max, defprops.*offset1, sizeof(int), true));
    store.push_back(TstoreParam((int64_t TSubtitleProps::*)offset2, min, max, defprops.*offset2, sizeof(int), true));

    TparenthesesContents contents;
    parse_parentheses(contents, arg);
    return store.writeProps(contents, props) == 2;
}

bool TsubtitleFormat::Tssa::arg2int(const ffstring &arg, int min, int max, int &enc)
{
    const wchar_t* buf = arg.c_str();
    wchar_t *bufend;
    enc = strtol(buf, &bufend, 10);
    return (buf != bufend && *bufend == '\0' && isIn(enc, min, max));
}

bool TsubtitleFormat::Tssa::color2int(ffstring arg, int &intval)
{
    if (!arg.empty()) {
        if (arg.compare(0, 2, ffstring(_l("&H"))) == 0) {
            arg.erase(0, 2);
        }
        if (arg.compare(0, 1, ffstring(_l("H"))) == 0) { // "H&" fix typo for a certain script. For compatibility with vsfilter.
            arg.erase(0, 1);
        }
        if (arg.compare(0, 1, ffstring(_l("&"))) == 0) {
            arg.erase(0, 1);
        }
        wchar_t *endbuf;
        intval = strtol(arg.c_str(), &endbuf, 16);
        return (*endbuf == '&' || *endbuf == NULL);
    } else {
        return false;
    }
}

template<COLORREF TSubtitleProps::*offset> void TsubtitleFormat::Tssa::color(ffstring &arg)
{
    if (props.transform.isTransform) {
        // Don't do anything until transform is complete
    } else {
        int c;
        if (color2int(arg, c)) {
            props.*offset = c;
            props.isColor = true;
        } else {
            props.*offset = defprops.*offset;
            props.isColor = defprops.isColor;
        }
    }
}

template<int TSubtitleProps::*offset> void TsubtitleFormat::Tssa::alpha(ffstring &arg)
{
    if (props.transform.isTransform) {
        // Don't do anything until transform is complete
    } else {
        int a;
        if (color2int(arg, a)) {
            a &= 0xff;
            props.*offset = 256 - a;
            props.isColor = true;
        } else {
            props.*offset = defprops.*offset;
            props.isColor = defprops.isColor;
        }
    }
}

void TsubtitleFormat::Tssa::alphaAll(ffstring &arg)
{
    int a;
    if (props.transform.isTransform && color2int(arg, a)) {
        props.transform.isAlpha = true;
        props.transform.alpha = a;
        props.transform.alphaT1 = props.transformT1 ? props.tStart + (props.transformT1 * 10000) : props.tStart;
        props.transform.alphaT2 = props.transformT2 ? props.tStart + (props.transformT2 * 10000) : props.tStop;
    } else {
        if (color2int(arg, a)) {
            a &= 0xff;
            props.colorA = 256 - a;
            props.SecondaryColourA = 256 - a;
            props.TertiaryColourA = 256 - a;
            props.OutlineColourA = 256 - a;
            props.ShadowColourA = 256 - a;
            props.isColor = true;
        } else {
            props.colorA = defprops.colorA;
            props.SecondaryColourA = defprops.SecondaryColourA;
            props.TertiaryColourA = defprops.TertiaryColourA;
            props.OutlineColourA = defprops.OutlineColourA;
            props.ShadowColourA = defprops.ShadowColourA;
            props.isColor = defprops.isColor;
        }
    }
}

template<bool TSubtitleProps::*offset> void TsubtitleFormat::Tssa::boolProp(ffstring &arg)
{
    if (arg.size() && arg[0] == '1') {
        props.*offset = true;
    } else if (arg.size() && arg[0] == '0') {
        props.*offset = false;
    } else {
        props.*offset = defprops.*offset;
    }
}

void TsubtitleFormat::Tssa::reset(ffstring &arg)
{
    if (arg.size()) {
        const TSubtitleProps* p = styles.getProps(arg);
        if (p) {
            int lineID = props.lineID;
            props = *p;
            props.lineID = lineID;
        }
    } else {
        props = defprops;
    }
}

bool TsubtitleFormat::Tssa::processTokenI(const wchar_t* &l2, const wchar_t *tok, TssaAction action, Tstr_cmp_func str_cmp_func)
{
    size_t toklen = strlen(tok);
    if (str_cmp_func(l2, tok, toklen) == 0) {
        const wchar_t *end1 = ((strchr(l2 + 2, '\\') > strchr(l2 + 2, '(')) && (strchr(l2 + 2, '\\') < strchr(l2 + 2, ')'))) ? strchr(l2 + 2, ')') + 1 : strchr(l2 + 2, '\\');
        const wchar_t *end2 = strchr(l2, '}');
        const wchar_t *end = (end1 && end1 < end2) ? end1 : end2;
        if (end) {
            const wchar_t *start = l2 + toklen;
            ffstring arg(start, end - start);
            if (action) {
                (this->*action)(arg);
            }
            l2 = (end1 && end1 < end2) ? end1 : end2 + 1;
        }
        return true;
    } else {
        return false;
    }
}

// case sensitive version
bool TsubtitleFormat::Tssa::processTokenC(const wchar_t* &l2, const wchar_t *tok, TssaAction action)
{
    return processTokenI(l2, tok, action, strncmp);
}

bool TsubtitleFormat::Tssa::processToken(const wchar_t* &l2, const wchar_t *tok, TssaAction action)
{
    return processTokenI(l2, tok, action, _strnicmp);
}

void TsubtitleFormat::Tssa::processTokens(const wchar_t *l, const wchar_t* &l1, const wchar_t* &l2, const wchar_t *end)
{
    const wchar_t *l3 = l2 + 1;
    words.add(l, l1, l2, props, end - l2 + 1);
    while (l3 < end) {
        if (
            !processToken(l3, L"\\1a", &Tssa::template alpha<&TSubtitleProps::colorA>) &&
            !processToken(l3, L"\\2a", &Tssa::template alpha<&TSubtitleProps::SecondaryColourA>) &&
            !processToken(l3, L"\\3a", &Tssa::template alpha<&TSubtitleProps::OutlineColourA>) &&
            !processToken(l3, L"\\4a", &Tssa::template alpha<&TSubtitleProps::ShadowColourA>) &&
            !processToken(l3, L"\\1c", &Tssa::template color<&TSubtitleProps::color>) &&
            !processToken(l3, L"\\2c", &Tssa::template color<&TSubtitleProps::SecondaryColour>) &&
            !processToken(l3, L"\\3c", &Tssa::template color<&TSubtitleProps::OutlineColour>) &&
            !processToken(l3, L"\\4c", &Tssa::template color<&TSubtitleProps::ShadowColour>) &&
            !processToken(l3, L"\\alpha", &Tssa::alphaAll) &&
            !processToken(l3, L"\\an", &Tssa::template intPropAn<1, 9>) &&
            !processToken(l3, L"\\a", &Tssa::template intPropA<1, 11>) &&
            !processToken(l3, L"\\blur", &Tssa::template doubleProp<&TSubtitleProps::gauss, 0, 100>) &&
            !processToken(l3, L"\\bord", &Tssa::template doubleProp<&TSubtitleProps::outlineWidth, 0, 100>) &&
            !processToken(l3, L"\\be", &Tssa::template intProp<&TSubtitleProps::blur_be, 0, 1000>) &&
            !processToken(l3, L"\\b", &Tssa::template intProp<&TSubtitleProps::bold, 0, 1>) &&
            !processToken(l3, L"\\clip", &Tssa::clip) &&
            !processToken(l3, L"\\c", &Tssa::template color<&TSubtitleProps::color>) &&
            !processToken(l3, L"\\fn", &Tssa::fontName) &&
            !processToken(l3, L"\\fscx", &Tssa::template doublePropDiv100<&TSubtitleProps::scaleX, 1, 1000>) &&
            !processToken(l3, L"\\fscy", &Tssa::template doublePropDiv100<&TSubtitleProps::scaleY, 1, 1000>) &&
            !processToken(l3, L"\\fsp", &Tssa::template doubleProp < &TSubtitleProps::spacing, INT_MIN + 1, INT_MAX >) &&
            !processToken(l3, L"\\fs", &Tssa::template doubleProp<&TSubtitleProps::size, 1, 1000>) &&
            !processToken(l3, L"\\frx", &Tssa::template doubleProp<&TSubtitleProps::angleX, 0, 10000>) &&
            !processToken(l3, L"\\fry", &Tssa::template doubleProp<&TSubtitleProps::angleY, 0, 10000>) &&
            !processToken(l3, L"\\frz", &Tssa::template doubleProp<&TSubtitleProps::angleZ, 0, 10000>) &&
            !processToken(l3, L"\\fr", &Tssa::template doubleProp<&TSubtitleProps::angleZ, 0, 10000>) &&
            !processToken(l3, L"\\fe", &Tssa::template intProp<&TSubtitleProps::encoding, 0, 255>) &&
            !processToken(l3, L"\\fade", &Tssa::fade) &&
            !processToken(l3, L"\\fad", &Tssa::fad) &&
            !processToken(l3, L"\\i", &Tssa::template boolProp<&TSubtitleProps::italic>) &&
            !processToken(l3, L"\\kf", &Tssa::karaoke_kf) &&
            !processToken(l3, L"\\ko", &Tssa::karaoke_ko) &&
            !processTokenC(l3, L"\\K", &Tssa::karaoke_kf) &&
            !processTokenC(l3, L"\\k", &Tssa::karaoke_k) &&
            !processToken(l3, L"\\move", &Tssa::move) &&
            !processToken(l3, L"\\org", &Tssa::org) &&
            !processToken(l3, L"\\pos", &Tssa::pos) &&
            !processToken(l3, L"\\p", &Tssa::template intProp<&TSubtitleProps::polygon, 0, 255>) &&
            !processToken(l3, L"\\q", &Tssa::template intPropQ<0, 3>) &&
            !processToken(l3, L"\\r", &Tssa::reset) &&
            !processToken(l3, L"\\shad", &Tssa::template doubleProp<&TSubtitleProps::shadowDepth, 0, 30>) &&
            !processToken(l3, L"\\s", &Tssa::template boolProp<&TSubtitleProps::strikeout>) &&
            !processToken(l3, L"\\t", &Tssa::transform) &&
            !processToken(l3, L"\\u", &Tssa::template boolProp<&TSubtitleProps::underline>)
        ) {
            l3++;
        }
    }
}

const TSubtitleProps& TsubtitleFormat::processSSA(Twords &words, const TsubtitleLine &line, int sfmt, TsubtitleText &parent)
{
    if (line.empty()) {
        return lineProps;
    }
    const wchar_t *l = line[0];
    const wchar_t *l1 = l, *l2 = l;
    Tssa ssa(props, lineProps, parent.defProps, parent.getStyles(), words, sfmt);
    while (*l2) {
        if (l2[0] == '{') {
            if (const wchar_t *end = strchr(l2 + 1, '}')) {
                ssa.processTokens(l, l1 , l2, end);
                l2 = end + 1;
                continue;
            }
            l2++;
        }
        // Process HTML tags in SSA subs when extended tags option is checked
        else if (parent.defProps.extendedTags) { // Add HTML support within SSA
            processHTMLTags(words, l, l1, l2);
        } else {
            l2++;
        }
    }

    words.add(l, l1, l2, props, 0);
    return lineProps;
}

void TsubtitleFormat::processMicroDVD(TsubtitleText &parent, std::vector< TsubtitleLine >::iterator it)
{
    if (it->empty()) {
        return;
    }
    const wchar_t *line0 = (*it)[0], *line = line0;
    while (*line)
        if (line[0] == '}' || line[0] == ' ') {
            line++;
        } else if (_strnicmp(line, L"{y:", 3) == 0) {
            const wchar_t *end = strchr(line + 3, '}');
            if (end == NULL) {
                break;
            }
            bool all = !!iswupper(line[1]);
            if (std::find_if(line + 3, end, Tncasecmp < 'i' > ()) != end) {
                parent.propagateProps(all ? parent.begin() : it, &TSubtitleProps::italic   , true, all ? parent.end() : it + 1);
            }
            if (std::find_if(line + 3, end, Tncasecmp < 'b' > ()) != end) {
                parent.propagateProps(all ? parent.begin() : it, &TSubtitleProps::bold     , 1, all ? parent.end() : it + 1);
            }
            if (std::find_if(line + 3, end, Tncasecmp < 'u' > ()) != end) {
                parent.propagateProps(all ? parent.begin() : it, &TSubtitleProps::underline, true, all ? parent.end() : it + 1);
            }
            if (std::find_if(line + 3, end, Tncasecmp < 's' > ()) != end) {
                parent.propagateProps(all ? parent.begin() : it, &TSubtitleProps::strikeout, true, all ? parent.end() : it + 1);
            }
            line = end + 1;
        } else if (_strnicmp(line, L"{s:", 3) == 0) {
            int size;
            if (swscanf(line, L"{s:%i}", &size) || swscanf(line, L"{S:%i}", &size)) {
                parent.propagateProps(iswupper(line[1]) ? parent.begin() : it, &TSubtitleProps::size, double(size), iswupper(line[1]) ? parent.end() : it + 1);
                const wchar_t *r = strchr(line, '}');
                if (r) {
                    line = r + 1;
                }
            }
        } else if (_strnicmp(line, L"{c:$", 4) == 0) {
            COLORREF color;
            if (swscanf(line, L"{c:$%x}", &color) || swscanf(line, L"{C:$%x}", &color)) {
                parent.propagateProps(iswupper(line[1]) ? parent.begin() : it, &TSubtitleProps::color, color, iswupper(line[1]) ? parent.end() : it + 1);
                const wchar_t *r = strchr(line, '}');
                if (r) {
                    parent.propagateProps(iswupper(line[1]) ? parent.begin() : it, &TSubtitleProps::isColor, true, iswupper(line[1]) ? parent.end() : it + 1);
                    line = r + 1;
                }
            }
        } else {
            break;
        }
    (*it)[0].eraseLeft(line - line0);
}

void TsubtitleFormat::processMPL2(TsubtitleLine &line)
{
    if (line.empty() || !line[0]) {
        return;
    }
    if (line[0][0] == '/') {
        foreach(TsubtitleWord & word, line)
        word.props.italic = true;
        line[0].eraseLeft(1);
    }
}
