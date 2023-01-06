
/*****************************************************************************
 * EXODE DATABASE MANAGER LIBRARY                                            *
 *                                                                           *
 * (C) 1997, Fred Pesch                                                      *
 *****************************************************************************/

/*****
* eXdbm.c : eXdbm main source file
*
* This file Version     $Revision: 2356 $
*
* Last modification:    $Date: 2015-10-29 07:47:26 +0100 (tor, 29 okt 2015) $
* By:                                   $Author: errolv $
* Current State:                $State$
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

/*      $Id: eXdbm.c 2356 2015-10-29 06:47:26Z errolv $         */

/* includes */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "eXdbmTypes.h"
#include "eXdbmErrors.h"
#include "misc.h"
#include "parse.h"

/* error messages */

const char * DbmErrorMsgs[] = {
  "eXdbm : No error",
  "eXdbm : Memory allocation error",
  "eXdbm : Database manager not initialized",
  "eXdbmInit : Database manager already initialized",
  "eXdbm : Cannot access the specified file",
  "eXdbmOpenDatabase : Cannot parse comment",
  "eXdbmOpenDatabase : Cannot parse identifier",
  "eXdbmOpenDatabase : Cannot parse variable value",
  "eXdbmOpenDatabase : Unexpected End Of File",
  "eXdbm : Cannot access Database file",
  "eXdbm : Wrong database ID",
  "eXdbm : Write error",
  "eXdbm : Cannot destroy database",
  "eXdbm : wrong variable type",
  "eXdbm : entry not found",
  "eXdbm : Bad parameter in eXdbm function call",
  "eXdbmCreate : Duplicate entry identifier"
};

/* global variables */

int DbmLastErrorCode = DBM_NO_ERROR; /* the current error value */
TDbmDbList *DbmDbList=NULL; /* private list variable */
unsigned long DbmParseLineNumber; /* line counter for parser */

/* public functions */

int eXdbmGetLastError(void);
const char *eXdbmGetErrorString(int errorcode);
int eXdbmLastLineParsed(void);

int eXdbmInit(void);
int eXdbmOpenDatabase(char *filename, DB_ID *dbid);
int eXdbmNewDatabase(char *filename, DB_ID *dbid);
int eXdbmUpdateDatabase(DB_ID dbid);
int eXdbmBackupDatabase(DB_ID dbid, char *filename);
int eXdbmCloseDatabase(DB_ID dbid, int update);
int eXdbmReloadDatabase(DB_ID *dbid, int update);

int eXdbmGetEntryType(DB_ID dbid, DB_LIST list, char *entryname);
char *eXdbmGetEntryComment(DB_ID dbid, DB_LIST list, char *entryname);

int eXdbmGetVarInt(DB_ID dbid, DB_LIST list, char *entryname, int *value);
int eXdbmGetVarReal(DB_ID dbid, DB_LIST list, char *entryname, double *value);
int eXdbmGetVarString(DB_ID dbid, DB_LIST list, char *entryname, char **value);
int eXdbmGetVarIdent(DB_ID dbid, DB_LIST list, char *entryname, char **value);
int eXdbmGetVarBool(DB_ID dbid, DB_LIST list, char *entryname, int *value);

DB_LIST eXdbmGetList(DB_ID dbid, DB_LIST list, char *listname);
DB_LIST eXdbmSearchList(DB_ID dbid, DB_LIST list, char *listname);
DB_LIST eXdbmPathList(DB_ID dbid, char *path);

int eXdbmChangeEntryComment(DB_ID dbid, DB_LIST list, char *entryname, char *comment);
int eXdbmChangeVarInt(DB_ID dbid, DB_LIST list, char *entryname, int newvalue);
int eXdbmChangeVarReal(DB_ID dbid, DB_LIST list, char *entryname, double newvalue);
int eXdbmChangeVarString(DB_ID dbid, DB_LIST list, char *entryname, char *newvalue);
int eXdbmChangeVarIdent(DB_ID dbid, DB_LIST list, char *entryname, char *newvalue);
int eXdbmChangeVarBool(DB_ID dbid, DB_LIST list, char *entryname, int newvalue);

int eXdbmCreateList(DB_ID dbid, DB_LIST list, char *entryname, char *comment);
int eXdbmCreateVarInt(DB_ID dbid, DB_LIST list, char *entryname, char *comment, int value);
int eXdbmCreateVarReal(DB_ID dbid, DB_LIST list, char *entryname, char *comment, double value);
int eXdbmCreateVarBool(DB_ID dbid, DB_LIST list, char *entryname, char *comment, int value);
int eXdbmCreateVarString(DB_ID dbid, DB_LIST list, char *entryname, char *comment, char *value);
int eXdbmCreateVarIdent(DB_ID dbid, DB_LIST list, char *entryname, char *comment, char *value);
int eXdbmDeleteEntry(DB_ID dbid, DB_LIST list, char *entryname);

/*****************************************************************************
 * ERROR HANDLING                                                            *
 *****************************************************************************/

int eXdbmGetLastError(void)
{
  int temp;

  temp = DbmLastErrorCode;
  DbmLastErrorCode = DBM_NO_ERROR;

  return temp;
}

const char *eXdbmGetErrorString(int errorcode)
{
  if(errorcode<DBM_END_ERROR)
    return (DbmErrorMsgs[errorcode]);

  return NULL;
}

int eXdbmLastLineParsed(void)
{
  return(DbmParseLineNumber);
}

/*****************************************************************************
 * DATABASE MANAGER INITIALIZATION                                           *
 *****************************************************************************/

