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

extern void _error_handler(dbi_conn_t *conn, dbi_error_flag errflag);

/* declarations for internal functions -- anything declared as static won't be accessible by name from client programs */
static _field_binding_t *_find_or_create_binding_node(dbi_result_t *result, const char *fieldname);
static void _remove_binding_node(dbi_result_t *result, _field_binding_t *deadbinding);
static unsigned int _find_field(dbi_result_t *result, const char *fieldname, dbi_error_flag* errflag);
static int _is_row_fetched(dbi_result_t *result, unsigned long long row);
static int _setup_binding(dbi_result_t *result, const char *fieldname, void *bindto, void *helperfunc);
static void _activate_bindings(dbi_result_t *result);
static unsigned int _parse_field_formatstr(const char *format, char ***tokens_dest, char ***fieldnames_dest);
static void _free_string_list(char **booyah, int total);
static void _free_result_rows(dbi_result_t *result);
int _disjoin_from_conn(dbi_result_t *result);

static void _bind_helper_char(_field_binding_t *binding);
static void _bind_helper_uchar(_field_binding_t *binding);
static void _bind_helper_short(_field_binding_t *binding);
static void _bind_helper_ushort(_field_binding_t *binding);
static void _bind_helper_int(_field_binding_t *binding);
static void _bind_helper_uint(_field_binding_t *binding);
static void _bind_helper_longlong(_field_binding_t *binding);
static void _bind_helper_ulonglong(_field_binding_t *binding);
static void _bind_helper_float(_field_binding_t *binding);
static void _bind_helper_double(_field_binding_t *binding);
static void _bind_helper_string(_field_binding_t *binding);
static void _bind_helper_binary(_field_binding_t *binding);
static void _bind_helper_string_copy(_field_binding_t *binding);
static void _bind_helper_binary_copy(_field_binding_t *binding);
static void _bind_helper_datetime(_field_binding_t *binding);

/* XXX ROW SEEKING AND FETCHING XXX */

/* returns 1 if ok, 0 on error */
int dbi_result_seek_row(dbi_result Result, unsigned long long rowidx) {
  dbi_result_t *result = Result;
  int retval;

  if (!result) {
    _error_handler(result->conn, DBI_ERROR_BADPTR);
    return 0;
  }	

  if ((result->result_state == NOTHING_RETURNED) || (rowidx <= 0) || (rowidx > result->numrows_matched)) {
    _error_handler(result->conn, DBI_ERROR_BADIDX);
    return 0;
  }

  if (_is_row_fetched(result, rowidx) == 1) {
    /* jump right to it */
    result->currowidx = rowidx;
    _activate_bindings(result);
    return 1;
  }
	
  /* row is one-based for the user, but zero-based to the dbd conn */
  retval = result->conn->driver->functions->goto_row(result, rowidx-1);
  if (retval == -1) {
    _error_handler(result->conn, DBI_ERROR_DBD);
    return 0;
  }
  retval = result->conn->driver->functions->fetch_row(result, rowidx-1);
  if (retval == 0) {
    _error_handler(result->conn, DBI_ERROR_DBD);
    return 0;
  }

  result->currowidx = rowidx;
  _activate_bindings(result);
  return retval;
}

int dbi_result_first_row(dbi_result Result) {
  return dbi_result_seek_row(Result, 1);
}

int dbi_result_last_row(dbi_result Result) {
  return dbi_result_seek_row(Result, dbi_result_get_numrows(Result));
}

int dbi_result_has_prev_row(dbi_result Result) {
  dbi_result_t *result = Result;
  if (!result) {
    _error_handler(result->conn, DBI_ERROR_BADPTR);
    return 0;
  }	

  if ((result->result_state == NOTHING_RETURNED) || (result->currowidx <= 1)) {
    return 0;
  }
  return 1;
}

int dbi_result_prev_row(dbi_result Result) {
  dbi_result_t *result = Result;
  if (!result) {
    _error_handler(result->conn, DBI_ERROR_BADPTR);
    return 0;
  }	

  if (!dbi_result_has_prev_row(Result)) {
    _error_handler(result->conn, DBI_ERROR_BADIDX);
    return 0;
  }
  return dbi_result_seek_row(Result, result->currowidx-1);
}

int dbi_result_has_next_row(dbi_result Result) {
  dbi_result_t *result = Result;
  if (!result) {
    _error_handler(result->conn, DBI_ERROR_BADPTR);
    return 0;
  }	

  if ((result->result_state == NOTHING_RETURNED) || (result->currowidx >= dbi_result_get_numrows(Result))) {
    return 0;
  }
  return 1;
}

int dbi_result_next_row(dbi_result Result) {
  dbi_result_t *result = Result;
  if (!result) {
    _error_handler(result->conn, DBI_ERROR_BADPTR);
    return 0;
  }	

  if (!dbi_result_has_next_row(Result)) {
    _error_handler(result->conn, DBI_ERROR_BADIDX);
    return 0;
  }
  return dbi_result_seek_row(Result, result->currowidx+1);
}

unsigned long long dbi_result_get_currow(dbi_result Result) {
  dbi_result_t *result = Result;
  if (!result) {
    _error_handler(result->conn, DBI_ERROR_BADPTR);
    return 0;
  }	

  return result->currowidx;
}

unsigned long long dbi_result_get_numrows(dbi_result Result) {
  dbi_result_t *result = Result;
  if (!result) {
    _error_handler(result->conn, DBI_ERROR_BADPTR);
    return DBI_ROW_ERROR;
  }	

  return result->numrows_matched;
}

