/*
 * libdbi - database independent abstraction layer for C.
 * Copyright (C) 2001, Brentwood Linux Users and Evangelists (BLUE).
 * Copyright (C) David Parker and Mark Tobenkin.
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

#ifndef __DBI_H__
#define __DBI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

	
/*********************
 * SQL RELATED TYPES *
 *********************/

typedef struct dbi_row_s {
	void *row_handle; /* will be typecast into driver-specific type */
	unsigned int numfields;
	const char **field_names;
	enum field_types[] { DBI_TYPE_INTEGER, DBI_TYPE_DECIMAL, DBI_TYPE_STRING, DBI_TYPE_BINARY };
	unsigned int field_type_attributes[];
	void *field_values[];
	struct dbi_row_s *next; /* NULL, unless we slurp all available rows at once */
} dbi_row_t;

typedef struct dbi_result_s {
	dbi_driver_t *driver; /* to link upwards to the parent driver node */
	void *result_handle; /* will be typecast into driver-specific type */
	unsigned long numrows_matched;
	unsigned long numrows_changed; /* not all servers differentiate rows changed from rows matched, so this may be zero */
	dbi_row_t *row;
} dbi_result_t; 

/* values for the bitmask in field_type_attributes[] */
#define DBI_INTEGER_UNSIGNED 1
#define DBI_INTEGER_SIZE1 2
#define DBI_INTEGER_SIZE2 4
#define DBI_INTEGER_SIZE3 8
#define DBI_INTEGER_SIZE4 16
#define DBI_INTEGER_SIZE8 32

#define DBI_DECIMAL_UNSIGNED 1
#define DBI_DECIMAL_SIZE4 2
#define DBI_DECIMAL_SIZE8 4


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
	/* for unchanging connection parameters such as username, password, hostname, port, etc */
	char *key;
	char *string_value;
	int numeric_value; /* use this for port and other numeric settings */
	struct dbi_option_s *next;
} dbi_option_t;

typedef struct dbi_functions_s {
	/* common function declarations */
	int (*initialize)(dbi_plugin_t *);
	int (*connect)(dbi_driver_t *);
	int (*disconnect)(dbi_driver_t *);
	int (*fetch_field)(dbi_result_t *, const char *, void *);
	int (*fetch_field_raw)(dbi_result_t *, const char *, unsigned char *);
	int (*fetch_row)(dbi_result_t *);
	int (*free_query)(dbi_result_t *);
	static const char **(*get_custom_functions_list)();
	static const dbi_info_t *(*get_info)();
	static const char **(*get_reserved_words_list)();
	int (*goto_row)(dbi_result_t *, unsigned int);
	const char **(*list_dbs)(dbi_driver_t *);
	const char **(*list_tables)(dbi_driver_t *, const char *);
	unsigned int (*num_rows)(dbi_result_t *);
	unsigned int (*num_rows_affected)(dbi_result_t *);
	dbi_result_t *(*query)(dbi_driver_t *, const char *);
	dbi_result_t *(*efficient_query)(dbi_driver_t *, const char *);
	int (*select_db)(dbi_driver_t *, const char *);
	const char *(*errstr)(dbi_driver_t *);
	int (*errno)(dbi_driver_t *);
} dbi_functions_t;

typedef struct dbi_custom_function_s {
	/* for driver-specific functions. using this will obviously restrict your code to one specific database. */
	const char *name;
	void *function_pointer;
	struct dbi_extra_function_s *next;
} dbi_custom_function_t;

typedef struct dbi_plugin_s {
	void *dlhandle;
	const char *filename; /* full pathname */
	dbi_info_t *info;
	dbi_functions_t *functions;
	dbi_custom_function_t *custom_functions;
	const char **reserved_words;
	struct dbi_plugin_s *next;
} dbi_plugin_t;
	
