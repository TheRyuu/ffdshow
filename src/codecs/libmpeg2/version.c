/*
 * Copyright (c) 2004-2006 Milan Cutka
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include <string.h>
#include <inttypes.h>
#include "mpeg2.h"
#include "../../compiler.h"

extern "C" void __stdcall getVersion(char *ver,const char* *license)
{
 strcpy(ver,VERSION", "COMPILER COMPILER_X64 COMPILER_INFO" ("__DATE__" "__TIME__")");
 *license="(C) 2000-2003 Michel Lespinasse <walken@zoy.org>\n(C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>";
}
