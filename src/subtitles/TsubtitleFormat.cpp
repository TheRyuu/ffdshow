/*
 * Copyright (c) 2003-2006 Milan Cutka
 *               2007-2011 h.yamagata
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
#include "TsubtitleFormat.h"
#include "Tsubreader.h"
#include "TsubtitlesSettings.h"
#include "Tconfig.h"
#include "ThtmlColors.h"

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
    if (scfg->fix&fixOrtography) {
        char_t dictflnm[MAX_PATH];
        _makepath_s(dictflnm, countof(dictflnm), NULL, ffcfg->pth, (::ffstring(_l("dict\\"))+scfg->fixDict).c_str(), _l("dic"));
        TstreamFile fs(dictflnm,false,false);
        if (fs) {
            wchar_t ln[1000];
            while (fs.fgets(ln,1000)) {
                wchar_t *eoln=strchr(ln,'\n');
                if (eoln) {
                    *eoln='\0';
                }
                odict.push_back(ln);
            }
            if (odd(odict.size())) {
                odict.erase(odict.end()-1);
            }
            if (!odict.empty())
                for (strings::iterator s=odict.begin(); s+1!=odict.end(); s++)
                    if (strstr((s+1)->c_str(),s->c_str())) {
                        s=odict.erase(s);
                        s=odict.erase(s);
                    }
        }
    }
}

bool TtextFix::process(ffstring &text,ffstring &fixed)
{
    if (cfg.fix==0) {
        return false;
    }
    int W1;
    passtring<wchar_t> S1=text;

    if (cfg.fix&fixAP) { //AP
        S1=stringreplace(S1,L"`"   , L"'" ,rfReplaceAll);
        S1=stringreplace(S1,L"\264", L"'" ,rfReplaceAll);
        S1=stringreplace(S1,L"''"  , L"\"",rfReplaceAll);
        S1=stringreplace(S1,L"\"\"", L"\"",rfReplaceAll);
    }

    if (cfg.fix&fixIl) { // Il
#define TrChar(n) St1[W1+(n)]
        bool TakeI=false,Takel=false;
        passtring<wchar_t> St1=L"  "+S1+L"  ";
        for (W1=1 + 2; W1<=St1.size() - 2; W1++) {
            //small L --> big i
            if (St1[W1] == 'l') {
                TakeI = false;
                //_lll trio
                if ((TrChar(-1)==' ' || TrChar(-1)=='#') && (TrChar(+1) == 'l') && (TrChar(+2) == 'l')) {
                    //_lllx --> _Illx (Illinois, Illegal etc.) + _Ill-
                    if (in(TrChar(+3),L"-abehinopstuy")) {
                        TakeI = true;
                    }
                    //._Ill_ (=bedridden) + ._Ill. (Ill. = Illustriert, Illustration [German])
                    //other than BoL cases not reflected
                    if (in(TrChar(-2),L".[") && (in(TrChar(+3),L" ."))) {
                        TakeI = true;
                    }
                    //Godfather III / Godfather III.
                    if (!(in(TrChar(-2),L".[") && (in(TrChar(+3),L" .!?,")))) {
                        TakeI = true;
                        St1[W1 + 1] = 'I';
                        St1[W1 + 2] = 'I';
                    }
                }
                //_lgelit _lglu _lgnition _lgrafpapier _lguana
                if (in(TrChar(-1),L" [(#") && (TrChar(+1) == 'g') && in(TrChar(+2),L"elnru")) {
                    TakeI = true;
                }
                //ME LOVES MAKARONl! (MY TO SLYSELl. (!?) [cz; no others affected])
                if (in(TrChar(+1),L".!?") &&
                        (TrChar(-1) == toupper(TrChar(-1))) &&
                        (TrChar(-1) != '.') && //»př.n.l.« at EoL case
                        (TrChar(-2) == toupper(TrChar(-2)))) {
                    TakeI = true;
                }
                //UNlVERSAL, lnternet, lnspektion, lnternational;  not changed - lněný [cz; no others affected]
                //contain  : _l_ case [no Fr!], XlX, Xl_, _lX
                //           V.l. Lenin, l.V. Lenin (initials of names)
                //safely   : -lx OR -_lx; [lx OR [_lx; "Ix OR "_Ix
                //dangerous: .lxxx
                if (((TrChar(-1) == toupper(TrChar(-1))) &&
                        (TrChar(-1) != '.') && //»př.n.l.« case [cz; no others affected]
                        (TrChar(+1) == toupper(TrChar(+1))))
                        ||
                        ((in(TrChar(-1),L" #")) && (TrChar(+1) == 'n') && (TrChar(+2) != '\354'))) {
                    TakeI = true;
                }
                //last sign at uniline OR space follows
                if ((TrChar(+1) == ' ') &&
                        (TrChar(-1) == toupper(TrChar(-1)))) {
                    TakeI = true;
                }
                //._l | -l | -_l; contain: ._ll_ (italian: Il brutto, il nero)
                if (((TrChar(-2) != '.') && (TrChar(-1) == '.')) ||
                        ((TrChar(-2) == ' ') && (TrChar(-1) == '-')) ||
                        ((in(TrChar(-2),L".-")) && (TrChar(-1) == ' ')) &&
                        (TrChar(+1) != 'i')) {
                    TakeI = true;
                }
                //_ll_ to _Il_ ...this is made only as prep for next
                if ((in(TrChar(-1),L" #")) && (TrChar(+1) == 'l') &&
                        (in(TrChar(+2),L" .,?!:"))) {
                    TakeI = true;
                }
                //_Il_ to _II_ (-2 <> . or [) Godfather II & Ramses II
                if ((TrChar(-3) != '.') && (TrChar(-3) != '[') && (TrChar(-3) != '\0') &&
                        (TrChar(-2) == ' ') && (TrChar(-1) == 'I') &&
                        (in(TrChar(+1),L" .,?!:"))) {
                    TakeI = true;
                }
                //"hey C.W. load the weapons" and "hey... load the weapons"
                //      432101                        432101
                if ((TrChar(-4) == '.') && (TrChar(-2) == '.') & (TrChar(-1) == ' ') &&
                        (in(TrChar(+1),L"aeiouy"))) {
                    TakeI = false;
                }
                //_Ital...
                if ((in(TrChar(-1),L" #")) && (TrChar(+1) == 't') && (TrChar(+2) == 'a')) {
                    TakeI = true;
                }
                //only for international abnormalities
                switch (cfg.fixLang) {
                    case 0:// StCorrectIl_En;
                        if (St1[W1] == 'l') {
                            //so shity due the styles
                            if ((in(TrChar(-1),L" \"#")) && (TrChar(+1) == 't' ) && (TrChar(+2) == '\'') && (TrChar(+3) == 's')) {
                                TakeI = true;    //_lt's
                            }
                            if ((in(TrChar(-1),L" \"#")) && (TrChar(+1) == '\'') && (TrChar(+2) == 'm' ) && (TrChar(+3) == ' ')) {
                                TakeI = true;    //_l'm_
                            }
                            if ((in(TrChar(-1),L" \"#")) && (TrChar(+1) == '\'') && (TrChar(+2) == 'd' ) && (TrChar(+3) == ' ')) {
                                TakeI = true;    //_l'd_
                            }
                            if ((in(TrChar(-1),L" \"#")) && (TrChar(+1) == '\'') && (TrChar(+2) == 'v' ) && (TrChar(+3) == 'e')) {
                                TakeI = true;    //_l've
                            }
                            if ((in(TrChar(-1),L" \"#")) && (TrChar(+1) == '\'') && (TrChar(+2) == 'l' ) && (TrChar(+3) == 'l')) {
                                TakeI = true;    //_l'll
                            }
                            if ((in(TrChar(-1),L" \"#")) && (in(TrChar(+1),L"fnst")) && (TrChar(+2) == ' ')) {
                                TakeI = true;    //_lf_, _ln_, _ls_, _lt_
                            }
                            //no English word beginning with "lc", "ld", "lh", "ln", "lr", "ls"
                            if ((in(TrChar(-1),L" \"#")) && (in(TrChar(+1),L"cdhnrs"))) {
                                TakeI = true;
                            }
                        }
                        break;
                    case 1:// StCorrectIl_Fr;
                        //*unknown*: I think that for french neither "I" alone nor "l" alone can exist so we don't check this. So the only thing we can write is that before an apostrophe there must be an "l".
                        if (((in(TrChar(-1),L" \"#")) && (TrChar(+1) == ' ')) || //but Brain as Frenchman: lonesome I doesn't exist
                                (TrChar(+1) == '\'')) { //*unknown*: An I followed by an apostrophe cannot exist. Must be an l.
                            TakeI = false;
                            Takel = true;
                        }
                        break;

                    case 2:// StCorrectIl_Ge;
                        if (St1[W1] == 'l')
                            //no German word beginning with "lc", "ld", "lh", "ln", "lr", "ls"
                            if ((in(TrChar(-1),L" \"#")) && (in(TrChar(+1),L"cdhnrs"))) {
                                TakeI = true;
                            }
                        break;
                    case 3:// StCorrectIl_Cz;
                        if (St1[W1] == 'l')
                            //no Czech word beginning with "lc", "ld", "lr"
                            if ((in(TrChar(-1),L" \"#")) && (in(TrChar(+1),L"cdr"))) {
                                TakeI = true;
                            }
                        break;
                    case 4:// StCorrectIl_It;
                        //Hint: "i" alone CAN exist (even capital: after a full stop) while l alone can't, never
                        //l' processing
                        if ((TrChar(+1) == '\'')) //Ita always has l before apostrophe (if ' is not used as an accent for a capital letter! [rare, and followed by whitespace])
                            if ((in(TrChar(+2),L"aAeEiIloOuUhH\xe0\xe8\xe9\xef\xf2\xf9"))) {
                                Takel = true;
                                TakeI = false;
                            }
                        //else: unknown pattern!? Probably an English sentence within an Italian movie. We rely on the general algorithm.
                        break;
                    case 5:// StCorrectIl_Pl;
                        if (St1[W1] == 'l')
                            //no Czech word beginning with "lc", "ld", "lr"
                            if ((in(TrChar(-1),L"., \"#")) && (in(TrChar(+1),L"bcdfhjkrstz\346\263\361\237"))) {
                                Takel = false;
                                TakeI = true;
                            }
                        break;
                }
                if (TakeI) {
                    St1[W1] = 'I';
                    //'-' for I-I-I...
                    if ((W1 > 4) && (St1[W1 - 2] == 'l') && (St1[W1 - 1] == '-'))
                        if (St1[W1 - 3] == ' ') {
                            St1[W1 - 2] = 'I';
                        } else if ((St1[W1 - 4] == 'l') && (St1[W1 - 3] == '-')) {
                            St1[W1 - 2] = 'I';
                            St1[W1 - 4] = 'I';
                        }
                }
            }
            //big i --> small L
            if (St1[W1] == 'I') {
                Takel = false;
                //_III to _Ill
                if ((in(TrChar(-1),L" #")) && (TrChar(+1) == 'I') && (TrChar(+2) == 'I')) {
                    //_IIIx --> _Illx (Illinois, Illegal etc.) + _Ill-
                    if ((in(TrChar(+3),L"-abehinopstuy"))
                            ||
                            //._Ill_ (=bedridden) + ._Ill. (Ill. = Illustriert, Illustration [German])
                            //other than BoL cases not reflected
                            ((in(TrChar(-2),L".[")) && (in(TrChar(+3),L" .")))) {
                        St1[W1 + 1] = 'l';
                        St1[W1 + 2] = 'l';
                    }
                    //Godfather III / Godfather III. *PASS*
                }
                //All, English, German
                if (TrChar(-1)>='a' && TrChar(-1)<='z' && in(TrChar(-1),L"\344\366\374")) {
                    Takel = true;
                } else if (!((in(TrChar(+1),
                                 //All, English, German; 'w' - Iwao (Japan name), 'v' - Ivan
                                 L"ABCDEFGHIJKLMNOPQRSTUVWXYZ\304\326\334 bcdghklmnoprstvwz\'.,!?")) || //',' prevents PRCl, PRCl, PRCICKY
                             //+East Europe - Czech, Slovak, Polish
                             ((cfg.font.charset== EASTEUROPE_CHARSET) &&
                              (in(TrChar(+1),L"\301\311\314\315\323\332\331\335\310\317\322\330\212\215\216\324\305\274\243\217\257\323\245\312\214\306\321"))) ||
                             //+Scandinavian
                             ((cfg.font.charset== ANSI_CHARSET) &&
                              in(TrChar(+1),L"\xc6\xd8\xc5")
                             ) ||
                             //+Italian: _Ieri_
                             ((in(TrChar(-1),L" #")) && (TrChar(+1) == 'e') &&
                              (TrChar(+2) == 'r') && (cfg.fixLang == 4)
                             ))) {
                    Takel = true;
                }
                //+East Europe - Czech, Slovak, Polish
                if ((cfg.font.charset== EASTEUROPE_CHARSET) &&
                        (in(TrChar(-1),L"\341\350\357\351\354\355\362\363\370\232\235\372\371\375\236\364\345\276\237\277\363\263\271\352\234\346\361"))) {
                    Takel = true;
                }
                //+Scandinavian
                if ((cfg.font.charset== ANSI_CHARSET) &&
                        (in(TrChar(-1),L"\xe6\xf8\xe5"))) {
                    Takel = true;
                }
                //_AIways AIden BIb BIouznit CIaire ČIověk PIný SIožit UItra ZIost
                if ((in(TrChar(-2),L" \"-c#")) && //'c' for Mac/Mc (McCIoy)
                        (in(TrChar(-1),L"ABCDEFGHIJKLMNOPQRSTUVWXYZ\310\212\216\217\214")) &&
                        (in(TrChar(+1),L"abcdefghijklmnopqrstuvwxyz\341\350\351\354\355\363\362\232\371\375"))) {
                    Takel = true;
                }
                //Ihned leave, but Ihostejný --> lhostejný [cz; no others affected]
                if ((TrChar(+1) == 'h') && (TrChar(+2) != 'n')) {
                    Takel = true;
                }
                //_AI_ to _Al_ - name (Artificial Intelligence (AI) not awaited :-)
                if ((in(TrChar(-2),L" \"#'")) &&
                        (TrChar(-1) == 'A') &&
                        (in(TrChar(+1),L" \".,!?'"))) {
                    Takel = true;
                }
                //._II_ to ._Il_ (Il nero on BoL italian)
                if ((in(TrChar(-3),L".[-")) && (TrChar(-2) == ' ') &&
                        (TrChar(-1) == 'I') && (TrChar(+1) == ' ')) {
                    Takel = true;
                }
                //(repd. from l-->I procedure too)  "EI gringo" to "El g." (BoL Spain)
                if ((TrChar(-1) == 'E') && (TrChar(+1) == ' ')) {
                    Takel = true;
                }
                //'II_ --> 'll_
                if ((TrChar(-1) == '\'') && (TrChar(+1) == 'I') && (TrChar(+2) == ' ')) {
                    Takel = true;
                    St1[W1 + 1] = 'l';
                }
                //_Iodic, Iolanthe/Iolite, Ion, Iowa, *Ioya* (but loyalty and more), Iota
                //vs Ioad Iocation Iook Iost Ioud Iove
                //Ioch Iogický/Iogo Ioket Iomcovák Iopata Iovit
                //. Io... (Italian)
                if (!(in(TrChar(-2),L".!?")) &&
                        (in(TrChar(-1),L" \"#")) && (TrChar(+1) == 'o') &&
                        !(in(TrChar(+2),L"dlnwt .,!?"))) {
                    Takel = true;
                }
                //Iodní etc. vs Iodate Ioderma Iodic Iodoethanol [cz only, won't affect others]
                if ((in(TrChar(-1),L" \"#")) && (TrChar(+1) == 'o') && (TrChar(+2) == 'd') &&
                        !(in(TrChar(+3),L"aeio") || TrChar(+3)=='\0')) {
                    Takel = true;
                }
                //Iondýnský [cz only, won't affect others]
                if ((in(TrChar(-1),L" \"#")) && (TrChar(+1) == 'o') && (TrChar(+2) == 'n') &&
                        (TrChar(+3) == 'd')) {
                    Takel = true;
                }
                //_Iong
                if ((in(TrChar(-1),L" \"#")) && (TrChar(+1) == 'o') &&
                        (TrChar(+2) == 'n') && (TrChar(+3) == 'g')) {
                    Takel = true;
                }
                //_Ioni_ --> _loni_  [cz only, don't affect others]
                if ((in(TrChar(-1),L" \"#")) && (TrChar(+1) == 'o') && (TrChar(+2) == 'n') &&
                        (TrChar(+3) == 'i') && (in(TrChar(+4),L" \".,!?:"))) {
                    Takel = true;
                }
                //Iot_ etc. vs Iota
                if ((in(TrChar(-1),L" \"#")) && (TrChar(+1) == 'o') &&
                        (TrChar(+2) == 't') && (TrChar(+3) != 'a')) {
                    Takel = true;
                }
                //Iow_ etc. vs Iowa
                if ((in(TrChar(-1),L" \"#")) && (TrChar(+1) == 'o') &&
                        (TrChar(+2) == 'w') && (TrChar(+3) != 'a')) {
                    Takel = true;
                }
                //_AII_ etc.
                if ((in(TrChar(-2),L" \"-#")) &&
                        (in(TrChar(-1),L"AEU")) && (TrChar(+1) == 'I') &&
                        (in(TrChar(+2),L" '-abcdefghijklmnopqrstuvwxyz"))) {
                    Takel = true;
                    St1[W1 + 1] = 'l';
                }
                //Eng only, no others affected: _If_, _If-, Iffy, Ifle, Ifor
                if ((in(TrChar(-1),L" \"#")) && (TrChar(+1) == 'f') &&
                        (in(TrChar(+2),L" -flo"))) {
                    Takel = false;
                }
                //"hey C.W. Ioad the weapons" and "hey... Ioad the weapons"
                //      432101                        432101
                if ((TrChar(-4) == '.') && (TrChar(-2) == '.') && (TrChar(-1) == ' ') &&
                        (in(TrChar(+1),L"aeiouy"))) {
                    Takel = true;
                }
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
                if (Takel) {
                    St1[W1] = 'l';
                    //'-' for 'Uh, l-ladies and gentleman...' / 'just as you l-left it'
                    if ((W1 > 4) && (St1[W1 - 2] == 'I') && (St1[W1 - 1] == '-'))
                        if (St1[W1 - 3] == ' ') {
                            St1[W1 - 2] = 'l';
                        } else if ((St1[W1 - 4] == 'I') && (St1[W1 - 3] == '-')) {
                            St1[W1 - 2] = 'l';
                            St1[W1 - 4] = 'l';
                        }
                }
            }
        }
#undef TrChar
        S1=St1.copy(1 + 2, St1.size() - 4);
    }

    if (cfg.fix&fixPunctuation) { //punctuation
        static const wchar_t *punctinations[]= {
            L"?.",L"?",
            L"?,",L"?",
            L" )",L")",
            L"( ",L"(",
            L" ]",L"]",
            L"[ ",L"[",
            L"!.",L"!",
            L"!,",L"!",
            L". . .",L"...",
            L". ..",L"...",
            L".. .",L"...",
            L"--",L"...",
            L"....",L"...",
            L" :",L":",
            L" ;",L";",
            L" %",L"%",
            L" !",L"!",
            L" ?",L"?",
            L" .",L".",
            L" ,",L",",
            L".:",L":",
            L":.",L":",
            L"eVe",L"eve",
            L"    }",L"  }",
            L"  ",L" ",
            NULL,NULL
        };
        int i=1;
        //"--" at the beginning of dialogs (Spy Kids 2, Stolen Summer, Sweet Home Alabama)
        if (S1[i]=='-' && S1[i+1]=='-' && S1[i+2]!='-') {
            S1.erase(i,1);
        }

        //dict
        for (i=0; punctinations[2*i]; i++) {
            S1=stringreplace(S1,punctinations[2*i],punctinations[2*i+1],rfReplaceAll);
        }

        //".." to "..."
        W1=S1.find(L"..");
        if (W1>0)
            while (S1.size()>W1) {
                if (((W1== 1) || (S1[W1 - 1] != '.')) &&
                        (S1[W1] == '.') && (S1[W1 + 1] == '.') &&
                        ((S1.size() == W1 + 1) || (S1[W1 + 2] != '.'))
                   ) {
                    S1.insert(W1,L".");
                }
                W1++;
            }

        //"..."
        do {
            W1 = S1.find(L"...");
            if (W1 > 0)
                //Example "It is ..." -> "It is..."
                if ((W1 == S1.size() - 2) && (S1[W1 - 1] == ' ')) {
                    S1 = S1.copy(1, S1.size() - 4) + L"<><>";
                } else
                    //Example "... it is" -> "...it is"
                    if ((W1 == 1) && (S1[W1 + 3] == ' ')) {
                        S1 = L"<><>" + S1.copy(5, S1.size() - 3);
                    } else
                        //Example "It...is" -> "It... is"
                        if ((W1 > 1) && (W1 < S1.size() - 2) &&
                                !(S1[W1 - 2]=='?' || S1[W1-2]=='!')) {
                            S1 = S1.copy(1, W1 - 1) + L"<><> " + S1.copy(W1 + 3, S1.size() - (W1 + 2));
                        } else
                            //Example "Who?... there..." -> "Who? ...there..."
                            if ((W1 > 1) && (W1 < S1.size() - 2) &&
                                    (S1[W1 - 1] =='?' || S1[W1-1]=='!')) {
                                S1 = S1.copy( 1, W1 - 1) + L" <><>" + S1.copy( W1 + 4, S1.size() - (W1 + 3));
                            } else {
                                S1 = S1.copy( 1, W1 - 1) + L"<><>" + S1.copy(W1 + 3, S1.size() - (W1 + 2));
                            }
        } while (W1!=0);
        S1 = stringreplace(S1, L"?<><> ", L"? <><>", rfReplaceAll);
        S1 = stringreplace(S1, L"<><> ?", L"<><>?", rfReplaceAll);
        S1 = stringreplace(S1, L"<><> !", L"<><>!", rfReplaceAll);
        S1 = stringreplace(S1, L"!<><> ", L"! <><>", rfReplaceAll);
        S1 = stringreplace(S1, L"\"<><> ", L"\"<><>", rfReplaceAll);
        S1 = stringreplace(S1, L"<><> \"", L"<><>\"", rfReplaceAll);
        S1 = stringreplace(S1, L"- <><> ", L"<><>", rfReplaceAll);
        S1 = stringreplace(S1, L"- <><>", L"<><>", rfReplaceAll);
        S1 = stringreplace(S1, L"-<><> ", L"<><>", rfReplaceAll);
        S1 = stringreplace(S1, L"-<><>", L"<><>", rfReplaceAll);
        S1 = stringreplace(S1, L" <><> ", L"<><> ", rfReplaceAll);
        S1 = stringreplace(S1, L",<><>", L"<><>", rfReplaceAll);

        //|"_| -> |"| or |_"| -> |"|  -  sometimes in safe cases
        int QoutCnt = 0;
        for (i=1; i<=S1.size(); i++)
            if (S1[i]=='"') {
                QoutCnt++;
            }
        if (QoutCnt > 0)
            if (!odd(QoutCnt)) {
                for (i=1; i<=S1.size(); i++)
                    if (S1[i] == '"') {
                        if (odd(QoutCnt)) {
                            if ((i > 1) && (S1[i - 1] == ' ')) {
                                S1.erase(i - 1, 1);
                            }
                        } else if ((i < S1.size()) && (S1[i + 1] == ' ')) {
                            S1.erase(i + 1, 1);
                        }
                        QoutCnt--;
                    }
            } else {
                //aftercheck 1: |"_| at position 1 (if only 1x " appear as beginning)
                if (S1.find(L"\" ") == 1) {
                    S1.erase(2, 1);
                }
                //aftercheck 2: |_"| at last position (if only 1x " appear as end)
                if (S1.find(L" \"") == S1.size() - 1) {
                    S1.erase(S1.size() - 1, 1);
                }
            }

        //"-"
        do {
            W1 = S1.find('-');
            if (W1 > 0) {
                //Example "-It is" -> "- It is"
                if ((W1 == 1) && (S1[W1 + 1] != ' ')) {
                    S1 = L"[][] " + S1.copy(2, S1.size() - 1);
                } else
                    //Example "Hi. -It is." -> "Hi. - It is." (works for styles: "<i>-It is")
                    if (((in(S1[W1 - 2],L".!?>")) || (S1[W1 - 1] == '>')) && (S1[W1 + 1] != ' ')) {
                        S1 = S1.copy(1, W1 - 1) + L"[][] " + S1.copy(W1 + 1, S1.size() - (W1));
                    } else {
                        S1 = S1.copy(1, W1 - 1) + L"[][]" + S1.copy(W1 + 1, S1.size() - (W1));
                    }
            }
        } while (W1 != 0);

        static const wchar_t *chars1[]= {L".",L",",L":",L";",L"%",L"$",L"!",L"?"};
        //Spaces after "," "." ":" ";" "$" "%"
        if ((S1.find(L"www.") == 0) && (S1.find(L".com ") == 0) &&
                (S1.find('@') == 0) && (S1.find(L"tp://") == 0))
            for (i=0; i<6; i++) {
                do {
                    W1 = S1.find(chars1[i]);
                    if (W1 > 0)
                        if ((W1 != S1.size() && !(S1[W1 + 1]==' ' || S1[W1 + 1]=='"' || S1[W1 + 1]=='\'' || S1[W1 + 1]=='?' || S1[W1 + 1]=='!' || S1[W1 + 1]==',' || S1[W1 + 1]==']' || S1[W1 + 1]=='}' || S1[W1 + 1]==')' || S1[W1 + 1]=='<'))
                                && ((S1[W1 - 1] < '0') || (S1[W1 - 1] > '9')) && ((S1[W1 + 1] < '0') || (S1[W1 + 1] > '9'))
                                && (S1[W1 + 2] != '.')) {
                            S1 = S1.copy(1, W1 - 1) + L"()() " + S1.copy(W1 + 1, S1.size() - (W1));
                        } else {
                            S1 = S1.copy(1, W1 - 1) + L"()()" + S1.copy(W1 + 1, S1.size() - W1);
                        }
                } while (W1 != 0);
                S1 = stringreplace(S1, L"()()", chars1[i], rfReplaceAll);
            }
        //Spaces after "!" "?"
        for (i=6; i<=7; i++) {
            do {
                W1 = S1.find(chars1[i]);
                if (W1 > 0)
                    if ((W1 != S1.size()) && (S1[W1 + 1] != ' ') && !(S1[W1 + 1]=='!' || S1[W1 + 1]=='?' || S1[W1 + 1]=='"' || S1[W1 + 1]==']' || S1[W1 + 1]=='}' || S1[W1 + 1]==')' || S1[W1 + 1]=='<')) { //'<' styles)
                        S1 = S1.copy(1, W1 - 1) + L"()() " + S1.copy( W1 + 1, S1.size() - (W1));
                    } else {
                        S1 = S1.copy(1, W1 - 1) + L"()()" + S1.copy(W1 + 1, S1.size() - (W1));
                    }
            } while (W1 != 0);
            S1 = stringreplace(S1, L"()()", chars1[i], rfReplaceAll);
        }
        S1 = stringreplace(S1, L"[][]", L"-", rfReplaceAll);
        S1 = stringreplace(S1, L"<><>", L"...", rfReplaceAll);
    }

    if (cfg.fix&fixOrtography)
        for (strings::const_iterator s=odict.begin(); s!=odict.end();) {
            const ffstring &oldstr=*s;
            s++;
            const ffstring &newstr=*s;
            s++;
            S1=stringreplace(S1,oldstr,newstr,rfReplaceAll);
        }

    if (cfg.fix&fixCapital) {
        if (cfg.fix&fixCapital2) {
            S1.ConvertToLowerCase();
        }

        //Change "..." to "<><>"
        S1 = stringreplace(S1, L"...", L"<><>", rfReplaceAll);

        static const wchar_t *chars2[]= {L"-",L".",L"!",L"?",L":"};

        //Capital letters after ". " "? " "! " "- "
        for (unsigned int i=0; i<countof(chars2); i++) {
            do {
                W1 = S1.find(chars2[i]+ffstring(L" "));
                if (W1 > 1) //take Pos from 2 and higher ("- " elsewhere)
                    if (W1 != S1.size())
                        if (((i == 1) /*'.'*/ && (S1[W1 - 1]>='0' && S1[W1-1]<='9')) || //prevents: 28. srpna -> 28. Srpna
                                ((i == 0) /*'-'*/ && !(S1[W1 - 2]=='.' || S1[W1 - 2]=='!' || S1[W1 - 2]=='?'))) { //UpperCase only after .!?
                            S1 = S1.copy(1, W1 - 1) + L"()()" + S1.copy(W1 + 1, S1.size() - W1);
                        } else {
                            wchar_t up[2];
                            up[0]=(wchar_t)toupper(S1[W1 + 2]);
                            up[1]='\0';
                            S1 = S1.copy(1, W1 - 1) + L"()() " + passtring<wchar_t>(up) + S1.copy(W1 + 3, S1.size() - (W1 + 2));
                        }
                    else {
                        S1 = S1.copy(1, W1 - 1) + L"()()" + S1.copy(W1 + 1, S1.size() - W1);
                    }
            } while (W1 > 1);
            S1 = stringreplace(S1, L"()()", chars2[i], rfReplaceAll);
        }

        //Change "<><>" back to "..."
        S1 = stringreplace(S1, L"<><>", L"...", rfReplaceAll);

        //change the 1st letter case according to previous subtitle line
        if (EndOfPrevSentence && (S1[1] != '.')) { //...bla
            int J = 1;
            //while ((S1[J] == '<') && (S1[J + 1]=='b' || S1[J + 1]=='i' || S1[J + 1]=='u')) && //styles
            //       (S1[J + 2] == '>'))
            // j+=, 3);
            if (S1[J] != '.') //<i>...bla
                while (J <= S1.size()) {
                    //Alex: (battle cries) --> (Battle cries)
                    //It's not expected behavior. It'll be much better to stay words in "()" intact.
                    if (!(S1[J]=='[' || S1[J]=='"' || S1[J]=='\'' || S1[J]=='#' || S1[J]=='-' || S1[J]==' ')) {
                        S1[J] = (wchar_t)toupper(S1[J]);
                        break;
                    }
                    J++;
                }
        }
        //prepare for next line
        passtring<wchar_t> Sstrip = S1; //styles
        unsigned int J = Sstrip.size();
        if (J==0) { //in case of empty string (only styles)
            EndOfPrevSentence = false;
        } else {
            while ((Sstrip[J]=='\'' || Sstrip[J]=='"' || Sstrip[J]==' ') && (J > 1)) {
                J--;
            }
            EndOfPrevSentence = Sstrip[J]=='.' || Sstrip[J]=='!' || Sstrip[J]=='?' || Sstrip[J]==':' || Sstrip[J]==']' || Sstrip[J]==')';
            if ((Sstrip[J] == '.') && (Sstrip[J - 1] == '.') && (Sstrip[J - 2] == '.')) {
                EndOfPrevSentence = false;
            }
        }
    }

    if (cfg.fix&fixNumbers) //1 993 --> 1993 etc.
        for (int I=1; I<=S1.size()-2; I++)
            if ((S1.size() >= I + 2) /*Delete!*/ &&
                    (S1[I]=='0' || S1[I]=='1' || S1[I]=='2' || S1[I]=='3' || S1[I]=='4' || S1[I]=='5' || S1[I]=='6' || S1[I]=='7' || S1[I]=='8' || S1[I]=='9' || S1[I]=='/') &&
                    (S1[I + 1] == ' '))
                if ((S1[I + 2]=='0' || S1[I + 2]=='1' || S1[I + 2]=='2' || S1[I + 2]=='3' || S1[I + 2]=='4' || S1[I + 2]=='5' || S1[I + 2]=='6' || S1[I + 2]=='7' || S1[I + 2]=='8' || S1[I + 2]=='9' || S1[I + 2]==',' || S1[I + 2]=='.' || S1[I + 2]==':' || S1[I + 2]=='/') ||
                        //5 -1-5
                        ((S1[I + 2] == '-') &&
                         (S1.size() >= I + 3) /*Delete!*/ &&
                         (S1[I + 3]=='0' || S1[I + 3]=='1' || S1[I + 3]=='2' || S1[I + 3]=='3' || S1[I + 3]=='4' || S1[I + 3]=='5' || S1[I + 3]=='6' || S1[I + 3]=='7' || S1[I + 3]=='8' || S1[I + 3]=='9'))) {
                    S1.erase(I + 1, 1);
                }

    if (cfg.fix&fixHearingImpaired)
        for (int I=1; I<=S1.size(); I++) {
            if (inHearing) {
                W1=S1.find(']',I);
                if (W1>0) {
                    S1.erase(std::max(1,I-1),std::max(W1 - std::max(1,I-1) + 1, 0));
                    if (S1 == passtring<wchar_t>(L"- ") || S1 == passtring<wchar_t>(L"-")) {
                        S1 = L"";
                    }
                } else {
                    S1.erase(std::max(1,I-1),S1.size());
                    break;
                }
                inHearing=false;
                I=W1+1;
            } else if (S1[I]=='[') {
                inHearing=true;
            }
        }
    bool useFixed=strcmp(text.c_str(),S1.c_str())!=0;
    if (useFixed) {
        fixed=S1.c_str();
    }
    return useFixed;
}