unsigned long long dbi_result_get_numrows_affected(dbi_result Result) {
  dbi_result_t *result = Result;
  if (!result) {
    _error_handler(result->conn, DBI_ERROR_BADPTR);
    return DBI_ROW_ERROR;
  }	

  return result->numrows_affected;
}

/* returns the length of the string or binary, excluding a trailing \0 */
size_t dbi_result_get_field_length(dbi_result Result, const char *fieldname) {
  dbi_result_t *result = Result;
  unsigned int fieldidx = 0;
  dbi_error_flag errflag;

  if (!result) {
    _error_handler(result->conn, DBI_ERROR_BADPTR);
    return DBI_LENGTH_ERROR;
  }

  fieldidx = _find_field(result, fieldname, &errflag);
  if (errflag != DBI_ERROR_NONE) {
    _error_handler(result->conn, errflag);
    return DBI_LENGTH_ERROR;
  }
  
  return dbi_result_get_field_length_idx(Result, fieldidx+1);
}

size_t dbi_result_get_field_length_idx(dbi_result Result, unsigned int fieldidx) {
  dbi_result_t *result = Result;
  unsigned long long currowidx;
  
  /* user-visible indexes start at 1 */
  fieldidx--;
  
  if (!result || !result->rows) {
    _error_handler(result->conn, DBI_ERROR_BADPTR);
    return DBI_LENGTH_ERROR;
  }

  currowidx = result->currowidx;
  if (!result->rows[currowidx] || !result->rows[currowidx]->field_sizes) {
    _error_handler(result->conn, DBI_ERROR_BADOBJECT);
    return DBI_LENGTH_ERROR;
  }
  if (fieldidx >= result->numfields) {
    _error_handler(result->conn, DBI_ERROR_BADIDX);
    return DBI_LENGTH_ERROR;
  }
  
  return result->rows[currowidx]->field_sizes[fieldidx];
}

/* the "field_size" functions are merely kept to support legacy
   code. Their purpose was, er, unclear, their implementations were,
   er, broke, and the name "field_length" is more appropriate to what
   these functions do anyway. Support for the "field_size" functions
   is likely to be dropped in the next release */
size_t dbi_result_get_field_size(dbi_result Result, const char *fieldname) {
  return dbi_result_get_field_length(Result, fieldname);
}

size_t dbi_result_get_field_size_idx(dbi_result Result, unsigned int fieldidx) {
  return dbi_result_get_field_length_idx(Result, fieldidx);
}

unsigned int dbi_result_get_field_idx(dbi_result Result, const char *fieldname) {
  dbi_result_t *result = Result;
  unsigned int fieldidx = 0;
  dbi_error_flag errflag;

  if (!result) {
    _error_handler(result->conn, DBI_ERROR_BADPTR);
    return 0;
  }	

  /* user-visible indexes start at 1 */
  fieldidx = _find_field(result, fieldname, &errflag)+1;
  if (errflag != DBI_ERROR_NONE) {
    _error_handler(result->conn, errflag);
    return 0;
  }
  
  return fieldidx;
}

const char *dbi_result_get_field_name(dbi_result Result, unsigned int fieldidx) {
  dbi_result_t *result = Result;
	
  if (!result) {
    _error_handler(result->conn, DBI_ERROR_BADPTR);
    return NULL;
  }

  else if (fieldidx > result->numfields) {
    _error_handler(result->conn, DBI_ERROR_BADIDX);
    return NULL;
  }
  else if (result->field_names == NULL) {
    _error_handler(result->conn, DBI_ERROR_BADOBJECT);
    return NULL;
  }
	
  return (const char *) result->field_names[fieldidx-1];
}

unsigned int dbi_result_get_numfields(dbi_result Result) {
  dbi_result_t *result = Result;
  if (!result) return DBI_FIELD_ERROR;
  return result->numfields;
}

unsigned short dbi_result_get_field_type(dbi_result Result, const char *fieldname) {
  dbi_result_t *result = Result;
  unsigned int fieldidx = 0;
  dbi_error_flag errflag;
	
  if (!result) {
    _error_handler(result->conn, DBI_ERROR_BADPTR);
    return DBI_TYPE_ERROR;
  }

  fieldidx = _find_field(result, fieldname, &errflag);
  if (errflag != DBI_ERROR_NONE) {
    _error_handler(result->conn, errflag);
    return DBI_TYPE_ERROR;
  }

  return dbi_result_get_field_type_idx(Result, fieldidx+1);
}

unsigned short dbi_result_get_field_type_idx(dbi_result Result, unsigned int fieldidx) {
  dbi_result_t *result = Result;
  fieldidx--;
	
  if (!result) {
    _error_handler(result->conn, DBI_ERROR_BADPTR);
    return DBI_TYPE_ERROR;
  }

  else if (!result->field_types) {
    _error_handler(result->conn, DBI_ERROR_BADOBJECT);
    return DBI_TYPE_ERROR;
  }
  else if (fieldidx >= result->numfields) {
    _error_handler(result->conn, DBI_ERROR_BADIDX);
    return DBI_TYPE_ERROR;
  }

  return result->field_types[fieldidx];
}

