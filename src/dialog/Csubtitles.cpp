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
#include "Csubtitles.h"
#include "TsubtitlesFile.h"
#include "TsubtitlesSettings.h"
#include "TffdshowPageDec.h"
#include "Ttranslate.h"

void TsubtitlesPage::init(void)
{
    SendDlgItemMessage(m_hwnd, IDC_CBX_SUB_FLNM, CB_LIMITTEXT, MAX_PATH, 0);
    edLimitText(IDC_ED_SUB_SEARCH_DIR, MAX_PATH);
    edLimitText(IDC_ED_SUB_SEARCH_EXT, MAX_PATH);
    autosubfirsttime = true;
    addHint(IDC_ED_SUB_SEARCH_EXT, _l("ffdshow searches subtitle files in the folders which are configured in the edit box above.\nFor video.avi, ffdshow searches video.ass, video.ssa,... and use the file which is found at the first time.\nEnumerate extensions in the order you like and separate them by semicolons.\n\nass;ssa;srt;idx;sub;smi;rt;txt;aqt;mpl;sup;utf is the default settings"));
    addHint(IDC_CHB_SUB_EMBEDDED_PRIORITY, _l("If embedded subtitles are present, use them instead of any detected subtitle file"));
    setFont(IDC_BT_SUBTITLES_EXPAND, parent->arrowsFont);
}

static const int idEmbedded[] = {IDC_CHB_SUBCC, IDC_CHB_SUBTEXT_SSA, IDC_CHB_VOBSUB, IDC_CHB_BLURAY, 0};

void TsubtitlesPage::cfg2dlg(void)
{
    sub2dlg();
    enable(filterMode & IDFF_FILTERMODE_PLAYER, IDC_BT_SUBTITLES_RESET);
    setCheck(IDC_CHB_SUB_WATCH, cfgGet(IDFF_subWatch));
    setCheck(IDC_CHB_SUB_EMBEDDED_PRIORITY, cfgGet(IDFF_subEmbeddedPriority));
    static const int idEmbedd[] = {IDC_CHB_SUBTEXTPIN, IDC_CHB_SUBCC, IDC_CHB_SUBTEXT_SSA, IDC_CHB_VOBSUB, IDC_CHB_BLURAY, 0};
    setCheck(IDC_CHB_SUBTEXTPIN, cfgGet(IDFF_subTextpin));
    setCheck(IDC_CHB_SUBCC, cfgGet(IDFF_subCC));
    setCheck(IDC_CHB_SUBTEXT_SSA, cfgGet(IDFF_subSSA));
    setCheck(IDC_CHB_VOBSUB, cfgGet(IDFF_subVobsub));
    setCheck(IDC_CHB_BLURAY, cfgGet(IDFF_subPGS));
    setCheck(IDC_CHB_SUBTEXT, cfgGet(IDFF_subText));
    setCheck(IDC_CHB_SUBTITLES_FILES, cfgGet(IDFF_subFiles));
    //enable(getCheck(IDC_CHB_SUBTEXTPIN), idEmbedded, FALSE);
    enable((filterMode & IDFF_FILTERMODE_VFW) == 0, idEmbedd);
}
void TsubtitlesPage::sub2dlg(void)
{
    const char_t *subfilename = cfgGetStr(IDFF_subFilename);
    if (SendDlgItemMessage(m_hwnd, IDC_CBX_SUB_FLNM, CB_SELECTSTRING, WPARAM(-1), LPARAM(subfilename)) == CB_ERR) {
        SendDlgItemMessage(m_hwnd, IDC_CBX_SUB_FLNM, CB_INSERTSTRING, 0, LPARAM(subfilename));
        cbxSetCurSel(IDC_CBX_SUB_FLNM, 0);
    }
    int autoflnm = cfgGet(IDFF_subAutoFlnm);
    setCheck(IDC_RBT_SUB_SEARCHDIR, autoflnm);
    setCheck(IDC_RBT_SUB_FLNM, !autoflnm);

    setDlgItemText(m_hwnd, IDC_ED_SUB_SEARCH_DIR, cfgGetStr(IDFF_subSearchDir));
    setDlgItemText(m_hwnd, IDC_ED_SUB_SEARCH_EXT, cfgGetStr(IDFF_subSearchExt));

    SetDlgItemInt(m_hwnd, IDC_ED_SUB_DELAY , cfgGet(IDFF_subDelay), TRUE);
    SetDlgItemInt(m_hwnd, IDC_ED_SUB_SPEED , cfgGet(IDFF_subSpeed), FALSE);
    SetDlgItemInt(m_hwnd, IDC_ED_SUB_SPEED2, cfgGet(IDFF_subSpeed2), FALSE);

    auto2dlg();
}
void TsubtitlesPage::auto2dlg(void)
{
    int a = getCheck(IDC_RBT_SUB_SEARCHDIR);
    static const int idFile[] = {IDC_CBX_SUB_FLNM, IDC_BT_SUB_LOADFILE, 0};
    enable(!a, idFile);
    setCheck(IDC_CHB_SUB_SEARCHHEURISTIC, cfgGet(IDFF_subSearchHeuristic));
    static const int idSearch[] = {IDC_ED_SUB_SEARCH_DIR, IDC_BT_SUB_SEARCHDIR, IDC_CHB_SUB_SEARCHHEURISTIC, 0};
    static const int idSearchExt[] = {IDC_ED_SUB_SEARCH_EXT, IDC_TXT_SERCH_ORDER, 0};
    enable(a, idSearch);
    int b = getCheck(IDC_CHB_SUB_SEARCHHEURISTIC);
    enable(a && !b, idSearchExt);
    if (a && (filterMode & IDFF_FILTERMODE_PLAYER)) {
        const char_t *autosubflnm = deciV->findAutoSubflnm3();
        /*
           if (autosubfirsttime)
            {
             autosubfirsttime=false;
             Tsubtitles::findPossibleSubtitles(deci->getSourceName(),autosubfiles);
            }
           if (!autosubfiles.empty())
            {
             //enable(1,IDC_CBX_SUB_FLNM);
             //SendDlgItemMessage(m_hwnd,IDC_CBX_SUB_FLNM,CB_RESETCONTENT,0,0);
             //for (strings::const_iterator f=autosubfiles.begin();f!=autosubfiles.end();f++)
             // cbxAdd(IDC_CBX_SUB_FLNM,f->c_str());
             */
        setDlgItemText(m_hwnd, IDC_CBX_SUB_FLNM, autosubflnm);
        //}
    }
}



