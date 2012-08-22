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
#include "Tsubreader.h"
#include "TsubtitlesSettings.h"
#include "Tconfig.h"

Tsubreader::~Tsubreader()
{
    clear();
}
void Tsubreader::clear(void)
{
    foreach(Tsubtitle* &subtitle, *this) {
        delete subtitle;
        subtitle = NULL;
    }
    std::vector<value_type>::clear();
    IsProcessOverlapDone = false;
}

int Tsubreader::sub_autodetect(Tstream &fd, const Tconfig *config)
{
    int j = 0;
    static const int LINE_LEN = 1000;
    int format = SUB_INVALID;
    DPRINTF(_l("Tsubreader::sub_autodetect"));
    while (j < 100) {
        j++;
        wchar_t line[LINE_LEN + 1];
        if (!fd.fgets(line, LINE_LEN)) {
            format = SUB_INVALID;
            break;
        }
        int i;
        wchar_t p;
        if (swscanf(line, L"{%d}{%d}", &i, &i) == 2) {
            format = SUB_MICRODVD;
            break;
        }
        if (swscanf(line, L"{%d}{}", &i) == 1) {
            format = SUB_MICRODVD;
            break;
        }
        if (swscanf(line, L"[%d][%d]", &i, &i) == 2) {
            format = SUB_MPL2 | SUB_USESTIME;
            break;
        }
        if (swscanf(line, L"%d:%d:%d.%d,%d:%d:%d.%d",     &i, &i, &i, &i, &i, &i, &i, &i) == 8) {
            format = SUB_SUBRIP | SUB_USESTIME;
            break;
        }
        if (swscanf(line, L"%d:%d:%d%[,.:]%d --> %d:%d:%d%[,.:]%d", &i, &i, &i, (wchar_t *)&i, &i, &i, &i, &i, (wchar_t *)&i, &i) == 10) {
            format = SUB_SUBVIEWER | SUB_USESTIME;
            break;
        }
        if (swscanf(line, L"{T %d:%d:%d:%d", &i, &i, &i, &i) == 4) {
            format = SUB_SUBVIEWER2 | SUB_USESTIME;
            break;
        }
        if (strstr(line, L"<SAMI>")) {
            format = SUB_SAMI | SUB_USESTIME;
            break;
        }
        if (swscanf(line, L"%d:%d:%d:",     &i, &i, &i) == 3) { //also true for "%d:%d:%d ". swscanf ignore last char.
            format = SUB_VPLAYER | SUB_USESTIME;
            break;
        }
        //TODO: just checking if first line of sub starts with "<" is WAY
        // too weak test for RT
        // Please someone who knows the format of RT... FIX IT!!!
        // It may conflict with other sub formats in the future (actually it doesn't)
        //should be better now
        if (stristr(line, L"time begin") != NULL) {
            format = SUB_RT | SUB_USESTIME;
            break;
        }

        if (!memcmp(line, L"Dialogue: Marked", 16 * 2)) {
            format = SUB_SSA | SUB_USESTIME;
            break;
        }
        if (!memcmp(line, L"Dialogue: ", 10 * 2)) {
            format = SUB_SSA | SUB_USESTIME;
            break;
        }
        if (!memcmp(line, L"# VobSub index file", 19*2)) {
            format=SUB_VOBSUB|SUB_USESTIME;
            break;
        }
        if (swscanf(line, L"%d,%d,\"%c", &i, &i, (wchar_t *) &i) == 3) {
            format = SUB_DUNNOWHAT;
            break;
        }
        if (swscanf(line, L"FORMAT=%d", &i) == 1) {
            format = SUB_MPSUB;
            break;
        }
        if (swscanf(line, L"FORMAT=TIM%c", &p) == 1 && p == 'E') {
            format = SUB_MPSUB | SUB_USESTIME;
            break;
        }
        if (strstr(line, L"-->>")) {
            format = SUB_AQTITLE | SUB_USESTIME;
            break;
        }
        if (swscanf(line, L"[%d:%d:%d]", &i, &i, &i) == 3) {
            format = SUB_SUBRIP09 | SUB_USESTIME;
            break;
        }
        //TODO : external bluray subtitles support. The test should also be improved
        if (j == 1 && !memcmp(line, L"PG", 2 * 2)) {
            // Keep file opened because it will take too much memory to load all the subtitles
            format = SUB_PGS | SUB_USESTIME | SUB_KEEP_FILE_OPENED;
            break;
        }
    }
    if (format != SUB_INVALID) {
        setSubEnc(format, fd);
    }
    return format;  // too many bad lines
}

bool Tsubreader::subComp(const Tsubtitle *s1, const Tsubtitle *s2)
{
    return s1->start < s2->start;
}
void Tsubreader::timesort(void)
{
    std::sort(begin(), end(), subComp);
}

