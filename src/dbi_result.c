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
 *
 * (anything that has to do with row seeking or fetching fields goes in this file)
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

extern void _error_handler(dbi_conn_t *conn);

/* declarations for internal functions -- anything declared as static won't be accessible by name from client programs */
static _field_binding_t *_find_or_create_binding_node(dbi_result_t *result, const char *fieldname);
static void _remove_binding_node(dbi_result_t *result, _field_binding_t *deadbinding);
static int _find_field(dbi_result_t *result, const char *fieldname);
static int _is_row_fetched(dbi_result_t *result, unsigned int row);
static int _setup_binding(dbi_result_t *result, const char *fieldname, void *bindto, void *helperfunc);
static void _activate_bindings(dbi_result_t *result);
static int _parse_field_formatstr(const char *format, char ***tokens_dest, char ***fieldnames_dest);
static void _free_string_list(char **booyah, int total);
static void _free_result_rows(dbi_result_t *result);

static void _bind_helper_char(_field_binding_t *binding);
static void _bind_helper_uchar(_field_binding_t *binding);
static void _bind_helper_short(_field_binding_t *binding);
static void _bind_helper_ushort(_field_binding_t *binding);
static void _bind_helper_long(_field_binding_t *binding);
static void _bind_helper_ulong(_field_binding_t *binding);
static void _bind_helper_longlong(_field_binding_t *binding);
static void _bind_helper_ulonglong(_field_binding_t *binding);
static void _bind_helper_float(_field_binding_t *binding);
static void _bind_helper_double(_field_binding_t *binding);
static void _bind_helper_string(_field_binding_t *binding);
static void _bind_helper_binary(_field_binding_t *binding);
static void _bind_helper_string_copy(_field_binding_t *binding);
static void _bind_helper_binary_copy(_field_binding_t *binding);
static void _bind_helper_set(_field_binding_t *binding);
static void _bind_helper_enum(_field_binding_t *binding);
static void _bind_helper_datetime(_field_binding_t *binding);

/* XXX ROW SEEKING AND FETCHING XXX */

int dbi_result_seek_row(dbi_result Result, unsigned int row) {
	dbi_result_t *result = Result;
	int retval;

	if (!result || (result->result_state == NOTHING_RETURNED) || (row > result->numrows_matched)) return 0;

	if (_is_row_fetched(result, row) == 1) {
		/* jump right to it */
		result->currowidx = row;
		_activate_bindings(result);
		return 1;
	}
	
	/* row is one-based for the user, but zero-based to the dbd conn */
	retval = result->conn->driver->functions->goto_row(result, row-1);
	if (retval == -1) {
		_error_handler(result->conn);
	}
	retval = result->conn->driver->functions->fetch_row(result, row-1);
	if (retval == 0) {
		_error_handler(result->conn);
		return 0;
	}

	result->currowidx = row;
	_activate_bindings(result);
	return retval;
}

int dbi_result_first_row(dbi_result Result) {
	return dbi_result_seek_row(Result, 1);
}

int dbi_result_last_row(dbi_result Result) {
	return dbi_result_seek_row(Result, dbi_result_get_numrows(Result));
}

int dbi_result_prev_row(dbi_result Result) {
	dbi_result_t *result = Result;
	if (!result) return 0;
	if (result->currowidx <= 1) return 0;
	return dbi_result_seek_row(Result, result->currowidx-1);
}

int dbi_result_next_row(dbi_result Result) {
	dbi_result_t *result = Result;
	if (!result) return 0;
	if (result->currowidx >= dbi_result_get_numrows(Result)) return 0;
	return dbi_result_seek_row(Result, result->currowidx+1);
}

unsigned int dbi_result_get_numrows(dbi_result Result) {
	dbi_result_t *result = Result;
	if (!result) return 0;
	return result->numrows_matched;
}

unsigned int dbi_result_get_numrows_affected(dbi_result Result) {
	dbi_result_t *result = Result;
	if (!result) return 0;
	return result->numrows_affected;
}

unsigned int dbi_result_get_field_size(dbi_result Result, const char *fieldname) {
	dbi_result_t *result = Result;
	int idx = 0;
	
	if (!result) return 0;
	idx = _find_field(result, fieldname);
	if (idx < 0) return 0;

	return dbi_result_get_field_size_idx(Result, idx+1);
}

unsigned int dbi_result_get_field_size_idx(dbi_result Result, unsigned int idx) {
	dbi_result_t *result = Result;
	int currowidx;
	int temp;
	idx--;
	
	if (!result || !result->rows) return 0;
	currowidx = result->currowidx;
	if (!result->rows[currowidx] || !result->rows[currowidx]->field_sizes) return 0;
	if (idx >= result->numfields) return 0;

	temp = result->rows[currowidx]->field_sizes[idx];
	if (temp == -1) return 0;
	else return temp;
}

unsigned int dbi_result_get_field_length(dbi_result Result, const char *fieldname) {
	unsigned int size = dbi_result_get_field_size(Result, fieldname);
	return (size == 0) ? 0 : size-1;
}

unsigned int dbi_result_get_field_length_idx(dbi_result Result, unsigned int idx) {
	unsigned int size = dbi_result_get_field_size_idx(Result, idx);
	return (size == 0) ? 0 : size-1;
}

