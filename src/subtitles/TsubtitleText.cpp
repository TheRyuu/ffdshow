/*
 * Copyright (c) 2003-2006 Milan Cutka
 *               2007-2009 h.yamagata
 * subtitles fixing code from SubRip by MJQ (subrip@divx.pl)
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
#include "TsubtitleText.h"
#include "TrenderedTextSubtitleWord.h"
#include "Tsubreader.h"
#include "TsubtitlesSettings.h"
#include "Tconfig.h"
#include "ThtmlColors.h"
#include "ffglobals.h"
#include "TwordWrap.h"
#include "IffdshowBase.h"
#include "IffdshowDecVideo.h"
#include "ffdebug.h"
#include "TsubreaderMplayer.h"
#include "TtoGdiFont.h"

#define _L1(x) L##x

//========================== TtextFixBase =========================
strings TtextFixBase::getDicts(const Tconfig *cfg)
{
 char_t msk[MAX_PATH];
 _makepath_s(msk, countof(msk), NULL, cfg->pth, _l("dict\\*"), _l("dic"));
 strings dicts;
 findFiles(msk,dicts,false);
 foreach (ffstring &dic, dicts)
  dic.erase(dic.find('.'),MAX_PATH);
 return dicts;
}

//============================= TtextFix ==========================
TtextFix::TtextFix(const TsubtitlesSettings *scfg,const Tconfig *ffcfg):
 EndOfPrevSentence(true),
 inHearing(false)
{
 memcpy(&cfg,scfg,sizeof(TsubtitlesSettings));
 if (scfg->fix&fixOrtography)
  {
   char_t dictflnm[MAX_PATH];
   _makepath_s(dictflnm, countof(dictflnm), NULL, ffcfg->pth, (::ffstring(_l("dict\\"))+scfg->fixDict).c_str(), _l("dic"));
   TstreamFile fs(dictflnm,false,false);
   if (fs)
    {
     wchar_t ln[1000];
     while (fs.fgets(ln,1000))
      {
       wchar_t *eoln=strchr(ln,'\n');if (eoln) *eoln='\0';
       odict.push_back(ln);
      }
     if (odd(odict.size()))
      odict.erase(odict.end()-1);
     if (!odict.empty())
      for (strings::iterator s=odict.begin();s+1!=odict.end();s++)
       if (strstr((s+1)->c_str(),s->c_str()))
        {
         s=odict.erase(s);
         s=odict.erase(s);
        }
    }
  }
}

bool TtextFix::process(ffstring &text,ffstring &fixed)
{
 if (cfg.fix==0) return false;
 int W1;
 passtring<wchar_t> S1=text;

 if (cfg.fix&fixAP) //AP
  {
   S1=stringreplace(S1,_L1("`")   , _L1("'") ,rfReplaceAll);
   S1=stringreplace(S1,_L1("\264"), _L1("'") ,rfReplaceAll);
   S1=stringreplace(S1,_L1("''")  , _L1("\""),rfReplaceAll);
   S1=stringreplace(S1,_L1("\"\""), _L1("\""),rfReplaceAll);
  }

 if (cfg.fix&fixIl) // Il
  {
   #define TrChar(n) St1[W1+(n)]
   bool TakeI=false,Takel=false;
   passtring<wchar_t> St1=_L1("  ")+S1+_L1("  ");
   for (W1=1 + 2;W1<=St1.size() - 2;W1++)
    {
     //small L --> big i
     if (St1[W1] == 'l')
      {
       TakeI = false;
       //_lll trio
       if ((TrChar(-1)==' ' || TrChar(-1)=='#') && (TrChar(+1) == 'l') && (TrChar(+2) == 'l'))
        {
         //_lllx --> _Illx (Illinois, Illegal etc.) + _Ill-
         if (in(TrChar(+3),_L1("-abehinopstuy"))) TakeI = true;
         //._Ill_ (=bedridden) + ._Ill. (Ill. = Illustriert, Illustration [German])
         //other than BoL cases not reflected
         if (in(TrChar(-2),_L1(".[")) && (in(TrChar(+3),_L1(" .")))) TakeI = true;
         //Godfather III / Godfather III.
         if (!(in(TrChar(-2),_L1(".[")) && (in(TrChar(+3),_L1(" .!?,")))))
          {
           TakeI = true;
           St1[W1 + 1] = 'I';
           St1[W1 + 2] = 'I';
          }
        }
       //_lgelit _lglu _lgnition _lgrafpapier _lguana
       if (in(TrChar(-1),_L1(" [(#")) && (TrChar(+1) == 'g') && in(TrChar(+2),_L1("elnru")))
        TakeI = true;
       //ME LOVES MAKARONl! (MY TO SLYSELl. (!?) [cz; no others affected])
       if (in(TrChar(+1),_L1(".!?")) &&
           (TrChar(-1) == toupper(TrChar(-1))) &&
           (TrChar(-1) != '.') && //»př.n.l.« at EoL case
           (TrChar(-2) == toupper(TrChar(-2))))
        TakeI = true;
       //UNlVERSAL, lnternet, lnspektion, lnternational;  not changed - lněný [cz; no others affected]
       //contain  : _l_ case [no Fr!], XlX, Xl_, _lX
       //           V.l. Lenin, l.V. Lenin (initials of names)
       //safely   : -lx OR -_lx; [lx OR [_lx; "Ix OR "_Ix
       //dangerous: .lxxx
       if (((TrChar(-1) == toupper(TrChar(-1))) &&
         (TrChar(-1) != '.') && //»př.n.l.« case [cz; no others affected]
         (TrChar(+1) == toupper(TrChar(+1))))
         ||
         ((in(TrChar(-1),_L1(" #"))) && (TrChar(+1) == 'n') && (TrChar(+2) != '\354')))
        TakeI = true;
       //last sign at uniline OR space follows
       if ((TrChar(+1) == ' ') &&
         (TrChar(-1) == toupper(TrChar(-1))))
        TakeI = true;
       //._l | -l | -_l; contain: ._ll_ (italian: Il brutto, il nero)
       if (((TrChar(-2) != '.') && (TrChar(-1) == '.')) ||
           ((TrChar(-2) == ' ') && (TrChar(-1) == '-')) ||
           ((in(TrChar(-2),_L1(".-"))) && (TrChar(-1) == ' ')) &&
           (TrChar(+1) != 'i')) TakeI = true;
       //_ll_ to _Il_ ...this is made only as prep for next
       if ((in(TrChar(-1),_L1(" #"))) && (TrChar(+1) == 'l') &&
         (in(TrChar(+2),_L1(" .,?!:")))) TakeI = true;
       //_Il_ to _II_ (-2 <> . or [) Godfather II & Ramses II
       if ((TrChar(-3) != '.') && (TrChar(-3) != '[') && (TrChar(-3) != '\0') &&
           (TrChar(-2) == ' ') && (TrChar(-1) == 'I') &&
           (in(TrChar(+1),_L1(" .,?!:")))) TakeI = true;
       //"hey C.W. load the weapons" and "hey... load the weapons"
       //      432101                        432101
       if ((TrChar(-4) == '.') && (TrChar(-2) == '.') & (TrChar(-1) == ' ') &&
           (in(TrChar(+1),_L1("aeiouy"))))
        TakeI = false;
       //_Ital...
       if ((in(TrChar(-1),_L1(" #"))) && (TrChar(+1) == 't') && (TrChar(+2) == 'a'))
        TakeI = true;
       //only for international abnormalities
       switch (cfg.fixLang)
        {
         case 0:// StCorrectIl_En;
          if (St1[W1] == 'l')
           {
            //so shity due the styles
            if ((in(TrChar(-1),_L1(" \"#"))) && (TrChar(+1) == 't' ) && (TrChar(+2) == '\'') && (TrChar(+3) == 's')) TakeI = true; //_lt's
            if ((in(TrChar(-1),_L1(" \"#"))) && (TrChar(+1) == '\'') && (TrChar(+2) == 'm' ) && (TrChar(+3) == ' ')) TakeI = true; //_l'm_
            if ((in(TrChar(-1),_L1(" \"#"))) && (TrChar(+1) == '\'') && (TrChar(+2) == 'd' ) && (TrChar(+3) == ' ')) TakeI = true; //_l'd_
            if ((in(TrChar(-1),_L1(" \"#"))) && (TrChar(+1) == '\'') && (TrChar(+2) == 'v' ) && (TrChar(+3) == 'e')) TakeI = true; //_l've
            if ((in(TrChar(-1),_L1(" \"#"))) && (TrChar(+1) == '\'') && (TrChar(+2) == 'l' ) && (TrChar(+3) == 'l')) TakeI = true; //_l'll
            if ((in(TrChar(-1),_L1(" \"#"))) && (in(TrChar(+1),_L1("fnst"))) && (TrChar(+2) == ' ')) TakeI = true; //_lf_, _ln_, _ls_, _lt_
            //no English word beginning with "lc", "ld", "lh", "ln", "lr", "ls"
            if ((in(TrChar(-1),_L1(" \"#"))) && (in(TrChar(+1),_L1("cdhnrs")))) TakeI = true;
           }
          break;
         case 1:// StCorrectIl_Fr;
          //*unknown*: I think that for french neither "I" alone nor "l" alone can exist so we don't check this. So the only thing we can write is that before an apostrophe there must be an "l".
          if (((in(TrChar(-1),_L1(" \"#"))) && (TrChar(+1) == ' ')) || //but Brain as Frenchman: lonesome I doesn't exist
              (TrChar(+1) == '\''))//*unknown*: An I followed by an apostrophe cannot exist. Must be an l.
           {
            TakeI = false;
            Takel = true;
           }
          break;

         case 2:// StCorrectIl_Ge;
          if (St1[W1] == 'l')
           //no German word beginning with "lc", "ld", "lh", "ln", "lr", "ls"
           if ((in(TrChar(-1),_L1(" \"#"))) && (in(TrChar(+1),_L1("cdhnrs")))) TakeI = true;
          break;
         case 3:// StCorrectIl_Cz;
          if (St1[W1] == 'l')
           //no Czech word beginning with "lc", "ld", "lr"
           if ((in(TrChar(-1),_L1(" \"#"))) && (in(TrChar(+1),_L1("cdr")))) TakeI = true;
          break;
         case 4:// StCorrectIl_It;
          //Hint: "i" alone CAN exist (even capital: after a full stop) while l alone can't, never
          //l' processing
          if ((TrChar(+1) == '\'')) //Ita always has l before apostrophe (if ' is not used as an accent for a capital letter! [rare, and followed by whitespace])
           if ((in(TrChar(+2),_L1("aAeEiIloOuUhH\xe0\xe8\xe9\xef\xf2\xf9"))))
            {
             Takel = true;
             TakeI = false;
            }
           //else: unknown pattern!? Probably an English sentence within an Italian movie. We rely on the general algorithm.
          break;
         case 5:// StCorrectIl_Pl;
          if (St1[W1] == 'l')
           //no Czech word beginning with "lc", "ld", "lr"
           if ((in(TrChar(-1),_L1("., \"#"))) && (in(TrChar(+1),_L1("bcdfhjkrstz\346\263\361\237"))))
            {
             Takel = false;
             TakeI = true;
            }
          break;
        }
       if (TakeI)
        {
         St1[W1] = 'I';
         //'-' for I-I-I...
         if ((W1 > 4) && (St1[W1 - 2] == 'l') && (St1[W1 - 1] == '-'))
          if (St1[W1 - 3] == ' ') St1[W1 - 2] = 'I';
          else
           if ((St1[W1 - 4] == 'l') && (St1[W1 - 3] == '-'))
            {
             St1[W1 - 2] = 'I';
             St1[W1 - 4] = 'I';
            }
        }
      }
     //big i --> small L
     if (St1[W1] == 'I')
      {
       Takel = false;
       //_III to _Ill
       if ((in(TrChar(-1),_L1(" #"))) && (TrChar(+1) == 'I') && (TrChar(+2) == 'I'))
        {
         //_IIIx --> _Illx (Illinois, Illegal etc.) + _Ill-
         if ((in(TrChar(+3),_L1("-abehinopstuy")))
          ||
           //._Ill_ (=bedridden) + ._Ill. (Ill. = Illustriert, Illustration [German])
           //other than BoL cases not reflected
           ((in(TrChar(-2),_L1(".["))) && (in(TrChar(+3),_L1(" .")))))
          {
           St1[W1 + 1] = 'l';
           St1[W1 + 2] = 'l';
          }
           //Godfather III / Godfather III. *PASS*
        }
       //All, English, German
       if (TrChar(-1)>='a' && TrChar(-1)<='z' && in(TrChar(-1),_L1("\344\366\374"))) Takel = true;
       else
        if (!((in(TrChar(+1),
          //All, English, German; 'w' - Iwao (Japan name), 'v' - Ivan
          _L1("ABCDEFGHIJKLMNOPQRSTUVWXYZ\304\326\334 bcdghklmnoprstvwz\'.,!?"))) || //',' prevents PRCl, PRCl, PRCICKY
          //+East Europe - Czech, Slovak, Polish
          ((cfg.font.charset== EASTEUROPE_CHARSET) &&
          (in(TrChar(+1),_L1("\301\311\314\315\323\332\331\335\310\317\322\330\212\215\216\324\305\274\243\217\257\323\245\312\214\306\321")))) ||
          //+Scandinavian
          ((cfg.font.charset== ANSI_CHARSET) &&
          in(TrChar(+1),_L1("\xc6\xd8\xc5"))
          ) ||
          //+Italian: _Ieri_
          ((in(TrChar(-1),_L1(" #"))) && (TrChar(+1) == 'e') &&
          (TrChar(+2) == 'r') && (cfg.fixLang == 4)
          )))
         Takel = true;
       //+East Europe - Czech, Slovak, Polish
       if ((cfg.font.charset== EASTEUROPE_CHARSET) &&
           (in(TrChar(-1),_L1("\341\350\357\351\354\355\362\363\370\232\235\372\371\375\236\364\345\276\237\277\363\263\271\352\234\346\361"))))
        Takel = true;
       //+Scandinavian
       if ((cfg.font.charset== ANSI_CHARSET) &&
           (in(TrChar(-1),_L1("\xe6\xf8\xe5"))))
        Takel = true;
       //_AIways AIden BIb BIouznit CIaire ČIověk PIný SIožit UItra ZIost
       if ((in(TrChar(-2),_L1(" \"-c#"))) && //'c' for Mac/Mc (McCIoy)
           (in(TrChar(-1),_L1("ABCDEFGHIJKLMNOPQRSTUVWXYZ\310\212\216\217\214"))) &&
           (in(TrChar(+1),_L1("abcdefghijklmnopqrstuvwxyz\341\350\351\354\355\363\362\232\371\375"))))
        Takel = true;
       //Ihned leave, but Ihostejný --> lhostejný [cz; no others affected]
       if ((TrChar(+1) == 'h') && (TrChar(+2) != 'n')) Takel = true;
       //_AI_ to _Al_ - name (Artificial Intelligence (AI) not awaited :-)
       if ((in(TrChar(-2),_L1(" \"#'"))) &&
           (TrChar(-1) == 'A') &&
           (in(TrChar(+1),_L1(" \".,!?'"))))
        Takel = true;
       //._II_ to ._Il_ (Il nero on BoL italian)
       if ((in(TrChar(-3),_L1(".[-"))) && (TrChar(-2) == ' ') &&
          (TrChar(-1) == 'I') && (TrChar(+1) == ' '))
        Takel = true;
       //(repd. from l-->I procedure too)  "EI gringo" to "El g." (BoL Spain)
       if ((TrChar(-1) == 'E') && (TrChar(+1) == ' ')) Takel = true;
       //'II_ --> 'll_
       if ((TrChar(-1) == '\'') && (TrChar(+1) == 'I') && (TrChar(+2) == ' '))
        {
         Takel = true;
         St1[W1 + 1] = 'l';
        }
       //_Iodic, Iolanthe/Iolite, Ion, Iowa, *Ioya* (but loyalty and more), Iota
       //vs Ioad Iocation Iook Iost Ioud Iove
       //Ioch Iogický/Iogo Ioket Iomcovák Iopata Iovit
       //. Io... (Italian)
       if (!(in(TrChar(-2),_L1(".!?"))) &&
            (in(TrChar(-1),_L1(" \"#"))) && (TrChar(+1) == 'o') &&
            !(in(TrChar(+2),_L1("dlnwt .,!?"))))
        Takel = true;
       //Iodní etc. vs Iodate Ioderma Iodic Iodoethanol [cz only, won't affect others]
       if ((in(TrChar(-1),_L1(" \"#"))) && (TrChar(+1) == 'o') && (TrChar(+2) == 'd') &&
            !(in(TrChar(+3),_L1("aeio")) || TrChar(+3)=='\0'))
        Takel = true;
       //Iondýnský [cz only, won't affect others]
       if ((in(TrChar(-1),_L1(" \"#"))) && (TrChar(+1) == 'o') && (TrChar(+2) == 'n') &&
            (TrChar(+3) == 'd'))
        Takel = true;
       //_Iong
       if ((in(TrChar(-1),_L1(" \"#"))) && (TrChar(+1) == 'o') &&
            (TrChar(+2) == 'n') && (TrChar(+3) == 'g'))
        Takel = true;
       //_Ioni_ --> _loni_  [cz only, don't affect others]
       if ((in(TrChar(-1),_L1(" \"#"))) && (TrChar(+1) == 'o') && (TrChar(+2) == 'n') &&
            (TrChar(+3) == 'i') && (in(TrChar(+4),_L1(" \".,!?:"))))
        Takel = true;
       //Iot_ etc. vs Iota
       if ((in(TrChar(-1),_L1(" \"#"))) && (TrChar(+1) == 'o') &&
            (TrChar(+2) == 't') && (TrChar(+3) != 'a'))
        Takel = true;
       //Iow_ etc. vs Iowa
       if ((in(TrChar(-1),_L1(" \"#"))) && (TrChar(+1) == 'o') &&
            (TrChar(+2) == 'w') && (TrChar(+3) != 'a'))
        Takel = true;
       //_AII_ etc.
       if ((in(TrChar(-2),_L1(" \"-#"))) &&
            (in(TrChar(-1),_L1("AEU"))) && (TrChar(+1) == 'I') &&
            (in(TrChar(+2),_L1(" '-abcdefghijklmnopqrstuvwxyz"))))
        {
         Takel = true;
         St1[W1 + 1] = 'l';
        }
       //Eng only, no others affected: _If_, _If-, Iffy, Ifle, Ifor
       if ((in(TrChar(-1),_L1(" \"#"))) && (TrChar(+1) == 'f') &&
          (in(TrChar(+2),_L1(" -flo"))))
        Takel = false;
       //"hey C.W. Ioad the weapons" and "hey... Ioad the weapons"
       //      432101                        432101
       if ((TrChar(-4) == '.') && (TrChar(-2) == '.') && (TrChar(-1) == ' ') &&
            (in(TrChar(+1),_L1("aeiouy"))))
        Takel = true;
       /* TODO
       //only for international abnormalities
       case RadioGroupCorrectIlLanguage.ItemIndex of
         //0: StCorrectIl_En;
         1: StCorrectIl_Fr;
         //2: StCorrectIl_Ge;
         //3: StCorrectIl_Cz;
         4: StCorrectIl_It;
       end;
       */
       if (Takel)
        {
         St1[W1] = 'l';
         //'-' for 'Uh, l-ladies and gentleman...' / 'just as you l-left it'
         if ((W1 > 4) && (St1[W1 - 2] == 'I') && (St1[W1 - 1] == '-'))
          if (St1[W1 - 3] == ' ') St1[W1 - 2] = 'l';
          else
           if ((St1[W1 - 4] == 'I') && (St1[W1 - 3] == '-'))
            {
             St1[W1 - 2] = 'l';
             St1[W1 - 4] = 'l';
            }
        }
      }
    }
   #undef TrChar
   S1=St1.copy(1 + 2, St1.size() - 4);
  }

 if (cfg.fix&fixPunctuation) //punctuation
  {
   static const wchar_t *punctinations[]=
    {
     _L1("?."),_L1("?"),
     _L1("?,"),_L1("?"),
     _L1(" )"),_L1(")"),
     _L1("( "),_L1("("),
     _L1(" ]"),_L1("]"),
     _L1("[ "),_L1("["),
     _L1("!."),_L1("!"),
     _L1("!,"),_L1("!"),
     _L1(". . ."),_L1("..."),
     _L1(". .."),_L1("..."),
     _L1(".. ."),_L1("..."),
     _L1("--"),_L1("..."),
     _L1("...."),_L1("..."),
     _L1(" :"),_L1(":"),
     _L1(" ;"),_L1(";"),
     _L1(" %"),_L1("%"),
     _L1(" !"),_L1("!"),
     _L1(" ?"),_L1("?"),
     _L1(" ."),_L1("."),
     _L1(" ,"),_L1(","),
     _L1(".:"),_L1(":"),
     _L1(":."),_L1(":"),
     _L1("eVe"),_L1("eve"),
     _L1("    }"),_L1("  }"),
     _L1("  "),_L1(" "),
     NULL,NULL
    };
   int i=1;
   //"--" at the beginning of dialogs (Spy Kids 2, Stolen Summer, Sweet Home Alabama)
   if (S1[i]=='-' && S1[i+1]=='-' && S1[i+2]!='-')
    S1.erase(i,1);

   //dict
   for (i=0;punctinations[2*i];i++)
    S1=stringreplace(S1,punctinations[2*i],punctinations[2*i+1],rfReplaceAll);

   //".." to "..."
   W1=S1.find(_L1(".."));
   if (W1>0)
    while (S1.size()>W1)
     {
      if (((W1== 1) || (S1[W1 - 1] != '.')) &&
          (S1[W1] == '.') && (S1[W1 + 1] == '.') &&
          ((S1.size() == W1 + 1) || (S1[W1 + 2] != '.'))
         )
       S1.insert(W1,_L1("."));
      W1++;
     }

   //"..."
   do
    {
     W1 = S1.find(_L1("..."));
     if (W1 > 0)
      //Example "It is ..." -> "It is..."
      if ((W1 == S1.size() - 2) && (S1[W1 - 1] == ' '))
         S1 = S1.copy(1, S1.size() - 4) + _L1("<><>");
       else
         //Example "... it is" -> "...it is"
         if ((W1 == 1) && (S1[W1 + 3] == ' '))
           S1 = _L1("<><>") + S1.copy(5, S1.size() - 3);
         else
          //Example "It...is" -> "It... is"
          if ((W1 > 1) && (W1 < S1.size() - 2) &&
              !(S1[W1 - 2]=='?' || S1[W1-2]=='!'))
           S1 = S1.copy(1, W1 - 1) + _L1("<><> ") + S1.copy(W1 + 3, S1.size() - (W1 + 2));
          else
           //Example "Who?... there..." -> "Who? ...there..."
           if ((W1 > 1) && (W1 < S1.size() - 2) &&
               (S1[W1 - 1] =='?' || S1[W1-1]=='!'))
            S1 = S1.copy( 1, W1 - 1) + _L1(" <><>") + S1.copy( W1 + 4, S1.size() - (W1 + 3));
             else
            S1 = S1.copy( 1, W1 - 1) + _L1("<><>") + S1.copy(W1 + 3, S1.size() - (W1 + 2));
    } while (W1!=0);
   S1 = stringreplace(S1, _L1("?<><> "), _L1("? <><>"), rfReplaceAll);
   S1 = stringreplace(S1, _L1("<><> ?"), _L1("<><>?"), rfReplaceAll);
   S1 = stringreplace(S1, _L1("<><> !"), _L1("<><>!"), rfReplaceAll);
   S1 = stringreplace(S1, _L1("!<><> "), _L1("! <><>"), rfReplaceAll);
   S1 = stringreplace(S1, _L1("\"<><> "), _L1("\"<><>"), rfReplaceAll);
   S1 = stringreplace(S1, _L1("<><> \""), _L1("<><>\""), rfReplaceAll);
   S1 = stringreplace(S1, _L1("- <><> "), _L1("<><>"), rfReplaceAll);
   S1 = stringreplace(S1, _L1("- <><>"), _L1("<><>"), rfReplaceAll);
   S1 = stringreplace(S1, _L1("-<><> "), _L1("<><>"), rfReplaceAll);
   S1 = stringreplace(S1, _L1("-<><>"), _L1("<><>"), rfReplaceAll);
   S1 = stringreplace(S1, _L1(" <><> "), _L1("<><> "), rfReplaceAll);
   S1 = stringreplace(S1, _L1(",<><>"), _L1("<><>"), rfReplaceAll);

   //|"_| -> |"| or |_"| -> |"|  -  sometimes in safe cases
   int QoutCnt = 0;
   for (i=1;i<=S1.size();i++)
    if (S1[i]=='"')
     QoutCnt++;
   if (QoutCnt > 0)
    if (!odd(QoutCnt))
     {
      for (i=1;i<=S1.size();i++)
       if (S1[i] == '"')
        {
         if (odd(QoutCnt))
          {
           if ((i > 1) && (S1[i - 1] == ' ')) S1.erase(i - 1, 1);
          }
         else
          if ((i < S1.size()) && (S1[i + 1] == ' ')) S1.erase(i + 1, 1);
         QoutCnt--;
        }
     }
    else
     {
      //aftercheck 1: |"_| at position 1 (if only 1x " appear as beginning)
      if (S1.find(_L1("\" ")) == 1) S1.erase(2, 1);
      //aftercheck 2: |_"| at last position (if only 1x " appear as end)
      if (S1.find(_L1(" \"")) == S1.size() - 1) S1.erase(S1.size() - 1, 1);
     }

   //"-"
   do
    {
     W1 = S1.find('-');
     if (W1 > 0)
      {
       //Example "-It is" -> "- It is"
       if ((W1 == 1) && (S1[W1 + 1] != ' '))
        S1 = _L1("[][] ") + S1.copy(2, S1.size() - 1);
       else
        //Example "Hi. -It is." -> "Hi. - It is." (works for styles: "<i>-It is")
        if (((in(S1[W1 - 2],_L1(".!?>"))) || (S1[W1 - 1] == '>')) && (S1[W1 + 1] != ' '))
         S1 = S1.copy(1, W1 - 1) + _L1("[][] ") + S1.copy(W1 + 1, S1.size() - (W1));
        else
         S1 = S1.copy(1, W1 - 1) + _L1("[][]") + S1.copy(W1 + 1, S1.size() - (W1));
      }
    } while (W1 != 0);

   static const wchar_t *chars1[]={_L1("."),_L1(","),_L1(":"),_L1(";"),_L1("%"),_L1("$"),_L1("!"),_L1("?")};
   //Spaces after "," "." ":" ";" "$" "%"
   if ((S1.find(_L1("www.")) == 0) && (S1.find(_L1(".com ")) == 0) &&
       (S1.find('@') == 0) && (S1.find(_L1("tp://")) == 0))
    for (i=0;i<6;i++)
     {
      do
       {
        W1 = S1.find(chars1[i]);
        if (W1 > 0)
         if ((W1 != S1.size() && !(S1[W1 + 1]==' ' || S1[W1 + 1]=='"' || S1[W1 + 1]=='\'' || S1[W1 + 1]=='?' || S1[W1 + 1]=='!' || S1[W1 + 1]==',' || S1[W1 + 1]==']' || S1[W1 + 1]=='}' || S1[W1 + 1]==')' || S1[W1 + 1]=='<'))
             && ((S1[W1 - 1] < '0') || (S1[W1 - 1] > '9')) && ((S1[W1 + 1] < '0') || (S1[W1 + 1] > '9'))
             && (S1[W1 + 2] != '.'))
          S1 = S1.copy(1, W1 - 1) + _L1("()() ") + S1.copy(W1 + 1, S1.size() - (W1));
         else
          S1 = S1.copy(1, W1 - 1) + _L1("()()") + S1.copy(W1 + 1, S1.size() - W1);
       } while (W1 != 0);
      S1 = stringreplace(S1, _L1("()()"), chars1[i], rfReplaceAll);
     }
   //Spaces after "!" "?"
   for (i=6;i<=7;i++)
    {
     do
      {
       W1 = S1.find(chars1[i]);
       if (W1 > 0)
        if ((W1 != S1.size()) && (S1[W1 + 1] != ' ') && !(S1[W1 + 1]=='!' || S1[W1 + 1]=='?' || S1[W1 + 1]=='"' || S1[W1 + 1]==']' || S1[W1 + 1]=='}' || S1[W1 + 1]==')' || S1[W1 + 1]=='<')) //'<' styles)
         S1 = S1.copy(1, W1 - 1) + _L1("()() ") + S1.copy( W1 + 1, S1.size() - (W1));
        else
         S1 = S1.copy(1, W1 - 1) + _L1("()()") + S1.copy(W1 + 1, S1.size() - (W1));
      } while (W1 != 0);
     S1 = stringreplace(S1, _L1("()()"), chars1[i], rfReplaceAll);
    }
   S1 = stringreplace(S1, _L1("[][]"), _L1("-"), rfReplaceAll);
   S1 = stringreplace(S1, _L1("<><>"), _L1("..."), rfReplaceAll);
  }

 if (cfg.fix&fixOrtography)
  for (strings::const_iterator s=odict.begin();s!=odict.end();)
   {
    const ffstring &oldstr=*s;s++;
    const ffstring &newstr=*s;s++;
    S1=stringreplace(S1,oldstr,newstr,rfReplaceAll);
   }

 if (cfg.fix&fixCapital)
  {
   if (cfg.fix&fixCapital2) S1.ConvertToLowerCase();

   //Change "..." to "<><>"
   S1 = stringreplace(S1, _L1("..."), _L1("<><>"), rfReplaceAll);

   static const wchar_t *chars2[]={_L1("-"),_L1("."),_L1("!"),_L1("?"),_L1(":")};

   //Capital letters after ". " "? " "! " "- "
   for (unsigned int i=0;i<countof(chars2);i++)
    {
     do
      {
       W1 = S1.find(chars2[i]+ffstring(_L1(" ")));
       if (W1 > 1) //take Pos from 2 and higher ("- " elsewhere)
        if (W1 != S1.size())
         if (((i == 1) /*'.'*/ && (S1[W1 - 1]>='0' && S1[W1-1]<='9')) || //prevents: 28. srpna -> 28. Srpna
             ((i == 0) /*'-'*/ && !(S1[W1 - 2]=='.' || S1[W1 - 2]=='!' || S1[W1 - 2]=='?')))  //UpperCase only after .!?
          S1 = S1.copy(1, W1 - 1) + _L1("()()") + S1.copy(W1 + 1, S1.size() - W1);
         else
          {
           wchar_t up[2];
           up[0]=(wchar_t)toupper(S1[W1 + 2]);up[1]='\0';
           S1 = S1.copy(1, W1 - 1) + _L1("()() ") + passtring<wchar_t>(up) + S1.copy(W1 + 3, S1.size() - (W1 + 2));
          }
        else
         S1 = S1.copy(1, W1 - 1) + _L1("()()") + S1.copy(W1 + 1, S1.size() - W1);
      } while (W1 > 1);
     S1 = stringreplace(S1, _L1("()()"), chars2[i], rfReplaceAll);
    }

   //Change "<><>" back to "..."
   S1 = stringreplace(S1, _L1("<><>"), _L1("..."), rfReplaceAll);

   //change the 1st letter case according to previous subtitle line
   if (EndOfPrevSentence && (S1[1] != '.')) //...bla
    {
     int J = 1;
     //while ((S1[J] == '<') && (S1[J + 1]=='b' || S1[J + 1]=='i' || S1[J + 1]=='u')) && //styles
     //       (S1[J + 2] == '>'))
     // j+=, 3);
     if (S1[J] != '.') //<i>...bla
      while (J <= S1.size())
       {
        //Alex: (battle cries) --> (Battle cries)
        //It's not expected behavior. It'll be much better to stay words in "()" intact.
        if (!(S1[J]=='[' || S1[J]=='"' || S1[J]=='\'' || S1[J]=='#' || S1[J]=='-' || S1[J]==' '))
         {
          S1[J] = (wchar_t)toupper(S1[J]);
          break;
         }
        J++;
       }
     }
   //prepare for next line
   passtring<wchar_t> Sstrip = S1; //styles
   unsigned int J = Sstrip.size();
   if (J==0) //in case of empty string (only styles)
    EndOfPrevSentence = false;
   else
    {
     while ((Sstrip[J]=='\'' || Sstrip[J]=='"' || Sstrip[J]==' ') && (J > 1))
      J--;
     EndOfPrevSentence = Sstrip[J]=='.' || Sstrip[J]=='!' || Sstrip[J]=='?' || Sstrip[J]==':' || Sstrip[J]==']' || Sstrip[J]==')';
     if ((Sstrip[J] == '.') && (Sstrip[J - 1] == '.') && (Sstrip[J - 2] == '.'))
      EndOfPrevSentence = false;
    }
  }

 if (cfg.fix&fixNumbers) //1 993 --> 1993 etc.
  for (int I=1;I<=S1.size()-2;I++)
   if ((S1.size() >= I + 2) /*Delete!*/ &&
       (S1[I]=='0' || S1[I]=='1' || S1[I]=='2' || S1[I]=='3' || S1[I]=='4' || S1[I]=='5' || S1[I]=='6' || S1[I]=='7' || S1[I]=='8' || S1[I]=='9' || S1[I]=='/') &&
       (S1[I + 1] == ' '))
    if ((S1[I + 2]=='0' || S1[I + 2]=='1' || S1[I + 2]=='2' || S1[I + 2]=='3' || S1[I + 2]=='4' || S1[I + 2]=='5' || S1[I + 2]=='6' || S1[I + 2]=='7' || S1[I + 2]=='8' || S1[I + 2]=='9' || S1[I + 2]==',' || S1[I + 2]=='.' || S1[I + 2]==':' || S1[I + 2]=='/') ||
        //5 -1-5
        ((S1[I + 2] == '-') &&
         (S1.size() >= I + 3) /*Delete!*/ &&
         (S1[I + 3]=='0' || S1[I + 3]=='1' || S1[I + 3]=='2' || S1[I + 3]=='3' || S1[I + 3]=='4' || S1[I + 3]=='5' || S1[I + 3]=='6' || S1[I + 3]=='7' || S1[I + 3]=='8' || S1[I + 3]=='9')))
     S1.erase(I + 1, 1);

 if (cfg.fix&fixHearingImpaired)
  for (int I=1;I<=S1.size();I++)
   {
    if (inHearing)
     {
      W1=S1.find(']',I);
      if (W1>0)
       {
        S1.erase(std::max(1,I-1),std::max(W1 - std::max(1,I-1) + 1, 0));
        if (S1 == passtring<wchar_t>(_L1("- ")) || S1 == passtring<wchar_t>(_L1("-")))
         S1 = _L1("");
       }
      else
       {
        S1.erase(std::max(1,I-1),S1.size());
        break;
       }
      inHearing=false;
      I=W1+1;
     }
    else if (S1[I]=='[')
     inHearing=true;
   }
 bool useFixed=strcmp(text.c_str(),S1.c_str())!=0;
 if (useFixed)
  fixed=S1.c_str();
 return useFixed;
}