int eXdbmInit(void)
{
  /* Dbm needs only one initialization */

  if(DbmDbList!=NULL) {
    RaiseError(DBM_INIT_REINIT);
    return -1;
  }

  /* create the DbmList variable */

  DbmDbList = (TDbmDbList *) malloc(sizeof(TDbmDbList));
  if(DbmDbList==NULL) {
    RaiseError(DBM_ALLOC);
    return -1;
  }

  DbmDbList->nb_db = 0;
  DbmDbList->dblist = NULL;
  DbmDbList->array_size = 0;

  return 1;
}

/*****************************************************************************
 * OPENING A NEW DATABASE                                                    *
 *****************************************************************************/

int eXdbmOpenDatabase(char *filename,  DB_ID *dbid)
{
  FILE *f;
  DB_ID temp_id = 0;
  int ret;
  int i;
  int addarray;

  /* the database manager must be initialized */

  ret = DbmIsInit();
  if( ret == -1) return(-1);

  /* try to open the file */

  f = fopen(filename, "rt");
  if(f==NULL) {
    RaiseError(DBM_OPEN_FILE);
    return -1;
  }

  /* search if there's an empty place in the array of databases */

  addarray = 1;

  for(i=0;i< DbmDbList->array_size; i++)
    if(DbmDbList->dblist[i].root==NULL) {
      temp_id = i;
      addarray = 0;
    }

  if(addarray == 1) {

    DbmDbList->array_size++;
    DbmDbList->dblist = (TDbmDatabase *) realloc( DbmDbList->dblist, sizeof(TDbmDatabase) * DbmDbList->array_size);
    if(DbmDbList->dblist==NULL) {
      RaiseError(DBM_ALLOC);
      fclose(f);
      return -1;
    }

    temp_id = DbmDbList->array_size - 1;
  }

  /* create the database variable */

 DbmDbList->dblist[temp_id].filename = (char *) malloc(sizeof(char)* (strlen(filename)+1));
  if(DbmDbList->dblist[temp_id].filename == NULL) {
    RaiseError(DBM_ALLOC);
    fclose(f);
    return(-1);
  }

  strcpy(DbmDbList->dblist[temp_id].filename, filename);

  DbmDbList->nb_db++;

  /* initialize the root list entry */

  DbmDbList->dblist[temp_id].root = (TDbmListEntry *) malloc (sizeof(TDbmListEntry));
  if(DbmDbList->dblist[temp_id].root == NULL) {
    RaiseError(DBM_ALLOC);
    fclose(f);
    return -1;
  }

  DbmDbList->dblist[temp_id].root->key = NULL ; /* unique root identifier */
  DbmDbList->dblist[temp_id].root->comment = NULL;
  DbmDbList->dblist[temp_id].root->entry_type = DBM_ENTRY_ROOT;
  DbmDbList->dblist[temp_id].root->value.int_val = -1;
  DbmDbList->dblist[temp_id].root->value.real_val = -1;
  DbmDbList->dblist[temp_id].root->value.str_val = NULL;
  DbmDbList->dblist[temp_id].root->next = NULL;

  DbmDbList->dblist[temp_id].root->order = (TDbmListEntry **) malloc(sizeof(TDbmListEntry *) * MIN_ORDER_SIZE);
  if(DbmDbList->dblist[temp_id].root->order == NULL) {
    RaiseError(DBM_ALLOC);
    fclose(f);
    return(-1);
  }

  DbmDbList->dblist[temp_id].root->size_order = MIN_ORDER_SIZE;
  DbmDbList->dblist[temp_id].root->current_order = 0;

  DbmDbList->dblist[temp_id].root->child = (TDbmListEntry **) malloc( sizeof(TDbmListEntry *) * HASH_MAX_ENTRIES);

  if(DbmDbList->dblist[temp_id].root->child == NULL) {
    RaiseError(DBM_ALLOC);
    fclose(f);
    return(-1);
  }

  for (i=0; i < HASH_MAX_ENTRIES ; i++)
    DbmDbList->dblist[temp_id].root->child[i]=NULL;

  /* now, parse the file */

  DbmParseLineNumber=1;

  ret = ParseFile(f, DbmDbList->dblist[temp_id].root, 0);

  if(ret==-1) {
    fclose(f);
    return -1;
  }

  /* termination */

  fclose(f);

  *dbid = temp_id;

  return 1;  /* no error */
}

/*****************************************************************************
 * CREATING A NEW DATABASE                                                   *
 *****************************************************************************/

