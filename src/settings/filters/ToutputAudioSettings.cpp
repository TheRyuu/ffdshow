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
#include "ToutputAudioSettings.h"
#include "TsampleFormat.h"
#include "TaudioFilterOutput.h"
#include <InitGuid.h>
#include <IffMmdevice.h> // Vista header import (MMDeviceAPI.h)

const char_t* ToutputAudioSettings::connetTos[] = {
    _l("any filter"),
    _l("DirectSound"),
    _l("WaveOut"),
    NULL
};


const TfilterIDFF ToutputAudioSettings::idffs = {
    /*name*/      _l("Output"),
    /*id*/        IDFF_filterOutputAudio,
    /*is*/        0,
    /*order*/     0,
    /*show*/      0,
    /*full*/      0,
    /*half*/      0,
    /*dlgId*/     0,
};

ToutputAudioSettings::ToutputAudioSettings(TintStrColl *Icoll, TfilterIDFFs *filters): TfilterSettingsAudio(sizeof(*this), Icoll, filters, &idffs, false)
{
    static const TintOptionT<ToutputAudioSettings> iopts[] = {
        IDFF_aoutpassthroughAC3     , &ToutputAudioSettings::passthroughAC3        , 0, 1, _l(""), 1,
        _l("passthroughAC3"), 0,
        IDFF_aoutpassthroughDTS     , &ToutputAudioSettings::passthroughDTS        , 0, 1, _l(""), 1,
        _l("passthroughDTS"), 0,
        IDFF_outsfs                 , &ToutputAudioSettings::outsfs                , 1, 1, _l(""), 1,
        _l("outsfs"), TsampleFormat::SF_PCM16,
        IDFF_outAC3bitrate          , &ToutputAudioSettings::outAC3bitrate         , 32, 640, _l(""), 1,
        _l("outAC3bitrate"), 640,
        IDFF_aoutConnectTo          , &ToutputAudioSettings::connectTo             , 0, 2, _l(""), 1,
        _l("connectTo"), 0,
        IDFF_aoutConnectToOnlySpdif , &ToutputAudioSettings::connectToOnlySpdif    , 0, 0, _l(""), 1,
        _l("connectToOnlySpdif"), 1,
        IDFF_aoutAC3EncodeMode      , &ToutputAudioSettings::outAC3EncodeMode      , 0, 0, _l(""), 1,
        _l("outAC3EncodeMode"), 0,
        IDFF_aoutpassthroughTRUEHD  , &ToutputAudioSettings::passthroughTRUEHD     , 0, 1, _l(""), 1,
        _l("passthroughTRUEHD"), 0,
        IDFF_aoutpassthroughDTSHD   , &ToutputAudioSettings::passthroughDTSHD      , 0, 1, _l(""), 1,
        _l("passthroughDTSHD"), 0,
        IDFF_aoutpassthroughEAC3   , &ToutputAudioSettings::passthroughEAC3      , 0, 1, _l(""), 1,
        _l("passthroughEAC3"), 0,
        IDFF_aoutUseIEC61937         , &ToutputAudioSettings::useIEC61937        , 0, 1, _l(""), 1,
        _l("useIEC61937"), 0,
        IDFF_aoutpassthroughPCMConnection, &ToutputAudioSettings::passthroughPCMConnection, 0, 1, _l(""), 1,
        _l("passthroughPCMConnection"), 1,
        IDFF_aoutpassthroughDeviceId, &ToutputAudioSettings::passthroughDeviceId, 0, 3, _l(""), 1,
        _l("passthroughDeviceId"), 0,
        0
    };
    addOptions(iopts);
    static const TcreateParamList1 listAoutConnectTo(connetTos);
    setParamList(IDFF_aoutConnectTo, &listAoutConnectTo);
}

void ToutputAudioSettings::createFilters(size_t filtersorder, Tfilters *filters, TfilterQueue &queue) const
{
    queueFilter<TaudioFilterOutput>(filtersorder, filters, queue);
}

const int* ToutputAudioSettings::getResets(unsigned int pageId)
{
    static const int idResets[] = {IDFF_aoutpassthroughAC3, IDFF_aoutpassthroughDTS, IDFF_outsfs, IDFF_outAC3bitrate, IDFF_aoutConnectTo, IDFF_aoutConnectToOnlySpdif, 0};
    return idResets;
}