//============================ TsubtitleFormat =============================
ffstring TsubtitleFormat::getAttribute(const wchar_t *start,const wchar_t *end,const wchar_t *attrname)
{
 if (const wchar_t *attr=strnistr(start,end-start+1,attrname))
  if (const wchar_t *eq=strnchr(attr,end-attr+1,'='))
   {
    eq++;
    bool in=false;
    for (int i=0;i<end-eq+1;i++)
     if (eq[i]=='"')
      in=!in;
     else if (!in && (eq[i]==' ' || eq[i]=='>' || eq[i]=='\0'))
      return stringreplace(ffstring(eq,i).Trim(),_L1("\""),_L1(""),rfReplaceAll);
   }
 return ffstring();
}

void TsubtitleFormat::processHTMLTags(Twords &words, const wchar_t* &l, const wchar_t* &l1, const wchar_t* &l2)
{
if (_strnicmp(l2,_L1("<i>"),3)==0) {words.add(l,l1,l2,props,3);props.italic=true;}
  else if (_strnicmp(l2,_L1("</i>"),4)==0) {words.add(l,l1,l2,props,4);props.italic=false;}
  else if (_strnicmp(l2,_L1("<u>"),3)==0) {words.add(l,l1,l2,props,3);props.underline=true;}
  else if (_strnicmp(l2,_L1("</u>"),4)==0) {words.add(l,l1,l2,props,4);props.underline=false;}
  else if (_strnicmp(l2,_L1("<b>"),3)==0) {words.add(l,l1,l2,props,3);props.bold=true;}
  else if (_strnicmp(l2,_L1("</b>"),4)==0) {words.add(l,l1,l2,props,4);props.bold=false;}
  else if (_strnicmp(l2,_L1("<font "),6)==0)
   {
    const wchar_t *end=strchr(l2,'>');
    if (end)
     {
      const wchar_t *start=l2+6;
      ffstring face=getAttribute(start,end,_L1("face"));
      ffstring color=getAttribute(start,end,_L1("color")).ConvertToLowerCase();
      ffstring size=getAttribute(start,end,_L1("size"));
      words.add(l,l1,l2,props,end-l2+1);
      if (!face.empty()) ff_strncpy(props.fontname, (const char_t *)text<char_t>(face.c_str()),countof(props.fontname));
      if (!color.empty() && ((color[0]=='#' && swscanf(color.c_str()+1,_L1("%x"),&props.color)) || (htmlcolors->getColor(color,&props.color,0xffffff),true)))
       {
        std::swap(((uint8_t*)&props.color)[0],((uint8_t*)&props.color)[2]);
        props.isColor=true;
       }
      if (!size.empty())
       {
        wchar_t *ll;
        int s=strtol(size.c_str(),&ll,10);
        if (*ll=='\0') props.size=s;
       }
     }
   }
  else if (_strnicmp(l2,_L1("</font>"),7)==0)
   {
    words.add(l,l1,l2,props,7);props.isColor=false;props.size=0;props.fontname[0]='\0';
   }
  else
   l2++;
}