//============================ TsubtitleFormat =============================
ffstring TsubtitleFormat::getAttribute(const wchar_t *start,const wchar_t *end,const wchar_t *attrname)
{
    if (const wchar_t *attr=strnistr(start,end-start+1,attrname))
        if (const wchar_t *eq=strnchr(attr,end-attr+1,'=')) {
            eq++;
            bool in=false;
            for (int i=0; i<end-eq+1; i++)
                if (eq[i]=='"') {
                    in=!in;
                } else if (!in && (eq[i]==' ' || eq[i]=='>' || eq[i]=='\0')) {
                    return stringreplace(ffstring(eq,i).Trim(),L"\"",L"",rfReplaceAll);
                }
        }
    return ffstring();
}

void TsubtitleFormat::processHTMLTags(Twords &words, const wchar_t* &l, const wchar_t* &l1, const wchar_t* &l2)
{
    if (_strnicmp(l2,L"<i>",3)==0) {
        words.add(l,l1,l2,props,3);
        props.italic=true;
    } else if (_strnicmp(l2,L"</i>",4)==0) {
        words.add(l,l1,l2,props,4);
        props.italic=false;
    } else if (_strnicmp(l2,L"<u>",3)==0) {
        words.add(l,l1,l2,props,3);
        props.underline=true;
    } else if (_strnicmp(l2,L"</u>",4)==0) {
        words.add(l,l1,l2,props,4);
        props.underline=false;
    } else if (_strnicmp(l2,L"<b>",3)==0) {
        words.add(l,l1,l2,props,3);
        props.bold=true;
    } else if (_strnicmp(l2,L"</b>",4)==0) {
        words.add(l,l1,l2,props,4);
        props.bold=false;
    } else if (_strnicmp(l2,L"<font ",6)==0) {
        const wchar_t *end=strchr(l2,'>');
        if (end) {
            const wchar_t *start=l2+6;
            ffstring face=getAttribute(start,end,L"face");
            ffstring color=getAttribute(start,end,L"color").ConvertToLowerCase();
            ffstring size=getAttribute(start,end,L"size");
            words.add(l,l1,l2,props,end-l2+1);
            if (!face.empty()) {
                ff_strncpy(props.fontname, (const char_t *)text<char_t>(face.c_str()),countof(props.fontname));
            }
            if (!color.empty() && ((color[0]=='#' && swscanf(color.c_str()+1,L"%x",&props.color)) || (htmlcolors->getColor(color,&props.color,0xffffff),true))) {
                std::swap(((uint8_t*)&props.color)[0],((uint8_t*)&props.color)[2]);
                props.isColor=true;
            }
            if (!size.empty()) {
                wchar_t *ll;
                int s=strtol(size.c_str(),&ll,10);
                if (*ll=='\0') {
                    props.size=s;
                }
            }
        }
    } else if (_strnicmp(l2,L"</font>",7)==0) {
        words.add(l,l1,l2,props,7);
        props.isColor=false;
        props.size=0;
        props.fontname[0]='\0';
    }
    // Hacks for badly written HTML tags go here. Comment everything to only parse good HTML tags
    else if (_strnicmp(l2,L"< i>",4)==0) {
        words.add(l,l1,l2,props,4);
        props.italic=true;
    } else if (_strnicmp(l2,L"< /i>",5)==0) {
        words.add(l,l1,l2,props,5);
        props.italic=false;
    } else if (_strnicmp(l2,L"< u>",4)==0) {
        words.add(l,l1,l2,props,4);
        props.underline=true;
    } else if (_strnicmp(l2,L"< /u>",5)==0) {
        words.add(l,l1,l2,props,5);
        props.underline=false;
    } else if (_strnicmp(l2,L"< b>",4)==0) {
        words.add(l,l1,l2,props,4);
        props.bold=true;
    } else if (_strnicmp(l2,L"< /b>",5)==0) {
        words.add(l,l1,l2,props,5);
        props.bold=false;
    }
    // End of hacks
    else {
        l2++;
    }
}

