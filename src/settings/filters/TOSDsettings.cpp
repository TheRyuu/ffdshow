/*
 * Copyright (c) 2005,2006 Milan Cutka
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
#include "TOSDsettings.h"
#include "TimgFilterOSD.h"
#include "TaudioFilterOSD.h"
#include "TffdshowPageDec.h"
#include "Cfont.h"
#include "COSD.h"

//========================================= TOSDsettings =========================================
const TfilterIDFF TOSDsettings::idffs = {
    /*name*/      _l("OSD"),
    /*id*/        IDFF_filterOSD,
    /*is*/        IDFF_isOSD,
    /*order*/     IDFF_orderOSD,
    /*show*/      IDFF_showOSD,
    /*full*/      0,
    /*half*/      0,
    /*dlgId*/     IDD_OSD,
};

// we need the size (IsizeofthisAll) parameter from inheriting class:
// otherwise, if we would use the size of the base class, additional parameters that belongs to the
// inheriting class will be stored beyond the memory size that allocated for the base class,
// and thus the inheriting class parameters won't be copied during base class operations.
TOSDsettings::TOSDsettings(size_t IsizeofthisAll, TintStrColl *Icoll, TfilterIDFFs *filters):
    TfilterSettingsVideo(IsizeofthisAll, Icoll, filters, &idffs)
{
    full = 0;
    half = 0;
    static const TintOptionT<TOSDsettings> iopts[] = {
        IDFF_isOSD                     , &TOSDsettings::is            , 0, 0, _l(""), 1,
        _l("isOSD"), 0,
        IDFF_showOSD                   , &TOSDsettings::show          , 0, 0, _l(""), 1,
        _l("showOSD"), 1,
        IDFF_orderOSD                  , &TOSDsettings::order         , 1, 1, _l(""), 1,
        _l("orderOSD"), 0,
        IDFF_OSDisAutoHide            , &TOSDsettings::isAutoHide    , 0, 0, _l(""), 1,
        _l("OSDisAutoHide"), 0,
        IDFF_OSDdurationVisible       , &TOSDsettings::durationVisible, 1, 10000, _l(""), 1,
        _l("OSDdurationVisible"), 100,
        IDFF_OSDisSave                 , &TOSDsettings::isSave        , 0, 0, _l(""), 1,
        _l("OSDisSave"), 0,
        IDFF_OSDsaveOnly               , &TOSDsettings::saveOnly      , 0, 0, _l(""), 1,
        _l("OSDsaveOnly"), 0,
        0
    };
    addOptions(iopts);
    static const TstrOption sopts[] = {
        IDFF_OSDformat, (TstrVal)&TOSDsettings::format          , 1024     , 0 , _l(""), 1,
        _l("OSDformat"), _l(""),
        IDFF_OSDsaveFlnm   , (TstrVal)&TOSDsettings::saveFlnm   , MAX_PATH , 0 , _l(""), 1,
        _l("OSDsaveFlnm"), _l(""),
        0
    };
    addOptions(sopts);
}

const char_t* TOSDsettings::getFormat(void) const
{
    return format;
}


//====================================== TOSDsettingsVideo =======================================
TOSDsettingsVideo::TOSDsettingsVideo(TintStrColl *Icoll, TfilterIDFFs *filters):
    TOSDsettings(sizeof(*this), Icoll, filters),
    font(Icoll)
{
    deepcopy = true;
    linespace = 100;

    static const TintOptionT<TOSDsettingsVideo> iopts[] = {
        IDFF_OSDposX                   , &TOSDsettingsVideo::posX          , 0, 100, _l(""), 1,
        _l("OSDposX"), 0,
        IDFF_OSDposY                   , &TOSDsettingsVideo::posY          , 0, 100, _l(""), 1,
        _l("OSDposY"), 0,
        IDFF_OSD_userformat            , &TOSDsettingsVideo::userFormat    , 0, 100, _l(""), 1,
        _l("OSDuserFormat"), 3,
        0
    };
    addOptions(iopts);
}

void TOSDsettingsVideo::copy(const TfilterSettings *src)
{
    memcpy(((uint8_t*)this) + sizeof(Toptions), ((uint8_t*)src) + sizeof(Toptions), sizeof(*this) - sizeof(font) - sizeof(Toptions));
    font = ((TOSDsettingsVideo*)src)->font;
}

void TOSDsettingsVideo::reg_op(TregOp &t)
{
    TfilterSettingsVideo::reg_op(t);
    font.reg_op(t);
}

void TOSDsettingsVideo::resetLook(void)
{
    posX = 0;
    posY = 0;
}

void TOSDsettingsVideo::createFilters(size_t filtersorder, Tfilters *filters, TfilterQueue &queue) const
{
    if (!queue.temporary) {
        setOnChange(IDFF_isOSD, filters, &Tfilters::onQueueChange);
    }
    queueFilter<TimgFilterOSD>(filtersorder, filters, queue);
}

void TOSDsettingsVideo::createPages(TffdshowPageDec *parent) const
{
    parent->addFilterPage<TOSDpageVideo>(&idffs);
    parent->addFilterPage<TfontPageOSD>(&idffs);
}

//====================================== TOSDsettingsAudio =======================================
TOSDsettingsAudio::TOSDsettingsAudio(TintStrColl *Icoll, TfilterIDFFs *filters):
    TOSDsettings(sizeof(*this), Icoll, filters)
{
}

void TOSDsettingsAudio::createFilters(size_t filtersorder, Tfilters *filters, TfilterQueue &queue) const
{
    if (!queue.temporary) {
        setOnChange(IDFF_isOSD, filters, &Tfilters::onQueueChange);
    }
    queueFilter<TaudioFilterOSD>(filtersorder, filters, queue);
}

void TOSDsettingsAudio::createPages(TffdshowPageDec *parent) const
{
    parent->addFilterPage<TOSDpageAudio>(&idffs);
}
