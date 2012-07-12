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
#include "COSD.h"
#include "TsubtitlesSettings.h"
#include "Tsubreader.h"
#include "TffdshowPageDec.h"
#include "IffdshowDecVideo.h"

//=================================== TOSDpageDec ==================================
void TOSDpageDec::init(void)
{
    tbrSetRange(IDC_TBR_OSD_POSX, 0, 100, 10);
    tbrSetRange(IDC_TBR_OSD_POSY, 0, 100, 10);

    setFont(IDC_BT_OSD_LINE_UP  , parent->arrowsFont);
    setFont(IDC_BT_OSD_LINE_DOWN, parent->arrowsFont);

    hlv = GetDlgItem(m_hwnd, IDC_LV_OSD_LINES);
    ListView_SetExtendedListViewStyleEx(hlv, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
    int ncol = 0;
    RECT r;
    GetWindowRect(hlv, &r);
    ListView_AddCol(hlv, ncol, r.right - r.left - 26, _l("OSD item"), false);

    edLimitText(IDC_ED_OSD_USER, 1019);
    dragitem = -1;
    CRect rp;
    GetWindowRect(m_hwnd, &rp);
    CRect rc;
    GetWindowRect(hlv, &rc);
    lvx = rc.left - rp.left;
    lvy = rc.top - rp.top;

    edLimitText(IDC_ED_OSD_SAVE, MAX_PATH);
    cbxAdd(IDC_CBX_OSD_USERFORMAT, _l("HTML"), Tsubreader::SUB_SUBVIEWER);
    cbxAdd(IDC_CBX_OSD_USERFORMAT, _l("SSA"), Tsubreader::SUB_SSA);
}

void TOSDpageDec::cfg2dlg(void)
{
    pos2dlg();
    osds2dlg();
    int format = cfgGet(IDFF_OSD_userformat);
    if (cbxSetDataCurSel(IDC_CBX_OSD_USERFORMAT, format) == CB_ERR) {
        format = (int)cbxGetItemData(IDC_CBX_OSD_USERFORMAT, 0);
        cfgSet(IDFF_OSD_userformat, format);
        cbxSetCurSel(IDC_CBX_OSD_USERFORMAT, 0);
    }
}

void TOSDpageDec::pos2dlg(void)
{
    char_t s[260];
    int x;

    x = cfgGet(IDFF_OSDposX);
    TsubtitlesSettings::getPosHoriz(x, s, this, IDC_LBL_OSD_POSX, countof(s));
    setDlgItemText(m_hwnd, IDC_LBL_OSD_POSX, s);
    tbrSet(IDC_TBR_OSD_POSX, x);

    x = cfgGet(IDFF_OSDposY);
    TsubtitlesSettings::getPosVert(x, s, this, IDC_LBL_OSD_POSY, countof(s));
    setDlgItemText(m_hwnd, IDC_LBL_OSD_POSY, s);
    tbrSet(IDC_TBR_OSD_POSY, x);
}

void TOSDpageDec::osds2dlg(void)
{
    nostate = true;
    osdslabels.clear();
    osd2dlg();
    nostate = false;
}

int CALLBACK TOSDpageDec::osdsSort(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    TOSDpageDec *self = (TOSDpageDec*)lParamSort;
    return std::find(self->osds.begin(), self->osds.end(), int(lParam1)) > std::find(self->osds.begin(), self->osds.end(), int(lParam2));
}

void TOSDpageDec::osd2dlg(void)
{
    const char_t *osdsStr = cfgGetStr(IDFF_OSDformat);
    if (strncmp(osdsStr, _l("user"), 4) == 0) {
        ListView_SetExtendedListViewStyleEx(hlv, LVS_EX_CHECKBOXES, 0);
        setCheck(IDC_CHB_OSD_USER, 1);
        enable(1, IDC_ED_OSD_USER);
        setText(IDC_ED_OSD_USER, _l("%s"), osdsStr + 4);
        user = true;
    } else {
        ListView_SetExtendedListViewStyleEx(hlv, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
        setCheck(IDC_CHB_OSD_USER, 0);
        enable(0, IDC_ED_OSD_USER);
        strtok(osdsStr, _l(" "), osds);
        int cnt = ListView_GetItemCount(hlv);
        nostate = true;
        for (int j = 0; j < cnt; j++) {
            ListView_SetCheckState(hlv, j, FALSE);
        }
        nostate = false;
        for (ints::const_iterator i = osds.begin(); i != osds.end(); i++) {
            checkOSDline(*i, true);
        }
        ListView_SortItems(hlv, osdsSort, LPARAM(this));
        user = false;
    }
    int isAutoHide = cfgGet(IDFF_OSDisAutoHide);
    setCheck(IDC_CHB_OSD_IS_AUTO_HIDE, isAutoHide);
    enable(isAutoHide, IDC_ED_OSD_DURATION_VISIBLE);
    SetDlgItemInt(m_hwnd, IDC_ED_OSD_DURATION_VISIBLE, cfgGet(IDFF_OSDdurationVisible), FALSE);
    save2dlg();
}

void TOSDpageDec::save2dlg(void)
{
    int is = cfgGet(IDFF_OSDisSave);
    setCheck(IDC_CHB_OSD_SAVE, is);
    int isUser = getCheck(IDC_CHB_OSD_USER);
    enable(!isUser, IDC_CHB_OSD_SAVE);
    setDlgItemText(m_hwnd, IDC_ED_OSD_SAVE, cfgGetStr(IDFF_OSDsaveFlnm));
    is &= !isUser;
    static const int idSave[] = {IDC_ED_OSD_SAVE, IDC_BT_OSD_SAVE, 0};
    enable(is, idSave);
}

void TOSDpageDec::checkOSDline(int idff, bool check)
{
    nostate = true;
    LVFINDINFO lvfi;
    memset(&lvfi, 0, sizeof(lvfi));
    lvfi.flags = LVFI_PARAM;
    lvfi.lParam = idff;
    int i = ListView_FindItem(hlv, -1, &lvfi);
    if (i != -1) {
        ListView_SetCheckState(hlv, i, check);
    }
    nostate = false;
}
void TOSDpageDec::lv2osdFormat(void)
{
    char_t format[1024] = _l("");
    int cnt = ListView_GetItemCount(hlv);
    for (int i = 0; i < cnt; i++)
        if (ListView_GetCheckState(hlv, i)) {
            strncatf(format, countof(format), _l("%i "), lvGetItemParam(IDC_LV_OSD_LINES, i));
        }
    if (format[strlen(format) - 1] == ' ') {
        format[strlen(format) - 1] = '\0';
    }
    cfgSet(IDFF_OSDformat, format);
    parent->setChange();
}

INT_PTR TOSDpageDec::msgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_CHB_OSD:
                    cfgSet(IDFF_isOSD, getCheck(IDC_CHB_OSD));
                    parent->drawInter();
                    return TRUE;
                case IDC_CHB_OSD_IS_AUTO_HIDE:
                    cfgSet(IDFF_OSDisAutoHide, getCheck(IDC_CHB_OSD_IS_AUTO_HIDE));
                    osd2dlg();
                    parent->setChange();
                    break;
                case IDC_ED_OSD_DURATION_VISIBLE:
                    if (HIWORD(wParam) == EN_CHANGE && !isSetWindowText) {
                        HWND hed = GetDlgItem(m_hwnd, LOWORD(wParam));
                        if (hed != GetFocus()) {
                            return FALSE;
                        }
                        repaint(hed);
                        parent->setChange();
                        break;
                    }
                    break;
                case IDC_CHB_OSD_USER:
                    if (!getCheck(IDC_CHB_OSD_USER)) {
                        lv2osdFormat();
                    } else {
                        cfgSet(IDFF_OSDformat, _l("user"));
                    }
                    osd2dlg();
                    parent->setChange();
                    break;
                case IDC_ED_OSD_USER:
                    if (HIWORD(wParam) == EN_CHANGE && !isSetWindowText) {
                        char_t ed[1020]; //4 chars are reserved for "user" prefix
                        GetDlgItemText(m_hwnd, IDC_ED_OSD_USER, ed, 1020);
                        char_t format[1024];
                        tsnprintf_s(format, 1024, _TRUNCATE, _l("user%s"), ed);
                        cfgSet(IDFF_OSDformat, format);
                        parent->setChange();
                    };
                    break;
                case IDC_ED_OSD_SAVE:
                    if (HIWORD(wParam) == EN_CHANGE && !isSetWindowText) {
                        char_t saveflnm[MAX_PATH];
                        GetDlgItemText(m_hwnd, IDC_ED_OSD_SAVE, saveflnm, MAX_PATH);
                        cfgSet(IDFF_OSDsaveFlnm, saveflnm);
                        return TRUE;
                    }
                    return TRUE;
            }
            break;
        case WM_NOTIFY: {
            NMHDR *nmhdr = LPNMHDR(lParam);
            if (!nostate && nmhdr->hwndFrom == hlv && nmhdr->idFrom == IDC_LV_OSD_LINES)
                switch (nmhdr->code) {
                    case LVN_ITEMCHANGED: {
                        LPNMLISTVIEW nmlv = LPNMLISTVIEW(lParam);
                        if (nmlv->uChanged & LVIF_STATE && ((nmlv->uOldState & 4096) != (nmlv->uNewState & 4096))) {
                            lv2osdFormat();
                        }
                        return TRUE;
                    }
                    case LVN_BEGINDRAG: {
                        if (!user) {
                            LPNMLISTVIEW nmlv = LPNMLISTVIEW(lParam);
                            if (nmlv->iItem != -1) {
                                dragitem = nmlv->iItem;
                                SetCapture(m_hwnd);
                            }
                        }
                        break;
                    }
                    case NM_DBLCLK: {
                        if (user) {
                            LPNMITEMACTIVATE nmia = LPNMITEMACTIVATE(lParam);
                            if (nmia->iItem != -1) {
                                const char_t *shortcut = deci->getInfoItemShortcut((int)lvGetItemParam(IDC_LV_OSD_LINES, nmia->iItem));
                                if (shortcut && shortcut[0]) {
                                    char_t osd[1020];
                                    tsnprintf_s(osd, countof(osd), _TRUNCATE, _l("%%%s"), shortcut);
                                    SendDlgItemMessage(m_hwnd, IDC_ED_OSD_USER, EM_REPLACESEL, TRUE, LPARAM(osd));
                                }
                            }
                        }
                        break;
                    }
                }
            break;
        }
        case WM_MOUSEMOVE:
            if (dragitem != -1) {
                LVHITTESTINFO lvhti;
                lvhti.pt.x = LOWORD(lParam) - lvx;
                lvhti.pt.y = HIWORD(lParam) - lvy;
                int target = ListView_HitTest(hlv, &lvhti);
                if (target != -1) {
                    lvSwapItems(IDC_LV_OSD_LINES, target, dragitem);
                    lv2osdFormat();
                    dragitem = target;
                }
                return TRUE;
            }
            break;
        case WM_LBUTTONUP:
            if (dragitem != -1) {
                dragitem = -1;
                ReleaseCapture();
                return TRUE;
            }
            break;
    }
    return TconfPageDec::msgProc(uMsg, wParam, lParam);
}
void TOSDpageDec::onLineUp(void)
{
    int ii = lvGetSelItem(IDC_LV_OSD_LINES);
    if (ii >= 1) {
        lvSwapItems(IDC_LV_OSD_LINES, ii, ii - 1);
        lv2osdFormat();
    }
}
void TOSDpageDec::onLineDown(void)
{
    int ii = lvGetSelItem(IDC_LV_OSD_LINES);
    if (ii != -1 && ii < ListView_GetItemCount(hlv) - 1) {
        lvSwapItems(IDC_LV_OSD_LINES, ii, ii + 1);
        lv2osdFormat();
    }
}

