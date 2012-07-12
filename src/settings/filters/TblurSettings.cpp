/*
 * Copyright (c) 2003-2006 Milan Cutka
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
#include "TblurSettings.h"
#include "TimgFilterBlur.h"
#include "TimgFilterSmoother.h"
#include "TimgFilterTimesmooth.h"
#include "TimgFilterGradualDenoise.h"
#include "TimgFilterDenoise3d.h"
#include "Cblur.h"
#include "TffdshowPageDec.h"

const TfilterIDFF TblurSettings::idffs = {
    /*name*/      _l("Blur & NR"),
    /*id*/        IDFF_filterBlur,
    /*is*/        IDFF_isBlur,
    /*order*/     IDFF_orderBlur,
    /*show*/      IDFF_showBlur,
    /*full*/      IDFF_fullBlur,
    /*half*/      IDFF_halfBlur,
    /*dlgId*/     IDD_BLUR,
};

TblurSettings::TblurSettings(TintStrColl *Icoll, TfilterIDFFs *filters): TfilterSettingsVideo(sizeof(*this), Icoll, filters, &idffs)
{
    static const TintOptionT<TblurSettings> iopts[] = {
        IDFF_isBlur                 , &TblurSettings::is                      , 0, 0, _l(""), 1,
        _l("isBlur"), 0,
        IDFF_showBlur               , &TblurSettings::show                    , 0, 0, _l(""), 1,
        _l("showBlur"), 1,
        IDFF_orderBlur              , &TblurSettings::order                   , 1, 1, _l(""), 1,
        _l("orderBlur"), 0,
        IDFF_fullBlur               , &TblurSettings::full                    , 0, 0, _l(""), 1,
        _l("fullBlur"), 0,
        IDFF_halfBlur               , &TblurSettings::half                    , 0, 0, _l(""), 1,
        _l("halfBlur"), 0,
        IDFF_blurIsSoften           , &TblurSettings::isSoften                , 0, 0, _l(""), 1,
        _l("blurIsSoften"), 0,
        IDFF_blurStrength           , &TblurSettings::soften                  , 0, 255, _l(""), 1,
        _l("blurStrength"), 30,
        IDFF_blurIsTempSmooth       , &TblurSettings::isTempSmooth            , 0, 0, _l(""), 1,
        _l("blurIsTempSmooth"), 0,
        IDFF_tempSmooth             , &TblurSettings::tempSmooth              , 0, 10, _l(""), 1,
        _l("tempSmooth"), 0,
        IDFF_tempSmoothColor        , &TblurSettings::tempSmoothColor         , 0, 0, _l(""), 1,
        _l("tempSmoothColor"), 1,
        IDFF_blurIsSmoothLuma       , &TblurSettings::isSmoothLuma            , 0, 0, _l(""), 1,
        _l("blurIsSmoothLuma"), 0,
        IDFF_smoothStrengthLuma     , &TblurSettings::smoothStrengthLuma      , 0, 5000, _l(""), 1,
        _l("blurIsSmoothChroma"), 0,
        IDFF_blurIsSmoothChroma     , &TblurSettings::isSmoothChroma          , 0, 0, _l(""), 1,
        _l("smoothStrengthLuma"), 300,
        IDFF_smoothStrengthChroma   , &TblurSettings::smoothStrengthChroma    , 0, 5000, _l(""), 1,
        _l("smoothStrengthChroma"), 0,
        IDFF_blurIsGradual          , &TblurSettings::isGradual               , 0, 0, _l(""), 1,
        _l("blurIsGradual"), 1,
        IDFF_gradualStrength        , &TblurSettings::gradualStrength         , 5, 100, _l(""), 1,
        _l("gradualStrength"), 40,
        IDFF_blurIsAvcodecTNR       , &TblurSettings::isAvcodecTNR            , 0, 0, _l(""), 1,
        _l("blurIsAvcodecTNR"), 0,
        IDFF_avcodecTNR1            , &TblurSettings::avcodecTNR1             , -3, -3, _l(""), 1,
        _l("avcodecTNR1"), 700,
        IDFF_avcodecTNR2            , &TblurSettings::avcodecTNR2             , -3, -3, _l(""), 1,
        _l("avcodecTNR2"), 1500,
        IDFF_avcodecTNR3            , &TblurSettings::avcodecTNR3             , -3, -3, _l(""), 1,
        _l("avcodecTNR3"), 3000,
        IDFF_blurIsAvcodec          , &TblurSettings::isAvcodecBlur           , 0, 0, _l(""), 1,
        _l("blurIsAvcodecBLur"), 0,
        IDFF_avcodecBlurRadius      , &TblurSettings::avcodecBlurRadius       , 1, 9, _l(""), 1,
        _l("avcodecBlurRadius"), 3,
        IDFF_avcodecBlurLuma        , &TblurSettings::avcodecBlurLuma         , 0, 400, _l(""), 1,
        _l("avcodecBlurLuma"), 100,
        IDFF_avcodecBlurChroma      , &TblurSettings::avcodecBlurChroma       , 0, 400, _l(""), 1,
        _l("avcodecBlurChroma"), 150,
        IDFF_blurIsDenoise3d        , &TblurSettings::isDenoise3d             , 0, 0, _l(""), 1,
        _l("isDenoise3d"), 0,
        IDFF_denoise3Dluma          , &TblurSettings::denoise3Dluma           , 0, 2000, _l(""), 1,
        _l("denoise3Dluma"), 400,
        IDFF_denoise3Dchroma        , &TblurSettings::denoise3Dchroma         , 0, 2000, _l(""), 1,
        _l("denoise3Dchroma"), 300,
        IDFF_denoise3Dtime          , &TblurSettings::denoise3Dtime           , 0, 2000, _l(""), 1,
        _l("denoise3Dtime"), 600,
        IDFF_denoise3Dhq            , &TblurSettings::denoise3Dhq             , 0, 0, _l(""), 1,
        _l("denoise3Dhq"), 0,
        0
    };
    addOptions(iopts);
}

