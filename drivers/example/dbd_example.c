/*
 * libdbi - database independent abstraction layer for C.
 * Copyright (C) 2001-2002, David Parker and Mark Tobenkin.
 * http://libdbi.sourceforge.net
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 * dbd_example.c: Example database support (using libexampleclient)
 * Copyright (C) 2005, E.X. Ample <example@users.sourceforge.net>
 * http://libdbi.sourceforge.net
 * 
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _GNU_SOURCE /* we need asprintf */

#ifndef HAVE_ATOLL
long long atoll(const char *str);
#endif

#ifndef HAVE_STRTOLL
long long strtoll(const char *nptr, char **endptr, int base);
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <dbi/dbi.h>
#include <dbi/dbi-dev.h>
#include <dbi/dbd.h>

#include <example.h>
#include "dbd_example.h"

static const dbi_info_t driver_info = {
  "example",
  "Example database support (using libexampleclient)",
  "E.X. Ample <example@users.sourceforge.net>",
  "http://libdbi-drivers.sourceforge.net",
  "dbd_example v" VERSION,
  __DATE__
};

static const char *custom_functions[] = {NULL}; // TODO
static const char *reserved_words[] = EXAMPLE_RESERVED_WORDS;

/* encoding strings, array is terminated by a pair of empty strings */
static const char example_encoding_hash[][16] = {
  /* Example, www.iana.org */
  "ascii", "US-ASCII",
  "utf8", "UTF-8",
  "latin1", "ISO-8859-1",
  "", ""
};

/* forward declarations of local functions */
void _translate_example_type(enum enum_field_types fieldtype, unsigned short *type, unsigned int *attribs);
void _get_field_info(dbi_result_t *result);
void _get_row_data(dbi_result_t *result, dbi_row_t *row, unsigned long long rowidx);



/* Driver Infrastructure Functions */


void dbd_register_driver(const dbi_info_t **_driver_info, const char ***_custom_functions, const char ***_reserved_words) {
  /* this is the first function called after the driver module is loaded into memory */
  *_driver_info = &driver_info;
  *_custom_functions = custom_functions;
  *_reserved_words = reserved_words;
}

int dbd_initialize(dbi_driver_t *driver) {
  /* perform any database-specific server initialization.
   * this is called right after dbd_register_driver().
   * return -1 on error, 0 on success. if -1 is returned, the driver will not
   * be added to the list of available drivers. */
	
  return 0;
}

int dbd_connect(dbi_conn_t *conn) {

  /* set options */
  /* create a database connection */
  /* return 0 if successful, -1 if an error occurs */

  return 0;
}

int dbd_disconnect(dbi_conn_t *conn) {
  /* close connection */
  /* return 0 if successful, otherwise -1 */
  return 0;
}

int dbd_geterror(dbi_conn_t *conn, int *errno, char **errstr) {
  /* put error number into errno, error string into errstr
   * return 0 if error, 1 if errno filled, 2 if errstr filled, 3 if both errno and errstr filled */

}

int dbd_get_socket(dbi_conn_t *conn){
  /* return connection socket, if any */
  /* return -1 if an error occurs */
  /* return 0 if the client library does not use sockets */
}


/* Internal Database Query Functions */

int dbd_goto_row(dbi_result_t *result, unsigned long long rowidx) {
  /* move internal row index to rowidx (may be a no-op) */
  return 1;
}

int dbd_fetch_row(dbi_result_t *result, unsigned long long rowidx) {
  /* 0 on error, 1 on successful fetchrow */
}

int dbd_free_query(dbi_result_t *result) {
  /* free result data */
  return 0;
}


/* Public Database Query Functions */
const char *dbd_get_encoding(dbi_conn_t *conn){
  /* return connection encoding as an IANA name */
}

const char* dbd_encoding_to_iana(const char *db_encoding) {
  int i = 0;

  /* loop over all even entries in hash and compare to menc */
  while (*example_encoding_hash[i]) {
    if (!strncmp(example_encoding_hash[i], db_encoding, strlen(example_encoding_hash[i]))) {
      /* return corresponding odd entry */
      return example_encoding_hash[i+1];
    }
    i+=2;
  }

  /* don't know how to translate, return original encoding */
  return db_encoding;
}

const char* dbd_encoding_from_iana(const char *iana_encoding) {
  int i = 0;

  /* loop over all odd entries in hash and compare to ienc */
  while (*example_encoding_hash[i+1]) {
    if (!strcmp(example_encoding_hash[i+1], iana_encoding)) {
      /* return corresponding even entry */
      return example_encoding_hash[i];
    }
    i+=2;
  }

  /* don't know how to translate, return original encoding */
  return iana_encoding;
}

dbi_result_t *dbd_list_dbs(dbi_conn_t *conn, const char *pattern) {
  /* return a list of available databases. If pattern is non-NULL,
     return only the databases that match. Return NULL if an error
     occurs */
}

dbi_result_t *dbd_list_tables(dbi_conn_t *conn, const char *db, const char *pattern) {
  /* return a list of available tables. If pattern is non-NULL,
     return only the tables that match */
}

