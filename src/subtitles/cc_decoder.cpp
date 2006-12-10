/*
 * Copyright (C) 2000-2003 the xine project
 * 
 * Copyright (C) Christian Vogler 
 *               cvogler@gradient.cis.upenn.edu - December 2001
 *
 * This file is part of xine, a free video player.
 * 
 * xine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * xine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Some small bits and pieces of the EIA-608 captioning decoder were
 * adapted from CCDecoder 0.9.1 by Mike Baker. The latest version is
 * available at http://sourceforge.net/projects/ccdecoder/.
 */

#include "stdafx.h"
#include "cc_decoder.h"
#include "ffdebug.h"
#include "IffdshowDecVideo.h"

char TccDecoder::chartbl[128];

int TccDecoder::good_parity(uint16_t data)
{
 struct Tparity
  {
  private:
   static int parity(uint8_t byte)
    {
     int ones = 0;
     for (int i = 0; i < 7; i++)
      if (byte & (1 << i))
       ones++;
     return ones & 1;
    }
  public:
   int table[256];
   Tparity(void)
    {
     for (uint8_t byte = 0; byte <= 127; byte++) 
      {
       int parity_v = parity(byte);
       /* CC uses odd parity (i.e., # of 1's in byte is odd.) */
       table[byte] = parity_v;
       table[byte | 0x80] = !parity_v;
      }
    }
  };
 static const Tparity parity;
 int ret = parity.table[data & 0xff] && parity.table[(data & 0xff00) >> 8];
 if (!ret)
  DPRINTFA("Bad parity in EIA-608 data (%x)\n", data);
 return ret;
}

TccDecoder::cc_buffer_t* TccDecoder::active_ccbuffer(void)
{
 cc_memory_t *mem=*this->active;
 return &mem->channel[mem->channel_no];
}

void TccDecoder::cc_row_t::ccrow_fill_transp(void)
{
 for (int i = this->num_chars; i < this->pos; i++) 
  {
   this->cells[i].c = TRANSP_SPACE;
   this->cells[i].midrow_attr = 0;
  }
}

int TccDecoder::cc_row_t::ccrow_find_next_text_part(int pos)
{
 while (pos < this->num_chars && this->cells[pos].c == TRANSP_SPACE)
   pos++;
 return pos;
}

int TccDecoder::cc_row_t::ccrow_find_end_of_text_part(int pos)
{
  while (pos < this->num_chars && this->cells[pos].c != TRANSP_SPACE)
    pos++;
  return pos;
}

int TccDecoder::cc_row_t::ccrow_find_current_attr(int pos)
{
  while (pos > 0 && !this->cells[pos].midrow_attr)
    pos--;
  return pos;
}

int TccDecoder::cc_row_t::ccrow_find_next_attr_change(int pos, int lastpos)
{
  pos++;
  while (pos < lastpos && !this->cells[pos].midrow_attr)
    pos++;
  return pos;
}

bool TccDecoder::cc_row_t::ccrow_render(cc_renderer_t *renderer, int rownum)
{
 bool was=false;
 int pos = ccrow_find_next_text_part(0);
 while (pos < this->num_chars) {
    int endpos = ccrow_find_end_of_text_part(pos);
    int seg_begin = pos;
    
    //int seg_pos[CC_COLUMNS + 1];seg_pos[0] = seg_begin;
    //int cumulative_seg_width[CC_COLUMNS + 1];cumulative_seg_width[0] = 0;
    char buf[CC_COLUMNS + 1];
    //int seg_attr[CC_COLUMNS];int num_seg = 0;
    
    while (seg_begin < endpos) {
      int attr_pos = ccrow_find_current_attr(seg_begin);
      int seg_end = ccrow_find_next_attr_change(seg_begin, endpos);

      /* compute text size of segment */
      for (int i = seg_begin; i < seg_end; i++)
	buf[i - seg_begin] = this->cells[i].c;
      buf[seg_end - seg_begin] = '\0';
      
      const cc_attribute_t *attr = &this->cells[attr_pos].attributes;
      char sub[CC_COLUMNS*2]="";
      if (attr->italic) strcat(sub,"<i>");
      if (attr->underline) strcat(sub,"<u>");
      strcat(sub,buf);
      if (attr->italic) strcat(sub,"</i>");
      if (attr->underline) strcat(sub,"</u>");
      renderer->deciV->addClosedCaption(sub);
      was=true;
      //ccrow_set_attributes(renderer, this, attr_pos);
      //osd_renderer->get_text_size(renderer->cap_display, buf, &seg_w, &seg_h);

      /* update cumulative segment statistics */
      //text_w += seg_w;
      //text_h += seg_h;
      //seg_pos[num_seg + 1] = seg_end;
      //seg_attr[num_seg] = attr_pos;
      //cumulative_seg_width[num_seg + 1] = text_w;
      //num_seg++;

      seg_begin = seg_end;
    }
    
    //renderer->deciV->shortOSDmessage(buf+pos,
    pos = ccrow_find_next_text_part(endpos);
 }
 return was;
}