int dbi_result_get_field_idx(dbi_result Result, const char *fieldname) {
	dbi_result_t *result = Result;
	if (!result) return -1;
	return _find_field(result, fieldname)+1;
}

const char *dbi_result_get_field_name(dbi_result Result, unsigned int fieldnum) {
	dbi_result_t *result = Result;
	if (!result || (fieldnum > result->numfields)) return NULL;
	return (const char *) result->field_names[fieldnum-1];
}

unsigned int dbi_result_get_numfields(dbi_result Result) {
	dbi_result_t *result = Result;
	if (!result) return 0;
	return result->numfields;
}

unsigned short dbi_result_get_field_type(dbi_result Result, const char *fieldname) {
	dbi_result_t *result = Result;
	int idx = 0;
	
	if (!result) return 0;
	idx = _find_field(result, fieldname);
	if (idx < 0) return 0;

	return dbi_result_get_field_type_idx(Result, idx+1);
}

unsigned short dbi_result_get_field_type_idx(dbi_result Result, unsigned int idx) {
	dbi_result_t *result = Result;
	idx--;
	
	if (!result || !result->field_types) return 0;
	if (idx >= result->numfields) return 0;

	return result->field_types[idx];
}

unsigned long dbi_result_get_field_attrib(dbi_result Result, const char *fieldname, unsigned long attribmin, unsigned long attribmax) {
	dbi_result_t *result = Result;
	int idx = 0;
	
	if (!result) return 0;
	idx = _find_field(result, fieldname);
	if (idx < 0) return 0;

	return dbi_result_get_field_attrib_idx(Result, idx+1, attribmin, attribmax);
}

unsigned long dbi_result_get_field_attrib_idx(dbi_result Result, unsigned int idx, unsigned long attribmin, unsigned long attribmax) {
	dbi_result_t *result = Result;
	idx--;
	
	if (!result || !result->field_attribs) return 0;
	if (idx >= result->numfields) return 0;

	return _isolate_attrib(result->field_attribs[idx], attribmin, attribmax);
}

unsigned long dbi_result_get_field_attribs(dbi_result Result, const char *fieldname) {
	dbi_result_t *result = Result;
	int idx = 0;
	
	if (!result) return 0;
	idx = _find_field(result, fieldname);
	if (idx < 0) return 0;

	return dbi_result_get_field_attribs_idx(Result, idx+1);
}

unsigned long dbi_result_get_field_attribs_idx(dbi_result Result, unsigned int idx) {
	dbi_result_t *result = Result;
	idx--;
	
	if (!result || !result->field_attribs) return 0;
	if (idx >= result->numfields) return 0;

	return result->field_attribs[idx];
}

int dbi_result_free(dbi_result Result) {
	dbi_result_t *result = Result;
	int idx = 0;
	int retval;
	if (!result) return -1;
	
	retval = result->conn->driver->functions->free_query(result);

	while (result->field_bindings) {
		_remove_binding_node(result, result->field_bindings);
	}
	
	if (result->rows) {
			}
		
	if ( result->numfields ) {
		_free_string_list(result->field_names, result->numfields);
		free(result->field_types);
		free(result->field_attribs);
	}

	if (retval == -1) {
		_error_handler(result->conn);
	}

	free(result);
	return retval;
}

dbi_conn dbi_result_get_conn(dbi_result Result) {
	dbi_result_t *result = Result;
	if (!result) return NULL;
	return result->conn;
}

/* RESULT: mass retrieval functions */

static int _parse_field_formatstr(const char *format, char ***tokens_dest, char ***fieldnames_dest) {
	int found = 0;
	int cur = 0;
	char **tokens = NULL;
	char **fieldnames = NULL;
	char *chunk;
	char *fieldtype;
	char *fieldname;
	char *temp1;
	char *temp2;
	char *line = strdup(format);
	
	temp1 = line;
	while (temp1 && (temp1 = strchr(temp1, '.')) != NULL) {
		temp1++;
		found++;
	}
	
	tokens = calloc(found, sizeof(char *));
	fieldnames = calloc(found, sizeof(char *));
	if (!tokens || !fieldnames) return -1;
	
	chunk = strtok_r(line, " ", &temp1);
	do {
		temp2 = strchr(chunk, '.');
		if (!temp2) continue;
		fieldname = chunk;
		*temp2 = '\0';
		fieldtype = (temp2+2); /* ignore the % */
		tokens[cur] = strdup(fieldtype);
		fieldnames[cur] = strdup(fieldname);
		cur++;
	} while ((chunk = strtok_r(NULL, " ", &temp1)));

	*tokens_dest = tokens;
	*fieldnames_dest = fieldnames;
	
	free(line);
	return found;
}

static void _free_string_list(char **booyah, int total) {
	int boomba = 0;
	while (boomba < total) {
		free(booyah[boomba]);
		boomba++;
	}
	free(booyah);
	return;
}

static void _free_result_rows(dbi_result_t *result) {
	int row_idx;

	for (row_idx = 0; row_idx <= result->numrows_matched; row_idx++) {
		if (result->rows[row_idx]) {
			for(row_idx = 0; row_idx < result->numfields ; row_idx++) {
				if (((result->field_types[row_idx] == DBI_TYPE_STRING) ||
						 (result->field_types[row_idx] == DBI_TYPE_ENUM) ||
						 (result->field_types[row_idx] == DBI_TYPE_SET)) &&
						(result->rows[row_idx]->field_values[row_idx].d_string)){
					free(result->rows[row_idx]->field_values[row_idx].d_string);
				}
			}	
			free(result->rows[row_idx]->field_values);
			free(result->rows[row_idx]->field_sizes);
			free(result->rows[row_idx]);
		}
	}		
	free(result->rows);
}