unsigned int dbi_result_get_field_attrib(dbi_result Result, const char *fieldname, unsigned int attribmin, unsigned int attribmax) {
  dbi_result_t *result = Result;
  unsigned int fieldidx = 0;
  dbi_error_flag errflag;
	
  if (!result) {
    _error_handler(result->conn, DBI_ERROR_BADPTR);
    return DBI_ATTRIBUTE_ERROR;
  }

  fieldidx = _find_field(result, fieldname, &errflag);
  if (errflag != DBI_ERROR_NONE) {
    _error_handler(result->conn, errflag);
    return DBI_ATTRIBUTE_ERROR;
  }

  return dbi_result_get_field_attrib_idx(Result, fieldidx+1, attribmin, attribmax);
}

unsigned int dbi_result_get_field_attrib_idx(dbi_result Result, unsigned int fieldidx, unsigned int attribmin, unsigned int attribmax) {
  dbi_result_t *result = Result;
  fieldidx--;
	
  if (!result) {
    _error_handler(result->conn, DBI_ERROR_BADPTR);
    return DBI_ATTRIBUTE_ERROR;
  }
  else if (!result->field_attribs) {
    _error_handler(result->conn, DBI_ERROR_BADOBJECT);
    return DBI_ATTRIBUTE_ERROR;
  }
  else if (fieldidx >= result->numfields) {
    _error_handler(result->conn, DBI_ERROR_BADIDX);
    return DBI_ATTRIBUTE_ERROR;
  }

  return _isolate_attrib(result->field_attribs[fieldidx], attribmin, attribmax);
}

unsigned int dbi_result_get_field_attribs(dbi_result Result, const char *fieldname) {
  dbi_result_t *result = Result;
  unsigned int fieldidx = 0;
  dbi_error_flag errflag;
	
  if (!result) {
    _error_handler(result->conn, DBI_ERROR_BADPTR);
    return DBI_ATTRIBUTE_ERROR;
  }

  fieldidx = _find_field(result, fieldname, &errflag);
  if (errflag != DBI_ERROR_NONE) {
    _error_handler(result->conn, errflag);
    return DBI_ATTRIBUTE_ERROR;
  }

  return dbi_result_get_field_attribs_idx(Result, fieldidx+1);
}

unsigned int dbi_result_get_field_attribs_idx(dbi_result Result, unsigned int fieldidx) {
  dbi_result_t *result = Result;
  fieldidx--;
	
  if (!result) {
    _error_handler(result->conn, DBI_ERROR_BADPTR);
    return DBI_ATTRIBUTE_ERROR;
  }
  else if (!result->field_attribs) {
    _error_handler(result->conn, DBI_ERROR_BADOBJECT);
    return DBI_ATTRIBUTE_ERROR;
  }
  else if (fieldidx >= result->numfields) {
    _error_handler(result->conn, DBI_ERROR_BADIDX);
    return DBI_ATTRIBUTE_ERROR;
  }

  return result->field_attribs[fieldidx];
}

void _set_field_flag(dbi_row_t *row, unsigned int fieldidx, unsigned char flag, unsigned char value) {
  unsigned char *flags = &row->field_flags[fieldidx];
  *flags &= ~flag; // set that bit to 0
  if (value) {
    *flags |= flag; // if value = 1, set the flag
  }
}

int _get_field_flag(dbi_row_t *row, unsigned int fieldidx, unsigned char flag) {
  return (row->field_flags[fieldidx] & flag) ? 1 : 0;
}

int dbi_result_field_is_null(dbi_result Result, const char *fieldname) {
  dbi_result_t *result = Result;
  unsigned int fieldidx = 0;
  dbi_error_flag errflag;

  if (!result) {
    _error_handler(result->conn, DBI_ERROR_BADPTR);
    return DBI_FIELD_FLAG_ERROR;
  }

  fieldidx = _find_field(result, fieldname, &errflag);
  if (errflag != DBI_ERROR_NONE) {
    _error_handler(result->conn, errflag);
    return DBI_FIELD_FLAG_ERROR;
  }

  return dbi_result_field_is_null_idx(Result, fieldidx+1);
}

int dbi_result_field_is_null_idx(dbi_result Result, unsigned int fieldidx) {
  dbi_result_t *result = Result;
  unsigned long long currowidx;

  fieldidx--;
	
  if (!result || !result->rows) {
    _error_handler(result->conn, DBI_ERROR_BADPTR);
    return DBI_FIELD_FLAG_ERROR;
  }

  currowidx = result->currowidx;
  if (!result->rows[currowidx] || !result->rows[currowidx]->field_flags) {
    _error_handler(result->conn, DBI_ERROR_BADOBJECT);
    return DBI_FIELD_FLAG_ERROR;
  }
  if (fieldidx >= result->numfields) {
    _error_handler(result->conn, DBI_ERROR_BADIDX);
    return DBI_FIELD_FLAG_ERROR;
  }

  return _get_field_flag(result->rows[currowidx], fieldidx, DBI_VALUE_NULL);
}

int _disjoin_from_conn(dbi_result_t *result) {
  /* todo: is int enough? */
  int idx;
  int found = -1;
  int retval;

  retval = result->conn->driver->functions->free_query(result);

  for (idx = 0; idx < result->conn->results_used; idx++) {
    if (found < 0) {
      /* keep looking */
      if (result->conn->results[idx] == result) {
	found = idx;
	result->conn->results[idx] = NULL;
      }
    }
    else {
      /* already found, shift remaining elements back one */
      result->conn->results[idx-1] = result->conn->results[idx];
    }
  }
  if (found >= 0) {
    result->conn->results[result->conn->results_used-1] = NULL;
    result->conn->results_used--;
  }

  result->conn = NULL;
	
  return retval;
}

