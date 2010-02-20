/*
 * Copyright (c) 2004-2010 Damien Bain-Thouverez
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
#include "Tconfig.h"
#include "TglobalSettings.h"
#include "ffdshow_mediaguids.h"
#include "TcodecSettings.h"
#include "rational.h"
#include "line.h"
#include "simd.h"
#include "TsubtitlePGSParser.h"
#include "TffPict.h"
#include "Tconvert.h"

#define DEBUG_PGS_PARSER 0

#pragma region Color conversion
#define RGBA(r,g,b,a) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))
const double Rec601_Kr = 0.299;
const double Rec601_Kb = 0.114;
const double Rec601_Kg = 0.587;
DWORD YCrCbToRGB_Rec601(BYTE A, BYTE Y, BYTE Cr, BYTE Cb)
{

  double rp = Y + 2*(Cr-128)*(1.0-Rec601_Kr);
  double gp = Y - 2*(Cb-128)*(1.0-Rec601_Kb)*Rec601_Kb/Rec601_Kg - 2*(Cr-128)*(1.0-Rec601_Kr)*Rec601_Kr/Rec601_Kg;
  double bp = Y + 2*(Cb-128)*(1.0-Rec601_Kb);

  return D3DCOLOR_ARGB(A, (BYTE)fabs(rp), (BYTE)fabs(gp), (BYTE)fabs(bp));
}

const double Rec709_Kr = 0.2125;
const double Rec709_Kb = 0.0721;
const double Rec709_Kg = 0.7154;
DWORD YCrCbToRGB_Rec709(BYTE A, BYTE Y, BYTE Cr, BYTE Cb)
{

  double rp = Y + 2*(Cr-128)*(1.0-Rec709_Kr);
  double gp = Y - 2*(Cb-128)*(1.0-Rec709_Kb)*Rec709_Kb/Rec709_Kg - 2*(Cr-128)*(1.0-Rec709_Kr)*Rec709_Kr/Rec709_Kg;
  double bp = Y + 2*(Cb-128)*(1.0-Rec709_Kb);

  return D3DCOLOR_ARGB (A, (BYTE)fabs(rp), (BYTE)fabs(gp), (BYTE)fabs(bp));
}
#pragma endregion

TsubtitlePGSParser::TsubtitlePGSParser(IffdshowBase *Ideci):
 deci(Ideci)
{
 m_pPalette = NULL;
 reset();
}

TsubtitlePGSParser::~TsubtitlePGSParser()
{
 for (TcompositionObjects::iterator c=m_compositionObjects.begin();c!=m_compositionObjects.end();c++)
  delete (*c).second;
 for (ThdmvPalettes::iterator p=m_palettes.begin();p!=m_palettes.end();p++)
  delete (*p).second;

}

void TsubtitlePGSParser::reset(void)
{
#if DEBUG_PGS_PARSER
 DPRINTF(_l("TsubtitlePGSParser::reset"));
#endif
 for (TcompositionObjects::iterator c=m_compositionObjects.begin();c!=m_compositionObjects.end();)
 {
   delete (*c).second;
   c = m_compositionObjects.erase(c);
 }
 for (ThdmvPalettes::iterator p=m_palettes.begin();p!=m_palettes.end();)
 {
   delete (*p).second;
   p = m_palettes.erase(p);
 }

 m_nColorNumber				= 0;
	m_nCurSegment				= NO_SEGMENT;
	m_nSegSize					= 0;
	m_pCurrentObject			= NULL;
	m_pDefaultPalette			= NULL;
	m_nDefaultPaletteNbEntry	= 0;
	memset (&m_VideoDescriptor, 0, sizeof(VIDEO_DESCRIPTOR));
}

HRESULT TsubtitlePGSParser::parse(REFERENCE_TIME Istart, REFERENCE_TIME Istop, const unsigned char *data, size_t datalen)
{
	HRESULT				hr=S_OK;
	REFERENCE_TIME		rtStart = Istart, rtStop = Istop;
 
 // Append the input buffer into global buffer
 //m_data.reserve(m_data.size()+datalen);
 m_data.append(data, datalen);
 
 while (m_data.size()>0)
 {
  m_bitdata = Tbitdata(&m_data[0], m_data.size());
	 if (m_nCurSegment == NO_SEGMENT)
	 {
   // 3 bytes
		 HDMV_SEGMENT_TYPE	nSegType	= (HDMV_SEGMENT_TYPE)m_bitdata.readByte();
		 USHORT				nUnitSize	= m_bitdata.readShort();

   // Not enough data for this segment but this is a bug (segments are truncated somewhere or nUnitSize is not reliable)
  if (m_data.size() < (size_t)nUnitSize+3)
  {
   //nUnitSize = m_data.size()-3;
   return S_OK;
  }

		 switch (nSegType)
		 {
		 case PALETTE :
		 case OBJECT :
		 case PRESENTATION_SEG :
		 case DISPLAY :
			 m_nCurSegment = nSegType;
    m_nSegSize=nUnitSize;
			 break;
   case WINDOW_DEF:
           /*
            * Window Segment Structure (No new information provided):
            *     2 bytes: Unkown,
            *     2 bytes: X position of subtitle,
            *     2 bytes: Y position of subtitle,
            *     2 bytes: Width of subtitle,
            *     2 bytes: Height of subtitle.
            */
    if (nUnitSize+3 <= (USHORT)m_data.size())
     m_data.erase(m_data.begin(), m_data.begin()+nUnitSize+3);
    else
     m_data.clear();
    continue;
		 default :
    // Delete unknown segment
    if (nUnitSize+3 <= (USHORT)m_data.size())
     m_data.erase(m_data.begin(), m_data.begin()+nUnitSize+3);
    else
     m_data.clear();
			 continue;
		 }
	 }
  		
  switch (m_nCurSegment)
	 {
	  case PALETTE :
#if DEBUG_PGS_PARSER
		  DPRINTF(_l("TsubtitlePGSParser::parse PALETTE            rtStart=%I64i, rtStop=%I64i"), rtStart, rtStop);
#endif
		  parsePalette(m_bitdata, m_nSegSize);
		  break;
	  case OBJECT :
#if DEBUG_PGS_PARSER
		  DPRINTF(_l("TsubtitlePGSParser::parse OBJECT            rtStart=%I64i, rtStop=%I64i"), rtStart, rtStop);
#endif
		  parseObject(m_bitdata, m_nSegSize);
		  break;
	  case PRESENTATION_SEG :
#if DEBUG_PGS_PARSER
		  DPRINTF(_l("TsubtitlePGSParser::parse PRESENTATION_SEG   rtStart=%I64i, rtStop=%I64i (size=%d)"), rtStart, rtStop, m_nSegSize);
#endif
    parsePresentationSegment(m_bitdata, rtStart);
		  break;
	  case WINDOW_DEF :
#if DEBUG_PGS_PARSER
		  DPRINTF(_l("TsubtitlePGSParser::parse WINDOW_DEF         rtStart=%I64i, rtStop=%I64i"), rtStart, rtStop);
#endif
		  break;
	  case DISPLAY :
#if DEBUG_PGS_PARSER
		  DPRINTF(_l("TsubtitlePGSParser::parse DISPLAY     rtStart=%I64i, rtStop=%I64i (size=%d)"), rtStart, rtStop, m_nSegSize);
#endif
		  break;
	  default :
#if DEBUG_PGS_PARSER
		  DPRINTF(_l("TsubtitlePGSParser::parse UNKNOWN Seg %d     rtStart=%I64i"), m_nCurSegment, rtStart);
#endif
    break;
	 }

	 m_nCurSegment = NO_SEGMENT;
  if (m_nSegSize+3 <= (USHORT)m_data.size())
   m_data.erase(m_data.begin(), m_data.begin()+m_nSegSize+3);
  else
   m_data.clear();
	 return hr;
 }
	return hr;
}