TsubtitleFormat::Twords TsubtitleFormat::processHTML(const TsubtitleLine &line)
{
 Twords words;
 if (line.empty()) return words;
 const wchar_t *l=line[0];
 const wchar_t *l1=l,*l2=l;
 while (*l2)
   processHTMLTags(words, l, l1, l2);
 
 words.add(l,l1,l2,props,0);
 return words;
}

int TsubtitleFormat::Tssa::parse_parentheses(TparenthesesContents &contents, ffstring arg)
{
    // (a1,a2, ... ,an) is expected.
    size_t first_paren = arg.find('(');
    if (first_paren == ffstring::npos)
        return -1;
    size_t second_paren = arg.find(')', first_paren + 1);
    if (second_paren == ffstring::npos)
        return -1;
    arg.erase(0,first_paren+1);
    arg.erase(second_paren - first_paren - 1);
    strings input_strings;
    strtok(arg.c_str(),_L1(","),input_strings);
    foreach (ffstring &s, input_strings)
        contents.push_back(TparenthesesContent(s));
    return second_paren + 1;
}

int TsubtitleFormat::Tssa::TstoreParams::writeProps(const TparenthesesContents &contents, TSubtitleProps *props)
{
    int count = 0;
    iterator store_i = begin();
    TparenthesesContents::const_iterator contents_i = contents.begin();
    for ( ; store_i != end() ; store_i++){
        if (store_i->offset) {
            int64_t val;
            if ( contents_i != contents.end()
              && contents_i->ok) {
                if (contents_i->intval < store_i->min) {
                    val = store_i->min;
                } else if (contents_i->intval > store_i->max) {
                    val = store_i->max;
                } else {
                    val = contents_i->intval;
                    count++;
                }
            } else {
                val = store_i->default_value;
            }
            void *dst = (uint8_t *)props + store_i->offset;
            memcpy(dst, &val, store_i->size);
        }
        if (contents_i != contents.end())
            contents_i++;
    }
    return count;
}

