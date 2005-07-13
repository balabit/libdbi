/*
 * libdbi - database independent abstraction layer for C.
 * Copyright (C) 2001-2003, David Parker and Mark Tobenkin.
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
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <limits.h>

#include <dbi/dbi.h>
#include <dbi/dbi-dev.h>

#ifndef HAVE_TIMEGM
time_t timegm(struct tm *tm);
#endif

static _capability_t *_find_or_create_driver_cap(dbi_driver_t *driver, const char *capname);
static _capability_t *_find_or_create_conn_cap(dbi_conn_t *conn, const char *capname);

int _dbd_result_add_to_conn(dbi_result_t *result) {
	dbi_conn_t *conn = result->conn;
	
	if (conn->results_size < conn->results_used+1) {
		dbi_result_t **results = (dbi_result_t **) realloc(conn->results, sizeof(dbi_result_t *) * (conn->results_size+1));
		if (!results) {
			return 0;
		}
		conn->results = results;
		conn->results_size++;
	}

	conn->results[conn->results_used] = result;
	conn->results_used++;
	return 1;
}

dbi_result_t *_dbd_result_create(dbi_conn_t *conn, void *handle, unsigned long long numrows_matched, unsigned long long numrows_affected) {
	dbi_result_t *result = (dbi_result_t *) malloc(sizeof(dbi_result_t));
	if (!result) return NULL;
	result->conn = conn;
	result->result_handle = handle;
	result->numrows_matched = numrows_matched;
	result->numrows_affected = numrows_affected;
	result->field_bindings = NULL;
	result->numfields = 0;
	result->field_names = NULL;
	result->field_types = NULL;
	result->field_attribs = NULL;
	result->result_state = (numrows_matched > 0) ? ROWS_RETURNED : NOTHING_RETURNED;
	result->rows = calloc(numrows_matched+1, sizeof(dbi_row_t *));
	result->currowidx = 0;

	if (!_dbd_result_add_to_conn(result)) {
		dbi_result_free((dbi_result)result);
		return NULL;
	}
	
	return result;
}

void _dbd_result_set_numfields(dbi_result_t *result, unsigned int numfields) {
	result->numfields = numfields;
	result->field_names = calloc(numfields, sizeof(char *));
	result->field_types = calloc(numfields, sizeof(unsigned short));
	result->field_attribs = calloc(numfields, sizeof(unsigned int *));
}

void _dbd_result_add_field(dbi_result_t *result, unsigned int idx, char *name, unsigned short type, unsigned int attribs) {
	if (name) result->field_names[idx] = strdup(name);
	result->field_types[idx] = type;
	result->field_attribs[idx] = attribs;
}

dbi_row_t *_dbd_row_allocate(unsigned int numfields) {
	dbi_row_t *row = malloc(sizeof(dbi_row_t));
	if (!row) return NULL;
	row->field_values = calloc(numfields, sizeof(dbi_data_t));
	row->field_sizes = calloc(numfields, sizeof(size_t));
	row->field_flags = calloc(numfields, sizeof(unsigned char));
	return row;
}

void _dbd_row_finalize(dbi_result_t *result, dbi_row_t *row, unsigned long long rowidx) {
	/* rowidx is one-based in the DBI user level */
	result->rows[rowidx+1] = row;
}

size_t _dbd_quote_chars(char *dest, const char *orig, size_t orig_size, const char *toescape) {
	char *curdest = dest;
	const char *curorig = orig;
	const char *curescaped;
	
/* 	strcpy(dest, quotes);  */// check, also use destidx < destsize, and null treminate
	
	strncpy(dest, orig, orig_size);

	while (curorig) {
		curescaped = toescape;
		while (curescaped) {
			if (*curorig == *curescaped) {
				*curdest = '\\';
				curdest++;
				*curdest = *curorig;
				continue;
			}
			curescaped++;
		}
		curorig++;
		curdest++;
	}

	return strlen(dest);
}