int dbi_result_disjoin(dbi_result Result) {
  dbi_result_t *result = Result;
  if (!result) return -1;
  return _disjoin_from_conn(result);
}

int dbi_result_free(dbi_result Result) {
  dbi_result_t *result = Result;
  int retval = 0;

  if (!result) return -1;
	
  if (result->conn) {
    retval = _disjoin_from_conn(result);
  }

  while (result->field_bindings) {
    _remove_binding_node(result, result->field_bindings);
  }
	
  if (result->rows) {
    _free_result_rows(result);
  }
		
  if (result->numfields) {
    _free_string_list(result->field_names, result->numfields);
    free(result->field_types);
    free(result->field_attribs);
  }

  if (retval == -1) {
    _error_handler(result->conn, DBI_ERROR_DBD);
  }

  free(result);
  return retval;
}

dbi_conn dbi_result_get_conn(dbi_result Result) {
  dbi_result_t *result = Result;

  if (!result) {
    _error_handler(result->conn, DBI_ERROR_BADPTR);
    return NULL;
  }

  return result->conn;
}

/* RESULT: mass retrieval functions */

static unsigned int _parse_field_formatstr(const char *format, char ***tokens_dest, char ***fieldnames_dest) {
  unsigned int found = 0;
  unsigned int cur = 0;
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
  if (!tokens || !fieldnames) return DBI_FIELD_ERROR;
	
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
  if (booyah == NULL) return;
  while (boomba < total) {
    if (booyah[boomba]) free(booyah[boomba]);
    boomba++;
  }
  free(booyah);
  return;
}

static void _free_result_rows(dbi_result_t *result) {
  unsigned long long rowidx = 0;
  unsigned int fieldidx = 0;

  for (rowidx = 0; rowidx <= result->numrows_matched; rowidx++) {
    if (!result->rows[rowidx]) continue;
			
    for (fieldidx = 0; fieldidx < result->numfields; fieldidx++) {
      if (((result->field_types[fieldidx] == DBI_TYPE_STRING) || (result->field_types[fieldidx] == DBI_TYPE_BINARY)) && result->rows[rowidx]->field_values[fieldidx].d_string) {
	free(result->rows[rowidx]->field_values[fieldidx].d_string);
      }
    }
		
    free(result->rows[rowidx]->field_values);
    free(result->rows[rowidx]->field_sizes);
    free(result->rows[rowidx]->field_flags);
    free(result->rows[rowidx]);
  }
	
  free(result->rows);
}