void TsubtitleFormat::Tssa::fontName(ffstring &arg)
{
    if (arg.size()) {
        memset(props.fontname,0,sizeof(props.fontname));
        text<char_t>(arg.c_str(), arg.size(), (char_t*)props.fontname, countof(props.fontname));
    } else
        ff_strncpy(props.fontname, defprops.fontname, countof(props.fontname));
}

template<int TSubtitleProps::*offset,int min,int max> void TsubtitleFormat::Tssa::intProp(ffstring &arg)
{
    int enc;
    if (arg2int(arg,min,max,enc))
        props.*offset=enc;
    else
        props.*offset=defprops.*offset;
}

template<int TSubtitleProps::*offset,int min,int max> void TsubtitleFormat::Tssa::intPropAn(ffstring &arg)
{
    int enc;
    if (arg2int(arg,min,max,enc))
        props.*offset=TSubtitleProps::alignASS2SSA(enc);
    else
        props.*offset=defprops.*offset;
}

template<double TSubtitleProps::*offset,int min,int max> void TsubtitleFormat::Tssa::doubleProp(ffstring &arg)
{
    const wchar_t* buf = arg.c_str();
    wchar_t *bufend;
    double enc=strtod(buf,&bufend);
    if (buf!=bufend && *bufend=='\0' && isIn(enc,(double)min,(double)max))
        props.*offset=enc;
    else
        props.*offset=defprops.*offset;
}