void TOSDpageDec::onSave(void)
{
    char_t flnm[MAX_PATH];
    cfgGet(IDFF_OSDsaveFlnm, flnm, MAX_PATH);
    if (dlgGetFile(true, m_hwnd, _(-IDD_OSD, _l("File where to write statistics")), _l("Comma delimited (*.csv)\0*.csv\0"), _l("csv"), flnm, _l("."), 0)) {
        setDlgItemText(m_hwnd, IDC_ED_OSD_SAVE, flnm);
        cfgSet(IDFF_OSDsaveFlnm, flnm);
    }
}

void TOSDpageDec::applySettings(void)
{
    char_t flnm[MAX_PATH];
    GetDlgItemText(m_hwnd, IDC_ED_OSD_SAVE, flnm, MAX_PATH);
    cfgSet(IDFF_OSDsaveFlnm, flnm);

    HWND hed = GetDlgItem(m_hwnd, IDC_ED_OSD_DURATION_VISIBLE);
    eval(hed, 0, 10000, IDFF_OSDdurationVisible);
}

void TOSDpageDec::translate(void)
{
    TconfPageDec::translate();

    nostate = true;
    ListView_DeleteAllItems(hlv);
    for (int i = 0;; i++) {
        LVITEM lvi;
        memset(&lvi, 0, sizeof(lvi));
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem = i + 1;
        const char_t *text;
        if (deci->getInfoItem(i, (int*)&lvi.lParam, &text) != S_OK) {
            break;
        }
        lvi.pszText = const_cast<char_t*>(_(IDC_LV_OSD_LINES, text));
        ListView_InsertItem(hlv, &lvi);
    }
    nostate = false;
}