int dbi_result_get_fields(dbi_result Result, const char *format, ...) {
	dbi_result_t *result = Result;
	char **tokens;
	char **fieldnames;
	int curidx = 0;
	int numtokens = 0;
	va_list ap;

	if (!result) return -1;

	numtokens = _parse_field_formatstr(format, &tokens, &fieldnames);
	
	va_start(ap, format);
	while (curidx < numtokens) {
		switch (tokens[curidx][strlen(tokens[curidx])-1]) {
			case 'c': /* char */
				if (strlen(tokens[curidx]) > 1) {
					switch (tokens[curidx][0]) {
						case 'u': /* unsigned */
							*va_arg(ap, unsigned char *) = dbi_result_get_uchar(Result, fieldnames[curidx]);
							break;
					}
				}
				else {
					*va_arg(ap, char *) = dbi_result_get_char(Result, fieldnames[curidx]);
				}
				break;
			case 'h': /* sHort ("S"tring was taken) */
				if (strlen(tokens[curidx]) > 1) {
					switch (tokens[curidx][0]) {
						case 'u': /* unsigned */
							*va_arg(ap, unsigned short *) = dbi_result_get_ushort(Result, fieldnames[curidx]);
							break;
					}
				}
				else {
					*va_arg(ap, short *) = dbi_result_get_short(Result, fieldnames[curidx]);
				}
				break;
			case 'l': /* long (both l and i work) */
			case 'i':
				if (strlen(tokens[curidx]) > 1) {
					switch (tokens[curidx][0]) {
						case 'u': /* unsigned */
							*va_arg(ap, unsigned long *) = dbi_result_get_ulong(Result, fieldnames[curidx]);
							break;
					}
				}
				else {
					*va_arg(ap, long *) = dbi_result_get_long(Result, fieldnames[curidx]);
				}
				break;
			case 'L': /* long long */
				if (strlen(tokens[curidx]) > 1) {
					switch (tokens[curidx][0]) {
						case 'u': /* unsigned */
							*va_arg(ap, unsigned long long *) = dbi_result_get_ulonglong(Result, fieldnames[curidx]);
							break;
					}
				}
				else {
					*va_arg(ap, long long *) = dbi_result_get_longlong(Result, fieldnames[curidx]);
				}
				break;
			case 'f': /* float */
				*va_arg(ap, float *) = dbi_result_get_float(Result, fieldnames[curidx]); 
				break;
			case 'd': /* double */
				*va_arg(ap, double *) = dbi_result_get_double(Result, fieldnames[curidx]); 
				break;
			case 's': /* string */
				*va_arg(ap, const char **) = dbi_result_get_string(Result, fieldnames[curidx]); 
				break;
			case 'b': /* binary */
				*va_arg(ap, const unsigned char **) = dbi_result_get_binary(Result, fieldnames[curidx]); 
				break;
			case 'S': /* string copy */
				*va_arg(ap, char **) = dbi_result_get_string_copy(Result, fieldnames[curidx]); 
				break;
			case 'B': /* binary copy */
				*va_arg(ap, unsigned char **) = dbi_result_get_binary_copy(Result, fieldnames[curidx]); 
				break;
			case 't': /* seT */
				*va_arg(ap, const char **) = dbi_result_get_set(Result, fieldnames[curidx]); 
				break;
			case 'e': /* enum */
				*va_arg(ap, const char **) = dbi_result_get_enum(Result, fieldnames[curidx]); 
				break;
			case 'm': /* datetiMe (what... you have any better ideas?? */
				*va_arg(ap, time_t *) = dbi_result_get_datetime(Result, fieldnames[curidx]);
				break;
		}
		curidx++;
	}
	va_end(ap);

	_free_string_list(tokens, numtokens);
	_free_string_list(fieldnames, numtokens);
	return numtokens;
}