void _dbd_internal_error_handler(dbi_conn_t *conn, const char *errmsg, const int errno) {
	if (conn->error_message) free(conn->error_message);
	
	conn->error_flag = DBI_ERROR_DBD;
	conn->error_number = errno;
	conn->error_message = strdup(errmsg);

	if (conn->error_handler != NULL) {
		conn->error_handler((dbi_conn)conn, conn->error_handler_argument);
	}
}

dbi_result_t *_dbd_result_create_from_stringarray(dbi_conn_t *conn, unsigned long long numrows_matched, const char **stringarray) {
	dbi_result_t *result = (dbi_result_t *) malloc(sizeof(dbi_result_t));
	unsigned long long currow = 0;
	const int numfields = 1;
	
	if (!result) return NULL;
	
	/* initialize the result */
	result->conn = conn;
	result->result_handle = NULL;
	result->numrows_matched = numrows_matched;
	result->numrows_affected = 0;
	result->field_bindings = NULL;
	result->numfields = numfields;
	result->field_names = NULL;
	result->field_types = calloc(numfields, sizeof(unsigned short));
	result->field_attribs = calloc(numfields, sizeof(unsigned int *));
	result->result_state = (numrows_matched > 0) ? ROWS_RETURNED : NOTHING_RETURNED;
	result->rows = calloc(numrows_matched+1, sizeof(dbi_row_t *));
	result->currowidx = 0;


	/* then set numfields */
	result->field_types[0] = DBI_TYPE_STRING;
	result->field_attribs[0] = 0;
	
	/* then alloc a row, set row's data, and finalize (for each row) */
	for (currow = 0; currow < numrows_matched; currow++) {
		dbi_row_t *row = _dbd_row_allocate(numfields);
		row->field_values[0].d_string = strdup(stringarray[currow]);
		row->field_sizes[0] = strlen(stringarray[currow]);
		_dbd_row_finalize(result, row, 0);
	}
	
	if (!_dbd_result_add_to_conn(result)) {
		dbi_result_free((dbi_result)result);
		return NULL;
	}
	
	return result;
}

void _dbd_register_driver_cap(dbi_driver_t *driver, const char *capname, int value) {
	_capability_t *cap = _find_or_create_driver_cap(driver, capname);
	if (!cap) return;
	cap->value = value;
	return;
}

void _dbd_register_conn_cap(dbi_conn_t *conn, const char *capname, int value) {
	_capability_t *cap = _find_or_create_conn_cap(conn, capname);
	if (!cap) return;
	cap->value = value;
	return;
}

static _capability_t *_find_or_create_driver_cap(dbi_driver_t *driver, const char *capname) {
	_capability_t *prevcap = NULL;
	_capability_t *cap = driver->caps;

	while (cap && strcmp(capname, cap->name)) {
		prevcap = cap;
		cap = cap->next;
	}

	if (cap == NULL) {
		/* allocate a new node */
		cap = (_capability_t *) malloc(sizeof(_capability_t));
		if (!cap) return NULL;
		cap->name = strdup(capname);
		cap->next = NULL;
		if (driver->caps == NULL) {
		    driver->caps = cap;
		}
		else {
		    prevcap->next = cap;
		}
	}

	return cap;
}

static _capability_t *_find_or_create_conn_cap(dbi_conn_t *conn, const char *capname) {
	_capability_t *prevcap = NULL;
	_capability_t *cap = conn->caps;

	while (cap && strcmp(capname, cap->name)) {
		prevcap = cap;
		cap = cap->next;
	}

	if (cap == NULL) {
		/* allocate a new node */
		cap = (_capability_t *) malloc(sizeof(_capability_t));
		if (!cap) return NULL;
		cap->next = NULL;
		cap->name = strdup(capname);
		if (conn->caps == NULL) {
		    conn->caps = cap;
		}
		else {
		    prevcap->next = cap;
		}
	}

	return cap;
}