const TSubtitleProps& TsubtitleFormat::processHTML(Twords &words, const TsubtitleLine &line)
{
    if (line.empty()) {
        return lineProps;
    }
    const wchar_t *l=line[0];
    const wchar_t *l1=l,*l2=l;
    while (*l2) {
        processHTMLTags(words, l, l1, l2);
    }

    words.add(l,l1,l2,props,0);
    return lineProps;
}

int TsubtitleFormat::Tssa::parse_parentheses(TparenthesesContents &contents, ffstring arg)
{
    // (a1,a2, ... ,an) is expected.
    size_t first_paren = arg.find('(');
    if (first_paren == ffstring::npos) {
        return -1;
    }
    size_t second_paren = arg.find(')', first_paren + 1);
    if (second_paren == ffstring::npos) {
        return -1;
    }
    arg.erase(0,first_paren+1);
    arg.erase(second_paren - first_paren - 1);
    strings input_strings;
    strtok(arg.c_str(),L",",input_strings);
    foreach (ffstring &s, input_strings)
    contents.push_back(TparenthesesContent(s));
    return (int)(second_paren + 1);
}

int TsubtitleFormat::Tssa::TstoreParams::writeProps(const TparenthesesContents &contents, TSubtitleProps *props)
{
    int count = 0;
    iterator store_i = begin();
    TparenthesesContents::const_iterator contents_i = contents.begin();
    for ( ; store_i != end() ; store_i++) {
        int64_t val;
        double doubleval;
        if (store_i->offset && store_i->isInteger) {
            if ( contents_i != contents.end()
                    && contents_i->ok) {
                if (contents_i->doubleval < store_i->min) {
                    val = (int64_t)store_i->min;
                } else if (contents_i->doubleval > store_i->max) {
                    val = (int64_t)store_i->max;
                } else {
                    val = (int64_t)contents_i->doubleval;
                    count++;
                }
            } else {
                val = (int64_t)store_i->default_value;
            }
            void *dst = (uint8_t *)props + store_i->offset;
            memcpy(dst, &val, store_i->size);
        }
        if (store_i->offset && !store_i->isInteger) {
            if ( contents_i != contents.end()
                    && contents_i->ok) {
                if (contents_i->doubleval < store_i->min) {
                    doubleval = store_i->min;
                } else if (contents_i->doubleval > store_i->max) {
                    doubleval = store_i->max;
                } else {
                    doubleval = contents_i->doubleval;
                    count++;
                }
            } else {
                doubleval = store_i->default_value;
            }
            void *dst = (uint8_t *)props + store_i->offset;
            memcpy(dst, &doubleval, store_i->size);
        }
        if (contents_i != contents.end()) {
            contents_i++;
        }
    }
    return count;
}