int dbi_result_bind_fields(dbi_result Result, const char *format, ...) {
	dbi_result_t *result = Result;
	char **tokens;
	char **fieldnames;
	int curidx = 0;
	int numtokens = 0;
	va_list ap;

	if (!result) return -1;

	numtokens = _parse_field_formatstr(format, &tokens, &fieldnames);
	
	va_start(ap, format);
	while (curidx < numtokens) {
		switch (tokens[curidx][strlen(tokens[curidx])-1]) {
			case 'c': /* char */
				if (strlen(tokens[curidx]) > 1) {
					switch (tokens[curidx][0]) {
						case 'u': /* unsigned */
							dbi_result_bind_uchar(Result, fieldnames[curidx], va_arg(ap, unsigned char *));
							break;
					}
				}
				else {
					dbi_result_bind_char(Result, fieldnames[curidx], va_arg(ap, char *));
				}
				break;
			case 'h': /* sHort ("S"tring was taken) */
				if (strlen(tokens[curidx]) > 1) {
					switch (tokens[curidx][0]) {
						case 'u': /* unsigned */
							dbi_result_bind_ushort(Result, fieldnames[curidx], va_arg(ap, unsigned short *));
							break;
					}
				}
				else {
					dbi_result_bind_short(Result, fieldnames[curidx], va_arg(ap, short *));
				}
				break;
			case 'l': /* long (both l and i work) */
			case 'i':
				if (strlen(tokens[curidx]) > 1) {
					switch (tokens[curidx][0]) {
						case 'u': /* unsigned */
							dbi_result_bind_ulong(Result, fieldnames[curidx], va_arg(ap, unsigned long *));
							break;
					}
				}
				else {
					dbi_result_bind_long(Result, fieldnames[curidx], va_arg(ap, long *));
				}
				break;
			case 'L': /* long long */
				if (strlen(tokens[curidx]) > 1) {
					switch (tokens[curidx][0]) {
						case 'u': /* unsigned */
							dbi_result_bind_ulonglong(Result, fieldnames[curidx], va_arg(ap, unsigned long long *));
							break;
					}
				}
				else {
					dbi_result_bind_longlong(Result, fieldnames[curidx], va_arg(ap, long long *));
				}
				break;
			case 'f': /* float */
				dbi_result_bind_float(Result, fieldnames[curidx], va_arg(ap, float *));
				break;
			case 'd': /* double */
				dbi_result_bind_double(Result, fieldnames[curidx], va_arg(ap, double *));
				break;
			case 's': /* string */
				dbi_result_bind_string(Result, fieldnames[curidx], va_arg(ap, const char **));
				break;
			case 'b': /* binary */
				dbi_result_bind_binary(Result, fieldnames[curidx], va_arg(ap, const unsigned char **));
				break;
			case 'S': /* string copy */
				dbi_result_bind_string_copy(Result, fieldnames[curidx], va_arg(ap, char **));
				break;
			case 'B': /* binary copy */
				dbi_result_bind_binary_copy(Result, fieldnames[curidx], va_arg(ap, unsigned char **));
				break;
			case 't': /* seT */
				dbi_result_bind_set(Result, fieldnames[curidx], va_arg(ap, const char **));
				break;
			case 'e': /* enum */
				dbi_result_bind_enum(Result, fieldnames[curidx], va_arg(ap, const char **));
				break;
			case 'm': /* datetiMe (what... you have any better ideas?? */
				dbi_result_bind_datetime(Result, fieldnames[curidx], va_arg(ap, time_t *));
				break;
		}
		curidx++;
	}
	va_end(ap);

	_free_string_list(tokens, numtokens);
	_free_string_list(fieldnames, numtokens);
	return numtokens;
}

/* RESULT: get_* functions */

signed char dbi_result_get_char(dbi_result Result, const char *fieldname) {
	signed char ERROR = 0;
	int idx = _find_field((dbi_result_t *)Result, fieldname);
	if (idx < 0) return ERROR;
	return dbi_result_get_char_idx(Result, idx+1);
}

signed char dbi_result_get_char_idx(dbi_result Result, unsigned int idx) {
	signed char ERROR = 0;
	dbi_result_t *result = Result;
	unsigned long sizeattrib;
	idx--;

	if (idx >= result->numfields) return ERROR;
	if (result->field_types[idx] != DBI_TYPE_INTEGER) return ERROR;
	if (result->rows[result->currowidx]->field_sizes[idx] == 0) return 0;
	sizeattrib = _isolate_attrib(result->field_attribs[idx], DBI_INTEGER_SIZE1, DBI_INTEGER_SIZE8);

	switch (sizeattrib) {
		case DBI_INTEGER_SIZE1:
			return result->rows[result->currowidx]->field_values[idx].d_char;
			break;
		case DBI_INTEGER_SIZE2:
		case DBI_INTEGER_SIZE3:
		case DBI_INTEGER_SIZE4:
		case DBI_INTEGER_SIZE8:
		default:
			return ERROR;
	}
}

short dbi_result_get_short(dbi_result Result, const char *fieldname) {
	short ERROR = 0;
	int idx = _find_field((dbi_result_t *)Result, fieldname);
	if (idx < 0) return ERROR;
	return dbi_result_get_short_idx(Result, idx+1);
}

short dbi_result_get_short_idx(dbi_result Result, unsigned int idx) {
	short ERROR = 0;
	dbi_result_t *result = Result;
	unsigned long sizeattrib;
	idx--;

	if (idx >= result->numfields) return ERROR;
	if (result->field_types[idx] != DBI_TYPE_INTEGER) return ERROR;
	if (result->rows[result->currowidx]->field_sizes[idx] == 0) return 0;
	sizeattrib = _isolate_attrib(result->field_attribs[idx], DBI_INTEGER_SIZE1, DBI_INTEGER_SIZE8);

	switch (sizeattrib) {
		case DBI_INTEGER_SIZE1:
		case DBI_INTEGER_SIZE2:
			return result->rows[result->currowidx]->field_values[idx].d_short;
			break;
		case DBI_INTEGER_SIZE3:
		case DBI_INTEGER_SIZE4:
		case DBI_INTEGER_SIZE8:
		default:
			return ERROR;
	}
}

long dbi_result_get_long(dbi_result Result, const char *fieldname) {
	long ERROR = 0;
	int idx = _find_field((dbi_result_t *)Result, fieldname);
	if (idx < 0) return ERROR;
	return dbi_result_get_long_idx(Result, idx+1);
}

