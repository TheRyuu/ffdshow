#ifndef _FFDSHOW_VERSION_H_
#define _FFDSHOW_VERSION_H_

#include "svn_version.h"

#define VERSION_MAJOR 1
#define VERSION_MINOR 2
#define VERSION_BUILD SVN_REVISION

#ifndef ISPP_INVOKED
#define _STR(x) #x
#define STR(x) _STR(x)

#define VERSION_NUMBER          VERSION_MAJOR,VERSION_MINOR,VERSION_BUILD,0
#define VERSION_STRING          STR(VERSION_MAJOR) "." STR(VERSION_MINOR) "." STR(VERSION_BUILD) ".0"
#define VERSION_COMPANY         ""
#define VERSION_COPYRIGHT       "Copyright © 2002-2012"
#define VERSION_TRADEMARK       "GNU GPL"
#define VERSION_BUILD_DATE_TIME BUILD_YEAR "-" BUILD_MONTH "-" BUILD_DAY

#endif // ISPP_INVOKED

#endif // _FFDSHOW_VERSION_H_