void TccDecoder::cc_buffer_t::ccbuf_add_char(uint8_t c)
{
  cc_row_t *rowbuf = &this->rows[this->rowpos];
  int pos = rowbuf->pos;
  int left_displayable = (pos > 0) && (pos <= rowbuf->num_chars);

#if LOG_DEBUG > 2
  printf("cc_decoder: ccbuf_add_char: %c @ %d/%d\n", c, this->rowpos, pos);
#endif

  if (pos >= CC_COLUMNS) {
    //printf("cc_decoder: ccbuf_add_char: row buffer overflow\n");
    return;
  }

  if (pos > rowbuf->num_chars) {
    /* fill up to indented position with transparent spaces, if necessary */
    rowbuf->ccrow_fill_transp();
  }

  /* midrow PAC attributes are applied only if there is no displayable */
  /* character to the immediate left. This makes the implementation rather */
  /* complicated, but this is what the EIA-608 standard specifies. :-( */
  if (rowbuf->pac_attr_chg && !rowbuf->attr_chg && !left_displayable) {
    rowbuf->attr_chg = 1;
    rowbuf->cells[pos].attributes = rowbuf->pac_attr;
#ifdef LOG_DEBUG
    printf("cc_decoder: ccbuf_add_char: Applying midrow PAC.\n");
#endif
  }

  rowbuf->cells[pos].c = c;
  rowbuf->cells[pos].midrow_attr = rowbuf->attr_chg;
  rowbuf->pos++;

  if (rowbuf->num_chars < rowbuf->pos)
    rowbuf->num_chars = rowbuf->pos;

  rowbuf->attr_chg = 0;
  rowbuf->pac_attr_chg = 0;
}

void TccDecoder::cc_buffer_t::ccbuf_set_cursor(int row, int column, int underline, int italics, int color)
{
 cc_row_t *rowbuf = &this->rows[row];
 cc_attribute_t attr;

 attr.italic = (uint8_t)italics;
 attr.underline = (uint8_t)underline;
 attr.foreground = (uint8_t)color;
 attr.background = BLACK;

 rowbuf->pac_attr = attr;
 rowbuf->pac_attr_chg = 1;

 this->rowpos = row; 
 rowbuf->pos = column;
 rowbuf->attr_chg = 0;
}

void TccDecoder::cc_buffer_t::ccbuf_apply_attribute(cc_attribute_t *attr)
{
  cc_row_t *rowbuf = &this->rows[this->rowpos];
  int pos = rowbuf->pos;
  
  rowbuf->attr_chg = 1;
  rowbuf->cells[pos].attributes = *attr;
  /* A midrow attribute always counts as a space */
  ccbuf_add_char(chartbl[(unsigned int) ' ']);
}

void TccDecoder::cc_buffer_t::ccbuf_tab(int tabsize)
{
  cc_row_t *rowbuf = &this->rows[this->rowpos];
  rowbuf->pos += tabsize;
  if (rowbuf->pos > CC_COLUMNS) {
    rowbuf->pos = CC_COLUMNS;
    return;
  }
  /* tabs have no effect on pending PAC attribute changes */
}

int TccDecoder::cc_buffer_t::cc_buf_has_displayable(void)
{
  for (int i = 0; i < CC_ROWS; i++) {
    if (this->rows[i].num_chars > 0)
      return 1;
  }
  return 0;
}

void TccDecoder::cc_buffer_t::ccbuf_render(cc_renderer_t *renderer)
{
 bool wasrow=false;
  for (int row = 0; row < CC_ROWS; ++row) {
    if (this->rows[row].num_chars > 0)
      wasrow|=this->rows[row].ccrow_render(renderer, row);
    else
      if (wasrow)
       renderer->deciV->addClosedCaption("");  
  }
}


void TccDecoder::cc_memory_t::ccmem_clear(void)
{
 memset(this, 0, sizeof (cc_memory_t));
}