void TblurSettings::getMinMax(int id, int &min, int &max)
{
    switch (id) {
        case IDFF_avcodecTNR1:
            min = 1;
            max = avcodecTNR2;
            return;
        case IDFF_avcodecTNR2:
            min = avcodecTNR1;
            max = avcodecTNR3;
            return;
        case IDFF_avcodecTNR3:
            min = avcodecTNR2;
            max = 5000;
            return;
    }
}
void TblurSettings::createFilters(size_t filtersorder, Tfilters *filters, TfilterQueue &queue) const
{
    idffOnChange(idffs, filters, queue.temporary);
    if (is && show) {
        if (!queue.temporary) {
            setOnChange(IDFF_blurIsSoften, filters, &Tfilters::onQueueChange);
            setOnChange(IDFF_blurIsTempSmooth, filters, &Tfilters::onQueueChange);
            setOnChange(IDFF_blurIsSmoothLuma, filters, &Tfilters::onQueueChange);
            setOnChange(IDFF_blurIsSmoothChroma, filters, &Tfilters::onQueueChange);
            setOnChange(IDFF_blurIsGradual, filters, &Tfilters::onQueueChange);
            setOnChange(IDFF_blurIsAvcodecTNR, filters, &Tfilters::onQueueChange);
            setOnChange(IDFF_blurIsAvcodec, filters, &Tfilters::onQueueChange);
            setOnChange(IDFF_blurIsDenoise3d, filters, &Tfilters::onQueueChange);
            setOnChange(IDFF_denoise3Dhq, filters, &Tfilters::onQueueChange);
        }
        if (isSoften) {
            queueFilter<TimgFilterBlur>(filtersorder, filters, queue);
        }
        if (isSmoothLuma) {
            queueFilter<TimgFilterSmootherLuma>(filtersorder, filters, queue);
        }
        if (isSmoothChroma) {
            queueFilter<TimgFilterSmootherChroma>(filtersorder, filters, queue);
        }
        if (isTempSmooth) {
            queueFilter<TimgFilterTimesmooth>(filtersorder, filters, queue);
        }
        if (isGradual) {
            queueFilter<TimgFilterGradualDenoise>(filtersorder, filters, queue);
        }
        if (isAvcodecTNR) {
            queueFilter<TimgFilterAvcodecTNR>(filtersorder, filters, queue);
        }
        if (isAvcodecBlur) {
            queueFilter<TimgFilterAvcodecBlur>(filtersorder, filters, queue);
        }
        if (isDenoise3d)
            if (denoise3Dhq) {
                queueFilter<TimgFilterDenoise3dHQ>(filtersorder, filters, queue);
            } else {
                queueFilter<TimgFilterDenoise3dNormal>(filtersorder, filters, queue);
            }
    }
}
void TblurSettings::createPages(TffdshowPageDec *parent) const
{
    parent->addFilterPage<TblurPage>(&idffs);
}

const int* TblurSettings::getResets(unsigned int pageId)
{
    static const int idResets[] = {
        IDFF_blurStrength,
        IDFF_tempSmooth, IDFF_tempSmoothColor,
        IDFF_gradualStrength,
        IDFF_avcodecTNR1, IDFF_avcodecTNR2, IDFF_avcodecTNR3,
        IDFF_avcodecBlurRadius, IDFF_avcodecBlurLuma, IDFF_avcodecBlurChroma,
        IDFF_denoise3Dluma, IDFF_denoise3Dchroma, IDFF_denoise3Dtime, IDFF_denoise3Dhq,
        0
    };
    return idResets;
}

bool TblurSettings::getTip(unsigned int pageId, char_t *tipS, size_t len)
{
    char_t tip[1000] = _l("");
    if (isSoften) {
        tsprintf(tip, _l("blur: %i\n"), soften);
    }
    if (isTempSmooth) {
        strncatf(tip, countof(tip), _l("temporal smooth: %i\n"), tempSmooth);
    }
    if (isSmoothLuma) {
        strncatf(tip, countof(tip), _l("luminance smoother: %i\n"), smoothStrengthLuma);
    }
    if (isSmoothChroma) {
        strncatf(tip, countof(tip), _l("chroma smoother %i\n"), smoothStrengthChroma);
    }
    if (isGradual) {
        strncatf(tip, countof(tip), _l("gradual denoise: %i\n"), gradualStrength);
    }
    if (isAvcodecTNR) {
        strncatf(tip, countof(tip), _l("avcodec TNR: %i,%i,%i\n"), avcodecTNR1, avcodecTNR2, avcodecTNR3);
    }
    if (isAvcodecBlur) {
        strncatf(tip, countof(tip), _l("swscaler gaussian blur - radius:%i, luma:%.2f, chroma:%.2f\n"), avcodecBlurRadius, avcodecBlurLuma / 100.0f, avcodecBlurChroma / 100.0f);
    }
    if (isDenoise3d) {
        strncatf(tip, countof(tip), _l("denoise 3d %s - luma:%.2f, chroma:%.2f, time:%.2f"), denoise3Dhq ? _l("hq") : _l(""), denoise3Dluma / 100.0f, denoise3Dchroma / 100.0f, denoise3Dtime / 100.0f);
    }
    if (tip[strlen(tip) - 1] == '\n') {
        tip[strlen(tip) - 1] = '\0';
    }
    ff_strncpy(tipS, tip, len);
    return true;
}
