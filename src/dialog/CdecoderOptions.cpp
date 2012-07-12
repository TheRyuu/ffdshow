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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "stdafx.h"
#include "CdecoderOptions.h"
#include "Tlibavcodec.h"
#include "TvideoCodec.h"
#include "CquantTables.h"
#include "Ttranslate.h"
#include "ffmpeg/libavcodec/avcodec.h"

const TdecoderOptionsPage::Tworkaround TdecoderOptionsPage::workarounds[] = {
    FF_BUG_AUTODETECT , IDC_CHB_WORKAROUND_AUTODETECT,
    FF_BUG_OLD_MSMPEG4, IDC_CHB_WORKAROUND_OLDMSMPEG4,
    FF_BUG_XVID_ILACE , IDC_CHB_WORKAROUND_XVIDILACE ,
    FF_BUG_UMP4       , IDC_CHB_WORKAROUND_UMP4      ,
    FF_BUG_NO_PADDING , IDC_CHB_WORKAROUND_NOPADDING ,
    FF_BUG_QPEL_CHROMA, IDC_CHB_WORKAROUND_QPELCHROMA,
    FF_BUG_EDGE       , IDC_CHB_WORKAROUND_EDGE      ,
    FF_BUG_HPEL_CHROMA, IDC_CHB_WORKAROUND_HPELCHROMA,
    FF_BUG_AMV        , IDC_CHB_WORKAROUND_AMV       ,
    FF_BUG_DC_CLIP    , IDC_CHB_WORKAROUND_DC_CLIP   ,
    0, 0
};

void TdecoderOptionsPage::init(void)
{
    const TvideoCodecDec *movie;
    deciV->getMovieSource(&movie);
    int source = movie ? movie->getType() : 0;
    islavc = ((filterMode & IDFF_FILTERMODE_PLAYER) && (source == IDFF_MOVIE_LAVC || source == IDFF_MOVIE_FFMPEG_DXVA)) || (filterMode & (IDFF_FILTERMODE_CONFIG | IDFF_FILTERMODE_VFW));
    for (int i = 0; workarounds[i].ff_bug; i++) {
        enable(islavc, workarounds[i].idc_chb);
    }
    static const int idLavc[] = {IDC_LBL_IDCT, IDC_CBX_IDCT, IDC_LBL_BUGS, IDC_BT_QUANTMATRIX_EXPORT, IDC_ED_NUMTHREADS, IDC_CHB_H264_SKIP_ON_DELAY, IDC_ED_H264SKIP_ON_DELAY_TIME, IDC_LBL_NUMTHREADS, 0};
    enable(islavc, idLavc);
    addHint(IDC_ED_NUMTHREADS, _l("For libavcodec H.264, MPEG-1/2, FFV1, and DV decoders"));
    addHint(IDC_CHB_SOFT_TELECINE, _l("Checked: If soft telecine is detected, frames are flagged as progressive.\n\nYou may want to unckeck if you have interlaced TV."));
}

void TdecoderOptionsPage::cfg2dlg(void)
{
    setCheck(IDC_CHB_SOFT_TELECINE, cfgGet(IDFF_softTelecine));
    if (islavc) {
        cbxSetCurSel(IDC_CBX_IDCT, cfgGet(IDFF_idct));

        int bugs = cfgGet(IDFF_workaroundBugs);
        for (int i = 0; workarounds[i].ff_bug; i++) {
            setCheck(workarounds[i].idc_chb, bugs & workarounds[i].ff_bug);
        }
        SetDlgItemInt(m_hwnd, IDC_ED_NUMTHREADS, cfgGet(IDFF_numLAVCdecThreads), FALSE);
        setCheck(IDC_CHB_H264_SKIP_ON_DELAY, cfgGet(IDFF_h264skipOnDelay));
        SetDlgItemInt(m_hwnd, IDC_ED_H264SKIP_ON_DELAY_TIME, cfgGet(IDFF_h264skipOnDelayTime), FALSE);
    }
    setCheck(IDC_CHB_DROP_ON_DELAY, cfgGet(IDFF_dropOnDelay));
    SetDlgItemInt(m_hwnd, IDC_ED_DROP_ON_DELAY_TIME, cfgGet(IDFF_dropOnDelayTime), FALSE);
}

