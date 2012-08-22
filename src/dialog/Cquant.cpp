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
#include "Cquant.h"

const char_t* TquantPage::dct_algos[] = {
    _l("automatic"),
    _l("Fastint"),
    _l("Int"),
    _l("MMX"),
    _l("FAAN"),
    NULL
};
const char_t* TquantPage::qnss[] = {
    _l("disable"),
    _l("1"),
    _l("2"),
    _l("3"),
    NULL
};
const char_t* TquantPage::dcPrecisions[] = {_l("8 bits"), _l("9 bits"), _l("10 bits"), _l("11 bits"), NULL};

bool TquantPage::enabled(void)
{
    return sup_quantProps(codecId);
}

void TquantPage::cfg2dlg(void)
{
    int is = deciE->isQuantControlActive();

    SetDlgItemInt(m_hwnd, IDC_ED_Q_I_MIN, cfgGet(IDFF_enc_q_i_min), FALSE);
    SetDlgItemInt(m_hwnd, IDC_ED_Q_I_MAX, cfgGet(IDFF_enc_q_i_max), FALSE);
    setText(IDC_ED_I_QUANTFACTOR, _l("%.2f"), float(cfgGet(IDFF_enc_i_quant_factor) / 100.0));
    setText(IDC_ED_I_QUANTOFFSET, _l("%.2f"), float(cfgGet(IDFF_enc_i_quant_offset) / 100.0));
    static const int idIquants[] = {IDC_LBL_Q_I, IDC_ED_Q_I_MIN, IDC_ED_Q_I_MAX, 0};
    setDlgItemText(m_hwnd, IDC_LBL_Q_P, _(IDC_LBL_Q_P, _l("P frames")));
    enable(is && 1, idIquants);
    static const int idIoffset[] = {IDC_LBL_I_QUANTFACTOR, IDC_LBL_I_QUANTOFFSET, IDC_ED_I_QUANTFACTOR, IDC_ED_I_QUANTOFFSET, 0};
    enable(lavc_codec(codecId) && codecId != AV_CODEC_ID_MJPEG, idIoffset);

    bias2dlg();

    static const int idDCT[] = {IDC_LBL_DCT_ALGO, IDC_CBX_DCT_ALGO, 0};
    cbxSetCurSel(IDC_CBX_DCT_ALGO, cfgGet(IDFF_enc_dct_algo));
    enable(sup_lavcQuant(codecId), idDCT);

    qns2dlg();
}

void TquantPage::bias2dlg(void)
{
    int is = sup_quantBias(codecId);
    setCheck(IDC_CHB_QUANT_INTER_BIAS, cfgGet(IDFF_enc_isInterQuantBias));
    enable(is, IDC_CHB_QUANT_INTER_BIAS);
    SetDlgItemInt(m_hwnd, IDC_ED_QUANT_INTER_BIAS, cfgGet(IDFF_enc_interQuantBias), TRUE);
    enable(is && cfgGet(IDFF_enc_isInterQuantBias), IDC_ED_QUANT_INTER_BIAS);
    setCheck(IDC_CHB_QUANT_INTRA_BIAS, cfgGet(IDFF_enc_isIntraQuantBias));
    enable(is, IDC_CHB_QUANT_INTRA_BIAS);
    SetDlgItemInt(m_hwnd, IDC_ED_QUANT_INTRA_BIAS, cfgGet(IDFF_enc_intraQuantBias), TRUE);
    enable(is && cfgGet(IDFF_enc_isIntraQuantBias), IDC_ED_QUANT_INTRA_BIAS);
}
void TquantPage::qns2dlg(void)
{
    cbxSetCurSel(IDC_CBX_QNS, cfgGet(IDFF_enc_qns));
    int is = sup_qns(codecId);
    static const int idQns[] = {IDC_LBL_QNS, IDC_CBX_QNS, 0};
    enable(is, idQns);
}

