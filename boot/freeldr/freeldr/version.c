/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <freeldr.h>

#define TOSTRING_(X) #X
#define TOSTRING(X) TOSTRING_(X)

static const PCSTR FreeLoaderVersionString =
#if (FREELOADER_PATCH_VERSION == 0)
    "FreeLoader v" TOSTRING(FREELOADER_MAJOR_VERSION) "." TOSTRING(FREELOADER_MINOR_VERSION);
#else
    "FreeLoader v" TOSTRING(FREELOADER_MAJOR_VERSION) "." TOSTRING(FREELOADER_MINOR_VERSION) "." TOSTRING(FREELOADER_PATCH_VERSION);
#endif

const PCSTR GetFreeLoaderVersionString(VOID)
{
    return FreeLoaderVersionString;
}