void TsubtitleFormat::Tssa::pos(ffstring &arg)
{
    // (x1,y1) is expected.
    TstoreParams store;
    store.push_back(TstoreParam(offsetof(TSubtitleProps, posx), 0,INT_MAX,defprops.posx,sizeof(props.posx)));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, posy), 0,INT_MAX,defprops.posy,sizeof(props.posy)));

    TparenthesesContents contents;
    parse_parentheses(contents,arg);
    if (store.writeProps(contents, &props) == 2)
        props.isPos=true;
}

void TsubtitleFormat::Tssa::org(ffstring &arg)
{
    // (x1,y1) is expected.
    TstoreParams store;
    store.push_back(TstoreParam(offsetof(TSubtitleProps, org.x), 0,INT_MAX,defprops.org.x,sizeof(props.org.x)));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, org.y), 0,INT_MAX,defprops.org.x,sizeof(props.org.y)));

    TparenthesesContents contents;
    parse_parentheses(contents,arg);
    if (store.writeProps(contents, &props) == 2)
        props.isOrg=true;
}

void TsubtitleFormat::Tssa::move(ffstring &arg)
{
     // (x1,y1,x2,y2,[t1[,t2]]) is expected.
     TstoreParams store;
     store.push_back(TstoreParam(offsetof(TSubtitleProps, posx), 0,INT_MAX, defprops.posx,  sizeof(props.posx)));
     store.push_back(TstoreParam(offsetof(TSubtitleProps, posy), 0,INT_MAX, defprops.posy,  sizeof(props.posy)));
     store.push_back(TstoreParam(offsetof(TSubtitleProps, posx2),0,INT_MAX, defprops.posx2, sizeof(props.posx2)));
     store.push_back(TstoreParam(offsetof(TSubtitleProps, posy2),0,INT_MAX, defprops.posy2, sizeof(props.posy2)));
     store.push_back(TstoreParam(offsetof(TSubtitleProps, t1),   0,UINT_MAX,0,              sizeof(props.t1)));
     store.push_back(TstoreParam(offsetof(TSubtitleProps, t2),   0,UINT_MAX,0,              sizeof(props.t2)));

    TparenthesesContents contents;
    parse_parentheses(contents,arg);
    if (store.writeProps(contents, &props) >= 4)
        props.isMove=true;
}

void TsubtitleFormat::Tssa::fad(ffstring &arg)
{
    TstoreParams store;
    store.push_back(TstoreParam(offsetof(TSubtitleProps, fadeT3), 0,LLONG_MAX/10000,defprops.fadeT3,sizeof(props.fadeT3)));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, fadeT4), 0,LLONG_MAX/10000,defprops.fadeT4,sizeof(props.fadeT4)));

    TparenthesesContents contents;
    parse_parentheses(contents,arg);
    if (store.writeProps(contents, &props) == 2) {
        props.isFad=true;
        props.fadeA1=0;
        props.fadeA2=255;
        props.fadeA3=0;
        props.fadeT1=props.tStart;
        props.fadeT2=props.fadeT1 + (props.fadeT3 * 10000);
        props.fadeT3=props.tStop - (props.fadeT4 * 10000);
        props.fadeT4=props.tStop;
    }
}

void TsubtitleFormat::Tssa::karaoke_kf(ffstring &arg)
{
    intProp<&TSubtitleProps::tmpFadT1, 0, INT_MAX>(arg);
    props.karaokeDuration = (REFERENCE_TIME)props.tmpFadT1 * 100000;
    props.karaokeMode = TSubtitleProps::KARAOKE_kf;
    props.karaokeNewWord = true;
}
void TsubtitleFormat::Tssa::karaoke_ko(ffstring &arg)
{
    intProp<&TSubtitleProps::tmpFadT1, 0, INT_MAX>(arg);
    props.karaokeDuration = (REFERENCE_TIME)props.tmpFadT1 * 100000;
    props.karaokeMode = TSubtitleProps::KARAOKE_ko;
    props.karaokeNewWord = true;
}
void TsubtitleFormat::Tssa::karaoke_k(ffstring &arg)
{
    intProp<&TSubtitleProps::tmpFadT1, 0, INT_MAX>(arg);
    props.karaokeDuration = (REFERENCE_TIME)props.tmpFadT1 * 100000;
    props.karaokeMode = TSubtitleProps::KARAOKE_k;
    props.karaokeNewWord = true;
}

void TsubtitleFormat::Tssa::fade(ffstring &arg)
{
    // \fade(<a1>, <a2>, <a3>, <t1>, <t2>, <t3>, <t4>)
    TstoreParams store;
    store.push_back(TstoreParam(offsetof(TSubtitleProps, fadeA1), 0, 255,             0,    sizeof(props.fadeA1)));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, fadeA2), 0, 255,             255,  sizeof(props.fadeA2)));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, fadeA3), 0, 255,             0,    sizeof(props.fadeA3)));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, fadeT1), 0, LLONG_MAX/10000, 0,    sizeof(props.fadeT1)));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, fadeT2), 0, LLONG_MAX/10000, 1000, sizeof(props.fadeT2)));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, fadeT3), 0, INT_MAX, 2000, sizeof(props.fadeT3)));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, fadeT4), 0, INT_MAX, 3000, sizeof(props.fadeT4)));

    TparenthesesContents contents;
    parse_parentheses(contents,arg);
    store.writeProps(contents, &props);

    props.fadeA1 = 256 - props.fadeA1;
    props.fadeA2 = 256 - props.fadeA2;
    props.fadeA3 = 256 - props.fadeA3;

    props.fadeT1 = props.tStart + props.fadeT1 * 10000;
    props.fadeT2 = props.tStart + props.fadeT2 * 10000;
    props.fadeT3 = props.tStart + props.fadeT3 * 10000;
    props.fadeT4 = props.tStart + props.fadeT4 * 10000;
    props.isFad = true;
}

