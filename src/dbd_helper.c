/*
 * libdbi - database independent abstraction layer for C.
 * Copyright (C) 2001, David Parker and Mark Tobenkin.
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

dbi_result_t *_dbd_result_create(dbi_driver_t *driver, void *handle, unsigned int numrows_matched, unsigned int numrows_affected) {
	dbi_result_t *result = (dbi_result_t *) malloc(sizeof(dbi_result_t));
	if (!result) return NULL;
	result->driver = driver;
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