void TsubtitlesPage::applySettings(void)
{
    loadSubtitles();
}

void TsubtitlesPage::loadSubtitles(void)
{
    if (!cfgGet(IDFF_subAutoFlnm)) {
        char_t subflnm[1024];
        GetDlgItemText(m_hwnd, IDC_CBX_SUB_FLNM, subflnm, 1023);
        cfgSet(IDFF_subFilename, subflnm);
    }
    sub2dlg();
}

INT_PTR TsubtitlesPage::msgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                    /*case IDC_CHB_SUBTEXTPIN:
                    {
                     enable(getCheck(IDC_CHB_SUBTEXTPIN), idEmbedded, FALSE);
                     break;
                    }*/
                case IDC_CBX_SUB_FLNM:
                    if (HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == CBN_EDITUPDATE) {
                        parent->setChange();
                    }
                    return TRUE;
                case IDC_BT_SUBTITLES_RESET:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        deciV->resetSubtitleTimes();
                        return TRUE;
                    }
                    break;
                case IDC_ED_SUB_SEARCH_DIR:
                    if (HIWORD(wParam) == EN_CHANGE && !isSetWindowText) {
                        char_t sdir[MAX_PATH];
                        GetDlgItemText(m_hwnd, IDC_ED_SUB_SEARCH_DIR, sdir, MAX_PATH);
                        cfgSet(IDFF_subSearchDir, sdir);
                        return TRUE;
                    }
                    break;
                case IDC_ED_SUB_SEARCH_EXT:
                    if (HIWORD(wParam) == EN_CHANGE && !isSetWindowText) {
                        char_t sext[MAX_PATH];
                        GetDlgItemText(m_hwnd, IDC_ED_SUB_SEARCH_EXT, sext, MAX_PATH);
                        cfgSet(IDFF_subSearchExt, sext);
                        return TRUE;
                    }
                    break;
                case IDC_BT_SUB_LOAD:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        deci->notifyParamStr(IDFF_subFilename, _l(""));
                        return TRUE;
                    }
                    break;
            }
        case WM_CTLCOLOREDIT: {
            HWND hwnd = HWND(lParam);
            bool ok;
            switch (getId(hwnd)) {
                case IDC_ED_SUBTITLES_EXPAND_X:
                case IDC_ED_SUBTITLES_EXPAND_Y:
                    ok = eval(hwnd, 1, 10000);
                    break;
                default:
                    goto endColor;
            }
            if (!ok) {
                HDC dc = HDC(wParam);
                SetBkColor(dc, RGB(255, 0, 0));
                return INT_PTR(getRed());
            } else {
                return FALSE;
            }
endColor:
            ;
        }
    }
    return TconfPageDecVideo::msgProc(uMsg, wParam, lParam);
}
void TsubtitlesPage::onLoadfile(void)
{
    ffstring dir;
    extractfilepath(cfgGetStr(IDFF_subFilename), dir);

    char_t subflnm[MAX_PATH] = _l("");
    if (dlgGetFile(false, m_hwnd, _(-IDD_SUBTITLES, _l("Load subtitles file")), TsubtitlesFile::mask, _l("txt"), subflnm, dir.c_str(), 0)) {
        setDlgItemText(m_hwnd, IDC_CBX_SUB_FLNM, subflnm);
        parent->setChange();
    }
}
void TsubtitlesPage::onSearchdir(void)
{
    char_t dir[MAX_PATH] = _l("");
    if (dlgGetDir(m_hwnd, dir, _(-IDD_SUBTITLES, _l("Select directory where subtitles are stored")))) {
        char_t sdir[2 * MAX_PATH];
        cfgGet(IDFF_subSearchDir, sdir, 2 * MAX_PATH);
        strncat_s(sdir, countof(sdir), _l(";"), _TRUNCATE);
        strncat_s(sdir, countof(sdir), dir, _TRUNCATE);
        cfgSet(IDFF_subSearchDir, sdir);
        sub2dlg();
    }
}