template<int TSubtitleProps::*offset1,int TSubtitleProps::*offset2,int min,int max> bool TsubtitleFormat::Tssa::intProp2(ffstring &arg)
{
    // (x,y) is expected.
    TstoreParams store;
    store.push_back(TstoreParam((int64_t TSubtitleProps::*)offset1,min,max,defprops.*offset1,sizeof(int)));
    store.push_back(TstoreParam((int64_t TSubtitleProps::*)offset2,min,max,defprops.*offset2,sizeof(int)));

    TparenthesesContents contents;
    parse_parentheses(contents,arg);
    return store.writeProps(contents,props) == 2;
}

bool TsubtitleFormat::Tssa::arg2int(const ffstring &arg, int min, int max, int &enc)
{
    const wchar_t* buf = arg.c_str();
    wchar_t *bufend;
    enc=strtol(buf,&bufend,10);
    return (buf!=bufend && *bufend=='\0' && isIn(enc,min,max));
}

bool TsubtitleFormat::Tssa::color2int(ffstring arg, int &intval)
{
    int radix;
    if (arg.ConvertToLowerCase().compare(0,2,ffstring(_l("&h")))==0) {
        radix=16;
        arg.erase(0,2);
    } else
        radix=10;
    wchar_t *endbuf;
    intval=strtol(arg.c_str(),&endbuf,radix);
    return *endbuf=='&';
}

template<COLORREF TSubtitleProps::*offset> void TsubtitleFormat::Tssa::color(ffstring &arg)
{
    int c;
    if (color2int(arg,c)) {
        props.*offset=c;
        props.isColor=true;
    } else {
        props.*offset=defprops.*offset;
        props.isColor=defprops.isColor;
    }
}

template<int TSubtitleProps::*offset> void TsubtitleFormat::Tssa::alpha(ffstring &arg)
{
    int a;
    if (color2int(arg,a)) {
        props.*offset=256-a;
        props.isColor=true;
    } else {
        props.*offset=defprops.*offset;
        props.isColor=defprops.isColor;
    }
}
void TsubtitleFormat::Tssa::alphaAll(ffstring &arg)
{
    int a;
    if (color2int(arg,a)) {
         props.colorA=256-a;
         props.OutlineColourA=256-a;
         props.ShadowColourA=256-a;
         props.isColor=true;
    } else {
         props.colorA=defprops.colorA;
         props.OutlineColourA=defprops.OutlineColourA;
         props.ShadowColourA=defprops.ShadowColourA;
         props.isColor=defprops.isColor;
    }
}

template<bool TSubtitleProps::*offset> void TsubtitleFormat::Tssa::boolProp(ffstring &arg)
{
    if (arg.size() && arg[0]=='1')
        props.*offset=true;
    else if (arg.size() && arg[0]=='0')
        props.*offset=false;
    else
        props.*offset=defprops.*offset;
}

void TsubtitleFormat::Tssa::reset(ffstring &arg)
{
    props=defprops;
}

bool TsubtitleFormat::Tssa::processTokenI(const wchar_t* &l2,const wchar_t *tok,TssaAction action,Tstr_cmp_func str_cmp_func)
{
    size_t toklen=strlen(tok);
    if (str_cmp_func(l2,tok,toklen)==0) {
        const wchar_t *end1=strchr(l2+2,'\\');
        const wchar_t *end2=strchr(l2,'}');
        const wchar_t *end=(end1 && end1<end2)?end1:end2;
        if (end)
         {
          const wchar_t *start=l2+toklen;
          ffstring arg(start,end - start);
          if (action)
           (this->*action)(arg);
          l2=(end1 && end1<end2)?end1:end2+1;
         }
        return true;
    } else
        return false;
}

// case sensitive version
bool TsubtitleFormat::Tssa::processTokenC(const wchar_t* &l2,const wchar_t *tok,TssaAction action)
{
    return processTokenI(l2,tok,action,strncmp);
}

bool TsubtitleFormat::Tssa::processToken(const wchar_t* &l2,const wchar_t *tok,TssaAction action)
{
    return processTokenI(l2,tok,action,_strnicmp);
}

void TsubtitleFormat::Tssa::processTokens(const wchar_t *l,const wchar_t* &l1,const wchar_t* &l2,const wchar_t *end)
{
    const wchar_t *l3=l2+1;
    words.add(l,l1,l2,props,end-l2+1);
    while (l3<end) {
        if (
            !processToken(l3,_L1("\\1a"),&Tssa::template alpha<&TSubtitleProps::colorA>) &&
            !processToken(l3,_L1("\\2a"),&Tssa::template alpha<&TSubtitleProps::SecondaryColourA>) &&
            !processToken(l3,_L1("\\3a"),&Tssa::template alpha<&TSubtitleProps::OutlineColourA>) &&
            !processToken(l3,_L1("\\4a"),&Tssa::template alpha<&TSubtitleProps::ShadowColourA>) &&
            !processToken(l3,_L1("\\1c"),&Tssa::template color<&TSubtitleProps::color>) && 
            !processToken(l3,_L1("\\2c"),&Tssa::template color<&TSubtitleProps::SecondaryColour>) &&
            !processToken(l3,_L1("\\3c"),&Tssa::template color<&TSubtitleProps::OutlineColour>) &&
            !processToken(l3,_L1("\\4c"),&Tssa::template color<&TSubtitleProps::ShadowColour>) &&
            !processToken(l3,_L1("\\alpha"),&Tssa::alphaAll) &&
            !processToken(l3,_L1("\\an"),&Tssa::template intPropAn<&TSubtitleProps::alignment,1,9>) &&
            !processToken(l3,_L1("\\a"),&Tssa::template intProp<&TSubtitleProps::alignment,1,11>) &&
            !processToken(l3,_L1("\\bord"),&Tssa::template doubleProp<&TSubtitleProps::outlineWidth,0,100>) &&
            !processToken(l3,_L1("\\be"),&Tssa::template boolProp<&TSubtitleProps::blur>) &&
            !processToken(l3,_L1("\\b"),&Tssa::template intProp<&TSubtitleProps::bold,0,1>) &&
            !processToken(l3,_L1("\\clip"),NULL) &&
            !processToken(l3,_L1("\\c"),&Tssa::template color<&TSubtitleProps::color>) &&
            !processToken(l3,_L1("\\fn"),&Tssa::fontName) &&
            !processToken(l3,_L1("\\fscx"),&Tssa::template intProp<&TSubtitleProps::scaleX,1,1000>) &&
            !processToken(l3,_L1("\\fscy"),&Tssa::template intProp<&TSubtitleProps::scaleY,1,1000>) &&
            !processToken(l3,_L1("\\fsp"),&Tssa::template doubleProp<&TSubtitleProps::spacing,INT_MIN+1,INT_MAX>) &&
            !processToken(l3,_L1("\\fs"),&Tssa::template intProp<&TSubtitleProps::size,1,INT_MAX>) &&
            !processToken(l3,_L1("\\fe"),&Tssa::template intProp<&TSubtitleProps::encoding,0,255>) &&
            !processToken(l3,_L1("\\i"),&Tssa::template boolProp<&TSubtitleProps::italic>) &&
            !processToken(l3,_L1("\\fade"),&Tssa::fade) &&
            !processToken(l3,_L1("\\fad"),&Tssa::fad) &&
            !processToken(l3,_L1("\\pos"),&Tssa::pos) &&
            !processToken(l3,_L1("\\move"),&Tssa::move) &&
            !processToken(l3,_L1("\\org"),&Tssa::org) &&
            !processToken(l3,_L1("\\q"),&Tssa::template intProp<&TSubtitleProps::wrapStyle,0,3>) &&
            !processToken(l3,_L1("\\r"),&Tssa::reset) &&
            !processToken(l3,_L1("\\shad"),&Tssa::template doubleProp<&TSubtitleProps::shadowDepth,0,30>) &&
            !processToken(l3,_L1("\\s"),&Tssa::template boolProp<&TSubtitleProps::strikeout>) &&
            !processToken(l3,_L1("\\u"),&Tssa::template boolProp<&TSubtitleProps::underline>) &&
            !processToken(l3,_L1("\\kf"),&Tssa::karaoke_kf) &&
            !processToken(l3,_L1("\\ko"),&Tssa::karaoke_ko) &&
            !processTokenC(l3,_L1("\\K"),&Tssa::karaoke_kf) &&
            !processTokenC(l3,_L1("\\k"),&Tssa::karaoke_k)
        )
        l3++;
    }
}

TsubtitleFormat::Twords TsubtitleFormat::processSSA(const TsubtitleLine &line, int sfmt, TsubtitleText &parent)
{
    Twords words;
    if (line.empty()) return words;
    const wchar_t *l=line[0];
    props=parent.defProps;
    const wchar_t *l1=l,*l2=l;
    Tssa ssa(props,parent.defProps,words);
    while (*l2) {
        if (l2[0]=='{' /*&& l2[1]=='\\'*/) {
            if (const wchar_t *end=strchr(l2+1,'}')) {
                ssa.processTokens(l,l1 ,l2,end);
                l2=end+1;
                continue;
            }
            l2++;
        }
        // Process HTML tags in SSA subs when extended tags option is checked
        else if (parent.defProps.extendedTags) // Add HTML support within SSA
            processHTMLTags(words,l,l1,l2);
        else
            l2++;
    }

    words.add(l,l1,l2,props,0);
    parent.defProps=props;
    return words;
}

