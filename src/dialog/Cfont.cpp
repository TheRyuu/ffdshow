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
#include "Cfont.h"
#include "TfontSettings.h"
#include "IffdshowParamsEnum.h"
#include "reg.h"
#include <string.h>

int CALLBACK TfontPage::EnumFamCallBackCharsets(CONST LOGFONT *lpelf, CONST TEXTMETRIC *lpntm, DWORD FontType, LPARAM lParam)
{
    if (FontType & (TRUETYPE_FONTTYPE | DEVICE_FONTTYPE)) {
        ints *sl = (ints*)lParam;
        ints::iterator i = std::find(sl->begin(), sl->end(), int(lpelf->lfCharSet));
        if (i == sl->end()) {
            sl->push_back(lpelf->lfCharSet);
        }
    }
    return 1;
}

int CALLBACK TfontPage::EnumFamCallBackFonts(CONST LOGFONT *lpelf, CONST TEXTMETRIC *lpntm, DWORD FontType, LPARAM lParam)
{
    if (FontType & (TRUETYPE_FONTTYPE | DEVICE_FONTTYPE)/* && lpelf->lfCharSet==0*/) {
        strings *sl = (strings*)lParam;
        strings::iterator i = std::find(sl->begin(), sl->end(), ffstring(lpelf->lfFaceName));
        if (i == sl->end()) {
            sl->push_back(lpelf->lfFaceName);
        }
    }
    return 1;
}

void TfontPage::init(void)
{
    tbrSetRange(IDC_TBR_FONT_SPACING, -10, 10, 2);
    tbrSetRange(IDC_TBR_FONT_XSCALE, 30, 300);
    tbrSetRange(IDC_TBR_FONT_YSCALE, 30, 300);
    tbrSetRange(IDC_TBR_FONT_SUBSHADOW_SIZE, 0, 20, 1);
    tbrSetRange(IDC_TBR_FONT_BODY_ALPHA, 0, 256, 10);
    tbrSetRange(IDC_TBR_FONT_OUTLINE_ALPHA, 0, 256, 10);
    tbrSetRange(IDC_TBR_FONT_SUBSHADOW_ALPHA, 0, 256, 10);
    tbrSetRange(IDC_TBR_FONT_OUTLINE_WIDTH, 0, 16, 1);

    strings sl;
    LOGFONT lf;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfPitchAndFamily = 0;
    lf.lfFaceName[0] = '\0';
    HDC dc = GetDC(m_hwnd);
    EnumFontFamiliesEx(dc, &lf, EnumFamCallBackFonts, LPARAM(&sl), 0);
    ReleaseDC(m_hwnd, dc);
    for (strings::const_iterator il = sl.begin(); il != sl.end(); il++) {
        cbxAdd(IDC_CBX_FONT_NAME, il->c_str());
    }

    cbxCharset = GetDlgItem(m_hwnd, IDC_CBX_FONT_CHARSET);
    boldFont = NULL;
    addHint(IDC_CBX_FONT_WEIGHT, _l("Sets font weight. Most fonts support only small subset of the listed weights"));
    addHint(IDC_TBR_FONT_SPACING, _l("Sets the distance between characters"));
    addHint(IDC_CHB_FONT_ASPECT_AUTO, _l("Corrects font scale when the video has non-square pixel aspect ratio"));
    addHint(IDC_CHB_FONT_AUTOSIZE, _l("Adjusts font size automatically so subtitles always have the same apparent size. Shadow size is scaled too, but it'll appear larger at lower resolutions"));
    if (idff_fontcharset == IDFF_fontCharset) {
        addHint(IDC_CHB_FONT_BLUR, _l("Controls if and where blur is applied\rUnchecked - Blur is not applied\rChecked - Blur is applied to the whole subtitle\rIndeterminate - Blur is applied to the subtitle's outline only"));
    } else {
        addHint(IDC_CHB_FONT_BLUR, _l("Controls if and where blur is applied\rUnchecked - Blur is not applied\rChecked - Blur is applied to the whole OSD\rIndeterminate - Blur is applied to the OSD's outline only"));
    }
    addHint(IDC_CHB_FONT_SETTINGS_OVERRIDE, _l("Overrides the following SSA/ASS font settings: name, charset, weight, italic, underline, spacing, scale X, scale Y, opaque box and blur"));
    addHint(IDC_CHB_FONT_SIZE_OVERRIDE, _l("Overrides SSA/ASS font size"));
    addHint(IDC_CHB_FONT_OUTLINEWIDTH_OVERRIDE, _l("Overrides SSA/ASS outline width"));
    addHint(IDC_CHB_FONT_SHADOW_OVERRIDE, _l("Overrides SSA/ASS shadow size"));
    addHint(IDC_CHB_FONT_COLOR_OVERRIDE, _l("Overrides SSA/ASS body, outline and shadow colors and alpha"));
    addHint(IDC_CHB_SCALEBORDERANDSHADOW_OVERRIDE, _l("In SSA/ASS subtitles, always scale outline width and shadow size"));
    addHint(IDC_CBX_BLUR_MODE, _l("Adjusts the amount of blur applied\rThe filter is a 3x3 box blur, this setting controls the weight of the center pixel compared to the others. Stronger values will lower the center pixel weight, thus blurring things more. Strong is the old non-configurable blur strength"));

    // Subtitles mode
    if (idff_fontcharset == IDFF_fontCharset) {
        show(true, IDC_CHB_FONT_SHADOW_OVERRIDE);
    } else {
        show(false, IDC_CHB_FONT_SHADOW_OVERRIDE);
    }

    if (idff_fontcharset == IDFF_fontCharset) {
        show(true, IDC_CHB_FONT_OUTLINEWIDTH_OVERRIDE);
    } else {
        show(false, IDC_CHB_FONT_OUTLINEWIDTH_OVERRIDE);
    }

    if (idff_fontcharset == IDFF_fontCharset) {
        show(true, IDC_CHB_FONT_SIZE_OVERRIDE);
    } else {
        show(false, IDC_CHB_FONT_SIZE_OVERRIDE);
    }

    if (idff_fontcharset == IDFF_fontCharset) {
        show(true, IDC_CHB_FONT_SETTINGS_OVERRIDE);
    } else {
        show(false, IDC_CHB_FONT_SETTINGS_OVERRIDE);
    }

    if (idff_fontcharset == IDFF_fontCharset) {
        show(true, IDC_CHB_FONT_COLOR_OVERRIDE);
    } else {
        show(false, IDC_CHB_FONT_COLOR_OVERRIDE);
    }

    if (idff_fontcharset == IDFF_fontCharset) {
        show(true, IDC_GRP_FONT_OVERRIDE);
    } else {
        show(false, IDC_GRP_FONT_OVERRIDE);
    }

    if (idff_fontcharset == IDFF_fontCharset) {
        show(true, IDC_CHB_SCALEBORDERANDSHADOW_OVERRIDE);
    } else {
        show(false, IDC_CHB_SCALEBORDERANDSHADOW_OVERRIDE);
    }
}