long dbi_result_get_long_idx(dbi_result Result, unsigned int idx) {
	long ERROR = 0;
	dbi_result_t *result = Result;
	unsigned long sizeattrib;
	idx--;
	
	if (idx >= result->numfields) return ERROR;
	if (result->field_types[idx] != DBI_TYPE_INTEGER) return ERROR;
	if (result->rows[result->currowidx]->field_sizes[idx] == 0) return 0;
	sizeattrib = _isolate_attrib(result->field_attribs[idx], DBI_INTEGER_SIZE1, DBI_INTEGER_SIZE8);

	switch (sizeattrib) {
		case DBI_INTEGER_SIZE1:
		case DBI_INTEGER_SIZE2:
		case DBI_INTEGER_SIZE3:
		case DBI_INTEGER_SIZE4:
			return result->rows[result->currowidx]->field_values[idx].d_long;
			break;
		case DBI_INTEGER_SIZE8:
		default:
			return ERROR;
	}
}

long long dbi_result_get_longlong(dbi_result Result, const char *fieldname) {
	long long ERROR = 0;
	int idx = _find_field((dbi_result_t *)Result, fieldname);
	if (idx < 0) return ERROR;
	return dbi_result_get_longlong_idx(Result, idx+1);
}

long long dbi_result_get_longlong_idx(dbi_result Result, unsigned int idx) {
	long long ERROR = 0;
	dbi_result_t *result = Result;
	unsigned long sizeattrib;
	idx--;
	
	if (idx >= result->numfields) return ERROR;
	if (result->field_types[idx] != DBI_TYPE_INTEGER) return ERROR;
	if (result->rows[result->currowidx]->field_sizes[idx] == 0) return 0;
	sizeattrib = _isolate_attrib(result->field_attribs[idx], DBI_INTEGER_SIZE1, DBI_INTEGER_SIZE8);

	switch (sizeattrib) {
		case DBI_INTEGER_SIZE1:
		case DBI_INTEGER_SIZE2:
		case DBI_INTEGER_SIZE3:
		case DBI_INTEGER_SIZE4:
		case DBI_INTEGER_SIZE8:
			return result->rows[result->currowidx]->field_values[idx].d_longlong;
			break;
		default:
			return ERROR;
	}
}

unsigned char dbi_result_get_uchar(dbi_result Result, const char *fieldname) {
	return (unsigned char)dbi_result_get_char(Result, fieldname);
}

unsigned char dbi_result_get_uchar_idx(dbi_result Result, unsigned int idx) {
	return (unsigned char)dbi_result_get_char_idx(Result, idx);
}

unsigned short dbi_result_get_ushort(dbi_result Result, const char *fieldname) {
	return (unsigned short)dbi_result_get_short(Result, fieldname);
}

unsigned short dbi_result_get_ushort_idx(dbi_result Result, unsigned int idx) {
	return (unsigned short)dbi_result_get_short_idx(Result, idx);
}

unsigned long dbi_result_get_ulong(dbi_result Result, const char *fieldname) {
	return (unsigned long)dbi_result_get_long(Result, fieldname);
}

unsigned long dbi_result_get_ulong_idx(dbi_result Result, unsigned int idx) {
	return (unsigned long)dbi_result_get_long_idx(Result, idx);
}

unsigned long long dbi_result_get_ulonglong(dbi_result Result, const char *fieldname) {
	return (unsigned long long)dbi_result_get_longlong(Result, fieldname);
}

unsigned long long dbi_result_get_ulonglong_idx(dbi_result Result, unsigned int idx) {
	return (unsigned long long)dbi_result_get_longlong_idx(Result, idx);
}

float dbi_result_get_float(dbi_result Result, const char *fieldname) {
	float ERROR = 0.0;
	int idx = _find_field((dbi_result_t *)Result, fieldname);
	if (idx < 0) return ERROR;
	return dbi_result_get_float_idx(Result, idx+1);
}

float dbi_result_get_float_idx(dbi_result Result, unsigned int idx) {
	float ERROR = 0.0;
	dbi_result_t *result = Result;
	unsigned long sizeattrib;
	idx--;
	
	if (idx >= result->numfields) return ERROR;	
	if (result->field_types[idx] != DBI_TYPE_DECIMAL) return ERROR;
	if (result->rows[result->currowidx]->field_sizes[idx] == 0) return 0.0;
	sizeattrib = _isolate_attrib(result->field_attribs[idx], DBI_DECIMAL_SIZE4, DBI_DECIMAL_SIZE8);

	switch (sizeattrib) {
		case DBI_DECIMAL_SIZE4:
			return result->rows[result->currowidx]->field_values[idx].d_float;
			break;
		case DBI_DECIMAL_SIZE8:
		default:
			return ERROR;
	}
}

double dbi_result_get_double(dbi_result Result, const char *fieldname) {
	double ERROR = 0.0;
	int idx = _find_field((dbi_result_t *)Result, fieldname);
	if (idx < 0) return ERROR;
	return dbi_result_get_double_idx(Result, idx+1);
}
	
