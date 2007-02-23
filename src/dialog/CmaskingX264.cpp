/*
 * Copyright (c) 2005,2006 Milan Cutka
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
#include "CmaskingX264.h"

bool TmaskingPageX264::enabled(void)
{
 return codecId==CODEC_ID_X264;
}

void TmaskingPageX264::init(void)
{
 tbrSetRange(IDC_TBR_X264_AQ_STRENGTH,1,100);
 tbrSetRange(IDC_TBR_X264_AQ_SENSITIVITY,1,30);
}

void TmaskingPageX264::cfg2dlg(void)
{
 int is=cfgGet(IDFF_enc_x264_is_aq);
 setCheck(IDC_CHB_X264_AQ,is);
 tbrSet(IDC_TBR_X264_AQ_STRENGTH,cfgGet(IDFF_enc_x264_aq_strength100),IDC_LBL_X264_AQ_STRENGTH);
 tbrSet(IDC_TBR_X264_AQ_SENSITIVITY,cfgGet(IDFF_enc_x264_f_aq_sensitivity),IDC_LBL_X264_AQ_SENSITIVITY);
 static const int idAQ[]={IDC_TBR_X264_AQ_STRENGTH,IDC_LBL_X264_AQ_STRENGTH,IDC_TBR_X264_AQ_SENSITIVITY,IDC_LBL_X264_AQ_SENSITIVITY,0};
 enable(is,idAQ);
}

TmaskingPageX264::TmaskingPageX264(TffdshowPageEnc *Iparent):TconfPageEnc(Iparent)
{
 dialogId=IDD_MASKING_X264;
 static const int props[]={IDFF_enc_x264_is_aq,IDFF_enc_x264_aq_strength100,IDFF_enc_x264_f_aq_sensitivity,0};
 propsIDs=props;
 static const TbindCheckbox<TmaskingPageX264> chb[]=
  {
   IDC_CHB_X264_AQ,IDFF_enc_x264_is_aq,&TmaskingPageX264::cfg2dlg,
   0,NULL,NULL
  };
 bindCheckboxes(chb);
 static const TbindTrackbar<TmaskingPageX264> htbr[]=
  {
   IDC_TBR_X264_AQ_STRENGTH,IDFF_enc_x264_aq_strength100,&TmaskingPageX264::cfg2dlg,
   IDC_TBR_X264_AQ_SENSITIVITY,IDFF_enc_x264_f_aq_sensitivity,&TmaskingPageX264::cfg2dlg,
   0,0,NULL
  };
 bindHtracks(htbr);
}
