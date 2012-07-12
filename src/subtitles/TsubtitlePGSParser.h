#ifndef _TSUBTITLEPGSPARSER_H_
#define _TSUBTITLEPGSPARSER_H_

#include "Tsubtitle.h"
#include "autoptr.h"
#include "Crect.h"
#include "TffRect.h"
#include "TimgFilter.h"
#include "Tbitdata.h"
#include "TsubtitleDVD.h"
#include "interfaces.h"

#define DEBUG_PGS 0

#pragma region PSG subs Structures & types

struct TsubtitlePGS;
static inline void rt2Str(REFERENCE_TIME rTime, char_t *string)
{
    if (!string) {
        return;
    }
    if (rTime <= 0)  {
        tsprintf(string, _l("%d:%d:%d.%d"), 0, 0, 0, 0);
        return;
    }
    float rtimems = (float)(rTime / 10000);
    float rtimes = rtimems / 1000;
    int h = (int)rtimes / 3600;
    int m = (int)(rtimes - h * 3600) / 60;
    int s = (int)(rtimes - h * 3600 - m * 60);
    int ms = (int)((float)(rtimems - (float)1000 * (h * 3600 + m * 60 + s)));
    tsprintf(string, _l("%d:%d:%d.%d"), h, m, s, ms);
}

#define MAX_WINDOWS 3 // Maximum number of windows
#define RLE_ARRAY_SIZE 2 // Maximum parts of RLE buffers in multipart subtitle

static const REFERENCE_TIME INVALID_TIME = _I64_MIN;
struct VIDEO_DESCRIPTOR {
    SHORT nVideoWidth;
    SHORT nVideoHeight;
    BYTE bFrameRate; // <= Frame rate here!
};

struct ThdmvPalette {
    DWORD m_Colors[256];
    ThdmvPalette() {
        reset();
    };
    void reset() {
        memsetd(m_Colors, 0xFF000000, sizeof(DWORD) * 256);
    }
    ~ThdmvPalette() {};
};

struct TwindowDefinition {
    int m_horizontal_position, m_vertical_position, m_width, m_height, m_objectId, m_windowId;
    TspuImage *ownimage;
    SHORT m_cropping_horizontal_position;
    SHORT m_cropping_vertical_position;
    SHORT m_cropping_width;
    SHORT m_cropping_height;
    bool m_object_cropped_flag;
    bool m_forced_on_flag;
    REFERENCE_TIME m_rtStart;
    REFERENCE_TIME m_rtStop;
    TbyteBuffer data[RLE_ARRAY_SIZE]; // RGB indexes
    int dataIndex;
    void reset() {
        m_horizontal_position = m_vertical_position = m_width = m_height = m_objectId = m_windowId = 0;
        ownimage = NULL;
        m_cropping_horizontal_position = m_cropping_vertical_position = m_cropping_width = m_cropping_height = 0;
        m_rtStart = m_rtStop = INVALID_TIME;
        m_object_cropped_flag = m_forced_on_flag = false;
        for (int i = 0; i < RLE_ARRAY_SIZE; i++) {
            data[i].clear();
        };
        dataIndex = 0;
    }
    TwindowDefinition() {
        reset();
    }
    bool isReady() {
        return (data[0].size() == 0 || (data[0].size() != 0 && m_rtStart != INVALID_TIME && m_rtStop != INVALID_TIME)) ? true : false;
    }
};

typedef std::vector<TwindowDefinition* > TwindowsDefinition;

class TcompositionObject
{
public :
    TcompositionObject() {
        reset();
    };
    void reset() {
        m_nWindows = 0;
        m_compositionNumber = -1;
        m_bReady = false;
        m_version_number = 0;
        m_palette_id_ref = 0;
        m_bGotPalette = false;
        memsetd(m_Colors, 0xFF000000, sizeof(DWORD) * 256);
        m_data_length = 0;
        m_pVideoDescriptor = NULL;
        m_bEmptySubtitles = false;
        for (int i = 0; i < MAX_WINDOWS; i++) {
            m_Windows[i].reset();
        }
        m_rtTime = INVALID_TIME;
        m_nState = 0;
        m_pSubtitlePGS = NULL;
    }
    bool isEmpty() {
        if (m_nWindows == 0) {
            return true;
        }
        for (int i = 0; i < MAX_WINDOWS; i++) {
            if (m_Windows[i].data[0].size() != 0) {
                return false;
            }
        }
        return true;
    }
    bool isReady() {
        for (int i = 0; i < MAX_WINDOWS; i++) {
            if (!m_Windows[i].isReady()) {
                return false;
            }
        }
        return true;
    }

