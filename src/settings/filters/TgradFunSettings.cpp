/*
 * Copyright (c) 2006 Milan Cutka
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
#include "TgradFunSettings.h"
#include "TimgFilterGradFun.h"
#include "CgradFun.h"
#include "TffdshowPageDec.h"

const TfilterIDFF TgradFunSettings::idffs = {
    /*name*/      _l("Deband"),
    /*id*/        IDFF_filterGradFun,
    /*is*/        IDFF_isGradFun,
    /*order*/     IDFF_orderGradFun,
    /*show*/      IDFF_showGradFun,
    /*full*/      0,
    /*half*/      0,
    /*dlgId*/     IDD_GRADFUN,
};

TgradFunSettings::TgradFunSettings(TintStrColl *Icoll, TfilterIDFFs *filters): TfilterSettingsVideo(sizeof(*this), Icoll, filters, &idffs)
{
    static const TintOptionT<TgradFunSettings> iopts[] = {
        IDFF_isGradFun       , &TgradFunSettings::is       , 0, 0, _l(""), 1,
        _l("isGradFun"), 0,
        IDFF_showGradFun     , &TgradFunSettings::show     , 0, 0, _l(""), 1,
        _l("showGradFun"), 1,
        IDFF_orderGradFun    , &TgradFunSettings::order    , 1, 1, _l(""), 1,
        _l("orderGradFun"), 0,
        IDFF_gradFunThreshold, &TgradFunSettings::threshold, 51, 2000, _l(""), 1,
        _l("gradFunThreshold"), 120,
        IDFF_gradFunRadius, &TgradFunSettings::radius, 4, 32, _l(""), 1,
        _l("gradFunRadius"), 16,

        0
    };
    addOptions(iopts);
}

void TgradFunSettings::createFilters(size_t filtersorder, Tfilters *filters, TfilterQueue &queue) const
{
    idffOnChange(idffs, filters, queue.temporary);
    if (is && show) {
        queueFilter<TimgFilterGradfun>(filtersorder, filters, queue);
    }
}
void TgradFunSettings::createPages(TffdshowPageDec *parent) const
{
    parent->addFilterPage<TgradFunPage>(&idffs);
}

const int* TgradFunSettings::getResets(unsigned int pageId)
{
    static const int idResets[] = {IDFF_gradFunThreshold, IDFF_gradFunRadius, 0};
    return idResets;
}

bool TgradFunSettings::getTip(unsigned int pageId, char_t *tipS, size_t len)
{
    tsnprintf_s(tipS, len, _TRUNCATE, _l("threshold: %.2f, radius: %d"), threshold / 100.0f, radius);
    return true;
}
