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

#ifndef __DBI_MAIN_H__
#define __DBI_MAIN_H__

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 * SQL RELATED TYPES *
 *********************/

/* to fool the compiler into letting us use the following structs before they're actually defined: */
typedef struct dbi_plugin_s *dbi_plugin_t_pointer;
typedef struct dbi_driver_s *dbi_driver_t_pointer;
typedef struct _field_binding_s *_field_binding_t_pointer;

typedef union dbi_data_u {
	char d_char;
	short d_short;
	long d_long;
	long long d_longlong;
	float d_float;
	double d_double;
	char *d_string;
} dbi_data_t;

typedef struct dbi_row_s {
	dbi_data_t *field_values;
	unsigned int *field_sizes; /* only set for field indexes that are strings */
} dbi_row_t;

typedef struct dbi_result_s {
	dbi_driver_t_pointer driver;
	void *result_handle; /* will be typecast into driver-specific type */
	unsigned long numrows_matched; /* set immediately after query */
	unsigned long numrows_affected;
	_field_binding_t_pointer field_bindings;
	
	unsigned int numfields; /* can be zero or NULL until first fetchrow */
	char **field_names;
	unsigned short *field_types;
	unsigned int *field_attribs;

	dbi_row_t **rows; /* array of filled rows, elements set to NULL if not fetched yet */
	unsigned long currowidx;
} dbi_result_t;

typedef struct _field_binding_s {
	void (*helper_function)(_field_binding_t_pointer);
	dbi_result_t *result;
	const char *fieldname;
	void *bindto;
	struct _field_binding_s *next;
} _field_binding_t;

/***************************************
 * PLUGIN INFRASTRUCTURE RELATED TYPES *
 ***************************************/

typedef struct dbi_info_s {
	const char *name; /* all lowercase letters and numbers, no spaces */
	const char *description; /* one or two short sentences, no newlines */
	const char *maintainer; /* Full Name <fname@fooblah.com> */
	const char *url; /* where this plugin came from (if maintained by a third party) */
	const char *version;
	const char *date_compiled;
} dbi_info_t;

typedef struct dbi_option_s {
	char *key;
	char *string_value;
	int numeric_value; /* use this for port and other numeric settings */
	struct dbi_option_s *next;
} dbi_option_t;

typedef struct dbi_functions_s {
	int (*initialize)(dbi_plugin_t_pointer);
	int (*connect)(dbi_driver_t_pointer);
	int (*disconnect)(dbi_driver_t_pointer);
	int (*fetch_row)(dbi_result_t *);
	int (*free_query)(dbi_result_t *);
	const char **(*get_custom_functions_list)();
	const dbi_info_t *(*get_info)();
	const char **(*get_reserved_words_list)();
	int (*goto_row)(dbi_result_t *, unsigned int);
	dbi_result_t *(*list_dbs)(dbi_driver_t_pointer);
	dbi_result_t *(*list_tables)(dbi_driver_t_pointer, const char *);
	dbi_result_t *(*query)(dbi_driver_t_pointer, const char *);
	int (*select_db)(dbi_driver_t_pointer, const char *);
	const char *(*errstr)(dbi_driver_t_pointer);
	int (*errno)(dbi_driver_t_pointer);
} dbi_functions_t;

typedef struct dbi_custom_function_s {
	const char *name;
	void *function_pointer;
	struct dbi_custom_function_s *next;
} dbi_custom_function_t;

typedef struct dbi_plugin_s {
	void *dlhandle;
	char *filename; /* full pathname */
	const dbi_info_t *info;
	dbi_functions_t *functions;
	dbi_custom_function_t *custom_functions;
	const char **reserved_words;
	struct dbi_plugin_s *next;
} dbi_plugin_t;
	
typedef struct dbi_driver_s {
	dbi_plugin_t *plugin; /* generic unchanging attributes shared by all instances of this driver */
	dbi_option_t *options;
	void *connection; /* will be typecast into driver-specific type */
	char *current_db;
	int error_number; /*XXX*/
	char *error_message; /*XXX*/
	void *error_handler;
	void *error_handler_argument;
	struct dbi_driver_s *next; /* so libdbi can unload all drivers at exit */
} dbi_driver_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* __DBI_MAIN_H__ */
