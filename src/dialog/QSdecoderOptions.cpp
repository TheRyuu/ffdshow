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
#include "QSdecoderOptions.h"
#include "Tlibavcodec.h"
#include "TvideoCodec.h"
#include "Ttranslate.h"
#include "ffmpeg/libavcodec/avcodec.h"

void TQSdecoderOptionsPage::init(void)
{
}

void TQSdecoderOptionsPage::cfg2dlg(void)
{
    // Update GUI from config
    setCheck(IDC_QS_ENABLE_TS_CORR     , cfgGet(IDFF_QS_ENABLE_TS_CORR     ));
    setCheck(IDC_QS_ENABLE_MT          , cfgGet(IDFF_QS_ENABLE_MT          ));
    setCheck(IDC_QS_FORCE_FIELD_ORDER  , cfgGet(IDFF_QS_FORCE_FIELD_ORDER  ));
    setCheck(IDC_QS_ENABLE_SW_EMULATION, cfgGet(IDFF_QS_ENABLE_SW_EMULATION));
    setCheck(IDC_QS_ENABLE_DVD_DECODE  , cfgGet(IDFF_QS_ENABLE_DVD_DECODE  ));
    setCheck(IDC_QS_ENABLE_DI          , cfgGet(IDFF_QS_ENABLE_DI          ));
    setCheck(IDC_QS_FORCE_DI           , cfgGet(IDFF_QS_FORCE_DI           ));
    setCheck(IDC_QS_ENABLE_FULL_RATE   , cfgGet(IDFF_QS_ENABLE_FULL_RATE   ));
    
    cbxSetCurSel(IDC_QS_FIELD_ORDER, cfgGet(IDFF_QS_FIELD_ORDER));

    detail2dlg();
    denoise2dlg();
}

void TQSdecoderOptionsPage::detail2dlg(void)
{
    tbrSetRange(IDC_QS_DETAIL, 0, 64, 1);
    tbrSet(IDC_QS_DETAIL, cfgGet(IDFF_QS_DETAIL), IDC_QS_DETAIL_LBL);
}

void TQSdecoderOptionsPage::denoise2dlg(void)
{
    tbrSetRange(IDC_QS_DENOISE, 0, 64, 1);
    tbrSet(IDC_QS_DENOISE, cfgGet(IDFF_QS_DENOISE), IDC_QS_DENOISE_LBL);
}

bool TQSdecoderOptionsPage::reset(bool testonly)
{
    if (!testonly) {
        deci->resetParam(IDFF_QS_ENABLE_TS_CORR     );
        deci->resetParam(IDFF_QS_ENABLE_MT          );
        deci->resetParam(IDFF_QS_FIELD_ORDER        );
        deci->resetParam(IDFF_QS_ENABLE_SW_EMULATION);
        deci->resetParam(IDFF_QS_FORCE_FIELD_ORDER  );
        deci->resetParam(IDFF_QS_ENABLE_DVD_DECODE  );
        deci->resetParam(IDFF_QS_ENABLE_DI          );
        deci->resetParam(IDFF_QS_FORCE_DI           );
        deci->resetParam(IDFF_QS_ENABLE_FULL_RATE   );
        deci->resetParam(IDFF_QS_DETAIL             );
        deci->resetParam(IDFF_QS_DENOISE            );
    }
    return true;
}

void TQSdecoderOptionsPage::translate(void)
{
    static const char_t* fieldOrderNames[]={
        _l("Auto"),
        _l("Top Field First"),
        _l("Bottom Field First"),
        NULL
    };

    TconfPageBase::translate();

    cbxTranslate(IDC_QS_FIELD_ORDER, fieldOrderNames);
}

void TQSdecoderOptionsPage::getTip(char_t *tipS,size_t len)
{
    tsnprintf_s(tipS, len, _TRUNCATE, _l("Intel\xae  QuickSync Decoder properties"));
}

TQSdecoderOptionsPage::TQSdecoderOptionsPage(TffdshowPageDec *Iparent):TconfPageDecVideo(Iparent)
{
    dialogId=IDD_INTEL_QS_DECODEROPTIONS;
    inPreset=1;
    //helpURL = _l("http://ffdshow-tryout.sourceforge.net/wiki/video:decoder_options");

    static const TbindCheckbox<TQSdecoderOptionsPage> chb[]= {
        IDC_QS_ENABLE_TS_CORR     , IDFF_QS_ENABLE_TS_CORR     , NULL,
        IDC_QS_ENABLE_MT          , IDFF_QS_ENABLE_MT          , NULL,
        IDC_QS_ENABLE_SW_EMULATION, IDFF_QS_ENABLE_SW_EMULATION, NULL,
        IDC_QS_FORCE_FIELD_ORDER  , IDFF_QS_FORCE_FIELD_ORDER  , NULL,
        IDC_QS_ENABLE_DVD_DECODE  , IDFF_QS_ENABLE_DVD_DECODE  , NULL,
        IDC_QS_ENABLE_DI          , IDFF_QS_ENABLE_DI          , NULL,
        IDC_QS_FORCE_DI           , IDFF_QS_FORCE_DI           , NULL,
        IDC_QS_ENABLE_FULL_RATE   , IDFF_QS_ENABLE_FULL_RATE   , NULL,
        0,NULL,NULL
    };
    bindCheckboxes(chb);

    static const TbindTrackbar<TQSdecoderOptionsPage> htbr[]= {
        IDC_QS_DETAIL , IDFF_QS_DETAIL , &TQSdecoderOptionsPage::detail2dlg,
        IDC_QS_DENOISE, IDFF_QS_DENOISE, &TQSdecoderOptionsPage::denoise2dlg,
        0,0,NULL
    };
    bindHtracks(htbr);

    static const TbindCombobox<TQSdecoderOptionsPage> cbx[]= {
        IDC_QS_FIELD_ORDER, IDFF_QS_FIELD_ORDER, BINDCBX_SEL, NULL,
        0
    };
    bindComboboxes(cbx);
}