INT_PTR TdecoderOptionsPage::msgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_CHB_WORKAROUND_AUTODETECT:
                case IDC_CHB_WORKAROUND_OLDMSMPEG4:
                case IDC_CHB_WORKAROUND_XVIDILACE:
                case IDC_CHB_WORKAROUND_UMP4:
                case IDC_CHB_WORKAROUND_NOPADDING:
                case IDC_CHB_WORKAROUND_QPELCHROMA:
                case IDC_CHB_WORKAROUND_EDGE:
                case IDC_CHB_WORKAROUND_HPELCHROMA:
                case IDC_CHB_WORKAROUND_AMV:
                case IDC_CHB_WORKAROUND_DC_CLIP: {
                    int bugs = 0;
                    for (int i = 0; workarounds[i].ff_bug; i++)
                        if (getCheck(workarounds[i].idc_chb)) {
                            bugs |= workarounds[i].ff_bug;
                        }
                    deci->putParam(IDFF_workaroundBugs, bugs);
                    return TRUE;
                }
            }
            break;
    }
    return TconfPageDecVideo::msgProc(uMsg, wParam, lParam);
}
bool TdecoderOptionsPage::reset(bool testonly)
{
    if (!testonly) {
        deci->resetParam(IDFF_idct);
        deci->resetParam(IDFF_softTelecine);
        deci->resetParam(IDFF_workaroundBugs);
        deci->resetParam(IDFF_numLAVCdecThreads);
        deci->resetParam(IDFF_dropOnDelay);
        deci->resetParam(IDFF_dropOnDelayTime);
        deci->resetParam(IDFF_h264skipOnDelay);
        deci->resetParam(IDFF_h264skipOnDelayTime);
    }
    return true;
}

void TdecoderOptionsPage::translate(void)
{
    TconfPageBase::translate();

    cbxTranslate(IDC_CBX_IDCT, Tlibavcodec::idctNames);
}

void TdecoderOptionsPage::getTip(char_t *tipS, size_t len)
{
    tsnprintf_s(tipS, len, _TRUNCATE, _l("IDCT: %s"), Tlibavcodec::idctNames[cfgGet(IDFF_idct)]);
    int bugs = cfgGet(IDFF_workaroundBugs);
    if (bugs && bugs != FF_BUG_AUTODETECT) {
        strncatf(tipS, len, _l("\nBugs workaround"));
    }
}

void TdecoderOptionsPage::onMatrixExport(void)
{
    uint8_t inter[64], intra[64];
    if (deciV->getQuantMatrices(intra, inter) != S_OK) {
        err(_(-IDD_DECODEROPTIONS, _l("No quantization matrices available.")));
        return;
    }
    TcurrentQuantDlg dlg(m_hwnd, deci, inter, intra);
    dlg.show();
}

TdecoderOptionsPage::TdecoderOptionsPage(TffdshowPageDec *Iparent): TconfPageDecVideo(Iparent)
{
    dialogId = IDD_DECODEROPTIONS;
    inPreset = 1;
    helpURL = _l("http://ffdshow-tryout.sourceforge.net/wiki/video:decoder_options");
    static const TbindCheckbox<TdecoderOptionsPage> chb[] = {
        IDC_CHB_DROP_ON_DELAY, IDFF_dropOnDelay, NULL,
        IDC_CHB_H264_SKIP_ON_DELAY, IDFF_h264skipOnDelay, NULL,
        IDC_CHB_SOFT_TELECINE, IDFF_softTelecine, NULL,
        0, NULL, NULL
    };
    bindCheckboxes(chb);
    static const TbindEditInt<TdecoderOptionsPage> edInt[] = {
        IDC_ED_NUMTHREADS, 1, 8, IDFF_numLAVCdecThreads, NULL,
        IDC_ED_DROP_ON_DELAY_TIME, 0, 20000, IDFF_dropOnDelayTime, NULL,
        IDC_ED_H264SKIP_ON_DELAY_TIME, 0, 20000, IDFF_h264skipOnDelayTime, NULL,
        0, NULL, NULL
    };
    bindEditInts(edInt);
    static const TbindCombobox<TdecoderOptionsPage> cbx[] = {
        IDC_CBX_IDCT, IDFF_idct, BINDCBX_SEL, NULL,
        0
    };
    bindComboboxes(cbx);
    static const TbindButton<TdecoderOptionsPage> bt[] = {
        IDC_BT_QUANTMATRIX_EXPORT, &TdecoderOptionsPage::onMatrixExport,
        0, NULL
    };
    bindButtons(bt);
}