TOSDpageDec::TOSDpageDec(TffdshowPageDec *Iparent, const TfilterIDFF *idff): TconfPageDec(Iparent, idff, 0)
{
    dialogId = IDD_OSD;
    idffInter = IDFF_isOSD;
    resInter = IDC_CHB_OSD;
    static const TbindCheckbox<TOSDpageDec> chb[] = {
        IDC_CHB_OSD_SAVE, IDFF_OSDisSave, &TOSDpageDec::save2dlg,
        IDC_CHB_OSD_IS_AUTO_HIDE, IDFF_OSDisAutoHide, &TOSDpageDec::save2dlg,
        0, NULL, NULL
    };
    bindCheckboxes(chb);
    static const TbindTrackbar<TOSDpageDec> htbr[] = {
        IDC_TBR_OSD_POSX, IDFF_OSDposX, &TOSDpageDec::pos2dlg,
        IDC_TBR_OSD_POSY, IDFF_OSDposY, &TOSDpageDec::pos2dlg,
        0, 0, NULL
    };
    bindHtracks(htbr);
    static const TbindCombobox<TOSDpageDec> cbx[] = {
        IDC_CBX_OSD_USERFORMAT, IDFF_OSD_userformat, BINDCBX_DATA, &TOSDpageDec::cfg2dlg,
        0
    };
    bindComboboxes(cbx);
    static const TbindButton<TOSDpageDec> bt[] = {
        IDC_BT_OSD_LINE_UP, &TOSDpageDec::onLineUp,
        IDC_BT_OSD_LINE_DOWN, &TOSDpageDec::onLineDown,
        IDC_BT_OSD_SAVE, &TOSDpageDec::onSave,
        0, NULL
    };
    bindButtons(bt);
}

//================================ TOSDpageDecVideo ================================
TOSDpageVideo::TOSDpageVideo(TffdshowPageDec *Iparent, const TfilterIDFF *idff): TOSDpageDec(Iparent, idff)
{
}

bool TOSDpageVideo::reset(bool testonly)
{
    return true;
}

//================================ TOSDpageDecAudio ================================
TOSDpageAudio::TOSDpageAudio(TffdshowPageDec *Iparent, const TfilterIDFF *idff): TOSDpageDec(Iparent, idff)
{
    idffOrder = maxOrder + 1;
}

void TOSDpageAudio::init(void)
{
    TOSDpageDec::init();

    static const int idPos[] = {IDC_LBL_OSD_POSX, IDC_TBR_OSD_POSX, IDC_LBL_OSD_POSY, IDC_TBR_OSD_POSY, 0};
    enable(0, idPos);
}
