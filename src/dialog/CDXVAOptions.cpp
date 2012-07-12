/*
 * Copyright (c) 2002-2009 Damien Bain-Thouverez
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "stdafx.h"
#include "CdecoderOptions.h"
#include "CDXVAOptions.h"
#include "Tlibavcodec.h"
#include "TvideoCodec.h"
#include "Ttranslate.h"
#include "ffmpeg/libavcodec/avcodec.h"

const char_t* TDXVAOptionsPage::dec_dxva_compatibilityModes[] = {
    _l("Normal"),
    _l("Skip level check"),
    _l("Skip ref frame check"),
    _l("Skip all checks"),
    NULL
};

const char_t* TDXVAOptionsPage::dec_dxva_postProcessingModes[] = {
    _l("Disabled"),
    _l("Surface overlay"),
    NULL
};

void TDXVAOptionsPage::init(void)
{
    addHint(IDC_GRP_DXVA2, _l("Enabling DXVA on those codecs will disable all FFDShow internal filters (subtitles, resize,...)"));
    addHint(IDC_CHB_H264, _l("Enable DXVA on H264 codec."));
    addHint(IDC_CHB_VC1, _l("Enable DXVA on VC1 codec."));
    addHint(IDC_CBX_COMPATIBILITY_MODE, _l("This option forces DXVA to be used even if the format is not compatible. You may get artifacts if you change this option."));
    addHint(IDC_CBX_DXVA_POST_PROCESSING_MODE, _l("This option selects how the selected image filters will be applied.\nSurface overlay: subtitles / OSD will be overlayed on top of the decoded picture surface (done by software)"));
    cfg2dlg();
}

void TDXVAOptionsPage::cfg2dlg(void)
{
    setCheck(IDC_CHB_H264, cfgGet(IDFF_dec_DXVA_H264));
    setCheck(IDC_CHB_VC1, cfgGet(IDFF_dec_DXVA_VC1));
    cbxSetCurSel(IDC_CBX_COMPATIBILITY_MODE, cfgGet(IDFF_dec_DXVA_CompatibilityMode));
    cbxSetCurSel(IDC_CBX_DXVA_POST_PROCESSING_MODE, cfgGet(IDFF_dec_DXVA_PostProcessingMode));
}

INT_PTR TDXVAOptionsPage::msgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    /*switch (uMsg)
     {
      case WM_COMMAND:
       switch (LOWORD(wParam))
        {
         case IDC_...:
          {
           return TRUE;
          }
        }
       break;
     }*/
    return TconfPageDecVideo::msgProc(uMsg, wParam, lParam);
}

void TDXVAOptionsPage::translate(void)
{
    TconfPageBase::translate();
    cbxTranslate(IDC_CBX_COMPATIBILITY_MODE, TDXVAOptionsPage::dec_dxva_compatibilityModes);
    cbxTranslate(IDC_CBX_DXVA_POST_PROCESSING_MODE, TDXVAOptionsPage::dec_dxva_postProcessingModes);
}


TDXVAOptionsPage::TDXVAOptionsPage(TffdshowPageDec *Iparent): TconfPageDecVideo(Iparent)
{
    dialogId = IDD_DXVAOPTIONS;
    inPreset = 1;
    static const TbindCheckbox<TDXVAOptionsPage> chb[] = {
        IDC_CHB_H264, IDFF_dec_DXVA_H264, NULL,
        IDC_CHB_VC1, IDFF_dec_DXVA_VC1, NULL,
        0, NULL, NULL
    };
    bindCheckboxes(chb);
    static const TbindCombobox<TDXVAOptionsPage> cbx[] = {
        IDC_CBX_COMPATIBILITY_MODE, IDFF_dec_DXVA_CompatibilityMode, BINDCBX_SEL, NULL,
        IDC_CBX_DXVA_POST_PROCESSING_MODE, IDFF_dec_DXVA_PostProcessingMode, BINDCBX_SEL, NULL,
        0
    };
    bindComboboxes(cbx);
}