int eXdbmNewDatabase(char *filename,  DB_ID *dbid)
{
  /* FILE *f; */
  DB_ID temp_id = 0;
  int ret;
  int i;
  int addarray;

  /* the database manager must be initialized */

  ret = DbmIsInit();
  if( ret == -1) return(-1);

  /* try to open the file */

  /*  f = fopen(filename, "wt");
  if(f==NULL) {
    RaiseError(DBM_OPEN_FILE);
    return -1;
  }

  fclose(f);
  */
  /* search if there's an empty place in the array of databases */

  addarray = 1;

  for(i=0;i< DbmDbList->array_size; i++)
    if(DbmDbList->dblist[i].root==NULL) {
      temp_id = i;
      addarray = 0;
    }

  if(addarray == 1) {

    DbmDbList->array_size++;
    DbmDbList->dblist = (TDbmDatabase *) realloc( DbmDbList->dblist, sizeof(TDbmDatabase) * DbmDbList->array_size);
    if(DbmDbList->dblist==NULL) {
      RaiseError(DBM_ALLOC);
      /* fclose(f); */
      return -1;
    }

    temp_id = DbmDbList->array_size - 1;
  }

  /* create the database variable */

  DbmDbList->dblist[temp_id].filename = (char *) malloc(sizeof(char)* (strlen(filename)+1));
  if(DbmDbList->dblist[temp_id].filename == NULL) {
    RaiseError(DBM_ALLOC);
    return(-1);
  }

  strcpy(DbmDbList->dblist[temp_id].filename, filename);

  DbmDbList->nb_db++;

  /* initialize the root list entry */

  DbmDbList->dblist[temp_id].root = (TDbmListEntry *) malloc (sizeof(TDbmListEntry));
  if(DbmDbList->dblist[temp_id].root == NULL) {
    RaiseError(DBM_ALLOC);
    /* fclose(f); */
    return -1;
  }

  DbmDbList->dblist[temp_id].root->key = NULL ; /* unique root identifier */
  DbmDbList->dblist[temp_id].root->comment = NULL;
  DbmDbList->dblist[temp_id].root->entry_type = DBM_ENTRY_ROOT;
  DbmDbList->dblist[temp_id].root->value.int_val = -1;
  DbmDbList->dblist[temp_id].root->value.real_val = -1;
  DbmDbList->dblist[temp_id].root->value.str_val = NULL;
  DbmDbList->dblist[temp_id].root->next = NULL;

  DbmDbList->dblist[temp_id].root->order = (TDbmListEntry **) malloc(sizeof(TDbmListEntry *) * MIN_ORDER_SIZE);
  if(DbmDbList->dblist[temp_id].root->order == NULL) {
    RaiseError(DBM_ALLOC);
    /* fclose(f); */
    return(-1);
  }

  DbmDbList->dblist[temp_id].root->size_order = MIN_ORDER_SIZE;
  DbmDbList->dblist[temp_id].root->current_order = 0;

  DbmDbList->dblist[temp_id].root->child = (TDbmListEntry **) malloc( sizeof(TDbmListEntry *) * HASH_MAX_ENTRIES);

  if(DbmDbList->dblist[temp_id].root->child == NULL) {
    RaiseError(DBM_ALLOC);
    /* fclose(f); */
    return(-1);
  }

  for (i=0; i < HASH_MAX_ENTRIES ; i++)
    DbmDbList->dblist[temp_id].root->child[i]=NULL;

  /* termination */

  *dbid = temp_id;

  return 1;  /* no error */
}

/*****************************************************************************
 * UPDATING A DATABASE                                                       *
 *****************************************************************************/

int eXdbmUpdateDatabase(DB_ID dbid)
{
  FILE *f;
  int ret;

  /* the database manager must be initialized */

  ret = DbmIsInit();
  if( ret == -1) return(-1);

  /* check the database identifier */

  ret = CheckDbIdent(dbid);
  if(ret==-1) {
    RaiseError(DBM_WRONG_ID);
    return(-1);
  }

  /* open the file for writing */

  f = fopen(DbmDbList->dblist[dbid].filename, "wt");
  if(f==NULL) {
    RaiseError(DBM_UPDATE_FILE);
    return -1;
  }

  ret = WriteDatabase(f, DbmDbList->dblist[dbid].root, 0);

  if(ret==-1) {
    RaiseError(DBM_UPDATE_WRITE_ERROR);
    return(-1);
  }

  fclose(f);

  return(1);

}

/*****************************************************************************
 * DATABASE BACKUP                                                           *
 *****************************************************************************/

int eXdbmBackupDatabase(DB_ID dbid, char *filename)
{
  FILE *f;
  int ret;

  /* the database manager must be initialized */

  ret = DbmIsInit();
  if( ret == -1) return(-1);

  /* check the database identifier */

  if(CheckDbIdent(dbid)==-1) {
    RaiseError(DBM_WRONG_ID);
    return(-1);
  }

  /* open the file for writing */

  f = fopen(filename, "wt");
  if(f==NULL) {
    RaiseError(DBM_UPDATE_FILE);
    return -1;
  }

  ret = WriteDatabase(f, DbmDbList->dblist[dbid].root, 0);

  if(ret==-1) {
    RaiseError(DBM_UPDATE_WRITE_ERROR);
    return(-1);
  }

  fclose(f);

  return(1);
}

/*****************************************************************************
 * CLOSING A DATABASE                                                        *
 *****************************************************************************/

int eXdbmCloseDatabase(DB_ID dbid, int update)

{
  int ret;

  /* the database manager must be initialized */

  ret = DbmIsInit();
  if( ret == -1) return(-1);

  /* check the database identifier */

  if(CheckDbIdent(dbid)==-1) {
    RaiseError(DBM_WRONG_ID);
    return(-1);
  }

  /* update the database if asked */

  if(update) ret = eXdbmUpdateDatabase(dbid);

  if(ret == -1) return (-1);

  /* now destroy the database entries */

  ret = DestroyDatabase(DbmDbList->dblist[dbid].root);

  if( ret == -1) return(-1);

  /* and the database itself */

  free(DbmDbList->dblist[dbid].root->child);

  free(DbmDbList->dblist[dbid].root->order);

  free(DbmDbList->dblist[dbid].root);
  DbmDbList->dblist[dbid].root=NULL;

  free(DbmDbList->dblist[dbid].filename);

  DbmDbList->nb_db--;

  return(1);
}

/*****************************************************************************
 * RELOAD A DATABASE                                                         *
 *****************************************************************************/

