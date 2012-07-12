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
#include "Cout.h"
#include "Tdll.h"
#include "Tmuxer.h"
#include "TcodecSettings.h"

void ToutPage::init(void)
{
    SendDlgItemMessage(m_hwnd, IDC_BT_ASPECT, WM_SETFONT, WPARAM(parent->arrowsFont), LPARAM(false));
    SendDlgItemMessage(m_hwnd, IDC_BT_FPS, WM_SETFONT, WPARAM(parent->arrowsFont), LPARAM(false));
}

void ToutPage::cfg2dlg(void)
{
    out2dlg();
}

void ToutPage::out2dlg(void)
{
    setCheck(IDC_CHB_STORE_AVI, cfgGet(IDFF_enc_storeAVI));
    setCheck(IDC_CHB_STORE_EXTERNAL, cfgGet(IDFF_enc_storeExt));
    static const int idStoreExt[] = {IDC_CBX_MUXER, IDC_BT_STORE_EXTERNAL, IDC_ED_STORE_EXTERNAL, 0};
    setDlgItemText(m_hwnd, IDC_ED_STORE_EXTERNAL, cfgGetStr(IDFF_enc_storeExtFlnm));
    cbxSetCurSel(IDC_CBX_MUXER, cfgGet(IDFF_enc_muxer));
    enable(cfgGet(IDFF_enc_storeExt), idStoreExt);
}

INT_PTR ToutPage::msgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_ED_STORE_EXTERNAL:
                    if (HIWORD(wParam) == EN_CHANGE && !isSetWindowText) {
                        char_t storeExtFlnm[MAX_PATH];
                        GetDlgItemText(m_hwnd, IDC_ED_STORE_EXTERNAL, storeExtFlnm, MAX_PATH);
                        cfgSet(IDFF_enc_storeExtFlnm, storeExtFlnm);
                        return TRUE;
                    }
                    break;
            }
            break;
    }
    return TconfPageEnc::msgProc(uMsg, wParam, lParam);
}

void ToutPage::onStoreExternal(void)
{
    char_t storeExtFlnm[MAX_PATH];
    cfgGet(IDFF_enc_storeExtFlnm, storeExtFlnm, MAX_PATH);
    if (dlgGetFile(true, m_hwnd, _(-IDD_OUT, _l("Select file for storing frames")), _l("All files (*.*)\0*.*\0"), _l(""), storeExtFlnm, _l("."), 0)) {
        cfgSet(IDFF_enc_storeExtFlnm, storeExtFlnm);
        out2dlg();
    }
}

void ToutPage::translate(void)
{
    TconfPageEnc::translate();

    cbxTranslate(IDC_CBX_MUXER, Tmuxer::muxers);
}

ToutPage::ToutPage(TffdshowPageEnc *Iparent): TconfPageEnc(Iparent), parentE(Iparent)
{
    dialogId = IDD_OUT;
    static const int props[] = {IDFF_enc_storeAVI, IDFF_enc_storeExt, IDFF_enc_storeExtFlnm, IDFF_enc_muxer, 0};
    propsIDs = props;
    static const TbindCheckbox<ToutPage> chb[] = {
        IDC_CHB_STORE_EXTERNAL, IDFF_enc_storeExt, &ToutPage::out2dlg,
        IDC_CHB_STORE_AVI, IDFF_enc_storeAVI, NULL,
        0, NULL, NULL
    };
    bindCheckboxes(chb);
    static const TbindCombobox<ToutPage> cbx[] = {
        IDC_CBX_MUXER, IDFF_enc_muxer, BINDCBX_SEL, NULL,
        0
    };
    bindComboboxes(cbx);
    static const TbindButton<ToutPage> bt[] = {
        IDC_BT_STORE_EXTERNAL, &ToutPage::onStoreExternal,
        0, NULL
    };
    bindButtons(bt);
}
