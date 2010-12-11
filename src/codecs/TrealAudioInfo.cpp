#include "stdafx.h"
#include "TrealAudioInfo.h"

void TrealAudioInfo::bswap()
{
    ::bswap(version1);
    ::bswap(version2);
    ::bswap(header_size);
    ::bswap(flavor);
    ::bswap(coded_frame_size);
    ::bswap(sub_packet_h);
    ::bswap(frame_size);
    ::bswap(sub_packet_size);
}

void TrealAudioInfo4::bswap()
{
    __super::bswap();
    ::bswap(sample_rate);
    ::bswap(sample_size);
    ::bswap(channels);
}

void TrealAudioInfo5::bswap()
{
    __super::bswap();
    ::bswap(sample_rate);
    ::bswap(sample_size);
    ::bswap(channels);
}