void TfontPage::selectCharset(int ii)
{
    int cnt = (int)SendMessage(cbxCharset, CB_GETCOUNT, 0, 0);
    for (int i = 0; i < cnt; i++) {
        int iii = (int)cbxGetItemData(IDC_CBX_FONT_CHARSET, i);
        if (ii == iii) {
            cbxSetCurSel(IDC_CBX_FONT_CHARSET, i);
            return;
        }
    }
    cbxSetCurSel(IDC_CBX_FONT_CHARSET, 0);
    cfgSet(idff_fontcharset, (int)cbxGetItemData(IDC_CBX_FONT_CHARSET, 0));
}

void TfontPage::fillCharsets(void)
{
    int oldi = cbxGetCurSel(IDC_CBX_FONT_CHARSET);
    int oldii = (oldi != CB_ERR) ? (int)cbxGetItemData(IDC_CBX_FONT_CHARSET, oldi) : ANSI_CHARSET;

    cbxClear(IDC_CBX_FONT_CHARSET);
    memset(validCharsets, 0, sizeof(validCharsets));
    ints sl;
    LOGFONT lf;
    cfgGet(idff_fontname, lf.lfFaceName, LF_FACESIZE);
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfPitchAndFamily = 0;
    HDC dc = GetDC(m_hwnd);
    EnumFontFamiliesEx(dc, &lf, EnumFamCallBackCharsets, LPARAM(&sl), 0);
    ReleaseDC(m_hwnd, dc);
    for (int i = 0; TfontSettings::charsets[i] != -1; i++) {
        int data = TfontSettings::charsets[i];
        if (isIn(sl, data)) {
            validCharsets[data] = true;
        }
        cbxAdd(IDC_CBX_FONT_CHARSET, _(IDC_CBX_FONT_CHARSET, TfontSettings::getCharset(data)), data);
    }
    selectCharset(oldii);
}

void TfontPage::cfg2dlg(void)
{
    font2dlg();
}

