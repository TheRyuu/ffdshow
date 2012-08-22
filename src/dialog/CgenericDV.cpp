/*
 * Copyright (c) 2004-2006 Milan Cutka
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
#include "CgenericDV.h"
#include "libavcodec/dv_profile.c"

void TgenericDVpage::init(void)
{
}

bool TgenericDVpage::enabled(void)
{
    return codecId == AV_CODEC_ID_DVVIDEO;
}

void TgenericDVpage::cfg2dlg(void)
{
    cbxSetDataCurSel(IDC_CBX_DV_PROFILE, cfgGet(IDFF_enc_dv_profile));
}

void TgenericDVpage::translate(void)
{
    TconfPageEnc::translate();

    int ii = cbxGetCurSel(IDC_CBX_DV_PROFILE);
    cbxClear(IDC_CBX_DV_PROFILE);
    cbxAdd(IDC_CBX_DV_PROFILE, _(IDC_CBX_DV_PROFILE, _l("automatic")), DV_PROFILE_AUTO);
    for (int i = 0; i < countof(dv_profiles); i++) {
        cbxAdd(IDC_CBX_DV_PROFILE, _(IDC_CBX_DV_PROFILE, text<char_t>(dv_profiles[i].name)), i);
    }
    cbxSetCurSel(IDC_CBX_DV_PROFILE, ii);
}

TgenericDVpage::TgenericDVpage(TffdshowPageEnc *Iparent): TconfPageEnc(Iparent)
{
    dialogId = IDD_GENERIC_DV;
    static const int props[] = {IDFF_enc_dv_profile, 0};
    propsIDs = props;
    static const TbindCombobox<TgenericDVpage> cbx[] = {
        IDC_CBX_DV_PROFILE, IDFF_enc_dv_profile, BINDCBX_DATA, NULL,
        0
    };
    bindComboboxes(cbx);
}
