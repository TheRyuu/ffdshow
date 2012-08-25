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
#include "Cinfosimd.h"
#include "TvideoCodec.h"
#include "TaudioCodec.h"
#include "ffdshow_mediaguids.h"
#include "Tconfig.h"
#include "TffdshowPageDec.h"
#include "Tinfo.h"
#include "Ttranslate.h"
#include "TcompatibilityList.h"

//=================================== TinfoPageDec =======================================
void TinfoPageDec::init(void)
{
    hlv = GetDlgItem(m_hwnd, IDC_LV_INFO);
    CRect r = getChildRect(IDC_LV_INFO);
    int ncol = 0;
    ListView_AddCol(hlv, ncol, r.Width(), _l("Property"), false);
    ListView_SetExtendedListViewStyleEx(hlv, LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP, LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);
    infoitems.clear();
    const int *infos = getInfos();
    for (int i = 0;; i++) {
        Titem it;
        if (!info->getInfo(i, &it.id, &it.name)) {
            break;
        }
        for (int j = 0; infos[j]; j++)
            if (infos[j] == it.id) {
                it.index = j;
                infoitems.push_back(it);
            }
    }
    std::sort(infoitems.begin(), infoitems.end(), TsortItem(infos));
    ListView_SetItemCount(hlv, infoitems.size());
    SendMessage(hlv, LVM_SETBKCOLOR, 0, GetSysColor(COLOR_BTNFACE));
    SendMessage(hlv, LVM_SETTEXTBKCOLOR, 0, GetSysColor(COLOR_BTNFACE));
    setCheck(IDC_CHB_WRITEINFO2DBG, cfgGet(IDFF_allowDPRINTF));
}

void TinfoPageDec::cfg2dlg(void)
{
}

INT_PTR TinfoPageDec::msgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_NOTIFY: {
            NMHDR *nmhdr = LPNMHDR(lParam);
            if (nmhdr->hwndFrom == hlv && nmhdr->idFrom == IDC_LV_INFO)
                switch (nmhdr->code) {
                    case LVN_GETDISPINFO: {
                        NMLVDISPINFO *nmdi = (NMLVDISPINFO*)lParam;
                        int i = nmdi->item.iItem;
                        if (i == -1) {
                            break;
                        }
                        if (nmdi->item.mask & LVIF_TEXT)
                            switch (nmdi->item.iSubItem) {
                                case 0: {
                                    nmdi->item.pszText = pszTextBuf;
                                    tsnprintf_s(pszTextBuf, countof(pszTextBuf), _TRUNCATE, _l("%s: %s"), infoitems[i].translatedName, infoitems[i].val ? infoitems[i].val : _l(""));
                                    break;
                                }
                            }
                        return TRUE;
                    }
                }
            break;
        }
    }
    return TconfPageDec::msgProc(uMsg, wParam, lParam);
}

void TinfoPageDec::onFrame(void)
{
    if (!IsWindowVisible(m_hwnd)) {
        return;
    }
    for (Titems::iterator i = infoitems.begin(); i != infoitems.end(); i++) {
        int wasChange;
        i->val = info->getVal(i->id, &wasChange, NULL);
        if (wasChange) {
            ListView_Update(hlv, i - infoitems.begin());
        }
    }
}

void TinfoPageDec::onAllowDPRINTF(void)
{
    cfgSet(IDFF_allowDPRINTFchanged, 1);
}

void TinfoPageDec::translate(void)
{
    TconfPageDec::translate();

    for (Titems::iterator i = infoitems.begin(); i != infoitems.end(); i++) {
        i->translatedName = _(IDC_LV_INFO, i->name);
    }
}

TinfoPageDec::TinfoPageDec(TffdshowPageDec *Iparent, TinfoBase *Iinfo): TconfPageDec(Iparent, NULL, 0), info(Iinfo)
{
    dialogId = IDD_INFOSIMD;

    static const TbindCheckbox<TinfoPageDec> chb[] = {
        IDC_CHB_WRITEINFO2DBG, IDFF_allowDPRINTF, &TinfoPageDec::onAllowDPRINTF,
        0, NULL, NULL
    };
    bindCheckboxes(chb);
}
TinfoPageDec::~TinfoPageDec()
{
    delete info;
}
//================================= TinfoPageDecVideo ====================================
TinfoPageDecVideo::TinfoPageDecVideo(TffdshowPageDec *Iparent): TinfoPageDec(Iparent, new TinfoDecVideo(Iparent->deci))
{
}

//================================= TinfoPageDecAudio ====================================
TinfoPageDecAudio::TinfoPageDecAudio(TffdshowPageDec *Iparent): TinfoPageDec(Iparent, new TinfoDecAudio(Iparent->deci))
{
}