    int m_compositionNumber;
    REFERENCE_TIME m_rtTime;
    BYTE m_version_number;
    DWORD m_data_length;
    BYTE m_palette_id_ref;

    bool m_bReady;
    DWORD m_Colors[256];
    bool m_bGotPalette;
    bool m_bEmptySubtitles;
    VIDEO_DESCRIPTOR *m_pVideoDescriptor;
    TwindowDefinition m_Windows[MAX_WINDOWS];
    int m_nWindows;
    int m_nState;
    TsubtitlePGS *m_pSubtitlePGS;
};

typedef std::vector<TcompositionObject* > TcompositionObjects;
#pragma endregion

class TsubtitlePGSParser
{
    #pragma region Internal structures
    enum HDMV_SEGMENT_TYPE {
        NO_SEGMENT       = 0xFFFF,
        PALETTE          = 0x14,
        OBJECT           = 0x15,
        PRESENTATION_SEG = 0x16,
        WINDOW_DEF       = 0x17,
        INTERACTIVE_SEG  = 0x18,
        DISPLAY          = 0x80,
        HDMV_SUB1        = 0x81,
        HDMV_SUB2        = 0x82
    };
    struct COMPOSITION_DESCRIPTOR {
        SHORT nNumber;
        BYTE bState;
    };
    struct SEQUENCE_DESCRIPTOR {
        BYTE bFirstIn  : 1;
        BYTE bLastIn   : 1;
        BYTE bReserved : 8;
    };
    struct HDMV_PALETTE {
        BYTE entry_id;
        BYTE Y;
        BYTE Cr;
        BYTE Cb;
        BYTE T; // HDMV rule : 0 transparent, 255 opaque (compatible DirectX)
    };
    #pragma endregion

public:
    TsubtitlePGSParser(IffdshowBase *deci);
    virtual ~TsubtitlePGSParser();
    HRESULT parse(REFERENCE_TIME Istart, REFERENCE_TIME Istop, const unsigned char *data, size_t datalen);
    void getObjects(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, TcompositionObjects *pObjects);
    virtual void reset(void);

private:
    void parsePalette(Tbitdata &bitData, int nSize);
    void parseObject(Tbitdata &bitData, int nSize);
    void parsePresentationSegment(Tbitdata &bitData, REFERENCE_TIME rtStart);
    void parseWindow(Tbitdata &bitData, int nSize);
    bool getPalette(TcompositionObject *pObject);

    IffdshowBase *deci;
    HDMV_SEGMENT_TYPE m_nCurSegment;
    int m_nTotalSegBuffer;
    int m_nSegBufferPos;
    int m_nSegSize;
    Tbitdata m_bitdata;
    TbyteBuffer m_data;
    TbyteBuffer m_segmentBuffer;
    typedef stdext::hash_map<int, ThdmvPalette*> ThdmvPalettes;
    //typedef std::vector<ThdmvPalette*> ThdmvPalettes;
    ThdmvPalettes m_palettes;
    TcompositionObjects  m_compositionObjects;
    TcompositionObject *m_pCurrentObject;
    TcompositionObject *m_pPreviousObject;
    bool m_bDisplayFlag;

    ThdmvPalette *m_pDefaultPalette;
    int m_nDefaultPaletteNbEntry;
    int m_nColorNumber;
    int m_nOSDCount;
    HDMV_PALETTE *m_pPalette;
    VIDEO_DESCRIPTOR m_VideoDescriptor;
    TcompositionObject m_CurrentPresentationDescriptor;
    DWORD m_Colors[256];
};

#endif