void TsubtitleFormat::processMicroDVD(TsubtitleText &parent, std::vector< TsubtitleLine >::iterator it)
{
 if (it->empty()) return;
 const wchar_t *line0=(*it)[0],*line=line0;
 while (*line)
  if (line[0]=='}' || line[0]==' ') line++;
  else if (_strnicmp(line,_L1("{y:"),3)==0)
   {
    const wchar_t *end=strchr(line+3,'}');
    if (end==NULL) break;
    bool all=!!iswupper(line[1]);
    if (std::find_if(line+3,end,Tncasecmp<'i'>())!=end) parent.propagateProps(all?parent.begin():it,&TSubtitleProps::italic   ,true,all?parent.end():it+1);
    if (std::find_if(line+3,end,Tncasecmp<'b'>())!=end) parent.propagateProps(all?parent.begin():it,&TSubtitleProps::bold     ,1,all?parent.end():it+1);
    if (std::find_if(line+3,end,Tncasecmp<'u'>())!=end) parent.propagateProps(all?parent.begin():it,&TSubtitleProps::underline,true,all?parent.end():it+1);
    if (std::find_if(line+3,end,Tncasecmp<'s'>())!=end) parent.propagateProps(all?parent.begin():it,&TSubtitleProps::strikeout,true,all?parent.end():it+1);
    line=end+1;
   }
  else if (_strnicmp(line,_L1("{s:"),3)==0)
   {
    int size;
    if (swscanf(line,_L1("{s:%i}"),&size) || swscanf(line,_L1("{S:%i}"),&size))
     {
      parent.propagateProps(iswupper(line[1])?parent.begin():it,&TSubtitleProps::size,size,iswupper(line[1])?parent.end():it+1);
      const wchar_t *r=strchr(line,'}');
      if (r)
       line=r+1;
     }
   }
  else if (_strnicmp(line,_L1("{c:$"),4)==0)
   {
    COLORREF color;
    if (swscanf(line,_L1("{c:$%x}"),&color) || swscanf(line,_L1("{C:$%x}"),&color))
     {
      parent.propagateProps(iswupper(line[1])?parent.begin():it,&TSubtitleProps::color,color,iswupper(line[1])?parent.end():it+1);
      const wchar_t *r=strchr(line,'}');
      if (r)
       {
        parent.propagateProps(iswupper(line[1])?parent.begin():it,&TSubtitleProps::isColor,true,iswupper(line[1])?parent.end():it+1);
        line=r+1;
       }
     }
   }
  else
   break;
 (*it)[0].eraseLeft(line-line0);
}

void TsubtitleFormat::processMPL2(TsubtitleLine &line)
{
 if (line.empty() || !line[0]) return;
 if (line[0][0]=='/')
  {
   foreach (TsubtitleWord &word,line)
    word.props.italic=true;
   line[0].eraseLeft(1);
  }
}

//================================= TsubtitleLine ==================================
size_t TsubtitleLine::strlen(void) const
{
 size_t len=0;
 for (Tbase::const_iterator w=this->begin();w!=this->end();w++)
  len+=::strlen(*w);
 return len;
}
void TsubtitleLine::applyWords(const TsubtitleFormat::Twords &words)
{
 bool karaokeNewWord = false;
 for (TsubtitleFormat::Twords::const_iterator w=words.begin();w!=words.end();w++)
  {
   karaokeNewWord |= w->props.karaokeNewWord;
   this->props=w->props;
   if (w->i1==w->i2 && !karaokeNewWord)
    continue;
   if (w->i1==0 && w->i2==this->front().size())
    {
     this->front().props=w->props;
     return;
    }
   TsubtitleWord word(this->front()+w->i1,w->i2-w->i1);
   word.props=w->props;
   word.props.karaokeNewWord = karaokeNewWord;
   this->push_back(word);
   karaokeNewWord = false;
  }
 if (!this->empty())
  this->erase(this->begin());
}
void TsubtitleLine::format(TsubtitleFormat &format,int sfmt, TsubtitleText &parent)
{
 // Use SSA parser for SRT subs when extended tags option is checked
 // This option will be removed (and SSA parser applied to SUBVIEWER)
 // when the garble issue with Shift JIS (ANSI/DBCS) subs will be resovled
 if (sfmt==Tsubreader::SUB_SSA || (sfmt==Tsubreader::SUB_SUBVIEWER && parent.defProps.extendedTags))
  applyWords(format.processSSA(*this, sfmt, parent));
 else
  applyWords(format.processHTML(*this));
}void TsubtitleLine::fix(TtextFix &fix)
{
 foreach (TsubtitleWord &word, *this)
  word.fix(fix);
}

//================================= TsubtitleText ==================================

// Copy constructor. mutex cannot be copied.
TsubtitleText::TsubtitleText(const TsubtitleText &src):
    Tsubtitle(src)
{
    insert(end(),src.begin(),src.end());
    lines = src.lines;
    subformat = src.subformat;
    defProps = src.defProps;
    old_prefs = src.old_prefs;
    rendering_ready = src.rendering_ready;
}

void TsubtitleText::format(TsubtitleFormat &format)
{
    int sfmt=subformat&Tsubreader::SUB_FORMATMASK;
    foreach (TsubtitleLine &line, *this)
        line.format(format,sfmt,*this);

    for (Tbase::iterator l=this->begin();l!=this->end();l++)
        format.processMicroDVD(*this,l);

    if (sfmt==Tsubreader::SUB_MPL2)
        foreach (TsubtitleLine &line, *this)
            format.processMPL2(line);
}

void TsubtitleText::prepareKaraoke(void)
{
    int sfmt=subformat&Tsubreader::SUB_FORMATMASK;
    if (sfmt != Tsubreader::SUB_SSA)
        return;

    TsubtitleText temp(subformat, defProps);
    TsubtitleLine tempLine;
    int wrapStyle = 0;
    foreach (TsubtitleLine &line, *this) {
        if (line.props.wrapStyle == 2 || line.lineBreakReason == 2) {
            temp.push_back(tempLine);
            tempLine.clear();
        } else if (!tempLine.empty()) {
            tempLine.back().addTailSpace();
        }

        tempLine.props = line.props;
        foreach (TsubtitleWord &word, line) {
            wrapStyle = word.props.wrapStyle;
            tempLine.push_back(word);
        }
    }
    if (!tempLine.empty())
        temp.push_back(tempLine);

    this->clear();
    this->insert(this->end(), temp.begin(),temp.end());

    REFERENCE_TIME karaokeStart = REFTIME_INVALID;
    REFERENCE_TIME karaokeDuration = 0;
    foreach (TsubtitleLine &line, *this) {
        if (karaokeStart != REFTIME_INVALID)
            karaokeStart += karaokeDuration;

        karaokeDuration = 0;
        foreach (TsubtitleWord &word, line) {
          if (karaokeStart == REFTIME_INVALID)
              karaokeStart = word.props.karaokeStart;
          else
              word.props.karaokeStart = karaokeStart;
          
          if (word.props.karaokeNewWord) {
              karaokeStart += karaokeDuration;
              karaokeDuration = word.props.karaokeDuration;
          }
          word.props.karaokeDuration = karaokeDuration;
        }
    }
}

void TsubtitleText::fix(TtextFix &fix)
{
    foreach (TsubtitleLine &line, *this)
        line.fix(fix);
    if (stop == REFTIME_INVALID) {
        size_t len = 0;
        foreach (TsubtitleLine &line, *this)
            len += line.strlen();
        stop = start + len * 900000;
    }
}

void TsubtitleText::print(
    REFERENCE_TIME time,
    bool wasseek,
    Tfont &f,
    bool forceChange,
    TprintPrefs &prefs,
    unsigned char **dst,
    const stride_t *stride)
{
    f.prepareC(this,prefs,false);
}