double dbi_result_get_double_idx(dbi_result Result, unsigned int idx) {
	double ERROR = 0.0;
	dbi_result_t *result = Result;
	unsigned long sizeattrib;
	idx--;
	
	if (idx >= result->numfields) return ERROR;
	if (result->field_types[idx] != DBI_TYPE_DECIMAL) return ERROR;
	if (result->rows[result->currowidx]->field_sizes[idx] == 0) return 0.0;
	sizeattrib = _isolate_attrib(result->field_attribs[idx], DBI_DECIMAL_SIZE4, DBI_DECIMAL_SIZE8);

	switch (sizeattrib) {
		case DBI_DECIMAL_SIZE4:
		case DBI_DECIMAL_SIZE8:
			return result->rows[result->currowidx]->field_values[idx].d_double;
			break;
		default:
			return ERROR;
	}
}

const char *dbi_result_get_string(dbi_result Result, const char *fieldname) {
	const char *ERROR = "ERROR";
	int idx = _find_field((dbi_result_t *)Result, fieldname);
	if (idx < 0) return ERROR;
	return dbi_result_get_string_idx(Result, idx+1);
}
	
const char *dbi_result_get_string_idx(dbi_result Result, unsigned int idx) {
	const char *ERROR = "ERROR";
	dbi_result_t *result = Result;
	idx--;
	
	if (idx >= result->numfields) return ERROR;
	if (result->field_types[idx] != DBI_TYPE_STRING) return ERROR;
	if (result->rows[result->currowidx]->field_sizes[idx] == 0) return NULL;
	
	return (const char *)(result->rows[result->currowidx]->field_values[idx].d_string);
}

const unsigned char *dbi_result_get_binary(dbi_result Result, const char *fieldname) {
	const unsigned char *ERROR = "ERROR";
	int idx = _find_field((dbi_result_t *)Result, fieldname);
	if (idx < 0) return ERROR;
	return dbi_result_get_binary_idx(Result, idx+1);
}
	
const unsigned char *dbi_result_get_binary_idx(dbi_result Result, unsigned int idx) {
	const unsigned char *ERROR = "ERROR";
	dbi_result_t *result = Result;
	idx--;
	
	if (idx >= result->numfields) return ERROR;
	if (result->field_types[idx] != DBI_TYPE_BINARY) return ERROR;
	if (result->rows[result->currowidx]->field_sizes[idx] == 0) return NULL;

	return (const unsigned char *)(result->rows[result->currowidx]->field_values[idx].d_string);
}

char *dbi_result_get_string_copy(dbi_result Result, const char *fieldname) {
	char *ERROR = "ERROR";
	int idx = _find_field((dbi_result_t *)Result, fieldname);
	if (idx < 0) return strdup(ERROR);
	return dbi_result_get_string_copy_idx(Result, idx+1);
}
	
char *dbi_result_get_string_copy_idx(dbi_result Result, unsigned int idx) {
	char *ERROR = "ERROR";
	char *newstring = NULL;
	dbi_result_t *result = Result;
	idx--;
	
	if (idx >= result->numfields) return strdup(ERROR);
	if (result->field_types[idx] != DBI_TYPE_STRING) return strdup(ERROR);
	if (result->rows[result->currowidx]->field_sizes[idx] == 0) return NULL;

	newstring = strdup(result->rows[result->currowidx]->field_values[idx].d_string);
	return newstring ? newstring : strdup(ERROR);
}

unsigned char *dbi_result_get_binary_copy(dbi_result Result, const char *fieldname) {
	unsigned char *ERROR = "ERROR";
	int idx = _find_field((dbi_result_t *)Result, fieldname);
	if (idx < 0) return strdup(ERROR);
	return dbi_result_get_binary_copy_idx(Result, idx+1);
}
	
unsigned char *dbi_result_get_binary_copy_idx(dbi_result Result, unsigned int idx) {
	unsigned char *ERROR = "ERROR";
	char *newblob = NULL;
	unsigned int size;
	dbi_result_t *result = Result;
	idx--;

	if (idx >= result->numfields) return strdup(ERROR);
	if (result->field_types[idx] != DBI_TYPE_BINARY) return strdup(ERROR);
	if (result->rows[result->currowidx]->field_sizes[idx] == 0) return NULL;

	size = dbi_result_get_field_size_idx(Result, idx);
	newblob = malloc(size);
	if (!newblob) return strdup(ERROR);
	memcpy(newblob, result->rows[result->currowidx]->field_values[idx].d_string, size);
	return newblob;
}

const char *dbi_result_get_enum(dbi_result Result, const char *fieldname) {
	const char *ERROR = "ERROR";
	int idx = _find_field((dbi_result_t *)Result, fieldname);
	if (idx < 0) return ERROR;
	return dbi_result_get_enum_idx(Result, idx+1);
}
	
const char *dbi_result_get_enum_idx(dbi_result Result, unsigned int idx) {
	const char *ERROR = "ERROR";
	dbi_result_t *result = Result;
	idx--;
	
	if (idx >= result->numfields) return ERROR;
	if (result->field_types[idx] != DBI_TYPE_ENUM) return ERROR;
	if (result->rows[result->currowidx]->field_sizes[idx] == 0) return NULL;

	return (const char *)(result->rows[result->currowidx]->field_values[idx].d_string);
}

const char *dbi_result_get_set(dbi_result Result, const char *fieldname) {
	const char *ERROR = "ERROR";
	int idx = _find_field((dbi_result_t *)Result, fieldname);
	if (idx < 0) return ERROR;
	return dbi_result_get_set_idx(Result, idx+1);
}
	