void TccDecoder::cc_set_channel(int channel)
{
 (*this->active)->channel_no = channel;
}

void TccDecoder::cc_renderer_t::cc_renderer_show_caption(cc_buffer_t *buf, int64_t vpts)
{
#ifdef LOG_DEBUG
  printf("spucc: cc_renderer: show\n");
#endif

  if (this->displayed) {
    cc_renderer_hide_caption(vpts);
  //printf("spucc: cc_renderer: show: OOPS - caption was already displayed!\n");
  }

  //this->osd_renderer->clear(this->cap_display);
  buf->ccbuf_render(this);
  //this->osd_renderer->set_position(this->cap_display, this->x, this->y);
  //vpts = std::max(vpts, this->last_hide_vpts);
  //this->osd_renderer->show(this->cap_display, vpts);
  
  this->displayed = 1;
  //this->display_vpts = vpts;
}

void TccDecoder::cc_renderer_t::cc_renderer_hide_caption(int64_t vpts)
{
  if (this->displayed) {
    //this->osd_renderer->hide(this->cap_display, vpts);
    deciV->hideClosedCaptions();
    this->displayed = 0;
    //this->last_hide_vpts = vpts;
  }
}


void TccDecoder::cc_decode_PAC(int channel,  uint8_t c1, uint8_t c2)
{
  cc_buffer_t *buf;
  int row, column = 0;
  int underline, italics = 0, color;

  /* There is one invalid PAC code combination. Ignore it. */
  if (c1 == 0x10 && c2 > 0x5f)
    return;

  cc_set_channel(channel);
  buf = active_ccbuffer();

 static const int rowdata[] = {10, -1, 0, 1, 2, 3, 11, 12, 13, 14, 4, 5, 6, 7, 8, 9};
  row = rowdata[((c1 & 0x07) << 1) | ((c2 & 0x20) >> 5)];
  if (c2 & 0x10) {
    column = ((c2 & 0x0e) >> 1) * 4;   /* preamble indentation */
    color = WHITE;                     /* indented lines have white color */
  }
  else if ((c2 & 0x0e) == 0x0e) {
    italics = 1;                       /* italics, they are always white */
    color = WHITE;
  }
  else
    color = (c2 & 0x0e) >> 1;
  underline = c2 & 0x01;

#ifdef LOG_DEBUG
  printf("cc_decoder: cc_decode_PAC: row %d, col %d, ul %d, it %d, clr %d\n",
	 row, column, underline, italics, color);
#endif

  buf->ccbuf_set_cursor(row, column, underline, italics, color);
}

void TccDecoder::cc_decode_standard_char(uint8_t c1, uint8_t c2)
{
 cc_buffer_t *buf = active_ccbuffer();

 /* c1 always is a valid character */
 buf->ccbuf_add_char(chartbl[c1]);
 /* c2 might not be a printable character, even if c1 was */
 if (c2 & 0x60)
  buf->ccbuf_add_char(chartbl[c2]);
}

void TccDecoder::cc_decode_ext_attribute(int channel, uint8_t c1, uint8_t c2)
{
  cc_set_channel(channel);
}

void TccDecoder::cc_decode_special_char(int channel,  uint8_t c1, uint8_t c2)
{
 /* FIXME: do real TM */
 /* must be mapped as a music note in the captioning font */ 
  //static const char specialchar[] = {'«','-','¯','¬','T','ó','ú','Â','Ó', TRANSP_SPACE,'À','Ô','à','þ','¢','û'};
  static const char specialchar[] = {'\253','-','\257','\254','T','\363','\372','\302','\323', TRANSP_SPACE,'\300','\324','\340','\376','\242','\373'};

  cc_set_channel(channel);
  cc_buffer_t *buf = active_ccbuffer();
  buf->ccbuf_add_char(specialchar[c2 & 0xf]);
}

void TccDecoder::cc_decode_midrow_attr(int channel,  uint8_t c1, uint8_t c2)
{
  cc_buffer_t *buf;
  cc_attribute_t attr;

  cc_set_channel(channel);
  buf = active_ccbuffer();
  if (c2 < 0x2e) {
    attr.italic = 0;
    attr.foreground = (c2 & 0xe) >> 1;
  }
  else {
    attr.italic = 1;
    attr.foreground = WHITE;
  }
  attr.underline = c2 & 0x1;
  attr.background = BLACK;
#ifdef LOG_DEBUG
  printf("cc_decoder: cc_decode_midrow_attr: attribute %x\n", c2);
  printf("cc_decoder: cc_decode_midrow_attr: ul %d, it %d, clr %d\n",
	 attr.underline, attr.italic, attr.foreground);
#endif

  buf->ccbuf_apply_attribute(&attr);
}