void TfontPage::font2dlg(void)
{
    size2dlg();
    shadow2dlg();
    spacingxscale2dlg();
    shadowSize2dlg();
    shadowAlpha2dlg();

    cbxSetDataCurSel(IDC_CBX_FONT_WEIGHT, cfgGet(idff_fontweight));
    SendDlgItemMessage(m_hwnd, IDC_CBX_FONT_NAME, CB_SELECTSTRING, WPARAM(-1), LPARAM(cfgGetStr(idff_fontname)));
    fillCharsets();
    setCheck(IDC_CHB_FONT_ASPECT_AUTO, cfgGet(idff_fontaspectauto));
    // Subtitles mode
    if (idff_fontcharset == IDFF_fontCharset) {
        setCheck(IDC_CHB_FONT_SHADOW_OVERRIDE, cfgGet(IDFF_fontShadowOverride));
    }

    if (idff_fontcharset == IDFF_fontCharset) {
        setCheck(IDC_CHB_FONT_OUTLINEWIDTH_OVERRIDE, cfgGet(IDFF_fontOutlineWidthOverride));
    }

    if (idff_fontcharset == IDFF_fontCharset) {
        setCheck(IDC_CHB_FONT_SIZE_OVERRIDE, cfgGet(IDFF_fontSizeOverride));
    }

    if (idff_fontcharset == IDFF_fontCharset) {
        setCheck(IDC_CHB_FONT_SETTINGS_OVERRIDE, cfgGet(IDFF_fontSettingsOverride));
    }

    if (idff_fontcharset == IDFF_fontCharset) {
        setCheck(IDC_CHB_FONT_COLOR_OVERRIDE, cfgGet(IDFF_fontColorOverride));
    }

    if (idff_fontcharset == IDFF_fontCharset) {
        setCheck(IDC_CHB_SCALEBORDERANDSHADOW_OVERRIDE, cfgGet(IDFF_scaleBorderAndShadowOverride));
    }

    selectCharset(cfgGet(idff_fontcharset));
    int opaquebox = cfgGet(idff_fontopaquebox);
    setCheck(IDC_FONT_OPAQUE_BOX, opaquebox);
    int fontBlur = cfgGet(idff_fontblur);
    setCheck3(IDC_CHB_FONT_BLUR, fontBlur);
    int italic = cfgGet(idff_fontitalic);
    setCheck(IDC_CHB_FONT_ITALIC, italic);
    int underline = cfgGet(idff_fontunderline);
    setCheck(IDC_CHB_FONT_UNDERLINE, underline);
    int shadowsize = cfgGet(idff_fontshadowsize);
    int shadowmode = cfgGet(idff_fontshadowmode);
    int blurmode = cfgGet(idff_fontblurmode);
    repaint(GetDlgItem(m_hwnd, IDC_IMG_FONT_COLOR));
    repaint(GetDlgItem(m_hwnd, IDC_IMG_FONT_COLOR_OUTLINE));
    repaint(GetDlgItem(m_hwnd, IDC_IMG_FONT_COLOR_SHADOW));
}
void TfontPage::size2dlg(void)
{
    int aut;
    if (idff_fontautosize) {
        aut = cfgGet(idff_fontautosize);
        setCheck(IDC_CHB_FONT_AUTOSIZE, aut);
        enable(1, IDC_CHB_FONT_AUTOSIZE);
    } else {
        aut = 0;
        enable(0, IDC_CHB_FONT_AUTOSIZE);
    }
    if (aut) {
        tbrSetRange(IDC_TBR_FONT_SIZE, 0, 100, 5);
        tbrSet(IDC_TBR_FONT_SIZE, cfgGet(idff_fontsizea), IDC_TXT_FONT_BODY);
        enable(1, IDC_CHB_FONT_AUTOSIZE_VIDEOWINDOW);
    } else {
        tbrSetRange(IDC_TBR_FONT_SIZE, 3, 127, 6);
        tbrSet(IDC_TBR_FONT_SIZE, cfgGet(idff_fontsizep), IDC_TXT_FONT_BODY);
        enable(0, IDC_CHB_FONT_AUTOSIZE_VIDEOWINDOW);
    }
    setCheck(IDC_CHB_FONT_AUTOSIZE_VIDEOWINDOW, cfgGet(IDFF_fontAutosizeVideoWindow));
}
void TfontPage::spacingxscale2dlg(void)
{
    if (idff_fontxscale) {
        int xscale = cfgGet(idff_fontxscale);
        tbrSet(IDC_TBR_FONT_XSCALE, cfgGet(idff_fontxscale));
        setText(IDC_LBL_FONT_XSCALE, _l("%s %i%%"), _(IDC_LBL_FONT_XSCALE), xscale);
    }
    if (idff_fontyscale) {
        int yscale = cfgGet(idff_fontyscale);
        tbrSet(IDC_TBR_FONT_YSCALE, cfgGet(idff_fontyscale));
        setText(IDC_LBL_FONT_YSCALE, _l("%s %i%%"), _(IDC_LBL_FONT_YSCALE), yscale);
    } else {
        show(0, IDC_TBR_FONT_YSCALE);
        show(0, IDC_LBL_FONT_YSCALE);
    }
    tbrSet(IDC_TBR_FONT_SPACING, cfgGet(idff_fontspacing), IDC_LBL_FONT_SPACING);
}

