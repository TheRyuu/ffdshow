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
#include "CsubtitlesPos.h"
#include "TffdshowPageDec.h"
#include "TsubtitlesSettings.h"

void TsubtitlesPosPage::init(void)
{
    tbrSetRange(IDC_TBR_SUB_POSX, 0, 100, 10);
    tbrSetRange(IDC_TBR_SUB_POSY, 0, 100, 10);
    tbrSetRange(IDC_TBR_SUB_STEREOSCOPICPAR, -100, 100);
    setCheck(IDC_CHB_SUB_SSA_OVERRIDE_POSITION, deci->getParam2(IDFF_subSSAOverridePlacement) == 1);
    setCheck(IDC_CHB_SUB_SSA_KEEP_TEXT_INSIDE , deci->getParam2(IDFF_subSSAMaintainInside) == 1);
    setCheck(IDC_CHB_SUB_SSA_USE_INPUT_DIMENSIONS , deci->getParam2(IDFF_subSSAUseMovieDimensions) == 1);
    addHint(IDC_CHB_SUB_SSA_OVERRIDE_POSITION, _l("Overrides SSA/ASS default positioning with the values set in the horizontal and vertical position sliders. Subtitles with \\pos(x,y) tags are not affected by this setting"));
    addHint(IDC_CHB_SUB_SSA_KEEP_TEXT_INSIDE, _l("If some text comes partially or totally out of the screen, then correct its position so that the whole text is visible"));
    addHint(IDC_CHB_SUB_SSA_USE_INPUT_DIMENSIONS, _l("Use movie dimensions to calculate coordinates instead of the SSA/ASS resolution references present in the script. This information is not always present or accurate"));
}

void TsubtitlesPosPage::cfg2dlg(void)
{
    char_t s[260];
    int x;

    x = cfgGet(IDFF_subPosX);
    TsubtitlesSettings::getPosHoriz(x, s, this, IDC_LBL_SUB_POSX, countof(s));
    setDlgItemText(m_hwnd, IDC_LBL_SUB_POSX, s);
    tbrSet(IDC_TBR_SUB_POSX, x);

    x = cfgGet(IDFF_subPosY);
    TsubtitlesSettings::getPosVert(x, s, this, IDC_LBL_SUB_POSY, countof(s));
    setDlgItemText(m_hwnd, IDC_LBL_SUB_POSY, s);
    tbrSet(IDC_TBR_SUB_POSY, x);
    expand2dlg();
    cbxSetCurSel(IDC_CBX_SUBTITLES_ALIGN, cfgGet(IDFF_subAlign));
    stereo2dlg();
}

void TsubtitlesPosPage::stereo2dlg(void)
{
    static const int idStereo[] = {IDC_LBL_SUB_STEREOSCOPICPAR, IDC_TBR_SUB_STEREOSCOPICPAR, 0};
    int is = cfgGet(IDFF_subStereoscopic);
    setCheck(IDC_CHB_SUB_STEREOSCOPIC, is);
    tbrSet(IDC_TBR_SUB_STEREOSCOPICPAR, cfgGet(IDFF_subStereoscopicPar), IDC_LBL_SUB_STEREOSCOPICPAR, NULL, 10.0f);
    enable(is, idStereo);
    enable(!is, IDC_CHB_SUBEXTENDEDTAGS, false);
}

void TsubtitlesPosPage::expand2dlg(void)
{
    int isExpand = cfgGet(IDFF_subIsExpand);
    setCheck(IDC_CHB_SUBTITLES_EXPAND, isExpand);
    // disable option to expand video output size for subtitles when configuring DXVA (we can't change output resolution when decoding using DXVA)
    enable(!(cfgGet(IDFF_filterMode) & IDFF_FILTERMODE_VIDEODXVA), IDC_CHB_SUBTITLES_EXPAND);
    static const int idExpand[] = {IDC_ED_SUBTITLES_EXPAND_X, IDC_LBL_SUBTITLES_EXPAND2, IDC_ED_SUBTITLES_EXPAND_Y, IDC_BT_SUBTITLES_EXPAND, 0};
    enable(isExpand, idExpand);
    int e1, e2;
    TsubtitlesSettings::getExpand(1, cfgGet(IDFF_subExpand), &e1, &e2);
    if (e1 == 0 && e2 == 0) {
        e1 = 4;
        e2 = 3;
        cfgSet(IDFF_subExpand, 1);
    }
    SetDlgItemInt(m_hwnd, IDC_ED_SUBTITLES_EXPAND_X, e1, FALSE);
    SetDlgItemInt(m_hwnd, IDC_ED_SUBTITLES_EXPAND_Y, e2, FALSE);
}