INT_PTR TquantPage::msgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_ED_Q_I_MIN:
                case IDC_ED_Q_I_MAX:
                case IDC_ED_Q_P_MIN:
                case IDC_ED_Q_P_MAX:
                case IDC_ED_Q_B_MIN:
                case IDC_ED_Q_B_MAX:
                case IDC_ED_Q_MB_MIN:
                case IDC_ED_Q_MB_MAX:
                    if (HIWORD(wParam) == EN_CHANGE && !isSetWindowText) {
                        HWND hed = GetDlgItem(m_hwnd, LOWORD(wParam));
                        if (hed != GetFocus()) {
                            return FALSE;
                        }
                        repaint(hed);
                        switch (LOWORD(wParam)) {
                            case IDC_ED_Q_I_MIN :
                                eval(hed, parent->qmin, parent->qmax, IDFF_enc_q_i_min);
                                break;
                            case IDC_ED_Q_I_MAX :
                                eval(hed, parent->qmin, parent->qmax, IDFF_enc_q_i_max);
                                break;
                        }
                        return TRUE;
                    }
                    break;
            }
            break;
        case WM_CTLCOLOREDIT: {
            HWND hwnd = HWND(lParam);
            bool ok;
            switch (getId(hwnd)) {
                case IDC_ED_Q_I_MIN:
                case IDC_ED_Q_I_MAX:
                    ok = eval(hwnd, parent->qmin, parent->qmax);
                    break;
                default:
                    goto colorEnd;
            }
            if (!ok) {
                HDC dc = HDC(wParam);
                SetBkColor(dc, RGB(255, 0, 0));
                return INT_PTR(getRed());
            } else {
                return FALSE;
            }
colorEnd:
            ;
        }
    }
    return TconfPageEnc::msgProc(uMsg, wParam, lParam);
}

void TquantPage::translate(void)
{
    TconfPageEnc::translate();

    cbxTranslate(IDC_CBX_DCT_ALGO, dct_algos);
    cbxTranslate(IDC_CBX_QNS, qnss);
    cbxTranslate(IDC_CBX_QUANT_DCPRECISION, dcPrecisions);
}

TquantPage::TquantPage(TffdshowPageEnc *Iparent): TconfPageEnc(Iparent)
{
    dialogId = IDD_QUANT;
    static const int props[] = {IDFF_enc_q_i_min, IDFF_enc_q_i_max, IDFF_enc_i_quant_factor, IDFF_enc_i_quant_offset, IDFF_enc_isInterQuantBias, IDFF_enc_interQuantBias, IDFF_enc_isIntraQuantBias, IDFF_enc_intraQuantBias, IDFF_enc_dct_algo, 0};
    propsIDs = props;
    helpURL = _l("Quantization.html");
    static const TbindCheckbox<TquantPage> chb[] = {
        IDC_CHB_QUANT_INTER_BIAS, IDFF_enc_isInterQuantBias, &TquantPage::bias2dlg,
        IDC_CHB_QUANT_INTRA_BIAS, IDFF_enc_isIntraQuantBias, &TquantPage::bias2dlg,
        0, NULL, NULL
    };
    bindCheckboxes(chb);
    static const TbindEditInt<TquantPage> edInt[] = {
        IDC_ED_QUANT_INTER_BIAS, -512, 512, IDFF_enc_interQuantBias, NULL,
        IDC_ED_QUANT_INTRA_BIAS, -512, 512, IDFF_enc_intraQuantBias, NULL,
        0
    };
    bindEditInts(edInt);
    static const TbindEditReal<TquantPage> edReal[] = {
        IDC_ED_I_QUANTFACTOR, -31.0, 31.0, IDFF_enc_i_quant_factor, 100.0, NULL,
        IDC_ED_I_QUANTOFFSET, -31.0, 31.0, IDFF_enc_i_quant_offset, 100.0, NULL,
        0
    };
    bindEditReals(edReal);
    static const TbindCombobox<TquantPage> cbx[] = {
        IDC_CBX_DCT_ALGO, IDFF_enc_dct_algo, BINDCBX_SEL, NULL,
        0
    };
    bindComboboxes(cbx);
}