int eXdbmReloadDatabase(DB_ID *dbid, int update)
{

  int ret;
  char *filename;

  /* the database manager must be initialized */

  ret = DbmIsInit();
  if( ret == -1) return(-1);

  /* check the database identifier */

  if(CheckDbIdent(*dbid) == -1) {
    RaiseError(DBM_WRONG_ID);
    return(-1);
  }

  /* get the database file name */

  filename = (char *) malloc(sizeof(char) * (strlen(DbmDbList->dblist[*dbid].filename)+1));
  strcpy(filename, DbmDbList->dblist[*dbid].filename);

  /* first, close the database */

  ret = eXdbmCloseDatabase(*dbid, update);

  if(ret == -1) {
    free(filename);
    return(-1);
  }

  /* then, reopen it */

  ret = eXdbmOpenDatabase(filename, dbid);
  if(ret == -1) {
    free(filename);
    return(-1);
  }

  free(filename);

  return(1);

}
/*****************************************************************************
 * GET THE NAME OF THE FILE ASSOCIATED TO A DATABASE                         *
 *****************************************************************************/

char * eXdbmGetDatabaseFileName(DB_ID dbid)
{
  int ret;

  /* the database manager must be initialized */

  ret = DbmIsInit();
  if( ret == -1) return(NULL);

  /* check the dbid parameter */

  if(CheckDbIdent(dbid) == -1) {
    RaiseError(DBM_WRONG_ID);
    return(NULL);
  }

  return(DbmDbList->dblist[dbid].filename);
}


/*****************************************************************************
 * DATABASE QUERY FUNCTIONS                                                  *
 *****************************************************************************/

/************************/
/*** Entry type query ***/
/************************/

int eXdbmGetEntryType(DB_ID dbid, DB_LIST list, char *entryname)
{
  TDbmListEntry *search;
  int ret;

  /* the database manager must be initialized */

  ret = DbmIsInit();
  if( ret == -1) return(-1);

  /* check the dbid parameter */

  if(CheckDbIdent(dbid) == -1) {
    RaiseError(DBM_WRONG_ID);
    return(DBM_ENTRY_NOT_FOUND);
  }

  /* search entryname in list */

  if(list==NULL)  /* level 0 search */

    search = SearchListEntry(DbmDbList->dblist[dbid].root, entryname);

  else /* sublist search */

    search = SearchListEntry(list, entryname);

  if (search==NULL) return(DBM_ENTRY_NOT_FOUND);

  return (search->entry_type);
}

/***************************/
/*** Entry comment query ***/
/***************************/

char *eXdbmGetEntryComment(DB_ID dbid, DB_LIST list, char *entryname)
{
  TDbmListEntry *search;
  int ret;

  /* the database manager must be initialized */

  ret = DbmIsInit();
  if( ret == -1) return(NULL);

  /* check the dbid parameter */

  if(CheckDbIdent(dbid) == -1) {
    RaiseError(DBM_WRONG_ID);
    return(NULL);
  }

  /* search entryname in list */

  if(list==NULL)  /* level 0 search */

    search = SearchListEntry(DbmDbList->dblist[dbid].root, entryname);

  else /* sublist search */

    search = SearchListEntry(list, entryname);

  if (search==NULL) return(NULL);

  /* a comment is available ? */

  if(search->comment!=NULL)
    return(search->comment);

  return(NULL);

}

/***************************/
/*** integer value query ***/
/***************************/

int eXdbmGetVarInt(DB_ID dbid, DB_LIST list, char *entryname, int *value)
{
  TDbmListEntry *search;
  int ret;

  /* the database manager must be initialized */

  ret = DbmIsInit();
  if( ret == -1) return(-1);

  /* check the dbid parameter */

  if(CheckDbIdent(dbid) == -1) {
    RaiseError(DBM_WRONG_ID);
    return(DBM_ENTRY_NOT_FOUND);
  }

  /* search entryname in list */

  if(list==NULL)  /* level 0 search */

    search = SearchListEntry(DbmDbList->dblist[dbid].root, entryname);

  else /* sublist search */

    search = SearchListEntry(list, entryname);

  if (search==NULL) return(DBM_ENTRY_NOT_FOUND);

  /* check the variable type */

  if(search->entry_type== DBM_ENTRY_VAR_INT) {
    *value = search->value.int_val;
    return (search->entry_type);
  }

  /* wrong type */

  RaiseError(DBM_WRONG_TYPE);
  return(DBM_ENTRY_NOT_FOUND);
}

/************************/
/*** Real value query ***/
/************************/

int eXdbmGetVarReal(DB_ID dbid, DB_LIST list, char *entryname, double *value)
{
  TDbmListEntry *search;
  int ret;
  /* the database manager must be initialized */

  ret = DbmIsInit();
  if( ret == -1) return(-1);

  /* check the dbid parameter */

  if(CheckDbIdent(dbid) == -1) {
    RaiseError(DBM_WRONG_ID);
    return(DBM_ENTRY_NOT_FOUND);
  }

  /* search entryname in list */

  if(list==NULL)  /* level 0 search */

    search = SearchListEntry(DbmDbList->dblist[dbid].root, entryname);

  else /* sublist search */

    search = SearchListEntry(list, entryname);

  if (search==NULL) return(DBM_ENTRY_NOT_FOUND);

  /* check the variable type */

  if(search->entry_type== DBM_ENTRY_VAR_REAL) {
    *value = search->value.real_val;
    return (search->entry_type);
  }

  /* wrong type */

  RaiseError(DBM_WRONG_TYPE);
  return(DBM_ENTRY_NOT_FOUND);
}