typedef struct dbi_driver_s {
	/* specific instance of a driver -- we can initialize and simultaneously use multiple connections from the same database driver (on different tables or hosts, unless you wanna fubar your data) */
	dbi_plugin_t *plugin; /* generic unchanging attributes shared by all instances of this driver */
	dbi_option_t *options;
	/* dbi_result_t *result; ---- this should be stored by the host program, not the driver. there can be more than one valid result handle at a time */
	void *connection; /* will be typecast into driver-specific type */
	char *current_db;
	int status; /* unused, as of now */
	int error_number;
	char *error_message;
	void *error_handler;
	void *error_handler_argument;
} dbi_driver_t;


/***********************************
 * PLUGIN INFRASTRUCTURE FUNCTIONS *
 ***********************************/

int dbi_initialize(const char *plugindir); /* locates the shared modules and dlopen's everything */
dbi_plugin_t *dbi_list_plugins(); /* returns first plugin, program has to travel linked list until it hits a NULL */
dbi_plugin_t *dbi_open_plugin(const char *name); /* goes thru linked list until it finds the right one */
dbi_driver_t *dbi_load_driver(const char *name); /* returns an actual instance of the driver. shortcut for finding the right plugin then calling dbi_start_driver */
dbi_driver_t *dbi_start_driver(const dbi_plugin_t *plugin); /* returns an actual instance of the driver */
void dbi_set_option(dbi_driver_t *driver, const char *key, char *value); /* if value is NULL, remove option from list */
void dbi_set_option_numeric(dbi_driver_t *driver, const char *key, int value);
const char *dbi_get_option(dbi_driver_t *driver, const char *key);
int dbi_get_option_numeric(dbi_driver_t *driver, const char *key);
void dbi_clear_options(dbi_driver_t *driver);
void *dbi_custom_function(dbi_plugin_t *plugin, const char *name);
int dbi_is_reserved_word(dbi_plugin_t *plugin, const char *word);
void dbi_close_driver(dbi_driver_t *driver); /* disconnects database link and cleans up the driver's session */
void dbi_shutdown();

const char *dbi_plugin_name(dbi_plugin_t *plugin);
const char *dbi_plugin_filename(dbi_plugin_t *plugin);
const char *dbi_plugin_description(dbi_plugin_t *plugin);
const char *dbi_plugin_maintainer(dbi_plugin_t *plugin);
const char *dbi_plugin_url(dbi_plugin_t *plugin);
const char *dbi_plugin_version(dbi_plugin_t *plugin);
const char *dbi_plugin_date_compiled(dbi_plugin_t *plugin);
const char *dbi_version();


/***********************
 * SQL LAYER FUNCTIONS *
 ***********************/

int dbi_connect(dbi_driver_t *driver); /* host and login info already stored in driver's info table */
/* int dbi_disconnect(dbi_driver_t *driver); ---- this will be done automatically by dbi_close_driver() */
int dbi_fetch_field(dbi_result_t *result, const char *key, void *dest);
int dbi_fetch_field_raw(dbi_result_t *result, const char *key, unsigned char *binary_dest); /* doesn't autodetect field types. should this stay here? */
int dbi_fetch_row(dbi_result_t *result);
int dbi_free_query(dbi_result_t *result);
int dbi_goto_row(dbi_result_t *result, unsigned int row);
const char **dbi_list_dbs(dbi_driver_t *driver);
const char **dbi_list_tables(dbi_driver_t *driver, const char *db);
unsigned int dbi_num_rows(dbi_result_t *result); /* number of rows in result set */
unsigned int dbi_num_rows_affected(dbi_result_t *result); /* only the rows in the result set that were actually modified */
dbi_result_t *dbi_query(dbi_driver_t *driver, const char *formatstr, ...); /* dynamic num of arguments, a la printf */
dbi_result_t *dbi_efficient_query(dbi_driver_t *driver, const char *formatstr, ...); /* better name instead of efficient_query? this will only request one row at a time, but has the downside that other queries can't be made until this one is closed. at least that's how it works in mysql, so it has to be the common denominator */
int dbi_select_db(dbi_driver_t *driver, const char *db);

int *dbi_error(dbi_driver_t *driver, char *errmsg_dest); /* returns formatted message with the error number and string */
void dbi_error_handler(dbi_driver_t *driver, void *function, void *user_argument); /* registers a callback that's activated when the database encounters an error */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* __DBI_H__ */