void TsubtitleFormat::Tssa::fontName(ffstring &arg)
{
    if (arg.size()) {
        memset(props.fontname,0,sizeof(props.fontname));
        text<char_t>(arg.c_str(), (int)arg.size(), (char_t*)props.fontname, countof(props.fontname));
    } else {
        ff_strncpy(props.fontname, defprops.fontname, countof(props.fontname));
    }
}

template<int TSubtitleProps::*offset,int min,int max> void TsubtitleFormat::Tssa::intProp(ffstring &arg)
{
    if (props.transform.isTransform) {
        // Don't do anything until transform is complete
    } else {
        int enc;
        if (arg2int(arg,min,max,enc)) {
            props.*offset=enc;
        } else {
            props.*offset=defprops.*offset;
        }
    }
}

template<int min,int max> void TsubtitleFormat::Tssa::intPropAn(ffstring &arg)
{
    int enc;
    if (lineProps.isAlignment == false && arg2int(arg,min,max,enc)) {
        lineProps.alignment = TSubtitleProps::alignASS2SSA(enc);
        lineProps.isAlignment = true;
    }
}

template<int min,int max> void TsubtitleFormat::Tssa::intPropA(ffstring &arg)
{
    int enc;
    if (lineProps.isAlignment == false && arg2int(arg,min,max,enc)) {
        lineProps.alignment = enc;
        lineProps.isAlignment = true;
    }
}