time_t _dbd_parse_datetime(const char *raw, unsigned long attribs) {
	struct tm unixtime;
	char *unparsed;
	char *cur;
	int check_time = 1;

	unixtime.tm_sec = unixtime.tm_min = unixtime.tm_hour = 0;
	unixtime.tm_mday = 1; /* days are 1 through 31 */
	unixtime.tm_mon = 0;
	unixtime.tm_year = 70; /* can't start before Unix epoch */
	unixtime.tm_isdst = -1;
	
	if (raw && (unparsed = strdup(raw)) != NULL) {
	  cur = unparsed;

	  /* this code assumes the following input in cur: */
	  /* DATE: YYYY-MM-DD (the dashes may be any other separator) */
	  /* TIME: HH:MM:SS (the colons may be any other separator) */
	  /* DATETIME: YYYY-MM-DD HH:MM:SS (the dashes and colons may 
	     be any other separator) */
	  if (strlen(cur) > 9 && attribs & DBI_DATETIME_DATE) {
	    if (strlen(cur) < 11) {
	      check_time = 0;
	    }
	    cur[4] = '\0';
	    cur[7] = '\0';
	    cur[10] = '\0';
	    unixtime.tm_year = atoi(cur)-1900;
	    unixtime.tm_mon = atoi(cur+5)-1; /* months are 0 through 11 */
	    unixtime.tm_mday = atoi(cur+8);
	    if (attribs & DBI_DATETIME_TIME) {
	      cur += 11;
	      if (*cur == ' ') {
		cur++;
	      }
	    }
	  }
	  
	  if (check_time && strlen(cur) > 7 && attribs & DBI_DATETIME_TIME) {
	    cur[2] = '\0';
	    cur[5] = '\0';
	    unixtime.tm_hour = atoi(cur);
	    unixtime.tm_min = atoi(cur+3);
	    unixtime.tm_sec = atoi(cur+6);
	  }

	  free(unparsed);
	}

	/* output is UTC, not local time */
	return timegm(&unixtime);
}

/* encoding/decoding of binary strings. The code, including the
   introductory comments, was literally stolen from the SQLite 2.8.16
   tarball, with minor adjustments. The author of the code is
   D. Richard Hipp, who released the code unhampered by any
   restrictive license whatsoever. See http://www.sqlite.org for
   further information */

/*
** How This Encoder Works
**
** The output is allowed to contain any character except 0x27 (') and
** 0x00.  This is accomplished by using an escape character to encode
** 0x27 and 0x00 as a two-byte sequence.  The escape character is always
** 0x01.  An 0x00 is encoded as the two byte sequence 0x01 0x01.  The
** 0x27 character is encoded as the two byte sequence 0x01 0x28.  Finally,
** the escape character itself is encoded as the two-character sequence
** 0x01 0x02.
**
** To summarize, the encoder works by using an escape sequences as follows:
**
**       0x00  ->  0x01 0x01
**       0x01  ->  0x01 0x02
**       0x27  ->  0x01 0x28
**
** If that were all the encoder did, it would work, but in certain cases
** it could double the size of the encoded string.  For example, to
** encode a string of 100 0x27 characters would require 100 instances of
** the 0x01 0x03 escape sequence resulting in a 200-character output.
** We would prefer to keep the size of the encoded string smaller than
** this.
**
** To minimize the encoding size, we first add a fixed offset value to each 
** byte in the sequence.  The addition is modulo 256.  (That is to say, if
** the sum of the original character value and the offset exceeds 256, then
** the higher order bits are truncated.)  The offset is chosen to minimize
** the number of characters in the string that need to be escaped.  For
** example, in the case above where the string was composed of 100 0x27
** characters, the offset might be 0x01.  Each of the 0x27 characters would
** then be converted into an 0x28 character which would not need to be
** escaped at all and so the 100 character input string would be converted
** into just 100 characters of output.  Actually 101 characters of output - 
** we have to record the offset used as the first byte in the sequence so
** that the string can be decoded.  Since the offset value is stored as
** part of the output string and the output string is not allowed to contain
** characters 0x00 or 0x27, the offset cannot be 0x00 or 0x27.
**
** Here, then, are the encoding steps:
**
**     (1)   Choose an offset value and make it the first character of
**           output.
**
**     (2)   Copy each input character into the output buffer, one by
**           one, adding the offset value as you copy.
**
**     (3)   If the value of an input character plus offset is 0x00, replace
**           that one character by the two-character sequence 0x01 0x01.
**           If the sum is 0x01, replace it with 0x01 0x02.  If the sum
**           is 0x27, replace it with 0x01 0x03.
**
**     (4)   Put a 0x00 terminator at the end of the output.
**
** Decoding is obvious:
**
**     (5)   Copy encoded characters except the first into the decode 
**           buffer.  Set the first encoded character aside for use as
**           the offset in step 7 below.
**
**     (6)   Convert each 0x01 0x01 sequence into a single character 0x00.
**           Convert 0x01 0x02 into 0x01.  Convert 0x01 0x28 into 0x27.
**
**     (7)   Subtract the offset value that was the first character of
**           the encoded buffer from all characters in the output buffer.
**
** The only tricky part is step (1) - how to compute an offset value to
** minimize the size of the output buffer.  This is accomplished by testing
** all offset values and picking the one that results in the fewest number
** of escapes.  To do that, we first scan the entire input and count the
** number of occurances of each character value in the input.  Suppose
** the number of 0x00 characters is N(0), the number of occurances of 0x01
** is N(1), and so forth up to the number of occurances of 0xff is N(255).
** An offset of 0 is not allowed so we don't have to test it.  The number
** of escapes required for an offset of 1 is N(1)+N(2)+N(40).  The number
** of escapes required for an offset of 2 is N(2)+N(3)+N(41).  And so forth.
** In this way we find the offset that gives the minimum number of escapes,
** and thus minimizes the length of the output string.
*/