const char *dbi_result_get_set_idx(dbi_result Result, unsigned int idx) {
	const char *ERROR = "ERROR";
	dbi_result_t *result = Result;
	idx--;
	
	if (idx >= result->numfields) return ERROR;
	if (result->field_types[idx] != DBI_TYPE_SET) return ERROR;
	if (result->rows[result->currowidx]->field_sizes[idx] == 0) return NULL;

	return (const char *)(result->rows[result->currowidx]->field_values[idx].d_string);
}

time_t dbi_result_get_datetime(dbi_result Result, const char *fieldname) {
	time_t ERROR = 0;
	int idx = _find_field((dbi_result_t *)Result, fieldname);
	if (idx < 0) return ERROR;
	return dbi_result_get_datetime_idx(Result, idx+1);
}
	
time_t dbi_result_get_datetime_idx(dbi_result Result, unsigned int idx) {
	time_t ERROR = 0;
	dbi_result_t *result = Result;
	idx--;

	if (idx >= result->numfields) return ERROR;
	if (result->field_types[idx] != DBI_TYPE_DATETIME) return ERROR;
	if (result->rows[result->currowidx]->field_sizes[idx] == 0) return 0;
	
	return (time_t)(result->rows[result->currowidx]->field_values[idx].d_datetime);
}

/* RESULT: bind_* functions */

static int _setup_binding(dbi_result_t *result, const char *fieldname, void *bindto, void *helperfunc) {
	_field_binding_t *binding;
	if (!result || !fieldname) return -1;
	binding = _find_or_create_binding_node(result, fieldname);
	if (!binding) return -1;

	if (bindto == NULL) {
		_remove_binding_node(result, binding);
	}
	else {
		binding->bindto = bindto;
		binding->helper_function = (void*)(_field_binding_t *)helperfunc;
	}

	return 0;
}

static void _activate_bindings(dbi_result_t *result) {
	_field_binding_t *binding = result->field_bindings;
	void (*helper_function)(_field_binding_t *);

	while (binding) {
		helper_function = binding->helper_function;
		helper_function(binding);
		binding = binding->next;
	}
	return;
}

int dbi_result_bind_char(dbi_result Result, const char *fieldname, char *bindto) {
	return _setup_binding((dbi_result_t *)Result, fieldname, bindto, _bind_helper_char);
}

int dbi_result_bind_uchar(dbi_result Result, const char *fieldname, unsigned char *bindto) {
	return _setup_binding((dbi_result_t *)Result, fieldname, bindto, _bind_helper_uchar);
}

int dbi_result_bind_short(dbi_result Result, const char *fieldname, short *bindto) {
	return _setup_binding((dbi_result_t *)Result, fieldname, bindto, _bind_helper_short);
}

int dbi_result_bind_ushort(dbi_result Result, const char *fieldname, unsigned short *bindto) {
	return _setup_binding((dbi_result_t *)Result, fieldname, bindto, _bind_helper_ushort);
}

int dbi_result_bind_long(dbi_result Result, const char *fieldname, long *bindto) {
	return _setup_binding((dbi_result_t *)Result, fieldname, bindto, _bind_helper_long);
}

int dbi_result_bind_ulong(dbi_result Result, const char *fieldname, unsigned long *bindto) {
	return _setup_binding((dbi_result_t *)Result, fieldname, bindto, _bind_helper_ulong);
}

int dbi_result_bind_longlong(dbi_result Result, const char *fieldname, long long *bindto) {
	return _setup_binding((dbi_result_t *)Result, fieldname, bindto, _bind_helper_longlong);
}

int dbi_result_bind_ulonglong(dbi_result Result, const char *fieldname, unsigned long long *bindto) {
	return _setup_binding((dbi_result_t *)Result, fieldname, bindto, _bind_helper_ulonglong);
}

int dbi_result_bind_float(dbi_result Result, const char *fieldname, float *bindto) {
	return _setup_binding((dbi_result_t *)Result, fieldname, bindto, _bind_helper_float);
}

int dbi_result_bind_double(dbi_result Result, const char *fieldname, double *bindto) {
	return _setup_binding((dbi_result_t *)Result, fieldname, bindto, _bind_helper_double);
}

int dbi_result_bind_string(dbi_result Result, const char *fieldname, const char **bindto) {
	return _setup_binding((dbi_result_t *)Result, fieldname, (char **)bindto, _bind_helper_string);
}

int dbi_result_bind_binary(dbi_result Result, const char *fieldname, const unsigned char **bindto) {
	return _setup_binding((dbi_result_t *)Result, fieldname, (unsigned char **)bindto, _bind_helper_binary);
}

int dbi_result_bind_string_copy(dbi_result Result, const char *fieldname, char **bindto) {
	return _setup_binding((dbi_result_t *)Result, fieldname, bindto, _bind_helper_string_copy);
}

int dbi_result_bind_binary_copy(dbi_result Result, const char *fieldname, unsigned char **bindto) {
	return _setup_binding((dbi_result_t *)Result, fieldname, bindto, _bind_helper_binary_copy);
}

int dbi_result_bind_enum(dbi_result Result, const char *fieldname, const char **bindto) {
	return _setup_binding((dbi_result_t *)Result, fieldname, (char **)bindto, _bind_helper_enum);
}

int dbi_result_bind_set(dbi_result Result, const char *fieldname, const char **bindto) {
	return _setup_binding((dbi_result_t *)Result, fieldname, (char **)bindto, _bind_helper_set);
}