template<int min,int max> void TsubtitleFormat::Tssa::intPropQ(ffstring &arg)
{
    int enc;
    if (arg2int(arg,min,max,enc))
        lineProps.wrapStyle = enc;
}

template<double TSubtitleProps::*offset,int min,int max> void TsubtitleFormat::Tssa::doubleProp(ffstring &arg)
{
    if (props.transform.isTransform) {
        // Don't do anything until transform is complete
    } else {
        const wchar_t* buf = arg.c_str();
        wchar_t *bufend;
        double enc=strtod(buf,&bufend);
        if (buf!=bufend && *bufend=='\0' && isIn(enc,(double)min,(double)max)) {
            props.*offset=enc;
        } else {
            props.*offset=defprops.*offset;
        }
    }
}

template<double TSubtitleProps::*offset,int min,int max> void TsubtitleFormat::Tssa::doublePropDiv100(ffstring &arg)
{
    if (props.transform.isTransform) {
        // Don't do anything until transform is complete
    } else {
        const wchar_t* buf = arg.c_str();
        wchar_t *bufend;
        double enc=strtod(buf,&bufend);
        if (buf!=bufend && *bufend=='\0' && isIn(enc,(double)min,(double)max)) {
            props.*offset=enc/100.0;
        } else {
            props.*offset=defprops.*offset;
        }
    }
}

