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

static _capability_t *_find_or_create_driver_cap(dbi_driver_t *driver, const char *capname);
static _capability_t *_find_or_create_conn_cap(dbi_conn_t *conn, const char *capname);

dbi_result_t *_dbd_result_create(dbi_conn_t *conn, void *handle, unsigned int numrows_matched, unsigned int numrows_affected) {
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
	return result;
}

void _dbd_result_set_numfields(dbi_result_t *result, unsigned int numfields) {
	result->numfields = numfields;
	result->field_names = calloc(numfields, sizeof(char *));
	result->field_types = calloc(numfields, sizeof(unsigned short));
	result->field_attribs = calloc(numfields, sizeof(unsigned int));
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
	row->field_sizes = calloc(numfields, sizeof(int));
	return row;
}

void _dbd_row_finalize(dbi_result_t *result, dbi_row_t *row, unsigned int idx) {
	/* rowidx is one-based in the DBI user level */
	result->rows[idx+1] = row;
}

int _dbd_quote_chars(const char *toescape, const char *quotes, const char *orig, char *dest, size_t destsize) {
	char *curdest = dest;
	const char *curorig = orig;
	const char *curescaped;
	
	strcpy(dest, quotes); // check, also use destidx < destsize, and null treminate
	
	strncpy(dest, orig, destsize);

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

dbi_result_t *_dbd_result_create_from_stringarray(dbi_conn_t *conn, unsigned int numrows_matched, const char **stringarray) {
	dbi_result_t *result = (dbi_result_t *) malloc(sizeof(dbi_result_t));
	unsigned int currow = 0;
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
	result->field_attribs = calloc(numfields, sizeof(unsigned int));
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