void Tsubreader::processDuration(const TsubtitlesSettings *cfg)
{
    timesort();
    if (cfg->isMinDuration) {
        foreach(Tsubtitle * subtitle, *this) {
            REFERENCE_TIME minduration = 0;
            switch (cfg->minDurationType) {
                case 0:
                    minduration = cfg->minDurationSubtitle;
                    break;
                case 1:
                    minduration = cfg->minDurationLine * subtitle->numlines();
                    break;
                case 2:
                    minduration  = cfg->minDurationChar * subtitle->numchars();
                    break;
            }
            minduration *= REF_SECOND_MULT / 1000;
            minduration = std::max(REFERENCE_TIME(1), minduration);
            if (subtitle->stop - subtitle->start < minduration) {
                subtitle->stop = subtitle->start + minduration;
            }
        }
    }
}

void Tsubreader::adjust_subs_time(float subtime)
{
    if (empty()) {
        return;
    }
    timesort();

    int n = 0, m = 0;
    iterator sub = begin(), nextsub;
    int i = (int)size();
    REFERENCE_TIME subfms = REFERENCE_TIME(subtime) * REF_SECOND_MULT;
    for (;;) {
        if ((*sub)->stop <= (*sub)->start) {
            (*sub)->stop = (*sub)->start + subfms;
            m++;
            n++;
        }
        if (!--i) {
            break;
        }
        nextsub = sub + 1;
        if ((*sub)->stop >= (*nextsub)->start) {
            (*sub)->stop = (*nextsub)->start - 1;
            if ((*sub)->stop - (*sub)->start > subfms) {
                (*sub)->stop = (*sub)->start + subfms;
            }
            if (!m) {
                n++;
            }
        }
        sub = nextsub;
        m = 0;
    }
}

Tstream::ENCODING Tsubreader::getSubEnc(int format)
{
    format &= SUB_ENC_MASK;
    switch (format) {
        case SUB_ENC_UTF8:
            return Tstream::ENC_UTF8;
        case SUB_ENC_UNILE:
            return Tstream::ENC_LE16;
        case SUB_ENC_UNIBE:
            return Tstream::ENC_BE16;
        default:
            return Tstream::ENC_ASCII;
    }
}
void Tsubreader::setSubEnc(int &format, const Tstream &fs)
{
    switch (fs.encoding) {
        case Tstream::ENC_BE16:
            format |= SUB_ENC_UNIBE;
            break;
        case Tstream::ENC_LE16:
            format |= SUB_ENC_UNILE;
            break;
        case Tstream::ENC_UTF8:
            format |= SUB_ENC_UTF8 ;
            break;
    }
}

void Tsubreader::dropRendered()
{
    foreach(Tsubtitle * subtitle, *this)
    subtitle->dropRenderedLines();
}

void Tsubreader::onSeek()
{
    dropRendered();
    // Why dropping text ? This clears all the parsed text subtitles
    //processedSubtitleTexts.clear();
}

Tsubtitle* Tsubreader::operator[](size_t pos) const
{
    if (empty()) {
        return NULL;
    }
    if (at(0)->isText()) {
        if (processedSubtitleTexts.size() <= pos) {
            return NULL;
        }
        return (Tsubtitle *)&processedSubtitleTexts[pos];
    } else {
        if (size() <= pos) {
            return NULL;
        }
        return at(pos);
    }
}

size_t Tsubreader::count() const
{
    if (empty()) {
        return 0;
    }
    if (at(0)->isText()) {
        return processedSubtitleTexts.size();
    } else {
        return size();
    }
}