int dbi_result_bind_datetime(dbi_result Result, const char *fieldname, time_t *bindto) {
	return _setup_binding((dbi_result_t *)Result, fieldname, (time_t *)bindto, _bind_helper_datetime);
}

static _field_binding_t *_find_or_create_binding_node(dbi_result_t *result, const char *fieldname) {
	_field_binding_t *prevbinding = NULL;
	_field_binding_t *binding = result->field_bindings;

	while (binding && strcasecmp(fieldname, binding->fieldname)) {
		prevbinding = binding;
		binding = binding->next;
	}
	if (!binding) {
		/* allocate a new option node */
		binding = (_field_binding_t *) malloc(sizeof(_field_binding_t));
		if (!binding) {
			return NULL;
		}
		binding->result = result;
		binding->fieldname = strdup(fieldname);
		binding->next = NULL;
		if (result->field_bindings == NULL) {
		    result->field_bindings = binding;
		}
		else {
		    prevbinding->next = binding;
		}
	}

	return binding;
}

static void _remove_binding_node(dbi_result_t *result, _field_binding_t *deadbinding) {
	_field_binding_t *prevbinding = NULL;
	_field_binding_t *binding = result->field_bindings;

	while (binding && (binding != deadbinding)) {
		prevbinding = binding;
		binding = binding->next;
	}
	if (!binding) {
		/* this should never ever happen. silently pretend it never did. */
		return;
	}
	free((char *)deadbinding->fieldname);
	if (result->field_bindings == deadbinding) {
		result->field_bindings = deadbinding->next;
	}
	else {
		prevbinding->next = deadbinding->next;
	}
	free(deadbinding);
}

static int _find_field(dbi_result_t *result, const char *fieldname) {
	int i = 0;
	if (!result) return -1;
	while (i < result->numfields) {
		if (strcasecmp(result->field_names[i], fieldname) == 0) {
			return i;
		}
		i++;
	}
	return -1;
}

static int _is_row_fetched(dbi_result_t *result, unsigned int row) {
	if (!result->rows || (row >= result->numrows_matched)) return -1;
	return !(result->rows[row] == NULL);
}


/* PRIVATE: bind helpers */

static void _bind_helper_char(_field_binding_t *binding) {
	*(char *)binding->bindto = dbi_result_get_char((dbi_result)binding->result, binding->fieldname);
}

static void _bind_helper_uchar(_field_binding_t *binding) {
	*(unsigned char *)binding->bindto = dbi_result_get_uchar((dbi_result)binding->result, binding->fieldname);
}

static void _bind_helper_short(_field_binding_t *binding) {
	*(short *)binding->bindto = dbi_result_get_short((dbi_result)binding->result, binding->fieldname);
}

static void _bind_helper_ushort(_field_binding_t *binding) {
	*(unsigned short *)binding->bindto = dbi_result_get_ushort((dbi_result)binding->result, binding->fieldname);
}

static void _bind_helper_long(_field_binding_t *binding) {
	*(long *)binding->bindto = dbi_result_get_long((dbi_result)binding->result, binding->fieldname);
}

static void _bind_helper_ulong(_field_binding_t *binding) {
	*(unsigned long *)binding->bindto = dbi_result_get_ulong((dbi_result)binding->result, binding->fieldname);
}

static void _bind_helper_longlong(_field_binding_t *binding) {
	*(long long *)binding->bindto = dbi_result_get_longlong((dbi_result)binding->result, binding->fieldname);
}

static void _bind_helper_ulonglong(_field_binding_t *binding) {
	*(unsigned long long *)binding->bindto = dbi_result_get_ulonglong((dbi_result)binding->result, binding->fieldname);
}

static void _bind_helper_float(_field_binding_t *binding) {
	*(float *)binding->bindto = dbi_result_get_float((dbi_result)binding->result, binding->fieldname);
}

static void _bind_helper_double(_field_binding_t *binding) {
	*(double *)binding->bindto = dbi_result_get_double((dbi_result)binding->result, binding->fieldname);
}

static void _bind_helper_string(_field_binding_t *binding) {
	*(const char **)binding->bindto = dbi_result_get_string((dbi_result)binding->result, binding->fieldname);
}

static void _bind_helper_binary(_field_binding_t *binding) {
	*(const unsigned char **)binding->bindto = dbi_result_get_binary((dbi_result)binding->result, binding->fieldname);
}

static void _bind_helper_string_copy(_field_binding_t *binding) {
	*(char **)binding->bindto = dbi_result_get_string_copy((dbi_result)binding->result, binding->fieldname);
}

static void _bind_helper_binary_copy(_field_binding_t *binding) {
	*(unsigned char **)binding->bindto = dbi_result_get_binary_copy((dbi_result)binding->result, binding->fieldname);
}

static void _bind_helper_set(_field_binding_t *binding) {
	*(const char **)binding->bindto = dbi_result_get_set((dbi_result)binding->result, binding->fieldname);
}

static void _bind_helper_enum(_field_binding_t *binding) {
	*(const char **)binding->bindto = dbi_result_get_enum((dbi_result)binding->result, binding->fieldname);
}

static void _bind_helper_datetime(_field_binding_t *binding) {
	*(time_t *)binding->bindto = dbi_result_get_datetime((dbi_result)binding->result, binding->fieldname);
}