INT_PTR TfontPage::msgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_DESTROY:
            if (boldFont) {
                DeleteObject(boldFont);
            }
            boldFont = NULL;
            break;
        case WM_HSCROLL:
            switch (getId(HWND(lParam))) {
                case IDC_TBR_FONT_SIZE:
                    if (cfgGet(idff_fontautosize)) {
                        cfgSet(idff_fontsizea, tbrGet(IDC_TBR_FONT_SIZE));
                    } else {
                        cfgSet(idff_fontsizep, tbrGet(IDC_TBR_FONT_SIZE));
                    }
                    size2dlg();
                    return TRUE;
            }
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_CHB_FONT_AUTOSIZE:
                    if (idff_fontautosize) {
                        cfgSet(idff_fontautosize, getCheck(IDC_CHB_FONT_AUTOSIZE));
                        size2dlg();
                    }
                    return TRUE;
                case IDC_CHB_FONT_BLUR:
                    if (idff_fontcharset == IDFF_fontCharset) {
                        cfgSet(IDFF_fontBlur, getCheck3(IDC_CHB_FONT_BLUR));
                    } else {
                        cfgSet(idff_fontblur, getCheck3(IDC_CHB_FONT_BLUR));
                    }
                    return TRUE;
                case IDC_CHB_FONT_AUTOSIZE_VIDEOWINDOW:
                    if (idff_fontautosizevideowindow) {
                        cfgSet(idff_fontautosizevideowindow, getCheck(IDC_CHB_FONT_AUTOSIZE_VIDEOWINDOW));
                    }
                    return TRUE;
                case IDC_IMG_FONT_COLOR:
                    if (HIWORD(wParam) == STN_CLICKED) {
                        if (chooseColor(idff_fontcolor)) {
                            font2dlg();
                            repaint(GetDlgItem(m_hwnd, IDC_IMG_FONT_COLOR));
                        }
                        return TRUE;
                    }
                    break;
                case IDC_IMG_FONT_COLOR_OUTLINE:
                    if (HIWORD(wParam) == STN_CLICKED) {
                        if (chooseColor(idff_fontoutlinecolor)) {
                            font2dlg();
                            repaint(GetDlgItem(m_hwnd, IDC_IMG_FONT_COLOR_OUTLINE));
                        }
                        return TRUE;
                    }
                    break;
                case IDC_IMG_FONT_COLOR_SHADOW:
                    if (HIWORD(wParam) == STN_CLICKED) {
                        if (chooseColor(idff_fontshadowcolor)) {
                            font2dlg();
                            repaint(GetDlgItem(m_hwnd, IDC_IMG_FONT_COLOR_SHADOW));
                        }
                        return TRUE;
                    }
                    break;
            }
            break;
        case WM_DRAWITEM:
            if (wParam == IDC_IMG_FONT_COLOR) {
                LPDRAWITEMSTRUCT dis = LPDRAWITEMSTRUCT(lParam);
                LOGBRUSH lb;
                lb.lbColor = cfgGet(idff_fontcolor);
                lb.lbStyle = BS_SOLID;
                HBRUSH br = CreateBrushIndirect(&lb);
                FillRect(dis->hDC, &dis->rcItem, br);
                DeleteObject(br);
                return TRUE;
            } else if (wParam == IDC_IMG_FONT_COLOR_OUTLINE) {
                LPDRAWITEMSTRUCT dis = LPDRAWITEMSTRUCT(lParam);
                LOGBRUSH lb;
                lb.lbColor = cfgGet(idff_fontoutlinecolor);
                lb.lbStyle = BS_SOLID;
                HBRUSH br = CreateBrushIndirect(&lb);
                FillRect(dis->hDC, &dis->rcItem, br);
                DeleteObject(br);
                return TRUE;
            } else if (wParam == IDC_IMG_FONT_COLOR_SHADOW) {
                LPDRAWITEMSTRUCT dis = LPDRAWITEMSTRUCT(lParam);
                LOGBRUSH lb;
                lb.lbColor = cfgGet(idff_fontshadowcolor);
                lb.lbStyle = BS_SOLID;
                HBRUSH br = CreateBrushIndirect(&lb);
                FillRect(dis->hDC, &dis->rcItem, br);
                DeleteObject(br);
                return TRUE;
            } else if (wParam == IDC_CBX_FONT_CHARSET) {
                DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT*)lParam;
                COLORREF crOldTextColor = GetTextColor(dis->hDC);
                COLORREF crOldBkColor = GetBkColor(dis->hDC);
                HBRUSH br;
                if ((dis->itemAction | ODA_SELECT) && (dis->itemState & ODS_SELECTED)) {
                    SetTextColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
                    SetBkColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHT));
                    br = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
                } else {
                    br = CreateSolidBrush(crOldBkColor);
                }
                FillRect(dis->hDC, &dis->rcItem, br);
                if (int(dis->itemData) != CB_ERR) {
                    RECT r = dis->rcItem;
                    r.left += 2;
                    char_t text[70];
                    SendMessage(cbxCharset, CB_GETLBTEXT, dis->itemID, LPARAM(text));
                    if (!boldFont) {
                        LOGFONT oldFont;
                        HFONT hf = (HFONT)GetCurrentObject(dis->hDC, OBJ_FONT);
                        GetObject(hf, sizeof(LOGFONT), &oldFont);
                        oldFont.lfWeight = FW_BLACK;
                        boldFont = CreateFontIndirect(&oldFont);
                    }
                    HGDIOBJ oldfont = NULL;
                    if (validCharsets[dis->itemData]) {
                        oldfont = SelectObject(dis->hDC, boldFont);
                    }
                    DrawText(dis->hDC, text, (int)strlen(text), &r, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
                    if (oldfont) {
                        SelectObject(dis->hDC, oldfont);
                    }
                }
                SetTextColor(dis->hDC, crOldTextColor);
                SetBkColor(dis->hDC, crOldBkColor);
                DeleteObject(br);
                return TRUE;
            } else {
                break;
            }
    }
    return TconfPageDecVideo::msgProc(uMsg, wParam, lParam);
}