void TsubtitlesPage::translate(void)
{
    TconfPageBase::translate();
}

TsubtitlesPage::TsubtitlesPage(TffdshowPageDec *Iparent, const TfilterIDFF *idff): TconfPageDecVideo(Iparent, idff, 1)
{
    resInter = IDC_CHB_SUBTITLES;
    helpURL = _l("http://ffdshow-tryout.sourceforge.net/wiki/video:subtitles");
    static const TbindCheckbox<TsubtitlesPage> chb[] = {
        IDC_CHB_SUB_WATCH, IDFF_subWatch, NULL,
        IDC_CHB_SUB_EMBEDDED_PRIORITY, IDFF_subEmbeddedPriority, NULL,
        IDC_CHB_SUBTEXTPIN, IDFF_subTextpin, NULL,
        IDC_CHB_SUB_SEARCHHEURISTIC, IDFF_subSearchHeuristic, &TsubtitlesPage::auto2dlg,
        IDC_CHB_SUBCC, IDFF_subCC, &TsubtitlesPage::cfg2dlg,
        IDC_CHB_SUBTEXT_SSA, IDFF_subSSA, NULL,
        IDC_CHB_VOBSUB, IDFF_subVobsub, NULL,
        IDC_CHB_BLURAY, IDFF_subPGS, NULL,
        IDC_CHB_SUBTEXT, IDFF_subText, NULL,
        IDC_CHB_SUBTITLES_FILES, IDFF_subFiles, NULL,
        0, NULL, NULL
    };
    bindCheckboxes(chb);
    static const TbindRadiobutton<TsubtitlesPage> rbt[] = {
        IDC_RBT_SUB_SEARCHDIR, IDFF_subAutoFlnm, 1, &TsubtitlesPage::sub2dlg,
        IDC_RBT_SUB_FLNM, IDFF_subAutoFlnm, 0, &TsubtitlesPage::sub2dlg,
        0, 0, 0, NULL
    };
    bindRadioButtons(rbt);
    static const TbindEditInt<TsubtitlesPage> edInt[] = {
        IDC_ED_SUB_DELAY , -INT_MAX / 2, INT_MAX / 2, IDFF_subDelay, NULL,
        IDC_ED_SUB_SPEED , 1, INT_MAX / 2, IDFF_subSpeed , NULL,
        IDC_ED_SUB_SPEED2, 1, INT_MAX / 2, IDFF_subSpeed2, NULL,
        0
    };
    bindEditInts(edInt);
    static const TbindButton<TsubtitlesPage> bt[] = {
        IDC_BT_SUB_LOADFILE, &TsubtitlesPage::onLoadfile,
        IDC_BT_SUB_SEARCHDIR, &TsubtitlesPage::onSearchdir,
        0, NULL
    };
    bindButtons(bt);
}