size_t TsubtitleText::prepareGlyph(const TprintPrefs &prefs, Tfont &font, bool forceChange)
{
    size_t used_memory = 0;
    if (!rendering_ready || forceChange || old_prefs != prefs) {
        //if (!prefs.isOSD) DPRINTF(_l("TsubtitleText::prepareGlyph %I64i"),start);
        old_prefs = prefs;

        unsigned int dx,dy;
        unsigned int gdi_font_scale = prefs.fontSettings.gdi_font_scale;
        if (prefs.sizeDx && prefs.sizeDy) {
            dx=prefs.sizeDx;
            dy=prefs.sizeDy;
        } else {
            dx=prefs.dx;
            dy=prefs.dy;
        }

        IffdshowBase *deci = font.deci;

        lines.clear();
        if (!font.fontManager)
            comptrQ<IffdshowDecVideo>(deci)->getFontManager(&font.fontManager);
        TfontManager *fontManager = font.fontManager;
        bool nosplit=!prefs.fontSettings.split && !(prefs.fontchangesplit && prefs.fontsplit);
        int splitdx0=nosplit ? 0 : ((int)dx-prefs.textBorderLR<1 ? 1 : dx-prefs.textBorderLR) * gdi_font_scale;

        int *pwidths=NULL;
        Tbuffer width;

        for (const_iterator l = begin() ; l != end() ; l++) {
            int charCount=0;
            ffstring allStr;
            Tbuffer tempwidth;
            double left=0.0,nextleft=0.0;
            int wordWrapMode=-1;
            int splitdxMax=splitdx0;
            if (l->empty()) {
                LOGFONT lf;
                TtoGdiFont gf(l->props, font.hdc, lf, prefs, dx, dy, fontManager);
                // empty lines have half height.
                TrenderedSubtitleLine *line=new TrenderedSubtitleLine(l->props, gf.getHeight() / 2.0);
                lines.push_back(line);
            }
            for (TsubtitleLine::const_iterator w=l->begin();w!=l->end();w++) {
                LOGFONT lf;
                TtoGdiFont gf(w->props, font.hdc, lf, prefs, dx, dy, fontManager);
                SetTextCharacterExtra(font.hdc,w->props.spacing==INT_MIN ? prefs.fontSettings.spacing : w->props.get_spacing(dy, prefs.clipdy, gdi_font_scale));
                const wchar_t *p=*w;
                int xscale=w->props.get_xscale(
                        prefs.fontSettings.xscale,
                        prefs.sar,
                        prefs.fontSettings.aspectAuto,
                        prefs.fontSettings.overrideScale)
                    * 100
                    / w->props.get_yscale(
                        prefs.fontSettings.yscale,prefs.sar,
                        prefs.fontSettings.aspectAuto,
                        prefs.fontSettings.overrideScale);
                wordWrapMode=w->props.wrapStyle;
                splitdxMax=get_splitdx_for_new_line(*w, splitdx0, dx, prefs, gdi_font_scale, deci);
                allStr+=p;
                pwidths=(int*)width.resize((allStr.size()+1)*sizeof(int));
                left=nextleft;
                int nfit;
                SIZE sz;
                size_t strlenp=strlen(p);
                int *ptempwidths=(int*)tempwidth.alloc((strlenp+1)*sizeof(int)*2); // *2 to work around Layer For Unicode on Windows 9x.
                GetTextExtentExPointW(font.hdc,p,(int)strlenp,INT_MAX,&nfit,ptempwidths,&sz);
                for (size_t x=0;x<strlenp;x++) {
                    nextleft=(double)ptempwidths[x]*xscale/100+left;
                    pwidths[charCount]=int(nextleft);
                    charCount++;
                }
            }
            if (allStr.empty()) continue;
            if (wordWrapMode==-1) {
                // non SSA/ASS/ASS2
                if (nosplit)
                    wordWrapMode=2;
                else {
                    deci->getParam(IDFF_subWordWrap,&wordWrapMode);
                    if (wordWrapMode>=2) wordWrapMode++;
                }
            }
            TwordWrap wordWrap(wordWrapMode,allStr.c_str(),pwidths,splitdxMax,l->props.version != -1);
            //wordWrap.debugprint();

            TrenderedSubtitleLine *line=NULL;
            int cx=0,cy=0;
            unsigned int refResX=prefs.xinput, refResY=prefs.yinput;
            bool firstLine=true;
            for (TsubtitleLine::const_iterator w0=l->begin();w0!=l->end();w0++) {
                TsubtitleWord w(*w0);
                LOGFONT lf;
                TtoGdiFont gf(w.props, font.hdc, lf, prefs, dx, dy, fontManager);
                SetTextCharacterExtra(font.hdc,w.props.spacing==INT_MIN ? prefs.fontSettings.spacing : w.props.get_spacing(dy, prefs.clipdy, gdi_font_scale));
                if (!line) {
                    line=new TrenderedSubtitleLine(w.props);
                    // Propagate input dimensions to the line properties 
                    // (unless movie dimensions are filled in the script and parameter 
                    // IDFF_subSSAUseMovieDimensions is not checked)
                    if (line->getPropsOfThisObject().refResX && line->getPropsOfThisObject().refResY 
                        && firstLine && !deci->getParam2(IDFF_subSSAUseMovieDimensions)) {
                        refResX=line->getPropsOfThisObject().refResX;
                        refResY=line->getPropsOfThisObject().refResY;
                        firstLine=false;
                    } else {
                        line->getPropsOfThisObject().refResX=refResX;
                        line->getPropsOfThisObject().refResY=refResY;
                    }
                }

                const wchar_t *p=w;
                #ifdef DEBUG
                  DPRINTF(L"%s",p);
                #endif
                int linesInWord=0;
                do {
                    if (linesInWord>0) {
                        while (*p && iswspace((unsigned short)*p)) {
                            cx++;
                            p++;
                        }
                    }
                    int strlenp=(int)strlen(p);
                    // If line goes out of screen, wraps it except if no wrap defined 
                    if (cx+strlenp-1<=wordWrap.getRightOfTheLine(cy)) {
                        // Propagate the input dimensions to the TsubtitleWord props
                        w.props.refResX=refResX;
                        w.props.refResY=refResY;

                        TrenderedTextSubtitleWord *rw = newWord(p, strlenp, prefs, &w, lf, font, w0+1==l->end());
                        if (rw) line->push_back(rw);
                        cx+=strlenp;
                        break;
                    } else {
                        int n=wordWrap.getRightOfTheLine(cy)-cx+1;
                        if (n<=0) {
                            cy++;
                            linesInWord++;
                            n=wordWrap.getRightOfTheLine(cy)-cx+1;
                            if (!line->empty()) {
                                lines.push_back(line);
                                line=new TrenderedSubtitleLine(w.props);
                            }
                            if (cy>=wordWrap.getLineCount())
                                break;
                        }
                        // Propagate the input dimensions to the TsubtitleWord props
                        w.props.refResX=refResX;
                        w.props.refResY=refResY;

                        TrenderedTextSubtitleWord *rw = newWord(p, n, prefs, &w, lf, font, true);
                        w.props.karaokeNewWord = false;
                        w.props.karaokeStart += w.props.karaokeDuration;
                        w.props.karaokeDuration = 0;
                        if (rw)
                            line->push_back(rw);
                        if (!line->empty()) {
                            lines.push_back(line);
                            line=new TrenderedSubtitleLine(w.props);
                        }
                        p+=wordWrap.getRightOfTheLine(cy)-cx+1;
                        cx=wordWrap.getRightOfTheLine(cy)+1;
                        cy++;
                        linesInWord++;
                    }
                } while(cy<wordWrap.getLineCount());
            }
            if (line) {
                if (!line->empty()) {
                    lines.push_back(line);
                } else {
                    delete line;
                }
            }
        }
        used_memory = getRenderedMemorySize();
    }
    rendering_ready = true;
    return used_memory;
}

// FIXME: This is a mixer of TprintPrefs and TSubtitleProps.
TrenderedTextSubtitleWord* TsubtitleText::newWord(
    const wchar_t *s,
    size_t slen,
    TprintPrefs prefs,
    const TsubtitleWord *w,
    const LOGFONT &lf,
    const Tfont &font,
    bool trimRightSpaces)
{
    int gdi_font_scale = prefs.fontSettings.gdi_font_scale;
    TfontSettings &fontSettings = prefs.fontSettings;
    ffstring s1(s);
    if (trimRightSpaces) {
        while (s1.size() && s1.at(s1.size()-1)==' ')
            s1.erase(s1.size()-1,1);
    }

    if (w->props.shadowDepth != -1)  {
        // SSA/ASS/ASS2
        if (w->props.shadowDepth == 0) {
            prefs.shadowMode = 3;
            prefs.shadowSize = 0;
        } else {
            prefs.shadowMode = 2;
            prefs.shadowSize = -1 * w->props.shadowDepth;
        }
    }

    prefs.outlineWidth=w->props.outlineWidth==-1 ? fontSettings.outlineWidth : w->props.outlineWidth;

    if (prefs.shadowMode==-1) {
        // OSD
        prefs.shadowMode = fontSettings.shadowMode;
        prefs.shadowSize = fontSettings.shadowSize;
    }

    YUVcolorA shadowYUV1;
    if (!w->props.isColor) {
        shadowYUV1=prefs.shadowYUV;
        if (prefs.shadowMode<=1)
            shadowYUV1.A = uint32_t(256*sqrt((double)shadowYUV1.A/256.0));
    }
    prefs.outlineBlur=w->props.blur ? true : false;

    if (fontSettings.blur || (w->props.version >= TsubtitleParserSSA::ASS && lf.lfHeight > int(37 * gdi_font_scale))) // FIXME: messy. just trying to resemble vsfilter.
        prefs.blur=true;
    else
        prefs.blur=false;

    if (w->props.outlineWidth==-1 && fontSettings.opaqueBox) {
        prefs.opaqueBox=true;
    }

    double xscale=(double)w->props.get_xscale(fontSettings.xscale,prefs.sar,fontSettings.aspectAuto,fontSettings.overrideScale)*100.0/(double)w->props.get_yscale(fontSettings.yscale,prefs.sar,fontSettings.aspectAuto,fontSettings.overrideScale);
    return new TrenderedTextSubtitleWord(
        font.hdc,
        s1.c_str(),
        slen,
        w->props.isColor ? YUVcolorA(w->props.color,w->props.colorA) : prefs.yuvcolor,
        w->props.isColor ? YUVcolorA(w->props.OutlineColour,w->props.OutlineColourA) : prefs.outlineYUV,
        w->props.isColor ? YUVcolorA(w->props.ShadowColour,w->props.ShadowColourA) : shadowYUV1,
        prefs,
        lf,
        xscale,
        w->props);
}

size_t TsubtitleText::dropRenderedLines(void)
{
    boost::unique_lock<boost::mutex> lock(mutex_lines);
    size_t released_memory = getRenderedMemorySize();
    lines.clear(); // clear pointers and delete objects.
    old_prefs.csp = 0;
    rendering_ready = false;
    return released_memory;
}

void TsubtitleTexts::print(
    REFERENCE_TIME time,
    bool wasseek,
    Tfont &f,
    bool forceChange,
    TprintPrefs &prefs,
    unsigned char **dst,
    const stride_t *stride)
{
    f.reset();
    foreach (TsubtitleText *subtitleText, *this) {
        boost::unique_lock<boost::mutex> lock(*subtitleText->get_lock_ptr(), boost::try_to_lock_t());
        if (!lock.owns_lock()) {
            // hustle glyphThread
            TthreadPriority pr(comptrQ<IffdshowDecVideo>(prefs.deci)->getGlyphThreadHandle(),
                THREAD_PRIORITY_ABOVE_NORMAL,
                THREAD_PRIORITY_BELOW_NORMAL);
            lock.lock();
        }
        subtitleText->print(time,wasseek,f,forceChange,prefs,dst,stride);
    }
    f.print(prefs,dst,stride);
}