void TsubtitlePGSParser::parsePresentationSegment(Tbitdata &bitData, REFERENCE_TIME rtStart)
{
	COMPOSITION_DESCRIPTOR	compositionDescriptor;
	BYTE					nObjectNumber;
	bool					palette_update_flag;
	BYTE					palette_id_ref;

 m_VideoDescriptor.nVideoWidth   = bitData.readShort();
	m_VideoDescriptor.nVideoHeight  = bitData.readShort();
	m_VideoDescriptor.bFrameRate	= bitData.readByte();
 compositionDescriptor.nNumber	= bitData.readShort();
	compositionDescriptor.bState	= bitData.readByte();

	palette_update_flag	= !!(bitData.readByte() & 0x80);
	palette_id_ref		= bitData.readByte();
	nObjectNumber		= bitData.readByte();
 
	BYTE	bTemp;
 SHORT object_id_ref = bitData.readShort();

 TcompositionObjects::iterator c = m_compositionObjects.find(object_id_ref);
 if (c!=m_compositionObjects.end()) {
  
  // Problem : another object with the same id has to be added but the existing one
  // is ready and has not been displayed yet (otherwise we wouldln't have it in the store)
  if (c->second->m_bReady)
  {
#if DEBUG_PGS_PARSER
   DPRINTF(_l("TsubtitlePGSParser::parsePresentationSegment Another object %d exists and should not be erased. Put it with %d id"), object_id_ref, (object_id_ref+1));
#endif
   m_pCurrentObject = new TcompositionObject();
   m_compositionObjects.insert(std::make_pair(object_id_ref+1,m_pCurrentObject));
  }
  else
  {
#if DEBUG_PGS_PARSER
   DPRINTF(_l("TsubtitlePGSParser::parsePresentationSegment Object %d exists"), object_id_ref);
#endif
   m_pCurrentObject = c->second;
  }
 }
 else
 {
  m_pCurrentObject = new TcompositionObject();
  m_compositionObjects.insert(std::make_pair(object_id_ref,m_pCurrentObject));
 }

 //TODO : Find a better way to detect a presentation(start time) vs presentation(end time) vs presentation(no time)
 if (m_pCurrentObject->m_rtStart==INVALID_TIME && m_nSegSize == 19)
  m_pCurrentObject->m_rtStart = rtStart;
 else if (m_pCurrentObject->m_rtStart!=INVALID_TIME
  && m_pCurrentObject->m_rtStop==INVALID_TIME && m_nSegSize == 11)
 {
  m_pCurrentObject->m_rtStop = rtStart;
  m_pCurrentObject->m_bReady = true;
  return;
 }
 else return;

 
 m_pCurrentObject->m_object_id_ref = object_id_ref;
 m_pCurrentObject->m_palette_id_ref = palette_id_ref;
	m_pCurrentObject->m_window_id_ref	= bitData.readByte();
 m_pCurrentObject->m_pVideoDescriptor = &m_VideoDescriptor;

	bTemp = bitData.readByte();
	m_pCurrentObject->m_object_cropped_flag	= !!(bTemp & 0x80);
	m_pCurrentObject->m_forced_on_flag		= !!(bTemp & 0x40);
	m_pCurrentObject->m_horizontal_position	= bitData.readShort();
	m_pCurrentObject->m_vertical_position		= bitData.readShort();

	if (m_pCurrentObject->m_object_cropped_flag)
	{
		m_pCurrentObject->m_cropping_horizontal_position	= bitData.readShort();
		m_pCurrentObject->m_cropping_vertical_position	= bitData.readShort();
		m_pCurrentObject->m_cropping_width				= bitData.readShort();
		m_pCurrentObject->m_cropping_height				= bitData.readShort();
	}
#if DEBUG_PGS_PARSER
 DPRINTF(_l("parsePresentationSegment (%dx%d) object n°%d cropped [%d] | video (%dx%d)"), m_pCurrentObject->m_horizontal_position,m_pCurrentObject->m_vertical_position,
  nObjectNumber, (m_pCurrentObject->m_object_cropped_flag) ? 1 : 0, m_VideoDescriptor.nVideoWidth, m_VideoDescriptor.nVideoHeight);
#endif
}