void Tsubreader::processOverlap(void)
{
    if (empty()) {
        return;
    }
    if (!at(0)->isText()) {
        IsProcessOverlapDone = true;
        return;
    }
    processedSubtitleTexts.clear();
    static const int SUB_MAX_TEXT = INT_MAX / 2;
    int sub_orig = (int)size();
    int n_first = (int)size();
    int sub_num = 0;
    std::vector<Tsubtitle*> newsubs;
    for (int sub_first = 0; sub_first < n_first; ++sub_first) {
        REFERENCE_TIME global_start = at(sub_first)->start, global_end = at(sub_first)->stop, local_start, local_end;
        int lines_to_add = (int)at(sub_first)->numlines(), sub_to_add = 0;
        int **placeholder = NULL, higher_line = 0, counter, start_block_sub = sub_num;
        char real_block = 1;

        // here we find the number of subtitles inside the 'block'
        // and its span interval. this works well only with sorted
        // subtitles
        while ((sub_first + sub_to_add + 1 < n_first) && (at(sub_first + sub_to_add + 1)->start < global_end)) {
            ++sub_to_add;
            lines_to_add += (int)at(sub_first + sub_to_add)->numlines();
            if (at(sub_first + sub_to_add)->start < global_start) {
                global_start = at(sub_first + sub_to_add)->start;
            }
            if (at(sub_first + sub_to_add)->stop > global_end) {
                global_end = at(sub_first + sub_to_add)->stop;
            }
        }

        // we need a structure to keep trace of the screen lines
        // used by the subs, a 'placeholder'
        counter = 2 * sub_to_add + 1;  // the maximum number of subs derived from a block of sub_to_add+1 subs
        placeholder = (int **) malloc(sizeof(int *) * counter);
        for (int i = 0; i < counter; ++i) {
            placeholder[i] = (int *) malloc(sizeof(int) * lines_to_add);
            for (int j = 0; j < lines_to_add; ++j) {
                placeholder[i][j] = -1;
            }
        }

        counter = 0;
        local_end = global_start - 1;
        do {
            // here we find the beginning and the end of a new subtitle in the block
            local_start = local_end + 1;
            local_end   = global_end;
            for (int j = 0; j <= sub_to_add; ++j) {
                if ((at(sub_first + j)->start - 1 > local_start) && (at(sub_first + j)->start - 1 < local_end)) {
                    local_end = at(sub_first + j)->start - 1;
                } else if ((at(sub_first + j)->stop > local_start) && (at(sub_first + j)->stop < local_end)) {
                    local_end = at(sub_first + j)->stop;
                }
            }

            // here we allocate the screen lines to subs we must
            // display in current local_start-local_end interval.
            // if the subs were yet presents in the previous interval
            // they keep the same lines, otherwise they get unused lines
            for (int j = 0; j <= sub_to_add; ++j) {
                if ((at(sub_first + j)->start <= local_end) && (at(sub_first + j)->stop > local_start)) {
                    unsigned long sub_lines = (unsigned long)at(sub_first + j)->numlines(), fragment_length = lines_to_add + 1, tmp = 0;
                    char boolean = 0;
                    int fragment_position = -1;

                    // if this is not the first new sub of the block
                    // we find if this sub was present in the previous
                    // new sub
                    if (counter)
                        for (int i = 0; i < lines_to_add; ++i)
                            if (placeholder[counter - 1][i] == sub_first + j) {
                                placeholder[counter][i] = sub_first + j;
                                boolean = 1;
                            }
                    if (boolean) {
                        continue;
                    }
                    // we are looking for the shortest among all groups of
                    // sequential blank lines whose length is greater than or
                    // equal to sub_lines. we store in fragment_position the
                    // position of the shortest group, in fragment_length its
                    // length, and in tmp the length of the group currently
                    // examined
                    int i;
                    for (i = 0; i < lines_to_add; ++i) {
                        if (placeholder[counter][i] == -1) {
                            ++tmp; // placeholder[counter][i] is part of the current group of blank lines
                        } else {
                            if (tmp == sub_lines) {
                                // current group's size fits exactly the one we
                                // need, so we stop looking
                                fragment_position = i - tmp;
                                tmp = 0;
                                break;
                            }
                            if ((tmp) && (tmp > sub_lines) && (tmp < fragment_length)) {
                                // current group is the best we found till here,
                                // but is still bigger than the one we are looking
                                // for, so we keep on looking
                                fragment_length = tmp;
                                fragment_position = i - tmp;
                                tmp = 0;
                            } else {
                                tmp = 0; // current group doesn't fit at all, so we forget it
                            }
                        }
                    }
                    if (tmp) {
                        if ((tmp >= sub_lines) && (tmp < fragment_length)) { // last screen line is blank, a group ends with it
                            fragment_position = i - tmp;
                        }
                    }
                    if (fragment_position == -1) {
                        // it was not possible to find free screen line(s) for a subtitle,
                        // usually this means a bug in the code; however we do not overlap
                        //mp_msg(MSGT_SUBREADER, MSGL_WARN, "SUB: we could not find a suitable position for an overlapping subtitle\n");
                        higher_line = SUB_MAX_TEXT + 1;
                        break;
                    } else
                        for (tmp = 0; tmp < sub_lines; ++tmp) {
                            placeholder[counter][fragment_position + tmp] = sub_first + j;
                        }
                }
            }
            for (int j = higher_line + 1; j < lines_to_add; ++j) {
                if (placeholder[counter][j] != -1) {
                    higher_line = j;
                } else {
                    break;
                }
            }

            // we read the placeholder structure and create the new subs.
            TsubtitleTexts texts;
            texts.start = local_start;
            texts.stop = local_end;
            for (int i = 0, j = 0; j < lines_to_add ; ++j) {
                if (placeholder[counter][j] != -1) {
                    texts.push_back((TsubtitleText *)at(placeholder[counter][j]));
                    j += (int)at(placeholder[counter][j])->numlines() - 1;
                }
            }
            processedSubtitleTexts.push_back(texts);

            ++sub_num;
            ++counter;
        } while (local_end < global_end);
        counter = 2 * sub_to_add + 1;
        for (int i = 0; i < counter; ++i) {
            free(placeholder[i]);
        }
        free(placeholder);
        sub_first += sub_to_add;
    }
    IsProcessOverlapDone = true;
}

size_t Tsubreader::getMemorySize() const
{
    size_t memSize = 0;
    foreach(const Tsubtitle * subtitle, *this)
    memSize += subtitle->getRenderedMemorySize();
    return memSize;
}
