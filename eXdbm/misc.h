
/* 	$Id: misc.h 230 2002-04-15 22:03:02Z linas $	 */

/*****
* misc.h : eXdbm misc. functions header
*
* This file Version	$Revision: 230 $
*
* Last modification: 	$Date: 2002-04-16 00:03:02 +0200 (tis, 16 apr 2002) $
* By:					$Author: linas $
* Current State:		$State$
*
* Copyright (C) 1997 Fred Pesch 
* All Rights Reserved
*
* This file is part of the eXdbm Library.
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Library General Public
* License as published by the Free Software Foundation; either
* version 2 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Library General Public License for more details.
*
* You should have received a copy of the GNU Library General Public
* License along with this library; if not, write to the Free
* Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*****/

#ifndef MISC_H
#define MISC_H

#include <stdio.h>
#include "eXdbmTypes.h"

void RaiseError(int errorcode);
int DbmIsInit(void);
int CheckDbIdent(DB_ID dbid);
int WriteDatabase(FILE *f, TDbmListEntry *list, int level);
int DestroyDatabase(TDbmListEntry *list);
void PrintDatabase(TDbmListEntry *list, int level);
TDbmListEntry * SearchListEntry(TDbmListEntry *list, char *namevar);
TDbmListEntry * SearchListEntryRec(TDbmListEntry *list, char *entryname);
TDbmListEntry * CreateListEntry(TDbmListEntry *list, char *entryname, char *comment, int entrytype);
int DeleteListEntry(TDbmListEntry *list, char *entryname);
int AddOrderEntry(TDbmListEntry *list, TDbmListEntry *element);
#endif /* end of misc.h */