Twidget* TfontPage::createDlgItem(int id, HWND h)
{
    if (id == IDC_TBR_FONT_SIZE) {
        static const TbindTrackbar<TfontPageSubtitles> htbr = {IDC_TBR_FONT_SIZE, IDFF_fontSizeA, NULL};
        return new TwidgetSubclassTbr(h, this, TbindTrackbars(&htbr));
    } else {
        return TconfPageDecVideo::createDlgItem(id, h);
    }
}
int TfontPage::getTbrIdff(int id, const TbindTrackbars bind)
{
    return getCheck(IDC_CHB_FONT_AUTOSIZE) ? idff_fontsizea : idff_fontsizep;
}

bool TfontPage::reset(bool testonly)
{
    if (!testonly) {
        deci->resetParam(idff_fontname);
        deci->resetParam(idff_fontcharset);
        deci->resetParam(idff_fontautosize);
        deci->resetParam(idff_fontsizep);
        deci->resetParam(idff_fontsizea);
        deci->resetParam(idff_fontweight);
        deci->resetParam(idff_fontcolor);
        deci->resetParam(idff_fontbodyalpha);
        deci->resetParam(idff_fontspacing);
        deci->resetParam(idff_fontxscale);
        if (idff_fontyscale) {
            deci->resetParam(idff_fontyscale);
        }
        deci->resetParam(idff_fontfast);
        deci->resetParam(idff_fontoutlinewidth);
        deci->resetParam(idff_fontoutlinecolor);
        deci->resetParam(idff_fontoutlinealpha);
        deci->resetParam(idff_fontshadowmode);
        deci->resetParam(idff_fontblurmode);
        deci->resetParam(idff_fontshadowcolor);
        deci->resetParam(idff_fontshadowsize);
        deci->resetParam(idff_fontshadowalpha);
        deci->resetParam(idff_fontopaquebox);
        deci->resetParam(idff_fontblur);
        deci->resetParam(idff_fontitalic);
        deci->resetParam(idff_fontunderline);
        deci->resetParam(idff_fontaspectauto);
    }
    return true;
}

void TfontPage::translate(void)
{
    TconfPageBase::translate();

    int sel = cbxGetCurSel(IDC_CBX_FONT_WEIGHT);
    cbxClear(IDC_CBX_FONT_WEIGHT);
    for (int i = 0; TfontSettings::weights[i].name; i++) {
        cbxAdd(IDC_CBX_FONT_WEIGHT, _(IDC_CBX_FONT_WEIGHT, TfontSettings::weights[i].name), TfontSettings::weights[i].id);
    }
    cbxSetCurSel(IDC_CBX_FONT_WEIGHT, sel);

    cbxTranslate(IDC_CBX_FONT_SUBSHADOW_MODE, TfontSettings::shadowModes);
    cbxTranslate(IDC_CBX_BLUR_MODE, TfontSettings::blurModes);
}