unsigned int dbi_result_get_fields(dbi_result Result, const char *format, ...) {
  dbi_result_t *result = Result;
  char **tokens;
  char **fieldnames;
  unsigned int curidx = 0;
  unsigned int numtokens = 0;
  va_list ap;

  if (!result) return DBI_FIELD_ERROR;

  numtokens = _parse_field_formatstr(format, &tokens, &fieldnames);

  if (numtokens == DBI_FIELD_ERROR) {
    return numtokens;
  }
	
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
    case 'l': /* 4-byte integer (both l and i work) */
    case 'i':
      if (strlen(tokens[curidx]) > 1) {
	switch (tokens[curidx][0]) {
	case 'u': /* unsigned */
	  *va_arg(ap, unsigned int *) = dbi_result_get_uint(Result, fieldnames[curidx]);
	  break;
	}
      }
      else {
	*va_arg(ap, int *) = dbi_result_get_int(Result, fieldnames[curidx]);
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

unsigned int dbi_result_bind_fields(dbi_result Result, const char *format, ...) {
  dbi_result_t *result = Result;
  char **tokens;
  char **fieldnames;
  unsigned int curidx = 0;
  unsigned int numtokens = 0;
  va_list ap;

  if (!result) return DBI_FIELD_ERROR;

  numtokens = _parse_field_formatstr(format, &tokens, &fieldnames);

  if (numtokens == DBI_FIELD_ERROR) {
    return numtokens;
  }

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
    case 'l': /* 4-byte integer (both l and i work) */
    case 'i':
      if (strlen(tokens[curidx]) > 1) {
	switch (tokens[curidx][0]) {
	case 'u': /* unsigned */
	  dbi_result_bind_uint(Result, fieldnames[curidx], va_arg(ap, unsigned int *));
	  break;
	}
      }
      else {
	dbi_result_bind_int(Result, fieldnames[curidx], va_arg(ap, int *));
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
  unsigned int fieldidx;
  dbi_error_flag errflag;

  fieldidx = _find_field((dbi_result_t *)Result, fieldname, &errflag);
  if (errflag != DBI_ERROR_NONE) {
    dbi_conn_t *conn = ((dbi_result_t *)Result)->conn;
    _error_handler(conn, errflag);
    return ERROR;
  }
  return dbi_result_get_char_idx(Result, fieldidx+1);
}

signed char dbi_result_get_char_idx(dbi_result Result, unsigned int fieldidx) {
  signed char ERROR = 0;
  dbi_result_t *result = Result;
  unsigned long sizeattrib;

  fieldidx--;

  result->conn->error_flag = DBI_ERROR_NONE;

  if (fieldidx >= result->numfields) {
    _error_handler(result->conn, DBI_ERROR_BADIDX);
    return ERROR;
  }
  if (result->field_types[fieldidx] != DBI_TYPE_INTEGER) {
    _error_handler(result->conn, DBI_ERROR_BADTYPE);
    return ERROR;
  }
  sizeattrib = _isolate_attrib(result->field_attribs[fieldidx], DBI_INTEGER_SIZE1, DBI_INTEGER_SIZE8);

  switch (sizeattrib) {
  case DBI_INTEGER_SIZE1:
    return result->rows[result->currowidx]->field_values[fieldidx].d_char;
    break;
  case DBI_INTEGER_SIZE2:
  case DBI_INTEGER_SIZE3:
  case DBI_INTEGER_SIZE4:
  case DBI_INTEGER_SIZE8:
  default:
    _error_handler(result->conn, DBI_ERROR_BADTYPE);
    return ERROR;
  }
}

short dbi_result_get_short(dbi_result Result, const char *fieldname) {
  short ERROR = 0;
  unsigned int fieldidx;
  dbi_error_flag errflag;

  fieldidx = _find_field((dbi_result_t *)Result, fieldname, &errflag);
  if (errflag != DBI_ERROR_NONE) {
    dbi_conn_t *conn = ((dbi_result_t *)Result)->conn;
    _error_handler(conn, DBI_ERROR_BADNAME);
    return ERROR;
  }
  return dbi_result_get_short_idx(Result, fieldidx+1);
}

short dbi_result_get_short_idx(dbi_result Result, unsigned int fieldidx) {
  short ERROR = 0;
  dbi_result_t *result = Result;
  unsigned long sizeattrib;
  fieldidx--;

  result->conn->error_flag = DBI_ERROR_NONE;

  if (fieldidx >= result->numfields) {
    _error_handler(result->conn, DBI_ERROR_BADIDX);
    return ERROR;
  }
  if (result->field_types[fieldidx] != DBI_TYPE_INTEGER) {
    _error_handler(result->conn, DBI_ERROR_BADTYPE);
    return ERROR;
  }
  sizeattrib = _isolate_attrib(result->field_attribs[fieldidx], DBI_INTEGER_SIZE1, DBI_INTEGER_SIZE8);

  switch (sizeattrib) {
  case DBI_INTEGER_SIZE1:
  case DBI_INTEGER_SIZE2:
    return result->rows[result->currowidx]->field_values[fieldidx].d_short;
    break;
  case DBI_INTEGER_SIZE3:
  case DBI_INTEGER_SIZE4:
  case DBI_INTEGER_SIZE8:
  default:
    _error_handler(result->conn, DBI_ERROR_BADTYPE);
    return ERROR;
  }
}

int dbi_result_get_long(dbi_result Result, const char *fieldname) {
  return dbi_result_get_int(Result, fieldname);
}

int dbi_result_get_int(dbi_result Result, const char *fieldname) {
  long ERROR = 0;
  unsigned int fieldidx;
  dbi_error_flag errflag;

  fieldidx = _find_field((dbi_result_t *)Result, fieldname, &errflag);
  if (errflag != DBI_ERROR_NONE) {
    dbi_conn_t *conn = ((dbi_result_t *)Result)->conn;
    _error_handler(conn, DBI_ERROR_BADNAME);
    return ERROR;
  }
  return dbi_result_get_long_idx(Result, fieldidx+1);
}

int dbi_result_get_long_idx(dbi_result Result, unsigned int fieldidx) {
  return dbi_result_get_int_idx(Result, fieldidx);
}

int dbi_result_get_int_idx(dbi_result Result, unsigned int fieldidx) {
  long ERROR = 0;
  dbi_result_t *result = Result;
  unsigned long sizeattrib;
  fieldidx--;
	
  result->conn->error_flag = DBI_ERROR_NONE;

  if (fieldidx >= result->numfields) {
    _error_handler(result->conn, DBI_ERROR_BADIDX);
    return ERROR;
  }
  if (result->field_types[fieldidx] != DBI_TYPE_INTEGER) {
    _error_handler(result->conn, DBI_ERROR_BADTYPE);
    return ERROR;
  }
  sizeattrib = _isolate_attrib(result->field_attribs[fieldidx], DBI_INTEGER_SIZE1, DBI_INTEGER_SIZE8);

  switch (sizeattrib) {
  case DBI_INTEGER_SIZE1:
  case DBI_INTEGER_SIZE2:
  case DBI_INTEGER_SIZE3:
  case DBI_INTEGER_SIZE4:
    return result->rows[result->currowidx]->field_values[fieldidx].d_long;
    break;
  case DBI_INTEGER_SIZE8:
  default:
    _error_handler(result->conn, DBI_ERROR_BADTYPE);
    return ERROR;
  }
}

long long dbi_result_get_longlong(dbi_result Result, const char *fieldname) {
  long long ERROR = 0;
  unsigned int fieldidx;
  dbi_error_flag errflag;
    
  fieldidx = _find_field((dbi_result_t *)Result, fieldname, &errflag);
  if (errflag != DBI_ERROR_NONE) {
    dbi_conn_t *conn = ((dbi_result_t *)Result)->conn;
    _error_handler(conn, DBI_ERROR_BADNAME);
    return ERROR;
  }
  return dbi_result_get_longlong_idx(Result, fieldidx+1);
}

long long dbi_result_get_longlong_idx(dbi_result Result, unsigned int fieldidx) {
  long long ERROR = 0;
  dbi_result_t *result = Result;
  unsigned long sizeattrib;
  fieldidx--;
	
  result->conn->error_flag = DBI_ERROR_NONE;

  if (fieldidx >= result->numfields) {
    _error_handler(result->conn, DBI_ERROR_BADIDX);
    return ERROR;
  }
  if (result->field_types[fieldidx] != DBI_TYPE_INTEGER) {
    _error_handler(result->conn, DBI_ERROR_BADTYPE);
    return ERROR;
  }
  sizeattrib = _isolate_attrib(result->field_attribs[fieldidx], DBI_INTEGER_SIZE1, DBI_INTEGER_SIZE8);

  switch (sizeattrib) {
  case DBI_INTEGER_SIZE1:
  case DBI_INTEGER_SIZE2:
  case DBI_INTEGER_SIZE3:
  case DBI_INTEGER_SIZE4:
  case DBI_INTEGER_SIZE8:
    return result->rows[result->currowidx]->field_values[fieldidx].d_longlong;
    break;
  default:
    _error_handler(result->conn, DBI_ERROR_BADTYPE);
    return ERROR;
  }
}

unsigned char dbi_result_get_uchar(dbi_result Result, const char *fieldname) {
  return (unsigned char)dbi_result_get_char(Result, fieldname);
}

unsigned char dbi_result_get_uchar_idx(dbi_result Result, unsigned int fieldidx) {
  return (unsigned char)dbi_result_get_char_idx(Result, fieldidx);
}

unsigned short dbi_result_get_ushort(dbi_result Result, const char *fieldname) {
  return (unsigned short)dbi_result_get_short(Result, fieldname);
}

unsigned short dbi_result_get_ushort_idx(dbi_result Result, unsigned int fieldidx) {
  return (unsigned short)dbi_result_get_short_idx(Result, fieldidx);
}

unsigned int dbi_result_get_uint(dbi_result Result, const char *fieldname) {
  return (unsigned int)dbi_result_get_int(Result, fieldname);
}

unsigned int dbi_result_get_ulong(dbi_result Result, const char *fieldname) {
  return dbi_result_get_uint(Result, fieldname);
}

unsigned int dbi_result_get_uint_idx(dbi_result Result, unsigned int fieldidx) {
  return (unsigned int)dbi_result_get_int_idx(Result, fieldidx);
}

unsigned int dbi_result_get_ulong_idx(dbi_result Result, unsigned int fieldidx) {
  return dbi_result_get_uint_idx(Result, fieldidx);
}

unsigned long long dbi_result_get_ulonglong(dbi_result Result, const char *fieldname) {
  return (unsigned long long)dbi_result_get_longlong(Result, fieldname);
}

unsigned long long dbi_result_get_ulonglong_idx(dbi_result Result, unsigned int fieldidx) {
  return (unsigned long long)dbi_result_get_longlong_idx(Result, fieldidx);
}

float dbi_result_get_float(dbi_result Result, const char *fieldname) {
  float ERROR = 0.0;
  dbi_error_flag errflag;
  unsigned int fieldidx;

  fieldidx = _find_field((dbi_result_t *)Result, fieldname, &errflag);
  if (errflag != DBI_ERROR_NONE) {
    dbi_conn_t *conn = ((dbi_result_t *)Result)->conn;
    _error_handler(conn, DBI_ERROR_BADNAME);
    return ERROR;
  }
  return dbi_result_get_float_idx(Result, fieldidx+1);
}

float dbi_result_get_float_idx(dbi_result Result, unsigned int fieldidx) {
  float ERROR = 0.0;
  dbi_result_t *result = Result;
  unsigned long sizeattrib;
  fieldidx--;
	
  result->conn->error_flag = DBI_ERROR_NONE;
	
  if (fieldidx >= result->numfields) {
    _error_handler(result->conn, DBI_ERROR_BADIDX);
    return ERROR;
  }
  if (result->field_types[fieldidx] != DBI_TYPE_DECIMAL) {
    _error_handler(result->conn, DBI_ERROR_BADTYPE);
    return ERROR;
  }
  sizeattrib = _isolate_attrib(result->field_attribs[fieldidx], DBI_DECIMAL_SIZE4, DBI_DECIMAL_SIZE8);

  switch (sizeattrib) {
  case DBI_DECIMAL_SIZE4:
    return result->rows[result->currowidx]->field_values[fieldidx].d_float;
    break;
  case DBI_DECIMAL_SIZE8:
  default:
    _error_handler(result->conn, DBI_ERROR_BADTYPE);
    return ERROR;
  }
}

double dbi_result_get_double(dbi_result Result, const char *fieldname) {
  double ERROR = 0.0;
  unsigned int fieldidx;
  dbi_error_flag errflag;

  fieldidx = _find_field((dbi_result_t *)Result, fieldname, &errflag);
  if (errflag != DBI_ERROR_NONE) {
    dbi_conn_t *conn = ((dbi_result_t *)Result)->conn;
    _error_handler(conn, DBI_ERROR_BADNAME);
    return ERROR;
  }
  return dbi_result_get_double_idx(Result, fieldidx+1);
}
	
double dbi_result_get_double_idx(dbi_result Result, unsigned int fieldidx) {
  double ERROR = 0.0;
  dbi_result_t *result = Result;
  unsigned long sizeattrib;
  fieldidx--;
	
  result->conn->error_flag = DBI_ERROR_NONE;
	
  if (fieldidx >= result->numfields) {
    _error_handler(result->conn, DBI_ERROR_BADIDX);
    return ERROR;
  }
  if (result->field_types[fieldidx] != DBI_TYPE_DECIMAL) {
    _error_handler(result->conn, DBI_ERROR_BADTYPE);
    return ERROR;
  }
  sizeattrib = _isolate_attrib(result->field_attribs[fieldidx], DBI_DECIMAL_SIZE4, DBI_DECIMAL_SIZE8);

  switch (sizeattrib) {
  case DBI_DECIMAL_SIZE4:
  case DBI_DECIMAL_SIZE8:
    return result->rows[result->currowidx]->field_values[fieldidx].d_double;
    break;
  default:
    _error_handler(result->conn, DBI_ERROR_BADTYPE);
    return ERROR;
  }
}

const char *dbi_result_get_string(dbi_result Result, const char *fieldname) {
  const char *ERROR = "ERROR";
  unsigned int fieldidx;
  dbi_error_flag errflag;

  fieldidx = _find_field((dbi_result_t *)Result, fieldname, &errflag);
  if (errflag != DBI_ERROR_NONE) {
    dbi_conn_t *conn = ((dbi_result_t *)Result)->conn;
    _error_handler(conn, DBI_ERROR_BADNAME);
    return ERROR;
  }
  return dbi_result_get_string_idx(Result, fieldidx+1);
}
	
const char *dbi_result_get_string_idx(dbi_result Result, unsigned int fieldidx) {
  const char *ERROR = "ERROR";
  dbi_result_t *result = Result;
  fieldidx--;
	
  result->conn->error_flag = DBI_ERROR_NONE;
	
  if (fieldidx >= result->numfields) {
    _error_handler(result->conn, DBI_ERROR_BADIDX);
    return ERROR;
  }
  if (result->field_types[fieldidx] != DBI_TYPE_STRING) {
    _error_handler(result->conn, DBI_ERROR_BADTYPE);
    return ERROR;
  }
  if (result->rows[result->currowidx]->field_sizes[fieldidx] == 0) return NULL;
	
  return (const char *)(result->rows[result->currowidx]->field_values[fieldidx].d_string);
}

const unsigned char *dbi_result_get_binary(dbi_result Result, const char *fieldname) {
  const char *ERROR = "ERROR";
  unsigned int fieldidx;
  dbi_error_flag errflag;

  fieldidx = _find_field((dbi_result_t *)Result, fieldname, &errflag);
  if (errflag != DBI_ERROR_NONE) {
    dbi_conn_t *conn = ((dbi_result_t *)Result)->conn;
    _error_handler(conn, DBI_ERROR_BADNAME);
    return (const unsigned char*)ERROR;
  }
  return dbi_result_get_binary_idx(Result, fieldidx+1);
}
	
const unsigned char *dbi_result_get_binary_idx(dbi_result Result, unsigned int fieldidx) {
  const char *ERROR = "ERROR";
  dbi_result_t *result = Result;
  fieldidx--;
	
  result->conn->error_flag = DBI_ERROR_NONE;
	
  if (fieldidx >= result->numfields) {
    _error_handler(result->conn, DBI_ERROR_BADIDX);
    return (const unsigned char*)ERROR;
  }
  if (result->field_types[fieldidx] != DBI_TYPE_BINARY) {
    _error_handler(result->conn, DBI_ERROR_BADTYPE);
    return (const unsigned char*)ERROR;
  }
  if (result->rows[result->currowidx]->field_sizes[fieldidx] == 0) return NULL;

  return (const unsigned char *)(result->rows[result->currowidx]->field_values[fieldidx].d_string);
}

char *dbi_result_get_string_copy(dbi_result Result, const char *fieldname) {
  char *ERROR = "ERROR";
  unsigned int fieldidx;
  dbi_error_flag errflag;

  fieldidx = _find_field((dbi_result_t *)Result, fieldname, &errflag);
  if (errflag != DBI_ERROR_NONE) {
    dbi_conn_t *conn = ((dbi_result_t *)Result)->conn;
    _error_handler(conn, DBI_ERROR_BADNAME);
    return strdup(ERROR);
  }
  return dbi_result_get_string_copy_idx(Result, fieldidx+1);
}
	
char *dbi_result_get_string_copy_idx(dbi_result Result, unsigned int fieldidx) {
  char *ERROR = "ERROR";
  char *newstring = NULL;
  dbi_result_t *result = Result;
  fieldidx--;
	
  result->conn->error_flag = DBI_ERROR_NONE;
	
  if (fieldidx >= result->numfields) {
    _error_handler(result->conn, DBI_ERROR_BADIDX);
    return strdup(ERROR);
  }
  if (result->field_types[fieldidx] != DBI_TYPE_STRING) {
    _error_handler(result->conn, DBI_ERROR_BADTYPE);
    return strdup(ERROR);
  }
  if ((result->rows[result->currowidx]->field_sizes[fieldidx] == 0) && (result->rows[result->currowidx]->field_values[fieldidx].d_string == NULL)) {
    // mysql returns 0 for the field size of an empty string, so size==0
    // doesn't necessarily mean NULL
    return NULL;
  }

  newstring = strdup(result->rows[result->currowidx]->field_values[fieldidx].d_string);

  if (newstring) {
    return newstring;
  }
  else {
    _error_handler(result->conn, DBI_ERROR_NOMEM);
    return strdup(ERROR);
  }
}

unsigned char *dbi_result_get_binary_copy(dbi_result Result, const char *fieldname) {
  char *ERROR = "ERROR";
  unsigned int fieldidx;
  dbi_error_flag errflag;

  fieldidx = _find_field((dbi_result_t *)Result, fieldname, &errflag);
  if (errflag != DBI_ERROR_NONE) {
    dbi_conn_t *conn = ((dbi_result_t *)Result)->conn;
    _error_handler(conn, DBI_ERROR_BADNAME);
    return (unsigned char *)strdup(ERROR);
  }
  return dbi_result_get_binary_copy_idx(Result, fieldidx+1);
}
	
unsigned char *dbi_result_get_binary_copy_idx(dbi_result Result, unsigned int fieldidx) {
  char *ERROR = "ERROR";
  unsigned char *newblob = NULL;
  unsigned long long size;
  dbi_result_t *result = Result;
  fieldidx--;

  result->conn->error_flag = DBI_ERROR_NONE;
	
  if (fieldidx >= result->numfields) {
    _error_handler(result->conn, DBI_ERROR_BADIDX);
    return (unsigned char *)strdup(ERROR);
  }
  if (result->field_types[fieldidx] != DBI_TYPE_BINARY) {
    _error_handler(result->conn, DBI_ERROR_BADTYPE);
    return (unsigned char *)strdup(ERROR);
  }
  if (result->rows[result->currowidx]->field_sizes[fieldidx] == 0) return NULL;

  size = dbi_result_get_field_size_idx(Result, fieldidx);
  newblob = malloc(size);
  if (!newblob) {
    _error_handler(result->conn, DBI_ERROR_NOMEM);
    return (unsigned char *)strdup(ERROR);
  }
  memcpy(newblob, result->rows[result->currowidx]->field_values[fieldidx].d_string, size);
  return newblob;
}

time_t dbi_result_get_datetime(dbi_result Result, const char *fieldname) {
  time_t ERROR = 0;
  unsigned int fieldidx;
  dbi_error_flag errflag;

  fieldidx = _find_field((dbi_result_t *)Result, fieldname, &errflag);
  if (errflag != DBI_ERROR_NONE) {
    dbi_conn_t *conn = ((dbi_result_t *)Result)->conn;
    _error_handler(conn, DBI_ERROR_BADNAME);
    return ERROR;
  }
  return dbi_result_get_datetime_idx(Result, fieldidx+1);
}
	
time_t dbi_result_get_datetime_idx(dbi_result Result, unsigned int fieldidx) {
  time_t ERROR = 0;
  dbi_result_t *result = Result;
  fieldidx--;

  result->conn->error_flag = DBI_ERROR_NONE;
	
  if (fieldidx >= result->numfields) {
    _error_handler(result->conn, DBI_ERROR_BADIDX);
    return ERROR;
  }
  if (result->field_types[fieldidx] != DBI_TYPE_DATETIME) {
    _error_handler(result->conn, DBI_ERROR_BADTYPE);
    return ERROR;
  }
	
  return (time_t)(result->rows[result->currowidx]->field_values[fieldidx].d_datetime);
}

/* RESULT: bind_* functions */

static int _setup_binding(dbi_result_t *result, const char *fieldname, void *bindto, void *helperfunc) {
  _field_binding_t *binding;
  if (!result) {
    _error_handler(result->conn, DBI_ERROR_BADPTR);
    return DBI_BIND_ERROR;
  }
  else if (!fieldname) {
    _error_handler(result->conn, DBI_ERROR_BADNAME);
    return DBI_BIND_ERROR;
  }
  binding = _find_or_create_binding_node(result, fieldname);
  if (!binding) {
    _error_handler(result->conn, DBI_ERROR_NOMEM);
    return DBI_BIND_ERROR;
  }

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

int dbi_result_bind_int(dbi_result Result, const char *fieldname, int *bindto) {
  return _setup_binding((dbi_result_t *)Result, fieldname, bindto, _bind_helper_int);
}

int dbi_result_bind_long(dbi_result Result, const char *fieldname, int *bindto) {
  return dbi_result_bind_int(Result, fieldname, bindto);
}

int dbi_result_bind_uint(dbi_result Result, const char *fieldname, unsigned int *bindto) {
  return _setup_binding((dbi_result_t *)Result, fieldname, bindto, _bind_helper_uint);
}

int dbi_result_bind_ulong(dbi_result Result, const char *fieldname, unsigned int *bindto) {
  return dbi_result_bind_uint(Result, fieldname, bindto);
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
    binding = malloc(sizeof(_field_binding_t));
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

/* returns the field index (>= 1), or 0 if no such field */
static unsigned int _find_field(dbi_result_t *result, const char *fieldname, dbi_error_flag *errflag) {
  unsigned long i = 0;
  if (!result || !result->field_names) return DBI_FIELD_ERROR;
  while (i < result->numfields) {
    if (strcasecmp(result->field_names[i], fieldname) == 0) {
      *errflag = DBI_ERROR_NONE;
      return i;
    }
    i++;
  }
  *errflag = DBI_ERROR_BADNAME;
  return 0;
}

static int _is_row_fetched(dbi_result_t *result, unsigned long long row) {
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

static void _bind_helper_int(_field_binding_t *binding) {
  *(int *)binding->bindto = dbi_result_get_int((dbi_result)binding->result, binding->fieldname);
}

static void _bind_helper_uint(_field_binding_t *binding) {
  *(unsigned int *)binding->bindto = dbi_result_get_uint((dbi_result)binding->result, binding->fieldname);
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

static void _bind_helper_datetime(_field_binding_t *binding) {
  *(time_t *)binding->bindto = dbi_result_get_datetime((dbi_result)binding->result, binding->fieldname);
}