/**************************/
/*** String value query ***/
/**************************/

int eXdbmGetVarString(DB_ID dbid, DB_LIST list, char *entryname, char **value)
{
  TDbmListEntry *search;
  int ret;

  /* the database manager must be initialized */

  ret = DbmIsInit();
  if( ret == -1) return(-1);

  /* check the dbid parameter */

  if(CheckDbIdent(dbid) == -1) {
    RaiseError(DBM_WRONG_ID);
    return(DBM_ENTRY_NOT_FOUND);
  }

  /* search entryname in list */

  if(list==NULL)  /* level 0 search */

    search = SearchListEntry(DbmDbList->dblist[dbid].root, entryname);

  else /* sublist search */

    search = SearchListEntry(list, entryname);

  if (search==NULL) return(DBM_ENTRY_NOT_FOUND);

  /* check the variable type */

  if(search->entry_type== DBM_ENTRY_VAR_STRING) {

    *value = (char *) malloc(sizeof(char) *(strlen(search->value.str_val)+1));
    if(*value == NULL) {
      RaiseError(DBM_ALLOC);
      return(-1);
    }
    strcpy(*value,search->value.str_val);
    return (search->entry_type);

  }

  /* wrong type */

  RaiseError(DBM_WRONG_TYPE);
  return(DBM_ENTRY_NOT_FOUND);
}

/******************************/
/*** Identifier value query ***/
/******************************/

int eXdbmGetVarIdent(DB_ID dbid, DB_LIST list, char *entryname, char **value)
{
  TDbmListEntry *search;
  int ret;

  /* the database manager must be initialized */

  ret = DbmIsInit();
  if( ret == -1) return(-1);

  /* check the dbid parameter */

  if(CheckDbIdent(dbid) == -1) {
    RaiseError(DBM_WRONG_ID);
    return(DBM_ENTRY_NOT_FOUND);
  }

  /* search entryname in list */

  if(list==NULL)  /* level 0 search */

    search = SearchListEntry(DbmDbList->dblist[dbid].root, entryname);

  else /* sublist search */

    search = SearchListEntry(list, entryname);

  if (search==NULL) return(DBM_ENTRY_NOT_FOUND);

  /* check the variable type */

  if(search->entry_type== DBM_ENTRY_VAR_IDENT) {
    *value = (char *) malloc(sizeof(char) *(strlen(search->value.str_val)+1));
    if(*value == NULL) {
      RaiseError(DBM_ALLOC);
      return(-1);
    }
    strcpy(*value,search->value.str_val);
    return (search->entry_type);
  }

  /* wrong type */

  RaiseError(DBM_WRONG_TYPE);
  return(DBM_ENTRY_NOT_FOUND);
}

/***************************/
/*** Boolean value query ***/
/***************************/

int eXdbmGetVarBool(DB_ID dbid, DB_LIST list, char *entryname, int *value)
{
  TDbmListEntry *search;
  int ret;

  /* the database manager must be initialized */

  ret = DbmIsInit();
  if( ret == -1) return(-1);

  /* check the dbid parameter */

  if(CheckDbIdent(dbid) == -1) {
    RaiseError(DBM_WRONG_ID);
    return(DBM_ENTRY_NOT_FOUND);
  }

  /* search entryname in list */

  if(list==NULL)  /* level 0 search */

    search = SearchListEntry(DbmDbList->dblist[dbid].root, entryname);

  else /* sublist search */

    search = SearchListEntry(list, entryname);

  if (search==NULL) return(DBM_ENTRY_NOT_FOUND);

  /* check the variable type */

  if(search->entry_type== DBM_ENTRY_VAR_BOOL) {
    *value = search->value.int_val;
    return (search->entry_type);
  }

  /* wrong type */

  RaiseError(DBM_WRONG_TYPE);
  return(DBM_ENTRY_NOT_FOUND);
}

/********************/
/*** List queries ***/
/********************/

/* one level search */

DB_LIST eXdbmGetList(DB_ID dbid, DB_LIST list, char *listname)
{
  TDbmListEntry *search;
  int ret;

  /* the database manager must be initialized */

  ret = DbmIsInit();
  if( ret == -1) return(NULL);

  /* check the dbid parameter */

  if(CheckDbIdent(dbid) == -1) {
    RaiseError(DBM_WRONG_ID);
    return(NULL);
  }

  /* search entryname in list */

  if(list==NULL)  /* level 0 search */

    search = SearchListEntry(DbmDbList->dblist[dbid].root, listname);

  else /* sublist search */

    search = SearchListEntry(list, listname);

  if (search==NULL) return(NULL);

  /* check the variable type */

  if(search->entry_type== DBM_ENTRY_LIST) return (search);

  /* wrong type */

  RaiseError(DBM_WRONG_TYPE);
  return(NULL);
}

/* recursive search */

DB_LIST eXdbmSearchList(DB_ID dbid, DB_LIST list, char *listname)
{
  TDbmListEntry *search;
  int ret;

  /* the database manager must be initialized */

  ret = DbmIsInit();
  if( ret == -1) return(NULL);

  /* check the dbid parameter */

  if(CheckDbIdent(dbid) == -1) {
    RaiseError(DBM_WRONG_ID);
    return(NULL);
  }

  /* search entryname in list */

  if(list==NULL)  /* level 0 search */

    search = SearchListEntryRec(DbmDbList->dblist[dbid].root, listname);

  else /* sublist search */

    search = SearchListEntryRec(list, listname);

  if (search==NULL) return(NULL);

  /* check the variable type */

  if(search->entry_type== DBM_ENTRY_LIST) return (search);

  /* wrong type */

  RaiseError(DBM_WRONG_TYPE);
  return(NULL);
}