void TsubtitleFormat::Tssa::clip(ffstring &arg)
{
    // (x1,y1,x2,y2,[t1[,t2]]) is expected.
    TstoreParams store;
    store.push_back(TstoreParam(offsetof(TSubtitleProps, clip.left),  0, INT_MAX,  defprops.clip.left,  sizeof(lineProps.clip.left),  true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, clip.top),  0, INT_MAX,  defprops.clip.top,  sizeof(lineProps.clip.top),  true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, clip.right), 0, INT_MAX,  defprops.clip.right, sizeof(lineProps.clip.right), true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, clip.bottom), 0, INT_MAX,  defprops.clip.bottom, sizeof(lineProps.clip.bottom), true));

    TparenthesesContents contents;
    parse_parentheses(contents,arg);
    if (store.writeProps(contents, &lineProps) >= 4) {
        lineProps.isClip=true;
    }
}

void TsubtitleFormat::Tssa::pos(ffstring &arg)
{
    if (lineProps.isMove)
        return;
    // (x1,y1) is expected.
    TstoreParams store;
    store.push_back(TstoreParam(offsetof(TSubtitleProps, pos.x), 0,INT_MAX,defprops.pos.x,sizeof(lineProps.pos.x),true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, pos.y), 0,INT_MAX,defprops.pos.y,sizeof(lineProps.pos.y),true));

    TparenthesesContents contents;
    parse_parentheses(contents,arg);
    if (store.writeProps(contents, &lineProps) == 2) {
		lineProps.pos2 = lineProps.pos;
        lineProps.isMove=true;
    }
}

void TsubtitleFormat::Tssa::move(ffstring &arg)
{
    if (lineProps.isMove)
        return;
    // (x1,y1,x2,y2,[t1[,t2]]) is expected.
    TstoreParams store;
    store.push_back(TstoreParam(offsetof(TSubtitleProps, pos.x),  0, INT_MAX,  defprops.pos.x,  sizeof(lineProps.pos.x),  true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, pos.y),  0, INT_MAX,  defprops.pos.y,  sizeof(lineProps.pos.y),  true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, pos2.x), 0, INT_MAX,  defprops.pos2.x, sizeof(lineProps.pos2.x), true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, pos2.y), 0, INT_MAX,  defprops.pos2.y, sizeof(lineProps.pos2.y), true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, moveT1), 0, UINT_MAX, 0,               sizeof(lineProps.moveT1), true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, moveT2), 0, UINT_MAX, 0,               sizeof(lineProps.moveT2), true));

    TparenthesesContents contents;
    parse_parentheses(contents,arg);
    if (store.writeProps(contents, &lineProps) >= 4) {
        lineProps.isMove=true;
    }
}

void TsubtitleFormat::Tssa::org(ffstring &arg)
{
    if (lineProps.isOrg)
        return;
    // (x1,y1) is expected.
    TstoreParams store;
    store.push_back(TstoreParam(offsetof(TSubtitleProps, org.x), 0,INT_MAX,defprops.org.x,sizeof(lineProps.org.x),true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, org.y), 0,INT_MAX,defprops.org.x,sizeof(lineProps.org.y),true));

    TparenthesesContents contents;
    parse_parentheses(contents,arg);
    if (store.writeProps(contents, &lineProps) == 2) {
        lineProps.isOrg=true;
    }
}

void TsubtitleFormat::Tssa::transform(ffstring &arg)
{
    // ([t1,t2,][accel,]<style modifiers>) is expected.
    TparenthesesContents contents;
    parse_parentheses(contents,arg);
    TstoreParams store;
    if (contents.size() != 2) {
        store.push_back(TstoreParam(offsetof(TSubtitleProps, transformT1),     0, UINT_MAX, 0, sizeof(props.transformT1),     true));
        store.push_back(TstoreParam(offsetof(TSubtitleProps, transformT2),     0, UINT_MAX, 0, sizeof(props.transformT2),     true));
        store.push_back(TstoreParam(offsetof(TSubtitleProps, transform.accel), 0, DBL_MAX,  1, sizeof(props.transform.accel), false));
    } else {
        props.transformT1=props.transformT2=0;
        store.push_back(TstoreParam(offsetof(TSubtitleProps, transform.accel), 0, DBL_MAX,  1, sizeof(props.transform.accel), false));
    }

    store.writeProps(contents, &props);
    if (contents.size() > 0) {
        props.transform.isTransform=true;
        const wchar_t *s=L"{", *e=L"}", *temp=L"";
        wchar_t tokensT[500] = {0};
        unsigned int i;
        for (i=0; i<contents.size(); i++) {
            if (!contents[i].ok) {
                temp=contents[i].str.c_str();
                wcscat(tokensT, s);
                wcscat(tokensT, (wchar_t *)temp);
                wcscat(tokensT, e);
            }
        }
        const wchar_t *tokens=tokensT, *tokens1=tokens, *tokens2=tokens, *end=strchr(tokens+1,'}');
        Tssa::processTokens(tokens,tokens1,tokens2,end);
    }
}

void TsubtitleFormat::Tssa::karaoke_fixProperties()
{
    /* SRT subtitles do not define (most of the time) any secondary color
        So we have to put the secondary color into the primary color and vice versa for the karaoke word
        that will be added later */
    if ((sfmt==Tsubreader::SUB_SUBVIEWER || sfmt==Tsubreader::SUB_SUBVIEWER2)
            && props.karaokeMode != TSubtitleProps::KARAOKE_NONE && props.SecondaryColour == 0xffffff) {
        props.SecondaryColour = DEFAULT_SECONDARY_COLOR;
        props.isColor=true;
    }
}

void TsubtitleFormat::Tssa::karaoke_kf(ffstring &arg)
{
    intProp<&TSubtitleProps::tmpFadT1, 0, INT_MAX>(arg);
    REFERENCE_TIME t = (REFERENCE_TIME)props.tmpFadT1 * 100000;
    props.karaokeDuration = t;
    props.karaokeStart = lineProps.karaokeStart;
    lineProps.karaokeStart += t;
    props.karaokeFillStart = props.karaokeStart;
    props.karaokeFillEnd = props.karaokeStart + t;

    props.karaokeMode = TSubtitleProps::KARAOKE_kf;
    props.karaokeNewWord = true;
    karaoke_fixProperties();
}
void TsubtitleFormat::Tssa::karaoke_ko(ffstring &arg)
{
    intProp<&TSubtitleProps::tmpFadT1, 0, INT_MAX>(arg);
    REFERENCE_TIME t = (REFERENCE_TIME)props.tmpFadT1 * 100000;
    props.karaokeDuration = t;
    props.karaokeStart = lineProps.karaokeStart;
    lineProps.karaokeStart += t;
    props.karaokeFillStart = props.karaokeFillEnd = props.karaokeStart + t;

    props.karaokeMode = TSubtitleProps::KARAOKE_ko;
    props.karaokeNewWord = true;
    karaoke_fixProperties();
}
void TsubtitleFormat::Tssa::karaoke_k(ffstring &arg)
{
    intProp<&TSubtitleProps::tmpFadT1, 0, INT_MAX>(arg);
    REFERENCE_TIME t = (REFERENCE_TIME)props.tmpFadT1 * 100000;
    props.karaokeDuration = t;
    props.karaokeStart = lineProps.karaokeStart;
    lineProps.karaokeStart += t;
    props.karaokeFillStart = props.karaokeFillEnd = props.karaokeStart + t;

    props.karaokeMode = TSubtitleProps::KARAOKE_k;
    props.karaokeNewWord = true;
    karaoke_fixProperties();
}