void TccDecoder::cc_swap_buffers(void)
{
  /* hide caption in displayed memory */
  cc_hide_displayed();

  DPRINTFA("cc_decoder: cc_swap_buffers: swapping caption memory\n");
  std::swap( this->on_buf, this->off_buf);

  /* show new displayed memory */
  cc_show_displayed();
}


void TccDecoder::cc_decode_misc_control_code(int channel,uint8_t c1, uint8_t c2)
{
  cc_set_channel( channel);

  switch (c2) {          /* 0x20 <= c2 <= 0x2f */

  case 0x20:             /* RCL */
    break;

  case 0x21:             /* backspace */
    //DPRINTFA("cc_decoder: backspace\n");
    break;

  case 0x24:             /* DER */
    break;

  case 0x25:             /* RU2 */
    break;

  case 0x26:             /* RU3 */
    break;

  case 0x27:             /* RU4 */
    break;

  case 0x28:             /* FON */
    break;

  case 0x29:             /* RDC */
    break;

  case 0x2a:             /* TR */
    break;

  case 0x2b:             /* RTD */
    break;

  case 0x2c:             /* EDM - erase displayed memory */
    cc_hide_displayed();
    this->on_buf->ccmem_clear();
    break;

  case 0x2d:             /* carriage return */
    break;

  case 0x2e:             /* ENM - erase non-displayed memory */
    this->off_buf->ccmem_clear();
    break;

  case 0x2f:             /* EOC - swap displayed and non displayed memory */
    cc_swap_buffers();
    break;
  }
}

void TccDecoder::cc_decode_tab(int channel, uint8_t c1, uint8_t c2)
{
  cc_set_channel(channel);
  cc_buffer_t *buf = active_ccbuffer();
  buf->ccbuf_tab(c2 & 0x3);
}

void TccDecoder::cc_decode_EIA608(uint16_t data)
{
  uint8_t c1 = uint8_t(data & 0x7f);
  uint8_t c2 = uint8_t((data >> 8) & 0x7f);

  if (c1 & 0x60) {             /* normal character, 0x20 <= c1 <= 0x7f */
    cc_decode_standard_char(c1, c2);
  }
  else if (c1 & 0x10) {        /* control code or special character */
                               /* 0x10 <= c1 <= 0x1f */
    int channel = (c1 & 0x08) >> 3;
    c1 &= ~0x08;

    /* control sequences are often repeated. In this case, we should */
    /* evaluate it only once. */
    if (data != this->lastcode) {

      if (c2 & 0x40) {         /* preamble address code: 0x40 <= c2 <= 0x7f */
        cc_decode_PAC(channel, c1, c2);
      }
      else {
        switch (c1) {
        
        case 0x10:             /* extended background attribute code */
	  cc_decode_ext_attribute(channel, c1, c2);
	  break;

        case 0x11:             /* attribute or special character */
	  if ((c2 & 0x30) == 0x30) { /* special char: 0x30 <= c2 <= 0x3f  */
	    cc_decode_special_char(channel, c1, c2);
	  }
	  else if (c2 & 0x20) {     /* midrow attribute: 0x20 <= c2 <= 0x2f */
	    cc_decode_midrow_attr(channel, c1, c2);
	  }
	  break;

        case 0x14:             /* possibly miscellaneous control code */
	  cc_decode_misc_control_code(channel, c1, c2);
	  break;

        case 0x17:            /* possibly misc. control code TAB offset */
	                      /* 0x21 <= c2 <= 0x23 */
	  if (c2 >= 0x21 && c2 <= 0x23) {
	    cc_decode_tab(channel, c1, c2);
	  }
	  break;
        }
      }
    }
  }
 this->lastcode = data;
}