void TfontPage::shadow2dlg(void)
{
    int shadowmode = cfgGet(idff_fontshadowmode);
    int blurmode = cfgGet(idff_fontblurmode);
    cbxSetCurSel(IDC_CBX_FONT_SUBSHADOW_MODE, shadowmode);
    cbxSetCurSel(IDC_CBX_BLUR_MODE, blurmode);
    static const int idShadows[] = {IDC_TBR_FONT_SUBSHADOW_ALPHA, IDC_TBR_FONT_SUBSHADOW_SIZE, 0};
    enable(shadowmode != 3, idShadows);
    int outlinewidth = cfgGet(idff_fontoutlinewidth);
    char_t s[256];
    tbrSet(IDC_TBR_FONT_OUTLINE_WIDTH, outlinewidth, IDC_TXT_FONT_OUTLINE);
    setDlgItemText(m_hwnd, IDC_LBL_FONT_OUTLINE_WIDTH, s);
    int bodyalpha = cfgGet(idff_fontbodyalpha);
}

void TfontPage::shadowSize2dlg(void)
{
    if (idff_fontshadowsize) {
        int shadowmode = cfgGet(idff_fontshadowmode);
        int subshadowsize = cfgGet(idff_fontshadowsize);
        tbrSet(IDC_TBR_FONT_SUBSHADOW_SIZE, cfgGet(idff_fontshadowsize), IDC_TXT_FONT_SHADOW);
    }
}

void TfontPage::shadowAlpha2dlg(void)
{
    tbrSet(IDC_TBR_FONT_SUBSHADOW_ALPHA, cfgGet(idff_fontshadowalpha));
    tbrSet(IDC_TBR_FONT_BODY_ALPHA, cfgGet(idff_fontbodyalpha));
    tbrSet(IDC_TBR_FONT_OUTLINE_ALPHA, cfgGet(idff_fontoutlinealpha));
}

TfontPage::TfontPage(TffdshowPageDec *Iparent, const TfilterIDFF *idff, int IfilterPageId): TconfPageDecVideo(Iparent, idff, IfilterPageId)
{
    dialogId = IDD_FONT;
}