/* one level search */

DB_LIST eXdbmPathList(DB_ID dbid, char *path)
{
  TDbmListEntry *search;
  char *found;
  int ret;

  /* the database manager must be initialized */

  ret = DbmIsInit();
  if( ret == -1) return(NULL);

  /* check the dbid parameter */

  if(CheckDbIdent(dbid) == -1) {
    RaiseError(DBM_WRONG_ID);
    return(NULL);
  }

  /* parse the path string */

  search = DbmDbList->dblist[dbid].root;

  found = strtok(path, ":");

  while(found!=NULL) {

  /* search entryname in list */

    search = SearchListEntry (search, found);

    if(search==NULL) return(NULL);

    found = strtok(NULL, ":");

  }

  return(search);
}

/*****************************************************************************
 * ENTRY VALUE MODIFICATIONS                                                 *
 *****************************************************************************/

/****************************/
/*** Change entry comment ***/
/****************************/

int eXdbmChangeEntryComment(DB_ID dbid, DB_LIST list, char *entryname, char *comment)
{
  TDbmListEntry *search;
  int ret;

  /* the database manager must be initialized */

  ret = DbmIsInit();
  if( ret == -1) return(-1);

  /* check the dbid parameter */

  if(CheckDbIdent(dbid) == -1) {
    RaiseError(DBM_WRONG_ID);
    return(-1);
  }

  /* check the comment value */

  if(comment==NULL) {
    RaiseError(DBM_BAD_PARAMETER);
    return(-1);
  }

  /* search entryname in list */

  if(list==NULL)  /* level 0 search */

    search = SearchListEntry(DbmDbList->dblist[dbid].root, entryname);

  else /* sublist search */

    search = SearchListEntry(list, entryname);

  if (search==NULL) return(DBM_ENTRY_NOT_FOUND);

  /* a comment is available ? */

  if(search->comment!=NULL) free(search->comment);

  search->comment = (char *) malloc( sizeof(char) * (strlen(comment)+1));
  if(search->comment == NULL) {
    RaiseError(DBM_ALLOC);
    return(-1);
  }

  strcpy(search->comment, comment);

  return(1);

}

/****************************/
/*** change integer value ***/
/****************************/

int eXdbmChangeVarInt(DB_ID dbid, DB_LIST list, char *entryname, int newvalue)
{
  TDbmListEntry *search;
  int ret;

  /* the database manager must be initialized */

  ret = DbmIsInit();
  if( ret == -1) return(-1);

  /* check the dbid parameter */

  if(CheckDbIdent(dbid) == -1) {
    RaiseError(DBM_WRONG_ID);
    return(DBM_ENTRY_NOT_FOUND);
  }

  /* search entryname in list */

  if(list==NULL)  /* level 0 search */

    search = SearchListEntry(DbmDbList->dblist[dbid].root, entryname);

  else /* sublist search */

    search = SearchListEntry(list, entryname);

  if (search==NULL) return(DBM_ENTRY_NOT_FOUND);

  /* check the variable type */

  if(search->entry_type== DBM_ENTRY_VAR_INT) {
    search->value.int_val=newvalue;
    return (search->entry_type);
  }

  /* wrong type */

  RaiseError(DBM_WRONG_TYPE);
  return(DBM_ENTRY_NOT_FOUND);
}

/*************************/
/*** change real value ***/
/*************************/

int eXdbmChangeVarReal(DB_ID dbid, DB_LIST list, char *entryname, double newvalue)
{
  TDbmListEntry *search;
  int ret;

  /* the database manager must be initialized */

  ret = DbmIsInit();
  if( ret == -1) return(-1);

  /* check the dbid parameter */

  if(CheckDbIdent(dbid) == -1) {
    RaiseError(DBM_WRONG_ID);
    return(DBM_ENTRY_NOT_FOUND);
  }

  /* search entryname in list */

  if(list==NULL)  /* level 0 search */

    search = SearchListEntry(DbmDbList->dblist[dbid].root, entryname);

  else /* sublist search */

    search = SearchListEntry(list, entryname);

  if (search==NULL) return(DBM_ENTRY_NOT_FOUND);

  /* check the variable type */

  if(search->entry_type== DBM_ENTRY_VAR_REAL) {
    search->value.real_val=newvalue;
    return (search->entry_type);
  }

  /* wrong type */

  RaiseError(DBM_WRONG_TYPE);
  return(DBM_ENTRY_NOT_FOUND);
}

/***************************/
/*** change string value ***/
/***************************/

