#ifndef _TSUBTITLEPGSPARSER_H_
#define _TSUBTITLEPGSPARSER_H_

#include "Tsubtitle.h"
#include "autoptr.h"
#include "Crect.h"
#include "TffRect.h"
#include "TimgFilter.h"
#include "Tbitdata.h"
#include "interfaces.h"

#pragma region PSG subs Structures & types
 static const REFERENCE_TIME INVALID_TIME = _I64_MIN;
 struct VIDEO_DESCRIPTOR
	{
		SHORT		nVideoWidth;
		SHORT		nVideoHeight;
		BYTE		bFrameRate;		// <= Frame rate here!
	};

 struct ThdmvPalette
 {
  DWORD		m_Colors[256];
  ThdmvPalette() {reset();};
  void reset() {memsetd (m_Colors, 0xFF000000, sizeof(DWORD)*256);}
  ~ThdmvPalette() {};
 };

 class TcompositionObject
 {
  public :
   TcompositionObject() { reset();};
   void reset()
   {
    m_bReady = false; m_rtStart		= INVALID_TIME;	m_rtStop		= INVALID_TIME;	m_object_id_ref = 0; m_window_id_ref = 0;
    m_object_cropped_flag = false; m_forced_on_flag = false; m_version_number = 0; m_horizontal_position = 0; m_vertical_position = 0;
    m_width = 0; m_height = 0; m_cropping_horizontal_position = 0; m_cropping_vertical_position = 0; m_cropping_width = 0; m_cropping_height= 0;
    m_palette_id_ref = 0; m_bGotPalette = false; memsetd (m_Colors, 0xFF000000, sizeof(DWORD)*256);
    m_data_length = 0; m_pVideoDescriptor = NULL;
   }

	  SHORT				m_object_id_ref;
	  BYTE				m_window_id_ref; // Not used
	  bool				m_object_cropped_flag;
	  bool				m_forced_on_flag;
	  BYTE				m_version_number;
   DWORD   m_data_length;

	  SHORT				m_horizontal_position;
	  SHORT				m_vertical_position;
	  SHORT				m_width;
	  SHORT				m_height;

	  SHORT				m_cropping_horizontal_position;
	  SHORT				m_cropping_vertical_position;
	  SHORT				m_cropping_width;
	  SHORT				m_cropping_height;

   BYTE m_palette_id_ref;

	  REFERENCE_TIME		m_rtStart;
	  REFERENCE_TIME		m_rtStop;
   TbyteBuffer data; // RGB indexes
   bool     m_bReady;
   DWORD		m_Colors[256];
   bool     m_bGotPalette;
   VIDEO_DESCRIPTOR *m_pVideoDescriptor;
 };

 typedef stdext::hash_map<int,TcompositionObject*> TcompositionObjects;
#pragma endregion

class TsubtitlePGSParser
{
 #pragma region Internal structures
	enum HDMV_SEGMENT_TYPE
	{
		NO_SEGMENT			= 0xFFFF,
		PALETTE				= 0x14,
		OBJECT				= 0x15,
		PRESENTATION_SEG	= 0x16,
		WINDOW_DEF			= 0x17,
		INTERACTIVE_SEG		= 0x18,
		DISPLAY		= 0x80,
		HDMV_SUB1			= 0x81,
		HDMV_SUB2			= 0x82
	};
	struct COMPOSITION_DESCRIPTOR
	{
		SHORT		nNumber;
		BYTE		bState;
	};
	struct SEQUENCE_DESCRIPTOR
	{
		BYTE		bFirstIn  : 1;
		BYTE		bLastIn	  : 1;
		BYTE		bReserved : 8;
	};
 struct HDMV_PALETTE
 {
  BYTE		entry_id;
  BYTE		Y;
  BYTE		Cr;
  BYTE		Cb;  
	 BYTE		T;		// HDMV rule : 0 transparent, 255 opaque (compatible DirectX)
 };
#pragma endregion

public:
 TsubtitlePGSParser(IffdshowBase *deci);
 virtual ~TsubtitlePGSParser();
 HRESULT parse(REFERENCE_TIME Istart, REFERENCE_TIME Istop, const unsigned char *data, size_t datalen);
 void getObjects(REFERENCE_TIME rt, TcompositionObjects *pObjects);
 virtual void reset(void);

private:
 void parsePalette(Tbitdata &bitData, USHORT nSize);
 void parseObject(Tbitdata &bitData, USHORT nSize);
 void parsePresentationSegment(Tbitdata &bitData, REFERENCE_TIME rtStart);
 bool getPalette(TcompositionObject *pObject);
 
 IffdshowBase *deci;
 HDMV_SEGMENT_TYPE				m_nCurSegment;
	int								m_nTotalSegBuffer;
	int								m_nSegBufferPos;
	int								m_nSegSize;
 Tbitdata   m_bitdata;
 TbyteBuffer m_data;
 TbyteBuffer m_segmentBuffer;
 typedef stdext::hash_map<int,ThdmvPalette*> ThdmvPalettes;
 //typedef std::vector<ThdmvPalette*> ThdmvPalettes;
 ThdmvPalettes m_palettes;
 TcompositionObjects  m_compositionObjects;
 TcompositionObject *m_pCurrentObject;

 ThdmvPalette *m_pDefaultPalette;
	int								m_nDefaultPaletteNbEntry;
	int								m_nColorNumber;
 HDMV_PALETTE       *m_pPalette;
 VIDEO_DESCRIPTOR	m_VideoDescriptor;
 TcompositionObject m_CurrentPresentationDescriptor;
 DWORD		m_Colors[256];
};

#endif