void TccDecoder::decode(const uint8_t *buffer,size_t buf_len)
{
  /* The first number may denote a channel number. I don't have the
   * EIA-708 standard, so it is hard to say.
   * From what I could figure out so far, the general format seems to be:
   *
   * repeat
   *
   *   0xfe starts 2 byte sequence of unknown purpose. It might denote
   *        field #2 in line 21 of the VBI. We'll ignore it for the
   *        time being.
   *
   *   0xff starts 2 byte EIA-608 sequence, field #1 in line 21 of the VBI.
   *        Followed by a 3-code triplet that starts either with 0xff or
   *        0xfe. In either case, the following triplet needs to be ignored
   *        for line 21, field 1.
   *
   *   0x00 is padding, followed by 2 more 0x00.
   *
   *   0x01 always seems to appear at the beginning, always seems to
   *        be followed by 0xf8, 8-bit number. 
   *        The lower 7 bits of this 8-bit number seem to denote the
   *        number of code triplets that follow.
   *        The most significant bit denotes whether the Line 21 field 1 
   *        captioning information is at odd or even triplet offsets from this
   *        beginning triplet. 1 denotes odd offsets, 0 denotes even offsets.
   *      
   *        Most captions are encoded with odd offsets, so this is what we
   *        will assume.
   *
   * until end of packet
   */
  const uint8_t *current = buffer;
  uint32_t curbytes = 0;
  uint8_t data1, data2;
  uint8_t cc_code;
  int odd_offset = 1;

  this->f_offset = 0;
  this->pts = pts;

  while (curbytes < buf_len) {
    int skip = 2;

    cc_code = *current++;
    curbytes++;
    
    if (buf_len - curbytes < 2) {
      DPRINTFA("Not enough data for 2-byte CC encoding\n");
      break;
    }
    
    data1 = *current;
    data2 = *(current + 1);
    
    switch (cc_code) {
    case 0xfe:
      /* expect 2 byte encoding (perhaps CC3, CC4?) */
      /* ignore for time being */
      skip = 2;
      break;
      
    case 0xff:
      /* expect EIA-608 CC1/CC2 encoding */
      if (good_parity(data1 | (data2 << 8))) {
	cc_decode_EIA608(data1 | (data2 << 8));
	this->f_offset++;
      }
      else 
       return;
      skip = 5;
      break;
      
    case 0x00:
      /* This seems to be just padding */
      skip = 2;
      break;
      
    case 0x01:
      odd_offset = data2 & 0x80;
      if (odd_offset)
	skip = 2;
      else
	skip = 5;
      break;
      
    default:
      DPRINTFA("Unknown CC encoding: %x\n", cc_code);
      skip = 2;
      break;
    }
    current += skip;
    curbytes += skip;
  }
}

int TccDecoder::cc_onscreen_displayable(void)
{
 return this->on_buf->channel[this->on_buf->channel_no].cc_buf_has_displayable();
}

void TccDecoder::cc_show_displayed(void)
{
 DPRINTFA("cc_decoder: cc_show_displayed\n");

 if (cc_onscreen_displayable()) {
    //int64_t vpts = cc_renderer_calc_vpts(this->cc_state->renderer, this->pts, this->f_offset);
    //DPRINTFA("cc_decoder: cc_show_displayed: showing caption %u at vpts %u\n", this->capid, vpts);
    int64_t vpts=0;
    this->capid++;
    this->cc_state.renderer->cc_renderer_show_caption(&this->on_buf->channel[this->on_buf->channel_no], vpts);
  }
}
void TccDecoder::cc_hide_displayed(void)
{
 cc_renderer.deciV->hideClosedCaptions();
}

TccDecoder::TccDecoder(IffdshowDecVideo *deciV):
 capid(0),
 lastcode(0),
 pts(0),f_offset(0),
 cc_renderer(deciV),
 cc_state(&cc_renderer)
{
 struct TcharTabInit
  {
   TcharTabInit(void)
    {
     // first the normal ASCII codes 
     for (int i = 0; i < 128; i++)
       chartbl[i] = (char) i;
     /// now the special codes 
     chartbl[0x2a] = '\337';  // 'ß'
     chartbl[0x5c] = '\332';  // 'Ú'
     chartbl[0x5e] = '\335';  // 'Ý'
     chartbl[0x5f] = '\241';  // '¡'
     chartbl[0x60] = '\377';  // 'ÿ'
     chartbl[0x7b] = '\232';  // 'š'
     chartbl[0x7c] = '\270';  // '¸'
     chartbl[0x7d] = '\320';  // 'Ð'
     chartbl[0x7e] = '\275';  // '½'
     chartbl[0x7f] = '\245';  // '¥'  // FIXME: this should be a solid block
    }
  };
 static const TcharTabInit charTabInit;   
 
 this->on_buf = &this->buffer[0];
 this->off_buf = &this->buffer[1];
 this->active = &this->off_buf; 
}