void TsubtitlePGSParser::parsePalette(Tbitdata &bitData, USHORT nSize)
{
 BYTE	palette_id				= bitData.readByte();
	BYTE	palette_version_number	= bitData.readByte(); //TODO

	ASSERT ((nSize-2) % sizeof(HDMV_PALETTE) == 0);
	int nNbEntry = (nSize-2) / sizeof(HDMV_PALETTE);
 
 // If palette already exists for this id overwrite it (it will be always the case)
 ThdmvPalettes::iterator p = m_palettes.find(palette_id);
 if (p!=m_palettes.end()) {
  m_pDefaultPalette = (*p).second;
 }
 else // Create the new palette and add it to the list (don't know if PGS can have several palettes in parallel)
 {
  m_pDefaultPalette	= new ThdmvPalette();
  m_palettes.insert(std::make_pair(palette_id,m_pDefaultPalette));
 }
 m_nDefaultPaletteNbEntry = palette_id;
 m_pDefaultPalette->reset();

 int y, cr, cb, a, entry_id;
 /*int r_add, g_add, b_add;
 int r, g, b;
 uint8_t ff_cropTbl[256 + 2 * 1024] = {0, };
 const uint8_t *cm      = ff_cropTbl + 1024;*/

 
 m_nColorNumber	= nNbEntry;
 bool bIsHD = m_VideoDescriptor.nVideoWidth>720;
 for (int i=0; i<m_nColorNumber; i++)
	{
  entry_id = bitData.readByte();
  y = bitData.readByte();
  cr = bitData.readByte();
  cb = bitData.readByte();
  a = bitData.readByte();
  //FFMpeg conversion method (doesn't work)
  /*YUV_TO_RGB1(cb, cr);
  YUV_TO_RGB2(r, g, b, y);
  m_pDefaultPalette->m_Colors[entry_id]=RGBA(r,g,b,a);*/
  if (bIsHD)
				m_pDefaultPalette->m_Colors[entry_id] = YCrCbToRGB_Rec709(a, y, cr, cb);
			else
				m_pDefaultPalette->m_Colors[entry_id] = YCrCbToRGB_Rec601(a, y, cr, cb);
	}
}