void TsubtitleFormat::Tssa::fad(ffstring &arg)
{
    if (lineProps.isFad)
        return;

    TstoreParams store;
    store.push_back(TstoreParam(offsetof(TSubtitleProps, fadeT3), 0,LLONG_MAX/10000,(double)defprops.fadeT3,sizeof(lineProps.fadeT3),true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, fadeT4), 0,LLONG_MAX/10000,(double)defprops.fadeT4,sizeof(lineProps.fadeT4),true));

    TparenthesesContents contents;
    parse_parentheses(contents,arg);
    if (store.writeProps(contents, &lineProps) == 2) {
        lineProps.isFad  = true;
        lineProps.fadeA1 = 0;
        lineProps.fadeA2 = 255;
        lineProps.fadeA3 = 0;
        lineProps.fadeT1 = lineProps.tStart;
        lineProps.fadeT2 = lineProps.fadeT1 + (lineProps.fadeT3 * 10000);
        lineProps.fadeT3 = lineProps.tStop - (lineProps.fadeT4 * 10000);
        lineProps.fadeT4 = lineProps.tStop;
    }
}

void TsubtitleFormat::Tssa::fade(ffstring &arg)
{
    // \fade(<a1>, <a2>, <a3>, <t1>, <t2>, <t3>, <t4>)

    if (lineProps.isFad)
        return;

    TparenthesesContents contents;
    parse_parentheses(contents,arg);

    if (contents.size() == 2) {
        // Workaround for \fade(<t1>, <t2>), found in some scripts,
        // meaning the same as \fad(<t1>, <t2>)
        Tssa::fad(arg);
        return;
    }

    TstoreParams store;
    store.push_back(TstoreParam(offsetof(TSubtitleProps, fadeA1), 0, 255,             0,    sizeof(lineProps.fadeA1),true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, fadeA2), 0, 255,             255,  sizeof(lineProps.fadeA2),true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, fadeA3), 0, 255,             0,    sizeof(lineProps.fadeA3),true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, fadeT1), 0, LLONG_MAX/10000, 0,    sizeof(lineProps.fadeT1),true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, fadeT2), 0, LLONG_MAX/10000, 1000, sizeof(lineProps.fadeT2),true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, fadeT3), 0, INT_MAX, 2000, sizeof(lineProps.fadeT3),true));
    store.push_back(TstoreParam(offsetof(TSubtitleProps, fadeT4), 0, INT_MAX, 3000, sizeof(lineProps.fadeT4),true));

    if (store.writeProps(contents, &lineProps) == 7) {
        lineProps.fadeA1 = 256 - lineProps.fadeA1;
        lineProps.fadeA2 = 256 - lineProps.fadeA2;
        lineProps.fadeA3 = 256 - lineProps.fadeA3;

        lineProps.fadeT1 = lineProps.tStart + lineProps.fadeT1 * 10000;
        lineProps.fadeT2 = lineProps.tStart + lineProps.fadeT2 * 10000;
        lineProps.fadeT3 = lineProps.tStart + lineProps.fadeT3 * 10000;
        lineProps.fadeT4 = lineProps.tStart + lineProps.fadeT4 * 10000;
        lineProps.isFad = true;
    }
}

template<int TSubtitleProps::*offset1,int TSubtitleProps::*offset2,int min,int max> bool TsubtitleFormat::Tssa::intProp2(ffstring &arg)
{
    // (x,y) is expected.
    TstoreParams store;
    store.push_back(TstoreParam((int64_t TSubtitleProps::*)offset1,min,max,defprops.*offset1,sizeof(int),true));
    store.push_back(TstoreParam((int64_t TSubtitleProps::*)offset2,min,max,defprops.*offset2,sizeof(int),true));

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
    if (!arg.empty()) {
        if (arg.compare(0,2,ffstring(_l("&H")))==0)
            arg.erase(0,2);
        if (arg.compare(0,1,ffstring(_l("H")))==0)  // "H&" fix typo for a certain script. For compatibility with vsfilter.
            arg.erase(0,1);
        if (arg.compare(0,1,ffstring(_l("&")))==0)
            arg.erase(0,1);
        wchar_t *endbuf;
        intval=strtol(arg.c_str(),&endbuf,16);
        return (*endbuf=='&' || *endbuf==NULL);
    } else {
        return false;
    }
}

template<COLORREF TSubtitleProps::*offset> void TsubtitleFormat::Tssa::color(ffstring &arg)
{
    if (props.transform.isTransform) {
        // Don't do anything until transform is complete
    } else {
        int c;
        if (color2int(arg,c)) {
            props.*offset=c;
            props.isColor=true;
        } else {
            props.*offset=defprops.*offset;
            props.isColor=defprops.isColor;
        }
    }
}

template<int TSubtitleProps::*offset> void TsubtitleFormat::Tssa::alpha(ffstring &arg)
{
    if (props.transform.isTransform) {
        // Don't do anything until transform is complete
    } else {
        int a;
        if (color2int(arg,a)) {
            a &= 0xff;
            props.*offset=256-a;
            props.isColor=true;
        } else {
            props.*offset=defprops.*offset;
            props.isColor=defprops.isColor;
        }
    }
}

void TsubtitleFormat::Tssa::alphaAll(ffstring &arg)
{
    int a;
    if (props.transform.isTransform && color2int(arg,a)) {
        props.transform.isAlpha=true;
        props.transform.alpha=a;
        props.transform.alphaT1= props.transformT1 ? props.tStart + (props.transformT1 * 10000) : props.tStart;
        props.transform.alphaT2= props.transformT2 ? props.tStart + (props.transformT2 * 10000) : props.tStop;
    } else {
        if (color2int(arg,a)) {
            a &= 0xff;
            props.colorA=256-a;
            props.SecondaryColourA=256-a;
            props.TertiaryColourA=256-a;
            props.OutlineColourA=256-a;
            props.ShadowColourA=256-a;
            props.isColor=true;
        } else {
            props.colorA=defprops.colorA;
            props.SecondaryColourA=defprops.SecondaryColourA;
            props.TertiaryColourA=defprops.TertiaryColourA;
            props.OutlineColourA=defprops.OutlineColourA;
            props.ShadowColourA=defprops.ShadowColourA;
            props.isColor=defprops.isColor;
        }
    }
}

template<bool TSubtitleProps::*offset> void TsubtitleFormat::Tssa::boolProp(ffstring &arg)
{
    if (arg.size() && arg[0]=='1') {
        props.*offset=true;
    } else if (arg.size() && arg[0]=='0') {
        props.*offset=false;
    } else {
        props.*offset=defprops.*offset;
    }
}

void TsubtitleFormat::Tssa::reset(ffstring &arg)
{
    if (arg.size()) {
        const TSubtitleProps* p = styles.getProps(arg);
        if (p) {
            int lineID = props.lineID;
            props = *p;
            props.lineID = lineID;
        }
    } else {
        props=defprops;
    }
}

