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
#include "Coutcsps.h"
#include "Tlibavcodec.h"
#include "ToutputVideoSettings.h"
#include "Ttranslate.h"

const int ToutcspsPage::idcs [] = {IDC_CHB_OUT_YV12, IDC_CHB_OUT_YUY2, IDC_CHB_OUT_UYVY, IDC_CHB_OUT_NV12, IDC_CHB_OUT_RGB32, IDC_CHB_OUT_RGB24, IDC_CHB_OUT_P016, IDC_CHB_OUT_P010, IDC_CHB_OUT_P210, IDC_CHB_OUT_P216, IDC_CHB_OUT_AYUV, IDC_CHB_OUT_Y416, 0};
const int ToutcspsPage::idffs[] = {IDFF_outYV12    , IDFF_outYUY2    , IDFF_outUYVY    , IDFF_outNV12    , IDFF_outRGB32    , IDFF_outRGB24    , IDFF_outP016    , IDFF_outP010    , IDFF_outP210    , IDFF_outP216    , IDFF_outAYUV    , IDFF_outY416};

#define TOUTCSPSPAGE_RECONNECTABLE_FILTERS _l("  Overlay Mixer\n  VMR\n  VMR9\n  VobSub\n  Haali's Video Renderer\n  EVR\n  MadVR\n  ffdshow")

void ToutcspsPage::init()
{
    addHint(IDC_CHB_HWDEINTERLACE,     _l("Send interlacing related information obtained from the input stream or ffdshow's internal decoders to the next filter. ")
            _l("Some filters (like video renderers) will use this information to deinterlace the video if neccessary. ")
            _l("Enabling ffdshow's internal deinterlacing filter will automatically disable this option, to avoid deinterlacing the video twice.\n\n")
            _l("This is just for informing the downstream filters - the actual result will depend purely on the implementation of these filters."));

    addHint(IDC_CBX_OUT_HWDEINT_METHOD, _l("Auto: according to source flags, bob for interlaced frames, weave for progressive frames.\n")
            _l("Force weave: weave for all frames, regardless of source flags.\n")
            _l("Force bob: bob for all frames, regardless of source flags."));
    addHint(IDC_CBX_OUT_PRIMARY_CSP, L"The selected color space is used whenever possible. Otherwise one of the enabled color spaces from below is used. Warning: forcing a preferred color space can cause extra conversions and performance loss.");
}
void ToutcspsPage::cfg2dlg()
{
    csp2dlg();
    setCheck(IDC_CHB_FLIP, cfgGet(IDFF_flip));
    overlay2dlg();
}

void ToutcspsPage::csp2dlg()
{
    setCheck(IDC_CHB_OUT_YV12 , cfgGet(IDFF_outYV12));
    setCheck(IDC_CHB_OUT_YUY2 , cfgGet(IDFF_outYUY2));
    setCheck(IDC_CHB_OUT_UYVY , cfgGet(IDFF_outUYVY));
    setCheck(IDC_CHB_OUT_NV12 , cfgGet(IDFF_outNV12));
    setCheck(IDC_CHB_OUT_RGB32, cfgGet(IDFF_outRGB32));
    setCheck(IDC_CHB_OUT_RGB24, cfgGet(IDFF_outRGB24));
    setCheck(IDC_CHB_OUT_P016 , cfgGet(IDFF_outP016));
    setCheck(IDC_CHB_OUT_P010 , cfgGet(IDFF_outP010));
    setCheck(IDC_CHB_OUT_P210 , cfgGet(IDFF_outP210));
    setCheck(IDC_CHB_OUT_P216 , cfgGet(IDFF_outP216));
    setCheck(IDC_CHB_OUT_AYUV , cfgGet(IDFF_outAYUV));
    setCheck(IDC_CHB_OUT_Y416 , cfgGet(IDFF_outY416));
    int i = cfgGet(IDFF_outPrimaryCSP);
    if (i > 0) {
        uint64_t csp = csp_reg2ffdshow(i);
        const TcspInfo *cspInfo = csp_getInfo(csp);
        int j = 0;
        while (ToutputVideoSettings::primaryColorSpaces[j]) {
            if (ToutputVideoSettings::primaryColorSpaces[j] == cspInfo->id) {
                break;
            }
            j++;
        };
        if (ToutputVideoSettings::primaryColorSpaces[j]) {
            i = j + 1;
        } else {
            i = 0;
        }
    } else {
        i = 0; // Auto
    }
    cbxSetCurSel(IDC_CBX_OUT_PRIMARY_CSP, i);
}

void ToutcspsPage::dlg2primaryCsp()
{
    int i = cbxGetCurSel(IDC_CBX_OUT_PRIMARY_CSP);
    if (i > 0) {
        uint64_t csp = ToutputVideoSettings::primaryColorSpaces[--i];
        i = csp_ffdshow2reg(csp);
    }
    cfgSet(IDFF_outPrimaryCSP, i);
}