void TsubtitlePGSParser::parseObject(Tbitdata &bitData, USHORT nSize)
{
	SHORT	object_id	= bitData.readShort();
	BYTE	m_sequence_desc;

 if (m_pCurrentObject == NULL) return;
	
	m_pCurrentObject->m_version_number	= bitData.readByte();
	m_sequence_desc	= bitData.readByte();

	if (m_sequence_desc & 0x80)
	{
  DWORD	object_data_length  = (DWORD)bitData.getBits(24);
  m_pCurrentObject->m_data_length = object_data_length;
		
		m_pCurrentObject->m_width			= bitData.readShort();
		m_pCurrentObject->m_height 			= bitData.readShort();
  m_pCurrentObject->data.reserve(object_data_length-4);
  m_pCurrentObject->data.append (bitData.wordpointer, nSize-11);
#if DEBUG_PGS_PARSER
  DPRINTF(_l("TsubtitlePGSParser::parseObject Object %d x %d size (%d)"), m_pCurrentObject->m_width, m_pCurrentObject->m_height, nSize-11);
#endif
  bitData.skipBytes(nSize-11);
	}
	else
 {
  DPRINTF(_l("TsubtitlePGSParser::parseObject Object size (%d)"), nSize-4);
		m_pCurrentObject->data.append(bitData.wordpointer, nSize-4);
  bitData.skipBytes(nSize-4);
 }
 getPalette(m_pCurrentObject);
}

bool TsubtitlePGSParser::getPalette(TcompositionObject *pObject)
{
 if (pObject->m_bGotPalette) return true;
 ThdmvPalettes::iterator p = m_palettes.find(pObject->m_palette_id_ref);
 if (p!=m_palettes.end()) {
  memcpy(&(pObject->m_Colors), &((*p).second->m_Colors), 256*sizeof(DWORD));
  pObject->m_bGotPalette = true;
  return true;
 }
#if DEBUG_PGS_PARSER
 DPRINTF(_l("TsubtitlePGSParser::getPalette Palette with reference %d not found"),pObject->m_palette_id_ref);
#endif
 return false;
}

// Objects returned must be freed by the caller
void TsubtitlePGSParser::getObjects(REFERENCE_TIME rt, TcompositionObjects *pObjects)
{
 /*for (TcompositionObjects::iterator c=m_compositionObjects.begin();c!=m_compositionObjects.end();)
 {
  // Clear passed subs and only those that were entirely parsed (the m_rtStop should be set)
  if ((*c).second->m_rtStop < rt && (*c).second->m_rtStop != INVALID_TIME) 
  {
   if (m_pCurrentObject == (*c).second) m_pCurrentObject = NULL;
   delete (*c).second;
   c=m_compositionObjects.erase(c);
  } else  c++;
 }*/
 
 // Build the list of subs ready to be displayed
 for (TcompositionObjects::iterator c=m_compositionObjects.begin();c!=m_compositionObjects.end();)
 {
  if (/*(*c).second->m_rtStart <= rt && (*c).second->m_rtStop > rt && */(*c).second->m_bReady)
  {
   // Get the right palette pointer before adding the element
   if (!getPalette((*c).second))
   {
    // Palette not found, use default
    memcpy(&((*c).second->m_Colors), &(m_pDefaultPalette->m_Colors), 256*sizeof(DWORD));
    (*c).second->m_bGotPalette = true;
   }
   // Pop the object from our store and push it to the returned store
   pObjects->insert(std::make_pair((*c).first,(*c).second));
   if (m_pCurrentObject == (*c).second) m_pCurrentObject = NULL;
   c=m_compositionObjects.erase(c); 
  }
  else
   c++;
 }
}