bool TsubtitleFormat::Tssa::processTokenI(const wchar_t* &l2,const wchar_t *tok,TssaAction action,Tstr_cmp_func str_cmp_func)
{
    size_t toklen=strlen(tok);
    if (str_cmp_func(l2,tok,toklen)==0) {
        const wchar_t *end1=((strchr(l2+2,'\\') > strchr(l2+2,'(')) && (strchr(l2+2,'\\') < strchr(l2+2,')'))) ? strchr(l2+2,')')+1 : strchr(l2+2,'\\');
        const wchar_t *end2=strchr(l2,'}');
        const wchar_t *end=(end1 && end1<end2)?end1:end2;
        if (end) {
            const wchar_t *start=l2+toklen;
            ffstring arg(start,end - start);
            if (action) {
                (this->*action)(arg);
            }
            l2=(end1 && end1<end2)?end1:end2+1;
        }
        return true;
    } else {
        return false;
    }
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
            !processToken(l3,L"\\1a",&Tssa::template alpha<&TSubtitleProps::colorA>) &&
            !processToken(l3,L"\\2a",&Tssa::template alpha<&TSubtitleProps::SecondaryColourA>) &&
            !processToken(l3,L"\\3a",&Tssa::template alpha<&TSubtitleProps::OutlineColourA>) &&
            !processToken(l3,L"\\4a",&Tssa::template alpha<&TSubtitleProps::ShadowColourA>) &&
            !processToken(l3,L"\\1c",&Tssa::template color<&TSubtitleProps::color>) &&
            !processToken(l3,L"\\2c",&Tssa::template color<&TSubtitleProps::SecondaryColour>) &&
            !processToken(l3,L"\\3c",&Tssa::template color<&TSubtitleProps::OutlineColour>) &&
            !processToken(l3,L"\\4c",&Tssa::template color<&TSubtitleProps::ShadowColour>) &&
            !processToken(l3,L"\\alpha",&Tssa::alphaAll) &&
            !processToken(l3,L"\\an",&Tssa::template intPropAn<1,9>) &&
            !processToken(l3,L"\\a",&Tssa::template intPropA<1,11>) &&
            !processToken(l3,L"\\blur",&Tssa::template doubleProp<&TSubtitleProps::gauss,0,100>) &&
            !processToken(l3,L"\\bord",&Tssa::template doubleProp<&TSubtitleProps::outlineWidth,0,100>) &&
            !processToken(l3,L"\\be",&Tssa::template intProp<&TSubtitleProps::blur_be,0,1000>) &&
            !processToken(l3,L"\\b",&Tssa::template intProp<&TSubtitleProps::bold,0,1>) &&
            !processToken(l3,L"\\clip",&Tssa::clip) &&
            !processToken(l3,L"\\c",&Tssa::template color<&TSubtitleProps::color>) &&
            !processToken(l3,L"\\fn",&Tssa::fontName) &&
            !processToken(l3,L"\\fscx",&Tssa::template doublePropDiv100<&TSubtitleProps::scaleX,1,1000>) &&
            !processToken(l3,L"\\fscy",&Tssa::template doublePropDiv100<&TSubtitleProps::scaleY,1,1000>) &&
            !processToken(l3,L"\\fsp",&Tssa::template doubleProp<&TSubtitleProps::spacing,INT_MIN+1,INT_MAX>) &&
            !processToken(l3,L"\\fs",&Tssa::template doubleProp<&TSubtitleProps::size,1,1000>) &&
            !processToken(l3,L"\\frx",&Tssa::template doubleProp<&TSubtitleProps::angleX,0,10000>) &&
            !processToken(l3,L"\\fry",&Tssa::template doubleProp<&TSubtitleProps::angleY,0,10000>) &&
            !processToken(l3,L"\\frz",&Tssa::template doubleProp<&TSubtitleProps::angleZ,0,10000>) &&
            !processToken(l3,L"\\fr",&Tssa::template doubleProp<&TSubtitleProps::angleZ,0,10000>) &&
            !processToken(l3,L"\\fe",&Tssa::template intProp<&TSubtitleProps::encoding,0,255>) &&
            !processToken(l3,L"\\fade",&Tssa::fade) &&
            !processToken(l3,L"\\fad",&Tssa::fad) &&
            !processToken(l3,L"\\i",&Tssa::template boolProp<&TSubtitleProps::italic>) &&
            !processToken(l3,L"\\kf",&Tssa::karaoke_kf) &&
            !processToken(l3,L"\\ko",&Tssa::karaoke_ko) &&
            !processTokenC(l3,L"\\K",&Tssa::karaoke_kf) &&
            !processTokenC(l3,L"\\k",&Tssa::karaoke_k) &&
            !processToken(l3,L"\\move",&Tssa::move) &&
            !processToken(l3,L"\\org",&Tssa::org) &&
            !processToken(l3,L"\\pos",&Tssa::pos) &&
            !processToken(l3,L"\\p",&Tssa::template intProp<&TSubtitleProps::polygon,0,255>) &&
            !processToken(l3,L"\\q",&Tssa::template intPropQ<0,3>) &&
            !processToken(l3,L"\\r",&Tssa::reset) &&
            !processToken(l3,L"\\shad",&Tssa::template doubleProp<&TSubtitleProps::shadowDepth,0,30>) &&
            !processToken(l3,L"\\s",&Tssa::template boolProp<&TSubtitleProps::strikeout>) &&
            !processToken(l3,L"\\t",&Tssa::transform) &&
            !processToken(l3,L"\\u",&Tssa::template boolProp<&TSubtitleProps::underline>)
        ) {
            l3++;
        }
    }
}

const TSubtitleProps& TsubtitleFormat::processSSA(Twords &words, const TsubtitleLine &line, int sfmt, TsubtitleText &parent)
{
    if (line.empty()) {
        return lineProps;
    }
    const wchar_t *l=line[0];
    const wchar_t *l1=l,*l2=l;
    Tssa ssa(props, lineProps, parent.defProps, parent.getStyles(), words, sfmt);
    while (*l2) {
        if (l2[0]=='{') {
            if (const wchar_t *end=strchr(l2+1,'}')) {
                ssa.processTokens(l,l1 ,l2,end);
                l2=end+1;
                continue;
            }
            l2++;
        }
        // Process HTML tags in SSA subs when extended tags option is checked
        else if (parent.defProps.extendedTags) { // Add HTML support within SSA
            processHTMLTags(words,l,l1,l2);
        } else {
            l2++;
        }
    }

    words.add(l,l1,l2,props,0);
    return lineProps;
}

void TsubtitleFormat::processMicroDVD(TsubtitleText &parent, std::vector< TsubtitleLine >::iterator it)
{
    if (it->empty()) {
        return;
    }
    const wchar_t *line0=(*it)[0],*line=line0;
    while (*line)
        if (line[0]=='}' || line[0]==' ') {
            line++;
        } else if (_strnicmp(line,L"{y:",3)==0) {
            const wchar_t *end=strchr(line+3,'}');
            if (end==NULL) {
                break;
            }
            bool all=!!iswupper(line[1]);
            if (std::find_if(line+3,end,Tncasecmp<'i'>())!=end) {
                parent.propagateProps(all?parent.begin():it,&TSubtitleProps::italic   ,true,all?parent.end():it+1);
            }
            if (std::find_if(line+3,end,Tncasecmp<'b'>())!=end) {
                parent.propagateProps(all?parent.begin():it,&TSubtitleProps::bold     ,1,all?parent.end():it+1);
            }
            if (std::find_if(line+3,end,Tncasecmp<'u'>())!=end) {
                parent.propagateProps(all?parent.begin():it,&TSubtitleProps::underline,true,all?parent.end():it+1);
            }
            if (std::find_if(line+3,end,Tncasecmp<'s'>())!=end) {
                parent.propagateProps(all?parent.begin():it,&TSubtitleProps::strikeout,true,all?parent.end():it+1);
            }
            line=end+1;
        } else if (_strnicmp(line,L"{s:",3)==0) {
            int size;
            if (swscanf(line,L"{s:%i}",&size) || swscanf(line,L"{S:%i}",&size)) {
                parent.propagateProps(iswupper(line[1])?parent.begin():it, &TSubtitleProps::size, double(size), iswupper(line[1])?parent.end():it+1);
                const wchar_t *r=strchr(line,'}');
                if (r) {
                    line=r+1;
                }
            }
        } else if (_strnicmp(line,L"{c:$",4)==0) {
            COLORREF color;
            if (swscanf(line,L"{c:$%x}",&color) || swscanf(line,L"{C:$%x}",&color)) {
                parent.propagateProps(iswupper(line[1])?parent.begin():it,&TSubtitleProps::color,color,iswupper(line[1])?parent.end():it+1);
                const wchar_t *r=strchr(line,'}');
                if (r) {
                    parent.propagateProps(iswupper(line[1])?parent.begin():it,&TSubtitleProps::isColor,true,iswupper(line[1])?parent.end():it+1);
                    line=r+1;
                }
            }
        } else {
            break;
        }
    (*it)[0].eraseLeft(line-line0);
}

void TsubtitleFormat::processMPL2(TsubtitleLine &line)
{
    if (line.empty() || !line[0]) {
        return;
    }
    if (line[0][0]=='/') {
        foreach (TsubtitleWord &word,line)
        word.props.italic=true;
        line[0].eraseLeft(1);
    }
}