void ToutcspsPage::overlay2dlg()
{
    int enabledHW = (filterMode & IDFF_FILTERMODE_VFW) == 0;
    int hwdeint = cfgGet(IDFF_setDeintInOutSample);
    int hwdeintmethod = cfgGet(IDFF_hwDeintMethod);
    setCheck(IDC_CHB_HWDEINTERLACE, hwdeint);
    enable(enabledHW, IDC_CHB_HWDEINTERLACE);
    cbxSetCurSel(IDC_CBX_OUT_HWDEINT_METHOD, cfgGet(IDFF_hwDeintMethod));
    cbxSetCurSel(IDC_CBX_OUT_HWDEINT_FIELDORDER, cfgGet(IDFF_hwDeintFieldOrder));
    enable(enabledHW && hwdeint, IDC_CBX_OUT_HWDEINT_METHOD);
    enable(enabledHW && hwdeint, IDC_CBX_OUT_HWDEINT_FIELDORDER);
}
INT_PTR ToutcspsPage::msgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_CHB_HWDEINTERLACE:
                    cfgSet(IDFF_setDeintInOutSample, getCheck(IDC_CHB_HWDEINTERLACE));
                    overlay2dlg();
                    csp2dlg();
                    return TRUE;
                case IDC_CHB_OUT_YV12:
                case IDC_CHB_OUT_YUY2:
                case IDC_CHB_OUT_UYVY:
                case IDC_CHB_OUT_NV12:
                case IDC_CHB_OUT_P016:
                case IDC_CHB_OUT_P010:
                case IDC_CHB_OUT_P210:
                case IDC_CHB_OUT_P216:
                case IDC_CHB_OUT_AYUV:
                case IDC_CHB_OUT_Y416:
                case IDC_CHB_OUT_RGB24:
                case IDC_CHB_OUT_RGB32: {
                    int ch[countof(idcs)], dv = false;
                    int is = 0;
                    for (int i = 0; i < countof(idcs); i++) {
                        is |= ch[i] = getCheck(idcs[i]);
                    }
                    if (is) {
                        for (int i = 0; i < countof(idcs); i++) {
                            cfgSet(idffs[i], ch[i]);
                        }
                    } else {
                        setCheck(LOWORD(wParam), !getCheck(LOWORD(wParam)));
                    }
                    csp2dlg();
                    overlay2dlg();
                    return TRUE;
                }
                case IDC_CBX_OUT_PRIMARY_CSP:
                    dlg2primaryCsp();
                    return TRUE;
            }
            break;
    }
    return TconfPageDecVideo::msgProc(uMsg, wParam, lParam);
}

void ToutcspsPage::getTip(char_t *tipS, size_t len)
{
    if (cfgGet(IDFF_flip)) {
        strcpy(tipS, _l("Flip video"));
    } else {
        tipS[0] = '\0';
    }
}

void ToutcspsPage::translate()
{
    TconfPageDecVideo::translate();

    cbxTranslate(IDC_CBX_OUT_HWDEINT_METHOD, ToutputVideoSettings::deintMethods);
    cbxTranslate(IDC_CBX_OUT_HWDEINT_FIELDORDER, ToutputVideoSettings::deintFieldOrder);
    cbxAdd(IDC_CBX_OUT_PRIMARY_CSP, L"Auto");
    int i = 0;
    while (ToutputVideoSettings::primaryColorSpaces[i]) {
        const TcspInfo *cspInfo = csp_getInfo(ToutputVideoSettings::primaryColorSpaces[i]);
        cbxAdd(IDC_CBX_OUT_PRIMARY_CSP, _(IDC_CBX_OUT_PRIMARY_CSP, cspInfo->name));
        i++;
    };
}

ToutcspsPage::ToutcspsPage(TffdshowPageDec *Iparent): TconfPageDecVideo(Iparent)
{
    backupDV = NULL;
    dialogId = IDD_OUTCSPS;
    helpURL = _l("http://ffdshow-tryout.sourceforge.net/wiki/video:output");
    inPreset = 1;
    idffOrder = maxOrder + 3;
    filterID = IDFF_filterOutputVideo;
    static const TbindCheckbox<ToutcspsPage> chb[] = {
        IDC_CHB_FLIP, IDFF_flip, NULL,
        0, NULL, NULL
    };
    bindCheckboxes(chb);
    static const TbindCombobox<ToutcspsPage> cbx[] = {
        IDC_CBX_OUT_HWDEINT_METHOD, IDFF_hwDeintMethod, BINDCBX_SEL, NULL,
        IDC_CBX_OUT_HWDEINT_FIELDORDER, IDFF_hwDeintFieldOrder, BINDCBX_SEL, NULL,
        0
    };
    bindComboboxes(cbx);
}
