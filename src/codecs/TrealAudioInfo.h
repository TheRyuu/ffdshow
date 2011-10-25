#ifndef _TREALAUDIOINFO_H_
#define _TREALAUDIOINFO_H_

// To get the real size of the structures (unaligned)
#pragma pack(push, 1)

struct TrealAudioInfo {
    DWORD fourcc1;             // '.', 'r', 'a', 0xfd
    WORD version1;             // 4 or 5
    WORD unknown1;             // 00 000
    DWORD fourcc2;             // .ra4 or .ra5
    DWORD unknown2;            // ???
    WORD version2;             // 4 or 5
    DWORD header_size;         // == 0x4e
    WORD flavor;               // codec flavor id
    DWORD coded_frame_size;    // coded frame size
    DWORD unknown3;            // big number
    DWORD unknown4;            // bigger number
    DWORD unknown5;            // yet another number
    WORD sub_packet_h;
    WORD frame_size;
    WORD sub_packet_size;
    WORD unknown6;             // 00 00
    void bswap();
};

struct TrealAudioInfo4 : TrealAudioInfo {
    WORD sample_rate;
    WORD unknown8;             // 0
    WORD sample_size;
    WORD channels;
    void bswap();
};

struct TrealAudioInfo5 : TrealAudioInfo {
    BYTE unknown7[6];          // 0, srate, 0
    WORD sample_rate;
    WORD unknown8;             // 0
    WORD sample_size;
    WORD channels;
    DWORD genr;                // "genr"
    DWORD fourcc3;             // fourcc
    void bswap();
};

#pragma pack(pop)

#endif