int eXdbmChangeVarString(DB_ID dbid, DB_LIST list, char *entryname, char *newvalue)
{
  TDbmListEntry *search;
  int ret;

  /* the database manager must be initialized */

  ret = DbmIsInit();
  if( ret == -1) return(-1);

  /* check the dbid parameter */

  if(CheckDbIdent(dbid) == -1) {
    RaiseError(DBM_WRONG_ID);
    return(-1);
  }

  /* check the newvalue parameter */

  if(newvalue==NULL) {
    RaiseError(DBM_BAD_PARAMETER);
    return(-1);
  }

  /* search entryname in list */

  if(list==NULL)  /* level 0 search */

    search = SearchListEntry(DbmDbList->dblist[dbid].root, entryname);

  else /* sublist search */

    search = SearchListEntry(list, entryname);

  if (search==NULL) return(DBM_ENTRY_NOT_FOUND);

  /* check the variable type */

  if(search->entry_type== DBM_ENTRY_VAR_STRING) {

    if(search->value.str_val!=NULL) free(search->value.str_val);

    search->value.str_val = (char *) malloc(sizeof(char) * (strlen(newvalue)+1));
    if(search->value.str_val==NULL) {
      RaiseError(DBM_ALLOC);
      return(-1);
    }

    strcpy(search->value.str_val, newvalue);
    return (search->entry_type);
  }

  /* wrong type */

  RaiseError(DBM_WRONG_TYPE);
  return(DBM_ENTRY_NOT_FOUND);
}

/*******************************/
/*** Change identifier value ***/
/*******************************/

int eXdbmChangeVarIdent(DB_ID dbid, DB_LIST list, char *entryname, char *newvalue)
{
  TDbmListEntry *search;
  int ret;

  /* the database manager must be initialized */

  ret = DbmIsInit();
  if( ret == -1) return(-1);

  /* check the dbid parameter */

  if(CheckDbIdent(dbid) == -1) {
    RaiseError(DBM_WRONG_ID);
    return(-1);
  }

  /* check the string parameter */

  if( newvalue==NULL) {
    RaiseError(DBM_BAD_PARAMETER);
    return(-1);
  }

  /* search entryname in list */

  if(list==NULL)  /* level 0 search */

    search = SearchListEntry(DbmDbList->dblist[dbid].root, entryname);

  else /* sublist search */

    search = SearchListEntry(list, entryname);

  if (search==NULL) return(DBM_ENTRY_NOT_FOUND);

  /* check the variable type */

  if(search->entry_type== DBM_ENTRY_VAR_IDENT) {

    if(search->value.str_val!=NULL) free(search->value.str_val);

    search->value.str_val = (char *) malloc(sizeof(char) * (strlen(newvalue)+1));
    if(search->value.str_val==NULL) {
      RaiseError(DBM_ALLOC);
      return(-1);
    }

    strcpy(search->value.str_val, newvalue);
    return (search->entry_type);
  }

  /* wrong type */

  RaiseError(DBM_WRONG_TYPE);
  return(DBM_ENTRY_NOT_FOUND);
}

/****************************/
/*** Change boolean value ***/
/****************************/

int eXdbmChangeVarBool(DB_ID dbid, DB_LIST list, char *entryname, int newvalue)
{
  TDbmListEntry *search;
  int ret;

  /* the database manager must be initialized */

  ret = DbmIsInit();
  if( ret == -1) return(-1);

  /* check the dbid parameter */

  if(CheckDbIdent(dbid) == -1) {
    RaiseError(DBM_WRONG_ID);
    return(DBM_ENTRY_NOT_FOUND);
  }

  /* check the newvalue parameter */

  if(newvalue!=0 && newvalue!=1) {
    RaiseError(DBM_BAD_PARAMETER);
    return(-1);
  }

  /* search entryname in list */

  if(list==NULL)  /* level 0 search */

    search = SearchListEntry(DbmDbList->dblist[dbid].root, entryname);

  else /* sublist search */

    search = SearchListEntry(list, entryname);

  if (search==NULL) return(DBM_ENTRY_NOT_FOUND);

  /* check the variable type */

  if(search->entry_type== DBM_ENTRY_VAR_BOOL) {
    search->value.int_val = newvalue;
    return (search->entry_type);
  }

  /* wrong type */

  RaiseError(DBM_WRONG_TYPE);
  return(DBM_ENTRY_NOT_FOUND);
}

/*****************************************************************************
 * ENTRY CREATIONS                                                           *
 *****************************************************************************/

/***************************/
/*** Create a list entry ***/
/***************************/

int eXdbmCreateList(DB_ID dbid, DB_LIST list, char *entryname, char *comment)
{
  TDbmListEntry *node;
  int ret;

  /* the database manager must be initialized */

  ret = DbmIsInit();
  if( ret == -1) return(-1);

  /* check the dbid parameter */

  if(CheckDbIdent(dbid) == -1) {
    RaiseError(DBM_WRONG_ID);
    return(-1);
  }

  /* create entryname in list */

  if(list==NULL)  /* level 0 search */

    node = CreateListEntry(DbmDbList->dblist[dbid].root, entryname, comment, DBM_ENTRY_LIST);

  else /* sublist search */

    node = CreateListEntry(list, entryname, comment, DBM_ENTRY_LIST);

  if (node==NULL) return(-1);

  return (1);
}

/**********************************/
/*** Create an integer variable ***/
/**********************************/

int eXdbmCreateVarInt(DB_ID dbid, DB_LIST list, char *entryname, char *comment, int value)
{
  TDbmListEntry *node;
  int ret;

  /* the database manager must be initialized */

  ret = DbmIsInit();
  if( ret == -1) return(-1);

  /* check the dbid parameter */

  if(CheckDbIdent(dbid) == -1) {
    RaiseError(DBM_WRONG_ID);
    return(-1);
  }

  /* create entryname in list */

  if(list==NULL)  /* level 0 search */

    node = CreateListEntry(DbmDbList->dblist[dbid].root, entryname, comment, DBM_ENTRY_VAR_INT);

  else /* sublist search */

    node = CreateListEntry(list, entryname, comment, DBM_ENTRY_VAR_INT);

  if (node==NULL) return(-1);

  node->value.int_val = value;
  node->value.real_val = value;

  return (1);
}