//====================================== TfontPageSubtitles =====================================
TfontPageSubtitles::TfontPageSubtitles(TffdshowPageDec *Iparent, const TfilterIDFF *idff): TfontPage(Iparent, idff, 2)
{
    idff_fontcharset = IDFF_fontCharset;
    idff_fontname = IDFF_fontName;
    idff_fontautosize = IDFF_fontAutosize;
    idff_fontautosizevideowindow = IDFF_fontAutosizeVideoWindow;
    idff_fontsizep = IDFF_fontSizeP;
    idff_fontsizea = IDFF_fontSizeA;
    idff_fontspacing = IDFF_fontSpacing;
    idff_fontoutlinewidth = IDFF_fontOutlineWidth;
    idff_fontweight = IDFF_fontWeight;
    idff_fontcolor = IDFF_fontColor;
    idff_fontxscale = IDFF_fontXscale;
    idff_fontyscale = IDFF_fontYscale;
    idff_fontaspectauto = IDFF_fontAspectAuto;
    idff_fontfast = IDFF_fontFast;
    idff_fontshadowmode = IDFF_fontShadowMode;
    idff_fontblurmode = IDFF_fontBlurMode;
    idff_fontoutlinecolor = IDFF_fontOutlineColor;
    idff_fontshadowcolor = IDFF_fontShadowColor;
    idff_fontbodyalpha = IDFF_fontBodyAlpha;
    idff_fontoutlinealpha = IDFF_fontOutlineAlpha;
    idff_fontshadowalpha = IDFF_fontShadowAlpha;
    idff_fontshadowsize = IDFF_fontShadowSize;
    idff_fontshadowmode = IDFF_fontShadowMode;
    idff_fontopaquebox = IDFF_fontOpaqueBox;
    idff_fontblur = IDFF_fontBlur;
    idff_fontitalic = IDFF_fontItalic;
    idff_fontunderline = IDFF_fontUnderline;
    static const TbindCheckbox<TfontPageSubtitles> chb[] = {
        IDC_FONT_OPAQUE_BOX, idff_fontopaquebox, &TfontPageSubtitles::font2dlg,
        IDC_CHB_FONT_BLUR, idff_fontblur, &TfontPageSubtitles::font2dlg,
        IDC_CHB_FONT_ITALIC, idff_fontitalic, &TfontPageSubtitles::font2dlg,
        IDC_CHB_FONT_UNDERLINE, idff_fontunderline, &TfontPageSubtitles::font2dlg,
        IDC_CHB_FONT_ASPECT_AUTO, idff_fontaspectauto, &TfontPageSubtitles::font2dlg,
        IDC_CHB_FONT_SHADOW_OVERRIDE, IDFF_fontShadowOverride, &TfontPageSubtitles::font2dlg,
        IDC_CHB_FONT_OUTLINEWIDTH_OVERRIDE, IDFF_fontOutlineWidthOverride, &TfontPageSubtitles::font2dlg,
        IDC_CHB_FONT_SIZE_OVERRIDE, IDFF_fontSizeOverride, &TfontPageSubtitles::font2dlg,
        IDC_CHB_FONT_SETTINGS_OVERRIDE, IDFF_fontSettingsOverride, &TfontPageSubtitles::font2dlg,
        IDC_CHB_FONT_COLOR_OVERRIDE, IDFF_fontColorOverride, &TfontPageSubtitles::font2dlg,
        IDC_CHB_SCALEBORDERANDSHADOW_OVERRIDE, IDFF_scaleBorderAndShadowOverride, &TfontPageSubtitles::font2dlg,
        0, NULL, NULL
    };
    bindCheckboxes(chb);
    static const TbindTrackbar<TfontPageSubtitles> htbr[] = {
        IDC_TBR_FONT_SPACING, idff_fontspacing, &TfontPageSubtitles::spacingxscale2dlg,
        IDC_TBR_FONT_OUTLINE_WIDTH, idff_fontoutlinewidth, &TfontPageSubtitles::shadow2dlg,
        IDC_TBR_FONT_XSCALE, idff_fontxscale, &TfontPageSubtitles::spacingxscale2dlg,
        IDC_TBR_FONT_YSCALE, idff_fontyscale, &TfontPageSubtitles::spacingxscale2dlg,
        IDC_TBR_FONT_SUBSHADOW_SIZE, idff_fontshadowsize, &TfontPageSubtitles::font2dlg,
        IDC_TBR_FONT_BODY_ALPHA, idff_fontbodyalpha, &TfontPageSubtitles::shadowAlpha2dlg,
        IDC_TBR_FONT_OUTLINE_ALPHA, idff_fontoutlinealpha, &TfontPageSubtitles::shadowAlpha2dlg,
        IDC_TBR_FONT_SUBSHADOW_ALPHA, idff_fontshadowalpha, &TfontPageSubtitles::shadowAlpha2dlg,
        0, 0, NULL
    };
    bindHtracks(htbr);
    static const TbindCombobox<TfontPageSubtitles> cbx[] = {
        IDC_CBX_FONT_CHARSET, idff_fontcharset, BINDCBX_DATA, NULL,
        IDC_CBX_FONT_WEIGHT, idff_fontweight, BINDCBX_DATA, NULL,
        IDC_CBX_FONT_NAME, idff_fontname, BINDCBX_TEXT, &TfontPageSubtitles::fillCharsets,
        IDC_CBX_FONT_SUBSHADOW_MODE, idff_fontshadowmode, BINDCBX_SEL, &TfontPageSubtitles::font2dlg,
        IDC_CBX_BLUR_MODE, idff_fontblurmode, BINDCBX_SEL, &TfontPageSubtitles::font2dlg,
        0
    };
    bindComboboxes(cbx);
    show(true, IDC_CHB_FONT_SHADOW_OVERRIDE);
    show(true, IDC_CHB_FONT_OUTLINEWIDTH_OVERRIDE);
    show(true, IDC_CHB_FONT_SIZE_OVERRIDE);
    show(true, IDC_CHB_FONT_SETTINGS_OVERRIDE);
    show(true, IDC_CHB_FONT_COLOR_OVERRIDE);
    show(true, IDC_GRP_FONT_OVERRIDE);
    show(true, IDC_CHB_SCALEBORDERANDSHADOW_OVERRIDE);
}

