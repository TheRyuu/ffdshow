/*
 * Copyright (c) 2002-2006 Milan Cutka
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
#include "CsubtitlesText.h"
#include "TsubtitleText.h"
#include "TffdshowPageDec.h"
#include "TsubtitlesSettings.h"

void TsubtitlesTextPage::init()
{
    tbrSetRange(IDC_TBR_SUB_LINESPACING, 0, 200, 10);
    cbxSetDroppedWidth(IDC_CBX_SUB_WORDWRAP, 250);
}

void TsubtitlesTextPage::cfg2dlg()
{
    split2dlg();
    linespacing2dlg();
    min2dlg();
    memory2dlg();
    addHint(IDC_ED_SUB_MEMORY, L"ffdshow rasterize the font in the background and store the images in this buffer.\nIf the memory capacity is small, 10MB may help.");
}

void TsubtitlesTextPage::split2dlg()
{
    int is = cfgGet(IDFF_fontSplitting);
    setCheck(IDC_CHB_SUB_SPLIT, is);
    SetDlgItemInt(m_hwnd, IDC_ED_SUB_SPLIT_BORDER, cfgGet(IDFF_subSplitBorder), FALSE);
    static const int idBorders[] = {IDC_LBL_SUB_SPLIT_BORDER, IDC_ED_SUB_SPLIT_BORDER, IDC_LBL_SUB_SPLIT_BORDER2, 0};
    enable(is, idBorders);
    cbxSetCurSel(IDC_CBX_SUB_WORDWRAP, cfgGet(IDFF_subWordWrap));
    enable(is, IDC_CBX_SUB_WORDWRAP);
}

void TsubtitlesTextPage::linespacing2dlg()
{
    int ls = cfgGet(IDFF_subLinespacing);
    tbrSet(IDC_TBR_SUB_LINESPACING, ls);
    setText(IDC_LBL_SUB_LINESPACING, _l("%s %i%%"), _(IDC_LBL_SUB_LINESPACING), ls);
}

void TsubtitlesTextPage::memory2dlg()
{
    // Removed because the high quality border isn't too slow for most users.
    // int ishq = cfgGet(IDFF_fontHqBorder);
    // setCheck(IDC_CHB_SUB_HQBORDER,ishq);

    SetDlgItemInt(m_hwnd, IDC_ED_SUB_MEMORY, cfgGet(IDFF_fontMemory), FALSE);
}

void TsubtitlesTextPage::min2dlg()
{
    int ismin = cfgGet(IDFF_subIsMinDuration);
    setCheck(IDC_CHB_SUB_MINDURATION, ismin);
    static const int mins[] = {IDC_CBX_SUB_MINDURATION, IDC_LBL_SUB_MINDURATION, IDC_ED_SUB_MINDURATION, IDC_LBL_SUB_MINDURATION2, 0};
    enable(ismin, mins);
    int mintype = cfgGet(IDFF_subMinDurationType);
    cbxSetCurSel(IDC_CBX_SUB_MINDURATION, mintype);
    switch (mintype) {
        case 0:
            SetDlgItemInt(m_hwnd, IDC_ED_SUB_MINDURATION, cfgGet(IDFF_subMinDurationSub), FALSE);
            break;
        case 1:
            SetDlgItemInt(m_hwnd, IDC_ED_SUB_MINDURATION, cfgGet(IDFF_subMinDurationLine), FALSE);
            break;
        case 2:
            SetDlgItemInt(m_hwnd, IDC_ED_SUB_MINDURATION, cfgGet(IDFF_subMinDurationChar), FALSE);
            break;
    }
}

INT_PTR TsubtitlesTextPage::msgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_ED_SUB_MINDURATION:
                    if (HIWORD(wParam) == EN_CHANGE && !isSetWindowText) {
                        HWND hed = GetDlgItem(m_hwnd, LOWORD(wParam));
                        if (hed != GetFocus()) {
                            return FALSE;
                        }
                        repaint(hed);
                        switch (LOWORD(wParam)) {
                            case IDC_ED_SUB_MINDURATION: {
                                static const int idffmins[] = {IDFF_subMinDurationSub, IDFF_subMinDurationLine, IDFF_subMinDurationChar};
                                eval(hed, 1, 3600000, idffmins[cbxGetCurSel(IDC_CBX_SUB_MINDURATION)]);
                                break;
                            }
                        }
                        return TRUE;
                    }
                    break;
            }
            break;
        case WM_CTLCOLOREDIT: {
            HWND hwnd = HWND(lParam);
            bool ok;
            switch (getId(hwnd)) {
                case IDC_ED_SUB_MINDURATION:
                    ok = eval(hwnd, 1, 3600000);
                    break;
                default:
                    return FALSE;
            }
            if (!ok) {
                HDC dc = HDC(wParam);
                SetBkColor(dc, RGB(255, 0, 0));
                return INT_PTR(getRed());
            } else {
                return FALSE;
            }
        }
    }
    return TconfPageDecVideo::msgProc(uMsg, wParam, lParam);
}

void TsubtitlesTextPage::translate()
{
    TconfPageBase::translate();

    cbxTranslate(IDC_CBX_SUB_MINDURATION, TsubtitlesSettings::durations);
    cbxTranslate(IDC_CBX_SUB_WORDWRAP, TsubtitlesSettings::wordWraps);
}

TsubtitlesTextPage::TsubtitlesTextPage(TffdshowPageDec *Iparent, const TfilterIDFF *idff): TconfPageDecVideo(Iparent, idff, 5)
{
    dialogId = IDD_SUBTITLESTEXT;
    static const TbindCheckbox<TsubtitlesTextPage> chb[] = {
        IDC_CHB_SUB_SPLIT, IDFF_fontSplitting, &TsubtitlesTextPage::split2dlg,
        IDC_CHB_SUB_MINDURATION, IDFF_subIsMinDuration, &TsubtitlesTextPage::min2dlg,
        //IDC_CHB_SUB_HQBORDER,IDFF_fontHqBorder,&TsubtitlesTextPage::memory2dlg,
        0, NULL, NULL
    };
    bindCheckboxes(chb);
    static const TbindTrackbar<TsubtitlesTextPage> htbr[] = {
        IDC_TBR_SUB_LINESPACING, IDFF_subLinespacing, &TsubtitlesTextPage::linespacing2dlg,
        0, 0, NULL
    };
    bindHtracks(htbr);
    static const TbindCombobox<TsubtitlesTextPage> cbx[] = {
        IDC_CBX_SUB_MINDURATION, IDFF_subMinDurationType, BINDCBX_SEL, &TsubtitlesTextPage::min2dlg,
        IDC_CBX_SUB_WORDWRAP, IDFF_subWordWrap, BINDCBX_SEL, &TsubtitlesTextPage::split2dlg,
        0
    };
    bindComboboxes(cbx);
    static const TbindEditInt<TsubtitlesTextPage> edInt[] = {
        IDC_ED_SUB_SPLIT_BORDER, 0, 4096, IDFF_subSplitBorder, NULL,
        IDC_ED_SUB_MEMORY, 0, 256, IDFF_fontMemory,
        0
    };
    bindEditInts(edInt);
}
