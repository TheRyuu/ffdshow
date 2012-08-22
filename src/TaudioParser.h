#ifndef _TAUDIOPARSER_H_
#define _TAUDIOPARSER_H_

#include "TaudioCodec.h"
#include "TffdshowDecAudio.h"
#include "TglobalSettings.h"
#include "Tbitdata.h"
#include "ffcodecs.h"

enum StreamFormat {
    UNDEFINED = 0,
    REGULAR_AC3 = 1,
    EAC3 = 2,
    MLP = 3,
    TRUEHD = 4,
    AC3_TRUEHD = 5,
    DTS = 6,
    DTS_HD = 7
};

/**
 * Possible frame sizes.
 * from ATSC A/52 Table 5.18 Frame Size Code Table.
 */
const uint16_t ff_ac3_frame_size_tab[38][3] = {
    { 64,   69,   96   },
    { 64,   70,   96   },
    { 80,   87,   120  },
    { 80,   88,   120  },
    { 96,   104,  144  },
    { 96,   105,  144  },
    { 112,  121,  168  },
    { 112,  122,  168  },
    { 128,  139,  192  },
    { 128,  140,  192  },
    { 160,  174,  240  },
    { 160,  175,  240  },
    { 192,  208,  288  },
    { 192,  209,  288  },
    { 224,  243,  336  },
    { 224,  244,  336  },
    { 256,  278,  384  },
    { 256,  279,  384  },
    { 320,  348,  480  },
    { 320,  349,  480  },
    { 384,  417,  576  },
    { 384,  418,  576  },
    { 448,  487,  672  },
    { 448,  488,  672  },
    { 512,  557,  768  },
    { 512,  558,  768  },
    { 640,  696,  960  },
    { 640,  697,  960  },
    { 768,  835,  1152 },
    { 768,  836,  1152 },
    { 896,  975,  1344 },
    { 896,  976,  1344 },
    { 1024, 1114, 1536 },
    { 1024, 1115, 1536 },
    { 1152, 1253, 1728 },
    { 1152, 1254, 1728 },
    { 1280, 1393, 1920 },
    { 1280, 1394, 1920 },
};

static const uint32_t dca_sample_rates[16] = {
    0, 8000, 16000, 32000, 0, 0, 11025, 22050, 44100, 0, 0,
    12000, 24000, 48000, 96000, 192000
};

static const uint32_t dca_bit_rates[32] = {
    32000, 56000, 64000, 96000, 112000, 128000,
    192000, 224000, 256000, 320000, 384000,
    448000, 512000, 576000, 640000, 768000,
    896000, 1024000, 1152000, 1280000, 1344000,
    1408000, 1411200, 1472000, 1536000, 1920000,
    2048000, 3072000, 3840000, 1/*open*/, 2/*variable*/, 3/*lossless*/
};

const uint8_t ff_ac3_channels_tab[8] = {
    2, 1, 2, 3, 3, 4, 4, 5
};
const uint16_t ff_ac3_sample_rate_tab[3] = { 48000, 44100, 32000 };
const uint16_t ff_ac3_bitrate_tab[19] = {
    32, 40, 48, 56, 64, 80, 96, 112, 128,
    160, 192, 224, 256, 320, 384, 448, 512, 576, 640
};
static const uint8_t eac3_blocks[4] = {
    1, 2, 3, 6
};

static const uint8_t mlp_channels[32] = {
    1, 2, 3, 4, 3, 4, 5, 3, 4, 5, 4, 5, 6, 4, 5, 4,
    5, 6, 5, 5, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static const uint8_t thd_chancount[13] = {
    //  LR    C   LFE  LRs LRvh  LRc LRrs  Cs   Ts  LRsd  LRw  Cvh  LFE2
    2,   1,   1,   2,   2,   2,   2,   1,   1,   2,   2,   1,   1
};

static int truehd_channels(int chanmap);

struct TframeData {
public:
    uint32_t frame_size;
    uint32_t space_size;

    TframeData(uint32_t Iframe_size) {
        frame_size = Iframe_size;
        space_size = 0;
    }
};

struct TaudioParserData {
public:
    int channels;
    uint32_t sample_rate;
    uint32_t bit_rate;
    uint32_t sample_blocks;
    uint32_t ratebits;
    uint32_t lastFrameTime;
    bool isFirst;
    DWORD wFormatTag;
    int sample_format;
    std::vector<TframeData> frames;
    int nbFormatChanges;
    int alternateSampleFormat;
    void reset() {
        wFormatTag = 0;
        sample_rate = 0;
        bit_rate = 0;
        sample_format = 0;
        sample_blocks = 0;
        lastFrameTime = 0;
        isFirst = true;
        nbFormatChanges = 0;
        frames.clear();
        alternateSampleFormat = -1;
        channels = 0;
    }
    TaudioParserData() {
        reset();
    }
};

class TaudioParser
{
private:
    TbyteBuffer backupbuf;
    int skipBytes;
    int includeBytes;
    bool hasMLPFrames;
    bool useAC3CoreOnly;
    bool useAC3Passthrough, usableAC3Passthrough;
    bool useDTSPassthrough, usableDTSPassthrough;
    bool useTrueHDPassthrough, usableTrueHDPassthrough;
    bool useDTSHDPassthrough, usableDTSHDPassthrough;
    bool useEAC3Passthrough, usableEAC3Passthrough;
    bool firstFrame;
    StreamFormat streamformat;
    TglobalSettingsDecAudio *globalSettings;
    AVCodecID codecId;
    bool initConfigDone;
protected:
    comptrQ<IffdshowBase> deci;
    comptrQ<IffdshowDecAudio> deciA;
    IdecAudioSink *sinkA;
    TaudioParserData audioParserData;
    virtual HRESULT parseDTS(unsigned char *src, int size, TbyteBuffer *newsrcBuffer);
    virtual HRESULT parseAC3(unsigned char *src, int size, TbyteBuffer *newsrcBuffer);
    virtual void initConfig(void);
    //virtual void printbitssimple(uint32_t n); //DEBUG function
    uint32_t frame_size;
    bool searchSync;
public:
    TaudioParser(IffdshowBase *Ideci, IdecAudioSink *Isink);
    virtual ~TaudioParser();
    virtual AVCodecID parseStream(unsigned char *src, int size, TbyteBuffer *newsrcBuffer);
    virtual AVCodecID getCodecIdFromStream(void);
    virtual void NewSegment(void);
    virtual void init(void);
    virtual bool checkOutputFormat(AVCodecID codecId);
    virtual TaudioParserData getParserData(void);
    void SearchSync() {
        searchSync = true;
        audioParserData.isFirst = true;
    }
};

#endif