//========================================= TfontPageOSD ========================================
TfontPageOSD::TfontPageOSD(TffdshowPageDec *Iparent, const TfilterIDFF *idff): TfontPage(Iparent, idff, 2)
{
    idff_fontcharset = IDFF_OSDfontCharset;
    idff_fontname = IDFF_OSDfontName;
    idff_fontautosize = 0;
    idff_fontautosizevideowindow = 0;
    idff_fontsizep = IDFF_OSDfontSize;
    idff_fontsizea = 0;
    idff_fontspacing = IDFF_OSDfontSpacing;
    idff_fontoutlinewidth = IDFF_OSDfontOutlineWidth;
    idff_fontweight = IDFF_OSDfontWeight;
    idff_fontcolor = IDFF_OSDfontColor;
    idff_fontxscale = IDFF_OSDfontXscale;
    idff_fontyscale = 0;
    idff_fontaspectauto = IDFF_OSDfontAspectAuto;
    idff_fontoutlinecolor = IDFF_OSDfontOutlineColor;
    idff_fontshadowcolor = IDFF_OSDfontShadowColor;
    idff_fontbodyalpha = IDFF_OSDfontBodyAlpha;
    idff_fontoutlinealpha = IDFF_OSDfontOutlineAlpha;
    idff_fontshadowalpha = IDFF_OSDfontShadowAlpha;
    idff_fontshadowsize = IDFF_OSDfontShadowSize;
    idff_fontshadowmode = IDFF_OSDfontShadowMode;
    idff_fontblurmode = IDFF_OSDfontBlurMode;
    idff_fontopaquebox = IDFF_OSDfontOpaqueBox;
    idff_fontblur = IDFF_OSDfontBlur;
    idff_fontitalic = IDFF_OSDfontItalic;
    idff_fontunderline = IDFF_OSDfontUnderline;
    static const TbindCheckbox<TfontPageOSD> chb[] = {
        IDC_FONT_OPAQUE_BOX, idff_fontopaquebox, &TfontPageOSD::font2dlg,
        IDC_CHB_FONT_BLUR, idff_fontblur, &TfontPageOSD::font2dlg,
        IDC_CHB_FONT_ITALIC, idff_fontitalic, &TfontPageOSD::font2dlg,
        IDC_CHB_FONT_UNDERLINE, idff_fontunderline, &TfontPageOSD::font2dlg,
        IDC_CHB_FONT_ASPECT_AUTO, idff_fontaspectauto, &TfontPageOSD::font2dlg,
        0, NULL, NULL
    };
    bindCheckboxes(chb);
    static const TbindTrackbar<TfontPageOSD> htbr[] = {
        IDC_TBR_FONT_SPACING, idff_fontspacing, &TfontPageOSD::spacingxscale2dlg,
        IDC_TBR_FONT_OUTLINE_WIDTH, idff_fontoutlinewidth, &TfontPageOSD::shadow2dlg,
        IDC_TBR_FONT_XSCALE, idff_fontxscale, &TfontPageOSD::spacingxscale2dlg,
        IDC_TBR_FONT_SUBSHADOW_SIZE, idff_fontshadowsize, &TfontPageOSD::font2dlg,
        IDC_TBR_FONT_BODY_ALPHA, idff_fontbodyalpha, &TfontPageOSD::shadowAlpha2dlg,
        IDC_TBR_FONT_OUTLINE_ALPHA, idff_fontoutlinealpha, &TfontPageOSD::shadowAlpha2dlg,
        IDC_TBR_FONT_SUBSHADOW_ALPHA, idff_fontshadowalpha, &TfontPageOSD::shadowAlpha2dlg,
        0, 0, NULL
    };
    bindHtracks(htbr);
    static const TbindCombobox<TfontPageOSD> cbx[] = {
        IDC_CBX_FONT_CHARSET, idff_fontcharset, BINDCBX_DATA, NULL,
        IDC_CBX_FONT_WEIGHT, idff_fontweight, BINDCBX_DATA, NULL,
        IDC_CBX_FONT_NAME, idff_fontname, BINDCBX_TEXT, &TfontPageOSD::fillCharsets,
        IDC_CBX_FONT_SUBSHADOW_MODE, idff_fontshadowmode, BINDCBX_SEL, &TfontPageOSD::font2dlg,
        IDC_CBX_BLUR_MODE, idff_fontblurmode, BINDCBX_SEL, &TfontPageOSD::font2dlg,
        0
    };
    bindComboboxes(cbx);
    show(false, IDC_CHB_FONT_SHADOW_OVERRIDE);
    show(false, IDC_CHB_FONT_OUTLINEWIDTH_OVERRIDE);
    show(false, IDC_CHB_FONT_SIZE_OVERRIDE);
    show(false, IDC_CHB_FONT_SETTINGS_OVERRIDE);
    show(false, IDC_CHB_FONT_COLOR_OVERRIDE);
    show(false, IDC_GRP_FONT_OVERRIDE);
    show(false, IDC_CHB_SCALEBORDERANDSHADOW_OVERRIDE);
}
