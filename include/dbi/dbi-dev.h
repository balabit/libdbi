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

#ifndef __DBI_DEV_H__
#define __DBI_DEV_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <dbi/dbi.h> /* for dbi_conn_error_handler_func */

/*********************
 * SQL RELATED TYPES *
 *********************/

/* to fool the compiler into letting us use the following structs before they're actually defined: */
typedef struct dbi_driver_s *dbi_driver_t_pointer;
typedef struct dbi_conn_s *dbi_conn_t_pointer;
typedef struct _field_binding_s *_field_binding_t_pointer;

typedef union dbi_data_u {
	char d_char;
	short d_short;
	long d_long;
	long long d_longlong;
	float d_float;
	double d_double;
	char *d_string;
	time_t d_datetime;
} dbi_data_t;

typedef struct dbi_row_s {
	dbi_data_t *field_values;
	unsigned long long *field_sizes; /* NULL field = 0, string field = len, anything else = -1 */
									 /* XXX TODO: above is false as of 8/6/02. no -1 */
} dbi_row_t;

typedef struct dbi_result_s {
	dbi_conn_t_pointer conn;
	void *result_handle; /* will be typecast into conn-specific type */
	unsigned long long numrows_matched; /* set immediately after query */
	unsigned long long numrows_affected;
	_field_binding_t_pointer field_bindings;
	
	unsigned short numfields; /* can be zero or NULL until first fetchrow */
	char **field_names;
	unsigned short *field_types;
	unsigned long *field_attribs;

	enum { NOTHING_RETURNED, ROWS_RETURNED, GETTING_ROWS } result_state; /* nothing_returned: wasn't a SELECT query. returns_rows: select, but no rows fetched yet. getting_rows: select, at least one row has been fetched */
	dbi_row_t **rows; /* array of filled rows, elements set to NULL if not fetched yet */
	unsigned long long currowidx;
} dbi_result_t;

typedef struct _field_binding_s {
	void (*helper_function)(_field_binding_t_pointer);
	dbi_result_t *result;
	const char *fieldname;
	void *bindto;
	struct _field_binding_s *next;
} _field_binding_t;

/***************************************
 * DRIVER INFRASTRUCTURE RELATED TYPES *
 ***************************************/

typedef struct dbi_info_s {
	const char *name; /* all lowercase letters and numbers, no spaces */
	const char *description; /* one or two short sentences, no newlines */
	const char *maintainer; /* Full Name <fname@fooblah.com> */
	const char *url; /* where this driver came from (if maintained by a third party) */
	const char *version;
	const char *date_compiled;
} dbi_info_t;

typedef struct _capability_s {
	char *name;
	int value;
	struct _capability_s *next;
} _capability_t;

typedef struct dbi_option_s {
	char *key;
	char *string_value;
	int numeric_value; /* use this for port and other numeric settings */
	struct dbi_option_s *next;
} dbi_option_t;

typedef struct dbi_functions_s {
	void (*register_driver)(const dbi_info_t **, const char ***, const char ***);
	int (*initialize)(dbi_driver_t_pointer);
	int (*connect)(dbi_conn_t_pointer);
	int (*disconnect)(dbi_conn_t_pointer);
	int (*fetch_row)(dbi_result_t *, unsigned long long);
	int (*free_query)(dbi_result_t *);
	int (*goto_row)(dbi_result_t *, unsigned long long);
	int (*get_socket)(dbi_conn_t_pointer);
	dbi_result_t *(*list_dbs)(dbi_conn_t_pointer, const char *);
	dbi_result_t *(*list_tables)(dbi_conn_t_pointer, const char *, const char *);
	dbi_result_t *(*query)(dbi_conn_t_pointer, const char *);
	dbi_result_t *(*query_null)(dbi_conn_t_pointer, const unsigned char *, unsigned long);
	int (*quote_string)(dbi_driver_t_pointer, const char *, char *);
	char *(*select_db)(dbi_conn_t_pointer, const char *);
	int (*geterror)(dbi_conn_t_pointer, int *, char **);
	unsigned long long (*get_seq_last)(dbi_conn_t_pointer, const char *);
	unsigned long long (*get_seq_next)(dbi_conn_t_pointer, const char *);
} dbi_functions_t;

typedef struct dbi_custom_function_s {
	const char *name;
	void *function_pointer;
	struct dbi_custom_function_s *next;
} dbi_custom_function_t;

typedef struct dbi_driver_s {
	void *dlhandle;
	char *filename; /* full pathname */
	const dbi_info_t *info;
	dbi_functions_t *functions;
	dbi_custom_function_t *custom_functions;
	const char **reserved_words;
	_capability_t *caps;
	struct dbi_driver_s *next;
} dbi_driver_t;
	
typedef struct dbi_conn_s {
	dbi_driver_t *driver; /* generic unchanging attributes shared by all instances of this conn */
	dbi_option_t *options;
	_capability_t *caps;
	void *connection; /* will be typecast into conn-specific type */
	char *current_db;
	dbi_error_flag error_flag;
	int error_number; /*XXX*/
	char *error_message; /*XXX*/
	dbi_conn_error_handler_func error_handler;
	void *error_handler_argument;
	dbi_result_t **results; /* for garbage-collector-mandated result disjoins */
	int results_used;
	int results_size;
	struct dbi_conn_s *next; /* so libdbi can unload all conns at exit */
} dbi_conn_t;

unsigned long _isolate_attrib(unsigned long attribs, unsigned long rangemin, unsigned rangemax);
void _error_handler(dbi_conn_t *conn, dbi_error_flag errflag);
int _disjoin_from_conn(dbi_result_t *result);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* __DBI_DEV_H__ */
