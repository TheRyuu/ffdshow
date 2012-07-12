/*
 * Copyright (c) 2007 h.yamagata
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "stdafx.h"
#include "CcspOptions.h"
#include "Ttranslate.h"
#include "TrgbPrimaries.h"
#include "ToutputVideoSettings.h"
#include "Tconfig.h"

void TcspOptionsPage::init(void)
{
    tbrSetRange(IDC_TBR_RGBCONV_BLACK, 0, 32, 1);
    tbrSetRange(IDC_TBR_RGBCONV_WHITE, 215, 255, 1);
    tbrSetRange(IDC_TBR_RGBCONV_CHROMA, 1, 32, 1);

    addHint(IDC_RBT_BT_AUTO, _l("Use the information from the stream (H.264 only).\nIf it isn't available,\n  width > 1024 or height >=600: BT.709\n  width <=1024 and height < 600: BT.601\n (BT.709 will always be used for Fraps, BT.601 will always be used for JPEG, MJPEG)"));
    addHint(IDC_RBT_BT601, _l("DVD, NTSC, PAL and SD-TV/videos use this."));
    addHint(IDC_RBT_BT709, _l("Blu-ray and HD-TV use this."));
    addHint(IDC_CBX_RGB_INTERLACE_METHOD, _l("This setting also applies to YV12 <-> YUY2 conversion."));
    addHint(IDC_RBT_YCbCr_input_levels_auto, L"Use information from the stream if available. Otherwise same as standard.");
    addHint(IDC_RBT_YCbCr_input_levels_16_to_235, L"Nearly all videos use this.");
    addHint(IDC_RBT_YCbCr_input_levels_0_to_255, _l("JPEG, MJPEG and Fraps sources usually use this"));
    addHint(IDC_RBT_CSP_OUTPUT_LEVELS_TV, _l("Most TV and Projectors use this.\nConsult the manual of your device."));
}

void TcspOptionsPage::cfg2dlg(void)
{
    int iturbt = cfgGet(IDFF_cspOptionsIturBt);
    setCheck(IDC_RBT_BT601,  iturbt == TrgbPrimaries::ITUR_BT601);
    setCheck(IDC_RBT_BT709,  iturbt == TrgbPrimaries::ITUR_BT709);
    setCheck(IDC_RBT_BT_AUTO, iturbt == TrgbPrimaries::ITUR_BT_AUTO);
    int mode = cfgGet(IDFF_cspOptionsInputLevelsMode);
    setCheck(IDC_RBT_YCbCr_input_levels_auto,         mode == TrgbPrimaries::AutoYCbCr);
    setCheck(IDC_RBT_YCbCr_input_levels_16_to_235,    mode == TrgbPrimaries::RecYCbCr);
    setCheck(IDC_RBT_YCbCr_input_levels_0_to_255,     mode == TrgbPrimaries::PcYCbCr);
    setCheck(IDC_RBT_CUSTOM_YUV, mode == TrgbPrimaries::CutomYCbCr);
    int modeOut = cfgGet(IDFF_cspOptionsOutputLevelsMode);
    setCheck(IDC_RBT_CSP_OUTPUT_LEVELS_PC, modeOut == TrgbPrimaries::PcRGB);
    setCheck(IDC_RBT_CSP_OUTPUT_LEVELS_TV, modeOut == TrgbPrimaries::TvRGB);

    int blackCutoff  = cfgGet(IDFF_cspOptionsBlackCutoff);
    int whiteCutoff  = cfgGet(IDFF_cspOptionsWhiteCutoff);
    int chromaCutoff = cfgGet(IDFF_cspOptionsChromaCutoff);
    tbrSet(IDC_TBR_RGBCONV_BLACK,  blackCutoff,  IDC_TXT_RGBCONV_BLACK);
    tbrSet(IDC_TBR_RGBCONV_WHITE,  whiteCutoff,  IDC_TXT_RGBCONV_WHITE);
    tbrSet(IDC_TBR_RGBCONV_CHROMA, chromaCutoff, IDC_TXT_RGBCONV_CHROMA);

    char_t customRange[256];
    int fixedChromaCutoff = ToutputVideoSettings::get_cspOptionsChromaCutoffStatic(blackCutoff, whiteCutoff, chromaCutoff);
    ffstring str(tr->translate(m_hwnd, dialogId, IDC_RBT_CUSTOM_YUV, NULL));
    strncpyf(customRange, countof(customRange), _l(" (Y: %d-%d, CbCr: %d-%d)"), blackCutoff, whiteCutoff, fixedChromaCutoff, 256 - fixedChromaCutoff);
    str += customRange;
    setText(IDC_RBT_CUSTOM_YUV, str.c_str());

    static const int customs[] = {IDC_TBR_RGBCONV_BLACK,  IDC_TXT_RGBCONV_BLACK,
                                  IDC_TBR_RGBCONV_WHITE,  IDC_TXT_RGBCONV_WHITE,
                                  IDC_TBR_RGBCONV_CHROMA, IDC_TXT_RGBCONV_CHROMA,
                                  0
                                 };
    enable(mode == TrgbPrimaries::CutomYCbCr, customs);

    cbxSetCurSel(IDC_CBX_RGB_INTERLACE_METHOD, cfgGet(IDFF_cspOptionsRgbInterlaceMode));
    int highQualityRGB = cfgGet(IDFF_highQualityRGB);
    setCheck(IDC_CHB_HIGH_QUALITY_RGB, highQualityRGB);
    setCheck(IDC_CHB_RGB_DITHER, cfgGet(IDFF_RGB_dithering));
    enable(highQualityRGB && (Tconfig::cpu_flags & FF_CPU_SSE2), IDC_CHB_RGB_DITHER);
}

void TcspOptionsPage::getTip(char_t *tipS, size_t len)
{
    const char_t *tip = _(-IDD_CSP_OPTIONS, _l("YCbCr <-> RGB conversion options"));
    ff_strncpy(tipS, tip, len);
}

void TcspOptionsPage::translate(void)
{
    TconfPageDecVideo::translate();

    cbxTranslate(IDC_CBX_RGB_INTERLACE_METHOD, ToutputVideoSettings::rgbInterlaceMethods);
}

bool TcspOptionsPage::reset(bool testonly)
{
    if (!testonly) {
        deci->resetParam(IDFF_cspOptionsIturBt);
        deci->resetParam(IDFF_cspOptionsInputLevelsMode);
        deci->resetParam(IDFF_cspOptionsOutputLevelsMode);
        deci->resetParam(IDFF_cspOptionsBlackCutoff);
        deci->resetParam(IDFF_cspOptionsWhiteCutoff);
        deci->resetParam(IDFF_cspOptionsChromaCutoff);
        deci->resetParam(IDFF_cspOptionsRgbInterlaceMode);
        deci->resetParam(IDFF_RGB_dithering);
        deci->resetParam(IDFF_highQualityRGB);
    }
    return true;
}

TcspOptionsPage::TcspOptionsPage(TffdshowPageDec *Iparent): TconfPageDecVideo(Iparent)
{
    dialogId = IDD_CSP_OPTIONS;
    helpURL = _l("http://ffdshow-tryout.sourceforge.net/wiki/video:rgb_conversion");
    inPreset = 1;
    idffOrder = maxOrder + 4;
    static const TbindRadiobutton<TcspOptionsPage> rbt[] = {
        IDC_RBT_BT_AUTO,                      IDFF_cspOptionsIturBt,          TrgbPrimaries::ITUR_BT_AUTO, &TcspOptionsPage::cfg2dlg,
        IDC_RBT_BT601,                        IDFF_cspOptionsIturBt,          TrgbPrimaries::ITUR_BT601,   &TcspOptionsPage::cfg2dlg,
        IDC_RBT_BT709,                        IDFF_cspOptionsIturBt,          TrgbPrimaries::ITUR_BT709,   &TcspOptionsPage::cfg2dlg,
        IDC_RBT_YCbCr_input_levels_auto,      IDFF_cspOptionsInputLevelsMode, TrgbPrimaries::AutoYCbCr,    &TcspOptionsPage::cfg2dlg,
        IDC_RBT_YCbCr_input_levels_16_to_235, IDFF_cspOptionsInputLevelsMode, TrgbPrimaries::RecYCbCr,     &TcspOptionsPage::cfg2dlg,
        IDC_RBT_YCbCr_input_levels_0_to_255,  IDFF_cspOptionsInputLevelsMode, TrgbPrimaries::PcYCbCr,      &TcspOptionsPage::cfg2dlg,
        IDC_RBT_CUSTOM_YUV,                   IDFF_cspOptionsInputLevelsMode, TrgbPrimaries::CutomYCbCr,   &TcspOptionsPage::cfg2dlg,
        IDC_RBT_CSP_OUTPUT_LEVELS_PC,         IDFF_cspOptionsOutputLevelsMode, TrgbPrimaries::PcRGB,        &TcspOptionsPage::cfg2dlg,
        IDC_RBT_CSP_OUTPUT_LEVELS_TV,         IDFF_cspOptionsOutputLevelsMode, TrgbPrimaries::TvRGB,        &TcspOptionsPage::cfg2dlg,
        0, 0, 0, NULL
    };
    bindRadioButtons(rbt);
    static const TbindTrackbar<TcspOptionsPage> htbr[] = {
        IDC_TBR_RGBCONV_BLACK,  IDFF_cspOptionsBlackCutoff,  &TcspOptionsPage::cfg2dlg,
        IDC_TBR_RGBCONV_WHITE,  IDFF_cspOptionsWhiteCutoff,  &TcspOptionsPage::cfg2dlg,
        IDC_TBR_RGBCONV_CHROMA, IDFF_cspOptionsChromaCutoff, &TcspOptionsPage::cfg2dlg,
        0, 0, NULL
    };
    bindHtracks(htbr);
    static const TbindCheckbox<TcspOptionsPage> chb[] = {
        IDC_CHB_HIGH_QUALITY_RGB, IDFF_highQualityRGB, &TcspOptionsPage::cfg2dlg,
        IDC_CHB_RGB_DITHER, IDFF_RGB_dithering, NULL,
        0, NULL, NULL
    };
    bindCheckboxes(chb);
    static const TbindCombobox<TcspOptionsPage> cbx[] = {
        IDC_CBX_RGB_INTERLACE_METHOD, IDFF_cspOptionsRgbInterlaceMode, BINDCBX_SEL, NULL,
        0
    };
    bindComboboxes(cbx);
}