size_t dbd_quote_string(dbi_driver_t *driver, const char *orig, char *dest) {
  /* foo's -> 'foo\'s' */
  /* driver-specific, deprecated */
}

size_t dbd_conn_quote_string(dbi_conn_t *conn, const char *orig, char *dest) {
  /* foo's -> 'foo\'s' */
  /* connection-specific. Should take character encoding of current
     connection into account if db engine supports this */
}

size_t dbd_quote_binary(dbi_conn_t *conn, const char* orig, size_t from_length, char **ptr_dest) {
  /* *ptr_dest shall point to a zero-terminated string that can be
     used in SQL queries. Returns the lenght of that string in
     bytes, or DBI_LENGTH_ERROR in case of an error */
}

dbi_result_t *dbd_query(dbi_conn_t *conn, const char *statement) {
  /* allocate a new dbi_result_t and fill its applicable members:
   * 
   * result_handle, numrows_matched, and numrows_changed.
   * everything else will be filled in by DBI */
	
}

dbi_result_t *dbd_query_null(dbi_conn_t *conn, const unsigned char *statement, size_t st_length) {
  /* run query using a query string that may contain NULL bytes */
}

const char *dbd_select_db(dbi_conn_t *conn, const char *db) {
  /* make the requested database the current database */
}

unsigned long long dbd_get_seq_last(dbi_conn_t *conn, const char *sequence) {
  /* return ID of last INSERT */
}

unsigned long long dbd_get_seq_next(dbi_conn_t *conn, const char *sequence) {
  /* return ID of next INSERT */
}

int dbd_ping(dbi_conn_t *conn) {
  /* return 1 if connection is alive, otherwise 0 */
}

/* CORE EXAMPLE DATA FETCHING STUFF */

void _translate_example_type(enum enum_field_types fieldtype, unsigned short *type, unsigned int *attribs) {
  unsigned int _type = 0;
  unsigned int _attribs = 0;

  switch (fieldtype) {
  case FIELD_TYPE_TINY:
    _type = DBI_TYPE_INTEGER;
    _attribs |= DBI_INTEGER_SIZE1;
    break;
  case FIELD_TYPE_YEAR:
    _attribs |= DBI_INTEGER_UNSIGNED;
  case FIELD_TYPE_SHORT:
    _type = DBI_TYPE_INTEGER;
    _attribs |= DBI_INTEGER_SIZE2;
    break;
  case FIELD_TYPE_INT24:
    _type = DBI_TYPE_INTEGER;
    _attribs |= DBI_INTEGER_SIZE3;
    break;
  case FIELD_TYPE_LONG:
    _type = DBI_TYPE_INTEGER;
    _attribs |= DBI_INTEGER_SIZE4;
    break;
  case FIELD_TYPE_LONGLONG:
    _type = DBI_TYPE_INTEGER;
    _attribs |= DBI_INTEGER_SIZE8;
    break;
			
  case FIELD_TYPE_FLOAT:
    _type = DBI_TYPE_DECIMAL;
    _attribs |= DBI_DECIMAL_SIZE4;
    break;
  case FIELD_TYPE_DOUBLE:
    _type = DBI_TYPE_DECIMAL;
    _attribs |= DBI_DECIMAL_SIZE8;
    break;
			
  case FIELD_TYPE_DATE: /* TODO parse n stuph to native DBI unixtime type. for now, string */
    _type = DBI_TYPE_DATETIME;
    _attribs |= DBI_DATETIME_DATE;
    break;
  case FIELD_TYPE_TIME:
    _type = DBI_TYPE_DATETIME;
    _attribs |= DBI_DATETIME_TIME;
    break;
  case FIELD_TYPE_DATETIME:
  case FIELD_TYPE_TIMESTAMP:
    _type = DBI_TYPE_DATETIME;
    _attribs |= DBI_DATETIME_DATE;
    _attribs |= DBI_DATETIME_TIME;
    break;
			
  case FIELD_TYPE_DECIMAL: /* decimal is actually a string, has arbitrary precision, no floating point rounding */
  case FIELD_TYPE_ENUM:
  case FIELD_TYPE_SET:
  case FIELD_TYPE_VAR_STRING:
  case FIELD_TYPE_STRING:
    _type = DBI_TYPE_STRING;
    break;
			
  case FIELD_TYPE_TINY_BLOB:
  case FIELD_TYPE_MEDIUM_BLOB:
  case FIELD_TYPE_LONG_BLOB:
  case FIELD_TYPE_BLOB:
    _type = DBI_TYPE_BINARY;
    break;
			
  default:
    _type = DBI_TYPE_STRING;
    break;
  }
	
  *type = _type;
  *attribs = _attribs;
}

void _get_field_info(dbi_result_t *result) {
  /* retrieve field meta info */
}

void _get_row_data(dbi_result_t *result, dbi_row_t *row, unsigned long long rowidx) {
  /* get data of the current row */
}


