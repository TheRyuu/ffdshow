/*
 * Copyright (c) 2002-2006 Milan Cutka
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "stdafx.h"
#include "Cgeneric.h"
#include "libavcodec/avcodec.h"

void TgenericPage::init(void)
{
    tbrSetRange(IDC_TBR_MAXKEYINTERVAL, 1, 500, 10);
    tbrSetRange(IDC_TBR_MINKEYINTERVAL, 1, 500, 10);

    hlv = GetDlgItem(m_hwnd, IDC_LV_GENERIC);
    ListView_SetExtendedListViewStyleEx(hlv, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
    int ncol = 0;
    RECT r;
    GetWindowRect(hlv, &r);
    ListView_AddCol(hlv, ncol, r.right - r.left - 36, _l("Option"), false);
    SendMessage(hlv, LVM_SETBKCOLOR, 0, GetSysColor(COLOR_BTNFACE));
    SendMessage(hlv, LVM_SETTEXTBKCOLOR, 0, GetSysColor(COLOR_BTNFACE));
}

void TgenericPage::cfg2dlg(void)
{
    flags.clear();
    if (sup_gray(codecId)) {
        flags.push_back(std::make_tuple(_(IDC_LV_GENERIC, _l("Greyscale")), IDFF_enc_gray, 1, false));
    }

    nostate = true;
    int iig = lvGetSelItem(IDC_LV_GENERIC);
    ListView_DeleteAllItems(hlv);
    for (Tflags::const_iterator f = flags.begin(); f != flags.end(); f++) {
        LVITEM lvi;
        memset(&lvi, 0, sizeof(lvi));
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.pszText = LPTSTR(std::get < NAME - 1 > (*f));
        lvi.lParam = LPARAM(&*f);
        lvi.iItem = 100;
        int ii = ListView_InsertItem(hlv, &lvi);
        ListView_SetCheckState(hlv, ii, cfgGet(std::get < IDFF - 1 > (*f))&std::get < VAL - 1 > (*f));
    }
    lvSetSelItem(IDC_LV_GENERIC, iig);
    nostate = false;
}

INT_PTR TgenericPage::msgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_NOTIFY: {
            NMHDR *nmhdr = LPNMHDR(lParam);
            if (!nostate && nmhdr->hwndFrom == hlv && nmhdr->idFrom == IDC_LV_GENERIC)
                switch (nmhdr->code) {
                    case LVN_ITEMCHANGED: {
                        LPNMLISTVIEW nmlv = LPNMLISTVIEW(lParam);
                        if (nmlv->uChanged & LVIF_STATE && ((nmlv->uOldState & 4096) != (nmlv->uNewState & 4096))) {
                            Tflag *f = (Tflag*)nmlv->lParam;
                            if (nmlv->uNewState & 8192) {
                                cfgSet(std::get < IDFF - 1 > (*f), cfgGet(std::get < IDFF - 1 > (*f)) | std::get < VAL - 1 > (*f));
                            } else if (nmlv->uNewState & 4096) {
                                cfgSet(std::get < IDFF - 1 > (*f), cfgGet(std::get < IDFF - 1 > (*f))&~std::get < VAL - 1 > (*f));
                            }
                            if (std::get < REPAINT - 1 > (*f)) {
                                cfg2dlg();
                            }
                        }
                        return TRUE;
                    }
                    break;
                }
            break;
        }
    }
    return TconfPageEnc::msgProc(uMsg, wParam, lParam);
}

TgenericPage::TgenericPage(TffdshowPageEnc *Iparent): TconfPageEnc(Iparent)
{
    dialogId = IDD_GENERIC;
    static const int props[] = {IDFF_enc_interlacing, IDFF_enc_interlacing_tff, IDFF_enc_gray, 0};
    propsIDs = props;
}