/*
** Encode a binary buffer "in" of size n bytes so that it contains
** no instances of characters '\'' or '\000'.  The output is 
** null-terminated and can be used as a string value in an INSERT
** or UPDATE statement.  Use sqlite_decode_binary() to convert the
** string back into its original binary.
**
** The result is written into a preallocated output buffer "out".
** "out" must be able to hold at least 2 +(257*n)/254 bytes.
** In other words, the output will be expanded by as much as 3
** bytes for every 254 bytes of input plus 2 bytes of fixed overhead.
** (This is approximately 2 + 1.0118*n or about a 1.2% size increase.)
**
** The return value is the number of characters in the encoded
** string, excluding the "\000" terminator.
**
** If out==NULL then no output is generated but the routine still returns
** the number of characters that would have been generated if out had
** not been NULL.
*/
size_t _dbd_encode_binary(const unsigned char *in, size_t n, unsigned char *out){
  int i, j, e, m;
  unsigned char x;
  int cnt[256];
  if( n<=0 ){
    if( out ){
      out[0] = 'x';
      out[1] = 0;
    }
    return (size_t)1;
  }
  memset(cnt, 0, sizeof(cnt));
  for(i=n-1; i>=0; i--){ cnt[in[i]]++; }
  m = n;
  for(i=1; i<256; i++){
    int sum;
    if( i=='\'' ) continue;
    sum = cnt[i] + cnt[(i+1)&0xff] + cnt[(i+'\'')&0xff];
    if( sum<m ){
      m = sum;
      e = i;
      if( m==0 ) break;
    }
  }
  if( out==0 ){
    return (size_t)n+m+1;
  }
  out[0] = e;
  j = 1;
  for(i=0; i<n; i++){
    x = in[i] - e;
    if( x==0 || x==1 || x=='\''){
      out[j++] = 1;
      x++;
    }
    out[j++] = x;
  }
  out[j] = 0;
  return (size_t)j;
}

/*
** Decode the string "in" into binary data and write it into "out".
** This routine reverses the encoding created by sqlite_encode_binary().
** The output will always be a few bytes less than the input.  The number
** of bytes of output is returned.  If the input is not a well-formed
** encoding, -1 is returned.
**
** The "in" and "out" parameters may point to the same buffer in order
** to decode a string in place.
*/
size_t _dbd_decode_binary(const unsigned char *in, unsigned char *out){
  int i, e;
  unsigned char c;
  e = *(in++);
  i = 0;
  while( (c = *(in++))!=0 ){
    if( c==1 ){
      c = *(in++) - 1;
    }
    out[i++] = c + e;
  }
  return (size_t)i;
}