INT_PTR TsubtitlesPosPage::msgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_ED_SUBTITLES_EXPAND_X:
                case IDC_ED_SUBTITLES_EXPAND_Y:
                    if (HIWORD(wParam) == EN_CHANGE && !isSetWindowText) {
                        HWND hed = GetDlgItem(m_hwnd, LOWORD(wParam));
                        if (hed != GetFocus()) {
                            return FALSE;
                        }
                        repaint(hed);
                        int x, y;
                        if (eval(GetDlgItem(m_hwnd, IDC_ED_SUBTITLES_EXPAND_X), 1, 10000, &x) && eval(GetDlgItem(m_hwnd, IDC_ED_SUBTITLES_EXPAND_Y), 1, 10000, &y)) {
                            cfgSet(IDFF_subExpand, MAKELONG(y, x));
                            return S_OK;
                        }
                    }
                    break;
            }
            break;
    }
    return TconfPageDecVideo::msgProc(uMsg, wParam, lParam);
}

void TsubtitlesPosPage::onExpandClick(void)
{
    static const char_t *letterboxes[] = {
        _l("4:3"),
        _l("5:4"),
        _l("16:9"),
        NULL
    };
    int cmd = selectFromMenu(letterboxes, IDC_BT_SUBTITLES_EXPAND, false);
    int x, y;
    switch (cmd) {
        case 0:
            x = 4;
            y = 3;
            break;
        case 1:
            x = 5;
            y = 4;
            break;
        case 2:
            x = 16;
            y = 9;
            break;
        default:
            return;
    }
    cfgSet(IDFF_subExpand, MAKELONG(y, x));
    expand2dlg();
}

void TsubtitlesPosPage::translate(void)
{
    TconfPageBase::translate();
    cbxTranslate(IDC_CBX_SUBTITLES_ALIGN, TsubtitlesSettings::alignments);
}

TsubtitlesPosPage::TsubtitlesPosPage(TffdshowPageDec *Iparent, const TfilterIDFF *idff): TconfPageDecVideo(Iparent, idff, 5)
{
    dialogId = IDD_SUBTITLES_POS;
    static const TbindCheckbox<TsubtitlesPosPage> chb[] = {
        IDC_CHB_SUB_STEREOSCOPIC, IDFF_subStereoscopic, &TsubtitlesPosPage::stereo2dlg,
        IDC_CHB_SUBTITLES_EXPAND, IDFF_subIsExpand, &TsubtitlesPosPage::expand2dlg,
        IDC_CHB_SUB_SSA_OVERRIDE_POSITION, IDFF_subSSAOverridePlacement, NULL,
        IDC_CHB_SUB_SSA_KEEP_TEXT_INSIDE, IDFF_subSSAMaintainInside, NULL,
        IDC_CHB_SUB_SSA_USE_INPUT_DIMENSIONS, IDFF_subSSAUseMovieDimensions, NULL,
        0, NULL, NULL
    };
    bindCheckboxes(chb);

    static const TbindTrackbar<TsubtitlesPosPage> htbr[] = {
        IDC_TBR_SUB_POSX, IDFF_subPosX, &TsubtitlesPosPage::cfg2dlg,
        IDC_TBR_SUB_POSY, IDFF_subPosY, &TsubtitlesPosPage::cfg2dlg,
        IDC_TBR_SUB_STEREOSCOPICPAR, IDFF_subStereoscopicPar, &TsubtitlesPosPage::stereo2dlg,
        0, 0, NULL
    };
    bindHtracks(htbr);
    static const TbindCombobox<TsubtitlesPosPage> cbx[] = {
        IDC_CBX_SUBTITLES_ALIGN, IDFF_subAlign, BINDCBX_SEL, NULL,
        0
    };
    bindComboboxes(cbx);
    static const TbindButton<TsubtitlesPosPage> bt[] = {
        IDC_BT_SUBTITLES_EXPAND, &TsubtitlesPosPage::onExpandClick,
        0, NULL
    };
    bindButtons(bt);
}