/******************************/
/*** Create a real variable ***/
/******** *********************/

int eXdbmCreateVarReal(DB_ID dbid, DB_LIST list, char *entryname, char *comment, double value)
{
  TDbmListEntry *node;
  int ret;

  /* the database manager must be initialized */

  ret = DbmIsInit();
  if( ret == -1) return(-1);

  /* check the dbid parameter */

  if(CheckDbIdent(dbid) == -1) {
    RaiseError(DBM_WRONG_ID);
    return(-1);
  }

  /* create entryname in list */

  if(list==NULL)  /* level 0 search */

    node = CreateListEntry(DbmDbList->dblist[dbid].root, entryname, comment, DBM_ENTRY_VAR_REAL);

  else /* sublist search */

    node = CreateListEntry(list, entryname, comment, DBM_ENTRY_VAR_REAL);

  if (node==NULL) return(-1);

  node->value.int_val = (int)ceil(value);
  node->value.real_val = value;

  return (1);
}

/*********************************/
/*** Create a boolean variable ***/
/******** ************************/

int eXdbmCreateVarBool(DB_ID dbid, DB_LIST list, char *entryname, char *comment, int value)
{
  TDbmListEntry *node;
  int ret;

  /* the database manager must be initialized */

  ret = DbmIsInit();
  if( ret == -1) return(-1);

  /* check the dbid parameter */

  if(CheckDbIdent(dbid) == -1) {
    RaiseError(DBM_WRONG_ID);
    return(-1);
  }

  /* check the value parameter */

  if(value!=0 && value!=1) {
    RaiseError(DBM_BAD_PARAMETER);
    return(-1);
  }

  /* create entryname in list */

  if(list==NULL)  /* level 0 search */

    node = CreateListEntry(DbmDbList->dblist[dbid].root, entryname, comment, DBM_ENTRY_VAR_BOOL);

  else /* sublist search */

    node = CreateListEntry(list, entryname, comment, DBM_ENTRY_VAR_BOOL);

  if (node==NULL) return(-1);

  node->value.int_val = value;

  return (1);
}

/***************** **************/
/*** Create a string variable ***/
/******** ***********************/

int eXdbmCreateVarString(DB_ID dbid, DB_LIST list, char *entryname, char *comment, char *value)
{
  TDbmListEntry *node;
  int ret;

  /* the database manager must be initialized */

  ret = DbmIsInit();
  if( ret == -1) return(-1);

  /* check the dbid parameter */

  if(CheckDbIdent(dbid) == -1) {
    RaiseError(DBM_WRONG_ID);
    return(-1);
  }

  /* check the value parameter */

  if(value==NULL) {
    RaiseError(DBM_BAD_PARAMETER);
    return(-1);
  }

  /* create entryname in list */

  if(list==NULL)  /* level 0 search */

    node = CreateListEntry(DbmDbList->dblist[dbid].root, entryname, comment, DBM_ENTRY_VAR_STRING);

  else /* sublist search */

    node = CreateListEntry(list, entryname, comment, DBM_ENTRY_VAR_STRING);

  if (node==NULL) return(-1);

  node->value.str_val = (char *) malloc(sizeof(char) * (strlen(value)+1));
  if(node->value.str_val==NULL) {
    RaiseError(DBM_ALLOC);
    return(-1);
  }

  strcpy(node->value.str_val, value);

  return (1);
}

/*************************************/
/*** Create an identifier variable ***/
/******** ****************************/

int eXdbmCreateVarIdent(DB_ID dbid, DB_LIST list, char *entryname, char *comment, char *value)
{
  TDbmListEntry *node;
  int ret;

  /* the database manager must be initialized */

  ret = DbmIsInit();
  if( ret == -1) return(-1);

  /* check the dbid parameter */

  if(CheckDbIdent(dbid) == -1) {
    RaiseError(DBM_WRONG_ID);
    return(-1);
  }

  /* check the value parameter */

  if(value==NULL) {
    RaiseError(DBM_BAD_PARAMETER);
    return(-1);
  }

  /* create entryname in list */

  if(list==NULL)  /* level 0 search */

    node = CreateListEntry(DbmDbList->dblist[dbid].root, entryname, comment, DBM_ENTRY_VAR_IDENT);

  else /* sublist search */

    node = CreateListEntry(list, entryname, comment, DBM_ENTRY_VAR_IDENT);

  if (node==NULL) return(-1);

  node->value.str_val = (char *) malloc(sizeof(char) * (strlen(value)+1));
  if(node->value.str_val==NULL) {
    RaiseError(DBM_ALLOC);
    return(-1);
  }

  strcpy(node->value.str_val, value);

  return (1);
}

/*****************************************************************************
 * DELETE ENTRY FUNCTION                                                     *
 *****************************************************************************/

int eXdbmDeleteEntry(DB_ID dbid, DB_LIST list, char *entryname)
{
   int ret;

  /* the database manager must be initialized */

  ret = DbmIsInit();
  if( ret == -1) return(-1);

  /* check the dbid parameter */

  if(CheckDbIdent(dbid) == -1) {
    RaiseError(DBM_WRONG_ID);
    return(-1);
  }

  /* search entryname in list */

  if(list==NULL) /* level 0 search */

    ret = DeleteListEntry(DbmDbList->dblist[dbid].root, entryname);

  else /* sublist search */

    ret = DeleteListEntry(list, entryname);

  if (ret==-1) return(-1);

  return(1);
}
