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
#include "CquantTables.h"
#include "Ttranslate.h"
#include "ffmpeg/libavcodec/avcodec.h"

void TDXVAOptionsPage::init(void)
{
    const TvideoCodecDec *movie;
    deciV->getMovieSource(&movie);
    int source = movie ? movie->getType() : 0;
    islavc = ((filterMode & IDFF_FILTERMODE_PLAYER) && (source == IDFF_MOVIE_LAVC || source == IDFF_MOVIE_FFMPEG_MT || source == IDFF_MOVIE_FFMPEG_DXVA)) || (filterMode & (IDFF_FILTERMODE_CONFIG | IDFF_FILTERMODE_VFW));
    addHint(IDC_GRP_DXVA2, _l("Enabling DXVA on those codecs will disable all FFDShow internal filters (subtitles, resize,...)"));
    addHint(IDC_CHB_H264, _l("Enable DXVA on H264 codec. Prior the H264/AVC codec must be enabled in the codec section"));
    addHint(IDC_CHB_VC1, _l("Enable DXVA on VC1 codec. Prior the H264/AVC codec must be enabled in the codec section"));
    cfg2dlg();
}

void TDXVAOptionsPage::cfg2dlg(void)
{
    setCheck(IDC_CHB_H264, cfgGet(IDFF_dec_DXVA_H264));
    setCheck(IDC_CHB_VC1, cfgGet(IDFF_dec_DXVA_VC1));
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
}
